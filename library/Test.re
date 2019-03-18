open Printf;
open PesyUtils;

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

  let mandatoryExpressions = [name];
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
      Stanza.createAtom("test"),
      ...mandatoryExpressions
         @ filterNone(optionalExpressions)
         @ rawBuildConfig,
    ]);

  (path, [executable, ...rawBuildConfigFooter]);
};
