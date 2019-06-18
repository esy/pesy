open! Stdune
open Import
open Stanza.Decoder

(* This file defines the jbuild types as well as the S-expression
   syntax for the various supported version of the specification.
*)

(* Deprecated *)
module Jbuild_version = struct
  type t =
    | V1

  let decode =
    enum
      [ "1", V1
      ]
end

let invalid_module_name ~loc =
  of_sexp_errorf loc "invalid module name: %S"

let module_name =
  plain_string (fun ~loc name ->
    match name with
    | "" -> invalid_module_name ~loc name
    | s ->
      try
        (match s.[0] with
         | 'A'..'Z' | 'a'..'z' -> ()
         | _ -> raise_notrace Exit);
        String.iter s ~f:(function
          | 'A'..'Z' | 'a'..'z' | '0'..'9' | '\'' | '_' -> ()
          | _ -> raise_notrace Exit);
        Module.Name.of_string s
      with Exit ->
        invalid_module_name ~loc name)

let file =
  plain_string (fun ~loc s ->
    match s with
    | "." | ".." ->
      of_sexp_errorf loc "'.' and '..' are not valid filenames"
    | fn -> fn)

let relative_file =
  plain_string (fun ~loc fn ->
    if Filename.is_relative fn then
      fn
    else
      of_sexp_errorf loc "relative filename expected")


let library_variants =
  let syntax =
    Syntax.create ~name:"library_variants"
      ~desc:"the experimental library variants feature."
      [ (0, 1) ]
  in
  Dune_project.Extension.register_simple ~experimental:true
    syntax (Dune_lang.Decoder.return []);
  syntax

let variants_field =
  field_o "variants" (
    let* () = Syntax.since library_variants (0, 1) in
    located (list Variant.decode >>| Variant.Set.of_list))

(* Parse and resolve "package" fields *)
module Pkg = struct
  let listing packages =
    let longest_pkg =
      String.longest_map packages ~f:(fun p ->
        Package.Name.to_string p.Package.name)
    in
    String.concat ~sep:"\n"
      (List.map packages ~f:(fun pkg ->
         sprintf "- %-*s (because of %s)" longest_pkg
           (Package.Name.to_string pkg.Package.name)
           (Path.to_string (Package.opam_file pkg))))

  let default (project : Dune_project.t) stanza =
    match Package.Name.Map.values (Dune_project.packages project) with
    | [pkg] -> Ok pkg
    | [] ->
      Error
        "The current project defines some public elements, \
         but no opam packages are defined.\n\
         Please add a <package>.opam file at the project root \
         so that these elements are installed into it."
    | _ :: _ :: _ ->
      Error
        (sprintf
           "I can't determine automatically which package this \
            stanza is for.\nI have the choice between these ones:\n\
            %s\n\
            You need to add a (package ...) field to this (%s) stanza."
           (listing (Package.Name.Map.values (Dune_project.packages project)))
           stanza)

  let resolve (project : Dune_project.t) name =
    let packages = Dune_project.packages project in
    match Package.Name.Map.find packages name with
    | Some pkg ->
      Ok pkg
    | None ->
      let name_s = Package.Name.to_string name in
      if Package.Name.Map.is_empty packages then
        Error (sprintf
                 "You cannot declare items to be installed without \
                  adding a <package>.opam file at the root of your project.\n\
                  To declare elements to be installed as part of package %S, \
                  add a %S file at the root of your project.\nn\
                 Root of the project as discovered by dune: %s@"
                 name_s (Package.Name.opam_fn name)
                 (Path.to_string_maybe_quoted
                    (Dune_project.in_source_root project)))
      else
        Error (sprintf
                 "The current scope doesn't define package %S.\n\
                  The only packages for which you can declare \
                  elements to be installed in this directory are:\n\
                  %s%s"
                 name_s
                 (listing (Package.Name.Map.values packages))
                 (hint name_s (Package.Name.Map.keys packages
                               |> List.map ~f:Package.Name.to_string)))

  let decode =
    let+ p = Dune_project.get_exn ()
    and+ (loc, name) = located Package.Name.decode in
    match resolve p name with
    | Ok    x -> x
    | Error e -> Errors.fail loc "%s" e

  let field stanza =
    map_validate
      (let+ p = Dune_project.get_exn ()
       and+ pkg = field_o "package" string in
       (p, pkg))
      ~f:(fun (p, pkg) ->
        match pkg with
        | None -> default p stanza
        | Some name -> resolve p (Package.Name.of_string name))
end

module Pps_and_flags = struct
  module Jbuild_syntax = struct
    let of_string ~loc s =
      if String.is_prefix s ~prefix:"-" then
        Right [s]
      else
        Left (loc, Lib_name.of_string_exn ~loc:(Some loc) s)

    let item =
      peek_exn >>= function
      | Template { loc; _ } ->
        no_templates loc "in the preprocessors field"
      | Atom _ | Quoted_string _ -> plain_string of_string
      | List _ -> list string >>| fun l -> Right l

    let split l =
      let pps, flags = List.partition_map l ~f:Fn.id in
      (pps, List.concat flags)

    let decode = list item >>| split
  end

  module Dune_syntax = struct
    let decode =
      let+ l, flags =
        until_keyword "--"
          ~before:(plain_string (fun ~loc s -> (loc, s)))
          ~after:(repeat string)
      in
      let pps, more_flags =
        List.partition_map l ~f:(fun (loc, s) ->
          if String.is_prefix s ~prefix:"-" then
            Right s
          else
            Left (loc, Lib_name.of_string_exn ~loc:(Some loc) s))
      in
      (pps, more_flags @ Option.value flags ~default:[])
  end

  let decode =
    switch_file_kind
      ~jbuild:Jbuild_syntax.decode
      ~dune:Dune_syntax.decode
end

module Dep_conf = struct
  type t =
    | File of String_with_vars.t
    | Alias of String_with_vars.t
    | Alias_rec of String_with_vars.t
    | Glob_files of String_with_vars.t
    | Source_tree of String_with_vars.t
    | Package of String_with_vars.t
    | Universe
    | Env_var of String_with_vars.t

  let remove_locs = function
    | File sw -> File (String_with_vars.remove_locs sw)
    | Alias sw -> Alias (String_with_vars.remove_locs sw)
    | Alias_rec sw -> Alias_rec (String_with_vars.remove_locs sw)
    | Glob_files sw -> Glob_files (String_with_vars.remove_locs sw)
    | Source_tree sw -> Source_tree (String_with_vars.remove_locs sw)
    | Package sw -> Package (String_with_vars.remove_locs sw)
    | Universe -> Universe
    | Env_var sw -> Env_var sw

  let decode =
    let decode =
      let sw = String_with_vars.decode in
      sum
        [ "file"       , (sw >>| fun x -> File x)
        ; "alias"      , (sw >>| fun x -> Alias x)
        ; "alias_rec"  , (sw >>| fun x -> Alias_rec x)
        ; "glob_files" , (sw >>| fun x -> Glob_files x)
        ; "package"    , (sw >>| fun x -> Package x)
        ; "universe"   , return Universe
        ; "files_recursively_in",
          (let+ () =
             Syntax.renamed_in Stanza.syntax (1, 0) ~to_:"source_tree"
           and+ x = sw in
           Source_tree x)
        ; "source_tree",
          (let+ () = Syntax.since Stanza.syntax (1, 0)
           and+ x = sw in
           Source_tree x)
        ; "env_var", (sw >>| fun x -> Env_var x)
        ]
    in
    if_list
      ~then_:decode
      ~else_:(String_with_vars.decode >>| fun x -> File x)

  open Dune_lang
  let encode = function
    | File t ->
      List [ Dune_lang.unsafe_atom_of_string "file"
           ; String_with_vars.encode t ]
    | Alias t ->
      List [ Dune_lang.unsafe_atom_of_string "alias"
           ; String_with_vars.encode t ]
    | Alias_rec t ->
      List [ Dune_lang.unsafe_atom_of_string "alias_rec"
           ; String_with_vars.encode t ]
    | Glob_files t ->
      List [ Dune_lang.unsafe_atom_of_string "glob_files"
           ; String_with_vars.encode t ]
    | Source_tree t ->
      List [ Dune_lang.unsafe_atom_of_string "files_recursively_in"
           ; String_with_vars.encode t ]
    | Package t ->
      List [ Dune_lang.unsafe_atom_of_string "package"
           ; String_with_vars.encode t]
    | Universe ->
      Dune_lang.unsafe_atom_of_string "universe"
    | Env_var t ->
      List [ Dune_lang.unsafe_atom_of_string "env_var"
           ; String_with_vars.encode t]

  let to_sexp t = Dune_lang.to_sexp (encode t)
