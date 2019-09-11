open! Stdune
open Import
module Menhir_rules = Menhir
open Dune_file
open! No_io

module Dir_modules = struct
  type t =
    { libraries : Modules.t Lib_name.Map.t
    ; executables : Modules.t String.Map.t
    ; (* Map from modules to the buildable they are part of *)
      rev_map : Buildable.t Module.Name.Map.t
    }

  let empty =
    { libraries = Lib_name.Map.empty
    ; executables = String.Map.empty
    ; rev_map = Module.Name.Map.empty
    }
end

type t =
  { kind : kind
  ; dir : Path.Build.t
  ; text_files : String.Set.t
  ; modules : Dir_modules.t Memo.Lazy.t
  ; c_sources : C_sources.t Memo.Lazy.t
  ; mlds : (Dune_file.Documentation.t * Path.Build.t list) list Memo.Lazy.t
  ; coq_modules : Coq_module.t list Lib_name.Map.t Memo.Lazy.t
  }

and kind =
  | Standalone
  | Group_root of t list
  | Group_part

type gen_rules_result =
  | Standalone_or_root of t * t list
  | Group_part of Path.Build.t

let dir t = t.dir

let dirs t =
  match t.kind with
  | Standalone -> [t]
  | Group_root subs -> t :: subs
  | Group_part ->
    Code_error.raise "Dir_contents.dirs called on a group part"
      [ "dir", Path.Build.to_dyn t.dir ]

let text_files t = t.text_files

let modules_of_library t ~name =
  let map = (Memo.Lazy.force t.modules).libraries in
  Lib_name.Map.find_exn map name

let modules_of_executables t ~obj_dir ~first_exe =
  let map = (Memo.Lazy.force t.modules).executables in
  (* we need to relocate the alias module to its own directory. *)
  let src_dir = Path.build (Obj_dir.obj_dir obj_dir) in
  String.Map.find_exn map first_exe
  |> Modules.relocate_alias_module ~src_dir

let c_sources_of_library t ~name =
  C_sources.for_lib (Memo.Lazy.force t.c_sources) ~name

let lookup_module t name =
  Module.Name.Map.find (Memo.Lazy.force t.modules).rev_map name

