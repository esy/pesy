open Cmdliner;
[%raw "process.argv.shift()"];

open Bindings;
open Utils;

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

let downloadTemplate = template => {
  let spinner =
    Spinner.start("\x1b[2mDownloading template\x1b[0m " ++ template);
  download_git(
    template,
    projectPath,
    error => {
      Spinner.stop(spinner);
      switch (error) {
      | Some(e) => Js.log(e)
      | None =>
        let setup_files_spinner =
          Spinner.start("\x1b[2mSetting up template\x1b[0m " ++ template);

        try({
          let files =
            walk_sync(projectPath)
            ->Belt.Array.keep(file_or_dir => statSync(file_or_dir)->isFile);

          Belt.Array.forEach(
            files,
            file => {
              let () =
                readFile(file)->substituteTemplateValues |> write(file);

              renameSync(
                file,
                file |> substituteFileNames |> stripTemplateExtension,
              );
            },
          );
          Spinner.stop(setup_files_spinner);
        }) {
        | Js.Exn.Error(e) =>
          Spinner.stop(setup_files_spinner);
          switch (Js.Exn.stack(e)) {
          | Some(trace) =>
            Js.log({j|\x1b[31mPesy encountered an error\x1b[0m|j});
            Js.log(trace);
          | None =>
            Js.log(
              {j|\x1b[31mPesy encountered an unknown error as it was trying to setup templates\x1b[0m|j},
            )
          };
          exit(-1);
        };

        let id = Spinner.start("\x1b[2mRunning\x1b[0m esy install");
        exec("esy i", (e, stdout, stderr) => {
          Spinner.stop(id);
          if (Js.eqNullable(e, Js.Nullable.null)) {
            let id =
              Spinner.start(
                "\x1b[2mRunning\x1b[0m esy pesy\x1b[2m and \x1b[0m building project dependencies",
              );
            exec("esy pesy", (e, stdout, stderr) => {
              Spinner.stop(id);
              if (Js.eqNullable(e, Js.Nullable.null)) {
                let id = Spinner.start("\x1b[2mRunning\x1b[0m esy build");
                exec("esy build", (e, stdout, stderr) => {
                  Spinner.stop(id);
                  if (Js.eqNullable(e, Js.Nullable.null)) {
                    Printf.printf(
                      "You may now run \x1b[32m'esy test'\x1b[0m\n",
                    );
                  } else {
                    Printf.printf(
                      "'esy build' \x1b[31mfailed.\x1b[0m Could not build project.\nLogs can be found in pesy.stdout.log and pesy.stderr.log\n",
                    );
                    write("pesy.stdout.log", stdout);
                    write("pesy.stderr.log", stderr);
                  };
                });
              } else {
                Printf.printf(
                  "'esy pesy' \x1b[31mfailed.\x1b[0m Dune files could not be created.\n Logs can be found in pesy.stdout.log and pesy.stderr.log\n",
                );
                write("pesy.stdout.log", stdout);
                write("pesy.stderr.log", stderr);
              };
            });
          } else {
            Printf.printf(
              "'esy install' \x1b[31mfailed.\x1b[0m Dependencies could not be installed.\nLogs can be found in pesy.stdout.log and pesy.stderr.log\n",
            );
            write("pesy.stdout.log", stdout);
            write("pesy.stderr.log", stderr);
          };
        });
      };
    },
  );
};

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

let main = template =>
  if (isDirectoryEmpty(projectPath)) {
    downloadTemplate(template);
  } else {
    let rl =
      Readline.createInterface({
        "input": [%raw "process.stdin"],
        "output": [%raw "process.stdout"],
      });
    askYesNoQuestion(
      ~rl,
      ~question="Current directory is not empty. Are you sure to continue?",
      ~onYes=() => downloadTemplate(template),
      (),
    );
  };

let version = "0.5.0-alpha.9";

let template = {
  let doc = "Specify URL of the remote template. This can be of hthe form https://repo-url.git#<commit|branch|tag>. Eg: https://github.com/reason-native-web/morph-hello-world-pesy-template#6e5cbbb9f28";
  Arg.(
    value
    & opt(string, "github:esy/pesy-reason-template#86b37d16dcfe15")
    & info(["t", "template"], ~docv="TEMPLATE_URL", ~doc)
  );
};

let cmd = {
  open Cmdliner.Term;
  let cmd = "pesy";
  let envs: list(env_info) = [];
  let exits: list(exit_info) = [];
  let doc = "Your Esy Assistant.";
  let cmdT = Term.(const(main) $ template);
  (cmdT, Term.info(cmd, ~envs, ~exits, ~doc, ~version));
};

Term.eval(cmd);
