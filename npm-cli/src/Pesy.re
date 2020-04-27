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

let substituteFileNames = (projectPath, s) => {
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
};

let isDirectoryEmpty = path =>
  !Node.Fs.existsSync(path)
  || Belt.Array.length(Node.Fs.readdirSync(path)) == 0;

let rec askYesNoQuestion = (~question, ~onYes, ~onNo, ()) => {
  let rl =
    Readline.createInterface({
      "input": [%raw "process.stdin"],
      "output": [%raw "process.stdout"],
    });
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
      | _ => askYesNoQuestion(~question, ~onYes, ~onNo, ())
      };
    },
  );
};

let forEachDirEnt = (dir, f) => {
  Js.Promise.(
    Fs.readdir(dir)
    |> then_(files => files |> Array.map(f) |> Promise.all)
    |> then_(_ => resolve(Ok()))
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
                    resolve(Ok());
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
        |> then_(_ => resolve(Ok()));
      },
    )
  );
};

/* Belt seems unnecessary */
let bootstrap = projectPath =>
  fun
  | Some(template) =>
    downloadGit(template, projectPath)
    |> Js.Promise.then_(_ => {
         // TODO: Hacky! Streamline what stays and what doesn't. Maybe add a ignore field to pesy config so that we know what to copy and what shouldn't be?
         Rimraf.run(
           Path.join([|projectPath, ".ci-self"|]),
         )
       })
    |> Js.Promise.then_(
         fun
         | Error(msg) => Error(msg) |> Js.Promise.resolve
         | Ok(_) => {
             Js.Promise.resolve(Ok());
           },
       )

  | None => copyBundledTemplate(projectPath);

let subst = (projectPath, file) => {
  let substitutedName =
    file |> substituteFileNames(projectPath) |> stripTemplateExtension;
  Promise.(
    Fs.(
      readFile(. file)
      |> then_(b =>
           Buffer.toString(b)
           |> substituteTemplateValues(projectPath)
           |> Buffer.from
           |> resolve
         )
      |> then_(s => writeFile(. substitutedName, s))
      |> then_(_ => {
           if (file != substitutedName) {
             Node.Fs.unlinkSync(file);
           };
           resolve();
         })
    )
  );
};

let substitute = projectPath => {
  walk_sync(projectPath)
  |> Array.map(file_or_dir => Path.join([|projectPath, file_or_dir|]))
  |> Array.filter(file_or_dir => {statSync(file_or_dir)->isFile})
  |> Array.map(subst(projectPath))
  |> Promise.all;
};

let runCommand = (cmd, args, projectPath, message) => {
  Js.log("");
  Js.log(message);
  Cmd.spawn(~args, ~cwd=projectPath, ~cmd);
};

let setup = (esy, template, projectPath, bootstrapOnly) =>
  ResultPromise.(
    {
      let bootstrapped =
        Fs.mkdir(~p=true, projectPath)
        |> Js.Promise.then_(() => {
             Process.chdir(projectPath);
             bootstrap(projectPath, template);
           })
        >>= (
          _ => {
            Js.log("");
            Js.log("Setting up");
            substitute(projectPath)
            |> Js.Promise.then_(_arrayOfCompletions => {
                 Js.log("");
                 [|
                   "azure-pipelines.yml",
                   Path.join([|"library", "Util.re"|]),
                   Path.join([|"test", "TestFile.re"|]),
                   Path.join([|"test", "TestFramework.re"|]),
                   "README.md",
                   Path.join([|
                     "bin",
                     packageNameUpperCamelCase(projectPath) ++ "App.re",
                   |]),
                   "dune-project",
                   packageNameKebab(projectPath) ++ ".opam",
                   "package.json",
                 |]
                 |> Js.Array.forEach(file =>
                      ("    created " |> Chalk.green)
                      ++ (file |> Chalk.whiteBright)
                      |> Js.log
                    );
                 Js.Promise.resolve(Ok());
               });
          }
        );

      let runningEsy = () =>
        runCommand(
          esy,
          [|"i"|],
          projectPath,
          ("Running" |> Chalk.dim) ++ (" esy install" |> Chalk.whiteBright),
        )
        >>= (
          () => {
            Js.log(
              ("Running" |> Chalk.dim) ++ (" pesy warm" |> Chalk.whiteBright),
            );
            Warmup.run(projectPath)
            |> Js.Promise.then_(warmupResult => {
                 switch (warmupResult) {
                 | Ok () => ()
                 | Error(msg) => Js.log2("Skipping warmup because: ", msg)
                 };
                 ResultPromise.ok();
               });
          }
        )
        >>= (
          () /* (_stdout, _stderr) */ => {
            runCommand(
              esy,
              [|"pesy"|],
              projectPath,
              ("Running" |> Chalk.dim)
              ++ (" esy pesy" |> Chalk.whiteBright)
              ++ (" and " |> Chalk.dim)
              ++ ("building project dependencies" |> Chalk.whiteBright),
            );
          }
        )
        >>= (
          () /* (_stdout, _stderr) */ => {
            runCommand(
              esy,
              [|"b"|],
              projectPath,
              ("Running" |> Chalk.dim) ++ (" esy build" |> Chalk.whiteBright),
            );
          }
        )
        >>| (
          () /* (_stdout, _stderr) */ => {
            "Ready for development!" |> Chalk.green |> Js.log;
          }
        );

      bootstrapOnly
        ? bootstrapped
          >>| (
            () => {
              Js.log("");
              "You may have to run " ++ Chalk.green("esy") |> Js.log;
            }
          )
        : bootstrapped >>= runningEsy;
    }
  );

let promptEmptyDirectory = () =>
  Js.Promise.make((~resolve, ~reject as _) => {
    askYesNoQuestion(
      ~question="Current directory is not empty. Are you sure to continue?",
      ~onYes=() => resolve(. Utils.Result.return()),
      ~onNo=
        () =>
          resolve(. Utils.Result.fail("User responded no to the prompt")),
      (),
    )
  });
let handleEmptyDirectory =
  (. path, force) =>
    if (isDirectoryEmpty(path) || force) {
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
