open Bindings;
open Utils;

type t = {
  cmd: string,
  env: Js.Dict.t(string),
};

type stdout = string;

type stderr = string;

let pathMissingFromEnv = "'PATH' variable not found in the environment";

let env_sep = Sys.unix ? ":" : ";";
let binPath = c => c.cmd;

let make = (~env, ~cmd) =>
  switch (Js.Dict.get(env, "PATH")) {
  | None => Error(pathMissingFromEnv) |> Js.Promise.resolve
  | Some(path) =>
    let cmds = Sys.unix ? [|cmd|] : [|cmd ++ ".exe", cmd ++ ".cmd"|];

    cmds
    |> Array.map(cmd =>
         Js.String.split(env_sep, path)
         |> Js.Array.map(p => Filename.concat(p, cmd))
       )
    |> Js.Array.reduce(Js.Array.concat, [||])
    |> Js.Array.map(p =>
         Fs.exists(p)
         |> Js.Promise.then_(exists => Js.Promise.resolve((p, exists)))
       )
    |> Js.Promise.all
    |> Js.Promise.then_(r =>
         Js.Promise.resolve(Js.Array.filter(((_p, exists)) => exists, r))
       )
    |> Js.Promise.then_(r => {
         let r = Array.to_list(r);
         let r =
           switch (r) {
           | [] => Error({j| Command "$cmd" not found |j})
           | [(cmd, _exists), ..._rest] => Ok({cmd, env})
           };

         Js.Promise.resolve(r);
       });
  };

let output = (~args, ~cwd, ~cmd) => {
  let {cmd, env} = cmd;
  let shellString =
    Js.Array.concat(args, [|cmd|]) |> Js.Array.joinWith(" ");
  ChildProcess.exec(shellString, ChildProcess.Options.make(~cwd, ~env, ()))
  |> Js.Promise.then_(r => {
       let r =
         switch (r) {
         | Error () => Result.fail({j| Exec failed: $shellString |j})
         | Ok((exitCode, stdout, stderr)) =>
           if (exitCode == 0) {
             Ok((stdout, stderr));
           } else {
             Error(
               {j| Command $cmd failed:
exitCode: $exitCode
stderr: $stderr
|j},
             );
           }
         };

       Js.Promise.resolve(r);
     });
};
