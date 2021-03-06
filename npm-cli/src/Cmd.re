open Bindings;

type t = {
  cmd: string,
  env: Js.Dict.t(string),
};

type stdout = string;

type stderr = string;

let pathMissingFromEnv = "'PATH' variable not found in the environment";

let env_sep = Process.platform == "win32" ? ";" : ":";
let binPath = c => c.cmd;

let make = (~env, ~cmd) =>
  switch (Js.Dict.get(env, "PATH")) {
  | None => Error(pathMissingFromEnv) |> Js.Promise.resolve
  | Some(path) =>
    let cmds =
      Process.platform == "win32"
        ? [|cmd ++ ".exe", cmd ++ ".cmd"|] : [|cmd|];

    cmds
    |> Array.map(cmd =>
         Js.String.split(env_sep, path)
         |> Js.Array.map(p => Path.join([|p, cmd|]))
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

let ofPathStr = path => {
  Fs.exists(path)
  |> Js.Promise.then_(exists =>
       if (exists) {
         Ok({cmd: path, env: Process.env}) |> Js.Promise.resolve;
       } else {
         Error({j| Command ($path) not found |j}) |> Js.Promise.resolve;
       }
     );
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

let spawn = (~args, ~cwd, ~cmd) => {
  let {cmd, env: _} = cmd;
  Js.Promise.make((~resolve, ~reject as _) => {
    let process =
      ChildProcess.spawn(
        cmd,
        args,
        ChildProcess.Options.make(~cwd, ~stdio="inherit", ()),
      );
    ChildProcess.onClose(process, exitCode =>
      if (exitCode == 0) {
        resolve(. Ok());
      } else {
        resolve(.
          Error(Js.Array.concat(args, [|cmd|]) |> Js.Array.joinWith(" ")),
        );
      }
    );
  });
};
