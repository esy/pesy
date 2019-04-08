open Utils;
open Printf;

let parent = Filename.dirname;

let tmpDir = Filename.get_temp_dir_name();
let testProject = "test-project";
let testProjectDir = Filename.concat(tmpDir, testProject);
let pesyBinPath = "pesy";

rimraf(testProjectDir); /* So that we can run it stateless locally */
mkdir(testProjectDir);
Sys.chdir(testProjectDir);

let exitStatus = runCommandWithEnv(pesyBinPath, [||]);
if (exitStatus != 0) {
  Printf.fprintf(
    stderr,
    "Test failed: Non zero exit when running bootstrapper",
  );
  exit(-1);
};

let exitStatus =
  runCommandWithEnv(esyCommand, [|"x", "TestProjectApp.exe"|]);

if (exitStatus != 0) {
  Printf.fprintf(
    stderr,
    "Test failed: Non zero exit when running TestProjectApp.exe\n Code: %d",
    exitStatus,
  );
  exit(-1);
};

let exitStatus = runCommandWithEnv(esyCommand, [|"b", "dune", "runtest"|]);

if (exitStatus != 0) {
  Printf.fprintf(
    stderr,
    "Test failed: Non zero exit when running 'esy b dune runtest'\n Code: %d\n",
    exitStatus,
  );
  exit(-1);
};
