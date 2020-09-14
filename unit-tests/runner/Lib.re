module TestFramework = {
  include Rely.Make({
    let config =
      Rely.TestFrameworkConfig.initialize({
        snapshotDir: "_build/default/unit-tests",
        projectDir: "",
      });
  });
};

open TestFramework;
open PesyEsyPesyLib;
open PesyConf;
open PesyEsyPesyErrors.Errors;
open PesyEsyPesyUtils.Utils;

describe("PesyConf.resolveRelativePath", ({test, _}) => {
  test("simple case", ({expect, _}) =>
    expect.string(resolveRelativePath("foo/lib")).toEqual("foo/lib")
  );
  test("with ../ in the path", ({expect, _}) =>
    expect.string(resolveRelativePath("foo/bar/..")).toEqual("foo")
  );
  test("with ../../ in the path", ({expect, _}) =>
    expect.string(resolveRelativePath("foo/bar/../..")).toEqual("")
  );
  test("with excessive ../", ({expect, _}) =>
    expect.fn(() => resolveRelativePath("foo/bar/../../..")).toThrowException(
      ResolveRelativePathFailure(
        "Failed resolving: foo/bar/../../.. Too many `../`",
      ),
    )
  );
  test("foo/bar/../baz/", ({expect, _}) =>
    expect.string(resolveRelativePath("foo/bar/../baz/")).toEqual("foo/baz")
  );
});

describe("PesyConf.moduleNameOf", ({test, _}) => {
  test("simple case .re file", ({expect, _}) =>
    expect.string(moduleNameOf("Foo.re")).toEqual("Foo")
  );
  test("simple case .ml file", ({expect, _}) =>
    expect.string(moduleNameOf("Foo.ml")).toEqual("Foo")
  );
});

describe("PesyConf.isValidBinaryFileName", ({test, _}) => {
  test("For an invalid binary", ({expect, _}) =>
    expect.bool(isValidBinaryFileName("Foo.re")).toBeFalse()
  );
  test("For a valid binary", ({expect, _}) =>
    expect.bool(isValidBinaryFileName("Foo.exe")).toBeTrue()
  );
});

describe("PesyConf.isValidSourceFile", ({test, _}) => {
  test("For an invalid source file", ({expect, _}) =>
    expect.bool(isValidSourceFile("Foo.exe")).toBeFalse()
  );
  test("For a valid source file", ({expect, _}) =>
    expect.bool(isValidSourceFile("Foo.ml")).toBeTrue()
  );
  test("For a valid source file", ({expect, _}) =>
    expect.bool(isValidSourceFile("Foo.re")).toBeTrue()
  );
});

describe("PesyConf.isValidScopeName", ({test, _}) => {
  test("For an invalid case", ({expect, _}) =>
    expect.bool(isValidScopeName("myscope")).toBeFalse()
  );
  test("For a valid case", ({expect, _}) =>
    expect.bool(isValidScopeName("@myscope")).toBeTrue()
  );
});

describe("PesyConf.doubleKebabifyIfScoped", ({test, _}) => {
  test("test foo-bar", ({expect, _}) =>
    expect.string(doubleKebabifyIfScoped("foo-bar")).toEqual("foo-bar")
  )
});

let testToPackages = (~duneVersion="1.11", jsonStr) => {
  let json = JSON.ofString(jsonStr);
  let pkgs = PesyConf.pkgs(json);
  let pesyPackages = List.map(toPesyConf("", "", ~duneVersion), pkgs);
  let dunePackages = List.map(toDunePackages("", ""), pesyPackages);
  List.map(
    p => {
      let (_, d) = p;
      d;
    },
    dunePackages,
  );
};

