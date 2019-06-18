open! Stdune
open Import
open Fiber.O

type job =
  { pid  : int
  ; ivar : Unix.process_status Fiber.Ivar.t
  }

module Signal = struct
  type t = Int | Quit | Term
  let compare : t -> t -> Ordering.t = compare

  module Set = Set.Make(struct
      type nonrec t = t
      let compare = compare
    end)

  let all = [Int; Quit; Term]

  let to_int = function
    | Int -> Sys.sigint
    | Quit -> Sys.sigquit
    | Term -> Sys.sigterm

  let of_int =
    List.map all ~f:(fun t -> to_int t, t)
    |> Int.Map.of_list_reduce ~f:(fun _ t -> t)
    |> Int.Map.find

  let name t = Utils.signal_name (to_int t)
end

module Thread = struct
  include Thread

  let block_signals = lazy (
    let signos = List.map Signal.all ~f:Signal.to_int in
    ignore (Unix.sigprocmask SIG_BLOCK signos : int list)
  )

  let create =
    if Sys.win32 then
      Thread.create
    else
      (* On unix, we make sure to block signals globally before
         starting a thread so that only the signal watcher thread can
         receive signals. *)
      fun f x ->
        Lazy.force block_signals;
        Thread.create f x
end

(** The event queue *)
module Event : sig
  type t =
    | Files_changed
    | Job_completed of job * Unix.process_status
    | Signal of Signal.t

  (** Return the next event. File changes event are always flattened
      and returned first. *)
  val next : unit -> t

  (** Ignore the ne next file change event about this file. *)
  val ignore_next_file_change_event : Path.t -> unit

  (** Register the fact that a job was started. *)
  val register_job_started : unit -> unit

  (** Number of jobs for which the status hasn't been reported yet .*)
  val pending_jobs : unit -> int

  (** Send an event to the main thread. *)
  val send_files_changed : Path.t list -> unit
  val send_job_completed : job -> Unix.process_status -> unit
  val send_signal : Signal.t -> unit
end = struct
  type t =
    | Files_changed
    | Job_completed of job * Unix.process_status
    | Signal of Signal.t

  let jobs_completed = Queue.create ()
  let files_changed = ref []
  let signals = ref Signal.Set.empty
  let mutex = Mutex.create ()
  let cond = Condition.create ()

  let ignored_files = String.Table.create 64
  let pending_jobs = ref 0

  let register_job_started () = incr pending_jobs

  let ignore_next_file_change_event path =
    assert (Path.is_in_source_tree path);
    String.Table.replace
      ignored_files
      ~key:(Path.to_absolute_filename path)
      ~data:()

  let available () =
    not (List.is_empty !files_changed
         && Queue.is_empty jobs_completed
         && Signal.Set.is_empty !signals)

  let next () =
    Stats.record ();
    Mutex.lock mutex;
    let rec loop () =
      while not (available ()) do Condition.wait cond mutex done;
      match Signal.Set.choose !signals with
      | Some signal ->
        signals := Signal.Set.remove !signals signal;
        Signal signal
      | None ->
        match !files_changed with
        | [] ->
          let (job, status) = Queue.pop jobs_completed in
          decr pending_jobs;
          Job_completed (job, status)
        | fns ->
          files_changed := [];
          let only_ignored_files =
            List.fold_left fns ~init:true ~f:(fun acc fn ->
              let fn = Path.to_absolute_filename fn in
              if String.Table.mem ignored_files fn then begin
                (* only use ignored record once *)
                String.Table.remove ignored_files fn;
                acc
              end else
                false)
          in
          if only_ignored_files then
            loop ()
          else
            Files_changed
    in
    let ev = loop () in
    Mutex.unlock mutex;
    ev

  let send_files_changed files =
    Mutex.lock mutex;
    let avail = available () in
    files_changed := List.rev_append files !files_changed;
    if not avail then Condition.signal cond;
    Mutex.unlock mutex

  let send_job_completed job status =
    Mutex.lock mutex;
    let avail = available () in
    Queue.push (job, status) jobs_completed;
    if not avail then Condition.signal cond;
    Mutex.unlock mutex

  let send_signal signal =
    Mutex.lock mutex;
    let avail = available () in
    signals := Signal.Set.add !signals signal;
    if not avail then Condition.signal cond;
    Mutex.unlock mutex

  let pending_jobs () = !pending_jobs
end

let ignore_for_watch = Event.ignore_next_file_change_event

module File_watcher : sig
  type t

  (** Create a new file watcher. *)
  val create : unit -> t

  (** Pid of the external file watcher process *)
  val pid : t -> int
end = struct
  type t = int

  let pid t = t

  let command =
    lazy (
      let excludes = [ {|/_build|}
                     ; {|/_opam|}
                     ; {|/_esy|}
                     ; {|/\..+|}
                     ; {|~$|}
                     ; {|/#[^#]*#$|}
                     ; {|4913|} (* https://github.com/neovim/neovim/issues/3460 *)
                     ]
      in
      let path = Path.to_string_maybe_quoted Path.root in
      match Bin.which ~path:(Env.path Env.initial) "inotifywait" with
      | Some inotifywait ->
        (* On Linux, use inotifywait. *)
        let excludes = String.concat ~sep:"|" excludes in
        inotifywait, [
          "-r"; path;
          "--exclude"; excludes;
          "-e"; "close_write"; "-e"; "delete";
          "--format"; "%w%f";
          "-m";
          "-q"]
      | None ->
        (* On all other platforms, try to use fswatch. fswatch's event
           filtering is not reliable (at least on Linux), so don't try to
           use it, instead act on all events. *)
        (match Bin.which ~path:(Env.path Env.initial) "fswatch" with
         | Some fswatch ->
           let excludes =
             List.concat_map excludes ~f:(fun x -> ["--exclude"; x])
           in
           fswatch, [
             "-r"; path;
             "--event"; "Created";
             "--event"; "Updated";
             "--event"; "Removed"] @ excludes
         | None ->
           die "@{<error>Error@}: fswatch (or inotifywait) was not found. \
                One of them needs to be installed for watch mode to work.\n"))

  let buffering_time = 0.5 (* seconds *)
  let buffer_capacity = 65536

  type buffer =
    { data : Bytes.t
    ; mutable size : int
    }

  let read_lines buffer fd =
    let len =
      Unix.read fd buffer.data buffer.size (buffer_capacity - buffer.size)
    in
    buffer.size <- buffer.size + len;
    let lines = ref [] in
    let line_start = ref 0 in
    for i = 0 to buffer.size - 1 do
      let c = Bytes.get buffer.data i in
      if c = '\n' || c = '\r' then begin
        if !line_start < i then begin
          let line = Bytes.sub_string
                       buffer.data
                       ~pos:!line_start
                       ~len:(i - !line_start) in
          lines := line :: !lines;
        end;
        line_start := i + 1
      end
    done;
    buffer.size <- buffer.size - !line_start;
    Bytes.blit
      ~src:buffer.data ~src_pos:!line_start
      ~dst:buffer.data ~dst_pos:0
      ~len:buffer.size;
    List.rev !lines

  let spawn_external_watcher () =
    let prog, args = Lazy.force command in
    let prog = Path.to_absolute_filename prog in
    let argv = prog :: args in
    let r, w = Unix.pipe () in
    let pid =
      Spawn.spawn ()
        ~prog
        ~argv
        ~stdout:w
    in
    Unix.close w;
    (r, pid)

  let create () =
    let files_changed = ref [] in
    let event_mtx = Mutex.create () in
    let event_cv = Condition.create () in

    let worker_thread pipe =
      let buffer =
        { data = Bytes.create buffer_capacity
        ; size = 0
        }
      in
      while true do
        let lines = List.map (read_lines buffer pipe) ~f:Path.of_string in
        Mutex.lock event_mtx;
        files_changed := List.rev_append lines !files_changed;
        Condition.signal event_cv;
        Mutex.unlock event_mtx;
      done
    in

    (* The buffer thread is used to avoid flooding the main thread
       with file changes events when a lot of file changes are reported
       at once. In particular, this avoids restarting the build over
       and over in a short period of time when many events are
       reported at once.

       It works as follow:

       - when the first event is received, send it to the main thread
       immediately so that we get a fast response time

       - after the first event is received, buffer subsequent events
       for [buffering_time]
    *)
    let rec buffer_thread () =
      Mutex.lock event_mtx;
      while List.is_empty !files_changed do
        Condition.wait event_cv event_mtx
      done;
      let files = !files_changed in
      files_changed := [];
      Mutex.unlock event_mtx;
      Event.send_files_changed files;
      Thread.delay buffering_time;
      buffer_thread ()
    in

    let pipe, pid = spawn_external_watcher () in

    ignore (Thread.create worker_thread pipe : Thread.t);
    ignore (Thread.create buffer_thread () : Thread.t);

    pid
end

module Process_watcher : sig
  (** Initialize the process watcher thread. *)
  val init : unit -> unit

  (** Register a new running job. *)
  val register_job : job -> unit

  (** Send the following signal to all running processes. *)
  val killall : int -> unit
end = struct
  let mutex = Mutex.create ()
  let something_is_running_cv = Condition.create ()

  module Process_table : sig
    val add : job -> unit
    val remove : pid:int -> Unix.process_status -> unit
    val running_count : unit -> int
    val iter : f:(job -> unit) -> unit
  end = struct
    type process_state =
      | Running of job
      | Zombie of Unix.process_status

    (* Invariant: [!running_count] is equal to the number of
       [Running _] values in [table]. *)
    let table = Hashtbl.create 128
    let running_count = ref 0

    let add job =
      match Hashtbl.find table job.pid with
      | None ->
        Hashtbl.add table job.pid (Running job);
        incr running_count;
        if !running_count = 1 then Condition.signal something_is_running_cv
      | Some (Zombie status) ->
        Hashtbl.remove table job.pid;
        Event.send_job_completed job status
      | Some (Running _) ->
        assert false

    let remove ~pid status =
      match Hashtbl.find table pid with
      | None ->
        Hashtbl.add table pid (Zombie status)
      | Some (Running job) ->
        decr running_count;
        Hashtbl.remove table pid;
        Event.send_job_completed job status
      | Some (Zombie _) ->
        assert false

    let iter ~f =
      Hashtbl.iter table ~f:(fun ~key:_ ~data ->
        match data with
        | Running job -> f job
        | Zombie _ -> ())

    let running_count () = !running_count
  end

  let register_job job =
    Event.register_job_started ();
    Mutex.lock mutex;
    Process_table.add job;
    Mutex.unlock mutex

  let killall signal =
    Mutex.lock mutex;
    Process_table.iter ~f:(fun job ->
      try
        Unix.kill job.pid signal
      with Unix.Unix_error _ -> ());
    Mutex.unlock mutex

  exception Finished of job * Unix.process_status

  let wait_nonblocking_win32 () =
    try
      Process_table.iter ~f:(fun job ->
        let pid, status = Unix.waitpid [WNOHANG] job.pid in
        if pid <> 0 then
          raise_notrace (Finished (job, status)));
      false
    with Finished (job, status) ->
      (* We need to do the [Unix.waitpid] and remove the process while
         holding the lock, otherwise the pid might be reused in
         between. *)
      Process_table.remove ~pid:job.pid status;
      true

  let wait_win32 () =
    while not (wait_nonblocking_win32 ()) do
      Mutex.unlock mutex;
      ignore (Unix.select [] [] [] 0.001);
      Mutex.lock mutex
    done

  let wait_unix () =
    Mutex.unlock mutex;
    let pid, status = Unix.wait () in
    Mutex.lock mutex;
    Process_table.remove ~pid status

  let wait =
    if Sys.win32 then
      wait_win32
    else
      wait_unix

  let run () =
    Mutex.lock mutex;
    while true do
      while Process_table.running_count () = 0 do
        Condition.wait something_is_running_cv mutex
      done;
      wait ()
    done

  let init = lazy (ignore (Thread.create run () : Thread.t))

  let init () = Lazy.force init
end

module Signal_watcher : sig
  val init : unit -> unit
end = struct

  let signos = List.map Signal.all ~f:Signal.to_int

  let warning = {|

**************************************************************
* Press Control+C again quickly to perform an emergency exit *
**************************************************************

|}

  external sys_exit : int -> _ = "caml_sys_exit"

  let signal_waiter () =
    if Sys.win32 then begin
      let r, w = Unix.pipe () in
      let buf = Bytes.create 1 in
      Sys.set_signal Sys.sigint
        (Signal_handle (fun _ -> assert (Unix.write w buf 0 1 = 1)));
      Staged.stage (fun () ->
        assert (Unix.read r buf 0 1 = 1);
        Signal.Int)
    end else
      Staged.stage (fun () ->
        Thread.wait_signal signos
        |> Signal.of_int
        |> Option.value_exn)

  let run () =
    let last_exit_signals = Queue.create () in
    let wait_signal = Staged.unstage (signal_waiter ()) in
    while true do
      let signal = wait_signal () in
      Event.send_signal signal;
      match signal with
      | Int | Quit | Term ->
        let now = Unix.gettimeofday () in
        Queue.push now last_exit_signals;
        (* Discard old signals *)
        while Queue.length last_exit_signals >= 0 &&
              now -. Queue.peek last_exit_signals > 1.
        do
          ignore (Queue.pop last_exit_signals : float)
        done;
        let n = Queue.length last_exit_signals in
        if n = 2 then prerr_endline warning;
        if n = 3 then sys_exit 1
    done

  let init = lazy (ignore (Thread.create run () : Thread.t))

  let init () = Lazy.force init
end

type t =
  { log                        : Log.t
  ; original_cwd               : string
  ; mutable concurrency        : int
  ; waiting_for_available_job  : t Fiber.Ivar.t Queue.t
  }

let log t = t.log

let with_chdir t ~dir ~f =
  Sys.chdir (Path.to_string dir);
  protectx () ~finally:(fun () -> Sys.chdir t.original_cwd) ~f

let t_var : t Fiber.Var.t = Fiber.Var.create ()

let update_status_line () =
  Console.update_status_line ~running_jobs:(Event.pending_jobs ())

let set_status_line_generator gen =
  Console.set_status_line_generator ~running_jobs:(Event.pending_jobs ()) gen

let set_concurrency n =
  let t = Fiber.Var.get_exn t_var in
  t.concurrency <- n

let wait_for_available_job () =
  let t = Fiber.Var.get_exn t_var in
  if Event.pending_jobs () < t.concurrency then
    Fiber.return t
  else begin
    let ivar = Fiber.Ivar.create () in
    Queue.push ivar t.waiting_for_available_job;
    Fiber.Ivar.read ivar
  end

let wait_for_process pid =
  let ivar = Fiber.Ivar.create () in
  Process_watcher.register_job { pid; ivar };
  Fiber.Ivar.read ivar

let rec restart_waiting_for_available_job t =
  if Queue.is_empty t.waiting_for_available_job ||
     Event.pending_jobs () >= t.concurrency then
    Fiber.return ()
  else begin
    let ivar = Queue.pop t.waiting_for_available_job in
    let* () = Fiber.Ivar.fill ivar t in
    restart_waiting_for_available_job t
  end

let got_signal t signal =
  if Console.display () = Verbose then
    Log.infof t.log "Got signal %s, exiting." (Signal.name signal)

type saw_signal =
  | Ok
  | Got_signal

let kill_and_wait_for_all_processes t () =
  Queue.clear t.waiting_for_available_job;
  Process_watcher.killall Sys.sigkill;
  let saw_signal = ref Ok in
  while Event.pending_jobs () > 0 do
    match Event.next () with
    | Signal signal ->
      got_signal t signal;
      saw_signal := Got_signal;
    | _ -> ()
  done;
  !saw_signal

let prepare ?(log=Log.no_log) ?(config=Config.default) () =
  Console.init config.Config.display;
  Log.infof log "Workspace root: %s"
    (Path.to_absolute_filename Path.root |> String.maybe_quoted);
  (* The signal watcher must be initialized first so that signals are
     blocked in all threads. *)
  Signal_watcher.init ();
  Process_watcher.init ();
  let cwd = Sys.getcwd () in
  if cwd <> initial_cwd && not !Clflags.no_print_directory then
    Printf.eprintf "Entering directory '%s'\n%!"
      (if Config.inside_dune then
         let descendant_simple p ~of_ =
           match
             String.drop_prefix p ~prefix:of_
           with
           | None | Some "" -> None
           | Some s -> Some (String.drop s 1)
         in
         match descendant_simple cwd ~of_:initial_cwd with
         | Some s -> s
         | None ->
           match descendant_simple initial_cwd ~of_:cwd with
           | None -> cwd
           | Some s ->
             let rec loop acc dir =
               if dir = Filename.current_dir_name then
                 acc
               else
                 loop (Filename.concat acc "..") (Filename.dirname dir)
             in
             loop ".." (Filename.dirname s)
       else
         cwd);
  let t =
    { log
    ; original_cwd = cwd
    ; concurrency  = (match config.concurrency with Auto -> 1 | Fixed n -> n)
    ; waiting_for_available_job = Queue.create ()
    }
  in
  t

module Run_once : sig

  type run_error =
    | Got_signal
    | Files_changed
    | Never
    | Exn of Exn.t * Printexc.raw_backtrace

  (** Run the build and clean up after it (kill any stray processes etc). *)
  val run_and_cleanup :
    t ->
    (unit -> 'a Fiber.t) ->
    ('a, run_error) Result.t

end = struct

  type pump_events_result =
    | Done
    | Got_signal
    | Files_changed

  let rec pump_events t =
    let* () = Fiber.yield () in
    let count = Event.pending_jobs () in
    if count = 0 then begin
      Console.hide_status_line ();
      Fiber.return Done
    end else begin
      update_status_line ();
      begin
        match Event.next () with
        | Job_completed (job, status) ->
          let* () = Fiber.Ivar.fill job.ivar status in
          let* () = restart_waiting_for_available_job t in
          pump_events t
        | Files_changed ->
          Fiber.return Files_changed
        | Signal signal ->
          got_signal t signal;
          Fiber.return Got_signal
      end
    end

  type run_error =
    | Got_signal
    | Files_changed
    | Never
    | Exn of Exn.t * Printexc.raw_backtrace

  let run t f =
    let fiber =
      Fiber.Var.set t_var t
        (fun () -> Fiber.with_error_handler f ~on_error:Report_error.report)
    in
    match Fiber.run
            (let* user_action_result = Fiber.fork (fun () -> fiber) in
             let* pump_events_result = pump_events t in
             Fiber.return (pump_events_result, user_action_result)) with
    | exception Fiber.Never ->
      Exn.code_error "[Scheduler.pump_events] got stuck somehow" []
    | exception exn ->
      Error (Exn (exn, (Printexc.get_raw_backtrace ())))
    | (a, b) ->
      let b = Fiber.Future.peek b in
      match (a, b) with
      | Done, None ->
        Error Never
      | Done, Some res ->
        Ok res
      | Got_signal, _ -> Error Got_signal
      | Files_changed, _ -> Error Files_changed

  let run_and_cleanup t f =
    let res = run t f in
    (match res with
     | Error Files_changed ->
       set_status_line_generator
         (fun () ->
            { message = Some "Had errors, killing current build..."
            ; show_jobs = false
            })
     | _ -> ());
    match kill_and_wait_for_all_processes t () with
    | Got_signal ->
      Error Got_signal
    | Ok ->
      res
end

let go ?log ?config f =
  let t = prepare ?log ?config () in
  match
    Run_once.run_and_cleanup t f
  with
  | Error (Exn (exn, bt)) ->
    Exn.raise_with_backtrace exn bt
  | Ok res ->
    res
  | Error Got_signal ->
    raise Already_reported
  | Error Never ->
    raise Fiber.Never
  | Error Files_changed ->
    Exn.code_error
      "Scheduler.go: files changed even though we're running without filesystem watcher"
      []

type exit_or_continue = Exit | Continue

let poll ?log ?config ~once ~finally () =
  let t = prepare ?log ?config () in
  let watcher = File_watcher.create () in
  let block_waiting_for_changes () =
    match Event.next () with
    | Job_completed _ -> assert false
    | Files_changed -> Continue
    | Signal signal ->
      got_signal t signal;
      Exit
  in
  let wait msg =
    let old_generator = Console.get_status_line_generator () in
    set_status_line_generator
      (fun () ->
         { message = Some (msg ^ ", waiting for filesystem changes...")
         ; show_jobs = false
         });
    let res = block_waiting_for_changes () in
    set_status_line_generator old_generator;
    res
  in
  let rec loop () =
    let res = Run_once.run_and_cleanup t once in
    finally ();
    match res with
    | Ok () ->
      wait (Colors.apply_string Colors.command_success "Success") |> after_wait
    | Error Got_signal ->
      (Already_reported, None)
    | Error Never ->
      wait (Colors.apply_string Colors.command_error "Had errors") |> after_wait
    | Error Files_changed ->
      loop ()
    | Error (Exn (exn, bt)) -> (exn, Some bt)
  and
    after_wait = function
    | Exit -> (Already_reported, None)
    | Continue -> loop ()
  in
  let exn, bt = loop () in
  ignore (wait_for_process (File_watcher.pid watcher) : _ Fiber.t);
  ignore (kill_and_wait_for_all_processes t () : saw_signal);
  match bt with
  | None -> Exn.raise exn
  | Some bt -> Exn.raise_with_backtrace exn bt
