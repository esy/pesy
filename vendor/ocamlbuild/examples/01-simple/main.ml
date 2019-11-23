
let main () =
  let argv = Array.to_list Sys.argv in
  let args = List.tl argv in
  match args with
  | []        -> Printf.printf "Hello, world!\n"
  | names     -> Printf.printf "Hello, %s!\n" (Util.join names)

let () = main ()

