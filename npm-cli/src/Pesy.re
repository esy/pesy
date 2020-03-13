Bindings.sourceMapSupportInstall();

open Utils;
open Bindings;
open Js;

let projectPath = cwd();
let packageNameKebab = kebab(basename(projectPath));
let packageNameKebabSansScope = removeScope(packageNameKebab);
let packageNameUpperCamelCase = upperCamelCasify(packageNameKebabSansScope);
let templateVersion = "0.0.0";
let packageLibName = packageNameKebabSansScope ++ "/library";
let packageTestName = packageNameKebabSansScope ++ "/test";

let substituteTemplateValues = s =>
  s
  |> Js.String.replaceByRe(
       [%bs.re "/<PACKAGE_NAME_FULL>/g"],
       packageNameKebab,
     )
  |> Js.String.replaceByRe([%bs.re "/<VERSION>/g"], templateVersion)
  |> Js.String.replaceByRe([%bs.re "/<PUBLIC_LIB_NAME>/g"], packageLibName)
  |> Js.String.replaceByRe([%bs.re "/<TEST_LIB_NAME>/g"], packageTestName)
  |> Js.String.replaceByRe([%bs.re "/<PACKAGE_NAME>/g"], packageNameKebab)
  |> Js.String.replaceByRe(
       [%bs.re "/<PACKAGE_NAME_UPPER_CAMEL>/g"],
       packageNameUpperCamelCase,
     );
let stripTemplateExtension = s => {
  s |> Js.String.replace("-template", "");
};

let substituteFileNames = s =>
  s
  |> Js.String.replaceByRe(
       [%bs.re "/__PACKAGE_NAME_FULL__/g"],
       packageNameKebab,
     )
  |> Js.String.replaceByRe([%bs.re "/__PACKAGE_NAME__/g"], packageNameKebab)
  |> Js.String.replaceByRe(
       [%bs.re "/__PACKAGE_NAME_UPPER_CAMEL__/g"],
       packageNameUpperCamelCase,
     );

let isDirectoryEmpty = path =>
  Belt.Array.length(Node.Fs.readdirSync(path)) == 0;

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

let copyBundledTemplate = () => {
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
        let dest = Js.String.replace(templatesDir, Process.cwd(), src);
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
  | None => copyBundledTemplate();

let subst = file => {
  Promise.(
    Fs.(
      readFile(. file)
      |> then_(b =>
           Buffer.toString(b)
           |> substituteTemplateValues
           |> Buffer.from
           |> resolve
         )
      |> then_(s => writeFile(. file, s))
      |> then_(() =>
           renameSync(
             file,
             file |> substituteFileNames |> stripTemplateExtension,
           )
           |> resolve
         )
    )
  );
};

let substitute = projectPath => {
  walk_sync(projectPath)
  |> Array.filter(file_or_dir => statSync(file_or_dir)->isFile)
  |> Array.map(subst)
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

let esyCommand = Sys.unix ? "esy": "esy.cmd";
let setup = (template, projectPath) =>
  Promise.(
    bootstrap(projectPath, template)
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
                  "bin" / (packageNameUpperCamelCase ++ "App.re"),
                  "dune-project",
                  packageNameKebab ++ ".opam",
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

let main' = (template, useDefaultOptions) =>
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

let main = (template, useDefaultOptions) =>
  try(main'(template, useDefaultOptions)) {
  | Js.Exn.Error(e) =>
    switch (Js.Exn.message(e)) {
    | Some(message) => Js.log({j|Error: $message|j})
    | None => Js.log("An unknown error occurred")
    }
  };

module CliOptions = {
  let bootstrap = ref(true);

  /* We presume that bootstrapping is the most likely action.
     Other info retrieving commands with '--help' and '--version'
     are less likely and turn the flag off */

  module Template = {
    let short = "-t";
    let long = "--template";
    let doc = "Specify URL of the remote template. This can be of the form https://repo-url.git#<commit|branch|tag>. Eg: https://github.com/reason-native-web/morph-hello-world-pesy-template#6e5cbbb9f28";
    let v = ref(None);
    let anonFun = Arg.String(template => v := Some(template));
  };
  module DefaultOptions = {
    let short = "-y";
    let long = "--yes";
    let doc = "Use default options";
    let v = ref(false);
    let anonFun = Arg.Set(v);
  };
  module Version = {
    let short = "-v";
    let long = "--version";
    let doc = "Prints version and exits";
    let v = "0.5.0-alpha.11";
    let anonFun =
      Arg.Unit(
        () => {
          bootstrap := false;
          print_endline(v);
        },
      );
  };
};

open CliOptions;
let specList = [
  Template.(long, anonFun, doc),
  Template.(short, anonFun, doc),
  DefaultOptions.(long, anonFun, doc),
  DefaultOptions.(short, anonFun, doc),
  Version.(short, anonFun, doc),
  Version.(long, anonFun, doc),
];

let usageMsg = "$ pesy [-t|--template <url>] [-y]";

Arg.parse(specList, _ => (), usageMsg);

if (bootstrap^) {
  main(Template.v^, DefaultOptions.v^);
};
