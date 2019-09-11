open Printf;
open Utils;
open Utils.NoLwt;

/*****
    localName ~~> dunePublicName
    pesy_modules_namespace ~~> namespace
    modules ~~> aliases
 ***/

type pkgType =
  | ExecutablePackage(Executable.t)
  | LibraryPackage(Library.t)
  | TestPackage(Test.t);

type package = {
  pkg_path: string,
  common: Common.t,
  pkgType,
};

type validationError =
  | StaleDuneFile(string)
  | StaleOpamFile((string, string));

/* private */
exception ShouldHaveRaised(unit);

/* public */
exception ShouldNotBeNull(string);
exception FatalError(string);
exception ShouldNotBeHere(unit);
exception InvalidRootName(string);
exception GenericException(string);
exception ResolveRelativePathFailure(string);
exception InvalidBinProperty(string);
exception BuildValidationFailures(list(validationError));
exception ImportsParserFailure(unit);

/* */
/*  Inline ppx is supported too. */
/* let%test "getSuffix(): must return suffix" = getSuffix("foo.lib") == "lib"; */
/* */

let findIndex = (s1, s2) => {
  let re = Str.regexp_string(s2);
  try(Str.search_forward(re, s1, 0)) {
  | Not_found => (-1)
  };
};

let resolveRelativePath = path => {
  let separator = "/";
  let revParts = List.rev(Str.split(Str.regexp(separator), path));

  let rec resolve = (parts, skipCount, acc) => {
    switch (parts) {
    | ["..", ...r] => resolve(r, skipCount + 1, acc)
    | [".", ...r] => resolve(r, skipCount, acc)
    | [h, ...r] =>
      resolve(
        r,
        skipCount > 0 ? skipCount - 1 : 0,
        skipCount == 0 ? [h, ...acc] : acc,
      )
    | [] =>
      if (skipCount == 0) {
        acc;
      } else {
        raise(
          ResolveRelativePathFailure(
            sprintf("Failed resolving: %s Too many `../`", path),
          ),
        );
      }
    };
  };

  String.concat(separator, resolve(revParts, 0, []));
};

/* let%expect_test _ = { */
/*   print_endline(resolveRelativePath("foo/lib")); */
/*   %expect */
/*   {| */
     /*      foo/lib */
     /*    |}; */
/* }; */

/* let%expect_test _ = { */
/*   print_endline(resolveRelativePath("foo/bar/..")); */
/*   %expect */
/*   {| */
     /*      foo */
     /*    |}; */
/* }; */

/* let%expect_test _ = { */
/*   print_endline(resolveRelativePath("foo/bar/../..")); */
/*   %expect */
/*   {| */

     /*    |}; */
/* }; */

/* let%expect_test _ = { */
/*   let result = */
/*     try (resolveRelativePath("foo/bar/../../..")) { */
/*     | ResolveRelativePathFailure(x) => */
/*       sprintf("ResolveRelativePathFailure(\"%s\")", x) */
/*     | e => raise(e) */
/*     }; */
/*   print_endline(result); */
/*   %expect */
/*   {| */
     /*           ResolveRelativePathFailure("Failed resolving: foo/bar/../../.. Too many `../`") */
     /*      |}; */
/* }; */

/* let%expect_test _ = { */
/*   print_endline(resolveRelativePath("foo/bar/../baz/")); */
/*   %expect */
/*   {| */
     /*      foo/baz */
     /*    |}; */
/* }; */

let moduleNameOf = fileName =>
  Str.global_replace(Str.regexp("\\.\\(re\\|ml\\)$"), "", fileName);

/* let%expect_test _ = { */
/*   print_endline(moduleNameOf("Foo.re")); */
/*   %expect */
/*   {| */
     /*      Foo */
     /*    |}; */
/* }; */

/* let%expect_test _ = { */
/*   print_endline(moduleNameOf("Foo.ml")); */
/*   %expect */
/*   {| */
     /*      Foo */
     /*    |}; */
/* }; */

let isValidBinaryFileName = fileName =>
  Str.string_match(Str.regexp("^.+\\.exe$"), fileName, 0);

/* let%expect_test _ = { */
/*   print_endline(string_of_bool(isValidBinaryFileName("Foo.re"))); */
/*   %expect */
/*   {| */
     /*      false */
     /*    |}; */
/* }; */

/* let%expect_test _ = { */
/*   print_endline(string_of_bool(isValidBinaryFileName("Foo.exe"))); */
/*   %expect */
/*   {| */
     /*      true */
     /*    |}; */
/* }; */

let isValidSourceFile = fileName =>
  Str.string_match(Str.regexp("^.+\\.\\(re\\|ml\\)$"), fileName, 0);

