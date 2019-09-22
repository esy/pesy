open Bindings;
open Utils;

let projectPath = cwd();
let packageNameKebab = kebab(Filename.basename(projectPath));
let packageNameKebabSansScope = removeScope(packageNameKebab);
let packageNameUpperCamelCase = upperCamelCasify(packageNameKebabSansScope);
let version = "0.0.0";
let packageLibName = packageNameKebabSansScope ++ "/library";
let packageTestName = packageNameKebabSansScope ++ "/test";

let substituteTemplateValues = s =>
  s
  |> Js.String.replaceByRe(
       [%bs.re "/<PACKAGE_NAME_FULL>/g"],
       packageNameKebab,
     )
  |> Js.String.replaceByRe([%bs.re "/<VERSION>/g"], version)
  |> Js.String.replaceByRe([%bs.re "/<PUBLIC_LIB_NAME>/g"], packageLibName)
  |> Js.String.replaceByRe([%bs.re "/<TEST_LIB_NAME>/g"], packageTestName)
  |> Js.String.replaceByRe([%bs.re "/<PACKAGE_NAME>/g"], packageNameKebab)
  |> Js.String.replaceByRe(
       [%bs.re "/<PACKAGE_NAME_UPPER_CAMEL>/g"],
       packageNameUpperCamelCase,
     );
let stripTemplateExtension = s => {
  s
  |> Js.String.replace(
       "-template",
       "",
     )
}

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

// TODO: Move the template to the esy or esy-ocaml org
let template =
  argv
  ->Belt.Array.keep(Js.String.includes("--template"))
  ->Belt.Array.get(0)
  ->Belt.Option.map(Js.String.replace("--template=", ""))
  ->Belt.Option.getWithDefault("github:esy/pesy-reason-template");

let download_spinner =
  Spinner.start("\x1b[2mDownloading template\x1b[0m " ++ template);
download_git(
  template,
  projectPath,
  error => {
    Spinner.stop(download_spinner);
    switch (error) {
    | Some(e) => Js.log(e)
    | None =>
      let setup_files_spinner =
        Spinner.start("\x1b[2mSetting up template\x1b[0m " ++ template);
      let files =
        walk_sync(projectPath)
        ->Belt.Array.keep(file_or_dir => statSync(file_or_dir)->isFile);

      Belt.Array.forEach(
        files,
        file => {
          let () = readFile(file)->substituteTemplateValues |> write(file);

          renameSync(file, file |> substituteFileNames |> stripTemplateExtension);
        },
      );
      Spinner.stop(setup_files_spinner);

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
