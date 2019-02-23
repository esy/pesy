open Printf;
open PesyUtils;
open PesyUtils.NoLwt;

exception NullJSONValue(unit);
exception FatalError(string);
exception ShouldNotBeHere(unit);
exception InvalidRootName(string);

module Library: {
  module Mode: {
    exception InvalidLibraryMode(string);
    type t;
    let ofString: string => t;
    let toString: t => string;
  };
  type t;
  let create:
    (
      string,
      option(list(Mode.t)),
      option(list(string)),
      option(list(string)),
      option(list(string)),
      option(bool)
    ) =>
    t;
  let toDuneStanza: (Common.t, t) => (string, string);
} = {
  module Mode = {
    exception InvalidLibraryMode(string);
    type t =
      | Native
      | Byte
      | Best;
    let ofString =
      fun
      | "best" => Best
      | "native" => Native
      | "byte" => Byte
      | x => raise(InvalidLibraryMode(x));
    let toString =
      fun
      | Best => "best"
      | Native => "native"
      | Byte => "byte";
  };
  type t = {
    namespace: string,
    modes: option(list(Mode.t)),
    cNames: option(list(string)),
    virtualModules: option(list(string)),
    implements: option(list(string)),
    wrapped: option(bool),
  };
  let create = (namespace, modes, cNames, virtualModules, implements, wrapped) => {
    namespace,
    modes,
    cNames,
    virtualModules,
    implements,
    wrapped,
  };
  let toDuneStanza = (common, lib) => {
    /* let {name: pkgName, require, path} = common */
    let {
      namespace,
      modes: modesP,
      cNames: cNamesP,
      virtualModules: virtualModulesP,
      implements: implementsP,
      wrapped: wrappedP,
    } = lib;
    let (
      public_name,
      libraries,
      flags,
      ocamlcFlags,
      ocamloptFlags,
      jsooFlags,
      preprocess,
      includeSubdirs,
      rawBuildConfig,
      rawBuildConfigFooter,
    ) =
      Common.toDuneStanzas(common);
    let path = Common.getPath(common);
    let name = Stanza.create("name", Stanza.createAtom(namespace));

    let modesD =
      switch (modesP) {
      | None => None
      | Some(l) =>
        Some(
          Stanza.createExpression([
            Stanza.createAtom("modes"),
            ...List.map(m => m |> Mode.toString |> Stanza.createAtom, l),
          ]),
        )
      };

    let cNamesD =
      switch (cNamesP) {
      | None => None
      | Some(l) =>
        Some(
          Stanza.createExpression([
            Stanza.createAtom("c_names"),
            ...List.map(Stanza.createAtom, l),
          ]),
        )
      };

    let virtualModulesD =
      switch (virtualModulesP) {
      | None => None
      | Some(l) =>
        Some(
          Stanza.createExpression([
            Stanza.createAtom("virtual_modules"),
            ...List.map(Stanza.createAtom, l),
          ]),
        )
      };

    let implementsD =
      switch (implementsP) {
      | None => None
      | Some(l) =>
        Some(
          Stanza.createExpression([
            Stanza.createAtom("implements"),
            ...List.map(Stanza.createAtom, l),
          ]),
        )
      };

    let wrappedD =
      switch (wrappedP) {
      | None => None
      | Some(w) =>
        Some(
          Stanza.createExpression([
            Stanza.createAtom("wrapped"),
            Stanza.createAtom(string_of_bool(w)),
          ]),
        )
      };

    let mandatoryExpressions = [name, public_name];
    let optionalExpressions = [
      libraries,
      modesD,
      cNamesD,
      virtualModulesD,
      implementsD,
      wrappedD,
      flags,
      ocamlcFlags,
      ocamloptFlags,
      jsooFlags,
      preprocess,
      includeSubdirs,
    ];

    let rawBuildConfig =
      switch (rawBuildConfig) {
      | Some(l) => l
      | None => []
      };

    let rawBuildConfigFooter =
      switch (rawBuildConfigFooter) {
      | Some(l) => l
      | None => []
      };

    let library =
      Stanza.createExpression([
        Stanza.createAtom("library"),
        ...mandatoryExpressions
           @ filterNone(optionalExpressions)
           @ rawBuildConfig,
      ]);

    (path, DuneFile.toString([library, ...rawBuildConfigFooter]));
  };
};

