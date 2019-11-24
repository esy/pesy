open Bindings;
open Utils;

let projectPath = cwd();
let packageNameKebab = kebab(Filename.basename(projectPath));
let packageNameKebabSansScope = removeScope(packageNameKebab);
let packageNameUpperCamelCase = upperCamelCasify(packageNameKebabSansScope);
let version = "0.0.0";
let packageJSONTemplate = loadTemplate("pesy-package.template.json");
let appReTemplate = loadTemplate("pesy-App.template.re");
let testReTemplate = loadTemplate("pesy-Test.template.re");
let runTestsTemplate = loadTemplate("pesy-RunTests.template.re");
let testFrameworkTemplate = loadTemplate("pesy-TestFramework.template.re");
let utilRe = loadTemplate("pesy-Util.template.re");
let readMeTemplate = loadTemplate("pesy-README.template.md");
let gitignoreTemplate = loadTemplate("pesy-gitignore.template");
let esyBuildStepsTemplate =
  loadTemplate(
    Path.("azure-pipeline-templates" / "pesy-esy-build-steps.template.yml"),
  );
let packageLibName = packageNameKebabSansScope ++ "/library";
let packageTestName = packageNameKebabSansScope ++ "/test";

let substituteTemplateValues = s =>
  s
  |> Js.String.replaceByRe(
       [%bs.re "/<PACKAGE_NAME_FULL>/g"],
       packageNameKebab,
     )
  |> Js.String.replaceByRe([%bs.re "/<VERSION>/g"], version)
  |> Js.String.replaceByRe([%bs.re "/<PUBLIC_LIB_NAME>/g"], packageLibName)
  |> Js.String.replaceByRe([%bs.re "/<TEST_LIB_NAME>/g"], packageTestName)
  |> Js.String.replaceByRe([%bs.re "/<PACKAGE_NAME>/g"], packageNameKebab)
  |> Js.String.replaceByRe(
       [%bs.re "/<PACKAGE_NAME_UPPER_CAMEL>/g"],
       packageNameUpperCamelCase,
     );

if (!exists("package.json")) {
  let packageJSON = packageJSONTemplate |> substituteTemplateValues;
  write("package.json", packageJSON);
};

let appReDir = Path.(projectPath / "executable");
let appRePath = Path.(appReDir / packageNameUpperCamelCase ++ "App.re");

if (!exists(appRePath)) {
  let appRe = appReTemplate |> substituteTemplateValues;
  mkdirp(appReDir);
  write(appRePath, appRe);
};

let utilReDir = Path.(projectPath / "library");
let utilRePath = Path.(utilReDir / "Util.re");

if (!exists(utilRePath)) {
  mkdirp(utilReDir);
  write(utilRePath, utilRe);
};

let testReDir = Path.(projectPath / "test");

if (!exists(testReDir)) {
  let _ = mkdirp(testReDir);
  ();
};

let testRePath =
  Path.(testReDir / "Test" ++ packageNameUpperCamelCase ++ ".re");

if (!exists(testRePath)) {
  let testRe = testReTemplate |> substituteTemplateValues;
  write(testRePath, testRe);
};

let testFrameworkPath = Path.(testReDir / "TestFramework.re");
if (!exists(testFrameworkPath)) {
  let testFramework = testFrameworkTemplate |> substituteTemplateValues;
  write(testFrameworkPath, testFramework);
};

let testExeDir = Path.(projectPath / "testExe");
if (!exists(testExeDir)) {
  let _ = mkdirp(testExeDir);
  ();
};

let testExeFileName =
  "Run<PACKAGE_NAME_UPPER_CAMEL>Tests.re" |> substituteTemplateValues;
let runTestsPath = Path.(testExeDir / testExeFileName);
if (!exists(runTestsPath)) {
  let runTests = runTestsTemplate |> substituteTemplateValues;

  write(runTestsPath, runTests);
};

let readMePath = Path.(projectPath / "README.md");

if (!exists(readMePath)) {
  let readMe = readMeTemplate |> substituteTemplateValues;
  write(readMePath, readMe);
};

let gitignorePath = Path.(projectPath / ".gitignore");

if (!exists(gitignorePath)) {
  let gitignore = gitignoreTemplate |> substituteTemplateValues;
  write(".gitignore", gitignore);
};

let azurePipelinesPath = Path.(projectPath / "azure-pipelines.yml");

