---
id: configuring
title: Configuring your project
---

`pesy` assumes that a project is broken down into smaller units called
subpackages. For any given project, you might want to expose multiple
things - multiple binaries and/or multiple libraries. `pesy`
(completely inspired by Dune) provides directory as a way to organise
your project into smaller subpackages. Similar to how in the `npm`
world, where one can require `lodash/core` and `lodash/fp`
separately. 

Note that: subpackages are imported, not files themselves!


Configure your `package.json`'s `buildDirs` field for multiple libraries and
executables.
`buildDirs.DirectoryName` means that a library or executable will be located at
`./DirectoryName`. The `buildDirs.DirectoryName.name` field determines the
public name of the library or executable.

TODO (example)

Keep in mind the following

**1. Libraries are identified with a path, much like Javascript
requires (`var curryN = require('lodash/fp/curryN');`)**


**2. External libraries that did not use pesy**

Libraries written using pesy are straightforward to consume - for the package.json looking
like this,


```json
{
  "name": "@scope/foo",
  "buildDirs": {
    "lib": {...}  
  }
}
```

it is `require('@scope/foo')`. 

Other external libraries have dune public names (eg. `sexplib` and
`yojson`). Some packages, eg. Ctypes, however, also export multiple libraries
Eg. `ctypes` and `ctypes.foreign`. Think of `.foreign` as a subpackage of ctypes and 
therefore as exisiting in a path like `ctypes/foreign` (this doesn't
have to be so in reality). Thus,

* Console (reason-native on npm): `require('console/lib')`
* Pastel (reason-native on npm): `require('pastel/lib')`
* Rely (reason-native on npm): `require('rely/lib')`
* Sexplib (opam): `require('sexplib')`
* Ctypes (opam): `require('ctypes')` and `require('ctypes/foreign')`

You can try looking up the correct require path using `esy pesy ls-libs`

```bash
$ esy pesy ls-libs
├── require('lwt')                    
├── require('lwt/unix')                           
├── require('lwt_ppx')                         
├── require('merlin')                          
├── require('@pesy/esy-pesy')                  
├── require('@pesy/esy-pesy/lib')              
└── require('@pesy/esy-pesy/utils')   
```


## Relation to Dune

Pesy as accepts package.json as input and producing dune files as output.

```

                  +----------------+
                  |                |
package.json ---> |      pesy      +  --->  dune files
                  |                |
                  +----------------+

```

Note that not all of Dune's features are supported in pesy (PRs
welcome). `pesy` doesn't intend to duplicate Dune's efforts - it's
meant to compliment it. For simple use cases, `pesy` wants to provide
a NPM friendly interface to Dune so that newcomers can quickly get
started, without confusing themselves with the library vs packages
nuances.

### Namespace collison

Every library, as we know, exposes a namespace/module under which it's APIs are
available. However, as package authors, it can hard to make sure one
is not using a namespace already taken by another package (Otherwise
it could lead to collisions). Pesy works around this by assigning the library
the upper camelcase of the root package name and directory the
library/sub-package resides in. 

Example: if a package.json looks like this 

```json
{
  "name": "@myscope/foo",
  "buildDirs": {
    "library": { ... }
  }
}
```

Then, subpackage `library` takes a namespace of
`MyScopeFooLibrary`. As a user, however, you shouldn't have to worry
much about yourself, since you can specify how you'd like to import
subpackages (and packages). In the above example, another subpackage
would consume it as follows

```json
{
  "name": "@myscope/foo",
  "buildDirs": {
    "library": { ... },
    "anotherLibrary": {
      "imports": [
        "ThatOtherLibrary = require('@myscope/foo/library')"
      ]
    }
  }
}
```

And if you were consuming this package (after having published to
npm), you can import it as follows

```json
{
  "name": "bar",
  "buildDirs": {
    "library": {
      "imports": [
        "ThatFooLibrary = require('@myscope/foo/library')"
      ]
    }
  }
```

With the new NPM like conventions, pesy automatically handles the
namespace for you so that we don't have to worry about the nuances of
a package and a library during development. 


## Commiting the dune files 🐫

While dune files can be thought of as generated artifacts,
we strongly recommended that you always commit them.

pesy is forced to generate these files in-source, and therefore must not
be run in the build environment since in-source compilation is known to be
slow. It is meant to assist your workflow. `pesy` doesn't function as a standalone
build tool.


## Workaround for scoped packages on NPM

Pesy is currently not meant to be run in the build environment. This means it cannot run Dune for you. While this is not an issue at all usually, when scoped packages are created using pesy, the package name must be transformed to work well with Dune and OPAM.

Example

```json
{
  "name": "@my-npm-scope/foobar",
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
  "name": "@my-npm-scope/foobar",
  ...
  "esy": {
    "build": "dune build -p my-npm-scope--foobar"
  }
}
```


[Here](https://github.com/esy/pesy/tree/master/e2e-tests/pesy-configure-test-projects/pesy-npm-scoped) is an example repo (that is run on the CI regularly)

### Ejecting:

It is always possible to eject out of pesy config by simply deleting
`buildDirs` and managing the dune files by hand. (Coming soon: [`pesy eject`](https://github.com/esy/pesy/issues/52))

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
      "require": ["bos.top"]
    },
    "bin": {
      "name": "my-package.exe",
      "require": ["my-package.lib"]
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
