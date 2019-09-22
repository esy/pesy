Successes:

  $ dune build --display short --root foo --debug-dep
  Entering directory 'foo'
      ocamldep .foo.objs/foo.ml.d
        ocamlc .foo.objs/byte/foo__.{cmi,cmo,cmt}
      ocamlopt .foo.objs/native/foo__.{cmx,o}
      ocamldep .foo.objs/intf.mli.d
        ocamlc .foo.objs/byte/foo__Intf.{cmi,cmti}
        ocamlc .foo.objs/byte/foo.{cmi,cmo,cmt}
        ocamlc foo.cma
      ocamldep test/.bar.objs/bar.ml.d
        ocamlc test/.bar.objs/byte/bar.{cmi,cmo,cmt}
        ocamlc test/bar.cma
      ocamlopt .foo.objs/native/foo.{cmx,o}
      ocamlopt foo.{a,cmxa}
      ocamlopt foo.cmxs
      ocamlopt test/.bar.objs/native/bar.{cmx,o}
      ocamlopt test/bar.{a,cmxa}
      ocamlopt test/bar.cmxs

Errors:

  $ dune build --display short --root a foo.cma
  Entering directory 'a'
  File "dune", line 1, characters 0-21:
  1 | (library
  2 |  (name foo))
  Warning: Some modules don't have an implementation.
  You need to add the following field to this stanza:
  
    (modules_without_implementation x y)
  
  This will become an error in the future.
        ocamlc .foo.objs/byte/foo.{cmi,cmo,cmt}
        ocamlc foo.cma
  $ dune build --display short --root b foo.cma
  Entering directory 'b'
  File "dune", line 3, characters 33-34:
  3 |  (modules_without_implementation x))
                                       ^
  Warning: The following modules must be listed here as they don't have an
  implementation:
  - Y
  This will become an error in the future.
        ocamlc .foo.objs/byte/foo.{cmi,cmo,cmt}
        ocamlc foo.cma
  $ dune build --display short --root c foo.cma
  Entering directory 'c'
  File "dune", line 3, characters 33-34:
  3 |  (modules_without_implementation x))
                                       ^
  Error: Module X doesn't exist.
  [1]
  $ dune build --display short --root d foo.cma
  Entering directory 'd'
  File "dune", line 3, characters 33-34:
  3 |  (modules_without_implementation x))
                                       ^
  Error: The following modules have an implementation, they cannot be listed as
  modules_without_implementation:
  - X
  [1]