end

module Preprocess = struct
  type pps =
    { loc : Loc.t
    ; pps : (Loc.t * Lib_name.t) list
    ; flags : string list
    ; staged : bool
    }
  type t =
    | No_preprocessing
    | Action of Loc.t * Action_dune_lang.t
    | Pps    of pps
    | Future_syntax of Loc.t

  let decode =
    sum
      [ "no_preprocessing", return No_preprocessing
      ; "action",
        (located Action_dune_lang.decode >>| fun (loc, x) ->
         Action (loc, x))
      ; "pps",
        (let+ loc = loc
         and+ pps, flags = Pps_and_flags.decode in
         Pps { loc; pps; flags; staged = false })
      ; "staged_pps",
        (let+ () = Syntax.since Stanza.syntax (1, 1)
         and+ loc = loc
         and+ pps, flags = Pps_and_flags.decode in
         Pps { loc; pps; flags; staged = true })
      ; "future_syntax",
        (let+ () = Syntax.since Stanza.syntax (1, 8)
         and+ loc = loc
         in
         Future_syntax loc)
      ]

  let pps = function
    | Pps { pps; _ } -> pps
    | _ -> []

  module Without_future_syntax = struct
    type t =
      | No_preprocessing
      | Action of Loc.t * Action_dune_lang.t
      | Pps    of pps
  end

  let remove_future_syntax t v : Without_future_syntax.t =
    match t with
    | No_preprocessing -> No_preprocessing
    | Action (loc, action) -> Action (loc, action)
    | Pps pps -> Pps pps
    | Future_syntax loc ->
      if Ocaml_version.supports_let_syntax v then
        No_preprocessing
      else
        Action
          (loc,
           Run
             (String_with_vars.make_var loc "libexec"
                ~payload:"dune.configurator:../future-syntax.exe",
              [ String_with_vars.make_text loc "-dump-ast"
              ; String_with_vars.make_var loc "input-file"
              ]))
end

module Blang = struct
  include Blang

  let ops =
    [ "=", Op.Eq
    ; ">=", Gte
    ; "<=", Lt
    ; ">", Gt
    ; "<", Lt
    ; "<>", Neq
    ]

  let decode =
    let ops =
      List.map ops ~f:(fun (name, op) ->
        ( name
        , (let+ x = String_with_vars.decode
           and+ y = String_with_vars.decode
           in
           Compare (op, x, y))))
    in
    let decode =
      fix begin fun t ->
        if_list
          ~then_:(
            [ "or", repeat t >>| (fun x -> Or x)
            ; "and", repeat t >>| (fun x -> And x)
            ] @ ops
            |> sum)
          ~else_:(String_with_vars.decode >>| fun v -> Expr v)
      end
    in
    let+ () = Syntax.since Stanza.syntax (1, 1)
    and+ decode = decode
    in
    decode
end

let enabled_if =
  field "enabled_if" ~default:Blang.true_
    (Syntax.since Stanza.syntax (1, 4) >>> Blang.decode)

module Per_module = struct
  include Per_item.Make(Module.Name)

  let decode ~default a =
    peek_exn >>= function
    | List (loc, Atom (_, A "per_module") :: _) ->
      sum [ "per_module",
            let+ x =
              repeat
                (let+ (pp, names) = pair a (list module_name) in
                (names, pp))
            in
            of_mapping x ~default
            |> function
            | Ok t -> t
            | Error (name, _, _) ->
              of_sexp_errorf loc
                "module %s present in two different sets"
                (Module.Name.to_string name)
          ]
    | _ -> a >>| for_all
end

module Preprocess_map = struct
  type t = Preprocess.t Per_module.t
  let decode = Per_module.decode Preprocess.decode ~default:Preprocess.No_preprocessing

  let no_preprocessing = Per_module.for_all Preprocess.No_preprocessing

  let find module_name t = Per_module.get t module_name

  let default = Per_module.for_all Preprocess.No_preprocessing

  let pps t =
    Per_module.fold t ~init:Lib_name.Map.empty ~f:(fun pp acc ->
      List.fold_left (Preprocess.pps pp) ~init:acc ~f:(fun acc (loc, pp) ->
        Lib_name.Map.add acc pp loc))
    |> Lib_name.Map.foldi ~init:[] ~f:(fun pp loc acc -> (loc, pp) :: acc)
end

module Lint = struct
  type t = Preprocess_map.t

  let decode = Preprocess_map.decode

  let default = Preprocess_map.default
  let no_lint = default
end

let field_oslu name = Ordered_set_lang.Unexpanded.field name

module Js_of_ocaml = struct

  type t =
    { flags            : Ordered_set_lang.Unexpanded.t
    ; javascript_files : string list
    }

  let decode =
    record
      (let+ flags = field_oslu "flags"
       and+ javascript_files = field "javascript_files" (list string) ~default:[]
       in
       { flags; javascript_files })

  let default =
    { flags = Ordered_set_lang.Unexpanded.standard
    ; javascript_files = [] }
end

module Lib_dep = struct
  type choice =
    { required  : Lib_name.Set.t
    ; forbidden : Lib_name.Set.t
    ; file      : string
    }

  type select =
    { result_fn : string
    ; choices   : choice list
    ; loc       : Loc.t (* For error messages *)
    }

  type t =
    | Direct of (Loc.t * Lib_name.t)
    | Select of select

  let choice =
    enter (
      let+ loc = loc
      and+ preds, file =
        until_keyword "->"
          ~before:(let+ s = string
                   and+ loc = loc in
                   let len = String.length s in
                   if len > 0 && s.[0] = '!' then
                     Right (Lib_name.of_string_exn ~loc:(Some loc)
                              (String.drop s 1))
                   else
                     Left (Lib_name.of_string_exn ~loc:(Some loc) s))
          ~after:file
      in
      match file with
      | None ->
        of_sexp_errorf loc "(<[!]libraries>... -> <file>) expected"
      | Some file ->
        let rec loop required forbidden = function
          | [] ->
            let common = Lib_name.Set.inter required forbidden in
            Option.iter (Lib_name.Set.choose common) ~f:(fun name ->
              of_sexp_errorf loc
                "library %S is both required and forbidden in this clause"
                (Lib_name.to_string name));
            { required
            ; forbidden
            ; file
            }
          | Left s :: l ->
            loop (Lib_name.Set.add required s) forbidden l
          | Right s :: l ->
            loop required (Lib_name.Set.add forbidden s) l
        in
        loop Lib_name.Set.empty Lib_name.Set.empty preds)

  let decode =
    if_list
      ~then_:(
        enter
          (let+ loc = loc
           and+ () = keyword "select"
           and+ result_fn = file
           and+ () = keyword "from"
           and+ choices = repeat choice in
           Select { result_fn; choices; loc }))
      ~else_:(
        let+ (loc, name) = located Lib_name.decode in
        Direct (loc, name))

  let to_lib_names = function
    | Direct (_, s) -> [s]
    | Select s ->
      List.fold_left s.choices ~init:Lib_name.Set.empty ~f:(fun acc x ->
        Lib_name.Set.union acc (Lib_name.Set.union x.required x.forbidden))
      |> Lib_name.Set.to_list

  let direct x = Direct x

  let of_lib_name (loc, pp) = Direct (loc, pp)
end

