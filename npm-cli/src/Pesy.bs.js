// Generated by BUCKLESCRIPT VERSION 5.0.4, PLEASE EDIT WITH CARE
'use strict';

var Fs = require("fs");
var Path = require("path");
var Block = require("bs-platform/lib/js/block.js");
var Curry = require("bs-platform/lib/js/curry.js");
var Js_exn = require("bs-platform/lib/js/js_exn.js");
var Printf = require("bs-platform/lib/js/printf.js");
var Process = require("process");
var Caml_obj = require("bs-platform/lib/js/caml_obj.js");
var Cmdliner = require("@elliottcable/bs-cmdliner/src/cmdliner.bs.js");
var Readline = require("readline");
var WalkSync = require("walk-sync");
var Belt_Array = require("bs-platform/lib/js/belt_Array.js");
var Pervasives = require("bs-platform/lib/js/pervasives.js");
var Caml_option = require("bs-platform/lib/js/caml_option.js");
var Child_process = require("child_process");
var DownloadGitRepo = require("download-git-repo");
var Caml_js_exceptions = require("bs-platform/lib/js/caml_js_exceptions.js");
var Utils$PesyBootstrapper = require("./Utils.bs.js");
var Spinner$PesyBootstrapper = require("./Spinner.bs.js");

((process.argv.shift()));

var projectPath = Process.cwd();

var packageNameKebab = Utils$PesyBootstrapper.kebab(Path.basename(projectPath));

var packageNameKebabSansScope = Utils$PesyBootstrapper.removeScope(packageNameKebab);

var packageNameUpperCamelCase = Utils$PesyBootstrapper.upperCamelCasify(packageNameKebabSansScope);

var templateVersion = "0.0.0";

var packageLibName = packageNameKebabSansScope + "/library";

var packageTestName = packageNameKebabSansScope + "/test";

function substituteTemplateValues(s) {
  return s.replace((/<PACKAGE_NAME_FULL>/g), packageNameKebab).replace((/<VERSION>/g), templateVersion).replace((/<PUBLIC_LIB_NAME>/g), packageLibName).replace((/<TEST_LIB_NAME>/g), packageTestName).replace((/<PACKAGE_NAME>/g), packageNameKebab).replace((/<PACKAGE_NAME_UPPER_CAMEL>/g), packageNameUpperCamelCase);
}

function stripTemplateExtension(s) {
  return s.replace("-template", "");
}

function substituteFileNames(s) {
  return s.replace((/__PACKAGE_NAME_FULL__/g), packageNameKebab).replace((/__PACKAGE_NAME__/g), packageNameKebab).replace((/__PACKAGE_NAME_UPPER_CAMEL__/g), packageNameUpperCamelCase);
}

function isDirectoryEmpty(path) {
  return Fs.readdirSync(path).length === 0;
}

