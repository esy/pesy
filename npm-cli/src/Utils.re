open Bindings;
module Caml = {
  module List = List;
  module String = String;
};

open Tablecloth;

let spf = Printf.sprintf;

let exists = Node.Fs.existsSync;

let write = (file, str) => {
  Node.Fs.writeFileSync(file, str, `utf8);
};

let readFile = file => {
  Node.Fs.readFileSync(file, `utf8);
};

let parent = Filename.dirname;

let copyTemplate = (a, b) => {
  write(
    b,
    readFile(Path.((scriptPath |> parent) / "share" / "template-repo" / a)),
  );
};

let kebab = str => {
  let charStrings = String.split(~on="", str);
  let k =
    String.concat(
      List.map(
        ~f=
          c => {
            let c = Caml.String.get(c, 0);
            if (Char.isUppercase(c)) {
              "-" ++ Char.toString(Char.toLowercase(c));
            } else {
              Char.toString(c);
            };
          },
        charStrings,
      ),
    );
  if (k |> String.toList |> Caml.List.hd == '-') {
    Caml.String.sub(k, 1, String.length(k) - 1);
  } else {
    k;
  };
};

let removeScope = kebab =>
  Js.String.replaceByRe([%bs.re "/[^\\\/]*\//g"], "", kebab);

let upperCamelCasify = kebab => {
  let parts = String.split(~on="-", kebab);
  let k = Caml.String.concat("", List.map(~f=String.capitalize, parts));
  if (Caml.String.get(k, 0) == '-') {
    Caml.String.sub(k, 1, String.length(k) - 1);
  } else {
    k;
  };
};

let loadTemplate = name =>
  readFile(Path.((scriptPath |> parent) / "share" / "template-repo" / name));
let rec mkdirp = p => {
  let directory_created = exists(p);
  if (!directory_created) {
    mkdirp(Filename.dirname(p));
    makedirSync(p);
  };
};

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
exception RenderAsciiTreeChildrenError(string);
let renderAscTreeChildren =
  fun
  | [] =>
    raise(RenderAsciiTreeChildrenError("Tree cannot have zero children"))
  | [firstChild, ...restChildren] => {
      let firstChildLog = spf("├─ %s", firstChild);
      let restChildrenLog =
        Caml.List.map(c => spf("│  %s", c), restChildren);
      String.join(~sep="\n", ["│", firstChildLog, ...restChildrenLog]);
    };
let renderAscLastTree =
  fun
  | [] =>
    raise(RenderAsciiTreeChildrenError("Tree cannot have zero children"))
  | [firstChild, ...restChildren] => {
      let firstChildLog = spf("└─ %s", firstChild);
      let restChildrenLog =
        Caml.List.map(c => spf("   %s", c), restChildren);
      String.join(~sep="\n", ["│", firstChildLog, ...restChildrenLog]);
    };

let rec renderAscTree =
  fun
  | [] => ()
  | [t] => print_endline(renderAscLastTree(t))
  | [t, ...rest] => {
      Js.log(renderAscTreeChildren(t));
      renderAscTree(rest);
    };
