  $ dune runtest --display short -p lib1 --debug-dependency-path
      ocamldep .lib1.objs/lib1.ml.d
        ocamlc .lib1.objs/byte/lib1.{cmi,cmo,cmt}
      ocamlopt .lib1.objs/native/lib1.{cmx,o}
      ocamlopt lib1.{a,cmxa}
      ocamldep .test1.eobjs/test1.ml.d
        ocamlc .test1.eobjs/byte/test1.{cmi,cmo,cmt}
      ocamlopt .test1.eobjs/native/test1.{cmx,o}
      ocamlopt test1.exe
         test1 alias runtest
