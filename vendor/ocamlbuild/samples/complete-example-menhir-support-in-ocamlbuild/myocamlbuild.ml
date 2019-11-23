rule "ocaml: modular menhir (mlypack)"
  ~prods:["%.mli" ; "%.ml"]
  ~deps:["%.mlypack"]
  ~doc:"Menhir supports building a parser by composing several .mly files \
        together, containing different parts of the grammar description. \
        To use that feature with ocamlbuild, you should create a .mlypack \
        file with the same syntax as .mllib or .mlpack files: \
        a whitespace-separated list of the capitalized module names \
        of the .mly files you want to combine together."
  (Ocaml_tools.menhir_modular "%" "%.mlypack" "%.mlypack.depends");

rule "ocaml: menhir modular dependencies"
  ~prod:"%.mlypack.depends"
  ~dep:"%.mlypack"
  (Ocaml_tools.menhir_modular_ocamldep_command "%.mlypack" "%.mlypack.depends");

rule "ocaml: menhir"
  ~prods:["%.ml"; "%.mli"]
  ~deps:["%.mly"; "%.mly.depends"]
  ~doc:"Invokes menhir to build the .ml and .mli files derived from a .mly \
        grammar. If you want to use ocamlyacc instead, you must disable the \
        -use-menhir option that was passed to ocamlbuild."
  (Ocaml_tools.menhir "%.mly");

rule "ocaml: menhir dependencies"
  ~prod:"%.mly.depends"
  ~dep:"%.mly"
  (Ocaml_tools.menhir_ocamldep_command "%.mly" "%.mly.depends");

flag ["ocaml"; "menhir"] (atomize !Options.ocaml_yaccflags);

flag [ "ocaml" ; "menhir" ; "explain" ] (S[A "--explain"]);
flag [ "ocaml" ; "menhir" ; "infer" ] (S[A "--infer"]);

List.iter begin fun mode ->
  flag [ mode; "only_tokens" ] (S[A "--only-tokens"]);
  pflag [ mode ] "external_tokens" (fun name ->
    S[A "--external-tokens"; A name]);
          end [ "menhir"; "menhir_ocamldep" ];