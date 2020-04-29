Bindings.sourceMapSupportInstall();

open Cmdliner;
open Utils;
open Bindings;

[%raw "process.argv.shift()"];

let setup = (esy, template, projectPath, bootstrapOnly) =>
  Fs.mkdir(~p=true, projectPath)
  |> Js.Promise.then_(() => {
       Process.chdir(projectPath);
       Bootstrapper.run(esy, projectPath, template, bootstrapOnly);
     });

let promptEmptyDirectory = () =>
  Js.Promise.make((~resolve, ~reject as _) => {
    askYesNoQuestion(
      ~question="Current directory is not empty. Are you sure to continue?",
      ~onYes=() => resolve(. Result.return()),
      ~onNo=() => resolve(. Result.fail("Aborting setup")),
      (),
    )
  });

let handleEmptyDirectory =
  (. path, force) =>
    if (Dir.isEmpty(path) || force) {
      ResultPromise.ok();
    } else {
      promptEmptyDirectory();
    };

let main' = (projectPath, template, useDefaultOptions, bootstrapOnly) => {
  ResultPromise.(
    Cmd.make(~cmd="esy", ~env=Process.env)
    >>= (
      esy => {
        let projectPath =
          Path.isAbsolute(projectPath)
            ? projectPath : Path.join([|Process.cwd(), projectPath|]);
        handleEmptyDirectory(. projectPath, useDefaultOptions)
        >>= (() => setup(esy, template, projectPath, bootstrapOnly));
      }
    )
  )
  |> Js.Promise.then_(
       fun
       | Ok () => Js.Promise.resolve()
       | Error(msg) => Js.log(msg) |> Js.Promise.resolve,
     )
  |> Js.Promise.catch(handlePromiseRejectInJS);
};

let warmup = () => {
  let projectPath = Process.cwd();
  Warmup.run(projectPath)
  |> Js.Promise.then_(
       fun
       | Ok () => Js.Promise.resolve()
       | Error(msg) => Js.log2("Error", msg) |> Js.Promise.resolve,
     );
};

let main = (projectPath, template, useDefaultOptions, bootstrapOnly) =>
  try(main'(projectPath, template, useDefaultOptions, bootstrapOnly)) {
  | Js.Exn.Error(e) =>
    let message =
      switch (Js.Exn.message(e)) {
      | Some(message) => message
      | None => ""
      };
    let stack =
      switch (Js.Exn.stack(e)) {
      | Some(stack) => stack
      | None => ""
      };
    Js.log(message);
    Js.log(stack);
    Js.Promise.resolve();
  };

let version = "0.5.0-dev";
let template = {
  let doc = "Specify URL of the remote template. This can be of the form https://repo-url.git#<commit|branch|tag>. Eg: https://github.com/reason-native-web/morph-hello-world-pesy-template#6e5cbbb9f28";
  Arg.(
    value
    & opt(some(string), None)
    & info(["t", "template"], ~docv="TEMPLATE_URL", ~doc)
  );
};

let directory = {
  let doc = "Initialises a new project at given directory";
  Arg.(
    value & opt(string, Process.cwd()) & info(["init"], ~docv="INIT", ~doc)
  );
};

let useDefaultOptions = {
  let doc = "Use default options";
  Arg.(value & flag & info(["y", "yes"], ~doc));
};

let bootstrapOnly = {
  let doc = "Only create initial set of files. Dont run esy commands on them.";
  Arg.(value & flag & info(["-b", "bootstrap-only"], ~doc));
};

open Cmdliner.Term;
let cmd = {
  let cmd = "pesy";
  let envs: list(env_info) = [];
  let exits: list(exit_info) = [];
  let doc = "Your Esy Assistant.";
  let cmdT =
    Term.(
      const(main) $ directory $ template $ useDefaultOptions $ bootstrapOnly
    );
  (cmdT, Term.info(cmd, ~envs, ~exits, ~doc, ~version));
};

let warmupCmd = {
  let cmd = "warmup";
  let envs: list(env_info) = [];
  let exits: list(exit_info) = [];
  let doc = "warmup - warms esy cache with prebuilts from CI";
  let cmdT = Term.(const(warmup) $ const());
  (cmdT, Term.info(cmd, ~envs, ~exits, ~doc, ~version));
};

Term.eval_choice(cmd, [warmupCmd]);
