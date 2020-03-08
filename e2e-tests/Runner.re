open Yojson.Basic;
open Unix;
open Utils;
open Bos;
open Printf;

/** TODO: Cleanup **/
let () = {
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
  let run = (cmd, args) =>
    if (run(cmd, args) != 0) {
      printf("%s failed\n", cmd);
    };
  let cwd = Sys.getcwd();
  printf("Running bootstrapper test");
  print_newline();
  chdir(Path.(cwd / "npm-cli"));
  printf("Installing pesy globally...");
  print_newline();
  run(makeCommand("yarn"), [||]);
  run(makeCommand("yarn"), [|"run", "package"|]);
  run(makeCommand("npm"), [|"pack"|]);
  run(makeCommand("npm"), [|"i", "-g", "./pesy-0.5.0-alpha.11.tgz"|]);
  chdir(cwd);
};

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

module Config: {
  type a;
  let ofString: string => result(a, [ | `Msg(string)]);
  let ofFile: Fpath.t => result(a, [ | `Msg(string)]);
  /** Basically a toString but results a string in a result **/
  let serialize:
    result(a, [ | `Msg(string)]) => result(string, [ | `Msg(string)]);
  let createError: string => result('a, [ | `Msg(string)]);
} = {
  type a =
    | FilePath(Fpath.t)
    | Contents(string);

  let ofString = x => Ok(Contents(x));
  let ofFile = x => Ok(FilePath(x));
  let createError = msg => Error(`Msg(msg));
  let serialize:
    result(a, [ | `Msg(string)]) => result(string, [ | `Msg(string)]) =
    fun
    | Ok(t) =>
      switch (t) {
      | FilePath(p) => OS.File.read(p)
      | Contents(s) => Ok(s)
      }
    | Error(`Msg(msg)) => createError(msg);
};

let mergeJsons = (j1, j2) => {
  switch (j1, j2) {
  | (`Assoc(kvs1), `Assoc(kvs2)) => Ok(`Assoc(kvs1 @ kvs2))
  | _ =>
    Error(
      `Msg(
        sprintf("Merging %s and %s failed", to_string(j1), to_string(j2)),
      ),
    )
  };
};

let addToManifest = (jsonString, manifest) => {
  OS.File.read(Fpath.v(manifest))
  >>= (
    manifestJsonString => {
      let manifestJson =
        try(Ok(from_string(manifestJsonString))) {
        | _ =>
          Error(
            `Msg(
              sprintf("Input manifest was not a valid json: %s", jsonString),
            ),
          )
        };
      let editJson =
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

      editJson
      >>= (edit => manifestJson >>= (manifest => mergeJsons(manifest, edit)))
      >>= (
        mergedJson =>
          mergedJson |> to_string |> OS.File.write(Fpath.v(manifest))
      );
    }
  );
};

module PesyConfig = {
  let edit = (config, manifest) => {
    Config.serialize(config)
    >>= (
      jsonString => {
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
                    `Assoc(kvs)
                    |> to_string
                    |> OS.File.write(Fpath.v(manifest))
                );
              | _ => Error(`Msg("Manifest file was invalid json"))
              }
            ) {
            | _ => Error(`Msg("Manifest file could not be parsed"))
            }
        );
      }
    );
  };

  let add = (jsonString, manifest) => {
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
                  pesyConfigJson
                  >>= (json => mergeJsons(v, json))
                  >>= (mergedConfig => Ok((k, mergedConfig)));
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
};

let checkProject = (msg, ()) =>
  L.withLog(Printf.sprintf("esy start (%s)", msg), () =>
    OS.Cmd.run_status(Cmd.(v(esyBinPath) % "start"))
    >>= failIfNotZeroExit("esy start: " ++ msg)
  )
  >>= (
    () =>
      L.withLog(Printf.sprintf("esy test (%s)", msg), () =>
        OS.Cmd.run_status(Cmd.(v(esyBinPath) % "test"))
        >>= failIfNotZeroExit("esy test: " ++ msg)
      )
  );