/* let%expect_test _ = { */
/*   print_endline(string_of_bool(isValidSourceFile("Foo.re"))); */
/*   %expect */
/*   {| */
     /*      true */
     /*    |}; */
/* }; */

/* let%expect_test _ = { */
/*   print_endline(string_of_bool(isValidSourceFile("Foo.ml"))); */
/*   %expect */
/*   {| */
     /*      true */
     /*    |}; */
/* }; */

/* let%expect_test _ = { */
/*   print_endline(string_of_bool(isValidSourceFile("Foo.mlsss"))); */
/*   %expect */
/*   {| */
     /*      false */
     /*    |}; */
/* }; */

/* Turns "Foo.re as Foo.exe" => ("Foo.re", "Foo.exe") */

/* Turns "foo/bar/baz" => "foo.bar.baz" */
let pathToOCamlLibName = p => Str.global_replace(Str.regexp("/"), ".", p);

let isBinPropertyPresent =
  fun
  | Some(_) => true
  | _ => false;

let isValidScopeName = n => {
  n.[0] == '@';
};

/* let%expect_test _ = { */
/*   print_endline(string_of_bool(isValidScopeName("blah"))); */
/*   %expect */
/*   {| */
     /*      false */
     /*    |}; */
/* }; */

/* let%expect_test _ = { */
/*   print_endline(string_of_bool(isValidScopeName("@myscope"))); */
/*   %expect */
/*   {| */
     /*      true */
     /*    |}; */
/* }; */

let stripAtTheRate = s => String.sub(s, 1, String.length(s) - 1);

let doubleKebabifyIfScoped = n => {
  switch (Str.split(Str.regexp("/"), n)) {
  | [pkgName] => pkgName
  | [scope, pkgName, ...rest] =>
    isValidScopeName(scope)
      ? String.concat(
          "/",
          [stripAtTheRate(scope) ++ "--" ++ pkgName, ...rest],
        )
      : raise(InvalidRootName(n))
  | _ => raise(InvalidRootName(n))
  };
};

/* let%expect_test _ = { */
/*   print_endline(doubleKebabifyIfScoped("foo-bar")); */
/*   %expect */
/*   {| */
     /*      foo-bar */
     /*    |}; */
/* }; */

