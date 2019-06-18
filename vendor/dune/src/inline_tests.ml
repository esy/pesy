open! Stdune
open Import
open Dune_file
open Build.O
open! No_io

module SC = Super_context

module Backend = struct
  module M = struct
    module Info = struct
      let name = Sub_system_name.make "inline_tests.backend"

      type t =
        { loc              : Loc.t
        ; runner_libraries : (Loc.t * Lib_name.t) list
        ; flags            : Ordered_set_lang.Unexpanded.t
        ; generate_runner  : (Loc.t * Action_unexpanded.t) option
        ; extends          : (Loc.t * Lib_name.t) list
        ; file_kind        : Stanza.File_kind.t
        }

      type Sub_system_info.t += T of t

      let loc t = t.loc

      (* The syntax of the driver sub-system is part of the main dune
         syntax, so we simply don't create a new one.

         If we wanted to make the ppx system an extension, then we
         would create a new one.
      *)
      let syntax = Stanza.syntax

      open Stanza.Decoder

      let parse =
        record
          (let+ loc = loc
           and+ runner_libraries = field "runner_libraries" (list (located Lib_name.decode)) ~default:[]
           and+ flags = Ordered_set_lang.Unexpanded.field "flags"
           and+ generate_runner = field_o "generate_runner" (located Action_dune_lang.decode)
           and+ extends = field "extends" (list (located Lib_name.decode)) ~default:[]
           and+ file_kind = Stanza.file_kind ()
           in
           { loc
           ; runner_libraries
           ; flags
           ; generate_runner
           ; extends
           ; file_kind
           })
    end

    type t =
      { info             : Info.t
      ; lib              : Lib.t
      ; runner_libraries : Lib.t list Or_exn.t
      ; extends          :     t list Or_exn.t
      }

    let desc ~plural = "inline tests backend" ^ if plural then "s" else ""
    let desc_article = "an"

    let lib  t = t.lib
    let extends t = t.extends

    let instantiate ~resolve ~get lib (info : Info.t) =
      { info
      ; lib
      ; runner_libraries =
          Result.List.map info.runner_libraries ~f:resolve
      ; extends =
          let open Result.O in
          Result.List.map info.extends ~f:(fun ((loc, name) as x) ->
            let* lib = resolve x in
            match get ~loc lib with
            | None ->
              Error (Errors.exnf loc "%S is not an %s"
                       (Lib_name.to_string name)
                       (desc ~plural:false))
            | Some t -> Ok t)
      }

    let encode t =
      let open Dune_lang.Encoder in
      let lib x = Lib_name.encode (Lib.name x) in
      let f x = Lib_name.encode (Lib.name x.lib) in
      ((1, 0),
       record_fields @@
         [ field_l "runner_libraries" lib (Result.ok_exn t.runner_libraries)
         ; field_i "flags" Ordered_set_lang.Unexpanded.encode_and_upgrade
             t.info.flags
         ; field_o "generate_runner" Action_dune_lang.encode_and_upgrade
             (Option.map t.info.generate_runner ~f:snd)
         ; field_l "extends" f (Result.ok_exn t.extends)
         ])
  end
  include M
  include Sub_system.Register_backend(M)
end

