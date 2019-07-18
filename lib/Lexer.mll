(* File lexer.mll *)
{
  open Parser        (* The type token is defined in parser.mli *)
}
rule read = parse
[' ' '\t']   { read lexbuf }     (* skip blanks *)
| ['(']      { LPAREN }
| [')']      { RPAREN }
| ['=']      { ASSN }
| "require"  { REQUIRE }
| ['\'']      { SQUOTE }
| ['A' - 'Z' ] + [ 'a' - 'z'] * as lxm { MODULE_NAME(lxm)  }
| ['a'-'z' '/' '-' '0'-'9'] + as lxm { MODULE_PATH(lxm)  }
| ['\n']     {EOL}
| eof      {EOF}