type t = (string, list(package));
let toPesyConf = (projectPath: string, json: JSON.t): t => {
  let pkgs = JSON.toKeyValuePairs(JSON.member(json, "buildDirs"));

  let rootName =
    /* "name" in root package.json */
    try(
      doubleKebabifyIfScoped(
        JSON.member(json, "name") |> JSON.toValue |> FieldTypes.toString,
      )
    ) {
    | JSON.NullJSONValue () => raise(ShouldNotBeNull("name"))
    | x => raise(x)
    };
  /* doubleKebabifyIfScoped turns @myscope/pkgName => myscope--pkgName */

  (
    rootName,
    List.map(
      pkg => {
        let (dir, conf) = pkg;

        let binJSON = JSON.member(conf, "bin");
        let bin =
          try(
            Some(
              {
                let kvPairs = JSON.toKeyValuePairs(binJSON);
                /* TODO: Multiple executable support */
                let kv = List.hd(kvPairs);
                let (k, v) = kv;
                (
                  /* Value is the source .re file */
                  v |> JSON.toValue |> FieldTypes.toString,
                  /* Key is the target executable name */
                  k,
                );
              },
            )
          ) {
          | JSON.NullJSONValue(_) => None
          /* If its a string and not a JSON */
          | JSON.InvalidJSONValue(_) =>
            let binaryMainFile =
              try(binJSON |> JSON.toValue |> FieldTypes.toString) {
              | _ => raise(InvalidBinProperty(dir))
              };
            if (!isValidSourceFile(binaryMainFile)) {
              raise(InvalidBinProperty(dir));
            };
            Some((binaryMainFile, moduleNameOf(binaryMainFile) ++ ".exe"));
          | e => raise(e)
          };

        /* Pesy'name is Dune's public_name */
        /* If name is provided and binary's public_name is also provided in the bin property, name takes precedence */
        let name =
          try(
            JSON.member(conf, "name") |> JSON.toValue |> FieldTypes.toString
          ) {
          | JSON.NullJSONValue(_) =>
            switch (bin) {
            | Some((_mainFileName, installedBinaryName)) => installedBinaryName
            | None => rootName ++ "." ++ pathToOCamlLibName(dir)
            }
          | e => raise(e)
          };

        let (<|>) = (f, g, x) => g(f(x));
        /* "my-package/lib/here" => "my-package.lib.here" */
        let require =
          try(
            JSON.member(conf, "require")
            |> JSON.toValue
            |> FieldTypes.toList
            |> List.map(
                 FieldTypes.toString
                 <|> (
                   x =>
                     x.[0] == '.' ? sprintf("%s/%s/%s", rootName, dir, x) : x
                 )
                 <|> (x => x.[0] == '@' ? doubleKebabifyIfScoped(x) : x)
                 <|> resolveRelativePath
                 <|> pathToOCamlLibName,
               )
          ) {
          | JSON.NullJSONValue(_) => []
          | e => raise(e)
          };

        let imports =
          try(
            JSON.member(conf, "imports")
            |> JSON.toValue
            |> FieldTypes.toList
            |> List.map(FieldTypes.toString <|> ImportsParser.parse)
          ) {
          | JSON.NullJSONValue(_) => []
          | e => raise(ImportsParserFailure())
          };

        let pesyModuleNamespace =
          [rootName]
          @ String.split_on_char('/', dir)
          @ ["PesyModules"]
          |> List.map(upperCamelCasify)
          |> List.fold_left((++), "");

        let pesyModules =
          PesyModule.create(
            ~namespace=pesyModuleNamespace,
            ~dunePublicName=
              sprintf(
                "%s.pesy-modules",
                pathToOCamlLibName(rootName ++ "/" ++ dir),
              ),
            ~aliases=
              imports
              |> List.map(import => {
                   let (importedNamespace, lib) = import;
                   let libraryAsPath =
                     lib
                     |> (
                       x => {
                         x.[0] == '.'
                           ? sprintf("%s/%s/%s", rootName, dir, x) : x;
                       }
                     )
                     |> resolveRelativePath
                     |> (x => x.[0] == '@' ? doubleKebabifyIfScoped(x) : x);

                   let exportedNamespace =
                     if (findIndex(libraryAsPath, rootName) != (-1)) {
                       libraryAsPath
                       |> String.split_on_char('/')
                       |> List.map(upperCamelCasify)
                       |> List.fold_left((++), "");
                     } else {
                       /** ie. external library. We use findlib and figure out namespace **/
                       Str.global_replace(
                         Str.regexp("\\.cmxa"),
                         "",
                         Findlib.package_property(
                           ["native"],
                           pathToOCamlLibName(libraryAsPath),
                           "archive",
                         ),
                       )
                       |> String.mapi((i, c) =>
                            i == 0 ? Char.uppercase_ascii(c) : c
                          );
                     };

                   PesyModule.Alias.create(
                     ~alias=
                       sprintf(
                         "module %s = %s;",
                         importedNamespace,
                         exportedNamespace,
                       ),
                     ~library=pathToOCamlLibName(libraryAsPath),
                     ~originalNamespace=importedNamespace,
                   );
                 }),
          );
        let flags =
          try(
            Some(
              JSON.member(conf, "flags")
              |> JSON.toValue
              |> FieldTypes.toList
              |> List.map(FieldTypes.toString),
            )
          ) {
          | _ => None
          };

        let ocamlcFlags =
          try(
            Some(
              JSON.member(conf, "ocamlcFlags")
              |> JSON.toValue
              |> FieldTypes.toList
              |> List.map(FieldTypes.toString),
            )
          ) {
          | _ => None
          };

        let ocamloptFlags =
          try(
            Some(
              JSON.member(conf, "ocamloptFlags")
              |> JSON.toValue
              |> FieldTypes.toList
              |> List.map(FieldTypes.toString),
            )
          ) {
          | _ => None
          };

        let jsooFlags =
          try(
            Some(
              JSON.member(conf, "jsooFlags")
              |> JSON.toValue
              |> FieldTypes.toList
              |> List.map(FieldTypes.toString),
            )
          ) {
          | _ => None
          };

        let preprocess =
          try(
            Some(
              JSON.member(conf, "preprocess")
              |> JSON.toValue
              |> FieldTypes.toList
              |> List.map(FieldTypes.toString),
            )
          ) {
          | _ => None
          };

        let includeSubdirs =
          try(
            Some(
              JSON.member(conf, "includeSubdirs")
              |> JSON.toValue
              |> FieldTypes.toString,
            )
          ) {
          | JSON.NullJSONValue(_) => None
          | e => raise(e)
          };

        let rawBuildConfig =
          try(
            Some(
              JSON.member(conf, "rawBuildConfig")
              |> JSON.toValue
              |> FieldTypes.toList
              |> List.map(FieldTypes.toString),
            )
          ) {
          | _ => None
          };

        let rawBuildConfigFooter =
          try(
            Some(
              JSON.member(conf, "rawBuildConfigFooter")
              |> JSON.toValue
              |> FieldTypes.toList
              |> List.map(FieldTypes.toString),
            )
          ) {
          | _ => None
          };

        let pkg_path = Path.(projectPath / dir);
        let common =
          Common.create(
            name,
            Path.(projectPath / dir),
            require,
            flags,
            ocamlcFlags,
            ocamloptFlags,
            jsooFlags,
            preprocess,
            includeSubdirs,
            rawBuildConfig,
            rawBuildConfigFooter,
            pesyModules,
          );

        if (isBinPropertyPresent(bin)) {
          /* Prioritising `bin` over `name` */
          let main =
            switch (bin) {
            | Some((mainFileName, _installedBinaryName)) =>
              moduleNameOf(mainFileName)

            | _ =>
              try(
                JSON.member(conf, "main")
                |> JSON.toValue
                |> FieldTypes.toString
              ) {
              | JSON.NullJSONValue () =>
                raise(
                  FatalError(
                    sprintf(
                      "Atleast one of `bin` or `main` must be provided for %s",
                      dir,
                    ),
                  ),
                )
              | e => raise(e)
              }
            };

          let modes =
            try(
              Some(
                Executable.Mode.ofList(
                  JSON.member(conf, "modes")
                  |> JSON.toValue
                  |> FieldTypes.toList
                  |> List.map(a => a |> FieldTypes.toString),
                ),
              )
            ) {
            | JSON.NullJSONValue () => None
            | e => raise(e)
            };
          {
            pkg_path,
            common,
            pkgType: ExecutablePackage(Executable.create(main, modes)),
          };
        } else {
          let namespace =
            try(
              JSON.member(conf, "namespace")
              |> JSON.toValue
              |> FieldTypes.toString
            ) {
            | JSON.NullJSONValue () =>
              sprintf("%s/%s", rootName, dir)
              |> String.split_on_char('/')
              |> List.map(upperCamelCasify)
              |> List.fold_left((++), "")
            | e => raise(e)
            };
          let libraryModes =
            try(
              Some(
                JSON.member(conf, "modes")
                |> JSON.toValue
                |> FieldTypes.toList
                |> List.map(FieldTypes.toString)
                |> List.map(Library.Mode.ofString),
              )
            ) {
            | JSON.NullJSONValue () => None
            | e => raise(e)
            };
          let cStubs =
            try(
              Some(
                JSON.member(conf, "cNames")
                |> JSON.toValue
                |> FieldTypes.toList
                |> List.map(FieldTypes.toString),
              )
            ) {
            | JSON.NullJSONValue () => None
            | e => raise(e)
            };
          let virtualModules =
            try(
              Some(
                JSON.member(conf, "virtualModules")
                |> JSON.toValue
                |> FieldTypes.toList
                |> List.map(FieldTypes.toString),
              )
            ) {
            | JSON.NullJSONValue () => None
            | e => raise(e)
            };
          let implements =
            try(
              Some(
                JSON.member(conf, "implements")
                |> JSON.toValue
                |> FieldTypes.toList
                |> List.map(
                     FieldTypes.toString
                     <|> (
                       x =>
                         x.[0] == '.'
                           ? sprintf("%s/%s/%s", rootName, dir, x) : x
                     )
                     <|> (x => x.[0] == '@' ? doubleKebabifyIfScoped(x) : x)
                     <|> resolveRelativePath
                     <|> pathToOCamlLibName,
                   ),
              )
            ) {
            | JSON.NullJSONValue () => None
            | e => raise(e)
            };
          let wrapped =
            try(
              Some(
                JSON.member(conf, "wrapped")
                |> JSON.toValue
                |> FieldTypes.toBool,
              )
            ) {
            | JSON.NullJSONValue () => None
            | e => raise(e)
            };
          {
            pkg_path,
            common,
            pkgType:
              LibraryPackage(
                Library.create(
                  namespace,
                  libraryModes,
                  cStubs,
                  virtualModules,
                  implements,
                  wrapped,
                ),
              ),
          };
        };
      },
      pkgs,
    ),
  );
};

