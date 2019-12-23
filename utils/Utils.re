open Printf;
let prompt = q => {
  print_endline(q);
  read_line();
};

let kebab = str => {
  let charStrings = Str.split(Str.regexp_string(""), str);
  let k =
    String.concat(
      "",
      List.map(
        c =>
          if (Char.code(c.[0]) >= 65 && Char.code(c.[0]) <= 90) {
            "-" ++ String.lowercase_ascii(c);
          } else {
            c;
          },
        charStrings,
      ),
    );
  if (k.[0] == '-') {
    String.sub(k, 1, String.length(k) - 1);
  } else {
    k;
  };
};

let getCurrentDirName = () => Filename.basename(Sys.getenv("PWD"));

let upperCamelCasify = kebab => {
  let parts = Str.split(Str.regexp_string("-"), kebab);
  let k = String.concat("", List.map(String.capitalize_ascii, parts));
  if (k.[0] == '-') {
    String.sub(k, 1, String.length(k) - 1);
  } else {
    k;
  };
};

let removeScope = kebab =>
  Str.global_replace(Str.regexp("[^\\/]*/"), "", kebab);

module NoLwt = {
  open Printf;
  let write = (file, str) => {
    let oc = open_out(file);
    fprintf(oc, "%s", str);
    close_out(oc);
  };
};

let readFile = file => {
  let buf = ref("");
  let breakOut = ref(false);
  let ic = open_in(file);
  while (! breakOut^) {
    let line =
      try(input_line(ic)) {
      | End_of_file =>
        breakOut := true;
        "";
      };
    buf := buf^ ++ "\n" ++ line;
  };
  buf^;
};

module Path = {
  let (/) = Filename.concat;
};

let parent = Filename.dirname;

let loadTemplate = name =>
  readFile(
    Path.(
      (Sys.executable_name |> parent |> parent)
      / "share"
      / "template-repo"
      / name
    ),
  );

let buffer_size = 8192;
let buffer = Bytes.create(buffer_size);
let copyFile = (input_name, output_name) => {
  open Unix;
  let fd_in = openfile(input_name, [O_RDONLY], 0);
  let fd_out = openfile(output_name, [O_WRONLY, O_CREAT, O_TRUNC], 438);
  let rec copy_loop = () =>
    switch (read(fd_in, buffer, 0, buffer_size)) {
    | 0 => ()
    | r =>
      ignore(write(fd_out, buffer, 0, r));
      copy_loop();
    };

  copy_loop();
  close(fd_in);
  close(fd_out);
};

let copyTemplate = (input_name, output_name) => {
  copyFile(
    Path.(
      (Sys.executable_name |> parent |> parent)
      / "share"
      / "template-repo"
      / input_name
    ),
    output_name,
  );
};

let r = Str.regexp;

let exists = Sys.file_exists;

let mkdir = (~perms=?, p) =>
  switch (perms) {
  | Some(x) => Unix.mkdir(p, x)
  | None => Unix.mkdir(p, 0o755)
  };

let rec mkdirp = p => {
  let directory_created =
    try(Sys.is_directory(p)) {
    | Sys_error(_) => false
    };
  if (!directory_created) {
    mkdirp(Filename.dirname(p));
    mkdir(p);
  };
};

let spf = Printf.sprintf;

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
      let firstChildLog = sprintf("├─ %s", firstChild);
      let restChildrenLog =
        List.map(c => sprintf("│  %s", c), restChildren);
      String.concat("\n", ["│", firstChildLog, ...restChildrenLog]);
    };
let renderAscLastTree =
  fun
  | [] =>
    raise(RenderAsciiTreeChildrenError("Tree cannot have zero children"))
  | [firstChild, ...restChildren] => {
      let firstChildLog = sprintf("└─ %s", firstChild);
      let restChildrenLog = List.map(c => sprintf("   %s", c), restChildren);
      String.concat("\n", ["│", firstChildLog, ...restChildrenLog]);
    };

let rec renderAscTree =
  fun
  | [] => ()
  | [t] => print_endline(renderAscLastTree(t))
  | [t, ...rest] => {
      print_endline(renderAscTreeChildren(t));
      renderAscTree(rest);
    };

let readFileOpt = f =>
  if (exists(f)) {
    Some(readFile(f));
  } else {
    None;
  };

/* module Cache = { */
/*   module CacheInternal = { */
/*     type t = {path: string}; */
/*     let init = path => {path: path}; */
/*   }; */
/*   let init = () => { */
/*     /\* let cacheStoragePath = *\/ */
/*     /\*   Path.((Sys.executable_name |> parent |> parent) / "share" / "cache"); *\/ */
/*   }; */
/*   let get = (cache: CacheInternal.t, key: string) => {}; */
/* }; */

let commandOutput = (command, args) => {
  open Unix;
  let cmd =
    String.concat(" ", Array.to_list(Array.append([|command|], args)));
  let (cout, cin, cerr) = open_process_full(cmd, Unix.environment());
  let repeat = ref(true);
  let stdout = ref("");
  let stderr = ref("");
  while (repeat^) {
    ignore(
      try(stdout := stdout^ ++ input_line(cout)) {
      | End_of_file => repeat := false
      },
    );
  };
  while (repeat^) {
    ignore(
      try(stderr := stderr^ ++ input_line(cerr)) {
      | End_of_file => repeat := false
      },
    );
  };
  switch (close_process_full((cout, cin, cerr))) {
  | WEXITED(c) => (stdout^, stderr^)
  | WSIGNALED(c) => (stdout^, stderr^)
  | WSTOPPED(c) => (stdout^, stderr^)
  };
};

let runCommandWithEnv = (command, args) => {
  /* let attach = */
  /*   Unix.create_process_env( */
  /*     command, */
  /*     Array.append([|command|], args), */
  /*     Unix.environment(), */
  /*   ); */
  /* let pid = attach(Unix.stdin, Unix.stdout, Unix.stderr); */
  /* switch (Unix.waitpid([], pid)) { */
  /* | (_, WEXITED(c)) => c */
  /* | (_, WSIGNALED(c)) => c */
  /* | (_, WSTOPPED(c)) => c */
  /* }; */

  open Unix;
  let cmd =
    String.concat(" ", Array.to_list(Array.append([|command|], args)));
  let (cout, cin, cerr) = open_process_full(cmd, Unix.environment());
  let repeat = ref(true);
  while (repeat^) {
    ignore(
      try({
        let line = input_line(cerr);
        print_endline(Pastel.(<Pastel dim=true> line </Pastel>));
      }) {
      | End_of_file => repeat := false
      },
    );
  };
  while (repeat^) {
    ignore(
      try({
        let line = input_line(cout);
        print_endline(Pastel.(<Pastel dim=true> line </Pastel>));
      }) {
      | End_of_file => repeat := false
      },
    );
  };
  switch (close_process_full((cout, cin, cerr))) {
  | WEXITED(c) => c
  | WSIGNALED(c) => c
  | WSTOPPED(c) => c
  };
};

let filterNone = l => {
  let result = ref([]);
  let rec loop =
    fun
    | [] => ()
    | [h, ...rest] => {
        switch (h) {
        | Some(a) => result := [a, ...result^]
        | None => ()
        };
        loop(rest);
      };
  loop(l);
  List.rev(result^);
};

module JSON = JSON;
module FieldTypes = FieldTypes;
