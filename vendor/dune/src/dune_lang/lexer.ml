module Token = Lexer_shared.Token
module Error = Lexer_shared.Error

type t = Lexer_shared.t

let token = Dune_lexer.token
let jbuild_token = Jbuild_lexer.token

let of_syntax = function
  | Syntax.Dune -> token
  | Jbuild -> jbuild_token

exception Error = Lexer_shared.Error