module Executable: {
  type t;
  module Mode: {
    type t;
    let ofList: list(string) => t;
    let toList: t => list(string);
  };
  let create: (string, option(Mode.t)) => t;
  let toDuneStanza: (Common.t, t) => (string, string);
} = {
  module Mode = {
    exception InvalidCompilationMode(unit);
    exception InvalidBinaryKind(unit);
    module Compilation: {
      type t;
      let toString: t => string;
      let ofString: string => t;
    } = {
      type t =
        | Byte
        | Native
        | Best;

      let toString =
        fun
        | Byte => "byte"
        | Native => "native"
        | Best => "best";

      let ofString =
        fun
        | "byte" => Byte
        | "native" => Native
        | "best" => Best
        | _ => raise(InvalidCompilationMode());
    };

    module BinaryKind: {
      type t;
      let toString: t => string;
      let ofString: string => t;
    } = {
      type t =
        | C
        | Exe
        | Object
        | Shared_object;

      let toString =
        fun
        | C => "c"
        | Exe => "exe"
        | Object => "object"
        | Shared_object => "shared_object";

      let ofString =
        fun
        | "c" => C
        | "exe" => Exe
        | "object" => Object
        | "shared_object" => Shared_object
        | _ => raise(InvalidBinaryKind());
    };

    type t = (Compilation.t, BinaryKind.t);
    exception InvalidExecutableMode(string);
    let ofList = parts =>
      switch (parts) {
      | [c, b] => (Compilation.ofString(c), BinaryKind.ofString(b))
      | _ =>
        raise(
          InvalidExecutableMode(
            sprintf(
              "Invalid executable mode: expected of the form (<compilation mode>, <binary_kind>). Got %s",
              List.fold_left((a, e) => sprintf("%s %s", a, e), "", parts),
            ),
          ),
        )
      };
    let toList = m => {
      let (c, b) = m;
      [Compilation.toString(c), BinaryKind.toString(b)];
    };
  };
  type t = {
    main: string,
    modes: option(Mode.t),
  };
  let create = (main, modes) => {main, modes};
  let toDuneStanza = (common: Common.t, e) => {
    /* let {name: pkgName, require, path} = common; */
    let {main, modes: modesP} = e;
    let (
      public_name,
      libraries,
      flags,
      ocamlcFlags,
      ocamloptFlags,
      jsooFlags,
      preprocess,
      includeSubdirs,
      rawBuildConfig,
      rawBuildConfigFooter,
    ) =
      Common.toDuneStanzas(common);
    let path = Common.getPath(common);
    /* Pesy's main is Dune's name */
    let name = Stanza.create("name", Stanza.createAtom(main));
    /* let public_name = */
    /*   Stanza.create("public_name", Stanza.createAtom(pkgName)); */

    /* let libraries = */
    /*   switch (require) { */
    /*   | [] => None */
    /*   | libs => */
    /*     Some( */
    /*       Stanza.createExpression([ */
    /*         Stanza.createAtom("libraries"), */
    /*         ...List.map(r => Stanza.createAtom(r), libs), */
    /*       ]), */
    /*     ) */
    /*   }; */

    let modesD =
      switch (modesP) {
      | None => None
      | Some(m) =>
        Some(
          Stanza.createExpression([
            Stanza.createAtom("modes"),
            Stanza.createExpression(
              m |> Mode.toList |> List.map(Stanza.createAtom),
            ),
          ]),
        )
      };

    let mandatoryExpressions = [name, public_name];
    let optionalExpressions = [
      libraries,
      modesD,
      flags,
      ocamlcFlags,
      ocamloptFlags,
      jsooFlags,
      preprocess,
      includeSubdirs,
    ];

    let rawBuildConfig =
      switch (rawBuildConfig) {
      | Some(l) => l
      | None => []
      };

    let rawBuildConfigFooter =
      switch (rawBuildConfigFooter) {
      | Some(l) => l
      | None => []
      };

    let executable =
      Stanza.createExpression([
        Stanza.createAtom("executable"),
        ...mandatoryExpressions
           @ filterNone(optionalExpressions)
           @ rawBuildConfig,
      ]);

    (path, DuneFile.toString([executable, ...rawBuildConfigFooter]));
  };
};

