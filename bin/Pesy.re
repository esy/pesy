open Lib;
open Utils;
open Printf;
open EsyCommand;
open Cmdliner;

exception BootstrappingError(string);

let reconcile = projectRoot => {
  /* use readFileOpt to read previously computed directory path */
  let packageJSONPath = Path.(projectRoot / "package.json");
  let operations = Lib.PesyConf.gen(projectRoot, packageJSONPath);

  switch (operations) {
  | [] => ()
  | _ as operations => Lib.PesyConf.log(operations)
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
  ignore(
    switch (Sys.getenv_opt("cur__root")) {
    | Some(curRoot) =>
      /**
     * This means the user ran pesy in an esy environment.
     * as esy pesy
     * TODO: when run as esy b pesy, it must exit early with an error
     */
      reconcile(curRoot)
    | None =>
      /**
     * This mean pesy is being run naked on the shell.
     * TODO: use readFileOpt to read previously computed directory path
     */
      print_endline("Pesy installed in the sandbox cannot be run globally.")
    },
  );
};

let main = () => {
  PesyConf.(
    try (main()) {
    | BootstrappingError(message) =>
      let message =
        Pastel.(
          <Pastel>
            <Pastel> "You have " </Pastel>
            <Pastel color=Red> "not installed" </Pastel>
            <Pastel> " esy\n" </Pastel>
            <Pastel>
              "pesy works together with esy to simplify your workflow. Please install esy.\n"
            </Pastel>
            <Pastel> "You could try\n\n" </Pastel>
            <Pastel bold=true> "    npm install -g esy\n" </Pastel>
          </Pastel>
        );
      fprintf(stderr, "%s\n", message);
      exit(-1);
    | InvalidBinProperty(pkgName) =>
      let mStr =
        sprintf("Invalid value in subpackage %s's bin property\n", pkgName);
      let message =
        Pastel.(
          <Pastel>
            <Pastel color=Red> mStr </Pastel>
            <Pastel>
              "'bin' property is usually of the form { \"target.exe\": \"sourceFilename.re\" } "
            </Pastel>
          </Pastel>
        );
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
};

let pesy_build = () =>
  PesyConf.(
    ignore(
      switch (Sys.getenv_opt("cur__root")) {
      | Some(curRoot) =>
        let buildTarget =
          try (build(curRoot)) {
          | BuildValidationFailures(failures) =>
            let errorMessages =
              String.concat(
                "\n",
                failures
                |> List.map(
                     fun
                     | StaleDuneFile(f) => {
                         let duneFile =
                           Str.global_replace(
                             Str.regexp(curRoot ++ (Sys.unix ? "/" : "\\")),
                             "",
                             f,
                           );
                         sprintf(
                           "      Dune file %s is stale",
                           Pastel.(<Pastel bold=true> duneFile </Pastel>),
                         );
                       }
                     | StaleOpamFile((o, n)) =>
                       sprintf(
                         "      Project target name has changed. Found %s instead of %s",
                         Pastel.(<Pastel bold=true> o </Pastel>),
                         Pastel.(<Pastel bold=true> n </Pastel>),
                       ),
                   ),
              );

            fprintf(
              stderr,
              "%s",
              Pastel.(
                <Pastel color=Red>
                  "\n  Found the following issues: \n\n"
                </Pastel>
              ),
            );
            fprintf(stderr, "%s\n", errorMessages);
            fprintf(
              stderr,
              "\n  These can be fixed by running %s \n\n",
              Pastel.(<Pastel color=Green> "esy pesy" </Pastel>),
            );
            exit(-1);
          };

        Sys.command("refmterr dune build -p " ++ buildTarget);
      | None =>
        let message =
          Pastel.(
            <Pastel>
              <Pastel color=Red>
                "'pesy build' must be run the build environment only\n"
              </Pastel>
              <Pastel> "Try esy b pesy build" </Pastel>
            </Pastel>
          );
        fprintf(stderr, "%s\n", message);
        exit(-1);
      },
    )
  );

let version = "0.5.0-alpha.3";

let cmd = () => {
  open Cmdliner.Term;
  let cmd = "pesy";
  let envs: list(env_info) = [];
  let exits: list(exit_info) = [];
  let doc = "Esy Pesy - Your Esy Assistant.";
  let cmd_t = Term.(const(main) $ const());
  (cmd_t, Term.info(cmd, ~envs, ~exits, ~doc, ~version));
};

let build = () => {
  open Cmdliner.Term;
  let cmd = "build";
  let envs: list(env_info) = [];
  let exits: list(exit_info) = [];
  let doc = "pesy-build - builds your pesy managed project";
  let build_t = Term.(const(pesy_build) $ const());
  (build_t, Term.info(cmd, ~envs, ~exits, ~doc, ~version));
};

Term.exit @@ Term.eval_choice(cmd(), [build()]);
