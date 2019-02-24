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

PesyLib.PesyConf.(
  try (main()) {
  | BinaryTupleNameError(e) =>
    let l =
      switch (e) {
      | InvalidSourceFilename(sourceFile) =>
        Pastel.(
          <Pastel>
            <Pastel color=Red> "Invalid source file " </Pastel>
            <Pastel bold=true> sourceFile </Pastel>
            "\nSource file must be a .re file\nMake sure bin property its of the form"
            <Pastel bold=true> " FooBar.re as BarBaz.exe" </Pastel>
          </Pastel>
        )

      | InvalidBinaryName(binaryFile) =>
        Pastel.(
          <Pastel>
            <Pastel color=Red> "Invalid binary file " </Pastel>
            <Pastel bold=true> binaryFile </Pastel>
            "\nBinary file must be a .exe file\nMake sure bin property is of the form"
            <Pastel bold=true> " FooBar.re as BarBaz.exe" </Pastel>
          </Pastel>
        )
      };
    let message = Pastel.(<Pastel> l </Pastel>);
    fprintf(stderr, "%s\n", message);
    exit(-1);
  | ShouldNotBeNull(e) =>
    let message =
      Pastel.(
        <Pastel>
          <Pastel color=Red> "Found null value for " </Pastel>
          <Pastel bold=true> e </Pastel>
          "\nExpected a non null value."
        </Pastel>
      );
    fprintf(stderr, "%s\n", message);
    exit(-1);
  | FatalError(e) =>
    let message =
      Pastel.(
        <Pastel> <Pastel color=Red> "Fatal Error " </Pastel> e </Pastel>
      );
    fprintf(stderr, "%s\n", message);
    exit(-1);
  | InvalidRootName(e) =>
    let message =
      Pastel.(
        <Pastel>
          <Pastel color=Red> "Invalid root name!\n" </Pastel>
          <Pastel> "Expected package name of the form " </Pastel>
          <Pastel bold=true> "@myscope/foo-bar" </Pastel>
          <Pastel> " or " </Pastel>
          <Pastel bold=true> "foo-bar\n" </Pastel>
          <Pastel> "Instead found " </Pastel>
          <Pastel bold=true> e </Pastel>
        </Pastel>
      );
    fprintf(stderr, "%s\n", message);
    exit(-1);
  | GenericException(e) =>
    let message =
      Pastel.(
        <Pastel>
          <Pastel color=Red> "Error: " </Pastel>
          <Pastel> e </Pastel>
        </Pastel>
      );
    fprintf(stderr, "%s\n", message);
    exit(-1);
  | ResolveRelativePathFailure(e) =>
    let message =
      Pastel.(
        <Pastel>
          <Pastel color=Red> "Could not find the library\n" </Pastel>
          <Pastel> e </Pastel>
        </Pastel>
      );
    fprintf(stderr, "%s\n", message);
    exit(-1);
  | x =>
    /* let message = Pastel.(<Pastel color=Red> "Failed" </Pastel>); */
    /* fprintf(stderr, "%s", message); */
    /* exit(-1); */
    raise(x)
  }
);