type pkgType =
  | ExecutablePackage(Executable.t)
  | LibraryPackage(Library.t);

type package = {
  common: Common.t,
  pkgType,
};

exception GenericException(string);

let getSuffix = name => {
  let parts = Str.split(Str.regexp("\\."), name);
  switch (List.rev(parts)) {
  | [_]
  | [] =>
    raise(
      GenericException(
        "`name` property of the package must either be <package>.exe (for executables) or <package>.<suffix> for libraries, where of course suffix is not exe for libraries",
      ),
    )
  | [suffix, ...r] => suffix
  };
};

/* */
/*  Inline ppx is supported too. */
/* let%test "getSuffix(): must return suffix" = getSuffix("foo.lib") == "lib"; */
/* */

let%expect_test _ = {
  print_endline(getSuffix("foo.lib"));
  %expect
  {|
     lib
   |};
};

let%expect_test _ = {
  print_endline(getSuffix("foo.bar.lib"));
  %expect
  {|
     lib
     |};
};

let%expect_test _ = {
  print_endline(
    try (getSuffix("foo")) {
    | GenericException(_) => "Must throw GenericException"
    | _ => "Did not throw GenericException"
    },
  );
  %expect
  {|
     Must throw GenericException
  |};
};

exception ResolveRelativePathFailure(string);
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

let%expect_test _ = {
  print_endline(resolveRelativePath("foo/lib"));
  %expect
  {|
     foo/lib
   |};
};

let%expect_test _ = {
  print_endline(resolveRelativePath("foo/bar/.."));
  %expect
  {|
     foo
   |};
};

let%expect_test _ = {
  print_endline(resolveRelativePath("foo/bar/../.."));
  %expect
  {|

   |};
};

let%expect_test _ = {
  let result =
    try (resolveRelativePath("foo/bar/../../..")) {
    | ResolveRelativePathFailure(x) =>
      sprintf("ResolveRelativePathFailure(\"%s\")", x)
    | e => raise(e)
    };
  print_endline(result);
  %expect
  {|
          ResolveRelativePathFailure("Failed resolving: foo/bar/../../.. Too many `../`")
     |};
};

let%expect_test _ = {
  print_endline(resolveRelativePath("foo/bar/../baz/"));
  %expect
  {|
     foo/baz
   |};
};

module FieldTypes = {
  type t =
    | Bool(bool)
    | String(string)
    | List(list(t));
  exception ConversionException(string);
  let toBool =
    fun
    | Bool(b) => b
    | String(s) =>
      raise(
        ConversionException(
          sprintf("Expected string. Actual string (%s)", s),
        ),
      )
    | List(l) => raise(ConversionException("Expected string. Actual list"));
  let toString =
    fun
    | String(s) => s
    | Bool(b) =>
      raise(
        ConversionException(
          sprintf("Expected string. Actual bool (%s)", string_of_bool(b)),
        ),
      )
    | List(_) => raise(ConversionException("Expected string. Actual list"));
  let toList =
    fun
    | List(l) => l
    | Bool(b) =>
      raise(
        ConversionException(
          sprintf("Expected list. Actual bool (%s)", string_of_bool(b)),
        ),
      )
    | String(_) =>
      raise(ConversionException("Expected list. Actual string"));
};

