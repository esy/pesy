Bindings.sourceMapSupportInstall();

open Cmdliner;
open Utils;
open Bindings;
open Js;

[%raw "process.argv.shift()"];

let packageNameKebab = projectPath => kebab(basename(projectPath));
let packageNameKebabSansScope = projectPath =>
  projectPath |> packageNameKebab |> removeScope;
let packageNameUpperCamelCase = projectPath =>
  projectPath |> packageNameKebabSansScope |> upperCamelCasify;
let templateVersion = "0.0.0";
let packageLibName = projectPath =>
  packageNameKebabSansScope(projectPath) ++ "/library";
let packageTestName = projectPath =>
  packageNameKebabSansScope(projectPath) ++ "/test";

let substituteTemplateValues = (projectPath, s) =>
  s
  |> Js.String.replaceByRe(
       [%bs.re "/<PACKAGE_NAME_FULL>/g"],
       packageNameKebab(projectPath),
     )
  |> Js.String.replaceByRe([%bs.re "/<VERSION>/g"], templateVersion)
  |> Js.String.replaceByRe(
       [%bs.re "/<PUBLIC_LIB_NAME>/g"],
       packageLibName(projectPath),
     )
  |> Js.String.replaceByRe(
       [%bs.re "/<TEST_LIB_NAME>/g"],
       packageTestName(projectPath),
     )
  |> Js.String.replaceByRe(
       [%bs.re "/<PACKAGE_NAME>/g"],
       packageNameKebab(projectPath),
     )
  |> Js.String.replaceByRe(
       [%bs.re "/<PACKAGE_NAME_UPPER_CAMEL>/g"],
       packageNameUpperCamelCase(projectPath),
     );
let stripTemplateExtension = s => {
  s |> Js.String.replace("-template", "");
};

let substituteFileNames = (projectPath, s) =>
  s
  |> Js.String.replaceByRe(
       [%bs.re "/__PACKAGE_NAME_FULL__/g"],
       packageNameKebab(projectPath),
     )
  |> Js.String.replaceByRe(
       [%bs.re "/__PACKAGE_NAME__/g"],
       packageNameKebab(projectPath),
     )
  |> Js.String.replaceByRe(
       [%bs.re "/__PACKAGE_NAME_UPPER_CAMEL__/g"],
       packageNameUpperCamelCase(projectPath),
     );

let isDirectoryEmpty = path =>
  !Node.Fs.existsSync(path)
  || Belt.Array.length(Node.Fs.readdirSync(path)) == 0;

let rec askYesNoQuestion =
        (~rl: Readline.rlType, ~question, ~onYes, ~onNo=() => (), ()) => {
  rl##question(
    question ++ " [y/n] ",
    input => {
      let response = input->Js.String.trim->Js.String.toLowerCase;
      switch (response) {
      | "y"
      | "yes" =>
        onYes();
        rl##close();
      | "n"
      | "no" =>
        onNo();
        rl##close();
      | _ => askYesNoQuestion(~rl, ~question, ~onYes, ~onNo, ())
      };
    },
  );
};

let forEachDirEnt = (dir, f) => {
  Js.Promise.(
    Fs.readdir(dir)
    |> then_(files => Promise.all(Array.map(file => f(file), files)))
    |> then_(_ => resolve(Result.Ok()))
  );
};

let rec scanDir = (dir, f) => {
  Promise.(
    forEachDirEnt(
      dir,
      entry => {
        let entryPath = Path.join([|dir, entry|]);
        f(entryPath)
        |> then_(_ =>
             Fs.isDirectory(entryPath)
             |> then_(isDir =>
                  if (isDir) {
                    scanDir(entryPath, f);
                  } else {
                    resolve(Result.Ok());
                  }
                )
           );
      },
    )
  );
};

let copyBundledTemplate = projectPath => {
  let templatesDir =
    Path.resolve([|
      dirname,
      "templates",
      "pesy-reason-template-0.1.0-alpha.3",
    |]);
  Process.chdir(projectPath);
  Promise.(
    scanDir(
      templatesDir,
      src => {
        let dest = Js.String.replace(templatesDir, projectPath, src);
        Fs.isDirectory(src)
        |> then_(isDir =>
             if (isDir) {
               Fs.mkdir(~dryRun=false, ~p=true, dest);
             } else {
               Fs.copy(~src, ~dest, ~dryRun=false, ());
             }
           )
        |> then_(_ => resolve(Result.Ok()));
      },
    )
  );
};

/* Belt seems unnecessary */
let bootstrap = projectPath =>
  fun
  | Some(template) => downloadGit(template, projectPath)
  | None => copyBundledTemplate(projectPath);

