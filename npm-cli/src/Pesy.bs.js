// Generated by BUCKLESCRIPT VERSION 5.0.4, PLEASE EDIT WITH CARE
'use strict';

var Fs = require("fs");
var Block = require("bs-platform/lib/js/block.js");
var Curry = require("bs-platform/lib/js/curry.js");
var Printf = require("bs-platform/lib/js/printf.js");
var Process = require("process");
var Caml_obj = require("bs-platform/lib/js/caml_obj.js");
var Filename = require("bs-platform/lib/js/filename.js");
var WalkSync = require("walk-sync");
var Belt_Array = require("bs-platform/lib/js/belt_Array.js");
var Belt_Option = require("bs-platform/lib/js/belt_Option.js");
var Caml_option = require("bs-platform/lib/js/caml_option.js");
var Child_process = require("child_process");
var DownloadGitRepo = require("download-git-repo");
var Utils$PesyBootstrapper = require("./Utils.bs.js");
var Spinner$PesyBootstrapper = require("./Spinner.bs.js");

var projectPath = Process.cwd();

var packageNameKebab = Utils$PesyBootstrapper.kebab(Curry._1(Filename.basename, projectPath));

var packageNameKebabSansScope = Utils$PesyBootstrapper.removeScope(packageNameKebab);

var packageNameUpperCamelCase = Utils$PesyBootstrapper.upperCamelCasify(packageNameKebabSansScope);

var version = "0.0.0";

var packageLibName = packageNameKebabSansScope + "/library";

var packageTestName = packageNameKebabSansScope + "/test";

function substituteTemplateValues(s) {
  return s.replace((/<PACKAGE_NAME_FULL>/g), packageNameKebab).replace((/<VERSION>/g), version).replace((/<PUBLIC_LIB_NAME>/g), packageLibName).replace((/<TEST_LIB_NAME>/g), packageTestName).replace((/<PACKAGE_NAME>/g), packageNameKebab).replace((/<PACKAGE_NAME_UPPER_CAMEL>/g), packageNameUpperCamelCase);
}

function stripTemplateExtension(s) {
  return s.replace("-template", "");
}

function substituteFileNames(s) {
  return s.replace((/__PACKAGE_NAME_FULL__/g), packageNameKebab).replace((/__PACKAGE_NAME__/g), packageNameKebab).replace((/__PACKAGE_NAME_UPPER_CAMEL__/g), packageNameUpperCamelCase);
}

var template = Belt_Option.getWithDefault(Belt_Option.map(Belt_Array.get(Belt_Array.keep(Process.argv, (function (param) {
                    return param.includes("--template");
                  })), 0), (function (param) {
            return param.replace("--template=", "");
          })), "github:esy/pesy-reason-template");

var download_spinner = Spinner$PesyBootstrapper.start("\x1b[2mDownloading template\x1b[0m " + template);

DownloadGitRepo(template, projectPath, (function (error) {
        Spinner$PesyBootstrapper.stop(download_spinner);
        if (error !== undefined) {
          console.log(Caml_option.valFromOption(error));
          return /* () */0;
        } else {
          var setup_files_spinner = Spinner$PesyBootstrapper.start("\x1b[2mSetting up template\x1b[0m " + template);
          var files = Belt_Array.keep(WalkSync(projectPath), (function (file_or_dir) {
                  return Fs.statSync(file_or_dir).isFile();
                }));
          Belt_Array.forEach(files, (function (file) {
                  Utils$PesyBootstrapper.write(file, substituteTemplateValues(Utils$PesyBootstrapper.readFile(file)));
                  Fs.renameSync(file, substituteFileNames(file).replace("-template", ""));
                  return /* () */0;
                }));
          Spinner$PesyBootstrapper.stop(setup_files_spinner);
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

exports.projectPath = projectPath;
exports.packageNameKebab = packageNameKebab;
exports.packageNameKebabSansScope = packageNameKebabSansScope;
exports.packageNameUpperCamelCase = packageNameUpperCamelCase;
exports.version = version;
exports.packageLibName = packageLibName;
exports.packageTestName = packageTestName;
exports.substituteTemplateValues = substituteTemplateValues;
exports.stripTemplateExtension = stripTemplateExtension;
exports.substituteFileNames = substituteFileNames;
exports.template = template;
exports.download_spinner = download_spinner;
/* projectPath Not a pure module */
