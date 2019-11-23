
let main () =
  let argv = Array.to_list Sys.argv in
  let args = List.tl argv in
  match args with
  | []        -> Printf.printf "%s, world!\n" Hello.hello
  | names     -> Printf.printf "%s, %s!\n" Hello.hello (Util.join names)

let () = main ()