module Lib_deps = struct
  type t = Lib_dep.t list

  type kind =
    | Required
    | Optional
    | Forbidden

  let decode =
    let+ loc = loc
    and+ t = repeat Lib_dep.decode
    in
    let add kind name acc =
      match Lib_name.Map.find acc name with
      | None -> Lib_name.Map.add acc name kind
      | Some kind' ->
        match kind, kind' with
        | Required, Required ->
          of_sexp_errorf loc "library %S is present twice"
            (Lib_name.to_string name)
        | (Optional|Forbidden), (Optional|Forbidden) ->
          acc
        | Optional, Required | Required, Optional ->
          of_sexp_errorf loc
            "library %S is present both as an optional \
             and required dependency"
            (Lib_name.to_string name)
        | Forbidden, Required | Required, Forbidden ->
          of_sexp_errorf loc
            "library %S is present both as a forbidden \
             and required dependency"
            (Lib_name.to_string name)
    in
    ignore (
      List.fold_left t ~init:Lib_name.Map.empty ~f:(fun acc x ->
        match x with
        | Lib_dep.Direct (_, s) -> add Required s acc
        | Select { choices; _ } ->
          List.fold_left choices ~init:acc ~f:(fun acc c ->
            let acc =
              Lib_name.Set.fold c.Lib_dep.required ~init:acc ~f:(add Optional)
            in
            Lib_name.Set.fold c.forbidden ~init:acc ~f:(add Forbidden)))
      : kind Lib_name.Map.t);
    t

  let decode = parens_removed_in_dune decode

  let of_pps pps =
    List.map pps ~f:(fun pp -> Lib_dep.of_lib_name (Loc.none, pp))

  let info t ~kind =
    List.concat_map t ~f:(function
      | Lib_dep.Direct (_, s) -> [(s, kind)]
      | Select { choices; _ } ->
        List.concat_map choices ~f:(fun c ->
          Lib_name.Set.to_list c.Lib_dep.required
          |> List.map ~f:(fun d -> (d, Lib_deps_info.Kind.Optional))))
    |> Lib_name.Map.of_list_reduce ~f:Lib_deps_info.Kind.merge
end

let modules_field name = Ordered_set_lang.field name

module Auto_format = struct
  let syntax =
    Syntax.create ~name:"fmt"
      ~desc:"integration with automatic formatters"
      [ (1, 1) ]

  type language =
    | Ocaml
    | Reason
    | Dune

  let language_to_sexp = function
    | Ocaml -> Sexp.Atom "ocaml"
    | Reason -> Sexp.Atom "reason"
    | Dune -> Sexp.Atom "dune"

  let language =
    sum
      [ ("ocaml", return Ocaml)
      ; ("reason", return Reason)
      ; ("dune",
         let+ () = Syntax.since syntax (1, 1) in
         Dune)
      ]

  type enabled_for =
    | Default of Syntax.Version.t
    | Only of language list

  let enabled_for_field =
    let+ r = field_o "enabled_for" (repeat language)
    and+ version = Syntax.get_exn syntax
    in
    match r with
    | Some l -> Only l
    | None -> Default version

  let enabled_for_to_sexp =
    function
    | Default v -> Sexp.List [Atom "default"; Syntax.Version.to_sexp v]
    | Only l -> List [Atom "only"; List (List.map ~f:language_to_sexp l)]

  type t =
    { loc : Loc.t
    ; enabled_for : enabled_for
    }

  let to_sexp {enabled_for; loc = _} =
    Sexp.List
      [ List [Atom "enabled_for"; enabled_for_to_sexp enabled_for]
      ]

  let dparse_args =
    let+ loc = loc
    and+ enabled_for = record enabled_for_field
    in
    ({loc; enabled_for}, [])

  let key =
    Dune_project.Extension.register syntax dparse_args to_sexp

  let enabled_languages config =
    match config.enabled_for with
    | Default ver ->
      let in_1_0 =
        [Ocaml; Reason]
      in
      let extra =
        match Syntax.Version.compare ver (1, 1) with
        | Lt -> []
        | Eq | Gt -> [Dune]
      in
      in_1_0 @ extra
    | Only l -> l

  let includes config language =
    List.mem language ~set:(enabled_languages config)

  let loc t = t.loc
end

module Buildable = struct
  type t =
    { loc                      : Loc.t
    ; modules                  : Ordered_set_lang.t
    ; modules_without_implementation : Ordered_set_lang.t
    ; libraries                : Lib_dep.t list
    ; preprocess               : Preprocess_map.t
    ; preprocessor_deps        : Dep_conf.t list
    ; lint                     : Preprocess_map.t
    ; flags                    : Ocaml_flags.Spec.t
    ; js_of_ocaml              : Js_of_ocaml.t
    ; allow_overlapping_dependencies : bool
    }

  let decode =
    let+ loc = loc
    and+ preprocess =
      field "preprocess" Preprocess_map.decode ~default:Preprocess_map.default
    and+ preprocessor_deps =
      field "preprocessor_deps" (list Dep_conf.decode) ~default:[]
    and+ lint = field "lint" Lint.decode ~default:Lint.default
    and+ modules = modules_field "modules"
    and+ modules_without_implementation =
      modules_field "modules_without_implementation"
    and+ libraries = field "libraries" Lib_deps.decode ~default:[]
    and+ flags = Ocaml_flags.Spec.decode
    and+ js_of_ocaml =
      field "js_of_ocaml" Js_of_ocaml.decode ~default:Js_of_ocaml.default
    and+ allow_overlapping_dependencies =
      field_b "allow_overlapping_dependencies"
    in
    { loc
    ; preprocess
    ; preprocessor_deps
    ; lint
    ; modules
    ; modules_without_implementation
    ; libraries
    ; flags
    ; js_of_ocaml
    ; allow_overlapping_dependencies
    }

  let single_preprocess t =
    if Per_module.is_constant t.preprocess then
      Per_module.get t.preprocess (Module.Name.of_string "")
    else
      Preprocess.No_preprocessing
end

module Public_lib = struct
  type t =
    { name    : Loc.t * Lib_name.t
    ; package : Package.t
    ; sub_dir : string option
    }

  let name t = snd t.name

  let public_name_field =
    map_validate
      (let+ project = Dune_project.get_exn ()
       and+ loc_name = field_o "public_name" (located Lib_name.decode) in
       (project, loc_name))
      ~f:(fun (project, loc_name) ->
        match loc_name with
        | None -> Ok None
        | Some ((_, s) as loc_name) ->
          let (pkg, rest) = Lib_name.split s in
          match Pkg.resolve project pkg with
          | Ok pkg ->
            Ok (Some
                  { package = pkg
                  ; sub_dir =
                      if rest = [] then None else
                        Some (String.concat rest ~sep:"/")
                  ; name    = loc_name
                  })
          | Error _ as e -> e)
end

module Mode_conf = struct
  module T = struct
    type t =
      | Byte
      | Native
      | Best
    let compare (a : t) b = compare a b
  end
  include T

  let decode =
    enum
      [ "byte"  , Byte
      ; "native", Native
      ; "best"  , Best
      ]

  let to_string = function
    | Byte -> "byte"
    | Native -> "native"
    | Best -> "best"

  let pp fmt t =
    Format.pp_print_string fmt (to_string t)

  let encode t =
    Dune_lang.unsafe_atom_of_string (to_string t)

  module Set = struct
    include Set.Make(T)

    let decode = list decode >>| of_list

    let default = of_list [Byte; Best]

    let eval t ~has_native =
      let has_best = mem t Best in
      let byte = mem t Byte || (has_best && (not has_native)) in
      let native = has_native && (mem t Native || has_best) in
      { Mode.Dict.byte; native }
  end
end

