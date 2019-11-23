open Lwt;

Library.Foo.foo();
Library.Util.foo();

let%lwt foo = Lwt.return("world");
print_endline(foo);