let toDunePackages = (_prjPath, rootName, pkgs) => {
  List.map(
    pkg =>
      switch (pkg.pkgType) {
      | LibraryPackage(l) => Library.toDuneStanza(pkg.common, l)
      | ExecutablePackage(e) => Executable.toDuneStanza(pkg.common, e)
      | TestPackage(e) => Test.toDuneStanza(pkg.common, e)
      },
    pkgs,
  );
};

type fileOperation =
  | UPDATE(string)
  | CREATE(string);

let gen = (projectPath, pkgPath) => {
  let json = JSON.fromFile(pkgPath);
  let (rootName, pesyPackages) = toPesyConf(projectPath, json);
  let rootNameOpamFile = rootName ++ ".opam";
  let dunePackages = toDunePackages(projectPath, rootName, pesyPackages);

  let operations = ref([]);
  List.iter(
    dpkg => {
      let (path, duneFile) = dpkg;
      let duneFilePath = Path.(path / "dune");

      mkdirp(path);

      let duneFileStr = DuneFile.toString(duneFile);
      try(
        if (duneFile != DuneFile.ofFile(duneFilePath)) {
          write(duneFilePath, duneFileStr);
          operations :=
            [
              UPDATE(
                Str.global_replace(
                  Str.regexp(Path.(projectPath / "")),
                  "",
                  duneFilePath,
                ),
              ),
              ...operations^,
            ];
        }
      ) {
      | _ =>
        write(duneFilePath, duneFileStr);
        operations :=
          [
            CREATE(
              Str.global_replace(
                Str.regexp(Path.(projectPath / "")),
                "",
                duneFilePath,
              ),
            ),
            ...operations^,
          ];
      };
    },
    dunePackages,
  );

  let foundAnOpamFile = ref(false);
  let dirForEachEntry = (f, dirname) => {
    let d = Unix.opendir(dirname);
    try(
      while (true) {
        f(Unix.readdir(d));
      }
    ) {
    | End_of_file => Unix.closedir(d)
    };
  };
  let contains = (n, s) =>
    try(Str.search_forward(Str.regexp(s), n, 0) != (-1)) {
    | Not_found => false
    };

  dirForEachEntry(
    n =>
      if (contains(n, ".opam") && ! foundAnOpamFile^) {
        foundAnOpamFile := true;
        if (n != rootNameOpamFile) {
          copyFile(
            Path.(projectPath / n),
            Path.(projectPath / rootNameOpamFile),
          );
          Unix.unlink(Path.(projectPath / n));
          operations := [CREATE(rootNameOpamFile), ...operations^];
        };
      },
    projectPath,
  );
  operations^;
};

