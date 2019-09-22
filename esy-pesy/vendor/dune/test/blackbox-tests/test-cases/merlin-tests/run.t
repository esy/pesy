  $ dune build @print-merlins --display short --profile release
      ocamldep pp/.pp.eobjs/pp.ml.d
        ocamlc pp/.pp.eobjs/byte/pp.{cmi,cmo,cmt}
      ocamlopt pp/.pp.eobjs/native/pp.{cmx,o}
      ocamlopt pp/pp.exe
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
  FLG -pp '$PP/_build/default/pp/pp.exe'
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
  FLG -ppx '$PPX/eff0afe1b49c6b759cd98bcc60fefc58/ppx.exe --as-ppx --cookie '\''library-name="foo"'\'''
  FLG -open Foo -w -40 -open Bar -w -40

Make sure a ppx directive is generated

  $ grep -q ppx lib/.merlin

Make sure pp flag is correct and variables are expanded

  $ dune build @print-merlins-pp
  sanitize_dot_merlin alias print-merlins-pp
  # Processing pp-with-expand/.merlin
  EXCLUDE_QUERY_DIR
  B ../_build/default/pp-with-expand/.foobar.eobjs/byte
  S .
  FLG -pp '$PP/_build/default/pp/pp.exe -nothing'
  FLG -w @1..3@5..28@30..39@43@46..47@49..57@61..62-40 -strict-sequence -strict-formats -short-paths -keep-locs

We want future-syntax to either be applied, or not, depending on OCaml version.
Adding the `echo` with expected output to the set of lines is a way of achieving that.

  $ (echo "FLG -pp '\$BIN/ocaml-syntax-shims'"; dune build @print-merlins-future-syntax 2>&1) | sort | uniq
  # Processing future-syntax/.merlin
  B ../_build/default/future-syntax/.pp_future_syntax.eobjs/byte
  EXCLUDE_QUERY_DIR
  FLG -pp '$BIN/ocaml-syntax-shims'
  FLG -w @1..3@5..28@30..39@43@46..47@49..57@61..62-40 -strict-sequence -strict-formats -short-paths -keep-locs
  S .
  sanitize_dot_merlin alias print-merlins-future-syntax

