open! Stdune
open Import
open Build.O
open! No_io

module SC = Super_context

let warn_dropped_pp loc ~allow_approx_merlin ~reason =
  if not allow_approx_merlin then
    User_warning.emit ~loc
      [ Pp.textf ".merlin generated is inaccurate. %s." reason
      ; Pp.text
          "Split the stanzas into different directories or silence \
           this warning by adding (allow_approximate_merlin) to your \
           dune-project."
      ]

module Preprocess = struct

  let merge ~allow_approx_merlin
        (a : Dune_file.Preprocess.t) (b : Dune_file.Preprocess.t) =
    match a, b with
    | No_preprocessing, No_preprocessing ->
      Dune_file.Preprocess.No_preprocessing
    | No_preprocessing, pp
    | pp, No_preprocessing ->
      let loc =
        Dune_file.Preprocess.loc pp
        |> Option.value_exn (* only No_preprocessing has no loc*)
      in
      warn_dropped_pp loc ~allow_approx_merlin
        ~reason:"Cannot mix preprocessed and non preprocessed specificiations";
      Dune_file.Preprocess.No_preprocessing
    | (Future_syntax _ as future_syntax), _
    | _, (Future_syntax _ as future_syntax) -> future_syntax
    | Action (loc, a1), Action (_, a2) ->
      if Action_dune_lang.compare_no_locs a1 a2 <> Ordering.Eq then
        warn_dropped_pp loc ~allow_approx_merlin
          ~reason:"this action preprocessor is not equivalent to other \
                   preprocessor specifications.";
      Action (loc, a1)
    | Pps _, Action (loc, _)
    | Action (loc, _), Pps _ ->
      warn_dropped_pp loc ~allow_approx_merlin
        ~reason:"cannot mix action and pps preprocessors";
      No_preprocessing
    | Pps pp1 as pp, Pps pp2 ->
      if Ordering.neq (Dune_file.Preprocess.Pps.compare_no_locs pp1 pp2)
      then begin
        warn_dropped_pp pp1.loc ~allow_approx_merlin
          ~reason:"pps specification isn't identical in all stanzas";
        No_preprocessing
      end
      else
        pp
end

let quote_for_merlin s =
  let s =
    if Sys.win32 then
      (* We need this hack because merlin unescapes backslashes (except when
         protected by single quotes). It is only a problem on windows
         because Filename.quote is using double quotes. *)
      String.escape_only '\\' s
    else
      s
  in
  if String.need_quoting s then
    Filename.quote s
  else
    s

module Dot_file = struct
  let b = Buffer.create 256

  let printf = Printf.bprintf b
  let print = Buffer.add_string b

  let to_string ~obj_dirs ~src_dirs ~flags ~pp ~remaindir =
    let serialize_path = Path.reach ~from:(Path.source remaindir) in
    Buffer.clear b;
    print "EXCLUDE_QUERY_DIR\n";
    Path.Set.iter obj_dirs ~f:(fun p ->
      printf "B %s\n" (serialize_path p));
    Path.Set.iter src_dirs ~f:(fun p ->
      printf "S %s\n" (serialize_path p));
    Option.iter pp ~f:(printf "%s\n");
    begin match flags with
    | [] -> ()
    | flags ->
      print "FLG";
      List.iter flags ~f:(fun f -> printf " %s" (quote_for_merlin f));
      print "\n"
    end;
    Buffer.contents b
end

type t =
  { requires   : Lib.Set.t
  ; flags      : (unit, string list) Build.t
  ; preprocess : Dune_file.Preprocess.t
  ; libname    : Lib_name.Local.t option
  ; source_dirs: Path.Source.Set.t
  ; objs_dirs  : Path.Set.t
  }

