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
  name: string,
  public: bool,
  namespace: string,
  modes: option(list(Mode.t)),
  ffi: option(Stubs.t),
  virtualModules: option(list(string)),
  implements: option(list(string)),
  wrapped: option(bool),
};
let create =
    (name, public, namespace, modes, ffi, virtualModules, implements, wrapped) => {
  name,
  public,
  namespace,
  modes,
  ffi,
  virtualModules,
  implements,
  wrapped,
};
let toDuneStanza = (common, lib) => {
  /* let {name: pkgName, require, path} = common */
  let {
    name,
    public,
    namespace,
    modes: modesP,
    ffi: stubsP,
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
  let public_name = if (public) { Stanza.createAtom(name) |> Stanza.create("public_name") |> Option.some } else { None }; // pesy's name is Dune's public_name
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

  let stubsD = Stubs.toDuneStanza(stubsP);

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

  let mandatoryExpressions = [name, modules];
  let optionalExpressions = [
    public_name,
    libraries,
    modesD,
    virtualModulesD,
    implementsD,
    wrappedD,
    flags,
    ocamlcFlags,
    ocamloptFlags,
    jsooFlags,
    preprocess,
    ...stubsD,
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
         @ rawBuildConfig,
    ]);

  (path, [library, ...optionalRootStanzas]);
};
