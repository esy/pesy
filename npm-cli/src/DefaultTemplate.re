open Bindings;

let path =
  Path.resolve([|
    dirname,
    "templates",
    "pesy-reason-template-0.1.0-alpha.11",
  |]);

let ciPath = Path.resolve([|dirname, "templates", "ci"|]);
let dockerPath = Path.resolve([|dirname, "templates", "docker"|]);
