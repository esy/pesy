  $ dune build @doc --display short
      ocamldep .bar.objs/bar.ml.d
        ocamlc .bar.objs/byte/bar.{cmi,cmo,cmt}
          odoc _doc/_odoc/pkg/bar/page-index.odoc
          odoc .bar.objs/byte/bar.odoc
          odoc _doc/_html/bar/index.html
          odoc _doc/_html/highlight.pack.js,_doc/_html/odoc.css
      ocamldep .foo.objs/foo.ml.d
        ocamlc .foo.objs/byte/foo.{cmi,cmo,cmt}
          odoc _doc/_odoc/pkg/foo/page-index.odoc
          odoc .foo.objs/byte/foo.odoc
      ocamldep .foo.objs/foo2.ml.d
        ocamlc .foo.objs/byte/foo2.{cmi,cmo,cmt}
          odoc .foo.objs/byte/foo2.odoc
      ocamldep .foo.objs/foo3.ml.d
        ocamlc .foo.objs/byte/foo3.{cmi,cmo,cmt}
          odoc .foo.objs/byte/foo3.odoc
      ocamldep .foo_byte.objs/foo_byte.ml.d
        ocamlc .foo_byte.objs/byte/foo_byte.{cmi,cmo,cmt}
          odoc .foo_byte.objs/byte/foo_byte.odoc
          odoc _doc/_html/foo/Foo2/.dune-keep,_doc/_html/foo/Foo2/index.html
          odoc _doc/_html/bar/Bar/.dune-keep,_doc/_html/bar/Bar/index.html
          odoc _doc/_html/foo/Foo3/.dune-keep,_doc/_html/foo/Foo3/index.html
          odoc _doc/_html/foo/Foo_byte/.dune-keep,_doc/_html/foo/Foo_byte/index.html
          odoc _doc/_html/foo/index.html
          odoc _doc/_html/foo/Foo/.dune-keep,_doc/_html/foo/Foo/index.html
  $ dune runtest --display short
  <!DOCTYPE html>
  <html xmlns="http://www.w3.org/1999/xhtml">
    <head>
      <title>index</title>
      <link rel="stylesheet" href="./odoc.css"/>
      <meta charset="utf-8"/>
      <meta name="viewport" content="width=device-width,initial-scale=1.0"/>
    </head>
    <body>
      <main class="content">
        <div class="by-name">
        <h2>OCaml package documentation</h2>
        <ol>
        <li><a href="bar/index.html">bar</a></li>
        <li><a href="foo/index.html">foo</a></li>
        </ol>
        </div>
      </main>
    </body>
  </html>

  $ dune build @foo-mld --display short
  {0 foo index}
  {1 Library foo}
  This library exposes the following toplevel modules:
  {!modules:Foo Foo2}
  {1 Library foo.byte}
  The entry point of this library is the module:
  {!module-Foo_byte}.

  $ dune build @bar-mld --display short
  {0 bar index}
  {1 Library bar}
  The entry point of this library is the module:
  {!module-Bar}.