if (!exists(azurePipelinesPath)) {
  let esyBuildSteps = esyBuildStepsTemplate |> substituteTemplateValues;
  copyTemplate(
    Path.("azure-pipeline-templates" / "pesy-azure-pipelines.yml"),
    Path.(projectPath / "azure-pipelines.yml"),
  );
  mkdirp(".ci");
  let ciFilesPath = Path.(projectPath / ".ci");
  write(Path.(ciFilesPath / "esy-build-steps.yml"), esyBuildSteps);
  copyTemplate(
    Path.("azure-pipeline-templates" / "pesy-publish-build-cache.yml"),
    Path.(ciFilesPath / "publish-build-cache.yml"),
  );
  copyTemplate(
    Path.("azure-pipeline-templates" / "pesy-restore-build-cache.yml"),
    Path.(ciFilesPath / "restore-build-cache.yml"),
  );
};

let libKebab = packageNameKebabSansScope;
let duneProjectFile = Path.(projectPath / "dune-project");
if (!exists(duneProjectFile)) {
  write(duneProjectFile, spf({|(lang dune 1.2)
(name %s)
|}, libKebab));
};

let opamFileName = libKebab ++ ".opam";
let opamFile = Path.(projectPath / opamFileName);
if (!exists(opamFile)) {
  write(opamFile, "");
};

let rootDuneFile = Path.(projectPath / "dune");

if (!exists(rootDuneFile)) {
  write(rootDuneFile, "(ignored_subdirs (node_modules))");
};

/* TODO: Box drawing characters are transform to weird char by bucklescript. Investigate */
/* Printf.printf("\x1b[1m%s@%s\x1b[0m\n", packageNameKebabSansScope, version); */
/* Utils.renderAscTree([ */
/*   [ */
/*     "executable", */
/*     spf( */
/*       "bin:    %sApp.re as %sApp.exe", */
/*       packageNameUpperCamelCase, */
/*       packageNameUpperCamelCase, */
/*     ), */
/*     spf( */
/*       "require: [%s]", */
/*       List.fold_left( */
/*         (acc, e) => acc ++ " " ++ e, */
/*         "", */
/*         [spf("\"%s\"", packageLibName)], */
/*       ), */
/*     ), */
/*   ], */
/*   ["library", "require: []"], */
/*   [ */
/*     "test", */
/*     spf( */
/*       "bin:    Test%s.re as Test%s.exe", */
/*       packageNameUpperCamelCase, */
/*       packageNameUpperCamelCase, */
/*     ), */
/*     spf( */
/*       "require: [%s]", */
/*       String.concat(", ", [spf("\"%s\"", packageLibName)]), */
/*     ), */
/*   ], */
/* ]); */
/* print_newline(); */

let id = Spinner.start("\x1b[2mRunning\x1b[0m esy install");
exec("esy i", (e, stdout, stderr) => {
  Spinner.stop(id);
  if (Js.eqNullable(e, Js.Nullable.null)) {
    let id = Spinner.start("\x1b[2mRunning\x1b[0m esy pesy");
    exec("esy pesy", (e, stdout, stderr) => {
      Spinner.stop(id);
      if (Js.eqNullable(e, Js.Nullable.null)) {
        let id = Spinner.start("\x1b[2mRunning\x1b[0m esy build");
        exec("esy build", (e, stdout, stderr) => {
          Spinner.stop(id);
          if (Js.eqNullable(e, Js.Nullable.null)) {
            Printf.printf("You may now run \x1b[32m'esy test'\x1b[0m\n");
          } else {
            Printf.printf(
              "'esy build' \x1b[31mfailed.\x1b[0m Could not build project.\nLogs can be found in pesy.stdout.log and pesy.stderr.log\n",
            );
            write("pesy.stdout.log", stdout);
            write("pesy.stderr.log", stderr);
          };
        });
      } else {
        Printf.printf(
          "'esy pesy' \x1b[31mfailed.\x1b[0m Dune files could not be created.\n Logs can be found in pesy.stdout.log and pesy.stderr.log\n",
        );
        write("pesy.stdout.log", stdout);
        write("pesy.stderr.log", stderr);
      };
    });
  } else {
    Printf.printf(
      "'esy install' \x1b[31mfailed.\x1b[0m Dependencies could not be installed.\nLogs can be found in pesy.stdout.log and pesy.stderr.log\n",
    );
    write("pesy.stdout.log", stdout);
    write("pesy.stderr.log", stderr);
  };
});
