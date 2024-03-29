open Printf;
module Utils = PesyEsyPesyUtils.Utils;
open Utils;
open PesyEsyPesyErrors.Errors;

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

module Version = {
  type t =
    | V1(int)
    | V2(int)
    | V3(int);
  let ofString = version => {
    let vs = String.split_on_char('.', version) |> List.map(int_of_string);
    switch (vs) {
    | [1, minorVersion] => V1(minorVersion)
    | [2, minorVersion] => V2(minorVersion)
    | [3, minorVersion] => V3(minorVersion)
    | _ => raise(InvalidDuneVersion(version))
    };
  };
};

/* private */
exception ShouldHaveRaised(unit);

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

  let dirSepChar = Filename.dir_sep.[0];
  (path.[0] == dirSepChar ? Filename.dir_sep : "")
  ++ String.concat(separator, resolve(revParts, 0, []));
};

let specialCaseOpamRequires = package =>
  Str.global_replace(Str.regexp("^@opam/"), "", package);

module Result = {
  let bind = (o, f) =>
    switch (o) {
    | Ok(x) => f(x)
    | Error(e) => Error(e)
    };
  let ( let* ) = (x, f) => bind(x, f);
};

let isValidScopeName = n => {
  n.[0] == '@';
};

let resolveDepNameToFindlib = depName => {
  let skipEnvVar = envVar =>
    Array.fold_left(
      (acc, e) =>
        if (Str.string_match(
              Str.regexp(Printf.sprintf("%s=.*", envVar)),
              e,
              0,
            )) {
          acc;
        } else {
          [e, ...acc];
        },
      [],
    );
  let list_diff = (l1, l2) => {
    l1 |> List.filter(e => !List.mem(e, l2));
  };

  /**
The above parses {|
foo (version: n/a)
foo.lib (version: n/a)
foo.bar.lib (version: n/a)
|}
to
[["foo"], ["foo", "lib"], ["foo", "bar", "lib"]]
              */
  let parseRelevantFindlibOutput = relevantFindlibOutputLines =>
    relevantFindlibOutputLines
    |> List.map(x =>
         Str.split(Str.regexp(" "), x)
         |> List.hd
         |> Str.split(Str.regexp("\\."))
       );
  let safeSeg = name => {
    /* Taken from esy's source */
    let replaceAt = Str.regexp("@");
    let replaceUnderscore = Str.regexp("_+");
    let replaceSlash = Str.regexp("\\/");
    let replaceDash = Str.regexp("\\-");
    let replaceColon = Str.regexp(":");
    name
    |> String.lowercase_ascii
    |> Str.global_replace(replaceAt, "")
    |> Str.global_replace(replaceUnderscore, "__")
    |> Str.global_replace(replaceSlash, "__s__")
    |> Str.global_replace(replaceColon, "__c__")
    |> Str.global_replace(replaceDash, "_");
  };
  let fullDepNameParts = Str.split(Str.regexp("/"), depName);
  let (depName, remainingParts) =
    switch (fullDepNameParts) {
    | [] => failwith("How can this be empty?")
    | [x] => (x, [])
    | [x, y, ...rest] =>
      if (isValidScopeName(x)) {
        (x ++ "/" ++ y, rest);
      } else {
        (x, [y, ...rest]);
      }
    };
  let process_env =
    Unix.environment() |> skipEnvVar("ESY__ROOT_PACKAGE_CONFIG_PATH");
  let esyfiedDepName = safeSeg(depName);
  open Result;
  let* ocamlpath =
    switch (Sys.getenv_opt("OCAMLPATH")) {
    | Some(v) => Ok(v)
    | None => Error("OCAMLPATH not found in env")
    };
  let candidates =
    Str.split(Str.regexp(Sys.unix ? ":" : ";"), ocamlpath)
    |> List.filter_map(v =>
         if (Str.string_match(
               Str.regexp(".*" ++ esyfiedDepName ++ "-.*"),
               v,
               0,
             )) {
           let (_exitCode, emptyOutput) =
             run(
               ~env=
                 Array.of_list([
                   "OCAMLPATH=",
                   ...process_env |> List.filter(x => x != "OCAMLPATH"),
                 ]),
               "ocamlfind",
               [|"list"|],
             );
           let (_exitCode, depOutput) =
             run(
               ~env=
                 Array.of_list([
                   Printf.sprintf("OCAMLPATH=%s", v),
                   ...process_env |> List.filter(x => x != "OCAMLPATH"),
                 ]),
               "ocamlfind",
               [|"list"|],
             );
           let relevantFindlibOutputLines = list_diff(depOutput, emptyOutput);
           let listOfLibraryNameAsList =
             parseRelevantFindlibOutput(relevantFindlibOutputLines);
           /** Since the library base name (say foo) need not match the
              npm package name, we are only going to compare the rest (["lib"], ["bar", "lib"] etc) */
           let listOfCandidateLibraryNameAsList =
             listOfLibraryNameAsList
             |> List.filter_map(
                  fun
                  | [] => None
                  | [_, ...rest] as libraryNameAsList =>
                    if (rest == remainingParts) {
                      Some(libraryNameAsList);
                    } else {
                      None;
                    },
                );
           switch (listOfCandidateLibraryNameAsList) {
           | [] => None
           | candidateLibraryNameAsList =>
             switch (candidateLibraryNameAsList) {
             | [] => None
             | x => Some(x |> List.map(String.concat("/")))
             }
           };
         } else {
           None;
         }
       )
    |> List.flatten;

  switch (candidates) {
  | [] =>
    Error(
      Printf.sprintf(
        "Library %s not found",
        String.concat("/", [depName, ...remainingParts]),
      ),
    )
  | [candidate] => Ok(candidate)
  | multipleCandidates =>
    Error(
      Printf.sprintf(
        {|Multiple candidates found for %s
         %s|},
        depName,
        multipleCandidates |> String.concat("\n"),
      ),
    )
  };
};

