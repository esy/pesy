open Unix;
open Utils;
open Printf;
open Bos;
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

/* let testPesyConfigureExe = */
/*   Path.( */
/*     cwd / "_build" / "install" / "default" / "bin" / "TestPesyConfigure.exe" */
/*   ); */

/* if (run( */
/*       testPesyConfigureExe, */
/*       [||], */
/*       ~env= */
/*         Array.append( */
/*           [| */
/*             sprintf( */
/*               "PESY_CLONE_PATH=%s", */
/*               Str.global_replace(Str.regexp("\\"), "/", Sys.getcwd()), */
/*             ), */
/*           |], */
/*           Unix.environment(), */
/*         ), */
/*     ) */
/*     != 0) { */
/*   exit(-1); */
/* }; */

printf("Running bootstrapper test");
print_newline();
chdir(Path.(cwd / "npm-cli"));
printf("Installing pesy globally...");
print_newline();
run(makeCommand("npm"), [|"install"|]);
run(makeCommand("npm"), [|"run", "package"|]);
run(makeCommand("npm"), [|"pack"|]);
run(makeCommand("npm"), [|"i", "-g", "./pesy-0.5.0-alpha.11.tgz"|]);
chdir(cwd);

/* let testBootstrapperExe = */
/*   Path.( */
/*     cwd / "_build" / "install" / "default" / "bin" / "TestBootstrapper.exe" */
/*   ); */

/* if (run(testBootstrapperExe, [||]) != 0) { */
/*   exit(-1); */
/* }; */

open Rresult.R.Infix;
module Log = {
  open Printf;
  let log = OS.File.(writef(dash));
  let warn = printf;
  let error = printf;
  let withLog = (name, f) => {
    log("Running '%s'", name)
    >>= (
      () => {
        print_newline();
        f();
      }
    )
    >>= (() => log("Finished '%s'", name))
    >>= (
      () => {
        print_newline();
        Ok();
      }
    );
  };
};

module L = Log;

let withCurrentWorkingDirectory =
    (path, f: string => result(unit, [> Rresult.R.msg] as 'a)) => {
  let fpath = Fpath.v(path);
  OS.Dir.current()
  >>= (
    cwd => {
      L.log("Entering %s\n", path)
      >>= (
        () =>
          L.withLog(
            Printf.sprintf("rimraf(%s)", path),
            () => {
              rimraf(path);
              Ok();
            },
          )
      )
      >>= (() => OS.Dir.create(~path=true, fpath))
      >>= (_ => OS.Dir.set_current(fpath))
      >>= (() => f(path))
      >>= (() => L.log("Leaving %s\n", path))
      >>= (_ => OS.Dir.set_current(cwd));
    }
  );
};

module Path = {
  let (/) = Filename.concat;
};
let createTestWorkspace = workspaceFolderName => {
  let tmpDir = Filename.get_temp_dir_name();
  Path.(tmpDir / workspaceFolderName);
};

let pesyBinPath = makeCommand("pesy");
let esyBinPath = makeCommand("esy");

let failIfNotZeroExit = taskName =>
  fun
  | `Exited(code) =>
    if (code == 0) {
      Ok();
    } else {
      Error(`Msg(sprintf("'%s' failed: non exit (%d)", taskName, code)));
    }
  | `Signaled(code) =>
    Error(`Msg(sprintf("'%s' failed: signaled (%d)", taskName, code)));

