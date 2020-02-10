/*         OS.Dir.create(~path=true, Fpath.(v(cwd) / "virtual-foo")) */ /*         >>= ( */ /*       L.withLog("Editing source: Add file virtual-foo/VirtualFoo.rei", () => */ /*     () => */ /*   >>= ( */
                                                                    /*           _ => */
                                                                    /*             OS.File.write( */
                                                                    /*               Fpath.(v(cwd) / "virtual-foo" / "VirtualFoo.rei"), */
                                                                    /*               {| */
                                                                                     /* let thisWillBeImplementedLater: unit => unit; */
                                                                                     /* |}, */
                                                                    /*             ) */
                                                                    /*         ) */
                                                                    /*       ) */
                                                                    /*       >>= ( */
                                                                    /*         () => */
                                                                    /*           L.withLog( */
                                                                    /*             "Editing source: Add file implementation-bar/VirtualFoo.re", () => */
                                                                    /*             OS.Dir.create(~path=true, Fpath.(v(cwd) / "implementation-bar")) */
                                                                    /*             >>= ( */
                                                                    /*               _ => */
                                                                    /*                 OS.File.write( */
                                                                    /*                   Fpath.(v(cwd) / "implementation-bar" / "VirtualFoo.re"), */
                                                                    /*                   {| */
                                                                                         /* let thisWillBeImplementedLater = () => { */
                                                                                         /*   print_endline("From implementation bar..."); */
                                                                                         /* }; */
                                                                                         /* |}, */
                                                                    /*                 ) */
                                                                    /*             ) */
                                                                    /*           ) */
                                                                    /*           >>= ( */
                                                                    /*             _ => */
                                                                    /*               L.withLog( */
                                                                    /*                 "Editing source: Add file implementation-baz/VirtualFoo.re", () => */
                                                                    /*                 OS.Dir.create( */
                                                                    /*                   ~path=true, */
                                                                    /*                   Fpath.(v(cwd) / "implementation-baz"), */
                                                                    /*                 ) */
                                                                    /*                 >>= ( */
                                                                    /*                   _ => */
                                                                    /*                     OS.File.write( */
                                                                    /*                       Fpath.(v(cwd) / "implementation-baz" / "VirtualFoo.re"), */
                                                                    /*                       {| */
                                                                                             /* let thisWillBeImplementedLater = () => { */
                                                                                             /*   print_endline("From implementation baz..."); */
                                                                                             /* }; */
                                                                                             /* |}, */
                                                                    /*                     ) */
                                                                    /*                 ) */
                                                                    /*               ) */
                                                                    /*               >>= ( */
                                                                    /*                 _ => */
                                                                    /*                   L.withLog( */
                                                                    /*                     "Editing source: Add file executable-virutal-bar/PesyVirtualApp.re", */
                                                                    /*                     () => */
                                                                    /*                     OS.Dir.create( */
                                                                    /*                       ~path=true, */
                                                                    /*                       Fpath.(v(cwd) / "executable-virtual-bar"), */
                                                                    /*                     ) */
                                                                    /*                     >>= ( */
                                                                    /*                       _ => */
                                                                    /*                         OS.File.write( */
                                                                    /*                           Fpath.( */
                                                                    /*                             v(cwd) */
                                                                    /*                             / "executable-virtual-bar" */
                                                                    /*                             / "PesyVirtualApp.re" */
                                                                    /*                           ), */
                                                                    /*                           {| */
                                                                                                 /* Bar.thisWillBeImplementedLater(); */
                                                                                                 /* |}, */
                                                                    /*                         ) */
                                                                    /*                     ) */
                                                                    /*                   ) */
                                                                    /*               ) */
                                                                    /*               >>= ( */
                                                                    /*                 _ => */
                                                                    /*                   L.withLog( */
                                                                    /*                     "Editing source: Add file executable-virtual-baz/PesyVirtualApp.re", */
                                                                    /*                     () => */
                                                                    /*                     OS.Dir.create( */
                                                                    /*                       ~path=true, */
                                                                    /*                       Fpath.(v(cwd) / "executable-virtual-baz"), */
                                                                    /*                     ) */
                                                                    /*                     >>= ( */
                                                                    /*                       _ => */
                                                                    /*                         OS.File.write( */
                                                                    /*                           Fpath.( */
                                                                    /*                             v(cwd) */
                                                                    /*                             / "executable-virtual-baz" */
                                                                    /*                             / "PesyVirtualApp.re" */
                                                                    /*                           ), */
                                                                    /*                           {| */
                                                                                                 /* Baz.thisWillBeImplementedLater(); */
                                                                                                 /* |}, */
                                                                    /*                         ) */
                                                                    /*                     ) */
                                                                    /*                   ) */
                                                                    /*               ) */
                                                                    /*               >>= checkPesyConfig("add virtual modules", () => */
                                                                    /*                     PesyConfig.add( */
                                                                    /*                       {| */
                                                                                             /* { */
                                                                                             /*     "virtual-foo": { */
                                                                                             /*       "virtualModules": [ */
                                                                                             /*         "VirtualFoo" */
                                                                                             /*       ] */
                                                                                             /*     }, */
                                                                                             /*     "implementation-bar": { */
                                                                                             /*       "implements": [ */
                                                                                             /*         "test-project/virtual-foo" */
                                                                                             /*       ] */
                                                                                             /*     }, */
                                                                                             /*     "implementation-baz": { */
                                                                                             /*       "implements": [ */
                                                                                             /*         "test-project/virtual-foo" */
                                                                                             /*       ] */
                                                                                             /*     }, */
                                                                                             /*     "executable-virtual-bar": { */
                                                                                             /*       "imports": [ */
                                                                                             /*         "Bar = require('test-project/implementation-bar')" */
                                                                                             /*       ], */
                                                                                             /*       "bin": { */
                                                                                             /*         "PesyVirtualAppBar.exe": "PesyVirtualApp.re" */
                                                                                             /*       } */
                                                                                             /*     }, */
                                                                                             /*     "executable-virtual-baz": { */
                                                                                             /*       "imports": [ */
                                                                                             /*         "Baz = require('test-project/implementation-baz')" */
                                                                                             /*       ], */
                                                                                             /*       "bin": { */
                                                                                             /*         "PesyVirtualAppBaz.exe": "PesyVirtualApp.re" */
                                                                                             /*       } */
                                                                                             /*     } */
                                                                                             /* } */
                                                                                             /* |}, */
                                                                    /*                       Path.(cwd / "package.json"), */
                                                                    /*                     ) */
                                                                    /*                   ) */
                                                                    /*           ) */
                                                                    /*       ) */
                                                                    /*   ); */
