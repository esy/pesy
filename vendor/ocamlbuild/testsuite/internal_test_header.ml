#load "unix.cma";;

#mod_use "../src/ocamlbuild_config.ml";;

#use "ocamlbuild_test.ml";;

module M = Match;;
module T = Tree;;

let _build = M.d "_build";;

let ocamlopt_available =
  if Ocamlbuild_config.ocaml_native then
    Fullfilled
  else
    Missing ("ocamlopt")