module Library = struct
  module Inherited = struct
    type 'a t =
      | This of 'a
      | From of (Loc.t * Lib_name.t)
  end

  module Stdlib = struct
    type t =
      { modules_before_stdlib : Module.Name.Set.t
      ; exit_module : Module.Name.t option
      ; internal_modules : Glob.t
      }

    let syntax =
      let syntax =
        Syntax.create ~name:"experimental_building_ocaml_compiler_with_dune"
          ~desc:"experimental feature for building the compiler with dune"
          [ (0, 1) ]
      in
      Dune_project.Extension.register_simple ~experimental:true
        syntax (Dune_lang.Decoder.return []);
      syntax

    let decode =
      fields
        (let+ modules_before_stdlib =
           field "modules_before_stdlib" (list module_name) ~default:[]
         and+ exit_module =
           field_o "exit_module" module_name
         and+ internal_modules =
           field "internal_modules" Glob.decode ~default:Glob.empty
         in
         { modules_before_stdlib = Module.Name.Set.of_list modules_before_stdlib
         ; exit_module
         ; internal_modules
         })
  end

  module Wrapped = struct
    include Wrapped

    let value = function
      | Inherited.From _ -> None
      | This s -> Some (to_bool s)

    let default = Simple true

    let make ~wrapped ~implements : t Inherited.t =
      match wrapped, implements with
      | None, None -> This default
      | None, Some w -> From w
      | Some (_loc, w), None -> This w
      | Some (loc, _), Some _ ->
        of_sexp_error loc
          "Wrapped cannot be set for implementations. \
           It is inherited from the virtual library."

    let field = field_o "wrapped" (located decode)
  end

  type t =
    { name                     : (Loc.t * Lib_name.Local.t)
    ; public                   : Public_lib.t option
    ; synopsis                 : string option
    ; install_c_headers        : string list
    ; ppx_runtime_libraries    : (Loc.t * Lib_name.t) list
    ; modes                    : Mode_conf.Set.t
    ; kind                     : Lib_kind.t
    ; c_flags                  : Ordered_set_lang.Unexpanded.t C.Kind.Dict.t
    ; c_names                  : Ordered_set_lang.t option
    ; cxx_names                : Ordered_set_lang.t option
    ; library_flags            : Ordered_set_lang.Unexpanded.t
    ; c_library_flags          : Ordered_set_lang.Unexpanded.t
    ; self_build_stubs_archive : string option
    ; virtual_deps             : (Loc.t * Lib_name.t) list
    ; wrapped                  : Wrapped.t Inherited.t
    ; optional                 : bool
    ; buildable                : Buildable.t
    ; dynlink                  : Dynlink_supported.t
    ; project                  : Dune_project.t
    ; sub_systems              : Sub_system_info.t Sub_system_name.Map.t
    ; no_keep_locs             : bool
    ; dune_version             : Syntax.Version.t
    ; virtual_modules          : Ordered_set_lang.t option
    ; implements               : (Loc.t * Lib_name.t) option
    ; variant                  : Variant.t option
    ; default_implementation   : (Loc.t * Lib_name.t) option
    ; private_modules          : Ordered_set_lang.t option
    ; stdlib                   : Stdlib.t option
    }

  let decode =
    record
      (let+ buildable = Buildable.decode
       and+ loc = loc
       and+ name = field_o "name" Lib_name.Local.decode_loc
       and+ public = Public_lib.public_name_field
       and+ synopsis = field_o "synopsis" string
       and+ install_c_headers =
         field "install_c_headers" (list string) ~default:[]
       and+ ppx_runtime_libraries =
         field "ppx_runtime_libraries" (list (located Lib_name.decode)) ~default:[]
       and+ c_flags = Dune_env.Stanza.c_flags ~since:None
       and+ c_names = field_o "c_names" Ordered_set_lang.decode
       and+ cxx_names = field_o "cxx_names" Ordered_set_lang.decode
       and+ library_flags = field_oslu "library_flags"
       and+ c_library_flags = field_oslu "c_library_flags"
       and+ virtual_deps =
         field "virtual_deps" (list (located Lib_name.decode)) ~default:[]
       and+ modes = field "modes" Mode_conf.Set.decode ~default:Mode_conf.Set.default
       and+ kind = field "kind" Lib_kind.decode ~default:Lib_kind.Normal
       and+ wrapped = Wrapped.field
       and+ optional = field_b "optional"
       and+ self_build_stubs_archive =
         located (field "self_build_stubs_archive" (option string) ~default:None)
       and+ no_dynlink = field_b "no_dynlink"
       and+ no_keep_locs = field_b "no_keep_locs"
                            ~check:(Syntax.deprecated_in Stanza.syntax (1, 7))
       and+ sub_systems =
         let* () = return () in
         Sub_system_info.record_parser ()
       and+ project = Dune_project.get_exn ()
       and+ dune_version = Syntax.get_exn Stanza.syntax
       and+ virtual_modules =
         field_o "virtual_modules" (
           Syntax.since Stanza.syntax (1, 7) >>>
           Ordered_set_lang.decode)
       and+ implements =
         field_o "implements" (
           Syntax.since Stanza.syntax (1, 7) >>>
           located Lib_name.decode)
       and+ variant =
         field_o "variant" (
           Syntax.since library_variants (0, 1) >>>
           located Variant.decode)
       and+ default_implementation =
         field_o "default_implementation" (
           Syntax.since library_variants (0, 1) >>>
           located Lib_name.decode)
       and+ private_modules =
         field_o "private_modules" (
           let* () = Syntax.since Stanza.syntax (1, 2) in
           Ordered_set_lang.decode)
       and+ stdlib =
         field_o "stdlib" (Syntax.since Stdlib.syntax (0, 1) >>> Stdlib.decode)
       in
       let wrapped = Wrapped.make ~wrapped ~implements in
       let name =
         let open Syntax.Version.Infix in
         match name, public with
         | Some (loc, res), _ ->
           let wrapped = Wrapped.value wrapped in
           (loc, Lib_name.Local.validate (loc, res) ~wrapped)
         | None, Some { name = (loc, name) ; _ }  ->
           if dune_version >= (1, 1) then
             match Lib_name.to_local name with
             | Ok m -> (loc, m)
             | Warn _ | Invalid ->
               of_sexp_errorf loc
                 "%s.\n\
                  Public library names don't have this restriction. \
                  You can either change this public name to be a valid library \
                  name or add a \"name\" field with a valid library name."
                 Lib_name.Local.invalid_message
           else
             of_sexp_error loc "name field cannot be omitted before version \
                                1.1 of the dune language"
         | None, None ->
           of_sexp_error loc (
             if dune_version >= (1, 1) then
               "supply at least least one of name or public_name fields"
             else
               "name field is missing"
           )
       in
       Option.both virtual_modules implements
       |> Option.iter ~f:(fun (virtual_modules, (_, impl)) ->
         of_sexp_errorf
           (Ordered_set_lang.loc virtual_modules
            |> Option.value_exn)
           "A library cannot be both virtual and implement %s"
           (Lib_name.to_string impl));
       match virtual_modules, default_implementation with
       | None, Some (loc, _) ->
         of_sexp_error loc
           "Only virtual libraries can specify a default implementation."
       | _ -> ();
         match implements, variant with
         | None, Some (loc, _) ->
           of_sexp_error loc
             "Only implementations can specify a variant."
         | _ -> ();
           let variant = Option.map variant ~f:(fun (_, v) -> v) in
           let self_build_stubs_archive =
             let loc, self_build_stubs_archive = self_build_stubs_archive in
             let err =
               match c_names, cxx_names, self_build_stubs_archive with
               | _, _, None -> None
               | Some _, _, Some _ -> Some "c_names"
               | _, Some _, Some _ -> Some "cxx_names"
               | None, None, _ -> None
             in
             match err with
             | None ->
               self_build_stubs_archive
             | Some name ->
               of_sexp_errorf loc
                 "A library cannot use (self_build_stubs_archive ...) \
                  and (%s ...) simultaneously." name
           in
           { name
           ; public
           ; synopsis
           ; install_c_headers
           ; ppx_runtime_libraries
           ; modes
           ; kind
           ; c_names
           ; c_flags
           ; cxx_names
           ; library_flags
           ; c_library_flags
           ; self_build_stubs_archive
           ; virtual_deps
           ; wrapped
           ; optional
           ; buildable
           ; dynlink = Dynlink_supported.of_bool (not no_dynlink)
           ; project
           ; sub_systems
           ; no_keep_locs
           ; dune_version
           ; virtual_modules
           ; implements
           ; variant
           ; default_implementation
           ; private_modules
           ; stdlib
           })

  let has_stubs t =
    match t.c_names, t.cxx_names, t.self_build_stubs_archive with
    | None, None, None -> false
    | _                -> true

  let stubs_name t =
    let name =
      match t.self_build_stubs_archive with
      | None -> Lib_name.Local.to_string (snd t.name)
      | Some s -> s
    in
    name ^ "_stubs"

  let stubs t ~dir = Path.relative dir (stubs_name t)

  let stubs_archive t ~dir ~ext_lib =
    Path.relative dir (sprintf "lib%s%s" (stubs_name t) ext_lib)

  let dll t ~dir ~ext_dll =
    Path.relative dir (sprintf "dll%s%s" (stubs_name t) ext_dll)

  let archive t ~dir ~ext =
    Path.relative dir (Lib_name.Local.to_string (snd t.name) ^ ext)

  let best_name t =
    match t.public with
    | None -> Lib_name.of_local t.name
    | Some p -> snd p.name

  let is_virtual t = Option.is_some t.virtual_modules
  let is_impl t = Option.is_some t.implements

  let obj_dir ~dir t =
    Obj_dir.make_lib ~dir
      ~has_private_modules:(t.private_modules <> None)
      (snd t.name)

  module Main_module_name = struct
    type t = Module.Name.t option Inherited.t
  end

  let main_module_name t : Main_module_name.t =
    match t.implements, t.wrapped with
    | Some x, From _ -> From x
    | Some _, This _ (* cannot specify for wrapped for implements *)
    | None, From _ -> assert false (* cannot inherit for normal libs *)
    | None, This (Simple false) -> This None
    | None, This (Simple true | Yes_with_transition _) ->
      This (Some (Module.Name.of_local_lib_name (snd t.name)))

  let special_compiler_module t (m : Module.t) =
    match t.stdlib with
    | None -> false
    | Some stdlib ->
      let name = Module.name m in
      Glob.test stdlib.internal_modules (Module.Name.to_string name) ||
      match stdlib.exit_module with
      | None -> false
      | Some n -> n = name
