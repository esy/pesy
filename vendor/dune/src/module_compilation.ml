open! Stdune
open Import
open Build.O
open! No_io

module CC = Compilation_context
module SC = Super_context

(* Arguments for the compiler to prevent it from being too clever.

   The compiler creates the cmi when it thinks a .ml file has no
   corresponding .mli. However this behavior is a bit racy and doesn't
   work well when the extension is not .ml or when the .ml and .mli
   are in different directories. This flags makes the compiler think
   there is a .mli file and will the read the cmi file rather than
   create it. *)
let force_read_cmi source_file =
  [ "-intf-suffix"; Path.extension source_file ]

(* Build the cm* if the corresponding source is present, in the case of cmi if
   the mli is not present it is added as additional target to the .cmo
   generation *)

let opens modules m =
  match Modules.alias_for modules m with
  | None -> Command.Args.S []
  | Some (m : Module.t) ->
    As ["-open"; Module.Name.to_string (Module.name m)]

let build_cm cctx ~dep_graphs ~precompiled_cmi ~cm_kind (m : Module.t) =
  let sctx     = CC.super_context cctx in
  let dir      = CC.dir           cctx in
  let obj_dir  = CC.obj_dir       cctx in
  let ctx      = SC.context       sctx in
  let stdlib   = CC.stdlib        cctx in
  let mode     = Mode.of_cm_kind cm_kind in
  let dynlink  = CC.dynlink cctx in
  let sandbox  = CC.sandbox cctx in
  Context.compiler ctx mode
  |> Option.iter ~f:(fun compiler ->
    let ml_kind = Cm_kind.source cm_kind in
    Module.file m ~ml_kind
    |> Option.iter  ~f:(fun src ->
      let dst = Obj_dir.Module.cm_file_unsafe obj_dir m ~kind:cm_kind in
      let copy_interface () =
        (* symlink the .cmi into the public interface directory *)
        if Module.visibility m <> Visibility.Private
        && (Obj_dir.need_dedicated_public_dir obj_dir) then
          SC.add_rule sctx ~sandbox:false ~dir
            (Build.symlink
               ~src:(Path.build (Obj_dir.Module.cm_file_unsafe obj_dir m ~kind:Cmi))
               ~dst:(Obj_dir.Module.cm_public_file_unsafe obj_dir m ~kind:Cmi)
            )
      in
      let extra_args, extra_deps, other_targets =
        if precompiled_cmi then
          force_read_cmi src, [], []
        else
          (* If we're compiling an implementation, then the cmi is present *)
          let public_vlib_module = Module.kind m = Impl_vmodule in
          match cm_kind, Module.file m ~ml_kind:Intf, public_vlib_module with
          (* If there is no mli, [ocamlY -c file.ml] produces both the
             .cmY and .cmi. We choose to use ocamlc to produce the cmi
             and to produce the cmx we have to wait to avoid race
             conditions. *)
          | Cmo, None, false ->
            copy_interface ();
            [], [], [Obj_dir.Module.cm_file_unsafe obj_dir m ~kind:Cmi]
          | Cmo, None, true
          | (Cmo | Cmx), _, _ ->
            force_read_cmi src,
            [Path.build (Obj_dir.Module.cm_file_unsafe obj_dir m ~kind:Cmi)],
            []
          | Cmi, _, _ ->
            copy_interface ();
            [], [], []
      in
      let other_targets =
        match cm_kind with
        | Cmx ->
          (Obj_dir.Module.obj_file obj_dir m ~kind:Cmx ~ext:ctx.lib_config.ext_obj)
          :: other_targets
        | Cmi | Cmo -> other_targets
      in
      let dep_graph = Ml_kind.Dict.get dep_graphs ml_kind in
      let opaque = CC.opaque cctx in
      let other_cm_files =
        Build.dyn_paths
          (Dep_graph.deps_of dep_graph m >>^ fun deps ->
           List.concat_map deps
             ~f:(fun m ->
               let deps =
                 [Path.build (Obj_dir.Module.cm_file_unsafe obj_dir m ~kind:Cmi)] in
               if Module.has m ~ml_kind:Impl && cm_kind = Cmx && not opaque then
                 let cmx = Obj_dir.Module.cm_file_unsafe obj_dir m ~kind:Cmx in
                 Path.build cmx :: deps
               else
                 deps))
      in
      let other_targets, cmt_args =
        match cm_kind with
        | Cmx -> (other_targets, Command.Args.S [])
        | Cmi | Cmo ->
          let fn =
            Option.value_exn (Obj_dir.Module.cmt_file obj_dir m ~ml_kind) in
          (fn :: other_targets, A "-bin-annot")
      in
      if CC.dir_kind cctx = Jbuild then begin
        (* Symlink the object files in the original directory for
           backward compatibility *)
        let old_dst =
          (Module.obj_name m) ^ (Cm_kind.ext cm_kind)
          |> Path.Build.relative dir
        in
        SC.add_rule sctx ~dir
          (Build.symlink ~src:(Path.build dst) ~dst:old_dst);
        List.iter other_targets ~f:(fun in_obj_dir ->
          let in_dir = Path.Build.relative dir
                         (Path.Build.basename in_obj_dir) in
          SC.add_rule sctx ~dir
            (Build.symlink ~src:(Path.build in_obj_dir) ~dst:in_dir))
      end;
      let opaque_arg =
        let intf_only = cm_kind = Cmi && not (Module.has m ~ml_kind:Impl) in
        if opaque
        || (intf_only && Ocaml_version.supports_opaque_for_mli ctx.version) then
          Command.Args.A "-opaque"
        else
          As []
      in
      let dir, no_keep_locs =
        match CC.no_keep_locs cctx
            , cm_kind
            , Ocaml_version.supports_no_keep_locs ctx.version
        with
        | true, Cmi, true ->
          (ctx.build_dir, Command.Args.As ["-no-keep-locs"])
        | true, Cmi, false ->
          (Obj_dir.byte_dir obj_dir, As [])
        (* emulated -no-keep-locs *)
        | true, (Cmo | Cmx), _
        | false, _, _ ->
          (ctx.build_dir, As [])
      in
      let flags =
        let flags = Ocaml_flags.get_for_cm (CC.flags cctx) ~cm_kind in
        match Module.pp_flags m with
        | None -> flags
        | Some pp ->
          Build.fanout flags pp >>^ fun (flags, pp_flags) ->
          flags @ pp_flags
      in
      let modules = Compilation_context.modules cctx in
      SC.add_rule sctx ?sandbox ~dir
        (Build.S.seqs [Build.paths extra_deps; other_cm_files]
           (Command.run ~dir:(Path.build dir) (Ok compiler)
              [ Command.Args.dyn flags
              ; no_keep_locs
              ; cmt_args
              ; Command.Args.S (
                  Obj_dir.all_obj_dirs obj_dir ~mode
                  |> List.concat_map ~f:(fun p -> [ Command.Args.A "-I"
                                                  ; Path (Path.build p)])
                )
              ; Cm_kind.Dict.get (CC.includes cctx) cm_kind
              ; As extra_args
              ; if dynlink || cm_kind <> Cmx then As [] else A "-nodynlink"
              ; A "-no-alias-deps"; opaque_arg
              ; opens modules m
              ; As (match stdlib with
                  | None -> []
                  | Some _ ->
                    (* XXX why aren't these just normal library flags? *)
                    ["-nopervasives"; "-nostdlib"])
              ; A "-o"; Target dst
              ; A "-c"; Command.Ml_kind.flag ml_kind; Dep src
              ; Hidden_targets other_targets
              ]))))

let build_module ~dep_graphs ?(precompiled_cmi=false) cctx m =
  build_cm cctx m ~dep_graphs ~precompiled_cmi ~cm_kind:Cmo;
  build_cm cctx m ~dep_graphs ~precompiled_cmi ~cm_kind:Cmx;
  if not precompiled_cmi then
    build_cm cctx m ~dep_graphs ~precompiled_cmi ~cm_kind:Cmi;
  Compilation_context.js_of_ocaml cctx
  |> Option.iter ~f:(fun js_of_ocaml ->
    (* Build *.cmo.js *)
    let sctx     = CC.super_context cctx in
    let dir      = CC.dir           cctx in
    let obj_dir = CC.obj_dir cctx in
    let src = Obj_dir.Module.cm_file_unsafe obj_dir m ~kind:Cm_kind.Cmo in
    let target = Path.Build.extend_basename src ~suffix:".js" in
    SC.add_rules sctx ~dir
      (Js_of_ocaml_rules.build_cm cctx ~js_of_ocaml ~src ~target))

let ocamlc_i ?(flags=[]) ~dep_graphs cctx (m : Module.t) ~output =
  let sctx     = CC.super_context cctx in
  let obj_dir  = CC.obj_dir       cctx in
  let dir      = CC.dir           cctx in
  let ctx      = SC.context       sctx in
  let src = Option.value_exn (Module.file m ~ml_kind:Impl) in
  let dep_graph = Ml_kind.Dict.get dep_graphs Impl in
  let sandbox = Compilation_context.sandbox cctx in
  let cm_deps =
    Build.dyn_paths
      (Dep_graph.deps_of dep_graph m >>^ fun deps ->
       List.concat_map deps
         ~f:(fun m -> [Path.build (Obj_dir.Module.cm_file_unsafe
                                     obj_dir m ~kind:Cmi)]))
  in
  let ocaml_flags = Ocaml_flags.get_for_cm (CC.flags cctx) ~cm_kind:Cmo in
  let modules = Compilation_context.modules cctx in
  SC.add_rule sctx ?sandbox ~dir
    (Build.S.seq cm_deps
       (Build.S.map ~f:(Action.with_stdout_to output)
          (Command.run (Ok ctx.ocamlc) ~dir:(Path.build ctx.build_dir)
             [ Command.Args.dyn ocaml_flags
             ; A "-I"; Path (Path.build (Obj_dir.byte_dir obj_dir))
             ; Cm_kind.Dict.get (CC.includes cctx) Cmo
             ; opens modules m
             ; As flags
             ; A "-short-paths"
             ; A "-i"; Command.Ml_kind.flag Impl; Dep src
             ]))
     >>> Build.action_dyn () ~targets:[output])


(* The alias module is an implementation detail to support wrapping library
   modules under a single toplevel name. Since OCaml doesn't have proper support
   for namespacing at the moment, in order to expose module `X` of library `foo`
   as `Foo.X`, Dune does the following:

   - it compiles x.ml to Foo__X.cmo, Foo__X.cmx, Foo__X.o, ...
   - it implictly exposes a module alias [module X = Foo__X] to all the
     modules of the `foo` library

   The second point is achieved by implicitely openning a module containing such
   aliases for all modules of `foo` when compiling modules of `foo` via the
   `-open` option of the compiler. This module is called the alias module and is
   implicitely generated by Dune.*)

let build_alias_module ~loc ~alias_module ~dir ~cctx =
  let sctx = Compilation_context.super_context cctx in
  let file = Option.value_exn (Module.file alias_module ~ml_kind:Impl) in
  let modules = Compilation_context.modules cctx in
  let alias_file () =
    let main_module_name =
      Modules.main_module_name modules
      |> Option.value_exn
    in
    Modules.for_alias modules
    |> Module.Name.Map.values
    |> List.map ~f:(fun (m : Module.t) ->
      let name = Module.Name.to_string (Module.name m) in
      sprintf "(** @canonical %s.%s *)\n\
               module %s = %s\n"
        (Module.Name.to_string main_module_name)
        name
        name
        (Module.Name.to_string (Module.real_unit_name m)))
    |> String.concat ~sep:"\n"
  in
  Super_context.add_rule ~loc sctx ~dir (
    Build.arr alias_file
    >>> Build.write_file_dyn (Path.as_in_build_dir_exn file)
  );
  let cctx = Compilation_context.for_alias_module cctx in
  build_module cctx alias_module
    ~dep_graphs:(Dep_graph.Ml_kind.dummy alias_module)

let build_all cctx ~dep_graphs =
  let for_wrapped_compat =
    lazy (Compilation_context.for_wrapped_compat cctx) in
  let modules = Compilation_context.modules cctx in
  Modules.iter_no_vlib modules ~f:(fun m ->
    match Module.kind m with
    | Alias ->
      let cctx = Compilation_context.for_alias_module cctx in
      let dir = Compilation_context.dir cctx in
      build_alias_module ~loc:Loc.none ~alias_module:m ~dir ~cctx
    | Wrapped_compat ->
      let cctx = Lazy.force for_wrapped_compat in
      build_module cctx ~dep_graphs m
    |  _ ->
      let cctx =
        if Modules.is_stdlib_alias modules m then
          (* XXX it would probably be simpler if the flags were just for this
             module in the definition of the stanza *)
          Compilation_context.for_alias_module cctx
        else
          cctx
      in
      build_module cctx ~dep_graphs m)
