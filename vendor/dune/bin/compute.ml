open Stdune
open Import

let doc = "Compute internal function."

let man =
  [ `S "DESCRIPTION"
  ; `P {|Run a registered memoize function with the given input and
           print the output. |}
  ; `P {|This should only be used for debugging dune.|}
  ; `Blocks Common.help_secs
  ]

let info = Term.info "compute" ~doc ~man

let term =
  Term.ret @@
  let+ common = Common.term
  and+ fn =
    Arg.(required
         & pos 0 (some string) None
         & info [] ~docv:"FUNCTION"
             ~doc:"Compute $(docv) for a given input.")
  and+ inp =
    Arg.(value
         & pos 1 (some string) None
         & info [] ~docv:"INPUT"
             ~doc:"Use $(docv) as the input to the function.")
  in
  Common.set_common common ~targets:[];
  let log = Log.create common in
  let action =
    Scheduler.go ~log ~common (fun () ->
      let open Fiber.O in
      let* _setup =
        Import.Main.setup ~log common ~external_lib_deps_mode:true
      in
      match fn, inp with
      | "list", None ->
        Fiber.return `List
      | "list", Some _ ->
        Fiber.return (`Error "'list' doesn't take an argument")
      | "help", Some fn ->
        Fiber.return (`Show_doc fn)
      | fn, Some inp ->
        let sexp =
          Dune_lang.parse_string
            ~fname:"<command-line>"
            ~mode:Dune_lang.Parser.Mode.Single inp
        in
        let+ res = Memo.call fn sexp in
        `Result res
      | fn, None ->
        Fiber.return (`Error (sprintf "argument missing for '%s'" fn))
    )
  in
  match action with
  | `Error msg ->
    `Error (true, msg)
  | `Result res ->
    Format.printf "%a\n%!" Sexp.pp res;
    `Ok ()
  | `List ->
    let fns = Memo.registered_functions () in
    let longest = String.longest_map fns ~f:(fun info -> info.name) in
    List.iter fns ~f:(fun { Memo.Function_info.name; doc } ->
      Printf.printf "%-*s : %s\n" longest name doc);
    flush stdout;
    `Ok ()
  | `Show_doc fn ->
    let info = Memo.function_info fn in
    Printf.printf "%s\n\
                   %s\n\
                   %s\n"
      info.name
      (String.make (String.length info.name) '=')
      info.doc;
    `Ok ()

let command = term, info