let subst = (projectPath, file) => {
  Promise.(
    Fs.(
      readFile(. file)
      |> then_(b =>
           Buffer.toString(b)
           |> substituteTemplateValues(projectPath)
           |> Buffer.from
           |> resolve
         )
      |> then_(s => writeFile(. file, s))
      |> then_(() =>
           renameSync(
             file,
             file
             |> substituteFileNames(projectPath)
             |> stripTemplateExtension,
           )
           |> resolve
         )
    )
  );
};

let substitute = projectPath => {
  walk_sync(projectPath)
  |> Array.filter(file_or_dir => statSync(file_or_dir)->isFile)
  |> Array.map(subst(projectPath))
  |> Promise.all;
};

let spinnerEnabledPromise = (cmd, args, projectPath, message) => {
  Js.log("");
  Js.log(message);
  Js.Promise.make((~resolve, ~reject as _) => {
    let process =
      ChildProcess.spawn(
        cmd,
        args,
        ChildProcess.Options.make(~cwd=projectPath, ~stdio="inherit", ()),
      );
    ChildProcess.onClose(process, () => resolve(. "dummy"));
  });
};

let esyCommand = Sys.unix ? "esy" : "esy.cmd";
let setup = (template, projectPath) =>
  Promise.(
    Fs.mkdir(~p=true, projectPath)
    |> then_(() => bootstrap(projectPath, template))
    |> then_(_ => {
         Js.log("");
         Js.log("Setting up");
         substitute(projectPath)
         |> then_(_arrayOfCompletions => {
              Js.log("");
              Utils.Path.(
                [|
                  "azure-pipelines.yml",
                  "library" / "Util.re",
                  "test" / "TestFile.re",
                  "test" / "TestFramework.re",
                  "README.md",
                  "bin" / (packageNameUpperCamelCase(projectPath) ++ "App.re"),
                  "dune-project",
                  packageNameKebab(projectPath) ++ ".opam",
                  "package.json",
                |]
                |> Js.Array.forEach(file =>
                     ("    created " |> Chalk.green)
                     ++ (file |> Chalk.whiteBright)
                     |> Js.log
                   )
              );
              resolve();
            });
       })
    |> then_(() => {
         spinnerEnabledPromise(
           esyCommand,
           [|"i"|],
           projectPath,
           ("Running" |> Chalk.dim) ++ (" esy install" |> Chalk.whiteBright),
         )
       })
    |> then_(_ => Warmup.run(projectPath))
    |> then_(_ /* (_stdout, _stderr) */ => {
         spinnerEnabledPromise(
           esyCommand,
           [|"pesy"|],
           projectPath,
           ("Running" |> Chalk.dim)
           ++ (" esy pesy" |> Chalk.whiteBright)
           ++ (" and " |> Chalk.dim)
           ++ ("building project dependencies" |> Chalk.whiteBright),
         )
       })
    |> then_(_ /* (_stdout, _stderr) */ => {
         spinnerEnabledPromise(
           esyCommand,
           [|"b"|],
           projectPath,
           ("Running" |> Chalk.dim) ++ (" esy build" |> Chalk.whiteBright),
         )
       })
    |> then_(_ /* (_stdout, _stderr) */ => {
         "Ready for development!" |> Chalk.green |> Js.log;
         resolve();
       })
    |> catch(handlePromiseRejectInJS)
  );

let main' = (projectPath, template, useDefaultOptions) => {
  let projectPath =
    Path.isAbsolute(projectPath)
      ? projectPath : Path.join([|Process.cwd(), projectPath|]);
  if (isDirectoryEmpty(projectPath) || useDefaultOptions) {
    ignore @@ setup(template, projectPath);
  } else {
    let rl =
      Readline.createInterface({
        "input": [%raw "process.stdin"],
        "output": [%raw "process.stdout"],
      });
    askYesNoQuestion(
      ~rl,
      ~question="Current directory is not empty. Are you sure to continue?",
      ~onYes=() => ignore @@ setup(template, projectPath),
      (),
    );
  };
};

let warmup = () => {
  let projectPath = Process.cwd();
  Warmup.run(projectPath)
  |> Js.Promise.then_(
       fun
       | Ok () => Js.Promise.resolve()
       | Error(msg) => Js.log2("Error", msg) |> Js.Promise.resolve,
     )
  |> ignore;
};

let main = (projectPath, template, useDefaultOptions) =>
  try(main'(projectPath, template, useDefaultOptions)) {
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
  };

let version = "0.5.0-alpha.14";
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

open Cmdliner.Term;
let cmd = {
  let cmd = "pesy";
  let envs: list(env_info) = [];
  let exits: list(exit_info) = [];
  let doc = "Your Esy Assistant.";
  let cmdT = Term.(const(main) $ directory $ template $ useDefaultOptions);
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
