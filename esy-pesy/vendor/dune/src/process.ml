open! Stdune
open Import
open Fiber.O

type accepted_codes =
  | These of int list
  | All

let code_is_ok accepted_codes n =
  match accepted_codes with
  | These set -> List.mem n ~set
  | All -> true

type ('a, 'b) failure_mode =
  | Strict : ('a, 'a) failure_mode
  | Accept : accepted_codes -> ('a, ('a, int) result) failure_mode

let accepted_codes : type a b. (a, b) failure_mode -> accepted_codes = function
  | Strict -> These [0]
  | Accept (These codes) -> These (0 :: codes)
  | Accept All -> All

let map_result
  : type a b. (a, b) failure_mode -> int Fiber.t -> f:(unit -> a) -> b Fiber.t
  = fun mode t ~f ->
    match mode with
    | Strict   -> t >>| fun _ -> f ()
    | Accept _ ->
      t >>| function
      | 0 -> Ok (f ())
      | n -> Error n

module Output = struct
  type t =
    { kind     : kind
    ; fd       : Unix.file_descr Lazy.t
    ; channel  : out_channel Lazy.t
    ; mutable status : status
    }

  and kind =
    | File of Path.t
    | Terminal

  and status =
    | Keep_open
    | Close_after_exec
    | Closed

  let terminal oc =
    let fd = Unix.descr_of_out_channel oc in
    { kind = Terminal
    ; fd = lazy fd
    ; channel = lazy stdout
    ; status = Keep_open
    }
  let stdout = terminal stdout
  let stderr = terminal stderr

  let file fn =
    let fd =
      lazy (Unix.openfile (Path.to_string fn)
              [O_WRONLY; O_CREAT; O_TRUNC; O_SHARE_DELETE] 0o666)
    in
    { kind = File fn
    ; fd
    ; channel = lazy (Unix.out_channel_of_descr (Lazy.force fd))
    ; status = Close_after_exec
    }

  let flush t =
    if Lazy.is_val t.channel then flush (Lazy.force t.channel)

  let fd t =
    flush t;
    Lazy.force t.fd

  let channel t = Lazy.force t.channel

  let release t =
    match t.status with
    | Closed -> ()
    | Keep_open -> flush t
    | Close_after_exec ->
      t.status <- Closed;
      if Lazy.is_val t.channel then
        close_out (Lazy.force t.channel)
      else
        Unix.close (Lazy.force t.fd)

  let multi_use t =
    { t with status = Keep_open }
end

type purpose =
  | Internal_job
  | Build_job of Path.Build.Set.t

module Temp = struct
  let tmp_files = ref Path.Set.empty
  let () =
    at_exit (fun () ->
      let fns = !tmp_files in
      tmp_files := Path.Set.empty;
      Path.Set.iter fns ~f:Path.unlink_no_err)

  let create prefix suffix =
    let fn = Path.of_string (Filename.temp_file prefix suffix) in
    tmp_files := Path.Set.add !tmp_files fn;
    fn

  let destroy fn =
    Path.unlink_no_err fn;
    tmp_files := Path.Set.remove !tmp_files fn
end

let command_line_enclosers ~dir ~(stdout_to:Output.t) ~(stderr_to:Output.t) =
  let quote fn = String.quote_for_shell (Path.to_string fn) in
  let prefix, suffix =
    match dir with
    | None -> "", ""
    | Some dir -> sprintf "(cd %s && " (quote dir), ")"
  in
  let suffix =
    match stdout_to.kind, stderr_to.kind with
    | File fn1, File fn2 when Path.equal fn1 fn2 ->
      " &> " ^ quote fn1
    | _ ->
      let suffix =
        match stdout_to.kind with
        | Terminal -> suffix
        | File fn -> suffix ^ " > " ^ quote fn
      in
      match stderr_to.kind with
      | Terminal -> suffix
      | File fn -> suffix ^ " 2> " ^ quote fn
  in
  (prefix, suffix)

let command_line ~prog ~args ~dir ~stdout_to ~stderr_to =
  let s =
    List.map (prog :: args) ~f:String.quote_for_shell
    |> String.concat ~sep:" "
  in
  let prefix, suffix = command_line_enclosers ~dir ~stdout_to ~stderr_to in
  prefix ^ s ^ suffix

