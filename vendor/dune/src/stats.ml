open Stdune

module Fd_count = struct
  type t = Unknown | This of int

  let try_to_use_lsof () =
    (* note: we do not use the Process module here, because it would
       create a circular dependency *)
    let temp = Filename.temp_file "dune" ".lsof" in
    let stdout =
      Unix.openfile temp [O_WRONLY; O_CREAT; O_TRUNC; O_SHARE_DELETE] 0o666
    in
    let prog = "/usr/sbin/lsof" in
    let argv = [prog; "-w"; "-p"; string_of_int (Unix.getpid ())] in
    let pid = Spawn.spawn ~prog ~argv ~stdout () in
    Unix.close stdout;
    match Unix.waitpid [] pid with
    | _, Unix.WEXITED 0 ->
      let num_lines = List.length (Io.input_lines (open_in temp)) in
      This (num_lines - 1) (* the output contains a header line *)
    | _ -> Unknown

  let get () =
    match Sys.readdir "/proc/self/fd" with
    | exception _ ->
      begin match try_to_use_lsof () with
      | exception _ -> Unknown
      | value -> value
      end
    | files -> This (Array.length files - 1 (* -1 for the dirfd *))
end

let catapult = ref None

let record () =
  Option.iter !catapult ~f:(fun reporter ->
    Catapult.emit_gc_counters reporter;
    match Fd_count.get () with
    | This fds ->
      Catapult.emit_counter reporter "fds" fds
    | Unknown -> ()
  )

let enable path =
  let reporter = Catapult.make path in
  catapult := Some reporter;
  at_exit (fun () -> Catapult.close reporter)

let with_process ~program ~args fiber =
  match !catapult with
  | None -> fiber
  | Some reporter ->
    let open Fiber.O in
    let event = Catapult.on_process_start reporter ~program ~args in
    let+ result = fiber in
    Catapult.on_process_end reporter event;
    result
