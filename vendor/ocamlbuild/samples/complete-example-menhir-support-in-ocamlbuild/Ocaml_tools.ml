let menhir_ocamldep_command' tags ~menhir_spec out =
  let menhir = if !Options.ocamlyacc = N then V"MENHIR" else !Options.ocamlyacc in
  Cmd(S[menhir; T tags; A"--raw-depend";
        A"--ocamldep"; Quote (ocamldep_command' Tags.empty);
        menhir_spec ; Sh ">"; Px out])

let menhir_ocamldep_command arg out env _build =
  let arg = env arg and out = env out in
  let tags = tags_of_pathname arg++"ocaml"++"menhir_ocamldep" in
  menhir_ocamldep_command' tags ~menhir_spec:(P arg) out

let import_mlypack build mlypack =
  let tags1 = tags_of_pathname mlypack in
  let files = string_list_of_file mlypack in
  let include_dirs = Pathname.include_dirs_of (Pathname.dirname mlypack) in
  let files_alternatives =
    List.map begin fun module_name ->
      expand_module include_dirs module_name ["mly"]
    end files
  in
  let files = List.map Outcome.good (build files_alternatives) in
  let tags2 =
    List.fold_right
      (fun file -> Tags.union (tags_of_pathname file))
      files tags1
  in
  (tags2, files)

let menhir_modular_ocamldep_command mlypack out env build =
  let mlypack = env mlypack and out = env out in
  let (tags,files) = import_mlypack build mlypack in
  let tags = tags++"ocaml"++"menhir_ocamldep" in
  let menhir_base = Pathname.remove_extensions mlypack in
  let menhir_spec = S[A "--base" ; P menhir_base ; atomize_paths files] in
  menhir_ocamldep_command' tags ~menhir_spec out

let menhir_modular menhir_base mlypack mlypack_depends env build =
  let menhir = if !Options.ocamlyacc = N then "menhir" else !Options.ocamlyacc in
  let menhir_base = env menhir_base in
  let mlypack = env mlypack in
  let mlypack_depends = env mlypack_depends in
  let (tags,files) = import_mlypack build mlypack in
  let () = List.iter Outcome.ignore_good (build [[mlypack_depends]]) in
  Ocaml_compiler.prepare_compile build mlypack;
  let ocamlc_tags = tags++"ocaml"++"byte"++"compile" in
  let tags = tags++"ocaml"++"parser"++"menhir" in
  Cmd(S[menhir ;
        A "--ocamlc"; Quote(S[!Options.ocamlc; T ocamlc_tags; ocaml_include_flags mlypack]);
        T tags ; A "--base" ; Px menhir_base ; atomize_paths files])