function downloadTemplate(template) {
  var spinner = Spinner$PesyBootstrapper.start("\x1b[2mDownloading template\x1b[0m " + template);
  DownloadGitRepo(template, projectPath, (function (error) {
          Spinner$PesyBootstrapper.stop(spinner);
          if (error !== undefined) {
            console.log(Caml_option.valFromOption(error));
            return /* () */0;
          } else {
            var setup_files_spinner = Spinner$PesyBootstrapper.start("\x1b[2mSetting up template\x1b[0m " + template);
            try {
              var files = Belt_Array.keep(WalkSync(projectPath), (function (file_or_dir) {
                      return Fs.statSync(file_or_dir).isFile();
                    }));
              Belt_Array.forEach(files, (function (file) {
                      Utils$PesyBootstrapper.write(file, substituteTemplateValues(Utils$PesyBootstrapper.readFile(file)));
                      Fs.renameSync(file, substituteFileNames(file).replace("-template", ""));
                      return /* () */0;
                    }));
              Spinner$PesyBootstrapper.stop(setup_files_spinner);
            }
            catch (raw_exn){
              var exn = Caml_js_exceptions.internalToOCamlException(raw_exn);
              if (exn[0] === Js_exn.$$Error) {
                Spinner$PesyBootstrapper.stop(setup_files_spinner);
                var match = exn[1].stack;
                if (match !== undefined) {
                  console.log("\x1b[31mPesy encountered an error\x1b[0m");
                  console.log(match);
                } else {
                  console.log("\x1b[31mPesy encountered an unknown error as it was trying to setup templates\x1b[0m");
                }
                Pervasives.exit(-1);
              } else {
                throw exn;
              }
            }
            var id = Spinner$PesyBootstrapper.start("\x1b[2mRunning\x1b[0m esy install");
            Child_process.exec("esy i", (function (e, stdout, stderr) {
                    Spinner$PesyBootstrapper.stop(id);
                    if (Caml_obj.caml_equal_nullable(e, null)) {
                      var id$1 = Spinner$PesyBootstrapper.start("\x1b[2mRunning\x1b[0m esy pesy\x1b[2m and \x1b[0m building project dependencies");
                      Child_process.exec("esy pesy", (function (e, stdout, stderr) {
                              Spinner$PesyBootstrapper.stop(id$1);
                              if (Caml_obj.caml_equal_nullable(e, null)) {
                                var id$2 = Spinner$PesyBootstrapper.start("\x1b[2mRunning\x1b[0m esy build");
                                Child_process.exec("esy build", (function (e, stdout, stderr) {
                                        Spinner$PesyBootstrapper.stop(id$2);
                                        if (Caml_obj.caml_equal_nullable(e, null)) {
                                          return Printf.printf(/* Format */[
                                                      /* String_literal */Block.__(11, [
                                                          "You may now run \x1b[32m'esy test'\x1b[0m\n",
                                                          /* End_of_format */0
                                                        ]),
                                                      "You may now run \x1b[32m'esy test'\x1b[0m\n"
                                                    ]);
                                        } else {
                                          Printf.printf(/* Format */[
                                                /* String_literal */Block.__(11, [
                                                    "'esy build' \x1b[31mfailed.\x1b[0m Could not build project.\nLogs can be found in pesy.stdout.log and pesy.stderr.log\n",
                                                    /* End_of_format */0
                                                  ]),
                                                "'esy build' \x1b[31mfailed.\x1b[0m Could not build project.\nLogs can be found in pesy.stdout.log and pesy.stderr.log\n"
                                              ]);
                                          Utils$PesyBootstrapper.write("pesy.stdout.log", stdout);
                                          return Utils$PesyBootstrapper.write("pesy.stderr.log", stderr);
                                        }
                                      }));
                                return /* () */0;
                              } else {
                                Printf.printf(/* Format */[
                                      /* String_literal */Block.__(11, [
                                          "'esy pesy' \x1b[31mfailed.\x1b[0m Dune files could not be created.\n Logs can be found in pesy.stdout.log and pesy.stderr.log\n",
                                          /* End_of_format */0
                                        ]),
                                      "'esy pesy' \x1b[31mfailed.\x1b[0m Dune files could not be created.\n Logs can be found in pesy.stdout.log and pesy.stderr.log\n"
                                    ]);
                                Utils$PesyBootstrapper.write("pesy.stdout.log", stdout);
                                return Utils$PesyBootstrapper.write("pesy.stderr.log", stderr);
                              }
                            }));
                      return /* () */0;
                    } else {
                      Printf.printf(/* Format */[
                            /* String_literal */Block.__(11, [
                                "'esy install' \x1b[31mfailed.\x1b[0m Dependencies could not be installed.\nLogs can be found in pesy.stdout.log and pesy.stderr.log\n",
                                /* End_of_format */0
                              ]),
                            "'esy install' \x1b[31mfailed.\x1b[0m Dependencies could not be installed.\nLogs can be found in pesy.stdout.log and pesy.stderr.log\n"
                          ]);
                      Utils$PesyBootstrapper.write("pesy.stdout.log", stdout);
                      return Utils$PesyBootstrapper.write("pesy.stderr.log", stderr);
                    }
                  }));
            return /* () */0;
          }
        }));
  return /* () */0;
}

