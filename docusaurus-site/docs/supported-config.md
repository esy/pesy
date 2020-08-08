---
id: supported-config
title: Supported Dune Config
---

This is reference guide explaining the config `pesy` supports. If
you're looking for a guide to explain how to configure the project,
please checkout [Configuring your Project](/docs/configuring)

# Binaries

Configuration that applies to subpackages that create binary
executables. Note that these executables can be ocaml bytecode or
native binaries (ELF/Mach/PE) 

## bin
#### *type: string*
A subpackage produces binary when it contains a `bin` property. 

```json
{
  "buildDirs": {
    "src": {
      "bin": "Main.re"
    }
  }
}
```
[A working example](https://github.com/prometheansacrifice/pesy-samples/blob/master/simple-bin/package.json)

## modes
#### *type: array(string)*
An array of [advanced linking
modes](https://dune.readthedocs.io/en/latest/dune-files.html?highlight=modules_without_implementation#linking-modes). Each
string should be of the form "`compilation-mode` `binary-kind`" where
`compilation-mode` is one byte, native or best and `binary-kind` is
one of c, exe, object, shared_object. 

```json
{
  "buildDirs": {
    "src": {
	  "bin": "Foo.re",
	  "modes": [ "native", "exe"] 
	}
  }
}
```

[A working example](https://github.com/prometheansacrifice/pesy-samples/blob/master/bin-modes/package.json)

## name (override)
#### *type: string*
A string that maps to Dune's `public_name`. Usually unnecessary (as
`bin` property takes care of it) and must only be used to override. 

## main (override)
#### *type: string*
A string that maps to Dune's `name`. Usually unnecessary (as `bin`
property takes care of it) and must only be used to override the entry
point. 

# Libraries

## modes
#### *type: array(string)*

`modes` can be used to configure the compilation target - [native](https://caml.inria.fr/pub/docs/manual-ocaml/native.html) or
[bytecode](https://caml.inria.fr/pub/docs/manual-ocaml/comp.html) An
array of string, any of `byte`, `native`, `best` 

### `byte` 
This mode generates byte code output

### `native`
This mode generates native output

### `best`
Sometimes it may not be obvious if native compilation is
supported on a machine. In such circumstances, use `best` and
`native` will be picked for you if it's available. Else, it'll be `byte`

## cNames
#### *type: array(string)*
When writing C stubs to FFI into a library, simply mention the file
name (without the `.c` extension) in the `cNames` field.

```json
{
  "buildDirs": {
    "src": {
      "cNames": ["my-stub1", "my-stub-2"]
	}
  }
}
```
[A working example](https://github.com/prometheansacrifice/pesy-samples/blob/master/cNames/package.json)

# Config supported by both libraries and binaries

## imports
#### *type: array(string)*

`imports` can be used to import a library (from a subpackage or an
external npm/opam package) and utilise the namespace of the imported
library.

```json
{
  "buildDirs": {
    "src": {
	  "imports": [
	    "Console = require('console')"
	  ]
	}
  }
}
```

The above config makes a namespace `Console` available inside the
subpackage `src`. Now any `.re` file inside `src` can use the
`console` library.

```reason
// src/SomeFile.re
let foo = () => Console.log("Hello, world")
```

We can also import a package/subpackage under a different namespace

```json
{
  "buildDirs": {
    "src": {
	  "imports": [
	    "NotConsole = require('console')"
	  ]
	}
  }
}
```

And we can import (oddly confusing) `NotConsole` from in `src`

```reason
// src/SomeFile.re
let foo = () => NotConsole.log("Hello, world");
```

We can also import other subpackages in the project.

```json
{
  "buildDirs": {
    "src": {
      "bin": "Main.re",
      "imports": [
        "FooConsole = require('console/lib')",
        "MyOwnLibrary = require('../my-own-lib')"
      ]
    },
    "my-own-lib": {}
  }
}
```

[A working example](https://github.com/prometheansacrifice/pesy-samples/blob/master/imports/package.json)

## Compiler flags
#### All of type *list(string)*

- `flags` - These flags will be passed to both the bytecode compiler and native compiler
- `ocamlcFlags` - These will be passed to `ocamlc` - the bytecode compiler
- `ocamloptFlags` - These will be passed to `ocamlopt` - the native compiler
- `jsooFlags` - These will be passed to `jsoo` compiler - the [javascript compiler](http://ocsigen.org/js_of_ocaml/3.5.1/manual/overview). *Note: This is unrelated to [Bucklescript](https://bucklescript.github.io/)*



## Preprocessor flags
#### *type:list(string)*

`preprocess` accepts options needed to pass the source files via a
preprocessor first. When using custom syntax not natively supported in
the compiler, we pass the sources in a subpackage via a preprocessor
first.

Example,

Say we'd like to use `let%lwt` syntax for our `Lwt` promises

```reason

let%lwt foo = Lwt.return("foo");
print_endline(foo);

// instead of 

Lwt.return >>= (foo => print_endline(foo); Lwt.return())
```

We specify `lwt_ppx` in `pps preprocess`

```json
{
  "buildDirs": {
    "src": {
	  "bin": "Main.re",
	  "preprocess": ["pps", "lwt_ppx"]
	}
  }
}
```
[A working example](https://github.com/prometheansacrifice/pesy-samples/blob/master/preprocess/package.json)

## includeSubdirs
#### *type: "no"|"unqualified"*

Default is "no", and changing to "unqualified" will compile modules at
deeper directories.

## Escape hatches for unsupported Dune config

It's always possible that there are features Dune offers are needed and
the options above are not enough. Use `rawBuildConfig` to add
options in a given library or binary. Use `rawBuildConfigFooter` to
add config to the footer.

### rawBuildConfig
#### *type: list(string)*

Example

```json
{
  "src": {
    "rawBuildConfig": [ "(libraries unix)" ],
    "bin": "Main.re"
  }
}
```
### rawBuildConfigFooter
#### *type: list(string)*

Example

```json
{
  "src": {
    "rawBuildConfigFooter": [ "(install (section share_root) (files (plaintext.txt as asset.txt)))" ]
  }
}
```

[A working example](https://github.com/prometheansacrifice/pesy-samples/blob/master/raw/package.json)
