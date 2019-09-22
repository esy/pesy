module Alias = {
  type t = {
    alias: string,
    library: string,
    originalNamespace: string,
  };
  let create = (~alias, ~library, ~originalNamespace) => {
    {alias, library, originalNamespace};
  };
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

let generateLibraryStanza = pesyModules =>
  switch (pesyModules) {
  | Some(x) =>
    Some(
      Stanza.createExpression([
        Stanza.createAtom("library"),
        Stanza.createExpression([
          Stanza.createAtom("public_name"),
          Stanza.createAtom(x.dunePublicName),
        ]),
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
               |> List.map(Alias.getLibrary)
               |> SS.of_list
               |> SS.elements
               |> List.map(Alias.transform(~f=Stanza.createAtom));
             },
        ]),
      ]),
    )
  | None => None
  };
