  $ dune build --root general src/test.exe --display short --debug-dependency-path
  Entering directory 'general'
      ocamllex src/lexer1.ml
      ocamldep src/.test.eobjs/lexer1.ml.d
      ocamllex src/lexer2.ml
      ocamldep src/.test.eobjs/lexer2.ml.d
      ocamldep src/.test.eobjs/test.ml.d
        menhir src/test_base.{ml,mli}
      ocamldep src/.test.eobjs/test_base.mli.d
        menhir src/test_menhir1.{ml,mli}
      ocamldep src/.test.eobjs/test_menhir1.mli.d
      ocamldep src/.test.eobjs/test_base.ml.d
      ocamldep src/.test.eobjs/test_menhir1.ml.d
        ocamlc src/.test.eobjs/byte/test_menhir1.{cmi,cmti}
      ocamlopt src/.test.eobjs/native/test_menhir1.{cmx,o}
        ocamlc src/.test.eobjs/byte/test_base.{cmi,cmti}
      ocamlopt src/.test.eobjs/native/test_base.{cmx,o}
        ocamlc src/.test.eobjs/byte/lexer1.{cmi,cmo,cmt}
      ocamlopt src/.test.eobjs/native/lexer1.{cmx,o}
        ocamlc src/.test.eobjs/byte/lexer2.{cmi,cmo,cmt}
        ocamlc src/.test.eobjs/byte/test.{cmi,cmo,cmt}
      ocamlopt src/.test.eobjs/native/test.{cmx,o}
      ocamlopt src/.test.eobjs/native/lexer2.{cmx,o}
      ocamlopt src/test.exe

  $ dune build --root cmly test.exe --display short --debug-dependency-path
  Entering directory 'cmly'
      ocamllex lexer1.ml
      ocamldep .test.eobjs/lexer1.ml.d
      ocamldep .test.eobjs/test.ml.d
        menhir test_menhir1.{cmly,ml,mli}
      ocamldep .test.eobjs/test_menhir1.mli.d
      ocamldep .test.eobjs/test_menhir1.ml.d
        ocamlc .test.eobjs/byte/test_menhir1.{cmi,cmti}
      ocamlopt .test.eobjs/native/test_menhir1.{cmx,o}
        ocamlc .test.eobjs/byte/lexer1.{cmi,cmo,cmt}
        ocamlc .test.eobjs/byte/test.{cmi,cmo,cmt}
      ocamlopt .test.eobjs/native/test.{cmx,o}
      ocamlopt .test.eobjs/native/lexer1.{cmx,o}
      ocamlopt test.exe

  $ dune build --root general-2.0 src/test.exe --display short --debug-dependency-path
  Entering directory 'general-2.0'
      ocamllex src/lexer1.ml
      ocamldep src/.test.eobjs/lexer1.ml.d
      ocamllex src/lexer2.ml
      ocamldep src/.test.eobjs/lexer2.ml.d
      ocamldep src/.test.eobjs/test.ml.d
        menhir src/test_base__mock.ml.mock
      ocamldep src/.test.eobjs/test_base__mock.ml.mock.d
        ocamlc src/test_base__mock.mli.inferred
        menhir src/test_base.{ml,mli}
      ocamldep src/.test.eobjs/test_base.mli.d
        menhir src/test_menhir1__mock.ml.mock
      ocamldep src/.test.eobjs/test_menhir1__mock.ml.mock.d
        ocamlc src/test_menhir1__mock.mli.inferred
        menhir src/test_menhir1.{ml,mli}
      ocamldep src/.test.eobjs/test_menhir1.mli.d
      ocamldep src/.test.eobjs/test_base.ml.d
      ocamldep src/.test.eobjs/test_menhir1.ml.d
        ocamlc src/.test.eobjs/byte/test_menhir1.{cmi,cmti}
      ocamlopt src/.test.eobjs/native/test_menhir1.{cmx,o}
        ocamlc src/.test.eobjs/byte/test_base.{cmi,cmti}
      ocamlopt src/.test.eobjs/native/test_base.{cmx,o}
        ocamlc src/.test.eobjs/byte/lexer1.{cmi,cmo,cmt}
      ocamlopt src/.test.eobjs/native/lexer1.{cmx,o}
        ocamlc src/.test.eobjs/byte/lexer2.{cmi,cmo,cmt}
        ocamlc src/.test.eobjs/byte/test.{cmi,cmo,cmt}
      ocamlopt src/.test.eobjs/native/test.{cmx,o}
      ocamlopt src/.test.eobjs/native/lexer2.{cmx,o}
      ocamlopt src/test.exe

Reproduction case for #1781, only the .ml and .mli should be promoted:

  $ dune build @all --root promote
  Entering directory 'promote'
  $ ls -1 promote/_build/default | sort | grep mock
  parser__mock.ml.mock
  parser__mock.mli.inferred
  $ ls -1 promote | grep mock
  [1]
Check what is being generated exactly:
  $ ls -1 promote
  _build
  dune
  dune-project
  parser.ml
  parser.mli
  parser.mly
