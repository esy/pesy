# Menhir

Menhir is an LR(1) parser generator for OCaml.

Menhir has a [home page](http://gallium.inria.fr/~fpottier/menhir/).

## Installation

OCaml (4.02 or later), ocamlbuild, and GNU make are needed.

The latest released version of Menhir can be easily installed via
`opam`, OCaml's package manager. Just type `opam install menhir`.

For manual installation, see [INSTALLATION.md](INSTALLATION.md).

Some instructions for developers can be found in [HOWTO.md](HOWTO.md).

## The Coq backend support library coq-menhirlib

The support library for the Coq backend of Menhir can be found in the
coq-menhirlib directory. It can be installed using
`opam install coq-menhirlib`, when the opam Coq "released" repository
is set up.

## Authors

* [François Pottier](Francois.Pottier@inria.fr)
* [Yann Régis-Gianas](Yann.Regis-Gianas@pps.jussieu.fr)

## Contributors

* Frédéric Bour (incremental engine, inspection API, attributes, SDK)
* Jacques-Henri Jourdan (Coq back-end)
