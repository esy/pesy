module Utils = PesyEsyPesyUtils.Utils;
open Utils;

type t =
  | CNames(list(string))
  | ForeignStubs(list(list((string, FieldTypes.t))));

let ofCNames = cns => CNames(cns);

let ofForeignStubs = fss => ForeignStubs(fss);

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

let cNamesD = cNamesP => [
  Some(
    Stanza.createExpression([
      Stanza.createAtom("c_names"),
      ...List.map(Stanza.createAtom, cNamesP),
    ]),
  ),
];
let foreignStubsD = foreignStubsP =>
  List.map(ForeignStub.ofFieldTypes, foreignStubsP)
  |> List.map(fs =>
       Some(
         Stanza.createExpression([
           Stanza.createAtom("foreign_stubs"),
           ...ForeignStub.toDuneStanza(fs),
         ]),
       )
     );

let toDuneStanza = stubsP =>
  switch (stubsP) {
  | Some(s) =>
    switch (s) {
    | CNames(cNamesP) => cNamesD(cNamesP)
    | ForeignStubs(foreignStubsP) => foreignStubsD(foreignStubsP)
    }
  | None => [None]
  };