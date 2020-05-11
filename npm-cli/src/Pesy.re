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

let main = (projectPath, template, forceInit, forceCacheFetch, bootstrapOnly) => {
  let projectPath =
    Path.isAbsolute(projectPath)
      ? projectPath : Path.join([|Process.cwd(), projectPath|]);
  handleEmptyDirectory(. projectPath, forceInit)
  >>= (() => Cmd.make(~cmd="esy", ~env=Process.env))
  >>= (
    esy =>
      Bootstrapper.run(
        esy,
        projectPath,
        template,
        bootstrapOnly,
        forceCacheFetch,
      )
  )
  |> catch;
};

let warmup = () => {
  let projectPath = Process.cwd();
  Esy.make()
  >>= (
    esy =>
      Project.ofPath(esy, projectPath)
      >>= (project => Warmup.run(esy, project))
  )
  |> ResultPromise.catch;
};

let testTemplate = () => {
  let templatePath = Process.cwd();
  let tmpdir = Os.tmpdir();
  let testProjectPath = Path.join([|tmpdir, "test-project"|]);
  Rimraf.run(testProjectPath)
  >>= (
    () =>
      Fs.mkdir(~p=true, testProjectPath)
      |> Js.Promise.then_(() => Cmd.make(~cmd="esy", ~env=Process.env))
  )
  >>= (
    esy => {
      Js.log("");
      Js.log(
        Chalk.dim("Creating ")
        ++ Chalk.whiteBright("test-project")
        ++ Chalk.dim(" at ")
        ++ Chalk.whiteBright(testProjectPath),
      );
      Bootstrapper.run(
        esy,
        testProjectPath,
        Template.Kind.path(templatePath),
        false,
        false,
      );
    }
  )
  |> ResultPromise.catch;
};

let version = "0.5.0-dev";
let template = {
  let doc = "Specify URL of the remote template. This can be of the form https://repo-url.git#<commit|branch|tag>. Eg: https://github.com/reason-native-web/morph-hello-world-pesy-template#6e5cbbb9f28";
  Arg.(
    value
    & opt(
        Template.Kind.cmdlinerConv,
        Template.Kind.path(DefaultTemplate.path),
      )
    & info(["t", "template"], ~docv="TEMPLATE_URL", ~doc)
  );
};

let directory = {
  let doc = "Initialises a new project at given directory";
  Arg.(value & opt(string, Process.cwd()) & info(["directory", "d"], ~doc));
};

let forceInit = {
  let doc = "Force generation of project files";
  Arg.(value & flag & info(["finit", "force-init"], ~doc));
};

let forceCacheFetch = {
  let doc = "Force fetching of relocatable assets from CI";
  Arg.(value & flag & info(["fcache", "force-cache-fetch"], ~doc));
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
      const(main)
      $ directory
      $ template
      $ forceInit
      $ forceCacheFetch
      $ bootstrapOnly
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

let testTemplateCmd = {
  let cmd = "test-template";
  let envs: list(env_info) = [];
  let exits: list(exit_info) = [];
  let doc = "test-template - convenient subcommand to test your pesy template";
  let cmdT = Term.(const(testTemplate) $ const());
  (cmdT, Term.info(cmd, ~envs, ~exits, ~doc, ~version));
};

let main =
  (. argv) =>
    switch (Term.eval_choice(~argv, cmd, [warmupCmd, testTemplateCmd])) {
    | exception (Invalid_argument(msg)) =>
      Js.log(msg);
      Js.Promise.resolve(1);
    | `Ok(p) => p |> Js.Promise.then_(() => Js.Promise.resolve(0))
    | _ => Js.Promise.resolve(1)
    };