let checkPesyConfig = (msg, editPesyConfig, ()) =>
  L.withLog("Editing pesy config: " ++ msg, () => editPesyConfig())
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
  >>= (
    () =>
      addToManifest(
        Str.global_replace(
          Str.regexp("<RESOLUTION_LINK>"),
          "link:"
          ++ (
            Sys.getenv("PESY_CLONE_PATH")
            |> Str.(global_replace(regexp("\\"), "/"))
          ),
          {|
{
  "resolutions": {
    "@pesy/esy-pesy": "<RESOLUTION_LINK>"
  }
}
|},
        ),
        Path.(cwd / "package.json"),
      )
      >>= (
        () =>
          L.withLog(Printf.sprintf("esy"), () =>
            OS.Cmd.run_status(
              Cmd.(v(esyBinPath) % "--skip-repository-update"),
            )
            >>= failIfNotZeroExit("esy")
          )
      )
  )
  >>= checkPesyConfig("try old (4.x) pesy syntax", () =>
        PesyConfig.edit(
          Config.ofString(
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
          ),
          Path.(cwd / "package.json"),
        )
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
      )
      >>= (() => OS.File.read(Fpath.(v(cwd) / "library" / "Util.re")))
      >>= (
        utilRe =>
          OS.File.write(
            Fpath.(v(cwd) / "library" / "Util.re"),
            utilRe ++ "\n" ++ "external foo: int => unit = \"caml_foo\";",
          )
      )
  )
  >>= checkPesyConfig("add cNames for stubs", () =>
        PesyConfig.edit(
          Config.ofString(
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
          ),
          Path.(cwd / "package.json"),
        )
      )
  >>= checkPesyConfig("add byte mode compilation", () =>
        PesyConfig.edit(
          Config.ofString(
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
      ],
      "modes": [
        "byte"
      ]
    },
    "testExe": {
      "imports": [
        "Test = require('test-project/test')"
      ],
      "bin": {
        "RunTestProjectTests.exe": "RunTestProjectTests.re"
      },
      "modes": [
        "byte",
        "exe"
      ]
    },
    "library": {
      "require": [
        "console.lib",
        "pastel.lib"
      ],
      "modes": [
        "byte",
        "native"
      ],
      "cNames": ["stubs"]
    },
    "bin": {
      "modes": [
        "byte",
        "exe"
      ],
      "imports": [
        "Library = require('test-project/library')"
      ],
      "bin": {
        "TestProjectApp.exe": "TestProjectApp.re"
      }
    }
}
           |},
          ),
          Path.(cwd / "package.json"),
        )
      )
  >>= (
    _ =>
      L.withLog(
        "Editing source: Add file library-with-relative-imports/Index.re", () =>
        OS.Dir.create(
          ~path=true,
          Fpath.(v(cwd) / "library-with-relative-imports"),
        )
        >>= (
          _ =>
            OS.File.write(
              Fpath.(v(cwd) / "library-with-relative-imports" / "Index.re"),
              {|
open Foo
let foo = () => {
  print_endline("relatively imported" ++ Foo.Util.hello());
};
                                    |},
            )
        )
      )
      >>= (
        _ =>
          L.withLog(
            "Editing source: Add file executable-with-relative-imports/Main.re",
            () =>
            OS.Dir.create(
              ~path=true,
              Fpath.(v(cwd) / "executable-with-relative-imports"),
            )
            >>= (
              _ =>
                OS.File.write(
                  Fpath.(
                    v(cwd) / "executable-with-relative-imports" / "Main.re"
                  ),
                  {|
open Foo
Foo.foo();
                                    |},
                )
            )
          )
      )
  )
  >>= checkPesyConfig("add relatively imported modules", () =>
        PesyConfig.add(
          {|
                           {
                               "library-with-relative-imports": {
                                 "imports": [
                                   "Foo = require('../library')"
                                 ]
                               },
                               "executable-with-relative-imports": {
                                 "imports": [
                                   "Foo = require('../library-with-relative-imports')"
                                 ],
                                 "bin": { "TestRelativelyImportedLib.exe": "Main.re" }
                               }
                           }
                       |},
          Path.(cwd / "package.json"),
        )
      )
  >>= (
    () =>
      L.withLog(Printf.sprintf("esy add @opam/lwt @opam/lwt_ppx"), () =>
        OS.Cmd.run_status(
          Cmd.(v(esyBinPath) % "add" % "@opam/lwt" % "@opam/lwt_ppx"),
        )
        >>= failIfNotZeroExit("esy add @opam/lwt @opam/lwt_ppx")
      )
  )
  >>= (
    () =>
      L.withLog(Printf.sprintf("esy build"), () =>
        OS.Cmd.run_status(Cmd.(v(esyBinPath) % "build"))
        >>= failIfNotZeroExit("esy build")
      )
  )
  >>= (
    _ =>
      L.withLog(
        "Editing source: Add file executable-with-lwt-preprocess/Main.re", () =>
        OS.Dir.create(
          ~path=true,
          Fpath.(v(cwd) / "executable-with-lwt-preprocess"),
        )
        >>= (
          _ =>
            OS.File.write(
              Fpath.(v(cwd) / "executable-with-lwt-preprocess" / "Main.re"),
              {|
open Lwt;
let foo = {
  let%lwt foo = Lwt.return("hello world from lwt");
  Lwt.return(foo ++ " blah");
};
print_endline(Lwt_main.run(foo));
                                    |},
            )
        )
      )
  )
  >>= checkPesyConfig("add preprocessor", () =>
        PesyConfig.add(
          {|
                           {
                               "executable-with-lwt-preprocess": {
                                 "imports": [
                                   "Lwt = require('lwt')",
       "LwtUnix = require('lwt/unix')"
                                 ],
                                "preprocess": [
                                    "pps",
                                    "lwt_ppx"
                                ],
                                "bin": { "TestLwtPreprocessor.exe": "Main.re" }
                               }
                           }
                       |},
          Path.(cwd / "package.json"),
        )
      )
  >>= (
    _ =>
      L.withLog(
        "Editing source: Add file executable-with-raw-config/Main.re", () =>
        OS.Dir.create(
          ~path=true,
          Fpath.(v(cwd) / "executable-with-raw-config"),
        )
        >>= (
          _ =>
            OS.File.write(
              Fpath.(v(cwd) / "executable-with-raw-config" / "Main.re"),
              {|
print_endline(Unix.getenv("PATH"));
             |},
            )
        )
      )
  )
  >>= checkPesyConfig("add raw build config", () =>
        PesyConfig.add(
          {|
                           {
                               "executable-with-raw-config": {
                                "rawBuildConfig": [ "(libraries unix)" ],
                                "bin": { "TestRawBuildConfig.exe": "Main.re" }
                               }
                           }
                       |},
          Path.(cwd / "package.json"),
        )
      )
  >>= (
    _ =>
      L.withLog(
        "Editing source: Add file raw-footer-config-assets/plaintext.txt", () =>
        OS.Dir.create(
          ~path=true,
          Fpath.(v(cwd) / "raw-footer-config-assets"),
        )
        >>= (
          _ =>
            OS.File.write(
              Fpath.(v(cwd) / "raw-footer-config-assets" / "plaintext.txt"),
              {|
Some random text here.
             |},
            )
        )
      )
  )
  >>= checkPesyConfig("add raw build config footer", () =>
        PesyConfig.add(
          {|
                           {
                              "raw-footer-config-assets" : {
                                "namespace": "RawAssets",
                                "name": "test-project.assets",
                                "rawBuildConfigFooter": [ "(install (section share_root) (files (plaintext.txt as asset.txt)))" ]
                               }
                           }
                       |},
          Path.(cwd / "package.json"),
        )
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