end

module Install_conf = struct
  type 'file t =
    { section : Install.Section.t
    ; files   : 'file list
    ; package : Package.t
    }

  let decode =
    record
      (let+ section = field "section" Install.Section.decode
       and+ files = field "files" File_binding.Unexpanded.L.decode
       and+ package = Pkg.field "install"
       in
       { section
       ; files
       ; package
       })
end

module Executables = struct
  module Names : sig
    type t

    val names : t -> (Loc.t * string) list

    val has_public_name : t -> bool

    val make
      :  multi:bool
      -> stanza:string
      -> allow_omit_names_version:Syntax.Version.t
      -> (t, fields) Dune_lang.Decoder.parser

    val install_conf
      : t
      -> ext:string
      -> (File_binding.Unexpanded.t Install_conf.t, string) Result.t option

    val loc_of_package_field : t -> Loc.t
  end = struct
    type t =
      { names : (Loc.t * string) list
      ; public_names : (Loc.t * string option) list option
      ; package : (Loc.t * Package.Name.t) option
      ; stanza : string
      ; project : Dune_project.t
      ; loc : Loc.t
      ; multi : bool
      ; file_kind : Dune_lang.syntax
      }

    let names t = t.names

    let loc_of_package_field t =
      match t.package with
      | None -> t.loc
      | Some (loc, _) -> loc

    let public_name =
      located string >>| fun (loc, s) ->
      (loc
      , match s with
      | "-" -> None
      | s   -> Some s)

    let multi_fields =
      map_validate
        (let+ names = field_o "names" (list (located string))
         and+ pub_names = field_o "public_names" (list public_name) in
         (names, pub_names))
        ~f:(fun (names, public_names) ->
          match names, public_names with
          | Some names, Some public_names ->
            if List.length public_names = List.length names then
              Ok (Some names, Some public_names)
            else
              Error "The list of public names must be of the same \
                     length as the list of names"
          | names, public_names ->
            Ok (names, public_names))

    let single_fields =
      let+ name = field_o "name" (located string)
      and+ public_name = field_o "public_name" (located string)
      in
      ( Option.map name ~f:List.singleton
      , Option.map public_name ~f:(fun (loc, s) -> [loc, Some s])
      )

    let pluralize s ~multi =
      if multi then
        s ^ "s"
      else
        s

    let make ~multi ~stanza ~allow_omit_names_version =
      let+ names =
        if multi then
          multi_fields
        else
          single_fields
      and+ loc = loc
      and+ dune_syntax = Syntax.get_exn Stanza.syntax
      and+ file_kind = Stanza.file_kind ()
      and+ package =
        field_o "package" (let+ loc = loc
                           and+ s = Package.Name.decode in
                           (loc, s))
      and+ project = Dune_project.get_exn ()
      in
      let (names, public_names) = names in
      let stanza = pluralize stanza ~multi in
      let names =
        let open Syntax.Version.Infix in
        match names, public_names with
        | Some names, _ -> names
        | None, Some public_names ->
          if dune_syntax >= allow_omit_names_version then
            List.map public_names ~f:(fun (loc, p) ->
              match p with
              | None ->
                of_sexp_error loc "This executable must have a name field"
              | Some s -> (loc, s))
          else
            of_sexp_errorf loc
              "%s field may not be omitted before dune version %s"
              (pluralize ~multi "name")
              (Syntax.Version.to_string allow_omit_names_version)
        | None, None ->
          if dune_syntax >= allow_omit_names_version then
            of_sexp_errorf loc "either the %s or the %s field must be present"
              (pluralize ~multi "name")
              (pluralize ~multi "public_name")
          else
            of_sexp_errorf loc "field %s is missing"
              (pluralize ~multi "name")
      in
      { names
      ; public_names
      ; package
      ; project
      ; stanza
      ; loc
      ; multi
      ; file_kind
      }

    let has_public_name t =
      (* user could omit public names by avoiding the field or writing - *)
      match t.public_names with
      | None -> false
      | Some pns -> List.exists ~f:(fun (_, n) -> Option.is_some n) pns

    let package t =
      match t.package with
      | None -> (t.loc, Pkg.default t.project t.stanza)
      | Some (loc, name) -> (loc, Pkg.resolve t.project name)

    let install_conf t ~ext =
      let files =
        let public_names =
          match t.public_names with
          | None -> List.map t.names ~f:(Fn.const (Loc.none, None))
          | Some pns -> pns
        in
        List.map2 t.names public_names
          ~f:(fun (locn, name) (locp, pub) ->
            Option.map pub ~f:(fun pub ->
              File_binding.Unexpanded.make
                ~src:(locn, name ^ ext)
                ~dst:(locp, pub)))
        |> List.filter_opt
      in
      begin match files, t.package with
      | [], None -> None
      | [], Some (loc, _) ->
        let func =
          match t.file_kind with
          | Jbuild -> Errors.warn
          | Dune   -> Errors.fail
        in
        func loc
          "This field is useless without a (%s ...) field."
          (pluralize "public_name" ~multi:t.multi);
        None
      | files, _ ->
        Some (
          let open Result.O in
          let (_loc, package) = package t in
          package >>| fun package ->
          { Install_conf. section = Bin; files; package }
        )
      end
  end

  module Link_mode = struct
    module T = struct
      type t =
        { mode : Mode_conf.t
        ; kind : Binary_kind.t
        ; loc : Loc.t
        }

      let compare a b =
        match compare a.mode b.mode with
        | Eq -> compare a.kind b.kind
        | ne -> ne
    end
    include T

    let make mode kind =
      { mode
      ; kind
      ; loc = Loc.none
      }

    let exe           = make Best Exe
    let object_       = make Best Object
    let shared_object = make Best Shared_object

    let byte_exe           = make Byte Exe

    let native_exe           = make Native Exe
    let native_object        = make Native Object
    let native_shared_object = make Native Shared_object

    let byte   = byte_exe
    let native = native_exe

    let installable_modes =
      [exe; native; byte]

    let simple_representations =
      [ "exe"           , exe
      ; "object"        , object_
      ; "shared_object" , shared_object
      ; "byte"          , byte
      ; "native"        , native
      ]

    let simple =
      Dune_lang.Decoder.enum simple_representations

    let decode =
      if_list
        ~then_:
          (enter
             (let+ mode = Mode_conf.decode
              and+ kind = Binary_kind.decode
              and+ loc = loc in
              { mode; kind; loc}))
        ~else_:simple

    let simple_encode link_mode =
      let is_ok (_, candidate) =
        compare candidate link_mode = Eq
      in
      List.find ~f:is_ok simple_representations
      |> Option.map ~f:(fun (s, _) -> Dune_lang.unsafe_atom_of_string s)

    let encode link_mode =
      match simple_encode link_mode with
      | Some s -> s
      | None ->
        let { mode; kind; loc = _ } = link_mode in
        Dune_lang.Encoder.pair Mode_conf.encode Binary_kind.encode (mode, kind)

    let pp fmt { mode ; kind ; loc = _ } =
      Fmt.record fmt
        [ "mode", Fmt.const Mode_conf.pp mode
        ; "kind", Fmt.const Binary_kind.pp kind
        ]

    module Set = struct
      include Set.Make(T)

      let decode =
        located (list decode) >>| fun (loc, l) ->
        match l with
        | [] -> of_sexp_errorf loc "No linking mode defined"
        | l ->
          let t = of_list l in
          if (mem t native_exe           && mem t exe          ) ||
             (mem t native_object        && mem t object_      ) ||
             (mem t native_shared_object && mem t shared_object) then
            of_sexp_errorf loc
              "It is not allowed use both native and best \
               for the same binary kind."
          else
            t

      let default =
        of_list
          [ byte
          ; exe
          ]

      let best_install_mode t =
        List.find ~f:(mem t) installable_modes
    end
  end

  type t =
    { names      : (Loc.t * string) list
    ; link_flags : Ordered_set_lang.Unexpanded.t
    ; link_deps  : Dep_conf.t list
    ; modes      : Link_mode.Set.t
    ; buildable  : Buildable.t
    ; variants   : (Loc.t * Variant.Set.t) option
    }

  let common =
    let+ buildable = Buildable.decode
    and+ (_ : bool) = field "link_executables" ~default:true
                       (Syntax.deleted_in Stanza.syntax (1, 0) >>> bool)
    and+ link_deps = field "link_deps" (list Dep_conf.decode) ~default:[]
    and+ link_flags = field_oslu "link_flags"
    and+ modes = field "modes" Link_mode.Set.decode ~default:Link_mode.Set.default
    and+ variants = variants_field
    and+ () = map_validate (
      field "inline_tests" (repeat junk >>| fun _ -> true) ~default:false)
      ~f:(function
        | false -> Ok ()
        | true  ->
          Error
            "Inline tests are only allowed in libraries.\n\
             See https://github.com/ocaml/dune/issues/745 for more details.")
    in
    fun names ~multi ->
      let has_public_name = Names.has_public_name names in
      let private_names = Names.names names in
      let t =
        { names = private_names
        ; link_flags
        ; link_deps
        ; modes
        ; buildable
        ; variants
        }
      in
      let install_conf =
        match Link_mode.Set.best_install_mode t.modes with
        | None when has_public_name ->
          let mode_to_string mode =
            " - " ^ Dune_lang.to_string ~syntax:Dune (Link_mode.encode mode) in
          let mode_strings = List.map ~f:mode_to_string Link_mode.installable_modes in
          Errors.fail
            buildable.loc
            "No installable mode found for %s.\n\
             One of the following modes is required:\n\
             %s"
            (if multi then "these executables" else "this executable")
            (String.concat ~sep:"\n" mode_strings)
        | None -> None
        | Some mode ->
          let ext =
            match mode.mode with
            | Native | Best -> ".exe"
            | Byte -> ".bc"
          in
          Names.install_conf names ~ext
      in
      match install_conf with
      | None -> (t, None)
      | Some (Error msg) ->
        let loc = Names.loc_of_package_field names in
        of_sexp_errorf loc "%s" msg
      | Some (Ok install_conf) ->
        (t, Some install_conf)

  let (single, multi) =
    let stanza = "executable" in
    let make multi =
      record
        (let+ names =
           Names.make ~multi ~stanza ~allow_omit_names_version:(1, 1)
         and+ f = common
         in
         f names ~multi)
    in
    (make false, make true)
