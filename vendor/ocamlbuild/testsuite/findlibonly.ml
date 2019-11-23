#use "internal_test_header.ml";;
#use "findlibonly_test_header.ml";;

let () = test "ThreadAndArchive"
  ~description:"Fixes PR#6058"
  ~options:[`use_ocamlfind; `package "threads"; `tag "thread"]
  ~tree:[T.f "t.ml" ~content:""]
  ~matching:[M.f "_build/t.cma"]
  ~targets:("t.cma",[]) ();;

let () = test "PredicateFlag"
  ~description:"ocamlfind ocamldep does not support the -predicate option"
  ~options:[`use_ocamlfind; `tag "predicate(byte)"]
  ~tree:[T.f "test.ml" ~content:"let x = List.map"]
  ~matching:[_build [M.f "test.ml.depends"]]
  ~targets:("test.ml.depends", []) ();;

let () = test "ToolsFlagsConflict"
  ~description:"PR#6300: conflicts between -ocamlc and -use-ocamlfind options"
  ~options:[`use_ocamlfind; `ocamlc "\"ocamlc -annot\""]
  ~tree:[T.f "test.ml" ~content:"let x = 1"]
  ~matching:[_build [M.f "test.annot"; M.f "test.byte"]]
  ~targets:("test.byte", []) ();;

run ~root:"_test_findlibonly";;
