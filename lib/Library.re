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
    pesyModulesLibrary,
    pesyModulesAliasModuleGen,
  ) =
    Common.toDuneStanzas(common);
  let path = Common.getPath(common);
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
    );

  let library =
    Stanza.createExpression([
      Stanza.createAtom("library"),
      ...mandatoryExpressions
         @ filterNone(optionalExpressions)
         @ rawBuildConfig,
    ]);

  (path, [library, ...optionalRootStanzas]);
};
