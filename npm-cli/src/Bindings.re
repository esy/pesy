open Js;

module Process = {
  type t;
  [@bs.val] external v: t = "process";
  [@bs.val] [@bs.scope "process"] external cwd: unit => string = "cwd";
  [@bs.val] [@bs.scope "process"] external platform: string = "platform";
  [@bs.val] [@bs.scope "process"] external env: Js.Dict.t(string) = "env";
  module Stdout = {
    type t;
    [@bs.val] external v: t = "process.stdout";
    [@bs.send] external write: (t, string) => unit = "write";
  };
};

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

[@bs.send] external isFile: stats => bool = "isFile";

[@bs.val] [@bs.module "process"] external cwd: unit => string = "cwd";
[@bs.val] [@bs.module "process"] external argv: array(string) = "argv";
let scriptPath: string = [%raw "process.argv[1]"];
[@bs.val] external dirname: string = "__dirname";

[@bs.module]
external downloadGit: (string, string, Js.Nullable.t(Error.t) => unit) => unit =
  "download-git-repo";

let downloadGit = (repo, path) =>
  Promise.make((~resolve, ~reject as _) => {
    downloadGit(repo, path, err => {
      switch (Nullable.toOption(err)) {
      | Some(x) => resolve(. Result.Error(x))
      | None => resolve(. Result.Ok())
      }
    })
  });

module Buffer = {
  type t;
  [@bs.send] external toString: t => string = "toString";
  [@bs.val] [@bs.scope "Buffer"] external from: string => t = "from";
  let ofString = from;
};

module type STREAM = {
  type t;
  let onData: (t, string, Buffer.t => unit) => unit;
  let onEnd: (t, string, unit => unit) => unit;
  let onClose: (t, string, unit => unit) => unit;
};

module StreamFunctor = (S: {type t;}) => {
  type t = S.t;
  [@bs.send] external onData': (t, string, Buffer.t => unit) => unit = "on";
  let onData = (t, cb) => onData'(t, "data", cb);
  [@bs.send] external onEnd': (t, string, unit => unit) => unit = "on";
  let onEnd = (t, cb) => onEnd'(t, "end", cb);
  let onClose = (t, cb) => onEnd'(t, "close", cb);
};

module Stream =
  StreamFunctor({
    type t;
  });

[@bs.module] external walk_sync: string => array(string) = "walk-sync";
module ChildProcess: {
  type t;

  module Options: {
    type t;
    let make: (~cwd: string=?, ~env: Js.Dict.t(string)=?, ~stdio: string=?, unit) => t;
  };
  exception ExecFailure((string, string, string));

