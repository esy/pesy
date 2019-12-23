module Utils = PesyEsyPesyUtils.Utils;
open Sexplib.Sexp;
open Printf;
exception InvalidDuneFile(unit);
let toString = (stanzas: list(Stanza.t)) =>
  List.fold_right(
    (s, acc) =>
      Str.global_replace(
        Str.regexp("\"\\\\\\\\\""),
        "\\\\",
        to_string_hum(~indent=4, Stanza.toSexp(s)),
      )
      ++ "\n"
      ++ acc,
    stanzas,
    "",
  );

let ofFile = n => {
  open Sexplib;
  let contentStr =
    try (Utils.readFile(n)) {
    | _ => ""
    };
  Sexp.(
    switch (Sexp.of_string(sprintf("(%s)", contentStr))) {
    | List(l) => List.map(Stanza.ofSexp, l)
    | _ => raise(InvalidDuneFile())
    }
  );
};
