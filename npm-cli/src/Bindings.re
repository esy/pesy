open Js;

module Error = {
  type t;
  [@bs.new] external make: string => t = "Error";
};

[@bs.val] [@bs.module "path"] external basename: string => string = "basename";
[@bs.val] [@bs.module "fs"] external makedirSync: string => unit = "mkdirSync";
[@bs.val] [@bs.module "fs"]
external renameSync: (string, string) => unit = "renameSync";

type stats;
[@bs.val] [@bs.module "fs"] external statSync: string => stats = "statSync";

[@bs.send] external isFile: stats => bool = "";

[@bs.val] [@bs.module "process"] external cwd: unit => string = "cwd";
[@bs.val] [@bs.module "process"] external argv: array(string) = "argv";
let scriptPath: string = [%raw "process.argv[1]"]; 
[@bs.val] external dirname: string = "__dirname";

[@bs.module]
external downloadGit: (string, string) => Js.Promise.t(unit) =
  "download-git-repo";

[@bs.module] external walk_sync: string => array(string) = "walk-sync";
module ChildProcess = {
  type t;

  exception ExecFailure((string, string, string));

  module Options = {
    type t;
    [@bs.obj]
    external make: (~cwd: string=?, ~env: Js.Dict.t(string)=?, unit) => t =
      "";
  };

  [@bs.val] [@bs.module "child_process"]
  external exec:
    (string, Options.t, (Js.Nullable.t(Error.t), string, string) => unit) =>
    t /* This should have been `t` - ChildProcess object in Node.js TODO: figure a way to pass it to callback */ =
    "exec";

  let exec = (cmd, options) => {
    Js.(
      Promise.(
        make((~resolve, ~reject) =>
          ignore @@
          exec(cmd, options, (err, stdout, stderr) =>
            if (Js.Nullable.isNullable(err)) {
              resolve(. Result.Ok((cmd, stdout, stderr)));
            } else {
              resolve(. Result.Error((cmd, stdout, stderr)));
            }
          )
        )
      )
    );
  };
};

module Process = {
  [@bs.val] [@bs.scope "process"] external cwd: unit => string = "cwd";
};

module Buffer = {
  type t;
  [@bs.send] external toString: t => string = "toString";
  [@bs.val] [@bs.scope "Buffer"] external from: string => t = "from";
  let ofString = from;
};

module Fs = {
  [@bs.module "../stubs/fs.js"]
  external writeFile: (. string, Buffer.t) => Js.Promise.t(unit) =
    "writeFile";

  [@bs.module "../stubs/fs.js"]
  external readFile: (. string) => Js.Promise.t(Buffer.t) = "readFile";

  let copy = (~dryRun=?, ~src, ~dest, ()) => {
    Promise.(
      {
        let dryRun =
          switch (dryRun) {
          | Some(x) => x
          | None => false
          };
        if (dryRun) {
          Js.log({j|Copying $src to $dest|j}) |> resolve;
        } else {
          readFile(. src) |> then_(b => writeFile(. dest, b));
        };
      }
    );
  };

  module DirEnt = {
    type t = {. "name": string};
    [@bs.send] external isDirectory: t => bool = "isDirectory";
    [@bs.send] external isFile: t => bool = "isFile";
  };

  module Dir = {
    type t = {. "path": string};
    [@bs.send] external close: t => Promise.t(unit) = "close";
    [@bs.send]
    external read: t => Promise.t(Js.Nullable.t(DirEnt.t)) = "read";
  };

  [@bs.module "../stubs/fs"]
  external opendir: string => Js.Promise.t(Dir.t) = "opendir";
  [@bs.module "../stubs/fs"]
  external readdir: string => Js.Promise.t(Js.Array.t(string)) = "readdir";

  module Stats = {
    type t;
    [@bs.send] external isDirectory: t => Js.Promise.t(bool) = "isDirectory";
  };

  [@bs.module "../stubs/fs"]
  external stat: string => Js.Promise.t(Stats.t) = "stat";

  let isDirectory = p =>
    Promise.(stat(p) |> then_(stats => Stats.isDirectory(stats)));

