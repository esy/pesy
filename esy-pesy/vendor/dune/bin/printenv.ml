open Stdune
open Import

let doc = "Print the environment of a directory"

let man =
  [ `S "DESCRIPTION"
  ; `P {|$(b,dune printenv DIR) prints the environment of a directory|}
  ; `Blocks Common.help_secs
  ]

let info = Term.info "printenv" ~doc ~man

let dump sctx ~dir =
  let open Build.O in
  Super_context.dump_env sctx ~dir
  >>^ fun env ->
  ((Super_context.context sctx).name, env)

let pp ppf sexps =
  Dune_lang.List sexps
  |> Dune_lang.add_loc ~loc:Loc.none
  |> Dune_lang.Cst.concrete
  |> List.singleton
  |> Format.fprintf ppf "@[<v1>@,%a@]@,"
       Dune.Format_dune_lang.pp_top_sexps

let term =
  let+ common = Common.term
  and+ dir = Arg.(value & pos 0 dir "" & info [] ~docv:"PATH")
  in
  Common.set_common common ~targets:[];
  let log = Log.create common in
  Scheduler.go ~log ~common (fun () ->
    let open Fiber.O in
    let* setup = Import.Main.setup ~log common in
    let dir = Path.of_string dir in
    let checked = Util.check_path setup.workspace.contexts dir in
    let request =
      Build.all (
        match checked with
        | In_build_dir (ctx, _) ->
          let sctx = String.Map.find_exn setup.scontexts ctx.name in
          [dump sctx ~dir:(Path.as_in_build_dir_exn dir)]
        | In_source_dir dir ->
          String.Map.values setup.scontexts
          |> List.map ~f:(fun sctx ->
            let dir =
              Path.Build.append_source (Super_context.build_dir sctx) dir in
            dump sctx ~dir)
        | External _ ->
          User_error.raise
            [ Pp.text "Environment is not defined for external paths" ]
        | In_install_dir _ ->
          User_error.raise
            [ Pp.text "Environment is not defined in install dirs" ]
      )
    in
    Build_system.do_build ~request
    >>| function
    | [(_, env)] ->
      Format.printf "%a" pp env
    | l ->
      List.iter l ~f:(fun (name, env) ->
        Format.printf "@[<v2>Environment for context %s:@,%a@]@." name pp env))

let command = term, info
