open Stdune

type t =
  { odig_files : Path.Build.t list
  ; ctx_build_dir : Path.Build.t
  ; lib_stanzas : Dune_file.Library.t Dir_with_dune.t list
  ; installs
    : File_binding.Expanded.t Dune_file.Install_conf.t Dir_with_dune.t list
  ; docs : Dune_file.Documentation.t Dir_with_dune.t list
  ; mlds : Path.Build.t list Lazy.t
  ; coqlibs : Dune_file.Coq.t Dir_with_dune.t list
  ; pkg : Package.t
  ; libs : Lib.Local.Set.t
  ; virtual_lib : Lib.Local.t option Lazy.t
  }

let to_dyn t = Package.to_dyn t.pkg

let hash t =
  Hashtbl.hash
    ( List.hash Path.Build.hash t.odig_files
    , Package.hash t.pkg
    )

let is_odig_doc_file fn =
  List.exists [ "README"; "LICENSE"; "CHANGE"; "HISTORY"]
    ~f:(fun prefix -> String.is_prefix fn ~prefix)

let add_stanzas t ~sctx =
  List.fold_left ~init:t
    ~f:(fun t ({ Dir_with_dune. ctx_dir = dir ; scope = _ ; data
               ; src_dir = _ ; kind = _; dune_version = _ } as d) ->
         let expander = Super_context.expander sctx ~dir in
         let path_expander =
           File_binding.Unexpanded.expand ~dir
             ~f:(Expander.expand_str expander)
         in
         let open Dune_file in
         match data with
         | Install i ->
           let i =
             let files = List.map ~f:path_expander i.files in
             { i with files }
           in
           { t with
             installs = { d with data = i } :: t.installs
           }
         | Library l ->
           { t with
             lib_stanzas = { d with data = l } :: t.lib_stanzas
           }
         | Documentation l ->
           { t with
             docs = { d with data = l } :: t.docs
           }
         | Coq.T l ->
           { t with
             coqlibs = { d with data = l } :: t.coqlibs
           }
         | _ -> t)

let stanzas_to_consider_for_install
      (stanzas : Stanza.t list Dir_with_dune.t list) ~external_lib_deps_mode =
  if external_lib_deps_mode then
    List.concat_map stanzas
      ~f:(fun d ->
        List.map d.data ~f:(fun stanza ->
          { d with data = stanza}))
  else
    List.concat_map stanzas
      ~f:(fun ({ Dir_with_dune.ctx_dir =  _; data = stanzas
               ; scope; kind = _ ; src_dir = _ ; dune_version = _ } as d) ->
           List.filter_map stanzas ~f:(fun stanza ->
             let keep =
               match (stanza : Stanza.t) with
               | Dune_file.Library lib ->
                 Lib.DB.available (Scope.libs scope)
                   (Dune_file.Library.best_name lib)
               | Dune_file.Documentation _
               | Dune_file.Install _ -> true
               | Dune_file.Coq.T d -> Option.is_some d.public
               | _ -> false
             in
             Option.some_if keep { d with data = stanza }))

