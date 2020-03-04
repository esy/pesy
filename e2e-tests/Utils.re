open Printf;

module Path = {
  let (/) = Filename.concat;
};

let makeCommand = cmd =>
  Sys.unix
    ? cmd
    : {
      let pathVars =
        Array.to_list(Unix.environment())
        |> List.map(e =>
             switch (Str.split(Str.regexp("="), e)) {
             | [k, v, ..._rest] => Some((k, v))
             | _ => None
             }
           )
        |> List.filter(
             fun
             | None => false
             | _ => true,
           )
        |> List.filter(e =>
             switch (e) {
             | Some((k, _)) => String.lowercase_ascii(k) == "path"
             | _ => false
             }
           )
        |> List.map(
             fun
             | Some(x) => x
             | None => ("", "") /* Why not filter_map? */
           );

      let v =
        List.fold_right(
          (e, acc) => {
            let (_, v) = e;
            acc ++ (Sys.unix ? ":" : ";") ++ v;
          },
          pathVars,
          "",
        );

      let paths = Str.split(Str.regexp(Sys.unix ? ":" : ";"), v);
      let npmPaths =
        List.filter(
          path =>
            Sys.file_exists(Filename.concat(path, sprintf("%s.cmd", cmd))),
          paths,
        );
      switch (npmPaths) {
      | [] =>
        fprintf(stderr, "No %s bin path found", cmd);
        exit(-1);
      | [h, ..._] => Filename.concat(h, sprintf("%s.cmd", cmd))
      };
    };

let rimraf = p => ignore(Rimraf.run(p));
let esyCommand = makeCommand("esy");

let buffer_size = 8192;
let buffer = Bytes.create(buffer_size);

let runCommandWithEnv = (command, args) => {
  let attach =
    Unix.create_process_env(
      command,
      Array.append([|command|], args),
      Unix.environment(),
    );
  let pid = attach(Unix.stdin, Unix.stdout, Unix.stderr);
  switch (Unix.waitpid([], pid)) {
  | (_, WEXITED(c)) => c
  | (_, WSIGNALED(c)) => c
  | (_, WSTOPPED(c)) => c
  };
};

let mkdir = (~perms=?, p) =>
  switch (perms) {
  | Some(x) => Unix.mkdir(p, x)
  | None => Unix.mkdir(p, 0o755)
  };

let file_copy = (input_name, output_name) => {
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

module IO = {
  let write = (file, str) => {
    let oc = open_out(file);
    fprintf(oc, "%s", str);
    close_out(oc);
  };

  let read = file => {
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
};

let contains = (s1, s2) => {
  let re = Str.regexp_string(s2);
  try(Str.search_forward(re, s1, 0)) {
  | Not_found => (-1)
  };
};

let rec fetch_files = (f, p) => {
  let rec walk = (dh, acc) => {
    let read_dir = dh =>
      try(Some(Unix.readdir(dh))) {
      | End_of_file => None
      };

    switch (read_dir(dh)) {
    | Some("..")
    | Some(".") => walk(dh, acc)
    | Some(entry) =>
      walk(dh, [Path.(p / entry), ...acc])
      @ fetch_files(f, Path.(p / entry))
    | None =>
      Unix.closedir(dh);
      acc;
    };
  };

  if (try(Sys.is_directory(p)) {
      | _ => false
      }) {
    let dh = Unix.opendir(p);
    List.filter(f, walk(dh, []));
  } else {
    [];
  };
};
