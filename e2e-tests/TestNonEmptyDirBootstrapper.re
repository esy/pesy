open Utils;
open Printf;

let parent = Filename.dirname;

let tmpDir = Filename.get_temp_dir_name();
let testProject = "test-project";
let testProjectDir = Filename.concat(tmpDir, testProject);
let pesyBinPath = makeCommand("pesy");

rimraf(testProjectDir); /* So that we can run it stateless locally */
mkdir(testProjectDir);
Sys.chdir(testProjectDir);

let exitStatus = runCommandWithEnv(pesyBinPath, [||]);
print_endline("status: " ++ string_of_int(exitStatus));