let main () =
  let argv = Array.to_list Sys.argv in
  let args = List.tl argv in
  let this = List.hd argv in
  match args with
  | [] -> Printf.eprintf "usage: %s [WORD]..." this; exit 1
  | _  -> Printf.eprintf "%s\n" (Util.join args)

let () = main ()
