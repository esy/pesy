open Bindings;
module Caml = {
  module List = List;
  module String = String;
};

module Path = {
  let (/) = Filename.concat;
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
          if (Char.uppercase_ascii(c) == c) {
            "-" ++ String.make(1, c);
          } else {
            String.make(1, c);
          };
        },
        charStrings,
      ),
    );
  if (Js.String.split("", k)->Array.unsafe_get(0) == "-") {
    Caml.String.sub(k, 1, String.length(k) - 1);
  } else {
    k;
  };
};

let removeScope = kebab =>
  Js.String.replaceByRe([%bs.re "/[^\\\/]*\//g"], "", kebab);

let upperCamelCasify = kebab => {
  let parts = Js.String.split("-", kebab);
  let k = Js.Array.joinWith("", Array.map(String.uppercase_ascii, parts));
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
