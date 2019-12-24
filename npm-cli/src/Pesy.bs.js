// Generated by BUCKLESCRIPT, PLEASE EDIT WITH CARE
'use strict';

var Fs = require('fs');
var Arg = require('bs-platform/lib/js/arg.js');
var Path = require('path');
var Curry = require('bs-platform/lib/js/curry.js');
var Js_exn = require('bs-platform/lib/js/js_exn.js');
var Process = require('process');
var Readline = require('readline');
var WalkSync = require('walk-sync');
var DownloadGitRepo = require('download-git-repo');
var Caml_js_exceptions = require('bs-platform/lib/js/caml_js_exceptions.js');
var SourceMapSupport = require('source-map-support');
var Caml_chrome_debugger = require('bs-platform/lib/js/caml_chrome_debugger.js');
var Utils$PesyBootstrapper = require('./Utils.bs.js');
var Spinner$PesyBootstrapper = require('./Spinner.bs.js');
var Bindings$PesyBootstrapper = require('./Bindings.bs.js');

SourceMapSupport.install();

var projectPath = Process.cwd();

var packageNameKebab = Utils$PesyBootstrapper.kebab(Path.basename(projectPath));

var packageNameKebabSansScope = Utils$PesyBootstrapper.removeScope(
  packageNameKebab
);

var packageNameUpperCamelCase = Utils$PesyBootstrapper.upperCamelCasify(
  packageNameKebabSansScope
);

var templateVersion = '0.0.0';

var packageLibName = packageNameKebabSansScope + '/library';

var packageTestName = packageNameKebabSansScope + '/test';

function substituteTemplateValues(s) {
  return s
    .replace(/<PACKAGE_NAME_FULL>/g, packageNameKebab)
    .replace(/<VERSION>/g, templateVersion)
    .replace(/<PUBLIC_LIB_NAME>/g, packageLibName)
    .replace(/<TEST_LIB_NAME>/g, packageTestName)
    .replace(/<PACKAGE_NAME>/g, packageNameKebab)
    .replace(/<PACKAGE_NAME_UPPER_CAMEL>/g, packageNameUpperCamelCase);
}

function stripTemplateExtension(s) {
  return s.replace('-template', '');
}

function substituteFileNames(s) {
  return s
    .replace(/__PACKAGE_NAME_FULL__/g, packageNameKebab)
    .replace(/__PACKAGE_NAME__/g, packageNameKebab)
    .replace(/__PACKAGE_NAME_UPPER_CAMEL__/g, packageNameUpperCamelCase);
}

function isDirectoryEmpty(path) {
  return Fs.readdirSync(path).length === 0;
}

function askYesNoQuestion(rl, question, onYes, $staropt$star, param) {
  var onNo =
    $staropt$star !== undefined
      ? $staropt$star
      : function(param) {
          return /* () */ 0;
        };
  return rl.question(question + ' [y/n] ', function(input) {
    var response = input.trim().toLowerCase();
    switch (response) {
      case 'n':
      case 'no':
        Curry._1(onNo, /* () */ 0);
        return rl.close();
      case 'y':
      case 'yes':
        Curry._1(onYes, /* () */ 0);
        return rl.close();
      default:
        return askYesNoQuestion(rl, question, onYes, onNo, /* () */ 0);
    }
  });
}

function forEachDirEnt(dir, f) {
  return dir.read().then(function(dirEnt) {
    if (dirEnt == null) {
      return dir.close();
    } else {
      return Curry._1(f, dirEnt).then(function(param) {
        return forEachDirEnt(dir, f);
      });
    }
  });
}

function scanDir(dir, f) {
  return Bindings$PesyBootstrapper.Fs.opendir(dir).then(function(dir) {
    return forEachDirEnt(dir, function(entry) {
      var entryPath = Path.join(dir.path, entry.name);
      return Curry._1(f, entryPath).then(function(param) {
        if (entry.isDirectory()) {
          return scanDir(entryPath, f);
        } else {
          return Promise.resolve(/* () */ 0);
        }
      });
    });
  });
}