describe("PesyConf.testToPackages", ({test, _}) => {
  test("Sample config - 1", ({expect, _}) =>
    expect.list(
      testToPackages(
        {|
           {
             "name": "foo",
             "buildDirs": {
               "test": {
                 "require": ["foo"],
                 "bin": { "Bar.exe": "Bar.re" }
               }
             }
           }
                |},
      )
      |> List.map(DuneFile.toString),
    ).
      toEqual([
      "(executables (names Bar) (modules (:standard)) (public_names Bar.exe)\n    (libraries foo))\n",
    ])
  );
  test("Sample config - 2", ({expect, _}) =>
    expect.list(
      testToPackages(
        {|
              {
                  "name": "foo",
                  "buildDirs": {
                  "testlib": {
                    "require": ["foo"],
                    "namespace": "Foo",
                    "name": "bar.lib",
                   "modes": ["best"]
                  }
                }
              }
            |},
      )
      |> List.map(DuneFile.toString),
    ).
      toEqual([
      "(library (name Foo) (public_name bar.lib) (modules (:standard))\n    (libraries foo) (modes best))\n",
    ])
  );
  test("Sample config - 3", ({expect, _}) =>
    expect.list(
      testToPackages(
        {|
           {
               "name": "foo",
               "buildDirs": {
               "testlib": {
                 "require": ["foo"],
                 "namespace": "Foo",
                 "name": "bar.lib",
                 "cNames": ["stubs"]
               }
             }
           }
         |},
      )
      |> List.map(DuneFile.toString),
    ).
      toEqual([
      "(library (name Foo) (public_name bar.lib) (modules (:standard))\n    (libraries foo) (c_names stubs))\n",
    ])
  );
  test("Sample config - 4", ({expect, _}) =>
    expect.list(
      testToPackages(
        {|
           {
               "name": "foo",
               "buildDirs": {
               "testlib": {
                 "namespace": "Foo",
                 "name": "bar.lib",
                 "implements": ["foo"]
               }
             }
           }
         |},
      )
      |> List.map(DuneFile.toString),
    ).
      toEqual([
      "(library (name Foo) (public_name bar.lib) (modules (:standard))\n    (implements foo))\n",
    ])
  );
  test("Sample config - 5", ({expect, _}) =>
    expect.list(
      testToPackages(
        {|
           {
               "name": "foo",
               "buildDirs": {
               "testlib": {
                 "namespace": "Foo",
                 "name": "bar.lib",
                 "wrapped": false
               }
             }
           }
         |},
      )
      |> List.map(DuneFile.toString),
    ).
      toEqual([
      "(library (name Foo) (public_name bar.lib) (modules (:standard))\n    (wrapped false))\n",
    ])
  );
  test("Sample config - 6", ({expect, _}) =>
    expect.list(
      testToPackages(
        {|
              {
                  "name": "foo",
                  "buildDirs": {
                  "testlib": {
                    "bin": { "bar.exe": "Foo.re" },
                    "modes": ["best", "c"]
                  }
                }
              }
            |},
      )
      |> List.map(DuneFile.toString),
    ).
      toEqual([
      "(executables (names Foo) (modules (:standard)) (public_names bar.exe)\n    (modes (best c)))\n",
    ])
  );
  test("Sample config - 7", ({expect, _}) =>
    expect.list(
      testToPackages(
        {|
                {
                    "name": "foo",
                    "buildDirs": {
                    "testlib": {
                        "require": ["foo"],
                        "namespace": "Foo",
                        "name": "bar.lib",
                        "flags": ["-w", "-33+9"]
                    }
                    }
                }
         |},
      )
      |> List.map(DuneFile.toString),
    ).
      toEqual([
      "(library (name Foo) (public_name bar.lib) (modules (:standard))\n    (libraries foo) (flags -w -33+9))\n",
    ])
  );
  test("Sample config - 8", ({expect, _}) =>
    expect.list(
      testToPackages(
        {|
                    {
                      "name": "foo",
                      "buildDirs": {
                        "testlib": {
                          "namespace": "Foo",
                          "name": "bar.lib",
                          "ocamlcFlags": ["-annot", "-c"]
                        }
                      }
                    }

         |},
      )
      |> List.map(DuneFile.toString),
    ).
      toEqual([
      "(library (name Foo) (public_name bar.lib) (modules (:standard))\n    (ocamlc_flags -annot -c))\n",
    ])
  );
  test("Sample config - 9", ({expect, _}) =>
    expect.list(
      testToPackages(
        {|
    {
        "name": "foo",
        "buildDirs": {
            "testlib": {
                "namespace": "Foo",
                "name": "bar.lib",
                "ocamloptFlags": ["-rectypes", "-nostdlib"]
            }
        }
    }

         |},
      )
      |> List.map(DuneFile.toString),
    ).
      toEqual([
      "(library (name Foo) (public_name bar.lib) (modules (:standard))\n    (ocamlopt_flags -rectypes -nostdlib))\n",
    ])
  );
  test("Sample config - 10", ({expect, _}) =>
    expect.list(
      testToPackages(
        {|

                    {
                      "name": "foo",
                      "buildDirs": {
                        "testlib": {
                          "namespace": "Foo",
                          "name": "bar.lib",
                          "jsooFlags": ["-pretty", "-no-inline"]
                        }
                      }
                    }

         |},
      )
      |> List.map(DuneFile.toString),
    ).
      toEqual([
      "(library (name Foo) (public_name bar.lib) (modules (:standard))\n    (js_of_ocaml -pretty -no-inline))\n",
    ])
  );
  test("Sample config - 11", ({expect, _}) =>
    expect.list(
      testToPackages(
        {|

                   {
                      "name": "foo",
                      "buildDirs": {
                        "testlib": {
                          "namespace": "Foo",
                          "name": "bar.lib",
                          "preprocess": [ "pps", "lwt_ppx" ]
                        }
                      }
                    }
         |},
      )
      |> List.map(DuneFile.toString),
    ).
      toEqual([
      "(library (name Foo) (public_name bar.lib) (modules (:standard))\n    (preprocess (pps lwt_ppx)))\n",
    ])
  );
  test("Sample config - 12", ({expect, _}) =>
    expect.list(
      testToPackages(
        {|

                    {
                      "name": "foo",
                      "buildDirs": {
                        "testlib": {
                          "namespace": "Foo",
                          "name": "bar.lib",
                          "includeSubdirs": "unqualified"
                        }
                      }
                    }

         |},
      )
      |> List.map(DuneFile.toString),
    ).
      toEqual([
      "(library (name Foo) (public_name bar.lib) (modules (:standard)))\n(include_subdirs unqualified)\n",
    ])
  );
  test("Sample config - 12", ({expect, _}) =>
    expect.list(
      testToPackages(
        {|

                    {
                      "name": "foo",
                      "buildDirs": {
                        "testlib": {
                          "namespace": "Foo",
                          "name": "bar.lib",
                          "includeSubdirs": "unqualified"
                        }
                      }
                    }

         |},
      )
      |> List.map(DuneFile.toString),
    ).
      toEqual([
      "(library (name Foo) (public_name bar.lib) (modules (:standard)))\n(include_subdirs unqualified)\n",
    ])
  );
  test("Sample config - 14", ({expect, _}) =>
    expect.list(
      testToPackages(
        {|
{
                      "name": "foo",
                      "buildDirs": {
                        "testexe": {
                          "bin": { "Foo.exe": "Foo.re" },
                          "rawBuildConfigFooter": [
                            "(install (section share_root) (files (asset.txt as asset.txt)))"
                          ]
                        }
                      }
                    }


         |},
      )
      |> List.map(DuneFile.toString),
    ).
      toEqual([
      "(executables (names Foo) (modules (:standard)) (public_names Foo.exe))\n(install (section share_root) (files (asset.txt as asset.txt)))\n",
    ])
  );
  test("Sample config - 14", ({expect, _}) =>
    expect.list(
      testToPackages(
        {|
                   {
                      "name": "foo",
                      "buildDirs": {
                        "testexe": {
                          "bin": { "Foo.exe": "Foo.re" }
                        }
                      }
                    }  |},
      )
      |> List.map(DuneFile.toString),
    ).
      toEqual([
      "(executables (names Foo) (modules (:standard)) (public_names Foo.exe))\n",
    ])
  );
  test("Sample config - 15", ({expect, _}) =>
    expect.list(
      testToPackages(
        {|
                    {
                      "name": "foo",
                      "buildDirs": {
                        "testlib": {
                          "namespace": "Foo",
                          "name": "bar.lib",
                          "rawBuildConfigFooter": [
                            "(install (section share_root) (files (asset.txt as asset.txt)))"
                          ]
                        }
                      }
                    }
|},
      )
      |> List.map(DuneFile.toString),
    ).
      toEqual([
      "(library (name Foo) (public_name bar.lib) (modules (:standard)))\n(install (section share_root) (files (asset.txt as asset.txt)))\n",
    ])
  );
  test("Sample config - 16", ({expect, _}) =>
    expect.list(
      testToPackages(
        {|
                    {
                      "name": "foo",
                      "buildDirs": {
                        "testlib": {
                          "namespace": "Foo",
                          "name": "bar.lib",
                          "rawBuildConfig": [
                            "(libraries lwt lwt.unix raw.lib)",
                            "(preprocess (pps lwt_ppx))"
                          ]
                        }
                      }
                    }
|},
      )
      |> List.map(DuneFile.toString),
    ).
      toEqual([
      "(library (name Foo) (public_name bar.lib) (modules (:standard))\n    (libraries lwt lwt.unix raw.lib) (preprocess (pps lwt_ppx)))\n",
    ])
  );

  test("Sample config - 17", ({expect, _}) =>
    expect.list(
      testToPackages(
        {|
                {
                    "name": "foo",
                    "buildDirs": {
                    "testlib": {
                        "require": ["foo"],
                        "namespace": "Foo",
                        "name": "bar.lib",
                        "flags": "-verbose"
                    }
                    }
                }
         |},
      )
      |> List.map(DuneFile.toString),
    ).
      toEqual([
      "(library (name Foo) (public_name bar.lib) (modules (:standard))\n    (libraries foo) (flags -verbose))\n",
    ])
  );
  test("Sample config - 18", ({expect, _}) =>
    expect.list(
      testToPackages(
        {|
                    {
                      "name": "foo",
                      "buildDirs": {
                        "testlib": {
                          "namespace": "Foo",
                          "name": "bar.lib",
                          "ocamlcFlags": "-annot -c"
                        }
                      }
                    }

         |},
      )
      |> List.map(DuneFile.toString),
    ).
      toEqual([
      "(library (name Foo) (public_name bar.lib) (modules (:standard))\n    (ocamlc_flags -annot -c))\n",
    ])
  );
  test("Sample config - 19", ({expect, _}) =>
    expect.list(
      testToPackages(
        {|
    {
        "name": "foo",
        "buildDirs": {
            "testlib": {
                "namespace": "Foo",
                "name": "bar.lib",
                "ocamloptFlags": "-rectypes -nostdlib"
            }
        }
    }

         |},
      )
      |> List.map(DuneFile.toString),
    ).
      toEqual([
      "(library (name Foo) (public_name bar.lib) (modules (:standard))\n    (ocamlopt_flags -rectypes -nostdlib))\n",
    ])
  );
  test("Sample config - 20", ({expect, _}) =>
    expect.list(
      testToPackages(
        {|

                    {
                      "name": "foo",
                      "buildDirs": {
                        "testlib": {
                          "namespace": "Foo",
                          "name": "bar.lib",
                          "jsooFlags": "-pretty -no-inline"
                        }
                      }
                    }

         |},
      )
      |> List.map(DuneFile.toString),
    ).
      toEqual([
      "(library (name Foo) (public_name bar.lib) (modules (:standard))\n    (js_of_ocaml -pretty -no-inline))\n",
    ])
  );
  test("Sample config - 21", ({expect, _}) =>
    expect.list(
      testToPackages(
        {|
             {
                 "name": "foo",
                 "buildDirs": {
                 "testlib": {
                   "bin": { "bar.bc.js": "Foo.re" },
                   "modes": "js"
                 }
               }
             }
           |},
      )
      |> List.map(DuneFile.toString),
    ).
      toEqual([
      "(executables (names Foo) (modules (:standard)) (public_names bar.bc.js)\n    (modes js))\n",
    ])
  );
  test("Sample config - 22", ({expect, _}) =>
    expect.list(
      testToPackages(
        {|
                  {
                      "name": "foo",
                      "buildDirs": {
                      "testlib": {
                        "bin": { "bar.exe": "Foo.re" },
                        "modes": "exe object shared_object"
                      }
                    }
                  }
                |},
      )
      |> List.map(DuneFile.toString),
    ).
      toEqual([
      "(executables (names Foo) (modules (:standard)) (public_names bar.exe)\n    (modes exe object shared_object))\n",
    ])
  );
  test("Sample config - 23", ({expect, _}) =>
    expect.list(
      testToPackages(
        {|
                    {
                        "name": "foo",
                        "buildDirs": {
                        "testlib": {
                          "bin": { "bar.exe": "Foo.re" },
                          "modes": ["js", "plugin"]
                        }
                      }
                    }
                  |},
      )
      |> List.map(DuneFile.toString),
    ).
      toEqual([
      "(executables (names Foo) (modules (:standard)) (public_names bar.exe)\n    (modes js plugin))\n",
    ])
  );

  test("Sample config - 24", ({expect, _}) =>
    expect.list(
      testToPackages(
        {|
             {
                 "name": "foo",
                 "buildDirs": {
                 "testlib": {
                   "bin": { "bar.exe": "Foo.re" },
                   "modes": [["byte", "exe"], ["byte", "js"]]
                 }
               }
             }
           |},
      )
      |> List.map(DuneFile.toString),
    ).
      toEqual([
      "(executables (names Foo) (modules (:standard)) (public_names bar.exe)\n    (modes (byte exe) (byte js)))\n",
    ])
  );

  test("Sample config - 25", ({expect, _}) =>
    expect.list(
      testToPackages(
        {|
             {
                 "name": "foo",
                 "buildDirs": {
                 "testlib": {
                   "bin": { "bar.exe": "Foo.re" },
                   "modes": ["byte", "exe"]
                 }
               }
             }
           |},
      )
      |> List.map(DuneFile.toString),
    ).
      toEqual([
      "(executables (names Foo) (modules (:standard)) (public_names bar.exe)\n    (modes (byte exe)))\n",
    ])
  );

  test("Sample config - 26", ({expect, _}) =>
    expect.list(
      testToPackages(
        {|
                    {
                        "name": "foo",
                        "buildDirs": {
                        "testlib": {
                          "bin": { "bar.exe": "Foo.re" },
                          "modes": ["exe"]
                        }
                      }
                    }
                  |},
      )
      |> List.map(DuneFile.toString),
    ).
      toEqual([
      "(executables (names Foo) (modules (:standard)) (public_names bar.exe)\n    (modes exe))\n",
    ])
  );

  test("Sample config - 27", ({expect, _}) =>
    expect.list(
      testToPackages(
        {|
         {
             "name": "foo",
             "buildDirs": {
             "testlib": {
               "require": ["foo"],
               "namespace": "Foo",
               "name": "bar.lib",
               "foreignStubs": [
                  {
                    "language": "c"
                  }
                ]
             }
           }
         }
       |},
        ~duneVersion="2.0",
      )
      |> List.map(DuneFile.toString),
    ).
      toEqual([
      "(library (name Foo) (public_name bar.lib) (modules (:standard))\n    (libraries foo)\n    (foreign_stubs (language c) (names :standard) (flags :standard)))\n",
    ])
  );

  test("Sample config - 28", ({expect, _}) =>
    expect.list(
      testToPackages(
        {|
         {
             "name": "foo",
             "buildDirs": {
             "testlib": {
               "require": ["foo"],
               "namespace": "Foo",
               "name": "bar.lib",
               "foreignStubs": [
                  {
                    "language": "c",
                    "names": ["src1", "src2"],
                    "flags": []
                  }
                ]
             }
           }
         }
       |},
        ~duneVersion="2.0",
      )
      |> List.map(DuneFile.toString),
    ).
      toEqual([
      "(library (name Foo) (public_name bar.lib) (modules (:standard))\n    (libraries foo)\n    (foreign_stubs (language c) (names src1 src2) (flags :standard)))\n",
    ])
  );

  test("Sample config - 29", ({expect, _}) =>
    expect.list(
      testToPackages(
        {|
         {
             "name": "foo",
             "buildDirs": {
             "testlib": {
               "require": ["foo"],
               "namespace": "Foo",
               "name": "bar.lib",
               "foreignStubs": [
                  {
                    "language": "c",
                    "names": ["src1", "src2"]
                  },
                  {
                    "language": "cxx",
                    "names": ["src3"],
                    "flags": ["-02"]
                  }
                ]
             }
           }
         }
       |},
        ~duneVersion="2.0",
      )
      |> List.map(DuneFile.toString),
    ).
      toEqual([
      "(library (name Foo) (public_name bar.lib) (modules (:standard))\n    (libraries foo)\n    (foreign_stubs (language c) (names src1 src2) (flags :standard))\n    (foreign_stubs (language cxx) (names src3) (flags -02)))\n",
    ])
  );

  test("Sample config - 30", ({expect, _}) =>
    expect.list(
      testToPackages(
        {|
         {
             "name": "foo",
             "buildDirs": {
             "testlib": {
               "require": ["foo"],
               "namespace": "Foo",
               "name": "bar.lib",
               "cNames": ["stubs"],
               "foreignStubs": [
                  {
                    "language": "c",
                    "names": ["src1", "src2"]
                  },
                  {
                    "language": "cxx",
                    "names": ["src3"],
                    "flags": ["-02"]
                  }
                ]
             }
           }
         }
       |},
        ~duneVersion="2.0",
      )
      |> List.map(DuneFile.toString),
    ).
      toEqual([
      "(library (name Foo) (public_name bar.lib) (modules (:standard))\n    (libraries foo)\n    (foreign_stubs (language c) (names src1 src2) (flags :standard))\n    (foreign_stubs (language cxx) (names src3) (flags -02)))\n",
    ])
  );

  test("Sample config - 31", ({expect, _}) =>
    expect.list(
      testToPackages(
        {|
         {
             "name": "foo",
             "buildDirs": {
             "testlib": {
               "require": ["foo"],
               "namespace": "Foo",
               "name": "bar.lib",
               "cNames": ["stub1", "stub2"],
               "foreignStubs": [
                  {
                    "language": "c",
                    "names": ["src1", "src2"]
                  },
                  {
                    "language": "cxx",
                    "names": ["src3"],
                    "flags": ["-02"]
                  }
                ]
             }
           }
         }
       |},
        ~duneVersion="1.11",
      )
      |> List.map(DuneFile.toString),
    ).
      toEqual([
      "(library (name Foo) (public_name bar.lib) (modules (:standard))\n    (libraries foo) (c_names stub1 stub2))\n",
    ])
  );
});