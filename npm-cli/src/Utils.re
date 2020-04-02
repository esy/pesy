let (<<) = (f, g, x) => f(g(x));
module Caml = {
  module List = List;
  module String = String;
};

module Path = {
  let (/) = (x, y) => Bindings.Path.resolve([|x, y|]);
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

module Result = {
  open Belt.Result;

  let (>>=) = flatMap;

  let (>>|) = map;

  let return = x => Ok(x);

  let fail = x => Error(x);
};

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

  let catch = (o, f) =>
    switch (o) {
    | None => f()
    | _ => ()
    };

  let return = x => Some(x);
};
module ResultPromise: {
  type t('a, 'b) = Js.Promise.t(result('a, 'b));
  let (>>=): (t('a, 'b), 'a => t('c, 'b)) => t('c, 'b);
  let (>>|): (t('a, 'b), 'a => 'c) => t('c, 'b);
  let ok: 'a => t('a, 'b);
  let fail: 'b => t('a, 'b);
} = {
  type t('a, 'b) = Js.Promise.t(result('a, 'b));
  let (>>=) = (rp, f) => {
    rp
    |> Js.Promise.then_(
         fun
         | Ok(x) => f(x)
         | Error(msg) => Js.Promise.resolve(Error(msg)),
       );
  };
  let (>>|) = (rp, f) => {
    rp
    |> Js.Promise.then_(
         fun
         | Error(msg) => Js.Promise.resolve(Error(msg))
         | Ok(x) => f(x) |> Result.return |> Js.Promise.resolve,
       );
  };
  let ok = x => x |> Result.return |> Js.Promise.resolve; // Js.Promise.resolve << Result.return;
  let fail = x => x |> Result.fail |> Js.Promise.resolve;
  /**
     Js.Promise.resolve << Result.fail;
     let fail: '_weak1 => Js.Promise.t(Belt.Result.t('a, '_weak1))
     is not included in let fail: 'b => t('a, 'b)
   */;
};
