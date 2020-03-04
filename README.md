<p align="center" style="margin: 2rem 0">
  <img src="./images/screenshot.png" width="256" title="Github Logo">
</p>

[![Build Status](https://dev.azure.com/pesy/pesy/_apis/build/status/esy.pesy?branchName=master)](https://dev.azure.com/pesy/pesy/_build/latest?definitionId=1&branchName=master)

`pesy` is a command line tool to assist with your native Reason/OCaml
workflow using the package.json itself. 

1. Quickly bootstrap a new Reason project
2. Configure your builds

Essentially, development with pesy looks like this

```sh
mkdir my-new-project
cd my-new-project
pesy # Bootstraps the project, installs the dependencies and builds the project
```

Once bootstrapped, you can use pesy to manage the builds

```sh
# edit code
pesy
# Repeat
```

### Installation

```sh
npm install -g pesy
```

### Consuming New Package And Library Dependencies:

- Add dependencies to `dependencies` in `package.json`.
- Add the name of that new dependencies _library_ to `package.json`'s
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
        "require": ["bos.top"]
      }
    }
  }
  ```
  
  Alternatively, with the new npm like require syntax, it can be
  
 ```json 
  {
    "name": "my-package",
    "dependencies": {
      "@opam/bos": "*"
    },
    "buildDirs": {
      "exampleLib": {
        "require": ["bos.top"]
      }
    }
  }
  ```
  Without specifying the namespace or name. Pesy will automatically
  assign it a name of `my-package.exampleLib` and a namespace
  `MyPackageExampleLib`
  
  You can either import this package using these name/namespace or use
  the convenient require syntax as explained in [NPM like features
  (experimental)](https://github.com/esy/pesy/tree/require-syntax-docs#npm-like-features-experimental)

  ```json
  {
     "name": "a-consuming-package",
     "buildDirs": {
       "lib": {
         "imports": [
           "Library = require('my-package/exampleLib')"
         ]
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

**version 0.4.0 (12/21/2018)**

- Allow `buildDirs` to contain deeper directories such as `"path/to/my-lib": {...}"`.
- Added support for `wrapped` property on libraries.
- Added support for `virtualModules` and `implements` - properties for Dune
  virtual libraries. (This will only be supported if you mark your project as
  Dune 1.7 - not yet released).
- Stopped using `ignore_subdirs` in new projects, instead using
  `(dirs (:standard \ _esy))` which only works in Dune `1.6.0+`, so made new
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

### Compiler Support

Please refer [this document](./notes/compiler-support.org)
    
### Checksum verification

As we create the build artifacts to publish to NPM, we also generate the SHA1 hash
of the `.tgz` file created by `npm pack`, in a manner similar to how npm does.
This way, you can verify that the package published to NPM is infact the same
set of binaries that were built on CI.

You can verify this by following this simple steps.

1. Head over to CI logs as per the release version

   a. [Pre-beta](https://dev.azure.com/pesy/pesy/_build/results?buildId=103)

2) Navigate to the `Release Job` section

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