module Fancy = struct
  let split_prog s =
    let len = String.length s in
    if len = 0 then
      "", "", ""
    else begin
      let rec find_prog_start i =
        if i < 0 then
          0
        else
          match s.[i] with
          | '\\' | '/' -> (i + 1)
          | _ -> find_prog_start (i - 1)
      in
      let prog_end =
        match s.[len - 1] with
        | '"' -> len - 1
        | _   -> len
      in
      let prog_start = find_prog_start (prog_end - 1) in
      let prog_end =
        match String.index_from s prog_start '.' with
        | None -> prog_end
        | Some i -> i
      in
      let before = String.take s prog_start in
      let after = String.drop s prog_end in
      let prog = String.sub s ~pos:prog_start ~len:(prog_end - prog_start) in
      before, prog, after
    end

  let color_combos =
    let open Ansi_color.Style in
    [| [ fg_blue;          bg_bright_green ]
     ; [ fg_red;           bg_bright_yellow ]
     ; [ fg_yellow;        bg_blue ]
     ; [ fg_magenta;       bg_bright_cyan ]
     ; [ fg_bright_green;  bg_blue ]
     ; [ fg_bright_yellow; bg_red ]
     ; [ fg_blue;          bg_yellow ]
     ; [ fg_bright_cyan;   bg_magenta ]
    |]

  let colorize_prog s =
    let len = String.length s in
    if len = 0 then
      Pp.verbatim s
    else
      let before, prog, after = split_prog s in
      let styles =
        let hash = Hashtbl.hash prog in
        let styles = color_combos.(hash mod (Array.length color_combos)) in
        User_message.Style.Ansi_styles styles
      in
      Pp.seq
        (Pp.verbatim before)
        (Pp.seq
           (Pp.tag (Pp.verbatim prog) ~tag:styles)
           (Pp.verbatim after))

  let rec colorize_args = function
    | [] -> []
    | "-o" :: fn :: rest ->
      Pp.verbatim "-o"
      :: Pp.tag (Pp.verbatim (String.quote_for_shell fn))
           ~tag:(User_message.Style.Ansi_styles
                   Ansi_color.Style.[bold; fg_green])
      :: colorize_args rest
    | x :: rest -> Pp.verbatim (String.quote_for_shell x) :: colorize_args rest

  let command_line ~prog ~args ~dir ~stdout_to ~stderr_to =
    let open Pp.O in
    let prog = colorize_prog (String.quote_for_shell prog) in
    let pp =
      Pp.concat ~sep:(Pp.char ' ') (prog :: colorize_args args)
    in
    let prefix, suffix = command_line_enclosers ~dir ~stdout_to ~stderr_to in
    Pp.verbatim prefix ++ pp ++ Pp.verbatim suffix

  let pp_purpose = function
    | Internal_job -> Pp.verbatim "(internal)"
    | Build_job targets ->
      let rec split_paths targets_acc ctxs_acc = function
        | [] -> List.rev targets_acc, String.Set.to_list ctxs_acc
        | path :: rest ->
          let add_ctx ctx acc =
            match ctx with
            | "default" -> acc
            | _ -> String.Set.add acc ctx
          in
          match Dpath.analyse_target path with
          | Other path ->
            split_paths
              (Path.Build.to_string path :: targets_acc)
              ctxs_acc rest
          | Regular (ctx, filename) ->
            split_paths
              (Path.Source.to_string filename :: targets_acc)
              (add_ctx ctx ctxs_acc) rest
          | Alias (ctx, name) ->
            split_paths
              (("alias " ^ Path.Source.to_string name) :: targets_acc)
              (add_ctx ctx ctxs_acc) rest
          | Install (ctx, name) ->
            split_paths
              (("install " ^ Path.Source.to_string name) :: targets_acc)
              (add_ctx ctx ctxs_acc) rest
      in
      let targets = Path.Build.Set.to_list targets in
      let target_names, contexts =
        split_paths [] String.Set.empty targets
      in
      let targets =
        List.map target_names ~f:Filename.split_extension_after_dot
        |> String.Map.of_list_multi
        |> String.Map.to_list
        |> List.map ~f:(fun (prefix, suffixes) ->
          match suffixes with
          | [] -> assert false
          | [suffix] -> prefix ^ suffix
          | _ ->
            sprintf "%s{%s}" prefix (String.concat ~sep:"," suffixes))
        |> String.concat ~sep:","
      in
      let pp = Pp.verbatim targets in
      match contexts with
      | [] -> pp
      | l ->
        let open Pp.O in
        pp ++ Pp.char ' ' ++
        Pp.tag ~tag:User_message.Style.Details
          (Pp.char '[' ++
           Pp.concat_map l ~sep:(Pp.char ',') ~f:Pp.verbatim ++
           Pp.char ']')
end

let gen_id =
  let next = ref (-1) in
  fun () -> incr next; !next

let cmdline_approximate_length prog args =
  List.fold_left args ~init:(String.length prog) ~f:(fun acc arg ->
    acc + String.length arg)

let pp_id id =
  let open Pp.O in
  Pp.char '[' ++
  Pp.tag ~tag:User_message.Style.Id (Pp.textf "%d" id) ++
  Pp.char ']'