/* TODO: Figure better test setup */
let testToPackages = jsonStr => {
  let json = JSON.ofString(jsonStr);
  let (_, pesyPackages) = toPesyConf("", json);
  let dunePackages = toDunePackages("", "", pesyPackages);
  List.map(
    p => {
      let (_, d) = p;
      d;
    },
    dunePackages,
  );
};

/* let%expect_test _ = { */
/*   let duneFiles = */
/*     testToPackages( */
/*       {| */
         /*   { */
         /*     "name": "foo", */
         /*     "buildDirs": { */
         /*       "test": { */
         /*         "require": ["foo"], */
         /*         "bin": { "Bar.exe": "Bar.re" } */
         /*       } */
         /*     } */
         /*   } */
         /*        |}, */
/*     ); */
/*   List.iter(print_endline, List.map(DuneFile.toString, duneFiles)); */
/*   %expect */
/*   {| */
     /*      (executable (name Bar) (public_name Bar.exe) (libraries foo)) */
     /*    |}; */
/* }; */

/* let%expect_test _ = { */
/*   let duneFiles = */
/*     testToPackages( */
/*       {| */
         /*   { */
         /*       "name": "foo", */
         /*       "buildDirs": { */
         /*       "testlib": { */
         /*         "require": ["foo"], */
         /*         "namespace": "Foo", */
         /*         "name": "bar.lib", */
         /*        "modes": ["best"] */
         /*       } */
         /*     } */
         /*   } */
         /*        |}, */
/*     ); */
/*   List.iter(print_endline, List.map(DuneFile.toString, duneFiles)); */
/*   %expect */
/*   {| */
     /*      (library (name Foo) (public_name bar.lib) (libraries foo) (modes best)) */
     /*    |}; */
/* }; */

/* let%expect_test _ = { */
/*   let duneFiles = */
/*     testToPackages( */
/*       {| */
         /*   { */
         /*       "name": "foo", */
         /*       "buildDirs": { */
         /*       "testlib": { */
         /*         "require": ["foo"], */
         /*         "namespace": "Foo", */
         /*         "name": "bar.lib", */
         /*         "cNames": ["stubs"] */
         /*       } */
         /*     } */
         /*   } */
         /*        |}, */
/*     ); */
/*   List.iter(print_endline, List.map(DuneFile.toString, duneFiles)); */
/*   %expect */
/*   {| */
     /*      (library (name Foo) (public_name bar.lib) (libraries foo) (c_names stubs)) */
     /*    |}; */
/* }; */

