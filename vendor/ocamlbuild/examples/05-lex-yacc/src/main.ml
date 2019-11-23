
let main () =
  let argv   = Array.to_list Sys.argv in
  let args   = List.tl argv in
  let expr   = String.concat " " args in
  let lexbuf = Lexing.from_string expr in
  let result = Parser.main Lexer.token lexbuf in
    Printf.printf "%d\n" result

let () = main ()

