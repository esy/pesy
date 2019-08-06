[@bs.val] [@bs.module "fs"] external makedirSync: string => unit = "mkdirSync";
[@bs.val] [@bs.module "fs"]
external renameSync: (string, string) => unit = "renameSync";

type stats;
[@bs.val] [@bs.module "fs"] external statSync: string => stats = "statSync";

[@bs.send] external isFile: stats => bool = "";

[@bs.val] [@bs.module "process"] external cwd: unit => string = "cwd";
[@bs.val] [@bs.module "process"] external argv: array(string) = "argv";
[@bs.val] external scriptPath: string = "__dirname";
[@bs.val] [@bs.module "child_process"]
external exec: (string, (Js.Nullable.t(_), string, string) => unit) => unit =
  "";

[@bs.module]
external download_git: (string, string, option('a) => unit) => unit =
  "download-git-repo";

[@bs.module] external walk_sync: string => array(string) = "walk-sync";
