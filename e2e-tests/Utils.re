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
             | [k, v, ...rest] => Some((k, v))
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

let esyCommand = makeCommand("esy");

let rimraf = s =>
  switch (Bos.OS.Dir.delete(~recurse=true, Fpath.v(s))) {
  | Ok () => ()
  | _ => Printf.fprintf(stdout, "Warning: Could not delete %s\n", s)
  };

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
