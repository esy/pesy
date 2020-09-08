module Utils = PesyEsyPesyUtils.Utils;
open Utils;

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

module ForeignStub = {
  exception LanguageNotSupported(string);
  module Language = {
    type t =
      | C
      | CXX;
    let ofString =
      fun
      | "c" => C
      | "cxx" => CXX
      | x => raise(LanguageNotSupported(x));
    let toString =
      fun
      | C => "c"
      | CXX => "cxx";
  };
  type t = {
    language: Language.t,
    names: option(list(string)),
    flags: option(list(string)),
  };
  let ofFieldTypes = tfts => {
    /* Note: here language is set C as default value it will be replaced by the
       correct value in the below pattern match */
    let fs = ref({language: Language.C, names: None, flags: None});
    List.iter(
      tft => {
        switch (tft) {
        | ("names", ft) =>
          fs :=
            {
              ...fs^,
              names:
                switch (FieldTypes.toList(ft)) {
                | [] => None
                | l => Some(List.map(FieldTypes.toString, l))
                },
            }
        | ("flags", ft) =>
          fs :=
            {
              ...fs^,
              flags:
                switch (FieldTypes.toList(ft)) {
                | [] => None
                | l => Some(List.map(FieldTypes.toString, l))
                },
            }
        | ("language", ft) =>
          fs :=
            {...fs^, language: Language.ofString(FieldTypes.toString(ft))}
        | _ => ()
        }
      },
      tfts,
    );
    fs^;
  };
  let toDuneStanza = fs => {
    let stanzas = [];
    stanzas
    @ [
      Stanza.createExpression([
        Stanza.createAtom("language"),
        Stanza.createAtom(Language.toString(fs.language)),
      ]),
    ]
    @ (
      switch (fs.names) {
      | None
      | Some([]) => [
          Stanza.createExpression([
            Stanza.createAtom("names"),
            Stanza.createAtom(":standard"),
          ]),
        ]
      | Some(xs) => [
          Stanza.createExpression([
            Stanza.createAtom("names"),
            ...List.map(Stanza.createAtom, xs),
          ]),
        ]
      }
    )
    @ (
      switch (fs.flags) {
      | None
      | Some([]) => [
          Stanza.createExpression([
            Stanza.createAtom("flags"),
            Stanza.createAtom(":standard"),
          ]),
        ]
      | Some(xs) => [
          Stanza.createExpression([
            Stanza.createAtom("flags"),
            ...List.map(Stanza.createAtom, xs),
          ]),
        ]
      }
    );
  };
};
type t = {
  name: string,
  namespace: string,
  modes: option(list(Mode.t)),
  cNames: option(list(string)),
  foreignStubs: option(list(list((string, FieldTypes.t)))),
  virtualModules: option(list(string)),
  implements: option(list(string)),
  wrapped: option(bool),
};
let create =
    (
      name,
      namespace,
      modes,
      cNames,
      foreignStubs,
      virtualModules,
      implements,
      wrapped,
    ) => {
  name,
  namespace,
  modes,
  cNames,
  foreignStubs,
  virtualModules,
  implements,
  wrapped,
};
let toDuneStanza = (common, lib) => {
  /* let {name: pkgName, require, path} = common */
  let {
    name,
    namespace,
    modes: modesP,
    cNames: cNamesP,
    foreignStubs: foreignStubsP,
    virtualModules: virtualModulesP,
    implements: implementsP,
    wrapped: wrappedP,
  } = lib;
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
  let path = Common.getPath(common);
  let public_name = Stanza.create("public_name", Stanza.createAtom(name)); // pesy's name is Dune's public_name
  let name = Stanza.create("name", Stanza.createAtom(namespace));
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

  let foreignStubsD =
    switch (foreignStubsP) {
    | None => [None]
    | Some(fss) =>
      fss
      |> List.map(ForeignStub.ofFieldTypes)
      |> List.map(fs =>
           Some(
             Stanza.createExpression([
               Stanza.createAtom("foreign_stubs"),
               ...ForeignStub.toDuneStanza(fs),
             ]),
           )
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

  let mandatoryExpressions = [name, public_name, modules];
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

  let library =
    Stanza.createExpression([
      Stanza.createAtom("library"),
      ...mandatoryExpressions
         @ filterNone(optionalExpressions)
         @ filterNone(foreignStubsD)
         @ rawBuildConfig,
    ]);

  (path, [library, ...optionalRootStanzas]);
};