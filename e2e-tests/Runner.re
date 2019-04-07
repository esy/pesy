open Unix;
open Utils;
open Printf;

let verdaccioCommand = makeCommand("verdaccio");
let cwd = Sys.getcwd();

rimraf(Path.(cwd / "scripts" / "storage"));
rimraf(Path.(cwd / "scripts" / "htpasswd"));
let default_npm_registry = "https://registry.npmjs.org";

let verdaccio_log = Filename.temp_file("verdaccio-", "-log");
let verdaccio_err = Filename.temp_file("verdaccio-", "-err");

let cin = Unix.stdin;
printf("Opening %s\n", verdaccio_log);
let cout = Unix.stdout; /* openfile(verdaccio_log, [O_RDWR], 0o766); */
printf("Opening %s\n", verdaccio_err);
let cerr = Unix.stderr; /* openfile(verdaccio_err, [O_RDWR], 0o766); */

printf(
  "Command: %s %s %s\n",
  verdaccioCommand,
  "-c",
  Path.(cwd / "scripts" / "verdaccio.yaml"),
);

print_endline("Starting verdaccio");
let pid =
  create_process(
    verdaccioCommand,
    [|verdaccioCommand, "-c", Path.(cwd / "scripts" / "verdaccio.yaml")|],
    cin,
    cout,
    cerr,
  );

printf("PID: %d\n", pid);

let read = buffer => {
  let buffer_size = 8192;
  let buffer = Bytes.create(buffer_size);
  if (read(cout, buffer, buffer_size, 0) != 0) {
    fprintf(Pervasives.stderr, "Read operation failed. Exiting");
    exit(-1);
  };
  Bytes.to_string(buffer);
};

/* let output_buffer_read_so_far = ref(""); */
/* let error_buffer_read_so_far = ref(""); */
/* let match_string = "http address - http://localhost:4873/"; */
/* while (! */
/*          Str.string_match( */
/*            Str.regexp(match_string), */
/*            output_buffer_read_so_far^, */
/*            0, */
/*          ) */
/*        && ! */
/*             Str.string_match( */
/*               Str.regexp(match_string), */
/*               error_buffer_read_so_far^, */
/*               0, */
/*             )) { */
/*   output_buffer_read_so_far := output_buffer_read_so_far^ ++ read(cout); */
/*   error_buffer_read_so_far := error_buffer_read_so_far^ ++ read(cerr); */
/* }; */

sleep(3);

let run = (~env=?, c, args) => {
  let env_vars =
    switch (env) {
    | Some(v) => v
    | None => Unix.environment()
    };
  let pid =
    create_process_env(
      c,
      Array.append([|c|], args),
      env_vars,
      stdin,
      stdout,
      stderr,
    );
  switch (Unix.waitpid([], pid)) {
  | (_, WEXITED(c)) => c
  | (_, WSIGNALED(c)) => c
  | (_, WSTOPPED(c)) => c
  };
};

let custom_registry_url = "http://localhost:4873";

print_endline("Creating npm release");
run(makeCommand("esy"), [|"npm-release"|]);

chdir(Path.(cwd / "_release"));

run(
  makeCommand("npx"),
  [|
    "npm-auth-to-token@1.0.0",
    "-u",
    "user",
    "-p",
    "password",
    "-e",
    "user@example.com",
    "-r",
    custom_registry_url,
  |],
);

print_endline("Publishing to local npm");
run(makeCommand("npm"), [|"publish", "--registry", custom_registry_url|]);
chdir(cwd);

print_endline("Pointing npm registry to local url " ++ custom_registry_url);
run(
  makeCommand("npm"),
  [|"config", "set", "registry", custom_registry_url|],
);

print_endline("Installing packed pesy globally..");
run(
  makeCommand("npm"),
  [|"install", "-g", "pesy@0.5.0-alpha.2", "--unsafe-perm=true"|],
);

let testBootstrapperExe =
  Path.(
    cwd / "_build" / "install" / "default" / "bin" / "TestBootstrapper.exe"
  );
run(
  testBootstrapperExe,
  [||],
  ~env=
    Array.append(
      [|sprintf("NPM_CONFIG_REGISTRY=%s", custom_registry_url)|],
      Unix.environment(),
    ),
);

let testPesyConfigureExe =
  Path.(
    cwd / "_build" / "install" / "default" / "bin" / "TestPesyConfigure.exe"
  );
run(
  testPesyConfigureExe,
  [||],
  ~env=
    Array.append(
      [|sprintf("NPM_CONFIG_REGISTRY=%s", custom_registry_url)|],
      Unix.environment(),
    ),
);

Unix.kill(pid, Sys.sigkill);