  [@bs.module "../stubs/fs.js"]
  external exists: string => Js.Promise.t(bool) = "exists";

  [@bs.module "../stubs/fs.js"]
  external mkdir': string => Js.Promise.t(unit) = "mkdir";

  let rec mkdir = (~dryRun=?, ~p=?, path) => {
    let forceCreate =
      switch (p) {
      | Some(x) => x
      | None => false
      };
    let dryRun =
      switch (dryRun) {
      | Some(x) => x
      | None => false
      };
    Js.Promise.(
      if (!dryRun) {
        if (forceCreate) {
          exists(path)
          |> then_(doesExist =>
               if (doesExist) {
                 resolve();
               } else {
                 let homePath = Sys.getenv(Sys.unix ? "HOME" : "USERPROFILE");
                 if (path == homePath) {
                   reject(
                     Failure(
                       "mkdir(~p=true) received home path and it was not found",
                     ),
                   );
                 } else {
                   mkdir(~p=true, Filename.dirname(path))
                   |> then_(() => mkdir'(path));
                 };
               }
             );
        } else {
          mkdir'(path);
        };
      } else {
        resolve();
      }
    );
  };
};

[@bs.module "source-map-support"]
external sourceMapSupportInstall: unit => unit = "install";

let throwJSError = [%raw "e => { throw e; }"];

[@bs.module]
external handlePromiseRejectInJS: Js.Promise.error => Js.Promise.t(unit) =
  "../stubs/handle-promise-rejection-in-js.js";

module Chalk = {
  [@bs.module "chalk"] external blue: string => string = "blue";
  [@bs.module "chalk"] external black: string => string = "black";
  [@bs.module "chalk"] external red: string => string = "red";
  [@bs.module "chalk"] external green: string => string = "green";
  [@bs.module "chalk"] external yellow: string => string = "yellow";
  [@bs.module "chalk"] external blue: string => string = "blue";
  [@bs.module "chalk"] external magenta: string => string = "magenta";
  [@bs.module "chalk"] external cyan: string => string = "cyan";
  [@bs.module "chalk"] external white: string => string = "white";
  [@bs.module "chalk"] external gray: string => string = "gray";
  [@bs.module "chalk"] external hex: (string, string) => string = "hex";
  [@bs.module "chalk"] external bold: string => string = "bold";
  [@bs.module "chalk"] external underline: string => string = "underline";
  [@bs.module "chalk"] external bgBlack: string => string = "bgBlack";
  [@bs.module "chalk"] external bgRed: string => string = "bgRed";
  [@bs.module "chalk"] external bgGreen: string => string = "bgGreen";
  [@bs.module "chalk"] external bgYellow: string => string = "bgYellow";
  [@bs.module "chalk"] external bgBlue: string => string = "bgBlue";
  [@bs.module "chalk"] external bgMagenta: string => string = "bgMagenta";
  [@bs.module "chalk"] external bgCyan: string => string = "bgCyan";
  [@bs.module "chalk"]
  external keyword: (string, string, string) => string = "keyword";
  [@bs.module "chalk"] external bgWhite: string => string = "bgWhite";
  [@bs.module "chalk"] external reset: string => string = "reset";
  [@bs.module "chalk"] external bold: string => string = "bold";
  [@bs.module "chalk"] external dim: string => string = "dim";
  [@bs.module "chalk"] external italic: string => string = "italic";
  [@bs.module "chalk"] external underline: string => string = "underline";
  [@bs.module "chalk"] external inverse: string => string = "inverse";
  [@bs.module "chalk"] external hidden: string => string = "hidden";
  [@bs.module "chalk"]
  external strikethrough: string => string = "strikethrough";
  [@bs.module "chalk"] external visible: string => string = "visible";
};

module Path = {
  [@bs.module "path"] external dirname: string => string = "dirname";
  [@bs.module "path"] [@bs.variadic]
  external join: array(string) => string = "join";
  [@bs.module "path"] [@bs.variadic]
  external resolve: array(string) => string = "resolve";
};