  let spawn: (string, array(string), Options.t) => t;
  let onClose: (t, unit => unit) => unit;
  let exec:
    (string, Options.t) =>
    Js.Promise.t(
      result((string, string, string), (string, string, string)),
    );
} = {
  type t;

  exception ExecFailure((string, string, string));

  module Options = {
    type t;
    [@bs.obj]
    external make: (~cwd: string=?, ~env: Js.Dict.t(string)=?, ~stdio: string=?, unit) => t =
      "";
  };

  [@bs.module "child_process"]
  external spawn: (string, array(string), Options.t) => t = "spawn";

 [@bs.send]
  external on': (t, string, unit => unit) => unit = "on";

  let onClose = (t, cb) => on'(t, "close", cb);

  [@bs.module "child_process"]
  external exec:
    (string, Options.t, (Js.Nullable.t(Error.t), string, string) => unit) =>
    t /* This should have been `t` - ChildProcess object in Node.js TODO: figure a way to pass it to callback */ =
    "exec";

  let spawn = spawn;

  let exec = (cmd, options) => {
    Js.(
      Promise.(
        make((~resolve, ~reject) =>
          ignore @@
          exec(cmd, options, (err, stdout, stderr) =>
            if (Js.Nullable.isNullable(err)) {
              resolve(. Ok((cmd, stdout, stderr)));
            } else {
              resolve(. Error((cmd, stdout, stderr)));
            }
          )
        )
      )
    );
  };
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
  // https://github.com/ecliptic/bucklescript-tools/blob/develop/packages/bs-chalk/src/Chalk.re

  module Level = {
    type t =
      | None
      | Basic
      | Ansi256
      | TrueColor;
    let fromInt = level =>
      switch (level) {
      | 0 => None
      | 1 => Basic
      | 2 => Ansi256
      | 3 => TrueColor
      | _ => None
      };
    let toInt = level =>
      switch (level) {
      | None => 0
      | Basic => 1
      | Ansi256 => 2
      | TrueColor => 3
      };
  };

  type colorSupport = {
    .
    "level": int,
    "hasBasic": bool,
    "has256": bool,
    "has16m": bool,
  };

  [@bs.module "chalk"] external enabled: bool = "enabled";

  [@bs.module "chalk"] external level_: int = "level";

  [@bs.module "chalk"] external supportsColor: colorSupport = "supportsColor";

  [@bs.module "chalk"] external stripColor: string => string = "stripColor";

  [@bs.module "chalk"] external hasColor: string => bool = "hasColor";

  [@bs.module "chalk"] external visible: string => bool = "visible";

  /* Styles */
  [@bs.module "chalk"] external reset: string => string = "chalk";

  [@bs.module "chalk"] external bold: string => string = "bold";

  [@bs.module "chalk"] external dim: string => string = "dim";

  [@bs.module "chalk"] external italic: string => string = "italic";

  [@bs.module "chalk"] external underline: string => string = "underline";

  [@bs.module "chalk"] external inverse: string => string = "inverse";

  [@bs.module "chalk"] external hidden: string => string = "hidden";

  [@bs.module "chalk"]
  external strikethrough: string => string = "strikethrough";

  /* Colors */
  [@bs.module "chalk"] external black: string => string = "black";

  [@bs.module "chalk"] external red: string => string = "red";

  [@bs.module "chalk"] external green: string => string = "green";

  [@bs.module "chalk"] external yellow: string => string = "yellow";

  [@bs.module "chalk"] external blue: string => string = "blue";

  [@bs.module "chalk"] external magenta: string => string = "magenta";

  [@bs.module "chalk"] external cyan: string => string = "cyan";

  [@bs.module "chalk"] external white: string => string = "white";

  [@bs.module "chalk"] external gray: string => string = "gray";

  [@bs.module "chalk"] external grey: string => string = "grey";

  [@bs.module "chalk"] external blackBright: string => string = "blackBright";

  [@bs.module "chalk"] external redBright: string => string = "redBright";

  [@bs.module "chalk"] external greenBright: string => string = "greenBright";

  [@bs.module "chalk"]
  external yellowBright: string => string = "yellowBright";

  [@bs.module "chalk"] external blueBright: string => string = "blueBright";

  [@bs.module "chalk"]
  external magentaBright: string => string = "magentaBright";

  [@bs.module "chalk"] external cyanBright: string => string = "cyanBright";

  [@bs.module "chalk"] external whiteBright: string => string = "whiteBright";

  [@bs.module "chalk"] external bgBlack: string => string = "bgBlack";

  [@bs.module "chalk"] external bgRed: string => string = "bgRed";

  [@bs.module "chalk"] external bgGreen: string => string = "bgGreen";

  [@bs.module "chalk"] external bgYellow: string => string = "bgYellow";

  [@bs.module "chalk"] external bgBlue: string => string = "bgBlue";

  [@bs.module "chalk"] external bgMagenta: string => string = "bgMagenta";

  [@bs.module "chalk"] external bgCyan: string => string = "bgCyan";

  [@bs.module "chalk"] external bgWhite: string => string = "bgWhite";

  [@bs.module "chalk"]
  external bgBlackBright: string => string = "bgBlackBright";

  [@bs.module "chalk"] external bgRedBright: string => string = "bgRedBright";

  [@bs.module "chalk"]
  external bgGreenBright: string => string = "bgGreenBright";

  [@bs.module "chalk"]
  external bgYellowBright: string => string = "bgYellowBright";

  [@bs.module "chalk"]
  external bgBlueBright: string => string = "bgBlueBright";

  [@bs.module "chalk"]
  external bgMagentaBright: string => string = "bgMagentaBright";

  [@bs.module "chalk"]
  external bgCyanBright: string => string = "bgCyanBright";

  [@bs.module "chalk"]
  external bgWhiteBright: string => string = "bgWhiteBright";

  /* Exact colors */
  [@bs.module "chalk"] external hex: string => string = "hex";

  [@bs.module "chalk"] external rgb_: (. int, int, int) => string = "rgb";

  [@bs.module "chalk"] external hsl_: (. int, int, int) => string = "hsl";

  [@bs.module "chalk"] external hsv_: (. int, int, int) => string = "hsv";

  [@bs.module "chalk"] external hwb_: (. int, int, int) => string = "hwb";

  [@bs.module "chalk"] external bgHex: string => string = "bgHex";

  [@bs.module "chalk"] external bgRgb_: (. int, int, int) => string = "bgRgb";

  [@bs.module "chalk"] external bgHsl_: (. int, int, int) => string = "bgHsl";

  [@bs.module "chalk"] external bgHsv_: (. int, int, int) => string = "bgHsv";

  [@bs.module "chalk"] external bgHwb_: (. int, int, int) => string = "bgHwb";

  [@bs.module "chalk"] external keyword: string => string = "keyword";

  /* Convenience Methods */
  let level: Level.t = level_ |> Level.fromInt;

  let rgb = (~r: int, ~g: int, ~b: int) => rgb_(. r, g, b);

  let hsl = (~h: int, ~s: int, ~l: int) => hsl_(. h, s, l);

  let hsv = (~h: int, ~s: int, ~v: int) => hsv_(. h, s, v);

  let hwb = (~h: int, ~w: int, ~b: int) => hwb_(. h, w, b);

  let bgRgb = (~r: int, ~g: int, ~b: int) => bgRgb_(. r, g, b);

  let bgHsl = (~h: int, ~s: int, ~l: int) => bgHsl_(. h, s, l);

  let bgHsv = (~h: int, ~s: int, ~v: int) => bgHsv_(. h, s, v);

  let bgHwb = (~h: int, ~w: int, ~b: int) => bgHwb_(. h, w, b);
};

module Path = {
  [@bs.module "path"] external dirname: string => string = "dirname";
  [@bs.module "path"] [@bs.variadic]
  external join: array(string) => string = "join";
  [@bs.module "path"] [@bs.variadic]
  external resolve: array(string) => string = "resolve";
};
