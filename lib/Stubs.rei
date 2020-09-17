module Utils = PesyEsyPesyUtils.Utils;
open Utils;

type t;

let ofCNames: list(string) => t;

let ofForeignStubs: list(list((string, FieldTypes.t))) => t;

let toDuneStanza: option(t) => list(option(Stanza.t));