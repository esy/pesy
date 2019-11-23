# OCamlbuild #

OCamlbuild is a generic build tool, that has built-in rules for
building OCaml library and programs.

OCamlbuild was distributed as part of the OCaml distribution for OCaml
versions between 3.10.0 and 4.02.3. Starting from OCaml 4.03, it is
now released separately.

Your should refer to the [OCambuild
manual](https://github.com/ocaml/ocamlbuild/blob/master/manual/manual.adoc)
for more informations on how to use ocamlbuild.

## Automatic Installation ##

With [opam](https://opam.ocaml.org/):

```
opam install ocamlbuild
```

If you are testing a not yet released version of OCaml, you may need
to use the development version of OCamlbuild. With opam:

```
opam pin add ocamlbuild --kind=git "https://github.com/ocaml/ocamlbuild.git#master"
```

## Compilation from source ##

We assume GNU make, which may be named `gmake` on your system.

1. Configure.
```
make configure
```

The installation location is determined by the installation location
of the ocaml compiler. You can set the following configuration
variables (`make configure VAR=foo`):

- `OCAMLBUILD_{PREFIX,BINDIR,LIBDIR}` will use opam or
  ocaml/ocamlfind's settings by default; see `configure.make` for the
  precise initialization logic.

- `OCAML_NATIVE`: should be `true` if native compilation is available
  on your machine, `false` otherwise

2. Compile the sources.
```
make
```

3. Install.
```
make install
```

You can also clean the compilation results with `make clean`, and
uninstall a manually-installed OCamlbuild with `make uninstall`.