end

module Rule = struct
  module Targets = struct
    type t =
      (* List of files in the current directory *)
      | Static of String_with_vars.t list
      | Infer

    let decode_static =
      let+ syntax_version = Syntax.get_exn Stanza.syntax
      and+ targets = list String_with_vars.decode
      in
      if syntax_version < (1, 3) then
        List.iter targets ~f:(fun target ->
          if String_with_vars.has_vars target then
            Syntax.Error.since (String_with_vars.loc target)
              Stanza.syntax
              (1, 3)
              ~what:"Using variables in the targets field");
      Static targets
  end


  module Mode = struct
    module Promotion_lifetime = struct
      type t =
        | Unlimited
        | Until_clean
    end

    module Into = struct
      type t =
        { loc : Loc.t
        ; dir : string
        }

      let decode =
        let+ (loc, dir) = located relative_file in
        { loc
        ; dir
        }
    end

    type t =
      | Standard
      | Fallback
      | Promote of Promotion_lifetime.t * Into.t option
      | Not_a_rule_stanza
      | Ignore_source_files

    let decode =
      let promote_into lifetime =
        let+ () = Syntax.since Stanza.syntax (1, 8)
        and+ into = Into.decode in
        Promote (lifetime, Some into)
      in
      sum
        [ "standard"           , return Standard
        ; "fallback"           , return Fallback
        ; "promote"            , return (Promote (Unlimited, None))
        ; "promote-until-clean", return (Promote (Until_clean, None))
        ; "promote-into"       , promote_into Unlimited
        ; "promote-until-clean-into", promote_into Until_clean
        ]

    let field = field "mode" decode ~default:Standard
  end

  type t =
    { targets  : Targets.t
    ; deps     : Dep_conf.t Bindings.t
    ; action   : Loc.t * Action_dune_lang.t
    ; mode     : Mode.t
    ; locks    : String_with_vars.t list
    ; loc      : Loc.t
    ; enabled_if : Blang.t
    }

  type action_or_field = Action | Field

  let atom_table =
    String.Map.of_list_exn
      [ "run"                         , Action
      ; "chdir"                       , Action
      ; "setenv"                      , Action
      ; "with-stdout-to"              , Action
      ; "with-stderr-to"              , Action
      ; "with-outputs-to"             , Action
      ; "ignore-stdout"               , Action
      ; "ignore-stderr"               , Action
      ; "ignore-outputs"              , Action
      ; "progn"                       , Action
      ; "echo"                        , Action
      ; "cat"                         , Action
      ; "copy"                        , Action
      ; "copy#"                       , Action
      ; "copy-and-add-line-directive" , Action
      ; "system"                      , Action
      ; "bash"                        , Action
      ; "write-file"                  , Action
      ; "diff"                        , Action
      ; "diff?"                       , Action
      ; "targets"                     , Field
      ; "deps"                        , Field
      ; "action"                      , Field
      ; "locks"                       , Field
      ; "fallback"                    , Field
      ; "mode"                        , Field
      ]

  let short_form =
    let+ (loc, action) = located Action_dune_lang.decode in
    { targets  = Infer
    ; deps     = Bindings.empty
    ; action   = (loc, action)
    ; mode     = Standard
    ; locks    = []
    ; loc      = loc
    ; enabled_if = Blang.true_
    }

  let long_form =
    let+ loc = loc
    and+ action = field "action" (located Action_dune_lang.decode)
    and+ targets = field "targets" Targets.decode_static
    and+ deps =
      field "deps" (Bindings.decode Dep_conf.decode) ~default:Bindings.empty
    and+ locks = field "locks" (list String_with_vars.decode) ~default:[]
    and+ mode =
      map_validate
        (let+ fallback =
           field_b
             ~check:(Syntax.renamed_in Stanza.syntax (1, 0)
                       ~to_:"(mode fallback)")
             "fallback"
         and+ mode = field_o "mode" Mode.decode
         in
         (fallback, mode))
        ~f:(function
          | true, Some _ ->
            Error "Cannot use both (fallback) and (mode ...) at the \
                   same time.\n\
                   (fallback) is the same as (mode fallback), \
                   please use the latter in new code."
          | false, Some mode -> Ok mode
          | true, None -> Ok Fallback
          | false, None -> Ok Standard)
    and+ enabled_if = enabled_if
    in
    { targets
    ; deps
    ; action
    ; mode
    ; locks
    ; loc
    ; enabled_if
    }

  let jbuild_syntax =
    peek_exn >>= function
    | List (_, (Atom _ :: _)) -> short_form
    | _ -> record long_form

  let dune_syntax =
    peek_exn >>= function
    | List (_, Atom (loc, A s) :: _) -> begin
        match String.Map.find atom_table s with
        | None ->
          of_sexp_errorf loc ~hint:{ on = s
                                   ; candidates = String.Map.keys atom_table
                                   }
            "Unknown action or rule field."
        | Some Field -> fields long_form
        | Some Action -> short_form
      end
    | sexp ->
      of_sexp_errorf (Dune_lang.Ast.loc sexp)
        "S-expression of the form (<atom> ...) expected"

  let decode =
    switch_file_kind
      ~jbuild:jbuild_syntax
      ~dune:dune_syntax

  type lex_or_yacc =
    { modules : string list
    ; mode    : Mode.t
    ; enabled_if : Blang.t
    }

  let ocamllex_jbuild =
    peek_exn >>= function
    | List (_, Atom (_, _) :: _) ->
      enter (
        let+ modules = repeat string in
        { modules
        ; mode  = Standard
        ; enabled_if = Blang.true_
        }
      )
    | _ ->
      record
        (let+ modules = field "modules" (list string)
         and+ mode = Mode.field in
         { modules; mode; enabled_if = Blang.true_ })

  let ocamllex_dune =
    if_eos
      ~then_:(
        return
          { modules = []
          ; mode  = Standard
          ; enabled_if = Blang.true_
          })
      ~else_:(
        if_list
          ~then_:(
            record
              (let+ modules = field "modules" (list string)
               and+ mode = Mode.field
               and+ enabled_if = enabled_if
               in
               { modules; mode; enabled_if }))
          ~else_:(
            repeat string >>| fun modules ->
            { modules
            ; mode  = Standard
            ; enabled_if = Blang.true_
            }))

  let ocamllex =
    switch_file_kind
      ~jbuild:ocamllex_jbuild
      ~dune:ocamllex_dune

  let ocamlyacc = ocamllex

  let ocamllex_to_rule loc { modules; mode; enabled_if } =
    let module S = String_with_vars in
    List.map modules ~f:(fun name ->
      let src = name ^ ".mll" in
      let dst = name ^ ".ml" in
      { targets = Static [S.make_text loc dst]
      ; deps    = Bindings.singleton (Dep_conf.File (S.virt_text __POS__ src))
      ; action  =
          (loc,
           Chdir
             (S.virt_var __POS__ "workspace_root",
              Run (S.virt_text __POS__ "ocamllex",
                   [ S.virt_text __POS__ "-q"
                   ; S.virt_text __POS__ "-o"
                   ; S.virt_var __POS__ "targets"
                   ; S.virt_var __POS__"deps"
                   ])))
      ; mode
      ; locks = []
      ; loc
      ; enabled_if
      })

  let ocamlyacc_to_rule loc { modules; mode; enabled_if } =
    let module S = String_with_vars in
    List.map modules ~f:(fun name ->
      let src = name ^ ".mly" in
      { targets = Static (List.map ~f:(S.make_text loc) [name ^ ".ml"; name ^ ".mli"])
      ; deps    = Bindings.singleton (Dep_conf.File (S.virt_text __POS__ src))
      ; action  =
          (loc,
           Chdir
             (S.virt_var __POS__ "workspace_root",
              Run (S.virt_text __POS__ "ocamlyacc",
                   [S.virt_var __POS__ "deps"])))
      ; mode
      ; locks = []
      ; loc
      ; enabled_if
      })
