type t = Cmd.t;
let make: unit => Js.Promise.t(result(t, string));
let manifestPath: (string, t) => Js.Promise.t(result(string, string));
let status: (string, t) => Js.Promise.t(result(EsyStatus.t, string));
let importDependencies:
  (~projectPath: Path.t, ~exportsPath: Path.t, t) =>
  Js.Promise.t(result((string, string), string));
