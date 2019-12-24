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

let rec forEachDirEnt = (dir, f) => {
  Js.Promise.(
    Fs.Dir.read(dir)
    |> then_(dirEnt =>
         switch (Js.Nullable.toOption(dirEnt)) {
         | Some(dirEnt) => f(dirEnt) |> then_(() => forEachDirEnt(dir, f))
         | None => Fs.Dir.close(dir)
         }
       )
  );
};

let rec scanDir = (dir, f) => {
  Promise.(
    Fs.opendir(dir)
    |> then_(dir =>
         forEachDirEnt(
           dir,
           entry => {
             let entryPath = Path.join([|dir##path, entry##name|]);
             f(entryPath)
             |> then_(() =>
                  if (Fs.DirEnt.isDirectory(entry)) {
                    scanDir(entryPath, f);
                  } else {
                    resolve();
                  }
                );
           },
         )
       )
  );
};

let copyBundledTemplate = () => {
  let templatesDir =
    Path.resolve([|
      Path.dirname(scriptPath),
      "..",
      "lib",
      "node_modules",
      "pesy",
      "templates",
      "pesy-reason-template-0.1.0-alpha.1",
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
           );
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

let spinnerEnabledPromise = (spinnerMessage, promiseThunk) => {
  let spinner = Spinner.start(spinnerMessage);
  Promise.(
    promiseThunk()
    |> then_(x => {
         Spinner.stop(spinner);
         resolve(x);
       })
    |> catch(e => {
         Spinner.stop(spinner);
         throwJSError(e);
       })
  );
};

let setup = (template, projectPath) =>
  Promise.(
    bootstrap(projectPath, template)
    |> then_(() =>
         spinnerEnabledPromise("\x1b[2mSetting up\x1b[0m ", () =>
           substitute(projectPath)
         )
       )
    |> then_(_arrayOfCompletions =>
         spinnerEnabledPromise("\x1b[2mRunning\x1b[0m esy install", () =>
           ChildProcess.exec(
             "esy i",
             ChildProcess.Options.make(~cwd=projectPath, ()),
           )
         )
       )
    |> then_(_ /* (_stdout, _stderr) */ => {
         spinnerEnabledPromise(
           "\x1b[2mRunning\x1b[0m esy pesy\x1b[2m and \x1b[0m building project dependencies",
           () =>
           ChildProcess.exec(
             "esy pesy",
             ChildProcess.Options.make(~cwd=projectPath, ()),
           )
         )
       })
    |> then_(_ /* (_stdout, _stderr) */ => {
         spinnerEnabledPromise("\x1b[2mRunning\x1b[0m esy build", () =>
           ChildProcess.exec(
             "esy b",
             ChildProcess.Options.make(~cwd=projectPath, ()),
           )
         )
       })
    |> then_(_ /* (_stdout, _stderr) */ => {resolve()})
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
    let v = "0.5.0-alpha.10";
    let anonFun = Arg.Unit(() => print_endline(v));
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

main(Template.v^, DefaultOptions.v^);