/* let%expect_test _ = { */
/*   let duneFiles = */
/*     testToPackages( */
/*       {| */
         /*   { */
         /*       "name": "foo", */
         /*       "buildDirs": { */
         /*       "testlib": { */
         /*         "namespace": "Foo", */
         /*         "name": "bar.lib", */
         /*         "virtualModules": ["foo"] */
         /*       } */
         /*     } */
         /*   } */
         /*        |}, */
/*     ); */
/*   List.iter(print_endline, List.map(DuneFile.toString, duneFiles)); */
/*   %expect */
/*   {| */
     /*      (library (name Foo) (public_name bar.lib) (virtual_modules foo)) */
     /*    |}; */
/* }; */

/* let%expect_test _ = { */
/*   let duneFiles = */
/*     testToPackages( */
/*       {| */
         /*   { */
         /*       "name": "foo", */
         /*       "buildDirs": { */
         /*       "testlib": { */
         /*         "namespace": "Foo", */
         /*         "name": "bar.lib", */
         /*         "implements": ["foo"] */
         /*       } */
         /*     } */
         /*   } */
         /*        |}, */
/*     ); */
/*   List.iter(print_endline, List.map(DuneFile.toString, duneFiles)); */
/*   %expect */
/*   {| */
     /*      (library (name Foo) (public_name bar.lib) (implements foo)) */
     /*    |}; */
/* }; */

/* let%expect_test _ = { */
/*   let duneFiles = */
/*     testToPackages( */
/*       {| */
         /*   { */
         /*       "name": "foo", */
         /*       "buildDirs": { */
         /*       "testlib": { */
         /*         "namespace": "Foo", */
         /*         "name": "bar.lib", */
         /*         "wrapped": false */
         /*       } */
         /*     } */
         /*   } */
         /*        |}, */
/*     ); */
/*   List.iter(print_endline, List.map(DuneFile.toString, duneFiles)); */
/*   %expect */
/*   {| */
     /*      (library (name Foo) (public_name bar.lib) (wrapped false)) */
     /*    |}; */
/* }; */

/* let%expect_test _ = { */
/*   let duneFiles = */
/*     testToPackages( */
/*       {| */
         /*   { */
         /*       "name": "foo", */
         /*       "buildDirs": { */
         /*       "testlib": { */
         /*         "bin": { "bar.exe": "Foo.re" }, */
         /*         "modes": ["best", "c"] */
         /*       } */
         /*     } */
         /*   } */
         /*        |}, */
/*     ); */
/*   List.iter(print_endline, List.map(DuneFile.toString, duneFiles)); */
/*   %expect */
/*   {| */
     /*      (executable (name Foo) (public_name bar.exe) (modes (best c))) */
     /*    |}; */
/* }; */

/* let%expect_test _ = { */
/*   let duneFiles = */
/*     testToPackages( */
/*       {| */
         /*            { */
         /*              "name": "foo", */
         /*              "buildDirs": { */
         /*                "testlib": { */
         /*                  "require": ["foo"], */
         /*                  "namespace": "Foo", */
         /*                  "name": "bar.lib", */
         /*                  "flags": ["-w", "-33+9"] */
         /*                } */
         /*              } */
         /*            } */
         /*                 |}, */
/*     ); */
/*   List.iter(print_endline, List.map(DuneFile.toString, duneFiles)); */
/*   %expect */
/*   {| */
     /*      (library (name Foo) (public_name bar.lib) (libraries foo) (flags -w -33+9)) */
     /*    |}; */
/* }; */

/* let%expect_test _ = { */
/*   let duneFiles = */
/*     testToPackages( */
/*       {| */
         /*            { */
         /*              "name": "foo", */
         /*              "buildDirs": { */
         /*                "testlib": { */
         /*                  "namespace": "Foo", */
         /*                  "name": "bar.lib", */
         /*                  "ocamlcFlags": ["-annot", "-c"] */
         /*                } */
         /*              } */
         /*            } */
         /*                 |}, */
/*     ); */
/*   List.iter(print_endline, List.map(DuneFile.toString, duneFiles)); */
/*   %expect */
/*   {| */
     /*      (library (name Foo) (public_name bar.lib) (ocamlc_flags -annot -c)) */
     /*    |}; */
/* }; */

/* let%expect_test _ = { */
/*   let duneFiles = */
/*     testToPackages( */
/*       {| */
         /*            { */
         /*              "name": "foo", */
         /*              "buildDirs": { */
         /*                "testlib": { */
         /*                  "namespace": "Foo", */
         /*                  "name": "bar.lib", */
         /*                  "ocamloptFlags": ["-rectypes", "-nostdlib"] */
         /*                } */
         /*              } */
         /*            } */
         /*                 |}, */
