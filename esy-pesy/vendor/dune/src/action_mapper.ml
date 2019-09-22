open! Stdune

module Make
    (Src : Action_intf.Ast)
    (Dst : Action_intf.Ast)
= struct
  type map
    = Src.t
    -> dir:Src.path
    -> f_program:(dir:Src.path -> Src.program -> Dst.program)
    -> f_string:(dir:Src.path -> Src.string -> Dst.string)
    -> f_path:(dir:Src.path -> Src.path -> Dst.path)
    -> f_target:(dir:Src.path -> Src.target -> Dst.target)
    -> Dst.t

  let map_one_step f (t : Src.t) ~dir ~f_program ~f_string ~f_path ~f_target : Dst.t =
    let f t ~dir = f t ~dir ~f_program ~f_string ~f_path ~f_target in
    match t with
    | Run (prog, args) ->
      Run (f_program ~dir prog, List.map args ~f:(f_string ~dir))
    | Chdir (fn, t) ->
      Chdir (f_path ~dir fn, f t ~dir:fn)
    | Setenv (var, value, t) ->
      Setenv (f_string ~dir var, f_string ~dir value, f t ~dir)
    | Redirect (outputs, fn, t) ->
      Redirect (outputs, f_target ~dir fn, f t ~dir)
    | Ignore (outputs, t) ->
      Ignore (outputs, f t ~dir)
    | Progn l -> Progn (List.map l ~f:(fun t -> f t ~dir))
    | Echo xs -> Echo (List.map xs ~f:(f_string ~dir))
    | Cat x -> Cat (f_path ~dir x)
    | Copy (x, y) -> Copy (f_path ~dir x, f_target ~dir y)
    | Symlink (x, y) ->
      Symlink (f_path ~dir x, f_target ~dir y)
    | Copy_and_add_line_directive (x, y) ->
      Copy_and_add_line_directive (f_path ~dir x, f_target ~dir y)
    | System x -> System (f_string ~dir x)
    | Bash x -> Bash (f_string ~dir x)
    | Write_file (x, y) -> Write_file (f_target ~dir x, f_string ~dir y)
    | Rename (x, y) -> Rename (f_target ~dir x, f_target ~dir y)
    | Remove_tree x -> Remove_tree (f_target ~dir x)
    | Mkdir x -> Mkdir (f_path ~dir x)
    | Digest_files x -> Digest_files (List.map x ~f:(f_path ~dir))
    | Diff ({ file1; file2 ; _ } as diff) ->
      Diff { diff with
             file1 = f_path ~dir file1
           ; file2 = f_path ~dir file2
           }
    | Merge_files_into (sources, extras, target) ->
      Merge_files_into
        (List.map sources ~f:(f_path ~dir),
         List.map extras ~f:(f_string ~dir),
         f_target ~dir target)

  let rec map t ~dir ~f_program ~f_string ~f_path ~f_target =
    map_one_step map t ~dir ~f_program ~f_string ~f_path ~f_target
end
