open Utils;

let ($) = (f, x) => f(x);

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

module Path = {
  let (/) = Filename.concat;
};

let parent = Filename.dirname;

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

let set_infos = (filename, infos) => {
  open Unix;
  utimes(filename, infos.st_atime, infos.st_mtime);
  chmod(filename, infos.st_perm);
  try (chown(filename, infos.st_uid, infos.st_gid)) {
  | Unix_error(EPERM, _, _) => ()
  };
};

let iter_dir = (f, d) => Sys.readdir(d) |> Array.to_list |> List.iter(f);

let rec copy_rec = (source, dest) => {
  open Unix;
  let infos = lstat(source);
  switch (infos.st_kind) {
  | S_REG => file_copy(source, dest)
  | S_LNK =>
    let link = readlink(source);
    symlink(link, dest);
  | S_DIR =>
    mkdir(dest, 0o755);
    iter_dir(
      file =>
        if (file != Filename.current_dir_name
            && file != Filename.parent_dir_name) {
          copy_rec(
            Filename.concat(source, file),
            Filename.concat(dest, file),
          );
        },
      source,
    );
  | _ => prerr_endline("Can't cope with special file " ++ source)
  };
};

let tmpDir = Filename.get_temp_dir_name();
let testProject = "test-project";
let testProjectDir = Filename.concat(tmpDir, testProject);
let pesyConfigureCommand = "pesy";

let mkdir = (~perms=?, p) =>
  switch (perms) {
  | Some(x) => Unix.mkdir(p, x)
  | None => Unix.mkdir(p, 0o755)
  };

let tmpDir = Filename.get_temp_dir_name();
let testDirName = "pesy-configure-test-projects";
let testDir = Path.(tmpDir / testDirName);

Printf.printf("Deleting %s", testDir);
print_newline();
rimraf(testDir);

let pesyConfigureTestProjectsDir =
  Path.(Sys.getcwd() / "e2e-tests" / testDirName);

Printf.printf("Copying %s to %s", pesyConfigureTestProjectsDir, testDir);
print_newline();
copy_rec(pesyConfigureTestProjectsDir, testDir);

/* Use resolutions so that e2e tests point to local clone */
List.iter(
  pkgJSONPath => {
    let curContents = IO.read(pkgJSONPath);
    IO.write(
      pkgJSONPath,
      Str.global_replace(
        Str.regexp("<RESOLUTION_LINK>"),
        "link:" ++ Sys.getenv("PESY_CLONE_PATH"),
        curContents,
      ),
    );
  },
  fetch_files(x => contains(x, "package.json") != (-1), testDir),
);

let testProjects =
  Sys.readdir(testDir)
  |> Array.to_list
  |> List.filter(dir =>
       try (Sys.is_directory(Path.(testDir / dir))) {
       | Sys_error(e) => false
       }
     )
  |> List.map(d => Path.(testDir / d));

List.iter(
  testProject => {
    Printf.printf("Entering %s", testProject);
    print_newline();
    Sys.chdir(testProject);

    /* So that we can run it stateless locally */
    rimraf(Path.(testProject / "esy.lock"));
    rimraf(Path.(testProject / "node_modules"));
    rimraf(Path.(testProject / "_esy"));

    Printf.printf("Running `esy install`");
    print_newline();
    let exitStatus = runCommandWithEnv(esyCommand, [|"install"|]);
    if (exitStatus != 0) {
      Printf.fprintf(
        stderr,
        "Test failed: Non zero exit when running esy install",
      );
      exit(-1);
    };

    Printf.printf("Running `esy pesy`");
    print_newline();
    let exitStatus = runCommandWithEnv(esyCommand, [|"#{pesy.root}/_build/default/bin/Pesy.exe"|]);
    if (exitStatus != 0) {
      Printf.fprintf(
        stderr,
        "Test failed: Non zero exit when running esy pesy",
      );
      exit(-1);
    };

    Printf.printf("Running `esy build`");
    print_newline();
    let exitStatus = runCommandWithEnv(esyCommand, [|"build"|]);
    if (exitStatus != 0) {
      Printf.fprintf(
        stderr,
        "Test failed: Non zero exit when running esy build",
      );
      exit(-1);
    };

    Printf.printf("Running `esy test`");
    print_newline();
    let exitStatus = runCommandWithEnv(esyCommand, [|"test"|]);
    if (exitStatus != 0) {
      Printf.fprintf(
        stderr,
        "Test failed: Non zero exit when running esy test",
      );
      exit(-1);
    };
  },
  testProjects,
);