end

module Menhir = struct
  type t =
    { merge_into : string option
    ; flags      : Ordered_set_lang.Unexpanded.t
    ; modules    : string list
    ; mode       : Rule.Mode.t
    ; loc        :  Loc.t
    ; infer      : bool
    ; enabled_if : Blang.t
    }

  let syntax =
    Syntax.create
      ~name:"menhir"
      ~desc:"the menhir extension"
      [ 1, 1
      ; 2, 0
      ]

  let decode =
    record
      (let+ merge_into = field_o "merge_into" string
       and+ flags = field_oslu "flags"
       and+ modules = field "modules" (list string)
       and+ mode = Rule.Mode.field
       and+ infer = field_o_b "infer" ~check:(Syntax.since syntax (2, 0))
       and+ menhir_syntax = Syntax.get_exn syntax
       and+ enabled_if = enabled_if
       and+ loc = loc
       in
       let infer =
         match infer with
         | Some infer -> infer
         | None -> menhir_syntax >= (2, 0)
       in
       { merge_into
       ; flags
       ; modules
       ; mode
       ; loc
       ; infer
       ; enabled_if
       })

  type Stanza.t += T of t

  let () =
    Dune_project.Extension.register_simple
      syntax
      (return [ "menhir", decode >>| fun x -> [T x] ])

  (* Syntax for jbuild files *)
  let jbuild_syntax =
    record
      (let+ merge_into = field_o "merge_into" string
       and+ flags = field_oslu "flags"
       and+ modules = field "modules" (list string)
       and+ mode = Rule.Mode.field
       and+ loc = loc
       in
       { merge_into
       ; flags
       ; modules
       ; mode
       ; loc
       ; infer = false
       ; enabled_if = Blang.true_
       })
end

module Coq = struct

  type t =
    { name       : Loc.t * Lib_name.Local.t
    (* TODO: validate name *)
    ; public     : Public_lib.t option
    ; synopsis   : string option
    ; modules    : Ordered_set_lang.t
    ; flags      : Ordered_set_lang.Unexpanded.t
    ; libraries  : Lib_dep.t list
    (** ocaml libraries *)
    ; loc        : Loc.t
    ; enabled_if : Blang.t
    }

  let syntax =
    Syntax.create
      ~name:"coq"
      ~desc:"the coq extension (experimental)"
      [ 0, 1 ]

  let decode =
    record
      (let+ name = field "name" Lib_name.Local.decode_loc
       and+ loc = loc
       and+ public = Public_lib.public_name_field
       and+ synopsis = field_o "synopsis" string
       and+ flags = field_oslu "flags"
       and+ modules = modules_field "modules"
       and+ libraries = field "libraries" Lib_deps.decode ~default:[]
       and+ enabled_if = enabled_if
       in
       let name =
         let (loc, res) = name in
         (loc, Lib_name.Local.validate (loc, res) ~wrapped:None)
       in
       { name
       ; public
       ; synopsis
       ; modules
       ; flags
       ; libraries
       ; loc
       ; enabled_if
       })

  let best_name t =
    match t.public with
    | None -> Lib_name.of_local t.name
    | Some p -> snd p.name

  type Stanza.t += T of t

  let unit_to_sexp () = Sexp.List []

  let coqlib_warn x =
    Errors.warn x.loc
      "(coqlib ...) is deprecated and will be removed in the Coq \
       language version 1.0, please use (coq.theory ...) instead";
    x

  let coqlib_p = "coqlib", decode >>| fun x -> [T (coqlib_warn x)]
  let coqtheory_p = "coq.theory", decode >>| fun x -> [T x]

  let unit_stanzas =
    let+ r = return [coqlib_p; coqtheory_p] in
    ((), r)

  let key =
    Dune_project.Extension.register syntax unit_stanzas unit_to_sexp

end


module Alias_conf = struct
  type t =
    { name    : string
    ; deps    : Dep_conf.t Bindings.t
    ; action  : (Loc.t * Action_dune_lang.t) option
    ; locks   : String_with_vars.t list
    ; package : Package.t option
    ; enabled_if : Blang.t
    ; loc : Loc.t
    }

  let alias_name =
    plain_string (fun ~loc s ->
      if Filename.basename s <> s then
        of_sexp_errorf loc "%S is not a valid alias name" s
      else
        s)

  let decode =
    record
      (let+ name = field "name" alias_name
       and+ loc = loc
       and+ package = field_o "package" Pkg.decode
       and+ action = field_o "action" (located Action_dune_lang.decode)
       and+ locks = field "locks" (list String_with_vars.decode) ~default:[]
       and+ deps = field "deps" (Bindings.decode Dep_conf.decode) ~default:Bindings.empty
       and+ enabled_if = field "enabled_if" Blang.decode ~default:Blang.true_
       in
       { name
       ; deps
       ; action
       ; package
       ; locks
       ; enabled_if
       ; loc
       })
end

module Tests = struct
  type t =
    { exes       : Executables.t
    ; locks      : String_with_vars.t list
    ; package    : Package.t option
    ; deps       : Dep_conf.t Bindings.t
    ; enabled_if : Blang.t
    ; action     : Action_dune_lang.t option
    }

  let gen_parse names =
    record
      (let+ buildable = Buildable.decode
       and+ link_flags = field_oslu "link_flags"
       and+ variants = variants_field
       and+ names = names
       and+ package = field_o "package" Pkg.decode
       and+ locks = field "locks" (list String_with_vars.decode) ~default:[]
       and+ modes = field "modes" Executables.Link_mode.Set.decode
                     ~default:Executables.Link_mode.Set.default
       and+ deps =
         field "deps" (Bindings.decode Dep_conf.decode) ~default:Bindings.empty
       and+ enabled_if = enabled_if
       and+ action =
         field_o
           "action"
           (Syntax.since ~fatal:false Stanza.syntax (1, 2) >>> Action_dune_lang.decode)
       in
       { exes =
           { Executables.
             link_flags
           ; link_deps = []
           ; modes
           ; buildable
           ; names
           ; variants
           }
       ; locks
       ; package
       ; deps
       ; enabled_if
       ; action
       })

  let multi = gen_parse (field "names" (list (located string)))

  let single = gen_parse (field "name" (located string) >>| List.singleton)
