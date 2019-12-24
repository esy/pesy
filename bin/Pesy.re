module Lib = PesyEsyPesyLib.Lib;
module Utils = PesyEsyPesyUtils.Utils;
open Lib;
open Utils;
open Printf;
open EsyCommand;
open Cmdliner;
open PesyEsyPesyErrors.Errors;

exception BootstrappingError(string);
exception LsLibsError(string);

let reconcile = projectRoot => {
  /* use readFileOpt to read previously computed directory path */
  let manifestFile =
    switch (Sys.getenv_opt("ESY__ROOT_PACKAGE_CONFIG_PATH")) {
    | Some(x) => x
    | None => Path.(projectRoot / "package.json")
    };

  let operations = Lib.gen(projectRoot, manifestFile);

  switch (operations) {
  | [] => ()
  | _ as operations => Lib.log(operations)
  };

  print_endline(
    Pastel.(
      <Pastel> "Ready for " <Pastel color=Green> "esy" </Pastel> </Pastel>
    ),
  );
};

let main = () => {
  Findlib.init();
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
      print_endline("Prebuilt Pesy must not be run globally.")
    },
  );
};

let main = () =>
  try(main()) {
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
  | ImportsParserFailure(e) =>
    /* TODO: Be more specific about which imports */
    let message =
      Pastel.(
        <Pastel>
          <Pastel color=Red> "Could not understand the imports\n" </Pastel>
          <Pastel>
            "There seems to be a syntax error in one of the imports"
          </Pastel>
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
  };

/** DEPRECATED: Pesy is not supposed to be run in build env https://github.com/jchavarri/rebez/issues/4 **/
let pesy_build = () =>
  ignore(
    switch (Sys.getenv_opt("cur__root")) {
    | Some(curRoot) =>
      let buildTarget =
        try(build(curRoot)) {
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
  );

let pesyLsLibs = () => {
  let scopedIfDoubleKebabified = str =>
    switch (Str.split(Str.regexp("\\-\\-"), str)) {
    | [pkgName] => pkgName
    | [scope, pkgName] => "@" ++ scope ++ "/" ++ pkgName
    | _ =>
      raise(
        LsLibsError("findlib name of the library was had more than one --"),
      )
    };

  Findlib.init();
  let pkgs = Findlib.list_packages'();
  List.iteri(
    (i, x) => {
      x
      |> Str.global_replace(Str.regexp("\\."), "/")
      |> scopedIfDoubleKebabified
      |> sprintf(
           "\x1b[2m%s──\x1b[0m require('\x1b[1m%s\x1b[0m')",
           i < List.length(pkgs) - 1 ? "├" : "└",
         )
      |> print_endline
    },
    pkgs,
  );
};
let version = "0.1.0-alpha.4";

let cmd = () => {
  open Cmdliner.Term;
  let cmd = "esy-pesy";
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
  let doc = "build - builds your pesy managed project";
  let build_t = Term.(const(pesy_build) $ const());
  (build_t, Term.info(cmd, ~envs, ~exits, ~doc, ~version));
};

let lsLibs = () => {
  open Cmdliner.Term;
  let cmd = "ls-libs";
  let envs: list(env_info) = [];
  let exits: list(exit_info) = [];
  let doc = "ls-libs - lists installed packages";
  let build_t = Term.(const(pesyLsLibs) $ const());
  (build_t, Term.info(cmd, ~envs, ~exits, ~doc, ~version));
};

Term.exit @@ Term.eval_choice(cmd(), [build(), lsLibs()]);
