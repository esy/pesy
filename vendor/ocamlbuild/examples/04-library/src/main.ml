
module J = Yojson.Basic

let json io =
  J.from_channel io |> J.to_channel stdout

let main () =
  let argv = Array.to_list Sys.argv in
  let args = List.tl argv in
  let this = List.hd argv in
  match args with
  | []          -> json stdin; print_newline ()
  | _           -> Printf.eprintf "usage: %s" this; exit 1

let () = main ()

