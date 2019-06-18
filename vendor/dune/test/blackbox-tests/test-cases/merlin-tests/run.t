  $ dune build @print-merlins --display short --profile release
      ocamldep sanitize-dot-merlin/.sanitize_dot_merlin.eobjs/sanitize_dot_merlin.ml.d
        ocamlc sanitize-dot-merlin/.sanitize_dot_merlin.eobjs/byte/sanitize_dot_merlin.{cmi,cmo,cmt}
      ocamlopt sanitize-dot-merlin/.sanitize_dot_merlin.eobjs/native/sanitize_dot_merlin.{cmx,o}
      ocamlopt sanitize-dot-merlin/sanitize_dot_merlin.exe
  sanitize_dot_merlin alias print-merlins
  # Processing exe/.merlin
  EXCLUDE_QUERY_DIR
  B $LIB_PREFIX/lib/bytes
  B $LIB_PREFIX/lib/findlib
  B $LIB_PREFIX/lib/ocaml
  B ../_build/default/exe/.x.eobjs/byte
  B ../_build/default/lib/.foo.objs/public_cmi
  S $LIB_PREFIX/lib/bytes
  S $LIB_PREFIX/lib/findlib
  S $LIB_PREFIX/lib/ocaml
  S .
  S ../lib
  FLG -pp '$PP/_build/default/exe/foo-bar'
  FLG -w -40
  # Processing lib/.merlin
  EXCLUDE_QUERY_DIR
  B $LIB_PREFIX/lib/bytes
  B $LIB_PREFIX/lib/findlib
  B $LIB_PREFIX/lib/ocaml
  B ../_build/default/lib/.bar.objs/byte
  B ../_build/default/lib/.foo.objs/byte
  S $LIB_PREFIX/lib/bytes
  S $LIB_PREFIX/lib/findlib
  S $LIB_PREFIX/lib/ocaml
  S .
  S subdir
  FLG -open Foo -w -40 -open Bar -w -40

Make sure a ppx directive is generated

  $ grep -q ppx lib/.merlin
  [1]
