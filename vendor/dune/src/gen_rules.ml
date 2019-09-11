open! Stdune
open Import
module Menhir_rules = Menhir
module Toplevel_rules = Toplevel.Stanza
open Dune_file
open! No_io

module For_stanza = struct
  type ('merlin, 'cctx, 'js, 'source_dirs) t =
    { merlin : 'merlin
    ; cctx   : 'cctx
    ; js     : 'js
    ; source_dirs : 'source_dirs
    }

  let empty_none =
    { merlin = None
    ; cctx = None
    ; js = None
    ; source_dirs = None
    }

  let empty_list =
    { merlin = []
    ; cctx = []
    ; js = []
    ; source_dirs = []
    }

  let cons_maybe hd_o tl =
    match hd_o with
    | Some hd -> hd :: tl
    | None -> tl

  let cons acc x =
    { merlin = cons_maybe x.merlin acc.merlin
    ; cctx = cons_maybe x.cctx acc.cctx
    ; source_dirs = cons_maybe x.source_dirs acc.source_dirs
    ; js =
        match x.js with
        | None -> acc.js
        | Some js -> List.rev_append acc.js js

    }

  let rev t =
    { t with
      merlin = List.rev t.merlin
    ; cctx = List.rev t.cctx
    ; source_dirs = List.rev t.source_dirs
    }
end

