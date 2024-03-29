module Mode: {
  exception InvalidLibraryMode(string);
  type t;
  let ofString: string => t;
  let toString: t => string;
};
type t;
let create:
  (
    string,
    bool,
    string,
    option(list(Mode.t)),
    option(Stubs.t),
    option(list(string)),
    option(list(string)),
    option(bool)
  ) =>
  t;
let toDuneStanza: (Common.t, t) => (string, list(Stanza.t));
