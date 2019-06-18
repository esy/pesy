open Stdune

type t =
  { dlls : Path.t list
  ; files : Path.t list
  }

let files t = t.files
let dlls t = t.dlls

let all { files; dlls } =
  Path.Set.of_list (List.rev_append dlls files)

module Library = Dune_file.Library

let make ~(ctx : Context.t) ~dir ~dir_contents (lib : Library.t) =
  let { Mode.Dict.byte; native } =
    Dune_file.Mode_conf.Set.eval lib.modes
      ~has_native:(Option.is_some ctx.ocamlopt)
  in
  let if_ cond l = if cond then l else [] in
  let files =
    let virtual_library = Library.is_virtual lib in
    List.concat
      [ if_ (byte && not virtual_library)
          [ Library.archive ~dir lib ~ext:".cma" ]
      ; if virtual_library then (
          let files =
            Dir_contents.c_sources_of_library dir_contents
              ~name:(Library.best_name lib)
          in
          C.Sources.objects files ~dir ~ext_obj:ctx.ext_obj
        ) else if Library.has_stubs lib then (
          [ Library.stubs_archive ~dir lib ~ext_lib:ctx.ext_lib ]
        ) else
          []
      ; if_ (native && not virtual_library)
          (let files =
             [ Library.archive ~dir lib ~ext:".cmxa"
             ; Library.archive ~dir lib ~ext:ctx.ext_lib
             ]
           in
           if Dynlink_supported.get lib.dynlink ctx.natdynlink_supported then
             files @ [ Library.archive ~dir lib ~ext:".cmxs" ]
           else
             files)
      ; List.map lib.buildable.js_of_ocaml.javascript_files ~f:(Path.relative dir)
      ; List.map lib.install_c_headers ~f:(fun fn ->
          Path.relative dir (fn ^ ".h"))
      ]
  in
  let dlls  =
    if_ (byte && Library.has_stubs lib &&
         Dynlink_supported.get lib.dynlink ctx.supports_shared_libraries)
      [Library.dll ~dir lib ~ext_dll:ctx.ext_dll]
  in
  { files
  ; dlls
  }
