module Utils = PesyEsyPesyUtils.Utils;
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
    | Atom(BinaryKind.t)
    | Tuple(Compilation.t, BinaryKind.t)
    | Array(list(t));
  exception InvalidExecutableMode(string);
  let rec ofFieldTypes = parts =>
    switch (parts) {
    | FieldTypes.String(bs) =>
      Array(
        bs
        |> Str.split(Str.regexp("[ \n\r\x0c\t]+"))
        |> List.map(b => Atom(BinaryKind.ofString(b))),
      )
    | FieldTypes.List([FieldTypes.String(c), FieldTypes.String(b)]) =>
      try(Tuple(Compilation.ofString(c), BinaryKind.ofString(b))) {
      | InvalidCompilationMode () =>
        Array([
          Atom(BinaryKind.ofString(c)),
          Atom(BinaryKind.ofString(b)),
        ])
      | _ =>
        raise(
          InvalidExecutableMode(
            "Invalid executable mode: expected of the form [(<compilation mode>, <binary_kind>)'s | <binary_kind>'s] or <binary_kind>'s",
          ),
        )
      }
    | FieldTypes.List(xs) => Array(List.map(ofFieldTypes, xs))
    | _ =>
      raise(
        InvalidExecutableMode(
          "Invalid executable mode: expected of the form [(<compilation mode>, <binary_kind>)'s | <binary_kind>'s] or <binary_kind>'s",
        ),
      )
    };
  let rec toStanzas = m =>
    switch (m) {
    | Array(x) => List.fold_left((acc, s) => acc @ toStanzas(s), [], x)
    | Tuple(c, b) => [
        Stanza.createExpression([
          Stanza.createAtom(Compilation.toString(c)),
          Stanza.createAtom(BinaryKind.toString(b)),
        ]),
      ]
    | Atom(b) => [Stanza.createAtom(BinaryKind.toString(b))]
    };
};
type t = {
  static: option(bool),
  binKVs: list((string, string)),
  modes: option(Mode.t),
};
let create = (binKVs, static, modes) => {binKVs, static, modes};
let toDuneStanza = (common: Common.t, e) => {
  /* let {name: pkgName, require, path} = common; */
  let {binKVs, static, modes: modesP} = e;
  let (
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

  /* TODO: hacky!
        ocamlopt flags are not lists. They are s-expresions.
        Each item in the list can a single item or another list of items.
        And so on..
        TODO: remodel some of the fields correctly as s-expressions
     */

  let ocamloptFlags =
    switch (static) {
    | Some(static) when static =>
      Sexplib.Sexp.(
        switch (ocamloptFlags) {
        | None =>
          Some(
            List([
              Atom("ocamlopt_flags"),
              List([Atom("-ccopt"), Atom("-static")]),
            ])
            |> Stanza.ofSexp,
          )
        | Some(flags) =>
          switch (Stanza.toSexp(flags)) {
          | List(l) =>
            switch (l) {
            | [Atom("ocamlopt_flags"), ...rest] =>
              Some(
                List([
                  Atom("ocamlopt_flags"),
                  ...(rest @ [List([Atom("-ccopt"), Atom("-static")])]),
                ])
                |> Stanza.ofSexp,
              )
            | [] =>
              Some(
                List([
                  Atom("ocamlopt_flags"),
                  List([Atom("-ccopt"), Atom("-static")]),
                ])
                |> Stanza.ofSexp,
              )
            | _ => failwith("Invalid s-exp obtained for ocamlopt_flags")
            }
          | _ =>
            Some(
              List([
                Atom("ocamlopt_flags"),
                List([Atom("-ccopt"), Atom("-static")]),
              ])
              |> Stanza.ofSexp,
            )
          }
        }
      )
    | Some(_) => ocamloptFlags
    | None => ocamloptFlags
    };

  let path = Common.getPath(common);
  let (mains, publicNames) =
    List.fold_right(
      (tuple, acc) => {
        let (main, publicName) = tuple;
        let (mains, publicNames) = acc;
        (mains @ [main], publicNames @ [publicName]);
      },
      binKVs,
      ([], []),
    );
  let name =
    Stanza.createExpression([
      Stanza.createAtom("names"),
      ...mains |> List.map(x => x |> moduleNameOf |> Stanza.createAtom),
    ]);
  let public_name =
    Stanza.createExpression([
      Stanza.createAtom("public_names"),
      ...publicNames |> List.map(Stanza.createAtom),
    ]);

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
          ...Mode.toStanzas(m),
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
      Stanza.createAtom("executables"),
      ...mandatoryExpressions
         @ filterNone(optionalExpressions)
         @ rawBuildConfig,
    ]);

  (path, [executable, ...optionalRootStanzas]);
};
