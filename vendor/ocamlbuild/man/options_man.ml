open Ocamlbuild_pack

let options = Options.spec ()

let print (option, _, descr) =
  let escape str = My_std.String.subst "-" "\\-" str in
  let argument, descr =
    try
      (* take an aligned description, for example "<param> foo bar",
         and splits it into an optional argument "<param>" (ends on
         a space, empty if the first character is a space) and
         a description "foo bar" (skipping the spaces before it) *)
      let arg_index = String.index descr ' ' in
      let rec rest index =
        if index >= String.length descr then ""
        else if descr.[index] = ' ' then rest (index + 1)
        else My_std.String.after descr index
      in
      (Some (My_std.String.before descr arg_index),
       rest arg_index)
    with Not_found -> None, descr
  in
  print_endline ".TP";
  print_string "\\fB"; print_string (escape option); print_string "\\fR";
  begin match argument with
    | None | Some "" -> ()
    | Some argument -> print_char ' '; print_string (escape argument)
  end;
  print_newline ();
  print_endline (escape descr);
  ()

let () =
  print_endline ".SH OPTIONS";
  print_newline ();
  print_endline "The following command-line options are recognized by";
  print_endline ".BR ocamlbuild (1).";
  print_newline ();
  List.iter print options;
  print_newline ();
  print_endline ".PP";
  ()