let make
      ?(requires=Ok [])
      ~flags
      ?(preprocess=Dune_file.Preprocess.No_preprocessing)
      ?libname
      ?(source_dirs=Path.Source.Set.empty)
      ~modules
      ~obj_dir
      () =
  (* Merlin shouldn't cause the build to fail, so we just ignore errors *)
  let requires =
    match requires with
    | Ok    l -> Lib.Set.of_list l
    | Error _ -> Lib.Set.empty
  in
  let objs_dirs =
    Obj_dir.byte_dir obj_dir
    |> Path.build
    |> Path.Set.singleton
  in
  let flags =
    match Modules.alias_module modules with
    | None -> Ocaml_flags.common flags
    | Some m ->
      Ocaml_flags.prepend_common
        ["-open"; Module.Name.to_string (Module.name m)] flags
      |> Ocaml_flags.common
  in
  { requires
  ; flags      = Build.catch flags    ~on_error:(fun _ -> [])
  ; preprocess
  ; libname
  ; source_dirs
  ; objs_dirs
  }

let add_source_dir t dir =
  { t with source_dirs = Path.Source.Set.add t.source_dirs dir }

let pp_flag_of_action sctx ~expander ~loc ~action
  : (unit, string option) Build.t =
  match (action : Action_dune_lang.t) with
  | Run (exe, args) ->
    let args =
      let open Option.O in
      let* (args, input_file) = List.destruct_last args in
      if String_with_vars.is_var input_file ~name:"input-file" then
        Some args
      else
        None
    in
    begin match args with
    | None -> Build.return None
    | Some args ->
      let action : (Path.t Bindings.t, Action.t) Build.t =
        let targets_dir = Expander.dir expander in
        let targets = Expander.Targets.Forbidden "preprocessing actions" in
        let action = Preprocessing.chdir (Run (exe, args)) in
        Super_context.Action.run sctx
          ~loc
          ~expander
          ~dep_kind:Optional
          ~targets
          ~targets_dir
          action
      in
      let pp_of_action exe args =
        match exe with
        | Error _ -> None
        | Ok exe ->
          (Path.to_absolute_filename exe :: args)
          |> List.map ~f:quote_for_merlin
          |> String.concat ~sep:" "
          |> Filename.quote
          |> sprintf "FLG -pp %s"
          |> Option.some
      in
      Build.return Bindings.empty
      >>>
      action
      >>^ begin function
      | Run (exe, args) -> pp_of_action exe args
      | Chdir (_, Run (exe, args)) -> pp_of_action exe args
      | Chdir (_, Chdir (_, Run (exe, args))) -> pp_of_action exe args
      | _ -> None
      end
    end
  | _ -> Build.return None

let pp_flags sctx ~expander ~dir_kind { preprocess; libname; _ }
  : (unit, string option) Build.t =
  let scope = Expander.scope expander in
  match Dune_file.Preprocess.remove_future_syntax preprocess
          ~for_:Merlin
          (Super_context.context sctx).version
  with
  | Pps { loc; pps; flags; staged = _ } -> begin
    match Preprocessing.get_ppx_driver sctx ~loc ~expander ~lib_name:libname
            ~flags ~scope ~dir_kind pps
    with
    | Error _exn ->
      Build.return None
    | Ok (exe, flags) ->
      (Path.to_absolute_filename (Path.build exe)
       :: "--as-ppx" :: flags)
      |> List.map ~f:quote_for_merlin
      |> String.concat ~sep:" "
      |> Filename.quote
      |> sprintf "FLG -ppx %s"
      |> Option.some
      |> Build.return
  end
  | Action (loc, (action : Action_dune_lang.t)) ->
    pp_flag_of_action sctx ~expander ~loc ~action
  | No_preprocessing ->
    Build.return None

let dot_merlin sctx ~dir ~more_src_dirs ~expander ~dir_kind
      ({ requires; flags; _ } as t) =
  Path.Build.drop_build_context dir
  |> Option.iter ~f:(fun remaindir ->
    let merlin_file = Path.Build.relative dir ".merlin" in
    (* We make the compilation of .ml/.mli files depend on the
       existence of .merlin so that they are always generated, however
       the command themselves don't read the merlin file, so we don't
       want to declare a dependency on the contents of the .merlin
       file.

       Currently dune doesn't support declaring a dependency only
       on the existence of a file, so we have to use this trick. *)
    SC.add_rule sctx ~dir
      (Build.path (Path.build merlin_file)
       >>>
       Build.create_file (Path.Build.relative dir ".merlin-exists"));
    Path.Set.singleton (Path.build merlin_file)
    |> Rules.Produce.Alias.add_deps (Alias.check ~dir);
    let pp_flags = pp_flags sctx ~expander ~dir_kind t in
    SC.add_rule sctx ~dir
      ~mode:(Promote
               { lifetime = Until_clean
               ; into = None
               ; only = None
               }) (
      flags &&& pp_flags
      >>^ (fun (flags, pp) ->
        let (src_dirs, obj_dirs) =
          Lib.Set.fold requires ~init:(
            (Path.set_of_source_paths t.source_dirs)
          , t.objs_dirs)
            ~f:(fun (lib : Lib.t) (src_dirs, obj_dirs) ->
              let info = Lib.info lib in
              let best_src_dir = Lib_info.best_src_dir info in
              ( Path.Set.add src_dirs (Path.drop_optional_build_context best_src_dir)
              , let public_cmi_dir = Obj_dir.public_cmi_dir (Lib.obj_dir lib) in
                Path.Set.add obj_dirs public_cmi_dir
              ))
        in
        let src_dirs =
          Path.Set.union src_dirs (
            Path.Set.of_list (List.map ~f:Path.source more_src_dirs))
        in
        Dot_file.to_string
          ~remaindir
          ~pp
          ~flags
          ~src_dirs
          ~obj_dirs)
      >>>
      Build.write_file_dyn merlin_file))

let merge_two ~allow_approx_merlin a b =
  { requires = Lib.Set.union a.requires b.requires
  ; flags = a.flags &&& b.flags >>^ (fun (a, b) -> a @ b)
  ; preprocess = Preprocess.merge ~allow_approx_merlin a.preprocess b.preprocess
  ; libname =
      (match a.libname with
       | Some _ as x -> x
       | None -> b.libname)
  ; source_dirs = Path.Source.Set.union a.source_dirs b.source_dirs
  ; objs_dirs = Path.Set.union a.objs_dirs b.objs_dirs
  }

let merge_all ~allow_approx_merlin = function
  | [] -> None
  | init :: ts ->
    Some (List.fold_left ~init ~f:(merge_two ~allow_approx_merlin) ts)

let add_rules sctx ~dir ~more_src_dirs ~expander ~dir_kind merlin =
  if (SC.context sctx).merlin then
    dot_merlin sctx ~dir ~more_src_dirs ~expander ~dir_kind merlin
