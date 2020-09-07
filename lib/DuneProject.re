open Sexplib.Sexp;

exception InvalidDuneProjectFile;
let findLangVersion =
  fun
  | [x, ..._] =>
    switch (Stanza.toSexp(x)) {
    | List([Atom("lang"), Atom("dune"), Atom(version)]) => version
    | _ => raise(InvalidDuneProjectFile)
    }
  | _ => raise(InvalidDuneProjectFile);