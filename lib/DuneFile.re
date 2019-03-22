open Sexplib.Sexp;
open Printf;

exception InvalidDuneFile(unit);
let toString = (stanzas: list(Stanza.t)) =>
  List.fold_right(
    (s, acc) => to_string_hum(~indent=4, Stanza.toSexp(s)) ++ " " ++ acc,
    stanzas,
    "",
  );

let ofFile = n => {
  open Sexplib;
  let contentStr = Utils.readFile(n);
  Sexp.(
    switch (Sexp.of_string(sprintf("(%s)", contentStr))) {
    | List(l) => List.map(Stanza.ofSexp, l)
    | _ => raise(InvalidDuneFile())
    }
  );
};
