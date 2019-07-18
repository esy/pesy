type t;
module Mode: {
  type t;
  let ofList: list(string) => t;
  let toList: t => list(string);
};
let create: (string, option(Mode.t)) => t;
let toDuneStanza: (Common.t, t, option(string)) => (string, list(Stanza.t));
