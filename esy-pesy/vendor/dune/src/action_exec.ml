open! Stdune
open Import
open Fiber.O

type exec_context =
  { context : Context.t option
  ; purpose : Process.purpose
  }

let exec_run ~ectx ~dir ~env ~stdout_to ~stderr_to prog args =
  begin match ectx.context with
  | None
  | Some { Context.for_host = None; _ } -> ()
  | Some ({ Context.for_host = Some host; _ } as target) ->
    let invalid_prefix prefix =
      match Path.descendant prog ~of_:prefix with
      | None -> ()
      | Some _ ->
        User_error.raise
          [ Pp.textf "Context %s has a host %s."
              target.name host.name
          ; Pp.textf "It's not possible to execute binary %s in it."
              (Path.to_string_maybe_quoted prog)
          ; Pp.nop
          ; Pp.text "This is a bug and should be reported upstream."
          ]
    in
    invalid_prefix (Path.relative Path.build_dir target.name);
    invalid_prefix (Path.relative Path.build_dir ("install/" ^ target.name));
  end;
  Process.run Strict ~dir ~env
    ~stdout_to ~stderr_to
    ~purpose:ectx.purpose
    prog args

let exec_echo stdout_to str =
  Fiber.return (output_string (Process.Output.channel stdout_to) str)

let rec exec t ~ectx ~dir ~env ~stdout_to ~stderr_to =
  match (t : Action.t) with
  | Run (Error e, _) ->
    Action.Prog.Not_found.raise e
  | Run (Ok prog, args) ->
    exec_run ~ectx ~dir ~env ~stdout_to ~stderr_to prog args
  | Chdir (dir, t) ->
    exec t ~ectx ~dir ~env ~stdout_to ~stderr_to
  | Setenv (var, value, t) ->
    exec t ~ectx ~dir ~stdout_to ~stderr_to
      ~env:(Env.add env ~var ~value)
  | Redirect (Stdout, fn, Echo s) ->
    Io.write_file (Path.build fn) (String.concat s ~sep:" ");
    Fiber.return ()
  | Redirect (outputs, fn, t) ->
    let fn = Path.build fn in
    redirect ~ectx ~dir outputs fn t ~env ~stdout_to ~stderr_to
  | Ignore (outputs, t) ->
    redirect ~ectx ~dir outputs Config.dev_null t ~env ~stdout_to ~stderr_to
  | Progn l ->
    exec_list l ~ectx ~dir ~env ~stdout_to ~stderr_to
  | Echo strs -> exec_echo stdout_to (String.concat strs ~sep:" ")
  | Cat fn ->
    Io.with_file_in fn ~f:(fun ic ->
      Io.copy_channels ic (Process.Output.channel stdout_to));
    Fiber.return ()
  | Copy (src, dst) ->
    let dst = Path.build dst in
    Io.copy_file ~src ~dst ();
    Fiber.return ()
  | Symlink (src, dst) ->
    if Sys.win32 then
      let dst = Path.build dst in
      Io.copy_file ~src ~dst ()
    else begin
      let src =
        match Path.Build.parent dst with
        | None -> Path.to_string src
        | Some from ->
          let from = Path.build from in
          Path.reach ~from src
      in
      let dst = Path.Build.to_string dst in
      match Unix.readlink dst with
      | target ->
        if target <> src then begin
          (* @@DRA Win32 remove read-only attribute needed when symlinking enabled *)
          Unix.unlink dst;
          Unix.symlink src dst
        end
      | exception _ ->
        Unix.symlink src dst
    end;
    Fiber.return ()
  | Copy_and_add_line_directive (src, dst) ->
    Io.with_file_in src ~f:(fun ic ->
      Path.build dst
      |> Io.with_file_out ~f:(fun oc ->
        let fn = Path.drop_optional_build_context src in
        output_string oc
          (Utils.line_directive
             ~filename:(Path.to_string fn)
             ~line_number:1);
        Io.copy_channels ic oc));
    Fiber.return ()
  | System cmd ->
    let path, arg =
      Utils.system_shell_exn ~needed_to:"interpret (system ...) actions"
    in
    exec_run ~ectx ~dir ~env ~stdout_to ~stderr_to path [arg; cmd]
  | Bash cmd ->
    exec_run ~ectx ~dir ~env ~stdout_to ~stderr_to
      (Utils.bash_exn ~needed_to:"interpret (bash ...) actions")
      ["-e"; "-u"; "-o"; "pipefail"; "-c"; cmd]
  | Write_file (fn, s) ->
    Io.write_file (Path.build fn) s;
    Fiber.return ()
  | Rename (src, dst) ->
    Unix.rename (Path.Build.to_string src) (Path.Build.to_string dst);
    Fiber.return ()
  | Remove_tree path ->
    Path.rm_rf (Path.build path);
    Fiber.return ()
  | Mkdir path ->
    begin
      if Path.is_in_build_dir path then
        Path.mkdir_p path
      else
        Code_error.raise "Action_exec.exec: mkdir on non build dir"
          ["path", Path.to_dyn path]
    end;
    Fiber.return ()
  | Digest_files paths ->
    let s =
      let data =
        List.map paths ~f:(fun fn ->
          (Path.to_string fn, Cached_digest.file fn))
      in
      Digest.generic data
    in
    exec_echo stdout_to (Digest.to_string_raw s)
  | Diff ({ optional = _; file1; file2; mode } as diff) ->
    if Diff.eq_files diff then
      Fiber.return ()
    else begin
      let is_copied_from_source_tree file =
        match Path.drop_build_context file with
        | None -> false
        | Some file -> Path.exists (Path.source file)
      in
      if is_copied_from_source_tree file1 &&
         not (is_copied_from_source_tree file2) then begin
        Promotion.File.register
          { src = Path.as_in_build_dir_exn file2
          ; dst = Option.value_exn (Path.drop_build_context file1)
          }
      end;
      if mode = Binary then
        User_error.raise
          [ Pp.textf "Files %s and %s differ."
              (Path.to_string_maybe_quoted file1)
              (Path.to_string_maybe_quoted file2)
          ]
      else
        Print_diff.print file1 file2
          ~skip_trailing_cr:(mode = Text && Sys.win32)
    end
  | Merge_files_into (sources, extras, target) ->
    let lines =
      List.fold_left
        ~init:(String.Set.of_list extras)
        ~f:(fun set source_path ->
          Io.lines_of_file source_path
          |> String.Set.of_list
          |> String.Set.union set
        )
        sources
    in
    let target = Path.build target in
    Io.write_lines target (String.Set.to_list lines);
    Fiber.return ()

