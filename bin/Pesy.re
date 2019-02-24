open PesyUtils;
open Printf;
open EsyCommand;

let bootstrap = projectRoot => {
  /* TODO: prompt user for their choice */
  let projectRoot = Sys.getcwd();

  /* use readFileOpt to read previously computed directory path */
  let (pkgName, versionString, packageNameUpperCamelCase, packageLibName) =
    PesyLib.bootstrapIfNecessary(projectRoot);

  let packageNameVersion = sprintf("%s@%s", pkgName, versionString);
  print_endline(Pastel.(<Pastel bold=true> packageNameVersion </Pastel>));

  PesyUtils.renderAscTree([
    [
      "executable",
      sprintf(
        "bin:    %sApp.re as %sApp.exe",
        packageNameUpperCamelCase,
        packageNameUpperCamelCase,
      ),
      sprintf(
        "require: [%s]",
        List.fold_left(
          (acc, e) => acc ++ " " ++ e,
          "",
          [sprintf("\"%s\"", packageLibName)],
        ),
      ),
    ],
    ["library", "require: []"],
    [
      "test",
      sprintf(
        "bin:    Test%s.re as Test%s.exe",
        packageNameUpperCamelCase,
        packageNameUpperCamelCase,
      ),
      sprintf(
        "require: [%s]",
        String.concat(", ", [sprintf("\"%s\"", packageLibName)]),
      ),
    ],
  ]);
  print_newline();

  ignore(PesyLib.generateBuildFiles(projectRoot));

  print_endline(
    Pastel.(<Pastel bold=true> "Running 'esy install'" </Pastel>),
  );

  let setupStatus = PesyUtils.runCommandWithEnv(esyCommand, [|"install"|]);
  if (setupStatus != 0) {
    fprintf(stderr, "esy (%s) install failed!", esyCommand);
    exit(-1);
  };

  print_endline(Pastel.(<Pastel bold=true> "Running 'esy build'" </Pastel>));
  let setupStatus = PesyUtils.runCommandWithEnv(esyCommand, [|"build"|]);
  if (setupStatus != 0) {
    fprintf(stderr, "esy (%s) build failed!", esyCommand);
    exit(-1);
  };
  print_endline(
    Pastel.(<Pastel color=Green> "You can now run 'esy test'" </Pastel>),
  );
};

let reconcile = projectRoot => {
  /* use readFileOpt to read previously computed directory path */
  let packageJSONPath = Path.(projectRoot / "package.json");
  let operations = PesyLib.PesyConf.gen(projectRoot, packageJSONPath);

  switch (operations) {
  | [] => ()
  | _ as operations => PesyLib.PesyConf.log(operations)
  };

  print_endline(
    Pastel.(
      <Pastel>
        "Ready for "
        <Pastel color=Green> "esy build" </Pastel>
      </Pastel>
    ),
  );
};

let main = () => {
  let userCommand =
    if (Array.length(Sys.argv) > 1) {
      Some(Sys.argv[1]);
    } else {
      None;
    };
  switch (Sys.getenv_opt("cur__root")) {
  | Some(curRoot) =>
    /**
     * This means the user ran pesy in an esy environment.
     * Either as
     * 1. esy pesy
     * 2. esy b pesy
     * 3. esy pesy build
     * 4. esy b pesy build
     */
    /* use readFileOpt to read previously computed directory path */
    PesyLib.Mode.EsyEnv.(
      switch (userCommand) {
      | Some(c) =>
        switch (c) {
        | "build" => print_endline("TODO: build")
        | _ as userCommand =>
          let message =
            Pastel.(
              <Pastel>
                <Pastel color=Red> "Error" </Pastel>
                "Unrecognised command: "
                <Pastel> userCommand </Pastel>
              </Pastel>
            );
          fprintf(stderr, "%s", message);
        }
      | None => reconcile(curRoot)
      }
    )

  | None =>
    /**
     * This mean pesy is being run naked on the shell.
     * Either it was:
     */
    bootstrap(Sys.getcwd())
  };
  ();
};

main();
