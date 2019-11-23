{
    module P = Parser

    exception Error of string
    let error fmt = Printf.kprintf (fun msg -> raise (Error msg)) fmt

    let get = Lexing.lexeme
}

rule token = parse
  | [' ' '\t' '\n']         { token lexbuf }     (* skip blanks *)
  | ['0'-'9']+ as lxm       { P.INT(int_of_string lxm) }
  | '+'                     { P.PLUS }
  | '-'                     { P.MINUS }
  | '*'                     { P.TIMES }
  | '/'                     { P.DIV }
  | '('                     { P.LPAREN }
  | ')'                     { P.RPAREN }
  | eof                     { P.EOF }
  | _  { error "don't know how to handle '%s'" (get lexbuf) }


