module Utils = PesyEsyPesyUtils.Utils;
open Printf;
open Utils;

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
      | Shared_object
      | JS
      | Plugin;

    let toString =
      fun
      | C => "c"
      | Exe => "exe"
      | Object => "object"
      | Shared_object => "shared_object"
      | JS => "js"
      | Plugin => "plugin";

    let ofString =
      fun
      | "c" => C
      | "exe" => Exe
      | "object" => Object
      | "shared_object" => Shared_object
      | "js" => JS
      | "plugin" => Plugin
      | _ => raise(InvalidBinaryKind());
  };

  type t =
    | BinaryKind(BinaryKind.t)
    | Tuple(Compilation.t, BinaryKind.t)
    | Array(list(t));
  exception InvalidExecutableMode(string);
  let rec ofFieldTypes = parts =>
    switch (parts) {
    | FieldTypes.String(bs) =>
      bs
      |> Str.split(Str.regexp("[ \n\r\x0c\t]+"))
      |> List.map(b => BinaryKind(BinaryKind.ofString(b)))
      |> (bs => Array(bs))
    | FieldTypes.List([FieldTypes.String(c), FieldTypes.String(b)]) =>
      try(Tuple(Compilation.ofString(c), BinaryKind.ofString(b))) {
      | InvalidCompilationMode () =>
        Array([
          BinaryKind(BinaryKind.ofString(c)),
          BinaryKind(BinaryKind.ofString(b)),
        ])
      | e => raise(e)
      }
    | FieldTypes.List([hd, ...tl]) =>
      Array([ofFieldTypes(hd), ...List.map(ofFieldTypes, tl)])
    | _ =>
      raise(
        InvalidExecutableMode(
          sprintf(
            /* TODO: meaningful error message */
            "Invalid executable mode: expected of the form (<compilation mode>, <binary_kind>)",
          ),
        ),
      )
    };
  let rec toStanza = m =>
    switch (m) {
    | Array(x) =>
      x
      |> List.map(toStanza)
      |> (
        xs =>
          switch (xs) {
          | [h] => h
          | [_, ..._] => Stanza.createExpression(xs)
          | _ => raise(InvalidExecutableMode("Invalid executable mode: "))
          }
      )
    | Tuple(c, b) =>
      Stanza.createExpression([
        Stanza.createAtom(Compilation.toString(c)),
        Stanza.createAtom(BinaryKind.toString(b)),
      ])
    | BinaryKind(b) => Stanza.createAtom(BinaryKind.toString(b))
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
    pesyModulesLibrary,
    pesyModulesAliasModuleGen,
  ) =
    Common.toDuneStanzas(common);
  let path = Common.getPath(common);
  /* Pesy's main is Dune's name */
  let name = Stanza.create("name", Stanza.createAtom(main));
  let modules =
    Stanza.createExpression([
      Stanza.createAtom("modules"),
      Stanza.createExpression(
        [Stanza.createAtom(":standard")]
        @ (
          switch (Common.getPesyModules(common)) {
          | Some(x) => [
              Stanza.createAtom("\\"),
              Stanza.createAtom(PesyModule.getNamespace(x)),
            ]
          | None => []
          }
        ),
      ),
    ]);
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
          Mode.toStanza(m),
        ]),
      )
    };

  let mandatoryExpressions = [name, modules, public_name];
  let optionalExpressions = [
    libraries,
    modesD,
    flags,
    ocamlcFlags,
    ocamloptFlags,
    jsooFlags,
    preprocess,
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

  let optionalRootStanzas =
    rawBuildConfigFooter
    @ (
      switch (pesyModulesLibrary) {
      | Some(s) => [s]
      | None => []
      }
    )
    @ (
      switch (pesyModulesAliasModuleGen) {
      | Some(s) => [s]
      | None => []
      }
    )
    @ (
      switch (includeSubdirs) {
      | Some(s) => [s]
      | None => []
      }
    );

  let executable =
    Stanza.createExpression([
      Stanza.createAtom("executable"),
      ...mandatoryExpressions
         @ filterNone(optionalExpressions)
         @ rawBuildConfig,
    ]);

  (path, [executable, ...optionalRootStanzas]);
};