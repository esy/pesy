[@bs.val] [@bs.module "fs"] external makedirSync: string => unit = "mkdirSync";
[@bs.val] [@bs.module "process"] external cwd: unit => string = "cwd";
[@bs.val] external scriptPath: string = "__dirname";
[@bs.val] [@bs.module "child_process"]
external exec: (string, (Js.Nullable.t(_), string, string) => unit) => unit =
  "";
