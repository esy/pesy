let parse = str => {
  let lexbuf = Lexing.from_string(str);
  Parser.main(Lexer.read, lexbuf);
};
