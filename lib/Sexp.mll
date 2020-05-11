{
(* Originally from https://github.com/ocamllabs/vscode-ocaml-platform/blob/0a62fd9a0b1a1898e288df4c3ee7293d62e11f78/src/sexp.mll#L1 *)
exception Error of string

let err m = raise (Error m)

type token =
  | Lparen
  | Rparen
  | Atom of string
  | Eof

let b = Buffer.create 256

let new_atom () = Buffer.clear b

let get_atom () = Atom (Buffer.contents b)

let add_string s = Buffer.add_string b s

let add_char c = Buffer.add_char b c

}

let atom_char = [^ ';' '(' ')' '"' '\000'-'\032' '\127'-'\255']
let newline   = '\r'? '\n'
let blank     = [' ' '\t' '\012']

rule token = parse
  | '(' { Lparen }
  | ')' { Rparen }
  | '"' { new_atom (); quoted_atom lexbuf }
  | atom_char+ as atom { new_atom (); add_string atom; get_atom () }
  | (blank | newline) { token lexbuf }
  | eof { Eof }

and quoted_atom = parse
  | '"' { get_atom () }
  | '\\' { escape_sequence lexbuf }
  | _ as c { add_char c; quoted_atom lexbuf }
  | eof { err "unterminated string" }

and escape_sequence = parse
  | '"' as c { add_char c; quoted_atom lexbuf }
  | '\\' as c { add_char c; quoted_atom lexbuf }
  | _ { err "unknown escape code" }
  | eof { err "unterminated escape sequence" }

{
  type t =
    | Atom of string
    | List of t list

  let parse_string (s : string) =
    let lexbuf = Lexing.from_string s in
    let rec list depth (acc : t list) =
      match (token lexbuf : token) with
      | Eof -> List (List.rev acc)
      | Rparen ->
        if depth = 0 then err "unbalanced parens";
        List (List.rev acc)
      | Lparen ->
        let sublist = list (depth + 1) [] in
        list depth (sublist :: acc)
      | Atom a ->
        list depth (Atom a :: acc)
    in
    list 0 []

  let rec print_string ppf x = Format.pp_print_string ppf x
  and print_list ppf = function
    | [] -> ()
    | a::[] -> Format.fprintf ppf "%a" print_sexp a
    | a::b -> Format.fprintf ppf "@[%a @; %a @]" print_sexp a print_list b;
  and print_sexp ppf = function
    | Atom a -> Format.fprintf ppf "%a" print_string a
    | List l -> Format.fprintf ppf "(%a)" print_list l
  let to_string_hum x = print_sexp Format.str_formatter x; Format.flush_str_formatter ()
}
