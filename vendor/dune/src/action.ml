open! Stdune
open Import

module Outputs = Action_ast.Outputs

module Prog = struct
  module Not_found = struct
    type t =
      { context : string
      ; program : string
      ; hint    : string option
      ; loc     : Loc.t option
      }

    let create ?hint ~context ~program ~loc () =
      { hint
      ; context
      ; program
      ; loc
      }

    let raise { context ; program ; hint ; loc } =
      let hint =
        match program with
        | "refmt" ->
          Some (Option.value ~default:"try: opam install reason" hint)
        | _ ->
          hint
      in
      Utils.program_not_found ?hint ~loc ~context program
  end

  type t = (Path.t, Not_found.t) result

  let decode : t Dune_lang.Decoder.t =
    Dune_lang.Decoder.map Dpath.decode ~f:Result.ok

  let encode = function
    | Ok s -> Dpath.encode s
    | Error (e : Not_found.t) -> Dune_lang.Encoder.string e.program
end

module type Ast = Action_intf.Ast
  with type program = Prog.t
  with type path    = Path.t
  with type target  = Path.Build.t
  with type string  = String.t
module rec Ast : Ast = Ast

module String_with_sexp = struct
  type t = string
  let decode = Dune_lang.Decoder.string
  let encode = Dune_lang.Encoder.string
  let is_dev_null s =
    Path.equal (Path.of_string s) Config.dev_null
end

include Action_ast.Make(Prog)(Dpath)(Dpath.Build)(String_with_sexp)(Ast)
type program = Prog.t
type path = Path.t
type target = Path.Build.t
type string = String.t

module For_shell = struct
  module type Ast = Action_intf.Ast
    with type program = string
    with type path    = string
    with type target  = string
    with type string  = string
  module rec Ast : Ast = Ast

  include Action_ast.Make
      (String_with_sexp)
      (String_with_sexp)
      (String_with_sexp)
      (String_with_sexp)
      (Ast)
end

module Relativise = Action_mapper.Make(Ast)(For_shell.Ast)

let for_shell t =
  let rec loop t ~dir ~f_program ~f_string ~f_path ~f_target =
    match t with
    | Symlink (src, dst) ->
      let src =
        match Path.Build.parent dst with
        | None -> Path.to_string src
        | Some from -> Path.reach ~from:(Path.build from) src
      in
      let dst = Path.reach ~from:dir (Path.build dst) in
      For_shell.Symlink (src, dst)
    | t ->
      Relativise.map_one_step loop t ~dir ~f_program ~f_string ~f_path ~f_target
  in
  loop t
    ~dir:Path.root
    ~f_string:(fun ~dir:_ x -> x)
    ~f_path:(fun ~dir x -> Path.reach x ~from:dir)
    ~f_target:(fun ~dir x -> Path.reach (Path.build x) ~from:dir)
    ~f_program:(fun ~dir x ->
      match x with
      | Ok p -> Path.reach p ~from:dir
      | Error e -> e.program)

module Unresolved = struct
  module Program = struct
    type t =
      | This   of Path.t
      | Search of Loc.t option * string

    let of_string ~dir ~loc s =
      if String.contains s '/' then
        This (Path.relative dir s)
      else
        Search (loc, s)
  end

  module type Uast = Action_intf.Ast
    with type program = Program.t
    with type path    = Path.t
    with type target  = Path.Build.t
    with type string  = String.t
  module rec Uast : Uast = Uast
  include Uast

  include Action_mapper.Make(Uast)(Ast)

  let resolve t ~f =
    map t
      ~dir:Path.root
      ~f_path:(fun ~dir:_ x -> x)
      ~f_target:(fun ~dir:_ x -> x)
      ~f_string:(fun ~dir:_ x -> x)
      ~f_program:(fun ~dir:_ -> function
        | This p -> Ok p
        | Search (loc, s) -> Ok (f loc s))
end

let fold_one_step t ~init:acc ~f =
  match t with
  | Chdir (_, t)
  | Setenv (_, _, t)
  | Redirect (_, _, t)
  | Ignore (_, t) -> f acc t
  | Progn l -> List.fold_left l ~init:acc ~f
  | Run _
  | Echo _
  | Cat _
  | Copy _
  | Symlink _
  | Copy_and_add_line_directive _
  | System _
  | Bash _
  | Write_file _
  | Rename _
  | Remove_tree _
  | Mkdir _
  | Digest_files _
  | Diff _
  | Merge_files_into _ -> acc

include Action_mapper.Make(Ast)(Ast)

let chdirs =
  let rec loop acc t =
    let acc =
      match t with
      | Chdir (dir, _) -> Path.Set.add acc dir
      | _ -> acc
    in
    fold_one_step t ~init:acc ~f:loop
  in
  fun t -> loop Path.Set.empty t

let symlink_managed_paths sandboxed deps ~eval_pred =
  let steps =
    Path.Set.fold (Dep.Set.paths deps ~eval_pred) ~init:[]
      ~f:(fun path acc ->
        match Path.as_in_build_dir path with
        | None ->
          assert (not (Path.is_in_source_tree path));
          acc
        | Some p -> Symlink (path, sandboxed p) :: acc)
  in
  Progn steps

let maybe_sandbox_path f p =
  match Path.as_in_build_dir p with
  | None -> p
  | Some p -> Path.build (f p)

let sandbox t ~sandboxed ~deps ~targets ~eval_pred : t =
  Progn
    [ symlink_managed_paths sandboxed deps ~eval_pred
    ; map t
        ~dir:Path.root
        ~f_string:(fun ~dir:_ x -> x)
        ~f_path:(fun ~dir:_ p -> maybe_sandbox_path sandboxed p)
        ~f_target:(fun ~dir:_ -> sandboxed)
        ~f_program:(fun ~dir:_ ->
          Result.map ~f:(maybe_sandbox_path sandboxed))
    ; Progn (List.filter_map targets ~f:(fun path ->
        Some (Rename (sandboxed path, path))))
    ]
