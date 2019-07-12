
![screenshot](./images/screenshot.png "Demo")

# pesy

[![Build Status](https://dev.azure.com/pesy/pesy/_apis/build/status/esy.pesy?branchName=master)](https://dev.azure.com/pesy/pesy/_build/latest?definitionId=1&branchName=master)

Pesy is a CLI tool to assist project management using `package.json` to automatically configure libraries and executables built
with Dune. Pesy is a great tool to
1. Quickly bootstrap a new Reason project
2. Configure dune using package.json itself. Checkout [Dune Configuration](#dune-configuration), as to why use pesy to build your project.
 
Essentially, development with pesy looks like this

```sh
mkdir my-new-project
cd my-new-project
pesy # Bootstraps the project, installs the dependencies and builds the project
```

Once bootstrapped, you can use pesy to manage the builds
```sh
# edit code
esy pesy
esy build
# Repeat
```

### Installation

```sh
npm install -g pesy
```

This installs a prebuilt binary on your system. For checksum verification - refer [these steps](#checksum-verification)

> Note: The new native rewrite of `pesy` has been published to `npm install -g pesy@next` and is undergoing alpha/beta testing.

### Creating a new project:

`pesy` global command creates `esy` projects instantly inside of any directory.

```sh
cd my-project
pesy
```

This creates:

- `package.json` with useful dependencies/compilers.
- `.gitignore` and `README.md` with instructions for new contributors.
- `.ci` continuous integration with cache configured for ultra-fast pull
    requests.
- `library/`, `executable/` and `test/` directory with starter modules.

If you all you wanted from pesy is to bootstrap a new project, then you are good to go!

### Dune configuration
Along with bootstrapping a project, pesy can also manage your dune config.

Dune is a great build tool and we couldn't have asked for a better tool from the OCaml community. However, with a couple of changes in Dune's conventions (example public_name and name) and syntax (JSON instead of s-expressions), we believe beginners would have easier time adopting Reason/OCaml.

Pesy tries to achieve this by accepting package build configuration mentioned in the package.json and creates the dune file (config files Dune reads to build your project) for you. For example,

```json
 "buildDirs" {
    "lib": {
      "require": [
        "str",
        "sexplib",
        "yojson",
        "my-project/utils"
      ]
    },
    ...
 }
```

This means, before calling dune, one must run `esy pesy` to create the dune files so that dune can build your project.

Think of pesy as accepting package.json as input and producing dune files as output.

```

                  +----------------+
                  |                |
package.json ---> |      pesy      +  --->  dune files
                  |                |
                  +----------------+

```

Anytime you update the project config in the package.json, make sure you run `esy pesy`.

(Hopefully, this could be automatically done in the future so you only ever run
`esy build` as usual).

### Commiting the dune files 🐫

While dune files can be thought of as generated artifacts,
we strongly recommended that you always commit them.

pesy is forced to generate these files in-source, and therefore must not 
be run in the build environment since in-source compilation is known to be
slow. It is meant to assist your workflow. `pesy` doesn't function as a standalone
build tool.

### NPM like features (experimental) 

Every library, as we know, exposes a namespace/module under which it's APIs are
available. In order to ease consumption, pesy tries to guess this namespace for you,
so that you can avoid configuring it yourself. It does so by assigning the library
the upper camelcase of the directory the library/sub-package resides in. If you are
not satisfied, you can override this by adding `namespace` property exactly like how 
you would with Dune.

With the new NPM like conventions, pesy automatically handles the namespace for you so that we don't have to worry about the nuances of a package and a library during development. All that we need to know is, in a configuration like the one above, 

**1. Libraries are identified with a path, much like Javascript requires (`var curryN = require('lodash/fp/curryN');`)**

```json
"buildDirs": {
  "library": {
    "require": [ "foo/foolib" ]
  }
}
```

You may look at [Creating and publishing libraries to NPM](#creating-and-publishing-libraries-and-tools-to-npm) for an example

**2. Namespaces are upper camel cased folder names**

Pesy chooses the capitalised folder name of the sub-package as the namespace of the package. This is to reduce the cognitive overhead of library v/s package differences.

**3. All sub-packages are considered to be libraries unless they have a `bin` property (much like NPM)**
Any sub-package with `bin` property is considered to be an executable subpackage.

```json
"buildDirs": {
   "executable": {
      "require": [
        "foo/library"
      ],
      "bin": {
        "FooApp.exe": "FooApp.re"
      }
    }
}
```
If not present, it is assumed that the subpackage is meant to be consumed as a library. This can be compared to NPM's behaviour.


### Workaround for scoped packages on NPM

Pesy is currently not meant to be run in the build environment. This means it cannot run Dune for you. While this is not an issue at all usually, when scoped packages are created using pesy, the package name must be transformed to work well with Dune and OPAM.

Example

```json
{
  "name": "@my-npm-scope/foobar"
  ...
  "esy": {
    "build": "dune build -p #{self.name}"
  }
}
```
This above config will fail. This is because `#{self.name}` evaluates to `@my-npm-scope/foobar`. This is being tracked [here](https://github.com/esy/pesy/issues/31). The workaround is to transform is for this form: `<scope>--<package_name>`.

This way `@my-npm-scope/foobar` becomes `my-npm-scope--foobar`. And use this double kebabified name in `esy.build`

```json
{
  "name": "@my-npm-scope/foobar"
  ...
  "esy": {
    "build": "dune build -p my-npm-scope--foobar"
  }
}
```

And run `esy pesy`

[Here](https://github.com/esy/pesy/tree/master/e2e-tests/pesy-configure-test-projects/pesy-npm-scoped) is an example repo (that is run on the CI regularly)

### Configuring:

Configure your `package.json`'s `buildDirs` field for multiple libraries and
executables.
`buildDirs.DirectoryName` means that a library or executable will be located at
`./DirectoryName`. The `buildDirs.DirectoryName.name` field determines the
public name of the library or executable.


```json
"buildDirs": {
  "MyLibrary": {
    "name": "packageNameMyLibrary",
    "namespace": "MyLibrary",
    "require": ["console.lib"]
  },
  "Tests": {
    "name": "Tests.exe",
    "description": "Runs all the tests natively",
    "flags": ["-linkall"],
    "require": ["console.lib", "packageNameMyLibrary""]
  }
}
```

### Supported Config
Not all config is supported. This is just a proof of concept. If you'd like to
add support for more config fields, PRs are welcomed.

**Binaries**

| Field  | Type            | Description                                                                                                                                                                                                                                                                                                                                             |
|--------|-----------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
|`name`  | `string`        | The name of the binary **that must end with `.exe`**.                                                                                                                                                                                                                                                                                                   |
|`main`  | `string`        | The name of the module that serves as the main entrypoint of the binary.                                                                                                                                                                                                                                                                                |
|`modes` | `list(string)`  | [Advanced linking modes](https://jbuilder.readthedocs.io/en/latest/dune-files.html?highlight=modules_without_implementation#linking-modes). Each string should be of the form `"(<compilation-mode> <binary-kind>)"` where `<compilation-mode>` is one `byte`, `native` or `best` and `<binary-kind>` is one of `c`, `exe`, `object`, `shared_object`.  |

**Libraries**

| Field           | Type                              | Description                                                                                                                                                                                                                                                                                                                   |
|-----------------|-----------------------------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
|`name`           | `string`                          | The name of the library                                                                                                                                                                                                                                                                                                       |
|`namespace`      | `string`                          | The root module name of the library                                                                                                                                                                                                                                                                                           |
|`modes`          | `list("byte"\|"native"\|"best")`  | Mode which should be built by default. Useful for disabling native compilation for some libraries.                                                                                                                                                                                                                            |
|`cNames`         | `list(string)`                    | List of strings to use as C stubs (filenames without the `.c` extension).                                                                                                                                                                                                                                                     |
|`virtualModules` | `list(string)`                    | List of modules within the library that will have interfaces but no implementation, causing this library to be considered "virtual". Another library can then claim to "implement" this library by including `"implements": "yourLibName"`. See [Virtual Libraries](https://jbuilder.readthedocs.io/en/latest/variants.html)  |
|`implements`     | `list(string)`                    | List of virtual library names that this library implements.                                                                                                                                                                                                                                                                   |
|`wrapped`        | `true|false`                      | Default `true`, and it's a good idea to keep it that way. Setting to `false` will put all your library modules in the global namespace.


**Both Libraries And Binaries**

| Field                 | Type                  | Description                                                                                                                                                                                                                                                              |
|-----------------------|-----------------------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
|`require`              | `list(string)`        | Public library names you want to be able to use.                                                                                                                                                                                                                         |
|`flags`                | `list(string)`        | List of strings to pass to both native and bytecode compilers.                                                                                                                                                                                                           |
|`ocamlcFlags`          | `list(string)`        | List of flags to pass to `ocamlc`                                                                                                                                                                                                                                        |
|`ocamloptFlags`        | `list(string)`        | List of flags to pass to `ocamlopt`                                                                                                                                                                                                                                      |
|`jsooFlags`            | `list(string)`        | List of flags passed to `jsoo`                                                                                                                                                                                                                                           |
|`preprocess`           | `list(string)`        | List of preprocess options to enable. Primarily used to enable PPX                                                                                                                                                                                                       |
|`ignoredSubdirs`       | `list(string)`        | Subdirectory names to ignore (This feature is soon to be deprecated).                                                                                                                                                                        |
|`includeSubdirs`       | `"no"\|"unqualified"` | Default is `"no"`, and changing to `"unqualified"` will compile modules at deeper directories than the place where the `dune` file is generated. See [Dune docs](https://jbuilder.readthedocs.io/en/latest/dune-files.html?highlight=include_subdirs#include-subdirs)    |
|`rawBuildConfig`       | `list(string)`        | Raw build config to be injected into the build config for _this_ target.                                                                                                                                                                                                 |
|`rawBuildConfigFooter` | `list(string)`        | Raw build config to be injected into the footer of the build config.                                                                                                                                                                                                     |




### Consuming New Package And Library Dependencies:

- Add dependencies to `dependencies` in `package.json`.
- Add the name of that new dependencies *library*  to `package.json`'s
    `buildDirs` section that you want to use the library within. For example, if
    your project builds a library in the `exampleLib/` directory, and you want it
    to depend on a library named `bos.top` from an opam package named `bos`,
    change the `package.json` to look like this:

    ```json
    {
      "name": "my-package",
      "dependencies": {
        "@opam/bos": "*"
      },
      "buildDirs": {
        "exampleLib": {
          "namespace": "Examples",
          "name": "my-package.example-lib",
          "require": [ "bos.top" ]
        }
      }
    }
    ```

- Then run:
    ```sh
    esy install  # Fetch dependency sources
    esy pesy     # Configure the build based on package.json
    esy build    # Do the build
    ```

> Note: After adding/building a new dependency you can use `esy ls-libs` to see
> which named libraries become available to you by adding the package
> dependency.



### Tradeoffs:
`esy-pesy` is good for rapidly making new small executables/libraries. Once they
grow, you'll want to "eject out" of `esy-pesy` and begin customizing using a more
advanced build system.


### Adding `pesy` to an existing project.

You probably don't need `pesy` if you have an existing project that is working
well, but to add `pesy` to an existing project, follow these steps:

**1. Add a dependency on `pesy`, and configure `buildDirs`:**

```json
{
  "name": "my-package",
  "dependencies": {
    "pesy": "*"
  },
  "buildDirs": {
    "exampleLib": {
      "namespace": "Examples",
      "name": "my-package.example-lib",
      "require": [ "bos.top" ]
    },
    "bin": {
      "name": "my-package.exe",
      "require": [
        "my-package.lib"
      ]
    }
  }
}
```

**2.Install and Build:**

```sh
esy install
esy pesy  # Generate the project build config from json
esy build
```

### Creating and publishing libraries and tools to NPM

#### Library in the source form

Easiest way to get started with distributing you library is to publish the source to NPM. Let's take a look at an example.

Consider a base package `foo` that you created and distributed on NPM. And let's assume, `bar` is the package that consumes `foo`.

```sh
$ mkdir foo
$ cd foo
$ pesy
# edit code
$ esy pesy # if build config has changes
$ esy build
$ npm publish
```

Publishing to npm is just a matter of running `npm publish`. Lets take closer look at the commands.

```sh
$ mkdir foo
$ cd foo
$ pesy
```
As you'd know by now, these commands, bootstrap, install dependencies and build the entire project.

At this point, our project looks like this

```
│
├── library 
│
├── executable
│   
├── test
│   
└── testExe (test runner)
   
```

With the `buildDirs` section of the package.json looking like the following

```json
  "buildDirs": {
    "test": {
      "require": [
        "foo/library",
        "rely.lib"
      ],
      "flags": [
        "-linkall",
        "-g"
      ]
    },
    "testExe": {
      "require": [
        "foo/test"
      ],
      "bin": {
        "RunFooTests.exe": "RunFooTests.re"
      }
    },
    "library": {},
    "executable": {
      "require": [
        "foo/library"
      ],
      "bin": {
        "FooApp.exe": "FooApp.re"
      }
    }
  },
```
Given that pesy tries to unify packages and libraries, for a config mentioned above it has made `library` available under the namespace `Library`. So anytime `foo` is added as a dependency, a module `Library` becomes available in the codebase.

Since `Library` is to generic to be useful, let's rename it to `FooLib` (i.e. make rename the package as foolib)

```sh
$ mv library foolib
```

And update the config

```diff
  "buildDirs": {
    "test": {
      "require": [
        "foo/library",
        "rely.lib"
      ],
      "flags": [
        "-linkall",
        "-g"
      ]
    },
    "testExe": {
      "require": [
        "foo/test"
      ],
      "bin": {
        "RunFooTests.exe": "RunFooTests.re"
      }
    },
-    "library": {},
+    "foolib": {},
    "executable": {
      "require": [
        "foo/library"
      ],
      "bin": {
        "FooApp.exe": "FooApp.re"
      }
    }
  },
```

Since config has changed, we run `esy pesy` and build the project

```sh
$ esy pesy
$ esy build
```

Let's edit the Utils.re too

```js
let foo = () => print_endline("Hello from foo");
```

Build and publish!

```sh
$ esy build
$ npm publish
```

#### Consuming `foo`

Let quickly create a new project, `bar` and add `foo`.

```sh
$ mkdir bar
$ cd bar
$ pesy
$ esy add foo
```

We can now require `foo` (sort of like we did in Javascript)

```diff
  "buildDirs": {
    "test": {
      "require": [
        "bar/library",
        "rely.lib"
      ],
      "flags": [
        "-linkall",
        "-g"
      ]
    },
    "testExe": {
      "require": [
        "bar/test"
      ],
      "bin": {
        "RunBarTests.exe": "RunBarTests.re"
      }
    },
    "library": {
      "require": [
+        "foo/foolib"
      ]
    },
    "executable": {
      "require": [
        "bar/library"
      ],
      "bin": {
        "BarApp.exe": "BarApp.re"
      }
    }
  },
```
And then edit Utils.re

```js
let foo = () => {
  Foolib.Util.foo();
  print_endline("This is from bar");
};
```

```sh
$ esy pesy
$ esy build
$ esy x BarApp.exe
Hello from foo!
This is from bar
```

Publishing and consuming Reason native packages from NPM is easy. We just need to keep in mind that the namespace exposed from our library depends on the name of the folder!

### Example Project:

The following example project already has an example config. You can base your
project off of this one.

```sh
npm install -g esy@next
git clone git@github.com:jordwalke/esy-peasy-starter.git

esy install
esy pesy    # Use pesy to configure build from package.json
esy build
```

- Change the `name` of the package, and names of libraries in `buildDirs`
  accordingly.
- Then re-run:

```sh
esy pesy
esy build
```

### Development

```sh
cd re/
esy install
esy build
esy dune runtest # Unit tests
```

### e2e tests
`./_build/install/default/bin` would contain (after running `esy build`) `TestBootstrapper.exe` and `TestPesyConfigure.exe` 

to test if simple workflows work as expected. They assume both `esy` and `pesy` are installed
globally (as on user's machines).

`run.bat` and `run.sh` inside `scripts` can be used to globally install using npm pack. Then run
the e2e scripts.

```sh
./scripts/run.sh
./_build/install/default/bin/TestBootstrapper.exe
./_build/install/default/bin/TestPesyConfigure.exe
```

### Changes:

**version 0.4.0  (12/21/2018)**

- Allow `buildDirs` to contain deeper directories such as `"path/to/my-lib": {...}"`.
- Added support for `wrapped` property on libraries.
- Added support for `virtualModules` and `implements` - properties for Dune
  virtual libraries. (This will only be supported if you mark your project as
  Dune 1.7 - not yet released).
- Stopped using `ignore_subdirs` in new projects, instead using
  `(dirs (:standard \ _esy))` which only works in  Dune `1.6.0+`, so made new
  projects have a lower bound of Dune `1.6.0`.
- Support new properties `rawBuildConfig` which will be inserted at the bottom
  of the _target_ being configured (library/executable).
  - It expects an array of strings, each string being a separate line in the
    generated config.
- Support new properties `rawBuildConfigFooter` which will be inserted at the
  bottom of the entire Dune file for the _target_ being configured.
  - It expects an array of strings, each string being a separate line in the
    generated config.
- Support new properties `modes` for binaries and libraries `list(string)`.

### Checksum verification

As we create the build artifacts to publish to NPM, we also generate the SHA1 hash
of the `.tgz` file created by `npm pack`, in a manner similar to how npm does.
This way, you can verify that the package published to NPM is infact the same
set of binaries that were built on CI. 

You can verify this by following this simple steps.

1. Head over to CI logs as per the release version

    a. [Pre-beta](https://dev.azure.com/pesy/pesy/_build/results?buildId=103)


2. Navigate to the `Release Job` section

![release-job](./images/release-job.png "Release job section")

3. Look for 'Calculating sha1' 

![calculating-sha1](./images/calculating-sha1.png "Calculating sha1 option in the logs")

4. Verify its the same as the one in `npm info pesy`. Of course, ensure that the version
you see in `npm info pesy` is the same the one in the logs.

![sha1-logs](./images/sha1-logs.png "SHA1 hash in the logs")

You can also download the package straight from the CI and check if it is the
same as the one on NPM.

1. In the same logs, on the top right you would see a blue button labeled `Artifacts`

![top-right-corner](./images/top-right-corner.png "Artifacts button on the top right corner")

2. In the sub menu drawn up by `Artifacts`, click on `Release`. This is the job where we
   collect are platform binaries and package them for NPM. You'll only find platform-specfic
   binaries in the other jobs.

![release-option](./images/release-option.png "Release option")

3. A file explorer like interface opens on clicking `Release` as explained in the previous step.
   Click on the `Release` folder - the only option. We run `npm pack` in this directory structure.

![artifacts-explorer](./images/artifacts-explorer.png "Artifacts explorer")

4. `pesy-<version>.tgz` is the tar file that published to npm. You can uncompress and inspect its
   contents, or check its SHA1 integrity and ensure it's the same as the one on NPM

![download](./images/download.png "Download option for pesy-<version>.tgz")

5. You might have to tap on the file once to show the kebab menu.

![tap-for-kebab](./images/tap-for-kebab.png "Tap for Kebab menu")