let mlds t (doc : Documentation.t) =
  let map = Memo.Lazy.force t.mlds in
  match
    List.find_map map ~f:(fun (doc', x) ->
      Option.some_if (Loc.equal doc.loc doc'.loc) x)
  with
  | Some x -> x
  | None ->
    Code_error.raise "Dir_contents.mlds"
      [ "doc", Loc.to_dyn doc.loc
      ; "available", Dyn.Encoder.(list Loc.to_dyn)
                       (List.map map ~f:(fun (d, _) -> d.Documentation.loc))
      ]

let coq_modules_of_library t ~name =
  let map = Memo.Lazy.force t.coq_modules in
  Lib_name.Map.find_exn map name

let modules_of_files ~dialects ~dir ~files =
  let dir = Path.build dir in
  let make_module dialect base fn =
    (Module.Name.of_string base,
     Module.File.make dialect (Path.relative dir fn))
  in
  let impl_files, intf_files =
    String.Set.to_list files
    |> List.filter_partition_map ~f:(fun fn ->
      (* we aren't using Filename.extension because we want to handle
         filenames such as foo.cppo.ml *)
      match String.lsplit2 fn ~on:'.' with
      | Some (s, ext) ->
        begin match Dialect.DB.find_by_extension dialects ("." ^ ext) with
        | Some (dialect, Ml_kind.Impl) -> Left  (make_module dialect s fn)
        | Some (dialect, Ml_kind.Intf) -> Right (make_module dialect s fn)
        | None                         -> Skip
        end
      | None -> Skip)
  in
  let parse_one_set (files : (Module.Name.t * Module.File.t) list)  =
    match Module.Name.Map.of_list files with
    | Ok x -> x
    | Error (name, f1, f2) ->
      let src_dir = Path.drop_build_context_exn dir in
      User_error.raise
        [ Pp.textf "Too many files for module %s in %s:"
            (Module.Name.to_string name)
            (Path.Source.to_string_maybe_quoted src_dir)
        ; Pp.textf "- %s" (Path.to_string_maybe_quoted f1.path)
        ; Pp.textf "- %s" (Path.to_string_maybe_quoted f2.path)
        ]
  in
  let impls = parse_one_set impl_files in
  let intfs = parse_one_set intf_files in
  Module.Name.Map.merge impls intfs ~f:(fun name impl intf ->
    Some (Module.Source.make name ?impl ?intf))

let build_mlds_map (d : _ Dir_with_dune.t) ~files =
  let dir = d.ctx_dir in
  let mlds = Memo.lazy_ (fun () -> (
      String.Set.fold files ~init:String.Map.empty ~f:(fun fn acc ->
        match String.lsplit2 fn ~on:'.' with
        | Some (s, "mld") -> String.Map.set acc s fn
        | _ -> acc)))
  in
  List.filter_map d.data ~f:(function
    | Documentation doc ->
      let mlds =
        let mlds = Memo.Lazy.force mlds in
        Ordered_set_lang.String.eval_unordered doc.mld_files
          ~parse:(fun ~loc s ->
            match String.Map.find mlds s with
            | Some s ->
              s
            | None ->
              User_error.raise ~loc
                [ Pp.textf "%s.mld doesn't exist in %s" s
                    (Path.to_string_maybe_quoted
                       (Path.drop_optional_build_context (Path.build dir)))
                ]
          )
          ~standard:mlds
      in
      Some (doc, List.map (String.Map.values mlds) ~f:(Path.Build.relative dir))
    | _ -> None)

let coq_modules_of_files ~subdirs =
  let filter_v_files (dir, local, files) =
    (dir, local, String.Set.filter files ~f:(fun f -> Filename.check_suffix f ".v")) in
  let subdirs = List.map subdirs ~f:filter_v_files in
  let build_mod_dir (dir, prefix, files) =
    String.Set.to_list files |> List.map ~f:(fun file ->
      let name, _ = Filename.split_extension file in
      let name = Coq_module.Name.make name in
      Coq_module.make
        ~source:(Path.Build.relative dir file)
        ~prefix ~name) in
  let modules = List.concat_map ~f:build_mod_dir subdirs in
  modules

(* TODO: Build reverse map and check duplicates, however, are duplicates harmful?
 * In Coq all libs are "wrapped" so including a module twice is not so bad.
 *)
let build_coq_modules_map (d : _ Dir_with_dune.t) ~dir ~modules =
  List.fold_left d.data ~init:Lib_name.Map.empty ~f:(fun map -> function
    | Coq.T coq ->
      let modules = Coq_module.Eval.eval coq.modules
        ~parse:(Coq_module.parse ~dir) ~standard:modules in
      Lib_name.Map.set map (Dune_file.Coq.best_name coq) modules
    | _ -> map)

module rec Load : sig
  val get : Super_context.t -> dir:Path.Build.t -> t
  val gen_rules : Super_context.t -> dir:Path.Build.t -> gen_rules_result
end = struct
  let virtual_modules sctx vlib =
    let info = Lib.info vlib in
    let modules =
      match Option.value_exn (Lib_info.virtual_ info) with
      | External modules -> modules
      | Local ->
        let src_dir =
          Lib_info.src_dir info
          |> Path.as_in_build_dir_exn
        in
        let t = Load.get sctx ~dir:src_dir in
        modules_of_library t ~name:(Lib.name vlib)
    in
    let existing_virtual_modules = Modules.virtual_module_names modules in
    let allow_new_public_modules =
      Modules.wrapped modules
      |> Wrapped.to_bool
      |> not
    in
    { Modules_field_evaluator.Implementation.
      existing_virtual_modules
    ; allow_new_public_modules
    }

  let make_modules sctx (d : _ Dir_with_dune.t) ~modules =
    let scope = d.scope in
    let libs, exes =
      List.filter_partition_map d.data ~f:(fun stanza ->
        match (stanza : Stanza.t) with
        | Library lib ->
          let src_dir = d.ctx_dir in
          let kind, main_module_name, wrapped =
            match lib.implements with
            | None ->
              (* In the two following pattern matching, we can only
                 get [From _] if [lib] is an implementation.  Since we
                 know that it is not one because of the above [match
                 lib.implements with ...], we know that we can't get
                 [From _]. That's why we have these [assert false]. *)
              let main_module_name =
                match Library.main_module_name lib with
                | This x -> x
                | From _ -> assert false
              in
              let wrapped =
                match lib.wrapped with
                | This x -> x
                | From _ -> assert false
              in
              let kind : Modules_field_evaluator.kind =
                match lib.virtual_modules with
                | None -> Exe_or_normal_lib
                | Some virtual_modules -> Virtual { virtual_modules }
              in
              (kind, main_module_name, wrapped)
            | Some _ ->
              assert (Option.is_none lib.virtual_modules);
              let resolved =
                let name = Library.best_name lib in
                Lib.DB.find_even_when_hidden (Scope.libs scope) name
                (* can't happen because this library is defined using
                   the current stanza *)
                |> Option.value_exn
              in
              (* diml: this [Result.ok_exn] means that if the user
                 writes an invalid [implements] field, we will get an
                 error immediately even if the library is not
                 built. We should change this to carry the [Or_exn.t]
                 a bit longer. *)
              let vlib = Result.ok_exn (
                (* This [Option.value_exn] is correct because the
                   above [lib.implements] is [Some _] and this [lib]
                   variable correspond to the same library. *)
                Option.value_exn (Lib.implements resolved))
              in
              let kind : Modules_field_evaluator.kind =
                Implementation (virtual_modules sctx vlib)
              in
              let main_module_name, wrapped =
                Result.ok_exn (
                  let open Result.O in
                  let* main_module_name = Lib.main_module_name resolved in
                  let+ wrapped = Lib.wrapped resolved in
                  (main_module_name, Option.value_exn wrapped))
              in
              (kind, main_module_name, wrapped)
          in
          let modules =
            Modules_field_evaluator.eval ~modules
              ~buildable:lib.buildable
              ~kind
              ~private_modules:(
                Option.value ~default:Ordered_set_lang.standard
                  lib.private_modules)
          in
          Left ( lib
               , Modules.lib ~lib ~src_dir ~modules ~main_module_name ~wrapped
               )
        | Executables exes
        | Tests { exes; _} ->
          let modules =
            Modules_field_evaluator.eval ~modules
              ~buildable:exes.buildable
              ~kind:Modules_field_evaluator.Exe_or_normal_lib
              ~private_modules:Ordered_set_lang.standard
          in
          let modules =
            let project = Scope.project scope in
            if Dune_project.wrapped_executables project then
              Modules.exe_wrapped ~src_dir:d.ctx_dir ~modules
            else
              Modules.exe_unwrapped modules
          in
          Right (exes, modules)
        | _ -> Skip)
    in
    let libraries =
      match
        Lib_name.Map.of_list_map libs ~f:(fun (lib, m) -> Library.best_name lib, m)
      with
      | Ok x -> x
      | Error (name, _, (lib2, _)) ->
        User_error.raise ~loc:lib2.buildable.loc
          [ Pp.textf "Library %S appears for the second time in this \
                      directory"
              (Lib_name.to_string name)
          ]
    in
    let executables =
      match
        String.Map.of_list_map exes
          ~f:(fun (exes, m) -> snd (List.hd exes.names), m)
      with
      | Ok x -> x
      | Error (name, _, (exes2, _)) ->
        User_error.raise ~loc:exes2.buildable.loc
          [ Pp.textf "Executable %S appears for the second time in \
                      this directory"
              name
          ]
    in
    let rev_map =
      let rev_modules =
        let by_name buildable =
          Modules.fold_user_written ~init:[] ~f:(fun m acc ->
            (Module.name m, buildable) :: acc)
        in
        List.rev_append
          (List.concat_map libs ~f:(fun (l, m) -> by_name l.buildable m))
          (List.concat_map exes ~f:(fun (e, m) -> by_name e.buildable m))
      in
      match d.kind with
      | Dune -> begin
          match Module.Name.Map.of_list rev_modules with
          | Ok x -> x
          | Error (name, _, _) ->
            let open Module.Name.Infix in
            let locs =
              List.filter_map rev_modules ~f:(fun (n, b) ->
                Option.some_if (n = name) b.loc)
              |> List.sort ~compare
            in
            User_error.raise ~loc:(Loc.drop_position (List.hd locs))
              [ Pp.textf "Module %S is used in several stanzas:"
                  (Module.Name.to_string name)
              ; Pp.enumerate locs ~f:(fun loc ->
                  Pp.verbatim (Loc.to_file_colon_line loc))
              ; Pp.text
                  "To fix this error, you must specify an explicit \
                   \"modules\" field in every library, executable, and \
                   executables stanzas in this dune file. Note that \
                   each module cannot appear in more than one \
                   \"modules\" field - it must belong to a single \
                   library or executable."
              ]
        end
      | Jbuild ->
        Module.Name.Map.of_list_multi rev_modules
        |> Module.Name.Map.mapi ~f:(fun name buildables ->
          match buildables with
          | [] -> assert false
          | [b] -> b
          | b :: rest ->
            let locs =
              List.sort ~compare
                (b.Buildable.loc :: List.map rest ~f:(fun b -> b.Buildable.loc))
            in
            (* DUNE2: make this an error *)
            User_warning.emit ~loc:(Loc.drop_position b.loc)
              [ Pp.textf "Module %S is used in several stanzas:"
                  (Module.Name.to_string name)
              ; Pp.enumerate locs ~f:(fun loc ->
                  Pp.verbatim (Loc.to_file_colon_line loc))
              ; Pp.text
                  "To remove this warning, you must specify an \
                   explicit \"modules\" field in every library, \
                   executable, and executables stanzas in this jbuild \
                   file. Note that each module cannot appear in more \
                   than one \"modules\" field - it must belong to a \
                   single library or executable."
              ];
            b)
    in
    { Dir_modules. libraries; executables; rev_map }

  (* As a side-effect, setup user rules and copy_files rules. *)
  let load_text_files sctx ft_dir
        { Dir_with_dune.
          ctx_dir = dir
        ; src_dir
        ; scope = _
        ; data = stanzas
        ; kind = _
        ; dune_version = _
        } =
    (* Interpret a few stanzas in order to determine the list of
       files generated by the user. *)
    let expander = Super_context.expander sctx ~dir in
    let generated_files =
      List.concat_map stanzas ~f:(fun stanza ->
        match (stanza : Stanza.t) with
        | Coqpp.T { modules; _ } ->
          List.map modules ~f:(fun m -> m ^ ".ml")
        | Menhir.T menhir ->
          Menhir_rules.targets menhir
        | Rule rule ->
          Simple_rules.user_rule sctx rule ~dir ~expander
          |> Path.Build.Set.to_list
          |> List.map ~f:Path.Build.basename
        | Copy_files def ->
          Simple_rules.copy_files sctx def ~src_dir ~dir ~expander
          |> Path.Set.to_list
          |> List.map ~f:Path.basename
        | Library { buildable; _ } | Executables { buildable; _ } ->
          (* Manually add files generated by the (select ...)
             dependencies *)
          List.filter_map buildable.libraries ~f:(fun dep ->
            match (dep : Dune_file.Lib_dep.t) with
            | Direct _ -> None
            | Select s -> Some s.result_fn)
        | _ -> [])
      |> String.Set.of_list
    in
    String.Set.union generated_files (File_tree.Dir.files ft_dir)


  type result0_here = {
    t : t;
    (* [rules] includes rules for subdirectories too *)
    rules : Rules.t option;
    subdirs : t Path.Build.Map.t;
  }

  type result0 =
    | See_above of Path.Build.t
    | Here of result0_here

  module Key = struct
    module Super_context = Super_context.As_memo_key

    type t = Super_context.t * Path.Build.t

    let to_dyn (sctx, path) =
      Dyn.Tuple [Super_context.to_dyn sctx; Path.Build.to_dyn path;]

    let equal = Tuple.T2.equal Super_context.equal Path.Build.equal
    let hash = Tuple.T2.hash Super_context.hash Path.Build.hash
  end

  let check_no_qualified loc qualif_mode =
    if qualif_mode = Include_subdirs.Qualified then
      User_error.raise ~loc
        [ Pp.text "(include_subdirs qualified) is not supported yet" ]

  let check_no_unqualified loc qualif_mode =
    if qualif_mode = Include_subdirs.Unqualified then
      User_error.raise ~loc
        [ Pp.text "(include_subdirs qualified) is not supported yet" ]

  let get0_impl (sctx, dir) : result0 =
    let dir_status_db = Super_context.dir_status_db sctx in
    match Dir_status.DB.get dir_status_db ~dir with
    | Standalone x ->
      (match x with
       | Some (ft_dir, Some d) ->
         let files, rules =
           Rules.collect_opt (fun () -> load_text_files sctx ft_dir d)
         in
         let dialects = Dune_project.dialects (Scope.project d.scope) in
         Here {
           t = { kind = Standalone
               ; dir
               ; text_files = files
               ; modules = Memo.lazy_ (fun () ->
                   make_modules sctx d
                     ~modules:(modules_of_files ~dialects ~dir:d.ctx_dir ~files))
               ; mlds = Memo.lazy_ (fun () -> build_mlds_map d ~files)
               ; c_sources = Memo.lazy_ (fun () ->
                   let dune_version = d.dune_version in
                   C_sources.make d
                     ~c_sources:(
                       C_sources.load_sources ~dune_version ~dir:d.ctx_dir ~files))
               ; coq_modules = Memo.lazy_ (fun () ->
                   build_coq_modules_map d ~dir:d.ctx_dir
                     ~modules:(
                       coq_modules_of_files ~subdirs:[dir,[],files]))
               };
           rules;
           subdirs = Path.Build.Map.empty;
         }
       | Some (_, None)
       | None ->
         Here {
           t = { kind = Standalone
               ; dir
               ; text_files = String.Set.empty
               ; modules = Memo.Lazy.of_val Dir_modules.empty
               ; mlds = Memo.Lazy.of_val []
               ; c_sources = Memo.Lazy.of_val C_sources.empty
               ; coq_modules = Memo.Lazy.of_val Lib_name.Map.empty
               };
           rules = None;
           subdirs = Path.Build.Map.empty;
         })
    | Is_component_of_a_group_but_not_the_root { group_root; _ } ->
      See_above group_root
    | Group_root (ft_dir, qualif_mode, d) ->
      let rec walk ft_dir ~dir ~local acc =
        match
          Dir_status.DB.get dir_status_db ~dir
        with
        | Is_component_of_a_group_but_not_the_root { stanzas = d; group_root = _ } ->
          let files =
            match d with
            | None -> File_tree.Dir.files ft_dir
            | Some d -> load_text_files sctx ft_dir d
          in
          walk_children ft_dir ~dir ~local ((dir, List.rev local, files) :: acc)
        | _ -> acc
      and walk_children ft_dir ~dir ~local acc =
        String.Map.foldi (File_tree.Dir.sub_dirs ft_dir) ~init:acc
          ~f:(fun name ft_dir acc ->
            let dir = Path.Build.relative dir name in
            let local = if qualif_mode = Qualified then name :: local else local in
            walk ft_dir ~dir ~local acc)
      in
      let (files, (subdirs : (Path.Build.t * _ * _) list)), rules =
        Rules.collect_opt (fun () ->
          let files = load_text_files sctx ft_dir d in
          let subdirs = walk_children ft_dir ~dir ~local:[] [] in
          files, subdirs)
      in
      let modules = Memo.lazy_ (fun () ->
        check_no_qualified Loc.none qualif_mode;
        let modules =
          let dialects = Dune_project.dialects (Scope.project d.scope) in
          List.fold_left ((dir, [], files) :: subdirs) ~init:Module.Name.Map.empty
            ~f:(fun acc ((dir : Path.Build.t), _local, files) ->
              let modules = modules_of_files ~dialects ~dir ~files in
              Module.Name.Map.union acc modules ~f:(fun name x y ->
                User_error.raise
                  ~loc:(Loc.in_file
                          (Path.source (match File_tree.Dir.dune_file ft_dir with
                             | None ->
                               Path.Source.relative (File_tree.Dir.path ft_dir)
                                 "_unknown_"
                             | Some d -> File_tree.Dune_file.path d)))
                  [ Pp.textf "Module %S appears in several directories:"
                      (Module.Name.to_string name)
                  ; Pp.textf "- %s"
                      (Path.to_string_maybe_quoted (Module.Source.src_dir x))
                  ; Pp.textf "- %s"
                      (Path.to_string_maybe_quoted (Module.Source.src_dir y))
                  ; Pp.text "This is not allowed, please rename one of them."
                  ]))
        in
        make_modules sctx d ~modules)
      in
      let c_sources = Memo.lazy_ (fun () ->
        check_no_qualified Loc.none qualif_mode;
        let dune_version = d.dune_version in
        let init = C.Kind.Dict.make_both String.Map.empty in
        let c_sources =
          List.fold_left ((dir, [], files) :: subdirs) ~init
            ~f:(fun acc (dir, _local, files) ->
              let sources = C_sources.load_sources ~dir ~dune_version ~files in
              let f acc sources =
                String.Map.union acc sources ~f:(fun name x y ->
                  User_error.raise
                    ~loc:(Loc.in_file
                            (Path.source (match File_tree.Dir.dune_file ft_dir with
                               | None ->
                                 Path.Source.relative (File_tree.Dir.path ft_dir)
                                   "_unknown_"
                               | Some d -> File_tree.Dune_file.path d)))
                    [ Pp.textf "%s file %s appears in several directories:"
                        (C.Kind.to_string (C.Source.kind x))
                        name
                    ; Pp.textf "- %s"
                        (Path.to_string_maybe_quoted
                           (Path.drop_optional_build_context
                              (Path.build (C.Source.src_dir x))))
                    ; Pp.textf "- %s"
                        (Path.to_string_maybe_quoted
                           (Path.drop_optional_build_context
                              (Path.build (C.Source.src_dir y))))
                    ; Pp.text "This is not allowed, please rename one of them."
                    ])
              in
              C.Kind.Dict.merge acc sources ~f)
        in
        C_sources.make d ~c_sources
      ) in
      let coq_modules = Memo.lazy_ (fun () ->
        check_no_unqualified Loc.none qualif_mode;
        build_coq_modules_map d ~dir:d.ctx_dir
          ~modules:(coq_modules_of_files ~subdirs:((dir,[],files)::subdirs))) in
      let subdirs =
        List.map subdirs ~f:(fun (dir, _local, files) ->
          { kind = Group_part
          ; dir
          ; text_files = files
          ; modules
          ; c_sources
          ; mlds = Memo.lazy_ (fun () -> (build_mlds_map d ~files))
          ; coq_modules
          })
      in
      let t =
        { kind = Group_root subdirs
        ; dir
        ; text_files = files
        ; modules
        ; c_sources
        ; mlds = Memo.lazy_ (fun () -> build_mlds_map d ~files)
        ; coq_modules
        }
      in
      Here {
        t;
        rules;
        subdirs = Path.Build.Map.of_list_map_exn subdirs ~f:(fun x -> x.dir, x)
      }

  let memo0 =
    let module Output = struct
      type t = result0
      let to_dyn _ = Dyn.Opaque
    end
    in
    Memo.create
      "dir-contents-get0"
      ~input:(module Key)
      ~output:(Simple (module Output))
      ~doc:"dir contents"
      ~visibility:Hidden
      Sync
      get0_impl

  let get sctx ~dir =
    match Memo.exec memo0 (sctx, dir) with
    | Here { t; rules = _; subdirs = _ } -> t
    | See_above group_root ->
      match Memo.exec memo0 (sctx, group_root) with
      | See_above _ -> assert false
      | Here { t; rules = _; subdirs = _ } -> t

  let gen_rules sctx ~dir =
    match Memo.exec memo0 (sctx, dir) with
    | See_above group_root ->
      Group_part group_root
    | Here { t; rules; subdirs } ->
      Rules.produce_opt rules;
      Standalone_or_root (t, Path.Build.Map.values subdirs)
end

include Load
