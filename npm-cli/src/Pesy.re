Bindings.sourceMapSupportInstall();

open Cmdliner;
open Utils;
open Bindings;
open ResultPromise;

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

let main = (projectPath, template, useDefaultOptions, bootstrapOnly) => {
  let projectPath =
    Path.isAbsolute(projectPath)
      ? projectPath : Path.join([|Process.cwd(), projectPath|]);
  handleEmptyDirectory(. projectPath, useDefaultOptions)
  >>= (() => Cmd.make(~cmd="esy", ~env=Process.env))
  >>= (esy => Bootstrapper.run(esy, projectPath, template, bootstrapOnly))
  |> catch;
};

let warmup = () => {
  let projectPath = Process.cwd();
  Warmup.run(projectPath) |> ResultPromise.catch;
};

let testTemplate = templatePath => {
  let tmpdir = Os.tmpdir();
  let testTemplateDir = "test-template";
  let testTemplatePath = Path.join([|tmpdir, testTemplateDir|]);
  Rimraf.run(testTemplatePath)
  >>= (
    () =>
      Fs.mkdir(~p=true, testTemplatePath)
      |> Js.Promise.then_(() => Cmd.make(~cmd="esy", ~env=Process.env))
  )
  >>= (
    esy => {
      Js.log({j| Creating test-project $testTemplatePath |j});
      Bootstrapper.run(
        esy,
        testTemplatePath,
        Template.Kind.path(templatePath),
        false,
      );
    }
  );
};

let version = "0.5.0-dev";
let template = {
  let doc = "Specify URL of the remote template. This can be of the form https://repo-url.git#<commit|branch|tag>. Eg: https://github.com/reason-native-web/morph-hello-world-pesy-template#6e5cbbb9f28";
  Arg.(
    value
    & opt(
        Template.Kind.cmdlinerConv,
        Template.Kind.path(
          Path.resolve([|
            dirname,
            "templates",
            "pesy-reason-template-0.1.0-alpha.6",
          |]),
        ),
      )
    & info(["t", "template"], ~docv="TEMPLATE_URL", ~doc)
  );
};

let directory = {
  let doc = "Initialises a new project at given directory";
  Arg.(value & opt(string, Process.cwd()) & info(["directory", "d"], ~doc));
};

let useDefaultOptions = {
  let doc = "Force generation of project files";
  Arg.(value & flag & info(["finit", "force-init"], ~doc));
};

let bootstrapOnly = {
  let doc = "Only create initial set of files. Dont run esy commands on them.";
  Arg.(value & flag & info(["b", "bootstrap-only"], ~doc));
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

let main =
  (. argv) =>
    switch (Term.eval_choice(~argv, cmd, [warmupCmd])) {
    | exception (Invalid_argument(msg)) =>
      Js.log(msg);
      Js.Promise.resolve(1);
    | `Ok(p) => p |> Js.Promise.then_(() => Js.Promise.resolve(0))
    | _ => Js.Promise.resolve(1)
    };