let isValidBinaryFileName = fileName =>
  Str.string_match(Str.regexp("^.+\\.exe$"), fileName, 0);

let isValidSourceFile = fileName =>
  Str.string_match(Str.regexp("^.+\\.\\(re\\|ml\\)$"), fileName, 0);

/* Turns "Foo.re as Foo.exe" => ("Foo.re", "Foo.exe") */

/* Turns "foo/bar/baz" => "foo.bar.baz" */
let pathToOCamlLibName = p => Str.global_replace(Str.regexp("/"), ".", p);

let stripAtTheRate = s => String.sub(s, 1, String.length(s) - 1);

/* doubleKebabifyIfScoped turns @myscope/pkgName => myscope--pkgName */
let doubleKebabifyIfScoped = n => {
  switch (Str.split(Str.regexp("/"), n)) {
  | [pkgName] => pkgName
  | [scope, pkgName, ...rest] =>
    isValidScopeName(scope)
      ? String.concat(
          "/",
          [stripAtTheRate(scope) ++ "--" ++ pkgName, ...rest],
        )
      : String.concat("/", [scope, pkgName, ...rest])
  | _ => raise(InvalidRootName(n))
  };
};

type t = (string, list(package));
let toPesyConf = (projectPath, rootName, pkg, ~duneVersion) => {
  let (dir, conf) = pkg;

  let binJSON = JSON.member(conf, "bin");
  let bin =
    try(
      Some(
        {
          JSON.toKeyValuePairs(binJSON)
          |> List.map(kv => {
               let (k, v) = kv;
               (
                 /* Value is the source .re file */
                 v |> JSON.toValue |> FieldTypes.toString,
                 /* Key is the target executable name */
                 k,
               );
             });
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
      Some([(binaryMainFile, moduleNameOf(binaryMainFile) ++ ".exe")]);
    | e => raise(e)
    };

  /* Pesy'name is Dune's public_name */
  /* If name is provided and binary's public_name is also provided in the bin property, name takes precedence */
  let name =
    try(JSON.member(conf, "name") |> JSON.toValue |> FieldTypes.toString) {
    | JSON.NullJSONValue(_) =>
      switch (bin) {
      | Some(names) =>
        names
        |> List.map(((_mainFileName, installedBinaryName)) =>
             installedBinaryName
           )
        |> List.hd
      | None => rootName ++ "." ++ pathToOCamlLibName(dir)
      }
    | e => raise(e)
    };

  let public =
    try(JSON.member(conf, "public") |> JSON.toValue |> FieldTypes.toBool) {
    | JSON.NullJSONValue(_) => false
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
             x => x.[0] == '.' ? sprintf("%s/%s/%s", rootName, dir, x) : x
           )
           <|> resolveRelativePath
           <|> (
             x =>
               x.[0] == '@'
                 ? if (findIndex(doubleKebabifyIfScoped(x), rootName) != 0) {
                     switch (x |> resolveDepNameToFindlib) {
                     | Ok(x) => x
                     | Error(msg) => raise(Failure(msg))
                     };
                   } else {
                     x |> doubleKebabifyIfScoped;
                   }
                 : x
           )
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
    | _e => raise(ImportsParserFailure())
    };

  let modesAsFieldTypes =
    try(Some(JSON.member(conf, "modes") |> JSON.toValue)) {
    | JSON.NullJSONValue () => None
    | e => raise(e)
    };

  let modes =
    switch (modesAsFieldTypes) {
    | Some(modesAsFieldTypes) =>
      try(Some(Executable.Mode.ofFieldTypes(modesAsFieldTypes))) {
      | _ => None
      }
    | None => None
    };

  let pesyModuleNamespace =
    [rootName]
    @ String.split_on_char('/', dir)
    @ ["PesyModules"]
    |> List.map(upperCamelCasify)
    |> List.fold_left((++), "");

  let isLocalLibrary = path => findIndex(path, rootName) == 0;
  let aliases =
    imports
    |> List.map(import => {
         let (exportedNamespace, lib) = import;
         let libraryAsPath =
           lib
           |> (
             x => {
               x.[0] == '.' ? sprintf("%s/%s/%s", rootName, dir, x) : x;
             }
           )
           |> resolveRelativePath
           |> (
             x =>
               x.[0] == '@'
                 ? if (findIndex(doubleKebabifyIfScoped(x), rootName) != 0) {
                     switch (x |> resolveDepNameToFindlib) {
                     | Ok(x) => x
                     | Error(msg) => raise(Failure(msg))
                     };
                   } else {
                     x |> doubleKebabifyIfScoped;
                   }
                 : x
           );
         let stripRootName =
           Str.global_replace(Str.regexp(rootName ++ "/"), "");
         let basePathToRequirePkg =
           libraryAsPath
           |> stripRootName
           |> (x => Path.(projectPath / x))
           |> resolveRelativePath;

         if (isLocalLibrary(libraryAsPath)) {
           if (!Sys.file_exists(basePathToRequirePkg)) {
             raise(PesyEsyPesyErrors.Errors.LocalLibraryPathNotFound(lib));
           };
         };

         let originalNamespace =
           if (isLocalLibrary(libraryAsPath)) {
             libraryAsPath
             |> String.split_on_char('/')
             |> List.map(upperCamelCasify)
             |> List.fold_left((++), "");
           } else {
             /** ie. external library. We use findlib and figure out namespace **/
             let findlibQueryModes =
               switch (modesAsFieldTypes) {
               | Some(modes) =>
                 modes |> FieldTypes.toList |> List.map(FieldTypes.toString)
               | None => ["native"]
               };
             Str.global_replace(
               Str.regexp("\\.cm.*"),
               "",
               Findlib.package_property(
                 findlibQueryModes,
                 pathToOCamlLibName(libraryAsPath),
                 "archive",
               ),
             )
             |> String.mapi((i, c) => i == 0 ? Char.uppercase_ascii(c) : c);
           };

         PesyModule.Alias.create(
           ~alias={
             switch (
               List.fold_left(
                 (accExt, ext) =>
                   List.fold_left(
                     (accEntry, entry) => {
                       switch (accEntry) {
                       | Some(x) => Some(x)
                       | None =>
                         if (findIndex(libraryAsPath, rootName) != 0) {
                           Some(
                             sprintf(
                               "module %s = %s;",
                               exportedNamespace,
                               originalNamespace,
                             ),
                           );
                         } else if (Sys.file_exists(
                                      Path.(basePathToRequirePkg / entry)
                                      ++ ext,
                                    )
                                    || Sys.file_exists(
                                         Path.(
                                           basePathToRequirePkg
                                           / String.lowercase_ascii(entry)
                                         )
                                         ++ ext,
                                       )) {
                           Some(
                             sprintf(
                               "module %s = %s.%s;",
                               exportedNamespace,
                               originalNamespace,
                               entry,
                             ),
                           );
                         } else {
                           None;
                         }
                       }
                     },
                     accExt,
                     ["Index", exportedNamespace] /* If it finds, Index.re, it doesn't look for Bar.re */
                   ),
                 None,
                 [".re", ".ml"],
               )
             ) {
             | Some(x) => x
             | None =>
               sprintf(
                 "module %s = %s;",
                 exportedNamespace,
                 originalNamespace,
               )
             };
           },
           ~internal=isLocalLibrary(libraryAsPath),
           ~library=pathToOCamlLibName(libraryAsPath),
           ~originalNamespace,
           ~exportedNamespace,
         );
       });

  let aliasesWithDifferentNamespace =
    PesyModule.Alias.(
      aliases
      |> List.filter(alias =>
           alias.originalNamespace != alias.exportedNamespace
         )
    );

  /*
    Ex: Pastel = require('pastel/lib'). In this case,
    we should skip aliasing. See: https://github.com/esy/pesy/issues/171
   */
  let aliasesWithSameNamespace =
    PesyModule.Alias.(
      aliases
      |> List.filter(alias =>
           alias.originalNamespace == alias.exportedNamespace
         )
    );

  let pesyModules =
    PesyModule.create(
      ~namespace=pesyModuleNamespace,
      ~dunePublicName=
        sprintf(
          "%s.pesy-modules",
          pathToOCamlLibName(rootName ++ "/" ++ dir),
        ),
      ~aliases=aliasesWithDifferentNamespace,
    );

  let fromListOrString = ls =>
    try(FieldTypes.toList(ls)) {
    | FieldTypes.ConversionException("Expected list. Actual string") =>
      FieldTypes.toString(ls)
      |> Str.split(Str.regexp("[ \n\r\x0c\t]+"))
      |> List.map(s => FieldTypes.String(s))
    | e => raise(e)
    };

  let flags =
    try(
      Some(
        JSON.member(conf, "flags")
        |> JSON.toValue
        |> fromListOrString
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
        |> fromListOrString
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
        |> fromListOrString
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
        |> fromListOrString
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
      Path.(projectPath / dir),
      List.append(
        require,
        PesyModule.Alias.(
          aliasesWithSameNamespace
          |> List.map(pesyModule => pesyModule.library)
        ),
      ),
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

  /* Prioritising `bin` over `name` */
  switch (bin) {
  | Some(binKVs) =>
    //bins |> List.map(((mainFileName, _installedBinaryName)) => moduleNameOf(mainFileName))
    {
      pkg_path,
      common,
      pkgType: ExecutablePackage(Executable.create(binKVs, modes)),
    }
  | None =>
    let namespace =
      try(
        JSON.member(conf, "namespace") |> JSON.toValue |> FieldTypes.toString
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

    let cStubs = cns =>
      try(
        JSON.toValue(cns)
        |> FieldTypes.toList
        |> List.map(FieldTypes.toString)
      ) {
      | e => raise(e)
      };

    let foreignStubs = fss =>
      try(
        JSON.toListKVPairs(fss)
        |> List.map(kvs =>
             List.map(
               kv => {
                 let (k, v) = kv;
                 (k, v |> JSON.toValue);
               },
               kvs,
             )
           )
      ) {
      | e => raise(e)
      };

    let (cnJSON, fsJSON) = (
      JSON.member(conf, "cNames"),
      JSON.member(conf, "foreignStubs"),
    );

    let ffi =
      switch (
        Version.ofString(duneVersion),
        JSON.toOption(cnJSON),
        JSON.toOption(fsJSON),
      ) {
      | (V1(_), None, None) => None
      | (V1(_), Some(cn), None)
      | (V1(_), Some(cn), Some(_)) => Some(Stubs.ofCNames(cStubs(cn)))
      | (V1(_), None, Some(_)) => raise(ForeignStubsIncorrectlyUsed)
      /* foreign stubs is supported in version > 1.0 */
      | (_, None, None) => None
      | (_, Some(_), None) => raise(CNamesIncorrectlyUsed)
      | (_, None, Some(fs))
      | (_, Some(_), Some(fs)) =>
        Some(Stubs.ofForeignStubs(foreignStubs(fs)))
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
                 x => x.[0] == '.' ? sprintf("%s/%s/%s", rootName, dir, x) : x
               )
               <|> (
                 x =>
                   x.[0] == '@'
                     ? if (findIndex(doubleKebabifyIfScoped(x), rootName)
                           != 0) {
                         switch (x |> resolveDepNameToFindlib) {
                         | Ok(x) => x
                         | Error(msg) => raise(Failure(msg))
                         };
                       } else {
                         x |> doubleKebabifyIfScoped;
                       }
                     : x
               )
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
          JSON.member(conf, "wrapped") |> JSON.toValue |> FieldTypes.toBool,
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
            name,
            public,
            namespace,
            libraryModes,
            ffi,
            virtualModules,
            implements,
            wrapped,
          ),
        ),
    };
  };
};

let toDunePackages = (_prjPath, _rootName, pkg) => {
  switch (pkg.pkgType) {
  | LibraryPackage(l) => Library.toDuneStanza(pkg.common, l)
  | ExecutablePackage(e) => Executable.toDuneStanza(pkg.common, e)
  | TestPackage(e) => Test.toDuneStanza(pkg.common, e)
  };
};

/* TODO: Figure better test setup */
/** DEPRECATED: Pesy is not supposed to be run in build env https://github.com/jchavarri/rebez/issues/4 **/;
/* let validateDuneFiles = (projectPath, pkgPath) => { */
/*   let json = JSON.fromFile(pkgPath); */
/*   let (rootName, pesyPackages) = toPesyConf(projectPath, json); */
/*   let rootNameOpamFile = rootName ++ ".opam"; */
/*   let dunePackages = toDunePackages(projectPath, rootName, pesyPackages); */
/*   let staleDuneFiles = */
/*     dunePackages */
/*     |> List.map(dpkg => { */
/*          let (path, updatedDuneFile) = dpkg; */
/*          let currentDuneFilePath = Path.(path / "dune"); */
/*          (updatedDuneFile, currentDuneFilePath); */
/*        }) */
/*     |> List.filter(dpkg => { */
/*          let (updatedDuneFile, currentDuneFilePath) = dpkg; */
/*          updatedDuneFile != DuneFile.ofFile(currentDuneFilePath); */
/*        }) */
/*     |> List.map(t => { */
/*          let (_updatedDuneFile, currentDuneFilePath) = t; */
/*          currentDuneFilePath; */
/*        }); */

/*   let foundAnOpamFile = ref(false); */
/*   let dirForEachEntry = (f, dirname) => { */
/*     let d = Unix.opendir(dirname); */
/*     try( */
/*       while (true) { */
/*         f(Unix.readdir(d)); */
/*       } */
/*     ) { */
/*     | End_of_file => Unix.closedir(d) */
/*     }; */
/*   }; */
/*   let contains = (n, s) => */
/*     try(Str.search_forward(Str.regexp(s), n, 0) != (-1)) { */
/*     | Not_found => false */
/*     }; */

/*   let staleOpamFile = ref(None); */
/*   dirForEachEntry( */
/*     n => */
/*       if (contains(n, ".opam") && ! foundAnOpamFile^) { */
/*         foundAnOpamFile := true; */
/*         if (n != rootNameOpamFile) { */
/*           staleOpamFile := Some((n, rootNameOpamFile)); */
/*         }; */
/*       }, */
/*     projectPath, */
/*   ); */

/*   let errors = */
/*     List.map(x => StaleDuneFile(x), staleDuneFiles) */
/*     @ ( */
/*       switch (staleOpamFile^) { */
/*       | Some(x) => [StaleOpamFile(x)] */
/*       | None => [] */
/*       } */
/*     ); */

/*   if (List.length(errors) != 0) { */
/*     raise(BuildValidationFailures(errors)); */
/*   }; */

/*   Str.global_replace(Str.regexp(".opam"), "", rootNameOpamFile); */
/* }; */

/* "name" in root package.json */
let rootName = json =>
  try(
    doubleKebabifyIfScoped(
      JSON.member(json, "name") |> JSON.toValue |> FieldTypes.toString,
    )
  ) {
  | JSON.NullJSONValue () => raise(ShouldNotBeNull("name"))
  | x => raise(x)
  };

let get = manifestFile => {
  JSON.fromFile(manifestFile);
};

let pkgs = json => JSON.toKeyValuePairs(JSON.member(json, "buildDirs"));