/*     ); */
/*   List.iter(print_endline, List.map(DuneFile.toString, duneFiles)); */
/*   %expect */
/*   {| */
     /*      (library (name Foo) (public_name bar.lib) */
     /*          (ocamlopt_flags -rectypes -nostdlib)) */
     /*    |}; */
/* }; */

/* let%expect_test _ = { */
/*   let duneFiles = */
/*     testToPackages( */
/*       {| */
         /*            { */
         /*              "name": "foo", */
         /*              "buildDirs": { */
         /*                "testlib": { */
         /*                  "namespace": "Foo", */
         /*                  "name": "bar.lib", */
         /*                  "jsooFlags": ["-pretty", "-no-inline"] */
         /*                } */
         /*              } */
         /*            } */
         /*                 |}, */
/*     ); */
/*   List.iter(print_endline, List.map(DuneFile.toString, duneFiles)); */
/*   %expect */
/*   {| */
     /*      (library (name Foo) (public_name bar.lib) (js_of_ocaml -pretty -no-inline)) */
     /*    |}; */
/* }; */

/* let%expect_test _ = { */
/*   let duneFiles = */
/*     testToPackages( */
/*       {| */
         /*            { */
         /*              "name": "foo", */
         /*              "buildDirs": { */
         /*                "testlib": { */
         /*                  "namespace": "Foo", */
         /*                  "name": "bar.lib", */
         /*                  "preprocess": [ "pps", "lwt_ppx" ] */
         /*                } */
         /*              } */
         /*            } */
         /*                 |}, */
/*     ); */
/*   List.iter(print_endline, List.map(DuneFile.toString, duneFiles)); */
/*   %expect */
/*   {| */
     /*      (library (name Foo) (public_name bar.lib) (preprocess (pps lwt_ppx))) */
     /*    |}; */
/* }; */

/* let%expect_test _ = { */
/*   let duneFiles = */
/*     testToPackages( */
/*       {| */
         /*            { */
         /*              "name": "foo", */
         /*              "buildDirs": { */
         /*                "testlib": { */
         /*                  "namespace": "Foo", */
         /*                  "name": "bar.lib", */
         /*                  "includeSubdirs": "unqualified" */
         /*                } */
         /*              } */
         /*            } */
         /*                 |}, */
/*     ); */
/*   List.iter(print_endline, List.map(DuneFile.toString, duneFiles)); */
/*   %expect */
/*   {| */
     /*      (library (name Foo) (public_name bar.lib) (include_subdirs unqualified)) */
     /*    |}; */
/* }; */

/* let%expect_test _ = { */
/*   let duneFiles = */
/*     testToPackages( */
/*       {| */
         /*            { */
         /*              "name": "foo", */
         /*              "buildDirs": { */
         /*                "testlib": { */
         /*                  "namespace": "Foo", */
         /*                  "name": "bar.lib", */
         /*                  "rawBuildConfig": [ */
         /*                    "(libraries lwt lwt.unix raw.lib)", */
         /*                    "(preprocess (pps lwt_ppx))" */
         /*                  ] */
         /*                } */
         /*              } */
         /*            } */
         /*                 |}, */
/*     ); */
/*   List.iter(print_endline, List.map(DuneFile.toString, duneFiles)); */
/*   %expect */
/*   {| */
     /*      (library (name Foo) (public_name bar.lib) (libraries lwt lwt.unix raw.lib) */
     /*          (preprocess (pps lwt_ppx))) */
     /*    |}; */
/* }; */

/* let%expect_test _ = { */
/*   let duneFiles = */
/*     testToPackages( */
/*       {| */
         /*            { */
         /*              "name": "foo", */
         /*              "buildDirs": { */
         /*                "testlib": { */
         /*                  "namespace": "Foo", */
         /*                  "name": "bar.lib", */
         /*                  "rawBuildConfigFooter": [ */
         /*                    "(install (section share_root) (files (asset.txt as asset.txt)))" */
         /*                  ] */
         /*                } */
         /*              } */
         /*            } */
         /*                 |}, */
/*     ); */
/*   List.iter(print_endline, List.map(DuneFile.toString, duneFiles)); */
/*   %expect */
/*   {| */
     /*      (library (name Foo) (public_name bar.lib)) (install (section share_root) (files (asset.txt as asset.txt))) */
     /*    |}; */
/* }; */

/* let%expect_test _ = { */
/*   let duneFiles = */
/*     testToPackages( */
/*       {| */
         /*            { */
         /*              "name": "foo", */
         /*              "buildDirs": { */
         /*                "testexe": { */
         /*                  "bin": { "Foo.exe": "Foo.re" }, */
         /*                  "rawBuildConfigFooter": [ */
         /*                    "(install (section share_root) (files (asset.txt as asset.txt)))" */
         /*                  ] */
         /*                } */
         /*              } */
         /*            } */
         /*                 |}, */
