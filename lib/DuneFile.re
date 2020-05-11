module Utils = PesyEsyPesyUtils.Utils;
open Sexp;
open Printf;
exception InvalidDuneFile(unit);
let toString = (stanzas: list(Stanza.t)) =>
  List.fold_right(
    (s, acc) =>
      Str.global_replace(
        Str.regexp("\"\\\\\\\\\""),
        "\\\\",
        to_string_hum(Stanza.toSexp(s)),
      )
      ++ "\n"
      ++ acc,
    stanzas,
    "",
  );

let ofFile = n => {
  let contentStr =
    try(Utils.readFile(n)) {
    | _ => ""
    };
  Sexp.(
    switch (Sexp.parse_string(sprintf("(%s)", contentStr))) {
    | List(l) => List.map(Stanza.ofSexp, l)
    | _ => raise(InvalidDuneFile())
    }
  );
};
