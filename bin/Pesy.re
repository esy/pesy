module Lib = PesyEsyPesyLib.Lib;
module Utils = PesyEsyPesyUtils.Utils;
open Lib;
open Utils;
open Printf;
open Cmdliner;
open PesyEsyPesyErrors.Errors;

exception BootstrappingError(unit);
exception LsLibsError(string);

module EnvVars = {
  let rootPackageConfigPath = Sys.getenv_opt("ESY__ROOT_PACKAGE_CONFIG_PATH");
};

let getManifestFile = projectRoot => {
  switch (EnvVars.rootPackageConfigPath) {
  | Some(x) => x
  | None => Path.(projectRoot / "package.json")
  };
};

let reconcile = projectRoot => {
  /* use readFileOpt to read previously computed directory path */

  let operations = projectRoot |> getManifestFile |> Lib.gen(projectRoot);

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
  | BootstrappingError () =>
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
  | ImportsParserFailure(_e) =>
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
  | LocalLibraryPathNotFound(e) =>
    let message =
      <Pastel>
        <Pastel>
          <Pastel color=Red> "Error: " </Pastel>
          "Could not find the library "
        </Pastel>
        <Pastel bold=true> e </Pastel>
        <Pastel>
          "\ncheck the require() in import of your package.json"
        </Pastel>
      </Pastel>;

    fprintf(stderr, "%s\n", message);
    exit(-1);
  | x =>
    /* let message = Pastel.(<Pastel color=Red> "Failed" </Pastel>); */
    /* fprintf(stderr, "%s", message); */
    /* exit(-1); */
    raise(x)
  };

let pesy_dune_file = dir => {
  switch (Sys.getenv_opt("cur__root")) {
  | None =>
    let message =
      Pastel.(
        <Pastel>
          <Pastel color=Red>
            "'pesy dune-file' must be run the build environment only\n"
          </Pastel>
        </Pastel>
      );
    fprintf(stderr, "%s\n", message);
    exit(-1);
  | Some(curRoot) =>
    try(duneFile(curRoot, getManifestFile(curRoot), dir)) {
    | LocalLibraryPathNotFound(e) =>
      let message =
        <Pastel>
          <Pastel>
            <Pastel color=Red> "Error: " </Pastel>
            "Could not find the library "
          </Pastel>
          <Pastel bold=true> e </Pastel>
          <Pastel>
            "\ncheck the require() in import of your package.json"
          </Pastel>
        </Pastel>;

      fprintf(stderr, "%s\n", message);
      exit(-1);
    }
  };
};

