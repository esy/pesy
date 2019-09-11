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
| ['A' - 'Z' ] [ 'A' - 'Z' 'a' - 'z' '_'] * as lxm { MODULE_NAME(lxm)  }
| ['@'] ? ['a'-'z' '.' '/' '-' '0'-'9'] + as lxm { MODULE_PATH(lxm)  }
| ['\n']     {EOL}
| eof      {EOF}