module Exit_status = struct
  type error =
    | Failed of int
    | Signaled of string

  type t = (int, error) result

  let parse_output = function
    | "" -> None
    | s -> Some (Pp.map_tags (Ansi_color.parse s)
                   ~f:(fun styles -> User_message.Style.Ansi_styles styles))

  (* In this module, we don't need the "Error: " prefix given that it
     is already included in the error message from the command. *)
  let fail paragraphs =
    raise (User_error.E (User_message.make paragraphs))

  let handle_verbose t ~id ~output ~command_line =
    let open Pp.O in
    let output = parse_output output in
    match t with
    | Ok n ->
      Option.iter output ~f:(fun output ->
        Console.print_user_message
          (User_message.make
             [ Pp.tag ~tag:User_message.Style.Kwd (Pp.verbatim "Output") ++
               pp_id id ++ Pp.char ':'
             ; output
             ]));
      if n <> 0 then
        User_warning.emit
          [ Pp.tag ~tag:User_message.Style.Kwd (Pp.verbatim "Command") ++
            Pp.space ++ pp_id id ++
            Pp.textf " exited with code %d, but I'm ignoring it, hope \
                      that's OK." n
          ];
      n
    | Error err ->
      let msg =
        match err with
        | Failed n ->
          sprintf "exited with code %d" n
        | Signaled signame ->
          sprintf "got signal %s" signame
      in
      fail (
        Pp.tag ~tag:User_message.Style.Kwd (Pp.verbatim "Command") ++
        Pp.space ++ pp_id id ++
        Pp.space ++ Pp.text msg ++ Pp.char ':'
        :: Pp.tag ~tag:User_message.Style.Prompt (Pp.char '$') ++ Pp.char ' ' ++
           command_line
        :: Option.to_list output)

  (* Check if the command output starts with a location, ignoring ansi
     escape sequences *)
  let outputs_starts_with_location =
    let rec loop s pos len prefix =
      match prefix with
      | [] -> true
      | c :: rest ->
        pos < len &&
        match s.[pos] with
        | '\027' -> begin
            match String.index_from s pos 'm' with
            | None -> false
            | Some pos -> loop s (pos + 1) len prefix
          end
        | c' -> c = c' && loop s (pos + 1) len rest
    in
    fun output -> loop output 0 (String.length output) ['F'; 'i'; 'l'; 'e'; ' ']

  let handle_non_verbose t ~display ~purpose ~output ~prog ~command_line =
    let open Pp.O in
    let show_command =
      Config.show_full_command_on_error () ||
      not (outputs_starts_with_location output)
    in
    let output = parse_output output in
    let _, progname, _ = Fancy.split_prog prog in
    let progname_and_purpose tag =
      let progname = sprintf "%12s" progname in
      Pp.tag ~tag (Pp.verbatim progname) ++
      Pp.char ' ' ++ Fancy.pp_purpose purpose
    in
    match t with
    | Ok n ->
      if Option.is_some output ||
         (display = Config.Display.Short && purpose <> Internal_job) then
        Console.print_user_message (
          User_message.make (
            if show_command then
              progname_and_purpose Ok :: Option.to_list output
            else
              Option.to_list output));
      n
    | Error err ->
      let msg =
        match err with
        | Failed n ->
          if show_command then
            sprintf "(exit %d)" n
          else
            fail (Option.to_list output)
        | Signaled signame ->
          sprintf "(got signal %s)" signame
      in
      fail (
        progname_and_purpose Error ++ Pp.char ' ' ++
        Pp.tag ~tag:User_message.Style.Error (Pp.verbatim msg)
        :: Pp.tag ~tag:User_message.Style.Details (Pp.verbatim command_line)
        :: Option.to_list output)
end