function copyBundledTemplate(param) {
  var templatesDir = Path.resolve(
    Path.dirname(Bindings$PesyBootstrapper.scriptPath),
    '..',
    'lib',
    'node_modules',
    'pesy',
    'templates',
    'pesy-reason-template-0.1.0-alpha.1'
  );
  return scanDir(templatesDir, function(src) {
    var dest = src.replace(templatesDir, process.cwd());
    return Bindings$PesyBootstrapper.Fs.isDirectory(src).then(function(isDir) {
      if (isDir) {
        return Bindings$PesyBootstrapper.Fs.mkdir(false, true, dest);
      } else {
        return Bindings$PesyBootstrapper.Fs.copy(false, src, dest, /* () */ 0);
      }
    });
  });
}

function bootstrap(projectPath, param) {
  if (param !== undefined) {
    return DownloadGitRepo(param, projectPath);
  } else {
    return copyBundledTemplate(/* () */ 0);
  }
}

function subst(file) {
  return Bindings$PesyBootstrapper.Fs.readFile(file)
    .then(function(b) {
      return Promise.resolve(
        Buffer.from(substituteTemplateValues(b.toString()))
      );
    })
    .then(function(s) {
      return Bindings$PesyBootstrapper.Fs.writeFile(file, s);
    })
    .then(function(param) {
      return Promise.resolve(
        (Fs.renameSync(
          file,
          substituteFileNames(file).replace('-template', '')
        ),
        /* () */ 0)
      );
    });
}

function substitute(projectPath) {
  return Promise.all(
    WalkSync(projectPath)
      .filter(function(file_or_dir) {
        return Fs.statSync(file_or_dir).isFile();
      })
      .map(subst)
  );
}

function spinnerEnabledPromise(spinnerMessage, promiseThunk) {
  var spinner = Spinner$PesyBootstrapper.start(spinnerMessage);
  return Curry._1(promiseThunk, /* () */ 0)
    .then(function(x) {
      Spinner$PesyBootstrapper.stop(spinner);
      return Promise.resolve(x);
    })
    .catch(function(e) {
      Spinner$PesyBootstrapper.stop(spinner);
      return Curry._1(Bindings$PesyBootstrapper.throwJSError, e);
    });
}

function setup(template, projectPath) {
  return bootstrap(projectPath, template)
    .then(function(param) {
      return spinnerEnabledPromise('\x1b[2mSetting up\x1b[0m ', function(
        param
      ) {
        return substitute(projectPath);
      });
    })
    .then(function(_arrayOfCompletions) {
      return spinnerEnabledPromise(
        '\x1b[2mRunning\x1b[0m esy install',
        function(param) {
          return Bindings$PesyBootstrapper.ChildProcess.exec('esy i', {
            cwd: projectPath,
          });
        }
      );
    })
    .then(function(param) {
      return spinnerEnabledPromise(
        '\x1b[2mRunning\x1b[0m esy pesy\x1b[2m and \x1b[0m building project dependencies',
        function(param) {
          return Bindings$PesyBootstrapper.ChildProcess.exec('esy pesy', {
            cwd: projectPath,
          });
        }
      );
    })
    .then(function(param) {
      return spinnerEnabledPromise('\x1b[2mRunning\x1b[0m esy build', function(
        param
      ) {
        return Bindings$PesyBootstrapper.ChildProcess.exec('esy b', {
          cwd: projectPath,
        });
      });
    })
    .then(function(param) {
      return Promise.resolve(/* () */ 0);
    })
    .catch(Bindings$PesyBootstrapper.handlePromiseRejectInJS);
}

function main$prime(template, useDefaultOptions) {
  if (isDirectoryEmpty(projectPath) || useDefaultOptions) {
    setup(template, projectPath);
    return /* () */ 0;
  } else {
    var rl = Readline.createInterface({
      input: process.stdin,
      output: process.stdout,
    });
    return askYesNoQuestion(
      rl,
      'Current directory is not empty. Are you sure to continue?',
      function(param) {
        setup(template, projectPath);
        return /* () */ 0;
      },
      undefined,
      /* () */ 0
    );
  }
}