module Of_sctx = struct
  module Output = struct
    type nonrec t = t Package.Name.Map.t

    let equal = Package.Name.Map.equal ~equal:(==)

    let hash t =
      Package.Name.Map.to_list t
      |> List.hash (fun (k, v) ->
        Hashtbl.hash (Package.Name.hash k, hash v))

    let to_dyn t =
      Dyn.Map (
        Package.Name.Map.to_list t
        |> List.map ~f:(fun (k, v) -> (Package.Name.to_dyn k, to_dyn v)))
  end

  let def =
    let f sctx =
      let ctx = Super_context.context sctx in
      let stanzas =
        let stanzas = Super_context.stanzas sctx in
        let external_lib_deps_mode =
          Super_context.external_lib_deps_mode sctx in
        stanzas_to_consider_for_install stanzas ~external_lib_deps_mode
      in
      let stanzas_per_package =
        List.filter_map stanzas
          ~f:(fun (installable : Stanza.t Dir_with_dune.t) ->
            match Dune_file.stanza_package installable.data with
            | None -> None
            | Some p -> Some (p.name, installable))
        |> Package.Name.Map.of_list_multi
      in
      let libs_of =
        let libs = Super_context.libs_by_package sctx in
        fun (pkg : Package.t) ->
          match Package.Name.Map.find libs pkg.name with
          | Some (_, libs) -> libs
          | None -> Lib.Local.Set.empty
      in
      Super_context.packages sctx
      |> Package.Name.Map.map ~f:(fun (pkg : Package.t) ->
        let odig_files =
          let files = Super_context.source_files sctx ~src_path:pkg.path in
          let pkg_dir = Path.Build.append_source ctx.build_dir pkg.path in
          String.Set.fold files ~init:[] ~f:(fun fn acc ->
            if is_odig_doc_file fn then
              Path.Build.relative pkg_dir fn :: acc
            else
              acc)
        in
        let libs = libs_of pkg in
        let virtual_lib = lazy (
          Lib.Local.Set.find libs ~f:(fun l ->
            let l = Lib.Local.to_lib l in
            let info = Lib.info l in
            let virtual_ = Lib_info.virtual_ info in
            Option.is_some virtual_)
        ) in
        let t =
          add_stanzas
            ~sctx
            { odig_files
            ; lib_stanzas = []
            ; docs = []
            ; installs = []
            ; coqlibs = []
            ; pkg
            ; ctx_build_dir = ctx.build_dir
            ; libs
            ; mlds = lazy (assert false)
            ; virtual_lib
            }
            (Package.Name.Map.find stanzas_per_package pkg.name
             |> Option.value ~default:[])
        in
        { t with mlds = lazy (Packages.mlds sctx pkg.name) }
      )
    in
    Memo.create "of-sctx-def"
      ~doc:"mapping from package names to local packages"
      ~input:(module Super_context.As_memo_key)
      ~output:(Allow_cutoff (module Output))
      ~visibility:Hidden
      Sync
      f
end

let of_sctx sctx = Memo.exec Of_sctx.def sctx

let odig_files t = t.odig_files
let libs t = t.libs
let installs t = t.installs
let lib_stanzas t = t.lib_stanzas
let mlds t = Lazy.force t.mlds

let package t = t.pkg
let opam_file t =
  Path.Build.append_source t.ctx_build_dir (Package.opam_file t.pkg)
let meta_file t =
  Path.Build.append_source t.ctx_build_dir (Package.meta_file t.pkg)
let build_dir t =
  Path.Build.append_source t.ctx_build_dir t.pkg.path

let name t = t.pkg.name
let dune_package_file t =
  Path.Build.relative (build_dir t)
    (Package.Name.to_string (name t) ^ ".dune-package")

let coqlibs t = t.coqlibs

let install_paths t =
  Install.Section.Paths.make ~package:t.pkg.name ~destdir:Path.root ()

let virtual_lib t = Lazy.force t.virtual_lib

let meta_template t =
  Path.Build.extend_basename (meta_file t) ~suffix:".template"

let local_packages_by_dir_def =
  let module Output = struct
    type nonrec t = t list Path.Build.Map.t

    let equal = Path.Build.Map.equal ~equal:(List.equal (==))

    let to_dyn t =
      let open Dyn in
      Dyn.Map (
        Path.Build.Map.to_list t
        |> List.map ~f:(fun (k, v) ->
          let v = List (List.map ~f:to_dyn v) in
          (Path.Build.to_dyn k, v)))
  end
  in
  let f local_packages =
    Package.Name.Map.values local_packages
    |> List.map ~f:(fun pkg ->
      let build_dir = build_dir pkg in
      (build_dir, pkg))
    |> Path.Build.Map.of_list_multi
  in
  Memo.create "local-package-by-dir"
    ~doc:"Map from paths to local packages"
    ~input:(module Of_sctx.Output)
    ~output:(Allow_cutoff (module Output))
    ~visibility:Hidden
    Sync
    f

let local_packages_by_dir = Memo.exec local_packages_by_dir_def

let defined_in sctx ~dir =
  let local_packages = of_sctx sctx in
  let by_build_dir = local_packages_by_dir local_packages in
  Path.Build.Map.find by_build_dir dir
  |> Option.value ~default:[]