function askYesNoQuestion(rl, question, onYes, $staropt$star, param) {
  var onNo = $staropt$star !== undefined ? $staropt$star : (function (param) {
        return /* () */0;
      });
  return rl.question(question + " [y/n] ", (function (input) {
                var response = input.trim().toLowerCase();
                switch (response) {
                  case "n" : 
                  case "no" : 
                      Curry._1(onNo, /* () */0);
                      return rl.close();
                  case "y" : 
                  case "yes" : 
                      Curry._1(onYes, /* () */0);
                      return rl.close();
                  default:
                    return askYesNoQuestion(rl, question, onYes, onNo, /* () */0);
                }
              }));
}

function main(template, useDefaultOptions) {
  if (isDirectoryEmpty(projectPath) || useDefaultOptions) {
    return downloadTemplate(template);
  } else {
    var rl = Readline.createInterface({
          input: (process.stdin),
          output: (process.stdout)
        });
    return askYesNoQuestion(rl, "Current directory is not empty. Are you sure to continue?", (function (param) {
                  return downloadTemplate(template);
                }), undefined, /* () */0);
  }
}

var version = "0.5.0-alpha.9";

var partial_arg = Cmdliner.Arg[/* string */32];

var partial_arg$1 = Cmdliner.Arg[/* opt */14];

var template = Cmdliner.Arg[/* & */9](Cmdliner.Arg[/* value */20], Cmdliner.Arg[/* & */9]((function (param) {
            return partial_arg$1(undefined, partial_arg, "github:esy/pesy-reason-template#86b37d16dcfe15", param);
          }), Cmdliner.Arg[/* info */8](undefined, "TEMPLATE_URL", "Specify URL of the remote template. This can be of hthe form https://repo-url.git#<commit|branch|tag>. Eg: https://github.com/reason-native-web/morph-hello-world-pesy-template#6e5cbbb9f28", undefined, /* :: */[
              "t",
              /* :: */[
                "template",
                /* [] */0
              ]
            ])));

var useDefaultOptions = Cmdliner.Arg[/* & */9](Cmdliner.Arg[/* value */20], Cmdliner.Arg[/* & */9](Cmdliner.Arg[/* flag */10], Cmdliner.Arg[/* info */8](undefined, undefined, "Use default options", undefined, /* :: */[
              "y",
              /* :: */[
                "yes",
                /* [] */0
              ]
            ])));

var cmdT = Cmdliner.Term[/* $ */3](Cmdliner.Term[/* $ */3](Cmdliner.Term[/* const */0](main), template), useDefaultOptions);

var cmd_001 = Curry.app(Cmdliner.Term[/* info */14], [
      undefined,
      undefined,
      /* [] */0,
      /* [] */0,
      undefined,
      undefined,
      "Your Esy Assistant.",
      version,
      "pesy"
    ]);

var cmd = /* tuple */[
  cmdT,
  cmd_001
];

Cmdliner.Term[/* eval */16](undefined, undefined, undefined, undefined, undefined, cmd);

exports.projectPath = projectPath;
exports.packageNameKebab = packageNameKebab;
exports.packageNameKebabSansScope = packageNameKebabSansScope;
exports.packageNameUpperCamelCase = packageNameUpperCamelCase;
exports.templateVersion = templateVersion;
exports.packageLibName = packageLibName;
exports.packageTestName = packageTestName;
exports.substituteTemplateValues = substituteTemplateValues;
exports.stripTemplateExtension = stripTemplateExtension;
exports.substituteFileNames = substituteFileNames;
exports.isDirectoryEmpty = isDirectoryEmpty;
exports.downloadTemplate = downloadTemplate;
exports.askYesNoQuestion = askYesNoQuestion;
exports.main = main;
exports.version = version;
exports.template = template;
exports.useDefaultOptions = useDefaultOptions;
exports.cmd = cmd;
/*  Not a pure module */