module Gen(P : sig val sctx : Super_context.t end) = struct
  module CC = Compilation_context
  module SC = Super_context
  (* We need to instantiate Install_rules earlier to avoid issues whenever
   * Super_context is used too soon.
   * See: https://github.com/ocaml/dune/pull/1354#issuecomment-427922592 *)
  module Lib_rules = Lib_rules.Gen(P)

  let sctx = P.sctx

  let with_format sctx ~dir ~f =
    let scope = SC.find_scope_by_dir sctx dir in
    let project = Scope.project scope in
    Dune_project.find_extension_args project Auto_format.key
    |> Option.iter ~f

  let gen_format_rules sctx ~expander ~output_dir =
    let scope = SC.find_scope_by_dir sctx output_dir in
    let project = Scope.project scope in
    let dialects = Dune_project.dialects project in
    with_format sctx ~dir:output_dir
      ~f:(Format_rules.gen_rules_output sctx ~dialects ~expander ~output_dir)

  (* Stanza *)

  let gen_rules dir_contents cctxs
        { Dir_with_dune. src_dir; ctx_dir; data = stanzas
        ; scope; kind = dir_kind ; dune_version = _ } =
    let expander =
      Super_context.expander sctx ~dir:ctx_dir in
    let for_stanza stanza =
      let dir = ctx_dir in
      match stanza with
      | Toplevel toplevel ->
        Toplevel_rules.setup ~sctx ~dir ~toplevel;
        For_stanza.empty_none
      | Library lib ->
        let cctx, merlin =
          Lib_rules.rules lib ~dir ~scope ~dir_contents ~expander ~dir_kind in
        { For_stanza.
          merlin = Some merlin
        ; cctx = Some (lib.buildable.loc, cctx)
        ; js = None
        ; source_dirs = None
        }
      | Executables exes ->
        let cctx, merlin =
          Exe_rules.rules exes
            ~sctx ~dir ~scope ~expander
            ~dir_contents ~dir_kind
        in
        { For_stanza.
          merlin = Some merlin
        ; cctx = Some (exes.buildable.loc, cctx)
        ; js =
            Some (List.concat_map exes.names ~f:(fun (_, exe) ->
              List.map
                [exe ^ ".bc.js" ; exe ^ ".bc.runtime.js"]
                ~f:(Path.Build.relative ctx_dir)))
        ; source_dirs = None
        }
      | Alias alias ->
        Simple_rules.alias sctx alias ~dir ~expander;
        For_stanza.empty_none
      | Tests tests ->
        let cctx, merlin =
          Test_rules.rules tests ~sctx ~dir ~scope ~expander ~dir_contents
            ~dir_kind
        in
        { For_stanza.
          merlin = Some merlin
        ; cctx = Some (tests.exes.buildable.loc, cctx)
        ; js = None
        ; source_dirs = None
        }
      | Copy_files { glob; _ } ->
        let source_dir =
          let loc = String_with_vars.loc glob in
          let src_glob = Expander.expand_str expander glob in
          Path.Source.relative src_dir src_glob ~error_loc:loc
          |> Path.Source.parent_exn
        in
        { For_stanza.
          merlin = None
        ; cctx = None
        ; js = None
        ; source_dirs = Some source_dir
        }
      | Install { Install_conf. section = _; files; package = _ } ->
        List.map files ~f:(fun fb ->
          File_binding.Unexpanded.expand_src ~dir:ctx_dir
            fb ~f:(Expander.expand_str expander)
          |> Path.build)
        |> Path.Set.of_list
        |> Rules.Produce.Alias.add_deps (Alias.all ~dir:ctx_dir);
        For_stanza.empty_none
      | Cinaps.T cinaps ->
        Cinaps.gen_rules sctx cinaps ~dir ~scope ~dir_kind;
        For_stanza.empty_none
      | _ ->
        For_stanza.empty_none
    in
    let { For_stanza.
          merlin = merlins
        ; cctx = cctxs
        ; js = js_targets
        ; source_dirs
        } = List.fold_left stanzas
              ~init:{ For_stanza.empty_list with cctx = cctxs }
              ~f:(fun acc a -> For_stanza.cons acc (for_stanza a))
            |> For_stanza.rev
    in
    let allow_approx_merlin =
      let dune_project = Scope.project scope in
      let dir_is_vendored = Super_context.dir_is_vendored sctx src_dir in
      dir_is_vendored || Dune_project.allow_approx_merlin dune_project in
    Option.iter (Merlin.merge_all ~allow_approx_merlin merlins)
      ~f:(fun m ->
        let more_src_dirs =
          Dir_contents.dirs dir_contents
          |> List.map ~f:(fun dc ->
            Path.Build.drop_build_context_exn (Dir_contents.dir dc))
          |> List.rev_append source_dirs
        in
        Merlin.add_rules sctx ~dir:ctx_dir ~more_src_dirs ~expander ~dir_kind
          (Merlin.add_source_dir m src_dir));
    let build_dir = Super_context.build_dir sctx in
    List.iter stanzas ~f:(fun stanza ->
      match (stanza : Stanza.t) with
      | Menhir.T m when Expander.eval_blang expander m.enabled_if ->
        begin match
          List.find_map (Menhir_rules.module_names m)
            ~f:(fun name ->
              Option.bind (Dir_contents.lookup_module dir_contents name)
                ~f:(fun buildable ->
                  List.find_map cctxs ~f:(fun (loc, cctx) ->
                    Option.some_if (Loc.equal loc buildable.loc) cctx)))
        with
        | None ->
          (* This happens often when passing a [-p ...] option that
             hides a library *)
          let targets =
            List.map (Menhir_rules.targets m)
              ~f:(Path.Build.relative ctx_dir)
          in
          SC.add_rule sctx ~dir:ctx_dir
            (Build.fail ~targets
               { fail = fun () ->
                   User_error.raise ~loc:m.loc
                     [ Pp.text
                         "I can't determine what library/executable \
                          the files produced by this stanza are part of."
                     ]
               })
        | Some cctx ->
          Menhir_rules.gen_rules cctx m ~build_dir ~dir:ctx_dir
        end
      | Coq.T m when Expander.eval_blang expander m.enabled_if ->
        Coq_rules.setup_rules ~sctx ~dir:ctx_dir ~dir_contents m
        |> SC.add_rules ~dir:ctx_dir sctx
      | Coqpp.T m ->
        Coq_rules.coqpp_rules ~sctx ~build_dir ~dir:ctx_dir m
        |> SC.add_rules ~dir:ctx_dir sctx
      | _ -> ());
    let dyn_deps =
      (* DUNE2: no need to filter out js targets anymore *)
      let pred =
        let id = lazy (
          let open Dyn.Encoder in
          constr "exclude" (List.map ~f:(fun p ->
            Path.Build.to_dyn p) js_targets)
        ) in
        List.iter js_targets ~f:(fun js_target ->
          assert (Path.Build.equal (Path.Build.parent_exn js_target)
                    ctx_dir));
        let f =
          if Dune_project.explicit_js_mode (Scope.project scope) then
            fun _ -> true
          else
            fun basename ->
              not (List.exists js_targets ~f:(fun js_target ->
                String.equal (Path.Build.basename js_target) basename))
        in
        Predicate.create ~id ~f
      in
      File_selector.create ~dir:(Path.build ctx_dir) pred
      |> Build.paths_matching ~loc:Loc.none
    in
    Rules.Produce.Alias.add_deps
      ~dyn_deps
      (Alias.all ~dir:ctx_dir) Path.Set.empty;
    cctxs

  let gen_rules dir_contents cctxs ~dir : (Loc.t * Compilation_context.t) list =
    with_format sctx ~dir ~f:(fun _ -> Format_rules.gen_rules ~dir);
    match SC.stanzas_in sctx ~dir with
    | None -> []
    | Some d -> gen_rules dir_contents cctxs d

  let gen_rules ~dir components : Build_system.extra_sub_directories_to_keep =
    Install_rules.init_meta sctx ~dir;
    let subdirs_to_keep1 = Install_rules.gen_rules sctx ~dir in
    Opam_create.add_rules sctx ~dir;
    let subdirs_to_keep2 : Build_system.extra_sub_directories_to_keep =
      (match components with
       | ".js"  :: rest ->
         Js_of_ocaml_rules.setup_separate_compilation_rules sctx rest;
         (match rest with | [] -> All | _ -> These String.Set.empty)
       | "_doc" :: rest ->
         Odoc.gen_rules sctx rest ~dir;
         (match rest with | [] -> All | _ -> These String.Set.empty)
       | ".ppx"  :: rest ->
         Preprocessing.gen_rules sctx rest;
         (match rest with | [] -> All | _ -> These String.Set.empty)
       | comps ->
         let subdirs = [".formatted"; ".bin"; ".utop"] in
         begin match List.last comps with
         | Some ".formatted" ->
           let expander = Super_context.expander sctx ~dir in
           gen_format_rules sctx ~expander ~output_dir:dir
         | Some ".bin" ->
           let src_dir = Path.Build.parent_exn dir in
           (Super_context.local_binaries sctx ~dir:src_dir
            |> List.iter ~f:(fun t ->
              let loc = File_binding.Expanded.src_loc t in
              let src = Path.build (File_binding.Expanded.src t) in
              let dst = File_binding.Expanded.dst_path t ~dir in
              Super_context.add_rule sctx ~loc ~dir (Build.symlink ~src ~dst)))
         | _ ->
           match
             File_tree.find_dir (SC.file_tree sctx)
               (Path.Build.drop_build_context_exn dir)
           with
           | None ->
             (* We get here when [dir] is a generated directory, such as
                [.utop] or [.foo.objs]. *)
             (if Utop.is_utop_dir dir then
                Utop.setup sctx ~dir:(Path.Build.parent_exn dir)
              else if components <> [] then
                Build_system.load_dir ~dir:(Path.parent_exn (Path.build dir)))
           | Some _ ->
             (* This interprets "rule" and "copy_files" stanzas. *)
             match Dir_contents.gen_rules sctx ~dir with
             | Group_part root ->
               Build_system.load_dir ~dir:(Path.build root)
             | Standalone_or_root (dir_contents, subs) ->
               let cctxs = gen_rules dir_contents [] ~dir in
               List.iter subs ~f:(fun dc ->
                 ignore (
                   gen_rules dir_contents cctxs ~dir:(Dir_contents.dir dc)
                   : _ list))
         end;
         These (String.Set.of_list subdirs))
    in
    let subdirs_to_keep3 =
      match components with
      | [] ->
        Build_system.Subdir_set.These
          (String.Set.of_list [".js"; "_doc"; ".ppx"])
      | _  -> These String.Set.empty
    in
    Build_system.Subdir_set.union_all [
      subdirs_to_keep1;
      subdirs_to_keep2;
      subdirs_to_keep3
    ]

end

module type Gen = sig
  val gen_rules
    :  dir:Path.Build.t
    -> string list
    -> Build_system.extra_sub_directories_to_keep
  val sctx : Super_context.t
end

let filter_out_stanzas_from_hidden_packages ~visible_pkgs =
  List.filter_map ~f:(fun stanza ->
    match Dune_file.stanza_package stanza with
    | None -> Some stanza
    | Some package ->
      if Package.Name.Set.mem visible_pkgs package.name then
        Some stanza
      else
        (* If the stanza is a hidden public implementation of a
           visible library, turn it into an external variant so that
           we can correctly compute the set of "knonw implementation"
           when generating the dune-package file. *)
        match stanza with
        | Library { public = Some { name = (_, name); _ }
                  ; variant = Some variant
                  ; implements = Some (_, virtual_lib)
                  ; project
                  ; buildable = { loc; _ }
                  ; _ }
          when Package.Name.Set.mem visible_pkgs
                 (Lib_name.package_name virtual_lib) ->
          Some
            (External_variant
               { implementation = name
               ; virtual_lib
               ; variant
               ; project
               ; loc
               })
        | _ -> None)

let gen ~contexts
      ?(external_lib_deps_mode=false)
      ?only_packages conf =
  let open Fiber.O in
  let { Dune_load. file_tree; dune_files; packages; projects } = conf in
  let packages =
    match only_packages with
    | None -> packages
    | Some pkgs ->
      Package.Name.Map.filter packages ~f:(fun { Package.name; _ } ->
        Package.Name.Set.mem pkgs name)
  in
  let sctxs = Hashtbl.create 4 in
  List.iter contexts ~f:(fun c ->
    Hashtbl.add_exn sctxs c.Context.name (Fiber.Ivar.create ()));
  let make_sctx (context : Context.t) : _ Fiber.t =
    let host () =
      match context.for_host with
      | None -> Fiber.return None
      | Some h ->
        Fiber.Ivar.read (Hashtbl.find_exn sctxs h.name)
        >>| Option.some
    in

    let stanzas () =
      let+ stanzas = Dune_load.Dune_files.eval ~context dune_files in
      match only_packages with
      | None -> stanzas
      | Some visible_pkgs ->
        List.map stanzas ~f:(fun (dir_conf : Dune_load.Dune_file.t) ->
          { dir_conf with
            stanzas = filter_out_stanzas_from_hidden_packages
                        ~visible_pkgs dir_conf.stanzas
          })
    in
    let* (host, stanzas) = Fiber.fork_and_join host stanzas in
    let sctx =
      Super_context.create
        ?host
        ~context
        ~projects
        ~file_tree
        ~packages
        ~external_lib_deps_mode
        ~stanzas
    in
    let module P = struct let sctx = sctx end in
    let module M = Gen(P) in
    let+ () =
      Fiber.Ivar.fill (Hashtbl.find_exn sctxs context.name) sctx in
    (context.name, (module M : Gen))
  in
  let+ contexts = Fiber.parallel_map contexts ~f:make_sctx in
  let map = String.Map.of_list_exn contexts in
  let sctxs = String.Map.map map ~f:(fun (module M : Gen) -> M.sctx) in
  let generators =
    String.Map.map map ~f:(fun (module M : Gen) -> M.gen_rules) in
  let () =
    Build_system.set_packages (fun path ->
      let open Option.O in
      Option.value ~default:Package.Name.Set.empty (
        let* ctx_name, _ = Path.Build.extract_build_context path in
        let* sctx = String.Map.find sctxs ctx_name in
        Path.Build.Map.find (Install_rules.packages sctx) path))
  in
  Build_system.set_rule_generators
    ~init:(fun () ->
      String.Map.iter map ~f:(fun (module M : Gen) ->
        Odoc.init M.sctx))
    ~gen_rules:
      (function
        | Install ctx ->
          Option.map (String.Map.find sctxs ctx) ~f:(fun sctx ->
            (fun ~dir _ ->
               Install_rules.gen_rules sctx ~dir))
        | Context ctx ->
          String.Map.find generators ctx);
  sctxs
