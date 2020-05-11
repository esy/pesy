open Sexp;
type t = Sexp.t;
let createAtom = a => Atom(a);
let create = (stanza: string, expression) =>
  List([Atom(stanza), expression]);
let createExpression = atoms => List(atoms);
let ofString = Sexp.parse_string;
let toSexp = x => x;
let ofSexp = x => x;
