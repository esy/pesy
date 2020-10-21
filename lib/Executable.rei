module Utils = PesyEsyPesyUtils.Utils;
open Utils;

type t;
module Mode: {
  type t;
  let ofFieldTypes: FieldTypes.t => t;
  let toStanzas: t => list(Stanza.t);
};
let create: (list((string, string)), option(bool), option(Mode.t)) => t;
let toDuneStanza: (Common.t, t) => (string, list(Stanza.t));
