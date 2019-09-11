open Import

let add_diff sctx loc alias ~dir ~input ~output =
  let open Build.O in
  let action = Action.diff input output in
  Super_context.add_alias_action sctx alias ~dir ~loc:(Some loc) ~locks:[]
    ~stamp:input
    (Build.paths [input; output]
     >>>
     Build.action
       ~dir:(Path.build dir)
       ~targets:[]
       action)

let rec subdirs_until_root dir =
  match Path.parent dir with
  | None -> [dir]
  | Some d -> dir :: subdirs_until_root d

let depend_on_files ~named dir =
  subdirs_until_root dir
  |> List.concat_map ~f:(fun dir -> List.map named ~f:(Path.relative dir))
  |> Build.paths_existing

let formatted = ".formatted"

let gen_rules_output sctx (config : Dune_file.Auto_format.t)
      ~dialects ~expander ~output_dir =
  assert (formatted = Path.Build.basename output_dir);
  let loc = Dune_file.Auto_format.loc config in
  let dir = Path.Build.parent_exn output_dir in
  let source_dir = Path.Build.drop_build_context_exn dir in
  let alias_formatted = Alias.fmt ~dir:output_dir in
  let resolve_program =
    Super_context.resolve_program ~dir sctx ~loc:(Some loc) in
  let depend_on_files named = depend_on_files ~named (Path.build dir) in
  let setup_formatting file =
    let input_basename = Path.Source.basename file in
    let input = Path.Build.relative dir input_basename in
    let output = Path.Build.relative output_dir input_basename in

    let formatter =
      let input = Path.build input in
      match Path.Source.basename file with
      | "dune" when Dune_file.Auto_format.includes config Dune ->
        let exe = resolve_program "dune" in
        let args = [Command.Args.A "format-dune-file"; Dep input] in
        let dir = Path.build dir in
        Some (Command.run ~dir ~stdout_to:output exe args)
      | _ ->
        let ext = Path.Source.extension file in
        let open Option.O in
        let* (dialect, kind) = Dialect.DB.find_by_extension dialects ext in
        let* () =
          Option.some_if (Dune_file.Auto_format.includes
                            config (Dialect (Dialect.name dialect))) ()
        in
        let+ (loc, action, extra_deps) =
          match Dialect.format dialect kind with
          | Some _ as action ->
            action
          | None ->
            begin match Dialect.preprocess dialect kind with
            | None -> Dialect.format Dialect.ocaml kind
            | Some _ -> None
            end
        in
        let src = Path.as_in_build_dir_exn input in
        let extra_deps =
          match extra_deps with
          | [] ->
            Build.return ()
          | extra_deps ->
            Build.S.ignore (depend_on_files extra_deps)
        in
        Build.S.seq extra_deps
          (Preprocessing.action_for_pp sctx ~dep_kind:Lib_deps_info.Kind.Required
             ~loc ~expander ~action ~src ~target:(Some output))
    in
    Option.iter formatter ~f:(fun arr ->
      Super_context.add_rule sctx ~mode:Standard ~loc ~dir arr;
      add_diff sctx loc alias_formatted ~dir
        ~input:(Path.build input) ~output:(Path.build output))
  in
  File_tree.files_of (Super_context.file_tree sctx) source_dir
  |> Path.Source.Set.iter ~f:setup_formatting;
  Rules.Produce.Alias.add_deps alias_formatted Path.Set.empty

let gen_rules ~dir =
  let output_dir = Path.Build.relative dir formatted in
  let alias = Alias.fmt ~dir in
  let alias_formatted = Alias.fmt ~dir:output_dir in
  Alias.stamp_file alias_formatted
  |> Path.build
  |> Path.Set.singleton
  |> Rules.Produce.Alias.add_deps alias
