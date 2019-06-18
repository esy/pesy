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
    Util.check_path setup.workspace.contexts dir;
    let request =
      Build.all (
        match Path.extract_build_context dir with
        | Some (ctx, _) ->
          let sctx = String.Map.find_exn setup.scontexts ctx in
          [dump sctx ~dir]
        | None ->
          String.Map.values setup.scontexts
          |> List.map ~f:(fun sctx ->
            let dir =
              Path.append (Super_context.context sctx).build_dir dir
            in
            dump sctx ~dir)
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
