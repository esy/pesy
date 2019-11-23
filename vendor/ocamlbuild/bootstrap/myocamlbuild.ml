(* You cannot use a myocamlbuild.ml here: depending on
   OCamlbuild_plugin will link with both the local library and the
   installed version, and you will have weird build errors as soon as
   their signatures differ.

   This is an unfortunate side-effect of using ocamlbuild to build its
   own plugin: it is too clever to *not* link with the local library.
*)
