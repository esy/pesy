module Utils = PesyEsyPesyUtils.Utils;
open Utils;

type t;
module Mode: {
  type t;
  let ofFieldTypes: FieldTypes.t => t;
  let toStanza: t => Stanza.t;
};
let create: (string, option(Mode.t)) => t;
let toDuneStanza: (Common.t, t) => (string, list(Stanza.t));