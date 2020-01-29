open Unix;
open Utils;
open Printf;

let cwd = Sys.getcwd();

let run = (~env=?, c, args) => {
  let env_vars =
    switch (env) {
    | Some(v) => v
    | None => Unix.environment()
    };
  let pid =
    create_process_env(
      c,
      Array.append([|c|], args),
      env_vars,
      stdin,
      stdout,
      stderr,
    );
  switch (Unix.waitpid([], pid)) {
  | (_, WEXITED(c)) => c
  | (_, WSIGNALED(c)) => c
  | (_, WSTOPPED(c)) => c
  };
};

let testPesyConfigureExe =
  Path.(
    cwd / "_build" / "install" / "default" / "bin" / "TestPesyConfigure.exe"
  );

if (run(
      testPesyConfigureExe,
      [||],
      ~env=
        Array.append(
          [|
            sprintf(
              "PESY_CLONE_PATH=%s",
              Str.global_replace(Str.regexp("\\"), "/", Sys.getcwd()),
            ),
          |],
          Unix.environment(),
        ),
    )
    != 0) {
  exit(-1);
};

/* Printf.printf("Cleaning up ~/.esy before running bootstrapper\n"); */
/* rimraf(Path.(Sys.getenv(Sys.unix ? "HOME" : "HOMEPATH") / ".esy")); */

print_endline("Running bootstrapper test");
chdir(Path.(cwd / "npm-cli"));
print_endline("Installing pesy globally..");
run(makeCommand("npm"), [|"install"|]);
run(makeCommand("npm"), [|"run", "package"|]);
run(makeCommand("npm"), [|"pack"|]);
run(makeCommand("npm"), [|"i", "-g", "./pesy-0.5.0-alpha.11.tgz"|]);
chdir(cwd);

let testBootstrapperExe =
  Path.(
    cwd / "_build" / "install" / "default" / "bin" / "TestBootstrapper.exe"
  );

if (run(testBootstrapperExe, [||]) != 0) {
  exit(-1);
};
