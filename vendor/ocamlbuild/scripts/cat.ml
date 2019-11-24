

let () =
  let cin = open_in Sys.argv.(1) in
  begin try
      while true do
        print_endline (input_line cin)
      done;
    with
    | End_of_file -> ()
    | _ -> ()
  end;
  close_in cin
