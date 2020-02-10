---
id: publishing-consuming-from-npm
title: Publishing and Consuming from NPM
---

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
      "imports": [
        "Library = require('foo/library')",
        "Rely = require('rely/lib')"
      ],
      "flags": [
        "-linkall",
        "-g"
      ]
    },
    "testExe": {
      "imports": [
        "Test = require('foo/test')"
      ],
      "bin": {
        "RunFooTests.exe": "RunFooTests.re"
      }
    },
    "library": {},
    "executable": {
      "imports": [
        "Library = require('foo/library')"
      ],
      "bin": {
        "FooApp.exe": "FooApp.re"
      }
    }
  },
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

We can now require `foo` (sort of like we do in Javascript)

```diff
  "buildDirs": {
    "test": {
      "imports": [
        "Library = require('bar/library')",
        "Rely = require('rely.lib')"
      ],
      "flags": [
        "-linkall",
        "-g"
      ]
    },
    "testExe": {
      "imports": [
        "Test = require('bar/test')"
      ],
      "bin": {
        "RunBarTests.exe": "RunBarTests.re"
      }
    },
    "library": {
      "imports": [
+        "FooLib = require('foo/library')"
      ]
    },
    "executable": {
      "imports": [
        "Library = require('bar/library')"
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
  FooLib.Util.foo();
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

Publishing and consuming Reason native packages from NPM is easy. We
just need to keep in mind that the namespace exposed from our library
depends on the name of the package and name of the folder! It's best
to stay out of trouble using using the new require syntax.

