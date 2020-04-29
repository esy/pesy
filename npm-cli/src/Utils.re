let (<<) = (f, g, x) => f(g(x));
module Caml = {
  module List = List;
  module String = String;
};

let spf = Printf.sprintf;

let parent = Filename.dirname;

/* let copyTemplate = (a, b) => { */
/*   write(b, readFile(Path.(scriptPath / "share" / "template-repo" / a))); */
/* }; */

let kebab = str => {
  let charStrings = Js.String.split("", str);
  let k =
    Js.Array.joinWith(
      "",
      Array.map(
        c => {
          let c = Caml.String.get(c, 0);
          if (c == ' ' || c == '_') {
            String.make(1, '-');
          } else if (Char.uppercase_ascii(c) != Char.lowercase_ascii(c)
                     && Char.uppercase_ascii(c) == c) {
            "-" ++ String.make(1, Char.lowercase_ascii(c));
          } else {
            String.make(1, c);
          };
        },
        charStrings,
      ),
    )
    |> Js.String.replaceByRe([%bs.re "/\\-\\-+/g"], "-");

  if (Js.String.split("", k)->Array.unsafe_get(0) == "-") {
    Caml.String.sub(k, 1, String.length(k) - 1);
  } else {
    k;
  };
};

let removeScope = kebab =>
  Js.String.replaceByRe([%bs.re "/[^\\/]*\\//g"], "", kebab);

let upperCamelCasify = kebab => {
  let parts = Js.String.split("-", kebab);
  let k = Js.Array.joinWith("", Array.map(String.capitalize_ascii, parts));
  if (Caml.String.get(k, 0) == '-') {
    Caml.String.sub(k, 1, String.length(k) - 1);
  } else {
    k;
  };
};

/* let loadTemplate = name => */
/*   readFile(Path.(scriptPath / "share" / "template-repo" / name)); */
/* let rec mkdirp = p => { */
/*   let directory_created = exists(p); */
/*   if (!directory_created) { */
/*     mkdirp(Filename.dirname(p)); */
/*     makedirSync(p); */
/*   }; */
/* }; */

let renderAsciiTree = (dir, name, namespace, require, isLast) =>
  if (isLast) {
    spf({js|│
└─ %s
   %s
   %s
|js}, dir, name, namespace);
  } else {
    Printf.sprintf(
      {js|│
├─ %s
│  %s
│  %s
|js},
      dir,
      name,
      namespace,
    )
    ++ (require == "" ? "" : (isLast ? "   " : "│  ") ++ require);
  };

/* Capable of rendering a tree of depth 1 */
/* exception RenderAsciiTreeChildrenError(string); */
/* let renderAscTreeChildren = */
/*   fun */
/*   | [] => */
/*     raise(RenderAsciiTreeChildrenError("Tree cannot have zero children")) */
/*   | [firstChild, ...restChildren] => { */
/*       let firstChildLog = spf("├─ %s", firstChild); */
/*       let restChildrenLog = */
/*         Caml.List.map(c => spf("│  %s", c), restChildren); */
/*       String.joinWith("\n", ["│", firstChildLog, ...restChildrenLog]); */
/*     }; */
/* let renderAscLastTree = */
/*   fun */
/*   | [] => */
/*     raise(RenderAsciiTreeChildrenError("Tree cannot have zero children")) */
/*   | [firstChild, ...restChildren] => { */
/*       let firstChildLog = spf("└─ %s", firstChild); */
/*       let restChildrenLog = */
/*         Caml.List.map(c => spf("   %s", c), restChildren); */
/*       String.join(~sep="\n", ["│", firstChildLog, ...restChildrenLog]); */
/*     }; */

/* let rec renderAscTree = */
/*   fun */
/*   | [] => () */
/*   | [t] => print_endline(renderAscLastTree(t)) */
/*   | [t, ...rest] => { */
/*       Js.log(renderAscTreeChildren(t)); */
/*       renderAscTree(rest); */
/*     }; */

module Option: {
  type t('a) = option('a);
  let (>>=): (t('a), 'a => t('b)) => t('b);
  let (>>|): (t('a), 'a => 'b) => t('b);
  let return: 'a => t('a);
} = {
  open Belt.Option;

  type t('a) = option('a);

  let (>>=) = flatMap;

  let (>>|) = map;

  let return = x => Some(x);
};

let rec askYesNoQuestion = (~question, ~onYes, ~onNo, ()) => {
  let rl =
    Readline.createInterface({
      "input": [%raw "process.stdin"],
      "output": [%raw "process.stdout"],
    });
  rl##question(
    question ++ " [y/n] ",
    input => {
      let response = input->Js.String.trim->Js.String.toLowerCase;
      switch (response) {
      | "y"
      | "yes" =>
        onYes();
        rl##close();
      | "n"
      | "no" =>
        onNo();
        rl##close();
      | _ => askYesNoQuestion(~question, ~onYes, ~onNo, ())
      };
    },
  );
};

let jsExnGuard = f =>
  try(f()) {
  | Js.Exn.Error(e) =>
    let message =
      switch (Js.Exn.message(e)) {
      | Some(message) => message
      | None => ""
      };
    let stack =
      switch (Js.Exn.stack(e)) {
      | Some(stack) => stack
      | None => ""
      };
    Js.log(message);
    Js.log(stack);
    Js.Promise.resolve();
  };
