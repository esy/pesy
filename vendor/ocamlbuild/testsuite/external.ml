#use "internal_test_header.ml";;
#use "findlibonly_test_header.ml";;
#use "external_test_header.ml";;

let () = test "SyntaxFlag"
  ~options:[`use_ocamlfind; `package "camlp4.macro"; `syntax "camlp4o"]
  ~requirements:(package_exists "camlp4.macro")
  ~description:"-syntax for ocamlbuild"
  ~tree:[T.f "dummy.ml" ~content:"IFDEF TEST THEN\nprint_endline \"Hello\";;\nENDIF;;"]
  ~matching:[M.f "dummy.native"]
  ~targets:("dummy.native",[]) ();;

(* This test fails with the recent versions of camlp4, see
   https://caml.inria.fr/mantis/view.php?id=5652#c8776.

let () = test "Camlp4NativePlugin"
  ~description:"Fixes PR#5652"
  ~requirements:(package_exists "camlp4.macro")
  ~options:[`use_ocamlfind; `package "camlp4.macro";
            `tags ["camlp4o.opt"; "syntax(camp4o)"];
            `ppflag "camlp4o.opt"; `ppflag "-parser"; `ppflag "macro";
            `ppflag "-DTEST"]
  ~tree:[T.f "dummy.ml"
            ~content:"IFDEF TEST THEN\nprint_endline \"Hello\";;\nENDIF;;"]
  ~matching:[M.x "dummy.native" ~output:"Hello"]
  ~targets:("dummy.native",[]) ();;
*)

let () = test "SubtoolOptions"
  ~description:"Options that come from tags that needs to be spliced \
                to the subtool invocation (PR#5763)"
  (* testing for the 'menhir' executable directly
     is too hard to do in a portable way; test the ocamlfind package instead *)
  ~requirements:(package_exists "menhirLib")
  ~options:[`use_ocamlfind; `use_menhir; `tags ["package(camlp4.fulllib)"]]
  ~tree:[T.f "parser.mly"
            ~content:"%{ %}
                      %token DUMMY
                      %start<Camlp4.PreCast.Syntax.Ast.expr option> test
                      %%
                      test: DUMMY {None}"]
  ~matching:[M.f "parser.native"; M.f "parser.byte"]
  ~targets:("parser.native",["parser.byte"])
  ();;

let () = test "ppopt"
  ~description:"Test the -ppopt option"
  ~requirements:(package_exists "camlp4")
  ~options:[`use_ocamlfind; `package "camlp4"; `syntax "camlp4o";
            `tags ["ppopt(-no_quot)"];
           ]
  ~tree:[T.f "test.ml"
           (* <<y>> looks like a camlp4 quotation and
              will fail to compile unless '-no_quot' is passed *)
           ~content:"let test (<<) (>>) x y z = x <<y>> z"]
  ~targets:("test.cmo",[])
  ();;

run ~root:"_test_external";;