let pesy_build = () =>
  ignore(
    switch (Sys.getenv_opt("cur__root")) {
    | Some(curRoot) =>
      let manifestFile = curRoot |> getManifestFile;

      try(validateManifestFile(curRoot, manifestFile)) {
      | LocalLibraryPathNotFound(e) =>
        let message =
          <Pastel>
            <Pastel>
              <Pastel color=Red> "Error: " </Pastel>
              "Could not find the library "
            </Pastel>
            <Pastel bold=true> e </Pastel>
            <Pastel>
              "\ncheck the require() in import of your package.json"
            </Pastel>
          </Pastel>;

        fprintf(stderr, "%s\n", message);
        exit(-1);
      | ForeignStubsIncorrectlyUsed =>
        let message =
          Pastel.(
            <Pastel>
              <Pastel color=Red> "Error: " </Pastel>
              <Pastel>
                <Pastel bold=true> "foreignStubs " </Pastel>
                "is introduced since dune version"
                <Pastel bold=true> " 2.0\n" </Pastel>
              </Pastel>
              <Pastel>
                "Use "
                <Pastel bold=true> "cNames" </Pastel>
                " to specify stubs"
              </Pastel>
            </Pastel>
          );
        fprintf(stderr, "%s\n", message);
        exit(-1);
      | CNamesIncorrectlyUsed =>
        let message =
          Pastel.(
            <Pastel>
              <Pastel color=Red> "Error: " </Pastel>
              <Pastel>
                <Pastel bold=true> "cNames " </Pastel>
                "is deprecated in dune version 2.x\n"
              </Pastel>
              <Pastel>
                "Use "
                <Pastel bold=true> "foreignStubs" </Pastel>
                " to specify stubs"
              </Pastel>
            </Pastel>
          );
        fprintf(stderr, "%s\n", message);
        exit(-1);
      };

      let buildTarget = build(manifestFile);
      Unix.(
        switch (
          create_process(
            "dune",
            [|"dune", "build", "-p", buildTarget|],
            Unix.stdin,
            Unix.stdout,
            Unix.stderr,
          )
          |> waitpid([])
        ) {
        | (_, WEXITED(c)) => c
        | (_, WSIGNALED(c)) => c
        | (_, WSTOPPED(c)) => c
        }
      );

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
let version = "0.1.0-alpha.14";


let build = () => {
  let cmd = "build";
  let envs: list(Cmd.Env.info) = [];
  let exits: list(Cmd.Exit.info) = [];
  let doc = "build - builds your pesy managed project";
  let build_t = Term.(const(pesy_build) $ const());
  let info = Cmd.info(cmd, ~envs, ~exits, ~doc, ~version);
  Cmd.v(info, build_t);
};

let subpkgTerm =
  Cmdliner.Arg.(
    required
    & pos(0, some(string), None)
    & info(
        [],
        ~doc="Subpackage which needs the Dune file generated",
        ~docv="SUBPACKAGE",
      )
  );

let pesy_dune_file = () => {
  let cmd = "dune-file";
  let envs: list(Cmd.Env.info) = [];
  let exits: list(Cmd.Exit.info) = [];
  let doc = "dune-file - prints dune file for a given dir/subpackage";
  let build_t = Term.(const(pesy_dune_file) $ subpkgTerm);
  let info = Cmd.info(cmd, ~envs, ~exits, ~doc, ~version);
  Cmd.v(info, build_t);
};

let pesy_eject = dir => {
  switch (Sys.getenv_opt("cur__root")) {
  | None =>
    let message =
      Pastel.(
        <Pastel>
          <Pastel color=Red>
            "'pesy eject' must be run the build environment only\n"
          </Pastel>
        </Pastel>
      );
    fprintf(stderr, "%s\n", message);
    exit(-1);
  | Some(curRoot) => duneEject(curRoot, getManifestFile(curRoot), dir)
  };
};

let eject = () => {
  let subpkgTerm =
    Arg.(
      required
      & pos(0, some(string), None)
      & info(
          [],
          ~doc="Subpackage which needs to get ejected",
          ~docv="SUBPACKAGE",
        )
    );

  let cmd = "eject";
  let envs: list(Cmd.Env.info) = [];
  let exits: list(Cmd.Exit.info) = [];
  let doc = "eject - eject dune file for a given dir/subpackage";
  let build_t = Term.(const(pesy_eject) $ subpkgTerm);
  let info = Cmd.info(cmd, ~envs, ~exits, ~doc, ~version);
  Cmd.v(info, build_t); 
};

let lsLibs = () => {
  let cmd = "ls-libs";
  let envs: list(Cmd.Env.info) = [];
  let exits: list(Cmd.Exit.info) = [];
  let doc = "ls-libs - lists installed packages";
  let build_t = Term.(const(pesyLsLibs) $ const());
  let info = Cmd.info(cmd, ~envs, ~exits, ~doc, ~version);
  Cmd.v(info, build_t);
};

  let cmd = "esy-pesy";
  let envs: list(Cmd.Env.info) = [];
  let exits: list(Cmd.Exit.info) = [];
  let doc = "Esy Pesy - Your Esy Assistant.";
  let cmd_t = Term.(const(main) $ const());
let cmd_info = Cmd.info(cmd, ~envs, ~exits, ~doc, ~version);

exit @@ Cmd.eval @@ Cmd.group(~default=cmd_t, cmd_info, [build(), pesy_dune_file(), lsLibs(), eject()]);
