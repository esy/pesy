open! Stdune
open! Import

type dune_file =
  | OCaml_syntax of Loc.t
  | Sexps of Dune_lang.Cst.t list

let parse_lexbuf lb =
  if Dune_lexer.is_script lb then
    OCaml_syntax (Loc.of_lexbuf lb)
  else
    Sexps (Dune_lang.Parser.parse_cst lb)

let parse_file path_opt =
  match path_opt with
  | Some path ->
    Io.with_lexbuf_from_file path ~f:parse_lexbuf
  | None ->
    parse_lexbuf @@ Lexing.from_channel stdin

let can_be_displayed_wrapped =
  List.for_all ~f:(fun (c : Dune_lang.Cst.t) ->
    match c with
    | Atom _
    | Quoted_string _
    | Template _
    | List (_, [])
    | List (_, [_])
      ->
      true
    | List _
    | Comment _
      ->
      false
  )

let pp_simple fmt t =
  Dune_lang.Cst.abstract t
  |> Option.value_exn
  |> Dune_lang.Ast.remove_locs
  |> Dune_lang.pp Dune fmt

let print_wrapped_list fmt =
  Format.fprintf fmt "(@[<hov 1>%a@])"
    (Fmt.list
      ~pp_sep:(fun fmt () -> Format.fprintf fmt "@ ")
      pp_simple
    )

let pp_comment_line fmt l =
  Format.fprintf fmt ";%s" l

let pp_comment loc fmt (comment:Dune_lang.Cst.Comment.t) =
  match comment with
  | Lines ls ->
    Format.fprintf fmt "@[<v 0>%a@]"
      (Fmt.list
         ~pp_sep:(fun fmt () -> Format.fprintf fmt "@;")
         pp_comment_line)
      ls
  | Legacy ->
    Errors.fail loc "Formatting is only supported with the dune syntax"

let pp_break fmt attached =
  if attached then
    Format.fprintf fmt " "
  else
    Format.fprintf fmt "@,"

let pp_list_with_comments pp_sexp fmt sexps =
  let rec go fmt (l:Dune_lang.Cst.t list) =
    match l with
    | x :: Comment (loc, c) :: xs ->
      let attached = Loc.on_same_line (Dune_lang.Cst.loc x) loc in
      Format.fprintf
        fmt
        "%a%a%a@,%a"
        pp_sexp x
        pp_break attached
        (pp_comment loc) c
        go xs
    | Comment (loc, c)::xs ->
      Format.fprintf fmt "%a@,%a" (pp_comment loc) c go xs
    | [x] ->
      Format.fprintf fmt "%a" pp_sexp x;
    | x :: xs ->
      Format.fprintf fmt "%a@,%a" pp_sexp x go xs
    | [] -> ()
  in
  go fmt sexps

let rec pp_sexp fmt : Dune_lang.Cst.t -> _ =
  function
  | ( Atom _
    | Quoted_string _
    | Template _
    ) as sexp
    ->
    pp_simple fmt sexp
  | List (_, sexps)
    ->
    Format.fprintf fmt "@[<v 1>%a@]"
      (if can_be_displayed_wrapped sexps then
         print_wrapped_list
       else
         pp_sexp_list)
      sexps
  | Comment (loc, c)
    ->
    pp_comment loc fmt c

and pp_sexp_list fmt =
  Format.fprintf fmt "(%a)"
    (pp_list_with_comments pp_sexp)

let pp_top_sexp fmt sexp =
  Format.fprintf fmt "%a\n" pp_sexp sexp

let pp_top_sexps =
  Fmt.list ~pp_sep:Fmt.nl pp_top_sexp

let write_file ~path sexps =
  let f oc =
    let fmt = (Format.formatter_of_out_channel oc) in
    Format.fprintf fmt "%a%!" pp_top_sexps sexps
  in
  Io.with_file_out ~binary:true path ~f

let format_file ~input =
  match parse_file input with
  | exception Dune_lang.Parse_error e ->
    Printf.eprintf
      "Parse error: %s\n"
      (Dune_lang.Parse_error.message e);
    exit 1
  | OCaml_syntax loc ->
    begin
      match input with
      | Some path ->
        Io.with_file_in path ~f:(fun ic ->
          Io.copy_channels ic stdout
        )
      | None ->
        Errors.fail loc "OCaml syntax is not supported."
    end
  | Sexps sexps ->
    Format.printf "%a%!" pp_top_sexps sexps
