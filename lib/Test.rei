type t;
module Mode: {
  type t;
  let ofList: list(string) => t;
  let toList: t => list(string);
};
let create: (string, option(Mode.t)) => t;
let toDuneStanza:
  (Common.t, t, option(PesyModule.t)) => (string, list(Stanza.t));