include Sub_system.Register_end_point(
  struct
    module Backend = Backend

    module Info = struct
      let name = Sub_system_name.make "inline_tests"

      type t =
        { loc       : Loc.t
        ; deps      : Dep_conf.t list
        ; flags     : Ordered_set_lang.Unexpanded.t
        ; backend   : (Loc.t * Lib_name.t) option
        ; libraries : (Loc.t * Lib_name.t) list
        }

      type Sub_system_info.t += T of t

      let empty loc =
        { loc
        ; deps      = []
        ; flags     = Ordered_set_lang.Unexpanded.standard
        ; backend   = None
        ; libraries = []
        }

      let loc      t = t.loc
      let backends t = Option.map t.backend ~f:(fun x -> [x])

      let syntax = Stanza.syntax

      open Stanza.Decoder

      let parse =
        if_eos
          ~then_:(loc >>| empty)
          ~else_:
            (record
               (let+ loc = loc
                and+ deps = field "deps" (list Dep_conf.decode) ~default:[]
                and+ flags = Ordered_set_lang.Unexpanded.field "flags"
                and+ backend = field_o "backend" (located Lib_name.decode)
                and+ libraries = field "libraries" (list (located Lib_name.decode)) ~default:[]
                in
                { loc
                ; deps
                ; flags
                ; backend
                ; libraries
                }))
    end

    let gen_rules c ~(info:Info.t) ~backends =
      let { Sub_system.Library_compilation_context.
            super_context = sctx
          ; dir
          ; stanza = lib
          ; scope
          ; source_modules
          ; compile_info = _
          } = c
      in

      let loc = lib.buildable.loc in

      let inline_test_name =
        sprintf "%s.inline-tests" (Lib_name.Local.to_string (snd lib.name))
      in

      let inline_test_dir = Path.relative dir ("." ^ inline_test_name) in

      let obj_dir =
        Obj_dir.make_exe ~dir:inline_test_dir ~name:inline_test_name in

      let name = "run" in
      let main_module_filename = name ^ ".ml" in
      let main_module_name = Module.Name.of_string name in
      let modules =
        Module.Name.Map.singleton main_module_name
          (Module.make main_module_name
             ~impl:{ path   = Path.relative inline_test_dir main_module_filename
                   ; syntax = OCaml
                   }
             ~kind:Impl
             ~visibility:Public
             ~obj_name:name
             ~obj_dir)
      in

      let bindings =
        Pform.Map.singleton "library-name"
          (Values [String (Lib_name.Local.to_string (snd lib.name))])
      in

      let expander = Super_context.expander sctx ~dir in

      let runner_libs =
        let open Result.O in
        let* libs =
          Result.List.concat_map backends
            ~f:(fun (backend : Backend.t) -> backend.runner_libraries)
        in
        let* lib =
          Lib.DB.find_many ~loc
            (Scope.libs scope) [Dune_file.Library.best_name lib]
        in
        let* more_libs =
          Result.List.map info.libraries ~f:(Lib.DB.resolve (Scope.libs scope))
        in
        Lib.closure ~linking:true (lib @ libs @ more_libs)
      in

      (* Generate the runner file *)
      SC.add_rule sctx ~dir ~loc (
        let target = Path.relative inline_test_dir main_module_filename in
        let source_modules = Module.Name.Map.values source_modules in
        let files ml_kind =
          Pform.Var.Values (Value.L.paths (
            List.filter_map source_modules ~f:(fun m ->
              Module.file m ml_kind)))
        in
        let bindings =
          Pform.Map.of_list_exn
            [ "impl-files", files Impl
            ; "intf-files", files Intf
            ]
        in
        let expander = Expander.add_bindings expander ~bindings in
        Build.return Bindings.empty
        >>>
        Build.all
          (List.filter_map backends ~f:(fun (backend : Backend.t) ->
             Option.map backend.info.generate_runner ~f:(fun (loc, action) ->
               SC.Action.run sctx action ~loc
                 ~expander
                 ~dep_kind:Required
                 ~targets:(Forbidden "inline test generators")
                 ~targets_dir:dir)))
        >>^ (fun actions ->
          Action.with_stdout_to target
            (Action.progn actions))
        >>>
        Build.action_dyn ~targets:[target] ());

      let cctx =
        Compilation_context.create ()
          ~super_context:sctx
          ~expander
          ~scope
          ~obj_dir
          ~modules
          ~opaque:false
          ~requires_compile:runner_libs
          ~requires_link:(lazy runner_libs)
          ~flags:(Ocaml_flags.of_list ["-w"; "-24"]);
      in
      Exe.build_and_link cctx
        ~program:{ name; main_module_name ; loc }
        ~linkages:[Exe.Linkage.native_or_custom (SC.context sctx)]
        ~link_flags:(Build.return ["-linkall"]);

      let flags =
        let flags =
          List.map backends ~f:(fun backend ->
            backend.Backend.info.flags) @ [info.flags]
        in
        let expander = Expander.add_bindings expander ~bindings in
        List.map flags ~f:(
          Expander.expand_and_eval_set expander ~standard:(Build.return []))
        |> Build.all
        >>^ List.concat
      in

      SC.add_alias_action sctx ~dir
        ~loc:(Some info.loc)
        (Alias.runtest ~dir)
        ~stamp:("ppx-runner", name)
        (let module A = Action in
         let exe = Path.relative inline_test_dir (name ^ ".exe") in
         Build.path exe >>>
         Build.fanout
           (Super_context.Deps.interpret sctx info.deps ~expander)
           flags
         >>^ fun (_deps, flags) ->
         A.chdir dir
           (A.progn
              (A.run (Ok exe) flags ::
               (Module.Name.Map.values source_modules
                |> List.concat_map ~f:(fun m ->
                  Module.sources m
                  |> List.map ~f:(fun fn ->
                    A.diff ~optional:true
                      fn (Path.extend_basename fn ~suffix:".corrected")))))))
  end)

let linkme = ()