open Yojson.Basic;
let editPesyConfig = (jsonString, manifest) => {
  OS.File.read(Fpath.v(manifest))
  >>= (
    manifestJsonString =>
      try(
        switch (from_string(manifestJsonString)) {
        | `Assoc(jsonKVPairs) =>
          let pesyConfigJson =
            try(Ok(from_string(jsonString))) {
            | _ =>
              Error(
                `Msg(
                  sprintf(
                    "Input pesy config was not a valid json: %s",
                    jsonString,
                  ),
                ),
              )
            };
          List.map(
            ((k, v)) =>
              if (k == "buildDirs") {
                pesyConfigJson >>= (json => Ok((k, json)));
              } else {
                Ok((k, v));
              },
            jsonKVPairs,
          )
          |> List.fold_left(
               (acc, v) =>
                 switch (acc, v) {
                 | (Ok(l), Ok(e)) => Ok([e, ...l])
                 | (_, Error(_) as e) => e
                 | (Error(_) as e, _) => e
                 },
               Ok([]),
             )
          >>= (
            kvs =>
              `Assoc(kvs) |> to_string |> OS.File.write(Fpath.v(manifest))
          );
        | _ => Error(`Msg("Manifest file was invalid json"))
        }
      ) {
      | _ => Error(`Msg("Manifest file could not be parsed"))
      }
  );
};

let checkProject = (msg, ()) =>
  L.withLog(Printf.sprintf("esy start"), () =>
    OS.Cmd.run_status(Cmd.(v(esyBinPath) % "start"))
    >>= failIfNotZeroExit("esy start: " ++ msg)
  )
  >>= (
    () =>
      L.withLog(Printf.sprintf("esy test"), () =>
        OS.Cmd.run_status(Cmd.(v(esyBinPath) % "test"))
        >>= failIfNotZeroExit("esy test: " ++ msg)
      )
  );

let checkPesyConfig = (cwd, msg, config, ()) =>
  L.withLog("Editing pesy config: " ++ msg, () =>
    editPesyConfig(config, Path.(cwd / "package.json"))
  )
  >>= (
    () =>
      L.withLog("esy pesy: " ++ msg, () =>
        OS.Cmd.run_status(Cmd.(v(esyBinPath) % "pesy"))
        >>= failIfNotZeroExit("esy pesy")
      )
  )
  >>= (
    () =>
      L.withLog(Printf.sprintf("esy build"), () =>
        OS.Cmd.run_status(Cmd.(v(esyBinPath) % "build"))
        >>= failIfNotZeroExit("esy build")
      )
  )
  >>= checkProject(msg);

let checkBootstrapper = cwd => {
  OS.Cmd.(run_status(Cmd.v(pesyBinPath)))
  >>= failIfNotZeroExit("pesy")
  >>= checkProject("checking if bootstrapper works")
  >>= checkPesyConfig(
        cwd,
        "try old (4.x) pesy syntax",
        {|
{
    "test": {
      "imports": [
        "Library = require('test-project/library')",
        "Rely = require('rely/lib')"
      ],
      "flags": [
        "-linkall",
        "-g",
        "-w",
        "-9"
      ]
    },
    "testExe": {
      "imports": [
        "Test = require('test-project/test')"
      ],
      "bin": {
        "RunTestProjectTests.exe": "RunTestProjectTests.re"
      }
    },
    "library": {
      "require": [
        "console.lib",
        "pastel.lib"
      ]
    },
    "bin": {
      "imports": [
        "Library = require('test-project/library')"
      ],
      "bin": {
        "TestProjectApp.exe": "TestProjectApp.re"
      }
    }
}
       |},
      )
  >>= (
    () =>
      L.withLog("Editing source: Add file stubs.c", () =>
        OS.File.write(
          Fpath.(v(cwd) / "library" / "stubs.c"),
          {|
#include <caml/mlvalues.h>
#include <stdio.h>

CAMLprim value
caml_foo(value a) {
    int c_a = Int_val(a);
    printf("foo received: %d", c_a);
    return Val_unit;
}
         |},
        )
        >>= (
          () =>
            OS.File.read(Fpath.(v(cwd) / "library" / "Util.re"))
            >>= (
              utilRe =>
                OS.File.write(
                  Fpath.(v(cwd) / "library" / "Util.re"),
                  utilRe ++ "\n" ++ "external foo: int => unit = \"caml_foo\";",
                )
            )
        )
      )
  )
  >>= checkPesyConfig(
        cwd,
        "add cNames for stubs",
        {|
{
    "test": {
      "imports": [
        "Library = require('test-project/library')",
        "Rely = require('rely/lib')"
      ],
      "flags": [
        "-linkall",
        "-g",
        "-w",
        "-9"
      ]
    },
    "testExe": {
      "imports": [
        "Test = require('test-project/test')"
      ],
      "bin": {
        "RunTestProjectTests.exe": "RunTestProjectTests.re"
      }
    },
    "library": {
      "require": [
        "console.lib",
        "pastel.lib"
      ],
      "cNames": ["stubs"]
    },
    "bin": {
      "imports": [
        "Library = require('test-project/library')"
      ],
      "bin": {
        "TestProjectApp.exe": "TestProjectApp.re"
      }
    }
}
           |},
      );
};

switch (
  withCurrentWorkingDirectory(
    createTestWorkspace("test-project"),
    checkBootstrapper,
  )
) {
| Ok () => ()
| Error(`Msg(msg)) => print_endline("checkBootstrapper failed with: " ++ msg)
};
