open Import
open! No_io
open Build.O
module SC = Super_context

let executables_rules ~sctx ~dir ~dir_kind ~expander
      ~dir_contents ~scope ~compile_info
      (exes : Dune_file.Executables.t) =
  (* Use "eobjs" rather than "objs" to avoid a potential conflict
     with a library of the same name *)
  let obj_dir = Obj_dir.make_exe ~dir ~name:(snd (List.hd exes.names)) in
  Check_rules.add_obj_dir sctx ~obj_dir;
  let modules =
    Dir_contents.modules_of_executables dir_contents
      ~first_exe:(snd (List.hd exes.names))
  in

  let preprocessor_deps =
    SC.Deps.interpret sctx exes.buildable.preprocessor_deps ~expander
  in
  let pp =
    Preprocessing.make sctx ~dir ~dep_kind:Required
      ~scope
      ~expander
      ~preprocess:exes.buildable.preprocess
      ~preprocessor_deps
      ~lint:exes.buildable.lint
      ~lib_name:None
      ~dir_kind
  in
  let modules =
    Module.Name.Map.map modules ~f:(fun m ->
      Preprocessing.pp_module_as pp (Module.name m) m)
  in

  let programs =
    List.map exes.names ~f:(fun (loc, name) ->
      let mod_name = Module.Name.of_string name in
      match Module.Name.Map.find modules mod_name with
      | Some m ->
        if not (Module.has_impl m) then
          Errors.fail loc "Module %a has no implementation."
            Module.Name.pp mod_name
        else
          { Exe.Program.name; main_module_name = mod_name ; loc }
      | None -> Errors.fail loc "Module %a doesn't exist."
                  Module.Name.pp mod_name)
  in

  let linkages =
    let module L = Dune_file.Executables.Link_mode in
    let ctx = SC.context sctx in
    let l =
      let has_native = Option.is_some ctx.ocamlopt in
      List.filter_map (L.Set.to_list exes.modes) ~f:(fun (mode : L.t) ->
        match has_native, mode.mode with
        | false, Native ->
          None
        | _ ->
          Some (Exe.Linkage.of_user_config ctx mode))
    in
    (* If bytecode was requested but not native or best version,
       add custom linking *)
    if L.Set.mem exes.modes L.byte         &&
       not (L.Set.mem exes.modes L.native) &&
       not (L.Set.mem exes.modes L.exe) then
      Exe.Linkage.custom :: l
    else
      l
  in

  let flags = SC.ocaml_flags sctx ~dir exes.buildable in
  let link_deps =
    SC.Deps.interpret sctx ~expander exes.link_deps
  in
  let link_flags =
    link_deps >>^ ignore >>>
    Expander.expand_and_eval_set expander exes.link_flags
      ~standard:(Build.return [])
  in

  let cctx =
    let requires_compile = Lib.Compile.direct_requires compile_info in
    let requires_link = Lib.Compile.requires_link compile_info in
    Compilation_context.create ()
      ~super_context:sctx
      ~expander
      ~scope
      ~obj_dir
      ~dir_kind
      ~modules
      ~flags
      ~requires_link
      ~requires_compile
      ~preprocessing:pp
      ~opaque:(SC.opaque sctx)
  in

  let requires_compile = Compilation_context.requires_compile cctx in

  Exe.build_and_link_many cctx
    ~programs
    ~linkages
    ~link_flags
    ~js_of_ocaml:exes.buildable.js_of_ocaml;

  (cctx,
   Merlin.make ()
     ~requires:requires_compile
     ~flags:(Ocaml_flags.common flags)
     ~preprocess:(Dune_file.Buildable.single_preprocess exes.buildable)
     (* only public_dir? *)
     ~objs_dirs:(Path.Set.singleton (Obj_dir.public_cmi_dir obj_dir)))

let rules ~sctx ~dir ~dir_contents ~scope ~expander ~dir_kind
      (exes : Dune_file.Executables.t) =
  let compile_info =
    Lib.DB.resolve_user_written_deps_for_exes
      (Scope.libs scope)
      exes.names
      exes.buildable.libraries
      ~pps:(Dune_file.Preprocess_map.pps exes.buildable.preprocess)
      ~allow_overlaps:exes.buildable.allow_overlapping_dependencies
      ~variants:exes.variants
  in
  SC.Libs.gen_select_rules sctx compile_info ~dir;
  SC.Libs.with_lib_deps sctx compile_info ~dir
    ~f:(fun () ->
      executables_rules exes ~sctx ~dir
        ~dir_contents ~scope ~expander ~compile_info ~dir_kind)