end

module Toplevel = struct
  type t =
    { name : string
    ; libraries : (Loc.t * Lib_name.t) list
    ; variants : (Loc.t * Variant.Set.t) option
    ; loc : Loc.t
    }

  let decode =
    let open Stanza.Decoder in
    record (
      let+ loc = loc
      and+ name = field "name" string
      and+ variants = variants_field
      and+ libraries =
        field "libraries" (list (located Lib_name.decode)) ~default:[]
      in
      { name
      ; libraries
      ; loc
      ; variants
      }
    )
end

module Copy_files = struct
  type t = { add_line_directive : bool
           ; glob : String_with_vars.t
           ; syntax_version : Syntax.Version.t
           }

  let decode = String_with_vars.decode
end

module Documentation = struct
  type t =
    { loc : Loc.t
    ; package : Package.t
    ; mld_files : Ordered_set_lang.t
    }

  let decode =
    record
      (let+ package = Pkg.field "documentation"
       and+ mld_files = Ordered_set_lang.field "mld_files"
       and+ loc = loc in
       { loc
       ; package
       ; mld_files
       }
      )
end

module Include_subdirs = struct

  type qualification = Unqualified | Qualified
  type t = No | Include of qualification

  let decode ~enable_qualified =
    let opts_list =
      [ "no", No
      ; "unqualified", Include Unqualified
      ] @ if enable_qualified then ["qualified", Include Qualified] else []
    in
    enum opts_list
end

type Stanza.t +=
  | Library         of Library.t
  | Executables     of Executables.t
  | Rule            of Rule.t
  | Install         of File_binding.Unexpanded.t Install_conf.t
  | Alias           of Alias_conf.t
  | Copy_files      of Copy_files.t
  | Documentation   of Documentation.t
  | Tests           of Tests.t
  | Include_subdirs of Loc.t * Include_subdirs.t
  | Toplevel        of Toplevel.t

module Stanzas = struct
  type t = Stanza.t list

  type syntax = OCaml | Plain

  let rules l = List.map l ~f:(fun x -> Rule x)

  let execs (exe, install) =
    match install with
    | None -> [Executables exe]
    | Some i -> [Executables exe; Install i]

  type Stanza.t += Include of Loc.t * string

  type constructors = (string * Stanza.t list Dune_lang.Decoder.t) list

  let stanzas : constructors =
    [ "library",
      (let+ x = Library.decode in
       [Library x])
    ; "executable" , Executables.single >>| execs
    ; "executables", Executables.multi  >>| execs
    ; "rule",
      (let+ loc = loc
       and+ x = Rule.decode in
       [Rule { x with loc }])
    ; "ocamllex",
      (let+ loc = loc
       and+ x = Rule.ocamllex in
       (rules (Rule.ocamllex_to_rule loc x)))
    ; "ocamlyacc",
      (let+ loc = loc
       and+ x = Rule.ocamlyacc in
       rules (Rule.ocamlyacc_to_rule loc x))
    ; "install",
      (let+ x = Install_conf.decode in
       [Install x])
    ; "alias",
      (let+ x = Alias_conf.decode in
       [Alias x])
    ; "copy_files",
      (let+ glob = Copy_files.decode
       and+ syntax_version = Syntax.get_exn Stanza.syntax in
       [Copy_files {add_line_directive = false; glob; syntax_version}])
    ; "copy_files#",
      (let+ glob = Copy_files.decode
       and+ syntax_version = Syntax.get_exn Stanza.syntax in
       [Copy_files {add_line_directive = true; glob; syntax_version}])
    ; "include",
      (let+ loc = loc
       and+ fn = relative_file in
       [Include (loc, fn)])
    ; "documentation",
      (let+ d = Documentation.decode in
       [Documentation d])
    ; "jbuild_version",
      (let+ () = Syntax.deleted_in Stanza.syntax (1, 0)
       and+ _ = Jbuild_version.decode in
       [])
    ; "tests",
      (let+ () = Syntax.since Stanza.syntax (1, 0)
       and+ t = Tests.multi in
       [Tests t])
    ; "test",
      (let+ () = Syntax.since Stanza.syntax (1, 0)
       and+ t = Tests.single in
       [Tests t])
    ; "env",
      (let+ x = Dune_env.Stanza.decode in
       [Dune_env.T x])
    ; "include_subdirs",
      (let* project = Dune_project.get_exn () in
       let+ () = Syntax.since Stanza.syntax (1, 1)
       and+ t =
         let enable_qualified =
           Option.is_some (Dune_project.find_extension_args project Coq.key) in
         Include_subdirs.decode ~enable_qualified
       and+ loc = loc in
       [Include_subdirs (loc, t)])
    ; "toplevel",
      (let+ () = Syntax.since Stanza.syntax (1, 7)
       and+ t = Toplevel.decode in
       [Toplevel t])
    ]

  let jbuild_parser =
    (* The menhir stanza was part of the vanilla jbuild
       syntax. Starting from Dune 1.0, it is presented as an
       extension with its own version. *)
    let stanzas =
      stanzas @
      [ "menhir",
        (let+ loc = loc
         and+ x = Menhir.jbuild_syntax in
         [Menhir.T { x with loc }])
      ]
    in
    Syntax.set Stanza.syntax (0, 0) (sum stanzas)

  let () =
    Dune_project.Lang.register Stanza.syntax stanzas

  let parser ~kind project =
    let syntax_parser (syntax : Dune_lang.Syntax.t) =
      match syntax with
      | Jbuild -> jbuild_parser
      | Dune   -> Dune_project.stanza_parser project
    in
    Dune_project.set project (syntax_parser kind)

  let parse parser = Dune_lang.Decoder.parse parser Univ_map.empty

  let of_ast ~kind (project : Dune_project.t) sexp =
    let parser = parser ~kind project in
    parse parser sexp

  exception Include_loop of Path.t * (Loc.t * Path.t) list

  let rec parse_file_includes ~stanza_parser ~lexer ~current_file
            ~include_stack sexps =
    List.concat_map sexps ~f:(parse stanza_parser)
    |> List.concat_map ~f:(function
      | Include (loc, fn) ->
        let include_stack = (loc, current_file) :: include_stack in
        let dir = Path.parent_exn current_file in
        let current_file = Path.relative dir fn in
        if not (Path.exists current_file) then
          Errors.fail loc "File %s doesn't exist."
            (Path.to_string_maybe_quoted current_file);
        if List.exists include_stack
             ~f:(fun (_, f) -> Path.equal f current_file) then
          raise (Include_loop (current_file, include_stack));
        let sexps = Dune_lang.Io.load ~lexer current_file ~mode:Many in
        parse_file_includes ~stanza_parser ~lexer
          ~current_file ~include_stack sexps
      | stanza -> [stanza])

  let parse ~file ~kind (project : Dune_project.t) sexps =
    let stanza_parser = parser ~kind project in
    let lexer = Dune_lang.Lexer.of_syntax kind in
    let stanzas =
      try
        parse_file_includes ~stanza_parser ~lexer ~include_stack:[]
          ~current_file:file sexps
      with
      | Include_loop (_, []) -> assert false
      | Include_loop (file, last :: rest) ->
        let loc = fst (Option.value (List.last rest) ~default:last) in
        let line_loc (loc, file) =
          sprintf "%s:%d"
            (Path.to_string_maybe_quoted file)
            loc.Loc.start.pos_lnum
        in
        Errors.fail loc
          "Recursive inclusion of jbuild files detected:\n\
           File %s is included from %s%s"
          (Path.to_string_maybe_quoted file)
          (line_loc last)
          (String.concat ~sep:""
             (List.map rest ~f:(fun x ->
                sprintf
                  "\n--> included from %s"
                  (line_loc x))))
    in
    match
      List.filter_map stanzas
        ~f:(function Dune_env.T e -> Some e | _ -> None)
    with
    | _ :: e :: _ ->
      Errors.fail e.loc "The 'env' stanza cannot appear more than once"
    | _ -> stanzas
end

let stanza_package = function
  | Library { public = Some { package; _ }; _ }
  | Alias { package = Some package ;  _ }
  | Install { package; _ }
  | Documentation { package; _ }
  | Tests { package = Some package; _} ->
    Some package
  | Coq.T { public = Some { package; _ }; _ } ->
    Some package
  | _ -> None
