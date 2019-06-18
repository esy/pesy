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
let build_cm cctx ?sandbox ?(dynlink=true) ~dep_graphs
      ~cm_kind (m : Module.t) =
  let sctx     = CC.super_context cctx in
  let dir      = CC.dir           cctx in
  let obj_dir  = CC.obj_dir       cctx in
  let ctx      = SC.context       sctx in
  let stdlib   = CC.stdlib        cctx in
  let vimpl    = CC.vimpl cctx in
  let mode     = Mode.of_cm_kind cm_kind in
  Context.compiler ctx mode
  |> Option.iter ~f:(fun compiler ->
    Option.iter (Module.cm_source m cm_kind) ~f:(fun src ->
      let ml_kind = Cm_kind.source cm_kind in
      let dst = Module.cm_file_unsafe m cm_kind in
      let copy_interface () =
        (* symlink the .cmi into the public interface directory *)
        if not (Module.is_private m)
        && (Obj_dir.need_dedicated_public_dir obj_dir) then
          SC.add_rule sctx ~sandbox:false ~dir
            (Build.symlink
               ~src:(Module.cm_file_unsafe m Cmi)
               ~dst:(Module.cm_public_file_unsafe m Cmi)
            )
      in
      let extra_args, extra_deps, other_targets =
        match cm_kind, Module.intf m
              , Vimpl.is_public_vlib_module vimpl m with
        (* If there is no mli, [ocamlY -c file.ml] produces both the
           .cmY and .cmi. We choose to use ocamlc to produce the cmi
           and to produce the cmx we have to wait to avoid race
           conditions. *)
        | Cmo, None, false ->
          copy_interface ();
          [], [], [Module.cm_file_unsafe m Cmi]
        | Cmo, None, true
        | (Cmo | Cmx), _, _ ->
          force_read_cmi src,
          [Module.cm_file_unsafe m Cmi],
          []
        | Cmi, _, _ ->
          copy_interface ();
          [], [], []
      in
      let other_targets =
        match cm_kind with
        | Cmx -> Module.obj_file m ~kind:Cmx ~ext:ctx.ext_obj :: other_targets
        | Cmi | Cmo -> other_targets
      in
      let dep_graph = Ml_kind.Dict.get dep_graphs ml_kind in
      let opaque = CC.opaque cctx in
      let other_cm_files =
        Build.dyn_paths
          (Dep_graph.deps_of dep_graph m >>^ fun deps ->
           List.concat_map deps
             ~f:(fun m ->
               let deps = [Module.cm_file_unsafe m Cmi] in
               if Module.has_impl m && cm_kind = Cmx && not opaque then
                 Module.cm_file_unsafe m Cmx :: deps
               else
                 deps))
      in
      let other_targets, cmt_args =
        match cm_kind with
        | Cmx -> (other_targets, Arg_spec.S [])
        | Cmi | Cmo ->
          let fn = Option.value_exn (Module.cmt_file m ml_kind) in
          (fn :: other_targets, A "-bin-annot")
      in
      if CC.dir_kind cctx = Jbuild then begin
        (* Symlink the object files in the original directory for
           backward compatibility *)
        let old_dst = Path.relative dir ((Module.obj_name m) ^ (Cm_kind.ext cm_kind)) in
        SC.add_rule sctx ~dir (Build.symlink ~src:dst ~dst:old_dst);
        List.iter other_targets ~f:(fun in_obj_dir ->
          let in_dir = Path.relative dir (Path.basename in_obj_dir) in
          SC.add_rule sctx ~dir (Build.symlink ~src:in_obj_dir ~dst:in_dir))
      end;
      let opaque_arg =
        let intf_only = cm_kind = Cmi && not (Module.has_impl m) in
        if opaque
        || (intf_only && Ocaml_version.supports_opaque_for_mli ctx.version) then
          Arg_spec.A "-opaque"
        else
          As []
      in
      let dir, no_keep_locs =
        match CC.no_keep_locs cctx
            , cm_kind
            , Ocaml_version.supports_no_keep_locs ctx.version
        with
        | true, Cmi, true ->
          (ctx.build_dir, Arg_spec.As ["-no-keep-locs"])
        | true, Cmi, false ->
          (Obj_dir.byte_dir obj_dir, As []) (* emulated -no-keep-locs *)
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
      SC.add_rule sctx ?sandbox ~dir
        (Build.paths extra_deps >>>
         other_cm_files >>>
         flags
         >>>
         Build.run ~dir (Ok compiler)
           [ Dyn (fun flags -> As flags)
           ; no_keep_locs
           ; cmt_args
           ; S (
               Obj_dir.all_obj_dirs obj_dir ~mode
               |> List.concat_map ~f:(fun p -> [Arg_spec.A "-I"; Path p])
             )
           ; Cm_kind.Dict.get (CC.includes cctx) cm_kind
           ; As extra_args
           ; if dynlink || cm_kind <> Cmx then As [] else A "-nodynlink"
           ; A "-no-alias-deps"; opaque_arg
           ; (match CC.alias_module cctx with
              | None -> S []
              | Some (m : Module.t) ->
                As ["-open"; Module.Name.to_string (Module.name m)])
           ; As (match stdlib with
               | None -> []
               | Some { Dune_file.Library.Stdlib.modules_before_stdlib; _ } ->
                 let flags = ["-nopervasives"; "-nostdlib"] in
                 if Module.Name.Set.mem modules_before_stdlib
                      (Module.name m) then
                   flags
                 else
                   match CC.lib_interface_module cctx with
                   | None -> flags
                   | Some m' ->
                     (* See comment in [Dune_file.Stdlib]. *)
                     if Module.name m = Module.name m' then
                       "-w" :: "-49" :: flags
                     else
                       "-open" :: Module.Name.to_string (Module.name m')
                       :: flags)
           ; A "-o"; Target dst
           ; A "-c"; Ml_kind.flag ml_kind; Dep src
           ; Hidden_targets other_targets
           ])))

let build_module ?sandbox ?js_of_ocaml ?dynlink ~dep_graphs cctx m =
  List.iter Cm_kind.all ~f:(fun cm_kind ->
    build_cm cctx m ?sandbox ?dynlink ~dep_graphs ~cm_kind);
  Option.iter js_of_ocaml ~f:(fun js_of_ocaml ->
    (* Build *.cmo.js *)
    let sctx     = CC.super_context cctx in
    let dir      = CC.dir           cctx in
    let src = Module.cm_file_unsafe m Cm_kind.Cmo in
    let target = Path.extend_basename src ~suffix:".js" in
    SC.add_rules sctx ~dir
      (Js_of_ocaml_rules.build_cm cctx ~js_of_ocaml ~src ~target))

let build_modules ?sandbox ?js_of_ocaml ?dynlink ~dep_graphs cctx =
  Module.Name.Map.iter
    (match CC.alias_module cctx with
     | None -> CC.modules cctx
     | Some (m : Module.t) ->
       Module.Name.Map.remove (CC.modules cctx) (Module.name m))
    ~f:(build_module cctx ?sandbox ?js_of_ocaml ?dynlink ~dep_graphs)

let ocamlc_i ?sandbox ?(flags=[]) ~dep_graphs cctx (m : Module.t) ~output =
  let sctx     = CC.super_context cctx in
  let obj_dir  = CC.obj_dir       cctx in
  let dir      = CC.dir           cctx in
  let ctx      = SC.context       sctx in
  let src = Option.value_exn (Module.file m Impl) in
  let dep_graph = Ml_kind.Dict.get dep_graphs Impl in
  let cm_deps =
    Build.dyn_paths
      (Dep_graph.deps_of dep_graph m >>^ fun deps ->
       List.concat_map deps
         ~f:(fun m -> [Module.cm_file_unsafe m Cmi]))
  in
  SC.add_rule sctx ?sandbox ~dir
    (cm_deps >>>
     Ocaml_flags.get_for_cm (CC.flags cctx) ~cm_kind:Cmo >>>
     Build.run (Ok ctx.ocamlc) ~dir:ctx.build_dir
       [ Dyn (fun ocaml_flags -> As ocaml_flags)
       ; A "-I"; Path (Obj_dir.byte_dir obj_dir)
       ; Cm_kind.Dict.get (CC.includes cctx) Cmo
       ; (match CC.alias_module cctx with
          | None -> S []
          | Some (m : Module.t) ->
            As ["-open"; Module.Name.to_string (Module.name m)])
       ; As flags
       ; A "-short-paths"
       ; A "-i"; Ml_kind.flag Impl; Dep src
       ]
     >>^ (fun act -> Action.with_stdout_to output act)
     >>> Build.action_dyn () ~targets:[output])