let run_internal ?dir ?(stdout_to=Output.stdout) ?(stderr_to=Output.stderr)
      ~env ~purpose fail_mode prog args =
  let* scheduler = Scheduler.wait_for_available_job () in
  let display = Console.display () in
  let dir =
    match dir with
    | Some p ->
      if Path.is_root p then
        None
      else
        Some p
    | None -> dir
  in
  let id = gen_id () in
  let ok_codes = accepted_codes fail_mode in
  let prog_str = Path.reach_for_running ?from:dir prog in
  let command_line =
    command_line ~prog:prog_str ~args ~dir ~stdout_to ~stderr_to
  in
  let fancy_command_line =
    match display with
    | Verbose ->
      let open Pp.O in
      let cmdline =
        Fancy.command_line ~prog:prog_str ~args ~dir ~stdout_to ~stderr_to
      in
      Console.print_user_message
        (User_message.make
           [ Pp.tag ~tag:User_message.Style.Kwd (Pp.verbatim "Running") ++
             pp_id id ++ Pp.verbatim ": " ++
             cmdline
           ]);
      cmdline
    | _ -> Pp.nop
  in
  let args, response_file =
    if Sys.win32 && cmdline_approximate_length prog_str args >= 1024 then
      match Response_file.get ~prog with
      | Not_supported -> (args, None)
      | Zero_terminated_strings arg ->
        let fn = Temp.create "responsefile" ".data" in
        Io.with_file_out fn ~f:(fun oc ->
          List.iter args ~f:(fun arg ->
            output_string oc arg;
            output_char oc '\000'));
        ([arg; Path.to_string fn], Some fn)
    else
      (args, None)
  in
  let argv = prog_str :: args in
  let output_filename, stdout_to, stderr_to =
    match stdout_to.kind, stderr_to.kind with
    | (Terminal, _ | _, Terminal) when !Clflags.capture_outputs ->
      let fn = Temp.create "dune" ".output" in
      let terminal = Output.file fn in
      let get (out : Output.t) =
        if out.kind = Terminal then begin
          Output.flush out;
          terminal
        end else
          out
      in
      (Some fn, get stdout_to, get stderr_to)
    | _ ->
      (None, stdout_to, stderr_to)
  in
  let run =
    (* Output.fd might create the file with Unix.openfile. We need to
       make sure to call it before doing the chdir as the path might
       be relative. *)
    let stdout = Output.fd stdout_to in
    let stderr = Output.fd stderr_to in
    fun () ->
      Spawn.spawn ()
        ~prog:prog_str
        ~argv
        ~env:(Spawn.Env.of_array (Env.to_unix env))
        ~stdout
        ~stderr
  in
  let pid =
    match dir with
    | None -> run ()
    | Some dir -> Scheduler.with_chdir scheduler ~dir ~f:run
  in
  Output.release stdout_to;
  Output.release stderr_to;
  let+ exit_status =
    Stats.with_process ~program:prog_str ~args (Scheduler.wait_for_process pid)
  in
  Option.iter response_file ~f:Path.unlink;
  let output =
    match output_filename with
    | None -> ""
    | Some fn ->
      let s = Io.read_file fn in
      Temp.destroy fn;
      s
  in
  Log.command (Scheduler.log scheduler) ~command_line ~output ~exit_status;
  let exit_status : Exit_status.t =
    match exit_status with
    | WEXITED n when code_is_ok ok_codes n -> Ok n
    | WEXITED n -> Error (Failed n)
    | WSIGNALED n -> Error (Signaled (Signal.name n))
    | WSTOPPED _ -> assert false
  in
  match display, exit_status, output with
  | (Quiet | Progress), Ok n, "" -> n (* Optimisation for the common case *)
  | Verbose, _, _ ->
    Exit_status.handle_verbose exit_status ~id ~command_line:fancy_command_line
      ~output
  | _ ->
    Exit_status.handle_non_verbose exit_status ~prog:prog_str ~command_line
      ~output ~purpose ~display

let run ?dir ?stdout_to ?stderr_to ~env ?(purpose=Internal_job) fail_mode
      prog args =
  map_result fail_mode
    (run_internal ?dir ?stdout_to ?stderr_to ~env ~purpose fail_mode prog args)
    ~f:ignore

let run_capture_gen ?dir ?stderr_to ~env ?(purpose=Internal_job) fail_mode
      prog args ~f =
  let fn = Temp.create "dune" ".output" in
  map_result fail_mode
    (run_internal ?dir ~stdout_to:(Output.file fn) ?stderr_to
       ~env ~purpose fail_mode prog args)
    ~f:(fun () ->
      let x = f fn in
      Temp.destroy fn;
      x)

let run_capture = run_capture_gen ~f:Io.read_file
let run_capture_lines = run_capture_gen ~f:Io.lines_of_file

let run_capture_line ?dir ?stderr_to ~env ?(purpose=Internal_job) fail_mode
      prog args =
  run_capture_gen ?dir ?stderr_to ~env ~purpose fail_mode prog args ~f:(fun fn ->
    match Io.lines_of_file fn with
    | [x] -> x
    | l ->
      let cmdline =
        let prog = Path.reach_for_running ?from:dir prog in
        let prog_display = String.concat (prog :: args) ~sep:" " in
        match dir with
        | None -> prog_display
        | Some dir -> sprintf "cd %s && %s" (Path.to_string dir) prog_display
      in
      match l with
      | [] ->
        User_error.raise
          [ Pp.textf "Command returned nothing: %s" cmdline ]
      | _ ->
        User_error.raise
          [ Pp.textf "command returned too many lines: %s" cmdline
          ; Pp.vbox
              (Pp.concat_map l ~sep:Pp.cut
                 ~f:(fun line ->
                   Pp.seq (Pp.verbatim "> ") (Pp.verbatim line)))
          ])
