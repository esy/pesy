module Alias = {
  type t = {
    alias: string,
    library: string,
    internal: bool, // if an alias is for a local sub-library
    originalNamespace: string,
    exportedNamespace: string,
  };
  let create = (~alias, ~internal, ~library, ~originalNamespace, ~exportedNamespace) => {
    {alias, library, internal, originalNamespace, exportedNamespace};
  };
  let isInternal = x => x.internal;
  let getOriginalNamespace = x => x.originalNamespace;
  let getLibrary = x => x.library;
  let transform = (~f, x) => f(x);
  let toReStatement = a => a.alias;
};

type t' = {
  namespace: string,
  dunePublicName: string,
  aliases: list(Alias.t),
};

type t = option(t');

let getNamespace = pm => pm.namespace;
let getDunePublicName = pm => pm.dunePublicName;
let create:
  (~namespace: string, ~dunePublicName: string, ~aliases: list(Alias.t)) => t =
  (~namespace, ~dunePublicName, ~aliases) => {
    List.length(aliases) == 0
      ? None : Some({namespace, dunePublicName, aliases});
  };

let generateAliasModuleStanza = pesyModules =>
  switch (pesyModules) {
  | Some(x) =>
    let pesyModulesReFile =
      x.aliases |> List.map(Alias.toReStatement) |> String.concat("\n");
    Some(
      Stanza.createExpression([
        Stanza.createAtom("rule"),
        Stanza.createExpression([
          Stanza.createAtom("with-stdout-to"),
          Stanza.createAtom(Printf.sprintf("%s.re", x.namespace)),
          Stanza.createExpression([
            Stanza.createAtom("run"),
            Stanza.createAtom("echo"),
            Stanza.createAtom(Printf.sprintf("%s", pesyModulesReFile)),
          ]),
        ]),
      ]),
    );
  | None => None
  };

let generateLibraryStanza = (preprocess, pesyModules) => {
  let preprocessStanza =
    switch (preprocess) {
    | None => []
    | Some(l) => [
        Stanza.createExpression([
          Stanza.createAtom("preprocess"),
          Stanza.createExpression(List.map(f => Stanza.createAtom(f), l)),
        ]),
      ]
    };

  switch (pesyModules) {
  | Some(x) =>
    Some(
      Stanza.createExpression([
        Stanza.createAtom("library"),
        Stanza.createExpression([
          Stanza.createAtom("name"),
          Stanza.createAtom(x.namespace),
        ]),
        Stanza.createExpression([
          Stanza.createAtom("modules"),
          Stanza.createAtom(x.namespace),
        ]),
        Stanza.createExpression([
          Stanza.createAtom("libraries"),
          ...{
               module SS = Set.Make(String);
               x.aliases
               |> List.map(alias => Alias.isInternal(alias) ? Alias.getOriginalNamespace(alias): Alias.getLibrary(alias))
               |> SS.of_list
               |> SS.elements
               |> List.map(Alias.transform(~f=Stanza.createAtom));
             },
        ]),
        ...preprocessStanza,
      ]),
    )
  | None => None
  };
};