and redirect outputs fn t ~ectx ~dir ~env ~stdout_to ~stderr_to =
  let out = Process.Output.file fn in
  let stdout_to, stderr_to =
    match outputs with
    | Stdout -> (out, stderr_to)
    | Stderr -> (stdout_to, out)
    | Outputs -> (out, out)
  in
  exec t ~ectx ~dir ~env ~stdout_to ~stderr_to >>| fun () ->
  Process.Output.release out

and exec_list l ~ectx ~dir ~env ~stdout_to ~stderr_to =
  match l with
  | [] ->
    Fiber.return ()
  | [t] ->
    exec t ~ectx ~dir ~env ~stdout_to ~stderr_to
  | t :: rest ->
    let* () =
      let stdout_to = Process.Output.multi_use stdout_to in
      let stderr_to = Process.Output.multi_use stderr_to in
      exec t ~ectx ~dir ~env ~stdout_to ~stderr_to
    in
    exec_list rest ~ectx ~dir ~env ~stdout_to ~stderr_to

let exec ~targets ~context ~env t =
  let env =
    match (context : Context.t option), env with
    | _ , Some e -> e
    | None, None   -> Env.initial
    | Some c, None -> c.env
  in
  let purpose = Process.Build_job targets in
  let ectx = { purpose; context } in
  exec t ~ectx ~dir:Path.root ~env
    ~stdout_to:Process.Output.stdout
    ~stderr_to:Process.Output.stderr