function main(template, useDefaultOptions) {
  try {
    return main$prime(template, useDefaultOptions);
  } catch (raw_exn) {
    var exn = Caml_js_exceptions.internalToOCamlException(raw_exn);
    if (exn[0] === Js_exn.$$Error) {
      var match = exn[1].message;
      if (match !== undefined) {
        console.log('Error: ' + (String(match) + ''));
        return /* () */ 0;
      } else {
        console.log('An unknown error occurred');
        return /* () */ 0;
      }
    } else {
      throw exn;
    }
  }
}

var $$short = '-t';

var $$long = '--template';

var doc =
  'Specify URL of the remote template. This can be of the form https://repo-url.git#<commit|branch|tag>. Eg: https://github.com/reason-native-web/morph-hello-world-pesy-template#6e5cbbb9f28';

var v = {
  contents: undefined,
};

var anonFun = /* String */ Caml_chrome_debugger.variant('String', 4, [
  function(template) {
    v.contents = template;
    return /* () */ 0;
  },
]);

var Template = {
  $$short: $$short,
  $$long: $$long,
  doc: doc,
  v: v,
  anonFun: anonFun,
};

var $$short$1 = '-y';

var $$long$1 = '--yes';

var doc$1 = 'Use default options';

var v$1 = {
  contents: false,
};

var anonFun$1 = /* Set */ Caml_chrome_debugger.variant('Set', 2, [v$1]);

var DefaultOptions = {
  $$short: $$short$1,
  $$long: $$long$1,
  doc: doc$1,
  v: v$1,
  anonFun: anonFun$1,
};

var $$short$2 = '-v';

var $$long$2 = '--version';

var doc$2 = 'Prints version and exits';

var v$2 = '0.5.0-alpha.10';

var anonFun$2 = /* Unit */ Caml_chrome_debugger.variant('Unit', 0, [
  function(param) {
    console.log(v$2);
    return /* () */ 0;
  },
]);

var Version = {
  $$short: $$short$2,
  $$long: $$long$2,
  doc: doc$2,
  v: v$2,
  anonFun: anonFun$2,
};

var CliOptions = {
  Template: Template,
  DefaultOptions: DefaultOptions,
  Version: Version,
};

var specList_000 = /* tuple */ [$$long, anonFun, doc];

var specList_001 = /* :: */ Caml_chrome_debugger.simpleVariant('::', [
  /* tuple */ [$$short, anonFun, doc],
  /* :: */ Caml_chrome_debugger.simpleVariant('::', [
    /* tuple */ [$$long$1, anonFun$1, doc$1],
    /* :: */ Caml_chrome_debugger.simpleVariant('::', [
      /* tuple */ [$$short$1, anonFun$1, doc$1],
      /* :: */ Caml_chrome_debugger.simpleVariant('::', [
        /* tuple */ [$$short$2, anonFun$2, doc$2],
        /* :: */ Caml_chrome_debugger.simpleVariant('::', [
          /* tuple */ [$$long$2, anonFun$2, doc$2],
          /* [] */ 0,
        ]),
      ]),
    ]),
  ]),
]);

var specList = /* :: */ Caml_chrome_debugger.simpleVariant('::', [
  specList_000,
  specList_001,
]);

var usageMsg = '$ pesy [-t|--template <url>] [-y]';

Arg.parse(
  specList,
  function(param) {
    return /* () */ 0;
  },
  usageMsg
);

main(v.contents, v$1.contents);

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
exports.askYesNoQuestion = askYesNoQuestion;
exports.forEachDirEnt = forEachDirEnt;
exports.scanDir = scanDir;
exports.copyBundledTemplate = copyBundledTemplate;
exports.bootstrap = bootstrap;
exports.subst = subst;
exports.substitute = substitute;
exports.spinnerEnabledPromise = spinnerEnabledPromise;
exports.setup = setup;
exports.main$prime = main$prime;
exports.main = main;
exports.CliOptions = CliOptions;
exports.specList = specList;
exports.usageMsg = usageMsg;
/*  Not a pure module */
