open Stdune
open Import

let doc = "Dump internal rules."

let man =
  [ `S "DESCRIPTION"
  ; `P {|Dump Dune internal rules for the given targets.
           If no targets are given, dump all the internal rules.|}
  ; `P {|By default the output is a list of S-expressions,
           one S-expression per rule. Each S-expression is of the form:|}
  ; `Pre "  ((deps    (<dependencies>))\n\
         \   (targets (<targets>))\n\
         \   (context <context-name>)\n\
         \   (action  <action>))"
  ; `P {|$(b,<context-name>) is the context is which the action is executed.
           It is omitted if the action is independent from the context.|}
  ; `P {|$(b,<action>) is the action following the same syntax as user actions,
           as described in the manual.|}
  ; `Blocks Common.help_secs
  ]

let info = Term.info "rules" ~doc ~man

let print_rule_makefile ppf (rule : Build_system.Rule.t) =
  let action =
    Action.For_shell.Progn
      [ Mkdir (Path.to_string rule.dir)
      ; Action.for_shell rule.action
      ]
  in
  let eval_pred = Build_system.eval_pred in
  Format.fprintf ppf
    "@[<hov 2>@{<makefile-stuff>%a:%t@}@]@,\
     @<0>\t@{<makefile-action>%a@}@,@,"
    (Format.pp_print_list ~pp_sep:Format.pp_print_space (fun ppf p ->
       Format.pp_print_string ppf (Path.to_string p)))
    (Path.Set.to_list rule.targets)
    (fun ppf ->
       Path.Set.iter (Dep.Set.paths rule.deps ~eval_pred) ~f:(fun dep ->
         Format.fprintf ppf "@ %s" (Path.to_string dep)))
    Pp.pp
    (Action_to_sh.pp action)

let print_rule_sexp ppf (rule : Build_system.Rule.t) =
  let sexp_of_action action =
    Action.for_shell action |> Action.For_shell.encode
  in
  let paths ps =
    Dune_lang.Encoder.list Path_dune_lang.encode (Path.Set.to_list ps)
  in
  let sexp =
    Dune_lang.Encoder.record (
      List.concat
        [ [ "deps"   , Dep.Set.encode rule.deps
          ; "targets", paths rule.targets ]
        ; (match rule.context with
           | None -> []
           | Some c -> ["context",
                        Dune_lang.atom_or_quoted_string c.name])
        ; [ "action" , sexp_of_action rule.action ]
        ])
  in
  Format.fprintf ppf "%a@," Dune_lang.pp_split_strings sexp

module Syntax = struct
  type t =
    | Makefile
    | Sexp

  let term =
    let doc = "Output the rules in Makefile syntax." in
    let+ makefile = Arg.(value & flag & info ["m"; "makefile"] ~doc) in
    if makefile then
      Makefile
    else
      Sexp

  let print_rule = function
    | Makefile -> print_rule_makefile
    | Sexp -> print_rule_sexp

  let print_rules syntax ppf rules =
    Dune_lang.prepare_formatter ppf;
    Format.pp_open_vbox ppf 0;
    Format.pp_print_list (print_rule syntax) ppf rules;
    Format.pp_print_flush ppf ()
end


let term =
  let+ common = Common.term
  and+ out =
    Arg.(value
         & opt (some string) None
         & info ["o"] ~docv:"FILE"
             ~doc:"Output to a file instead of stdout.")
  and+ recursive =
    Arg.(value
         & flag
         & info ["r"; "recursive"]
             ~doc:"Print all rules needed to build the transitive \
                   dependencies of the given targets.")
  and+ syntax = Syntax.term
  and+ targets =
    Arg.(value
         & pos_all string []
         & Arg.info [] ~docv:"TARGET")
  in
  let out = Option.map ~f:Path.of_string out in
  Common.set_common common ~targets;
  let log = Log.create common in
  Scheduler.go ~log ~common (fun () ->
    let open Fiber.O in
    let* setup =
      Import.Main.setup ~log common ~external_lib_deps_mode:true in
    let request =
      match targets with
      | [] ->
        Build.paths (Build_system.all_targets ())
      | _  ->
        Target.resolve_targets_exn ~log common setup targets
        |> Target.request setup
    in
    let* rules = Build_system.evaluate_rules ~request ~recursive in
    let print oc =
      let ppf = Format.formatter_of_out_channel oc in
      Syntax.print_rules syntax ppf rules;
      Fiber.return ()
    in
    match out with
    | None -> print stdout
    | Some fn -> Io.with_file_out fn ~f:print)

let command = term, info
