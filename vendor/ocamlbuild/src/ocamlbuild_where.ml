(***********************************************************************)
(*                                                                     *)
(*                             ocamlbuild                              *)
(*                                                                     *)
(*  Nicolas Pouillard, Berke Durak, projet Gallium, INRIA Rocquencourt *)
(*                                                                     *)
(*  Copyright 2007 Institut National de Recherche en Informatique et   *)
(*  en Automatique.  All rights reserved.  This file is distributed    *)
(*  under the terms of the GNU Library General Public License, with    *)
(*  the special exception on linking described in file ../LICENSE.     *)
(*                                                                     *)
(***********************************************************************)

module O = Ocamlbuild_config;;

let bindir = ref O.bindir;;
let libdir = ref begin
  let root, suffix =
    let ocaml_lib_len = String.length O.ocaml_libdir + 1 in
    let lib_len = String.length O.libdir_abs in
    (* Windows note: O.ocaml_libdir and O.libdir_abs have both been passed
       through GNU make's abspath function and will be forward-slash normalised.
       Filename.dir_sep is therefore not appropriate here. *)
    if lib_len < ocaml_lib_len
       || String.sub O.libdir_abs 0 ocaml_lib_len <> O.ocaml_libdir ^ "/" then
      O.libdir, "ocamlbuild"
    else
      (* https://github.com/ocaml/ocamlbuild/issues/69. Only use OCAMLLIB if
         the configured LIBDIR is a subdirectory (lexically) of OCAML_LIBDIR.
         If it is, append the difference between LIBDIR and OCAML_LIBDIR to
         OCAMLLIB. This allows `OCAMLLIB=/foo ocamlbuild -where` to return
         /foo/site-lib/ocamlbuild for a findlib-based installation and also
         to ignore OCAMLLIB in an opam-based installation (where setting
         OCAMLLIB is already a strange thing to have done). *)
      try
        let normalise_slashes =
          if Sys.os_type = "Win32" then
            String.map (function '/' -> '\\' | c -> c)
          else
            function s -> s
        in
        let subroot =
          String.sub O.libdir_abs ocaml_lib_len (lib_len - ocaml_lib_len)
          |> normalise_slashes
        in
        Sys.getenv "OCAMLLIB", Filename.concat subroot "ocamlbuild"
      with Not_found -> O.libdir, "ocamlbuild"
  in
  Filename.concat root suffix
end;;
