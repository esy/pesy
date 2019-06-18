{
open StdLabels

type kind = Ignore | Expect | Error
}

let newline = eof | '\r'? '\n'

rule code txt start = parse
  | "[%%ignore]\n" {
    let pos = start.Lexing.pos_cnum in
    let len = Lexing.lexeme_start lexbuf - pos in
    let s = String.sub txt ~pos ~len in
    Lexing.new_line lexbuf;
    (Ignore, start, s) :: code txt lexbuf.lex_curr_p lexbuf
  }
  | "[%%expect{|\n" {
    let pos = start.Lexing.pos_cnum in
    let len = Lexing.lexeme_start lexbuf - pos in
    let s = String.sub txt ~pos ~len in
    Lexing.new_line lexbuf;
    (Expect, start, s) :: expectation txt lexbuf
  }
  | "[%%error{|\n" {
    let pos = start.Lexing.pos_cnum in
    let len = Lexing.lexeme_start lexbuf - pos in
    let s = String.sub txt ~pos ~len in
    Lexing.new_line lexbuf;
    (Error, start, s) :: expectation txt lexbuf
  }
  | [^'\n']*'\n' {
    Lexing.new_line lexbuf;
    code txt start lexbuf
  }
  | eof {
    let pos = start.Lexing.pos_cnum in
    let len = String.length txt - pos in
    if pos > 0 then begin
      let s = String.sub txt ~pos ~len in
      if String.trim s = "" then
        []
      else
        [(Expect, start, s)]
    end else
      []
  }

and expectation txt = parse
  | "|}]" newline {
      Lexing.new_line lexbuf;
      code txt lexbuf.lex_curr_p lexbuf
    }
  | [^'\n']*'\n' {
    Lexing.new_line lexbuf;
    expectation txt lexbuf
  }

{
module Outcometree_cleaner = struct
  open Outcometree

  let lid s =
    match String.rindex s '.' with
    | exception Not_found -> s
    | i ->
      let pos = i + 1 in
      let len = String.length s in
      String.sub s ~pos ~len:(len - pos)

  let ident = function
    | Oide_dot (_, s) -> Oide_ident (lid s)
    | Oide_ident s -> Oide_ident (lid s)
    | id -> id

  let rec value = function
    | Oval_array l -> Oval_array (values l)
    | Oval_constr (id, l) -> Oval_constr (ident id, values l)
    | Oval_list l -> Oval_list (values l)
    | Oval_record l ->
      Oval_record (List.map l ~f:(fun (id, v) -> ident id, value v))
    | Oval_tuple l -> Oval_tuple (values l)
    | Oval_variant (s, Some v) -> Oval_variant (s, Some (value v))
    | v -> v

  and values l = List.map l ~f:value

  let () =
    let print_out_value = !Toploop.print_out_value in
    Toploop.print_out_value := (fun ppf v -> print_out_value ppf (value v))
end

let main () =
  Clflags.real_paths := false;
  Test_common.run_expect_test Sys.argv.(1) ~f:(fun file_contents lexbuf ->
    let chunks = code file_contents lexbuf.lex_curr_p lexbuf in

    Toploop.initialize_toplevel_env ();
    List.iter
      [ "src/dune_lang/.dune_lang.objs/byte"
      ; "src/stdune/.stdune.objs/byte"
      ; "src/fiber/.fiber.objs/byte"
      ; "src/dag/.dag.objs/byte"
      ; "src/memo/.memo.objs/byte"
      ; "src/.dune.objs/byte"
      ]
      ~f:Topdirs.dir_directory;

    let buf = Buffer.create (String.length file_contents + 1024) in
    let ppf = Format.formatter_of_buffer buf in
    List.iter chunks ~f:(fun (kind, pos, s) ->
      begin match kind with
      | Ignore -> Format.fprintf ppf "%s[%%%%ignore]@." s
      | Expect -> Format.fprintf ppf "%s[%%%%expect{|@." s
      | Error -> Format.fprintf ppf "%s[%%%%error{|@." s
      end;
      let lexbuf = Lexing.from_string s in
      lexbuf.lex_curr_p <- pos;
      let phrases = !Toploop.parse_use_file lexbuf in
      List.iter phrases ~f:(fun phr ->
        try
          let print_types_and_values =
            match kind with
            | Expect | Error -> true
            | Ignore -> false
          in
          Stdune.In_expect_test.formatter := Some ppf;
          ignore (Toploop.execute_phrase print_types_and_values ppf phr : bool)
        with exn ->
          let ppf =
            match kind with
            | Error -> ppf
            | Ignore | Expect -> Format.err_formatter
          in
          Location.report_exception ppf exn
      );
      begin match kind with
      | Ignore -> ()
      | Expect | Error -> Format.fprintf ppf "@?|}]@."
      end);
    Buffer.contents buf)

let () =
  try
    main ()
  with exn ->
    Location.report_exception Format.err_formatter exn;
    exit 1
}