/*     ); */
/*   List.iter(print_endline, List.map(DuneFile.toString, duneFiles)); */
/*   %expect */
/*   {| */
     /*      (executable (name Foo) (public_name Foo.exe)) (install (section share_root) (files (asset.txt as asset.txt))) */
     /*    |}; */
/* }; */

/* let%expect_test _ = { */
/*   let duneFiles = */
/*     testToPackages( */
/*       {| */
         /*            { */
         /*              "name": "foo", */
         /*              "buildDirs": { */
         /*                "testexe": { */
         /*                  "bin": { "Foo.exe": "Foo.re" } */
         /*                } */
         /*              } */
         /*            } */
         /*        |}, */
/*     ); */
/*   List.iter(print_endline, List.map(DuneFile.toString, duneFiles)); */
/*   %expect */
/*   {| */
     /*      (executable (name Foo) (public_name Foo.exe)) */
     /*    |}; */
/* }; */

/* let%expect_test _ = { */
/*   let duneFileErrors = */
/*     try ( */
/*       { */
/*         ignore( */
/*           testToPackages( */
/*             {| */
               /*            { */
               /*              "name": "foo", */
               /*              "buildDirs": { */
               /*                "testexe": { */
               /*                  "bin": [] */
               /*                } */
               /*              } */
               /*            } */
               /*        |}, */
/*           ), */
/*         ); */
/*         raise(ShouldHaveRaised()); */
/*       } */
/*     ) { */
/*     | InvalidBinProperty(p) => [sprintf("InvalidBinProperty for %s", p)] */
/*     }; */
/*   List.iter(print_endline, duneFileErrors); */
/*   %expect */
/*   {| */
     /*      InvalidBinProperty for testexe */
     /*    |}; */
/* }; */

let log = operations => {
  print_newline();
  List.iter(
    o => {
      switch (o) {
      | CREATE(x) =>
        print_endline(
          Pastel.(
            <Pastel> "    Created " <Pastel bold=true> x </Pastel> </Pastel>
          ),
        )
      | UPDATE(x) =>
        print_endline(
          Pastel.(
            <Pastel> "    Updated " <Pastel bold=true> x </Pastel> </Pastel>
          ),
        )
      };
      ();
    },
    operations,
  );
  print_newline();
};

/** DEPRECATED: Pesy is not supposed to be run in build env https://github.com/jchavarri/rebez/issues/4 **/
let validateDuneFiles = (projectPath, pkgPath) => {
  let json = JSON.fromFile(pkgPath);
  let (rootName, pesyPackages) = toPesyConf(projectPath, json);
  let rootNameOpamFile = rootName ++ ".opam";
  let dunePackages = toDunePackages(projectPath, rootName, pesyPackages);
  let staleDuneFiles =
    dunePackages
    |> List.map(dpkg => {
         let (path, updatedDuneFile) = dpkg;
         let currentDuneFilePath = Path.(path / "dune");
         (updatedDuneFile, currentDuneFilePath);
       })
    |> List.filter(dpkg => {
         let (updatedDuneFile, currentDuneFilePath) = dpkg;
         updatedDuneFile != DuneFile.ofFile(currentDuneFilePath);
       })
    |> List.map(t => {
         let (_updatedDuneFile, currentDuneFilePath) = t;
         currentDuneFilePath;
       });

  let foundAnOpamFile = ref(false);
  let dirForEachEntry = (f, dirname) => {
    let d = Unix.opendir(dirname);
    try(
      while (true) {
        f(Unix.readdir(d));
      }
    ) {
    | End_of_file => Unix.closedir(d)
    };
  };
  let contains = (n, s) =>
    try(Str.search_forward(Str.regexp(s), n, 0) != (-1)) {
    | Not_found => false
    };

  let staleOpamFile = ref(None);
  dirForEachEntry(
    n =>
      if (contains(n, ".opam") && ! foundAnOpamFile^) {
        foundAnOpamFile := true;
        if (n != rootNameOpamFile) {
          staleOpamFile := Some((n, rootNameOpamFile));
        };
      },
    projectPath,
  );

  let errors =
    List.map(x => StaleDuneFile(x), staleDuneFiles)
    @ (
      switch (staleOpamFile^) {
      | Some(x) => [StaleOpamFile(x)]
      | None => []
      }
    );

  if (List.length(errors) != 0) {
    raise(BuildValidationFailures(errors));
  };

  Str.global_replace(Str.regexp(".opam"), "", rootNameOpamFile);
};