/* TODO: Making parsing more lenient? Eg. allow string where single element list is valid */
module JSON: {
  type t;
  let ofString: string => t;
  let fromFile: string => t;
  let member: (t, string) => t;
  let toKeyValuePairs: t => list((string, t));
  let toValue: t => FieldTypes.t;
  let debug: t => string;
} = {
  open Yojson.Basic;
  type t = Yojson.Basic.t;
  exception InvalidJSONValue(string);
  exception MissingJSONMember(string);
  let ofString = jstr => from_string(jstr);
  let fromFile = path => from_file(path);
  let member = (j, m) =>
    try (Util.member(m, j)) {
    | _ =>
      raise(
        MissingJSONMember(Printf.sprintf("%s was missing in the json", m)),
      )
    };
  let toKeyValuePairs = (json: Yojson.Basic.t) =>
    switch (json) {
    | `Assoc(jsonKeyValuePairs) => jsonKeyValuePairs
    | _ => raise(InvalidJSONValue("Expected key value pairs"))
    };
  let rec toValue = (json: Yojson.Basic.t) =>
    switch (json) {
    | `Bool(b) => FieldTypes.Bool(b)
    | `String(s) => FieldTypes.String(s)
    | `List(jl) => FieldTypes.List(List.map(j => toValue(j), jl))
    | `Null => raise(NullJSONValue())
    | _ =>
      raise(
        InvalidJSONValue(
          sprintf(
            "Value must be either string, bool or list of string. Found %s",
            to_string(json),
          ),
        ),
      )
    };
  let debug = t => to_string(t);
};

let moduleNameOf = fileName =>
  Str.global_replace(Str.regexp("\\.\\(re\\|ml\\)$"), "", fileName);

let%expect_test _ = {
  print_endline(moduleNameOf("Foo.re"));
  %expect
  {|
     Foo
   |};
};

let%expect_test _ = {
  print_endline(moduleNameOf("Foo.ml"));
  %expect
  {|
     Foo
   |};
};

let isValidBinaryFileName = fileName =>
  Str.string_match(Str.regexp("^.+\\.exe$"), fileName, 0);

let%expect_test _ = {
  print_endline(string_of_bool(isValidBinaryFileName("Foo.re")));
  %expect
  {|
     false
   |};
};

let%expect_test _ = {
  print_endline(string_of_bool(isValidBinaryFileName("Foo.exe")));
  %expect
  {|
     true
   |};
};

let isValidSourceFile = fileName =>
  Str.string_match(Str.regexp("^.+\\.\\(re\\|ml\\)$"), fileName, 0);

let%expect_test _ = {
  print_endline(string_of_bool(isValidSourceFile("Foo.re")));
  %expect
  {|
     true
   |};
};

let%expect_test _ = {
  print_endline(string_of_bool(isValidSourceFile("Foo.ml")));
  %expect
  {|
     true
   |};
};

let%expect_test _ = {
  print_endline(string_of_bool(isValidSourceFile("Foo.mlsss")));
  %expect
  {|
     false
   |};
};

/* Turns "Foo.re as Foo.exe" => ("Foo.re", "Foo.exe") */

type bte =
  | InvalidSourceFilename
  | InvalidBinaryName;

let bte_to_string =
  fun
  | InvalidSourceFilename => "InvalidSourceFilename"
  | InvalidBinaryName => "InvalidBinaryName";

exception BinaryTupleNameError(bte);
let getBinaryNameTuple = namesString => {
  let parts = Str.(split(regexp(" +as +"), namesString));
  switch (parts) {
  | [a] =>
    if (!isValidSourceFile(a)) {
      raise(BinaryTupleNameError(InvalidSourceFilename));
    };
    (a, moduleNameOf(a) ++ ".exe");
  | [a, b, ...r] when List.length(r) == 0 =>
    if (!isValidSourceFile(a)) {
      raise(BinaryTupleNameError(InvalidSourceFilename));
    };
    if (!isValidBinaryFileName(b)) {
      raise(BinaryTupleNameError(InvalidBinaryName));
    };
    (a, b);
  | _ => raise(FatalError("Invalid binary name (syntax)"))
  };
};

let%expect_test _ = {
  let (a, b) = getBinaryNameTuple("foo.re as blah.exe");
  printf("(%s, %s)", a, b);
  %expect
  {|
     (foo.re, blah.exe)
   |};
};

let%expect_test _ = {
  let result =
    try (
      {
        let (a, b) = getBinaryNameTuple("foo as blah.exe");
        sprintf("(%s, %s)", a, b);
      }
    ) {
    | BinaryTupleNameError(x) =>
      sprintf("BinaryTupleNameError(%s)", bte_to_string(x))
    | e => raise(e)
    };
  print_endline(result);
  %expect
  {|
          BinaryTupleNameError(InvalidSourceFilename)
     |};
};

let%expect_test _ = {
  let result =
    try (
      {
        let (a, b) = getBinaryNameTuple("foo.re as blah");
        sprintf("(%s, %s)", a, b);
      }
    ) {
    | BinaryTupleNameError(x) =>
      sprintf("BinaryTupleNameError(%s)", bte_to_string(x))
    | e => raise(e)
    };
  print_endline(result);
  %expect
  {|
          BinaryTupleNameError(InvalidBinaryName)
     |};
};

let%expect_test _ = {
  let (a, b) = getBinaryNameTuple("foo.re");
  printf("(%s, %s)", a, b);
  %expect
  {|
     (foo.re, foo.exe)
     |};
};

let%expect_test _ = {
  let result =
    try (
      {
        let (a, b) = getBinaryNameTuple("foo");
        sprintf("(%s, %s)", a, b);
      }
    ) {
    | BinaryTupleNameError(x) =>
      sprintf("BinaryTupleNameError(%s)", bte_to_string(x))
    | e => raise(e)
    };
  print_endline(result);
  %expect
  {|
          BinaryTupleNameError(InvalidSourceFilename)
     |};
};

/* Turns "foo/bar/baz" => "foo.bar.baz" */
let pathToOCamlLibName = p => Str.global_replace(Str.regexp("/"), ".", p);

let isBinPropertyPresent =
  fun
  | Some(_) => true
  | _ => false;

let isValidScopeName = n => {
  n.[0] == '@';
};

let%expect_test _ = {
  print_endline(string_of_bool(isValidScopeName("blah")));
  %expect
  {|
     false
   |};
};

let%expect_test _ = {
  print_endline(string_of_bool(isValidScopeName("@myscope")));
  %expect
  {|
     true
   |};
};

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

let%expect_test _ = {
  print_endline(doubleKebabifyIfScoped("foo-bar"));
  %expect
  {|
     foo-bar
   |};
};

type t = (string, list(package));
let toPesyConf = (projectPath: string, json: JSON.t): t => {
  let pkgs = JSON.toKeyValuePairs(JSON.member(json, "buildDirs"));

  let rootName =
    /* "name" in root package.json */
    doubleKebabifyIfScoped(
      JSON.member(json, "name") |> JSON.toValue |> FieldTypes.toString,
    );
  /* doubleKebabifyIfScoped turns @myscope/pkgName => myscope--pkgName */

  (
    rootName ++ ".opam",
    List.map(
      pkg => {
        let (dir, conf) = pkg;
        let bin =
          try (
            Some(
              JSON.member(conf, "bin")
              |> JSON.toValue
              |> FieldTypes.toString
              |> getBinaryNameTuple,
            )
          ) {
          | NullJSONValue(_) => None
          | e => raise(e)
          };

        let name_was_guessed = ref(false);
        /* Pesy'name is Dune's public_name */
        let name =
          try (
            JSON.member(conf, "name") |> JSON.toValue |> FieldTypes.toString
          ) {
          | NullJSONValue(_) =>
            name_was_guessed := true;
            switch (bin) {
            | Some((_mainFileName, installedBinaryName)) => installedBinaryName
            | None => rootName ++ "." ++ pathToOCamlLibName(dir)
            };
          | e => raise(e)
          };

        let (<|>) = (f, g, x) => g(f(x));
        let require =
          try (
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
          /* "my-package/lib/here" => "my-package.lib.here" */
          | _ => []
          };

        let flags =
          try (
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
          try (
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
          try (
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
          try (
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
          try (
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
          try (
            Some(
              JSON.member(conf, "includeSubdirs")
              |> JSON.toValue
              |> FieldTypes.toString,
            )
          ) {
          | NullJSONValue(_) => None
          | e => raise(e)
          };

        let rawBuildConfig =
          try (
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
          try (
            Some(
              JSON.member(conf, "rawBuildConfigFooter")
              |> JSON.toValue
              |> FieldTypes.toList
              |> List.map(FieldTypes.toString),
            )
          ) {
          | _ => None
          };

        let nameSpecifiedHasBinSuffix =
          ! name_was_guessed^ && getSuffix(name) == "exe";
        let binPropertyIsPresent = isBinPropertyPresent(bin);

        let isBinary = nameSpecifiedHasBinSuffix || binPropertyIsPresent;
        if (! name_was_guessed^
            && !nameSpecifiedHasBinSuffix
            && binPropertyIsPresent) {
          raise(FatalError("Conflicting values for `bin` property `name`"));
        };

        if (isBinary) {
          /* Prioritising `bin` over `name` */
          let main =
            switch (bin) {
            | Some((mainFileName, _installedBinaryName)) =>
              moduleNameOf(mainFileName)

            | _ =>
              try (
                JSON.member(conf, "main")
                |> JSON.toValue
                |> FieldTypes.toString
              ) {
              | NullJSONValue () =>
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
            try (
              Some(
                Executable.Mode.ofList(
                  JSON.member(conf, "modes")
                  |> JSON.toValue
                  |> FieldTypes.toList
                  |> List.map(a => a |> FieldTypes.toString),
                ),
              )
            ) {
            | NullJSONValue () => None
            | e => raise(e)
            };
          {
            common:
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
              ),
            pkgType: ExecutablePackage(Executable.create(main, modes)),
          };
        } else {
          let namespace =
            try (
              JSON.member(conf, "namespace")
              |> JSON.toValue
              |> FieldTypes.toString
            ) {
            | NullJSONValue () => upperCamelCasify(Filename.basename(dir))
            | e => raise(e)
            };
          let libraryModes =
            try (
              Some(
                JSON.member(conf, "modes")
                |> JSON.toValue
                |> FieldTypes.toList
                |> List.map(FieldTypes.toString)
                |> List.map(Library.Mode.ofString),
              )
            ) {
            | NullJSONValue () => None
            | e => raise(e)
            };
          let cStubs =
            try (
              Some(
                JSON.member(conf, "cNames")
                |> JSON.toValue
                |> FieldTypes.toList
                |> List.map(FieldTypes.toString),
              )
            ) {
            | NullJSONValue () => None
            | e => raise(e)
            };
          let virtualModules =
            try (
              Some(
                JSON.member(conf, "virtualModules")
                |> JSON.toValue
                |> FieldTypes.toList
                |> List.map(FieldTypes.toString),
              )
            ) {
            | NullJSONValue () => None
            | e => raise(e)
            };
          let implements =
            try (
              Some(
                JSON.member(conf, "implements")
                |> JSON.toValue
                |> FieldTypes.toList
                |> List.map(FieldTypes.toString),
              )
            ) {
            | NullJSONValue () => None
            | e => raise(e)
            };
          let wrapped =
            try (
              Some(
                JSON.member(conf, "wrapped")
                |> JSON.toValue
                |> FieldTypes.toBool,
              )
            ) {
            | NullJSONValue () => None
            | e => raise(e)
            };
          {
            common:
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
              ),
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

let toPackages = (_prjPath, pkgs) =>
  List.map(
    pkg =>
      switch (pkg.pkgType) {
      | LibraryPackage(l) => Library.toDuneStanza(pkg.common, l)
      | ExecutablePackage(e) => Executable.toDuneStanza(pkg.common, e)
      },
    pkgs,
  );

let gen = (projectPath, pkgPath) => {
  let json = JSON.fromFile(pkgPath);
  let (rootNameOpamFile, pesyPackages) = toPesyConf(projectPath, json);
  let dunePackages = toPackages(projectPath, pesyPackages); /* TODO: Could return added, updated, deleted files i.e. packages updated so that the main exe could log nicely */
  List.iter(
    dpkg => {
      let (path, duneFile) = dpkg;
      mkdirp(path);
      write(Path.(path / "dune"), duneFile);
    },
    dunePackages,
  );

  let foundAnOpamFile = ref(false);
  let dirForEachEntry = (f, dirname) => {
    let d = Unix.opendir(dirname);
    try (
      while (true) {
        f(Unix.readdir(d));
      }
    ) {
    | End_of_file => Unix.closedir(d)
    };
  };
  let contains = (n, s) =>
    try (Str.search_forward(Str.regexp(s), n, 0) != (-1)) {
    | Not_found => false
    };

  dirForEachEntry(
    n =>
      if (contains(n, ".opam") && ! foundAnOpamFile^) {
        printf("%s is an opam file\n", n);
        foundAnOpamFile := true;
        if (n != rootNameOpamFile) {
          copyFile(
            Path.(projectPath / n),
            Path.(projectPath / rootNameOpamFile),
          );
          Unix.unlink(Path.(projectPath / n));
        };
      },
    projectPath,
  );
};

/* TODO: Figure better test setup */
let testToPackages = jsonStr => {
  let json = JSON.ofString(jsonStr);
  let (_, pesyPackages) = toPesyConf("", json);
  let dunePackages = toPackages("", pesyPackages);
  List.map(
    p => {
      let (_, d) = p;
      d;
    },
    dunePackages,
  );
};

let%expect_test _ = {
  let duneFiles =
    testToPackages(
      {|
  {
    "name": "foo",
    "buildDirs": {
      "test": {
        "require": ["foo"],
        "main": "Bar",
        "name": "Bar.exe"
      }
    }
  }
       |},
    );
  List.iter(print_endline, duneFiles);
  %expect
  {|
     (executable (name Bar) (public_name Bar.exe) (libraries foo))
   |};
};

let%expect_test _ = {
  let duneFiles =
    testToPackages(
      {|
  {
      "name": "foo",
      "buildDirs": {
      "testlib": {
        "require": ["foo"],
        "namespace": "Foo",
        "name": "bar.lib",
       "modes": ["best"]
      }
    }
  }
       |},
    );
  List.iter(print_endline, duneFiles);
  %expect
  {|
     (library (name Foo) (public_name bar.lib) (libraries foo) (modes best))
   |};
};

let%expect_test _ = {
  let duneFiles =
    testToPackages(
      {|
  {
      "name": "foo",
      "buildDirs": {
      "testlib": {
        "require": ["foo"],
        "namespace": "Foo",
        "name": "bar.lib",
        "cNames": ["stubs"]
      }
    }
  }
       |},
    );
  List.iter(print_endline, duneFiles);
  %expect
  {|
     (library (name Foo) (public_name bar.lib) (libraries foo) (c_names stubs))
   |};
};

let%expect_test _ = {
  let duneFiles =
    testToPackages(
      {|
  {
      "name": "foo",
      "buildDirs": {
      "testlib": {
        "namespace": "Foo",
        "name": "bar.lib",
        "virtualModules": ["foo"]
      }
    }
  }
       |},
    );
  List.iter(print_endline, duneFiles);
  %expect
  {|
     (library (name Foo) (public_name bar.lib) (virtual_modules foo))
   |};
};

let%expect_test _ = {
  let duneFiles =
    testToPackages(
      {|
  {
      "name": "foo",
      "buildDirs": {
      "testlib": {
        "namespace": "Foo",
        "name": "bar.lib",
        "implements": ["foo"]
      }
    }
  }
       |},
    );
  List.iter(print_endline, duneFiles);
  %expect
  {|
     (library (name Foo) (public_name bar.lib) (implements foo))
   |};
};

let%expect_test _ = {
  let duneFiles =
    testToPackages(
      {|
  {
      "name": "foo",
      "buildDirs": {
      "testlib": {
        "namespace": "Foo",
        "name": "bar.lib",
        "wrapped": false
      }
    }
  }
       |},
    );
  List.iter(print_endline, duneFiles);
  %expect
  {|
     (library (name Foo) (public_name bar.lib) (wrapped false))
   |};
};

let%expect_test _ = {
  let duneFiles =
    testToPackages(
      {|
  {
      "name": "foo",
      "buildDirs": {
      "testlib": {
        "main": "Foo",
        "name": "bar.exe",
        "modes": ["best", "c"]
      }
    }
  }
       |},
    );
  List.iter(print_endline, duneFiles);
  %expect
  {|
     (executable (name Foo) (public_name bar.exe) (modes (best c)))
   |};
};

let%expect_test _ = {
  let duneFiles =
    testToPackages(
      {|
           {
             "name": "foo",
             "buildDirs": {
               "testlib": {
                 "require": ["foo"],
                 "namespace": "Foo",
                 "name": "bar.lib",
                 "flags": ["-w", "-33+9"]
               }
             }
           }
                |},
    );
  List.iter(print_endline, duneFiles);
  %expect
  {|
     (library (name Foo) (public_name bar.lib) (libraries foo) (flags -w -33+9))
   |};
};

let%expect_test _ = {
  let duneFiles =
    testToPackages(
      {|
           {
             "name": "foo",
             "buildDirs": {
               "testlib": {
                 "namespace": "Foo",
                 "name": "bar.lib",
                 "ocamlcFlags": ["-annot", "-c"]
               }
             }
           }
                |},
    );
  List.iter(print_endline, duneFiles);
  %expect
  {|
     (library (name Foo) (public_name bar.lib) (ocamlc_flags -annot -c))
   |};
};

let%expect_test _ = {
  let duneFiles =
    testToPackages(
      {|
           {
             "name": "foo",
             "buildDirs": {
               "testlib": {
                 "namespace": "Foo",
                 "name": "bar.lib",
                 "ocamloptFlags": ["-rectypes", "-nostdlib"]
               }
             }
           }
                |},
    );
  List.iter(print_endline, duneFiles);
  %expect
  {|
     (library (name Foo) (public_name bar.lib)
         (ocamlopt_flags -rectypes -nostdlib))
   |};
};

let%expect_test _ = {
  let duneFiles =
    testToPackages(
      {|
           {
             "name": "foo",
             "buildDirs": {
               "testlib": {
                 "namespace": "Foo",
                 "name": "bar.lib",
                 "jsooFlags": ["-pretty", "-no-inline"]
               }
             }
           }
                |},
    );
  List.iter(print_endline, duneFiles);
  %expect
  {|
     (library (name Foo) (public_name bar.lib) (js_of_ocaml -pretty -no-inline))
   |};
};

let%expect_test _ = {
  let duneFiles =
    testToPackages(
      {|
           {
             "name": "foo",
             "buildDirs": {
               "testlib": {
                 "namespace": "Foo",
                 "name": "bar.lib",
                 "preprocess": [ "pps", "lwt_ppx" ]
               }
             }
           }
                |},
    );
  List.iter(print_endline, duneFiles);
  %expect
  {|
     (library (name Foo) (public_name bar.lib) (preprocess (pps lwt_ppx)))
   |};
};

let%expect_test _ = {
  let duneFiles =
    testToPackages(
      {|
           {
             "name": "foo",
             "buildDirs": {
               "testlib": {
                 "namespace": "Foo",
                 "name": "bar.lib",
                 "includeSubdirs": "unqualified"
               }
             }
           }
                |},
    );
  List.iter(print_endline, duneFiles);
  %expect
  {|
     (library (name Foo) (public_name bar.lib) (include_subdirs unqualified))
   |};
};

let%expect_test _ = {
  let duneFiles =
    testToPackages(
      {|
           {
             "name": "foo",
             "buildDirs": {
               "testlib": {
                 "namespace": "Foo",
                 "name": "bar.lib",
                 "rawBuildConfig": [
                   "(libraries lwt lwt.unix raw.lib)",
                   "(preprocess (pps lwt_ppx))"
                 ]
               }
             }
           }
                |},
    );
  List.iter(print_endline, duneFiles);
  %expect
  {|
     (library (name Foo) (public_name bar.lib) (libraries lwt lwt.unix raw.lib)
         (preprocess (pps lwt_ppx)))
   |};
};

let%expect_test _ = {
  let duneFiles =
    testToPackages(
      {|
           {
             "name": "foo",
             "buildDirs": {
               "testlib": {
                 "namespace": "Foo",
                 "name": "bar.lib",
                 "rawBuildConfigFooter": [
                   "(install (section share_root) (files (asset.txt as asset.txt)))"
                 ]
               }
             }
           }
                |},
    );
  List.iter(print_endline, duneFiles);
  %expect
  {|
     (library (name Foo) (public_name bar.lib)) (install (section share_root) (files (asset.txt as asset.txt)))
   |};
};

let%expect_test _ = {
  let duneFiles =
    testToPackages(
      {|
           {
             "name": "foo",
             "buildDirs": {
               "testexe": {
                 "main": "Foo",
                 "name": "Foo.exe",
                 "rawBuildConfigFooter": [
                   "(install (section share_root) (files (asset.txt as asset.txt)))"
                 ]
               }
             }
           }
                |},
    );
  List.iter(print_endline, duneFiles);
  %expect
  {|
     (executable (name Foo) (public_name Foo.exe)) (install (section share_root) (files (asset.txt as asset.txt)))
   |};
};
