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


(* Original author: Nicolas Pouillard *)
open My_std
open Format
open Log
open Pathname.Operators
open Tags.Operators
open Rule
open Tools
open Rule.Common_commands
open Outcome
open Command;;

open Ocaml_utils

module C_tools = struct
  let link_C_library clib a libname env build =
    let clib = env clib and a = env a and libname = env libname in
    let objs = string_list_of_file clib in
    let include_dirs = Pathname.include_dirs_of (Pathname.dirname a) in
    let obj_of_o x =
      if Filename.check_suffix x ".o" && !Options.ext_obj <> "o" then
        Pathname.update_extension !Options.ext_obj x
      else x in
    let results = build (List.map (fun o -> List.map (fun dir -> dir / obj_of_o o) include_dirs) objs) in
    let objs = List.map begin function
      | Good o -> o
      | Bad exn -> raise exn
    end results in
    Cmd(S[!Options.ocamlmklib; A"-o"; Px libname; T(tags_of_pathname a++"c"++"ocamlmklib"); atomize objs]);;

  let make_single_library cobj_base clib env _build =
    Echo ([env cobj_base; ".o"; "\n"], env clib)
end

open Flags
open Command
open Rule

let init () = let module M = struct

let ext_lib = !Options.ext_lib;;
let ext_obj = !Options.ext_obj;;
let ext_dll = !Options.ext_dll;;
let x_o = "%"-.-ext_obj;;
let x_a = "%"-.-ext_lib;;
let x_dll = "%"-.-ext_dll;;
let x_p_o = "%.p"-.-ext_obj;;
let x_p_a = "%.p"-.-ext_lib;;
let x_p_dll = "%.p"-.-ext_dll;;

(* -output-obj targets *)
let x_byte_c = "%.byte.c";;
let x_byte_o = "%.byte"-.-ext_obj;;
let x_byte_so = "%.byte"-.-ext_dll;;
let x_native_o = "%.native"-.-ext_obj;;
let x_native_so = "%.native"-.-ext_dll;;

rule "target files"
  ~dep:"%.itarget"
  ~stamp:"%.otarget"
  ~doc:"If foo.itarget contains a list of ocamlbuild targets, \
        asking ocamlbuild to produce foo.otarget will \
        build each of those targets in turn."
  begin fun env build ->
    let itarget = env "%.itarget" in
    let targets =
      let dir = Pathname.dirname itarget in
      let files = string_list_of_file itarget in
      List.map (fun file -> [Pathname.concat dir file]) files
    in
    let results = List.map Outcome.good (build targets) in
    let link_command result =
      Cmd (S [A "ln"; A "-sf";
              P (Pathname.concat !Options.build_dir result);
              A Pathname.pwd])
    in
    if not !Options.make_links
    then Nop
    else Seq (List.map link_command results)
  end;;

rule "ocaml: mli -> cmi"
  ~prod:"%.cmi"
  ~deps:["%.mli"; "%.mli.depends"]
  (Ocaml_compiler.compile_ocaml_interf "%.mli" "%.cmi");;

rule "ocaml: mlpack & d.cmo* -> d.cmo & cmi"
  ~prods:["%.d.cmo"]
  ~deps:["%.mlpack"; "%.cmi"]
  (Ocaml_compiler.byte_debug_pack_mlpack "%.mlpack" "%.d.cmo");;

rule "ocaml: mlpack & cmo* & cmi -> cmo"
  ~prod:"%.cmo"
  ~deps:["%.mli"; "%.cmi"; "%.mlpack"]
  ~doc:"If foo.mlpack contains a list of capitalized module names, \
  the target foo.cmo will produce a packed module containing \
  those modules as submodules. You can also have a foo.mli file \
  to restrict the interface of the resulting module.

\
  Warning: to produce a native foo.cmx out of a foo.mlpack, you must \
  manually tag the included compilation units with for-pack(foo). \
  See the documentation of the corresponding rules for more details.

\
  The modules named in the .mlpack \
  will be dynamic dependencies of the compilation action. \
  You cannot give the .mlpack the same name as one of the module \
  it contains, as this would create a circular dependency."
  (Ocaml_compiler.byte_pack_mlpack "%.mlpack" "%.cmo");;

rule "ocaml: mlpack & cmo* -> cmo & cmi"
  ~prods:["%.cmo"; "%.cmi"]
  ~dep:"%.mlpack"
  (Ocaml_compiler.byte_pack_mlpack "%.mlpack" "%.cmo");;

rule "ocaml: ml & cmi -> d.cmo"
  ~prod:"%.d.cmo"
  ~deps:["%.mli"(* This one is inserted to force this rule to be skiped when
    a .ml is provided without a .mli *); "%.ml"; "%.ml.depends"; "%.cmi"]
  ~doc:"The foo.d.cmo target compiles foo.ml with the 'debug' tag enabled (-g).\
        See also foo.d.byte.

\
        For technical reason, .d.cmx and .d.native are not yet supported, \
        so you should explicitly add the 'debug' tag \
        to native targets (both compilation and linking)."
  (Ocaml_compiler.byte_compile_ocaml_implem ~tag:"debug" "%.ml" "%.d.cmo");;

rule "ocaml: ml & cmi -> cmo"
  ~prod:"%.cmo"
  ~deps:["%.mli"(* This one is inserted to force this rule to be skiped when
    a .ml is provided without a .mli *); "%.ml"; "%.ml.depends"; "%.cmi"]
  (Ocaml_compiler.byte_compile_ocaml_implem "%.ml" "%.cmo");;

rule "ocaml: mlpack & cmi & p.cmx* & p.o* -> p.cmx & p.o"
  ~prods:["%.p.cmx"; x_p_o
          (* no cmi here you must make the byte version to have it *)]
  ~deps:["%.mlpack"; "%.cmi"]
  (Ocaml_compiler.native_profile_pack_mlpack "%.mlpack" "%.p.cmx");;

rule "ocaml: mlpack & cmi & cmx* & o* -> cmx & o"
  ~prods:["%.cmx"; x_o
          (* no cmi here you must make the byte version to have it *)]
  ~deps:["%.mlpack"; "%.cmi"]
  ~doc:"If foo.mlpack contains a list of capitalized module names, \
  the target foo.cmx will produce a packed module containing \
  those modules as submodules.

\
  Warning: The .cmx files that will be included must be manually tagged \
  with the tag \"for-pack(foo)\". This means that you cannot include \
  the same bar.cmx in several .mlpack files, and that you should not \
  use an included .cmx as a separate module on its own.

\
  This requirement comes from a technical limitation of \
  native module packing: ocamlopt needs the -for-pack argument to be passed \
  ahead of time, when compiling each included submodule, \
  because there is no reliable, portable way to rewrite \
  native object files afterwards."
  (Ocaml_compiler.native_pack_mlpack "%.mlpack" "%.cmx");;

rule "ocaml: ml & cmi -> p.cmx & p.o"
  ~prods:["%.p.cmx"; x_p_o]
  ~deps:["%.ml"; "%.ml.depends"; "%.cmi"]
  ~doc:"The foo.p.cmx target compiles foo.ml with the 'profile' \
       tag enabled (-p). Note that ocamlbuild provides no support \
       for the bytecode profiler, which works completely differently."
  (Ocaml_compiler.native_compile_ocaml_implem ~tag:"profile" ~cmx_ext:"p.cmx" "%.ml");;

rule "ocaml: ml & cmi -> cmx & o"
  ~prods:["%.cmx"; x_o]
  ~deps:["%.ml"; "%.ml.depends"; "%.cmi"]
  (Ocaml_compiler.native_compile_ocaml_implem "%.ml");;

rule "ocaml: ml -> d.cmo & cmi"
  ~prods:["%.d.cmo"]
  ~deps:["%.ml"; "%.ml.depends"; "%.cmi"]
  (Ocaml_compiler.byte_compile_ocaml_implem ~tag:"debug" "%.ml" "%.d.cmo");;

rule "ocaml: ml -> cmo & cmi"
  ~prods:["%.cmo"; "%.cmi"]
  ~deps:["%.ml"; "%.ml.depends"]
  ~doc:"This rule allows to produce a .cmi from a .ml file \
        when the corresponding .mli is missing.

\
        Note: you are strongly encourage to have a .mli file \
        for each of your .ml module, as it is a good development \
        practice which also simplifies the way build systems work, \
        as it avoids producing .cmi files as a silent side-effect of \
        another compilation action."
  (Ocaml_compiler.byte_compile_ocaml_implem "%.ml" "%.cmo");;

rule "ocaml: d.cmo* -> d.byte"
  ~prod:"%.d.byte"
  ~dep:"%.d.cmo"
  ~doc:"The target foo.d.byte will build a bytecode executable \
        with debug information enabled."
  (Ocaml_compiler.byte_debug_link "%.d.cmo" "%.d.byte");;

rule "ocaml: cmo* -> byte"
  ~prod:"%.byte"
  ~dep:"%.cmo"
  (Ocaml_compiler.byte_link "%.cmo" "%.byte");;

rule "ocaml: cmo* -> byte.(o|obj)"
  ~prod:x_byte_o
  ~dep:"%.cmo"
  ~doc:"The foo.byte.o target, or foo.byte.obj under Windows, \
  will produce an object file by passing the -output-obj option \
  to the OCaml compiler. See also foo.byte.c, and foo.native.{o,obj}."
  (Ocaml_compiler.byte_output_obj "%.cmo" x_byte_o);;

rule "ocaml: cmo* -> byte.c"
  ~prod:x_byte_c
  ~dep:"%.cmo"
  (Ocaml_compiler.byte_output_obj "%.cmo" x_byte_c);;

rule "ocaml: cmo* -> byte.(so|dll)"
  ~prod:x_byte_so
  ~dep:"%.cmo"
  ~doc:"The foo.byte.so target, or foo.byte.dll under Windows \
  and macOS will produce a shared library file by passing \
  the -output-obj option to the OCaml compiler. \
  See also foo.native.{so,dll}."
  (Ocaml_compiler.byte_output_shared "%.cmo" x_byte_so);;

rule "ocaml: p.cmx* & p.o* -> p.native"
  ~prod:"%.p.native"
  ~deps:["%.p.cmx"; x_p_o]
  ~doc:"The foo.p.native target builds the native executable \
        with the 'profile' tag (-p) enabled throughout compilation and linking."
  (Ocaml_compiler.native_profile_link "%.p.cmx" "%.p.native");;

rule "ocaml: cmx* & o* -> native"
  ~prod:"%.native"
  ~deps:["%.cmx"; x_o]
  ~doc:"Builds a native executable"
  (Ocaml_compiler.native_link "%.cmx" "%.native");;

rule "ocaml: cmx* & o* -> native.(o|obj)"
  ~prod:x_native_o
  ~deps:["%.cmx"; x_o]
  (Ocaml_compiler.native_output_obj "%.cmx" x_native_o);;

rule "ocaml: cmx* & o* -> native.(so|dll)"
  ~prod:x_native_so
  ~deps:["%.cmx"; x_o]
  (Ocaml_compiler.native_output_shared "%.cmx" x_native_so);;

rule "ocaml: mllib & d.cmo* -> d.cma"
  ~prod:"%.d.cma"
  ~dep:"%.mllib"
  (Ocaml_compiler.byte_debug_library_link_mllib "%.mllib" "%.d.cma");;

rule "ocaml: mllib & cmo* -> cma"
  ~prod:"%.cma"
  ~dep:"%.mllib"
  ~doc:"Build a .cma archive file (bytecode library) containing \
        the list of modules (`Foo`, `subdir/Bar`) given in the .mllib file \
        of the same name, one per line. \
        Note that the .cma archive will contain exactly the modules listed, \
        so it may not be self-contained if some dependencies are missing."

  (Ocaml_compiler.byte_library_link_mllib "%.mllib" "%.cma");;

rule "ocaml: d.cmo* -> d.cma"
  ~prod:"%.d.cma"
  ~dep:"%.d.cmo"
  (Ocaml_compiler.byte_debug_library_link "%.d.cmo" "%.d.cma");;

rule "ocaml: cmo* -> cma"
  ~prod:"%.cma"
  ~dep:"%.cmo"
  ~doc:"The preferred way to build a .cma archive is to create a .mllib file \
        with a list of modules to include. It is however possible to build one \
        from a .cmo of the same name; the archive will include this module and \
        the local modules it depends upon, transitively."
  (Ocaml_compiler.byte_library_link "%.cmo" "%.cma");;

rule "ocaml C stubs: clib & (o|obj)* -> (a|lib) & (so|dll)"
  ~prods:(["%(path:<**/>)lib%(libname:<*> and not <*.*>)"-.-ext_lib] @
          if Ocamlbuild_config.supports_shared_libraries then
            ["%(path:<**/>)dll%(libname:<*> and not <*.*>)"-.-ext_dll]
          else
	    [])
  ~dep:"%(path)lib%(libname).clib"
  ~doc:"OCamlbuild can create a C library by calling ocamlmklib \
        with the file paths listed (one per line) in libfoo.clib. \
        To build a static library from libfoo.clib, you should \
        request libfoo.a (or libfoo.lib on Windows), and to build \
        a dynamic library you should request libfoo.so (or libfoo.dll \
        on Windows). Finally, any file listed in the .clib \
        with name 'bar/baz.o' will link 'bar/baz.obj' instead on Windows. \
        This means that using the .o extension will give portable clib files, \
        even if it is not the object file extension on your system."
  (C_tools.link_C_library "%(path)lib%(libname).clib" ("%(path)lib%(libname)"-.-ext_lib) "%(path)%(libname)");;

rule "ocaml C stubs: (o|obj) -> clib"
  ~prod:"%(path:<**/>)lib%(libname:<*> and not <*.*>).clib"
  ~dep:("%(path)%(libname)"-.-ext_obj)
  ~doc:"The preferred way to build a C library libfoo.a is to create \
        a libfoo.clib file with a list of C object files to include. \
        It is however possible to build one from a .(o|obj) file of \
        corresponding name, foo.(o|obj); the library will include this \
        object file only."
  (C_tools.make_single_library "%(path)%(libname)" "%(path)lib%(libname).clib");;

(** Static libraries (.cmxa) and (.cmxs):

    Historically ocamlbuild had a bit too many .cmxs-related rules for
    its own good. There were rules to build a .cmxs from a .mldylib,
    but also from a .cmxa, and from a .cmx. In particular, the latter
    two options conflicted with the rule to build a .cmxa from a .cmx,
    giving two distinct ways to build a .cmxs from a .cmx, giving two
    different results.

    The new design is as follows:

    - First we declare, with the highest priority, the *explicit*
      rules, the one where the users expresses its intent.
      + .mllib -> .cmxa
      + .mldylib -> .cmxs
    - Then we declare, with lower priority, the *implicit* rules
      that are given for the user convenience. We ask that these
      rules do not conflict with each other by creating distinct
      paths to build the same target.
      + .cmx -> .cmxa (to mirror the bytecode rule .cmo -> .cma)
      + .cmxa -> .cmxs

    In particular, we had to remove the (.cmx -> .cmxs) rules, which
    conflicts with the two other low-priority rules. The user willing
    to recover this semantics should write a .mldylib rule with
    a single module name in it.
*)

rule "ocaml: mllib & p.cmx* & p.o* -> p.cmxa & p.a"
  ~prods:["%.p.cmxa"; x_p_a]
  ~dep:"%.mllib"
  (Ocaml_compiler.native_profile_library_link_mllib "%.mllib" "%.p.cmxa");;

rule "ocaml: mllib & cmx* & o* -> cmxa & a"
  ~prods:["%.cmxa"; x_a]
  ~dep:"%.mllib"
  ~doc:"Creates a native archive file .cmxa, using the .mllib file \
        as the .cma rule above. Note that whereas bytecode .cma can \
        be used both for static and dynamic linking, .cmxa only support \
        static linking. For an archive usable with Dynlink, \
        see the rules producing a .cmxs from a .mllib or a .mldylib."
  (Ocaml_compiler.native_library_link_mllib "%.mllib" "%.cmxa");;

rule "ocaml: mldylib & p.cmx* & p.o* -> p.cmxs & p.so"
  ~prods:["%.p.cmxs"; x_p_dll]
  ~dep:"%.mldylib"
  (Ocaml_compiler.native_profile_shared_library_link_mldylib "%.mldylib" "%.p.cmxs");;

rule "ocaml: mldylib & cmx* & o* -> cmxs & so"
  ~prods:["%.cmxs"; x_dll]
  ~dep:"%.mldylib"
  ~doc:"Builds a .cmxs (native archive for dynamic linking) containing exactly \
        the modules listed in the corresponding .mldylib file."
  (Ocaml_compiler.native_shared_library_link_mldylib "%.mldylib" "%.cmxs");;

rule "ocaml: p.cmx & p.o -> p.cmxa & p.a"
  ~prods:["%.p.cmxa"; x_p_a]
  ~deps:["%.p.cmx"; x_p_o]
  (Ocaml_compiler.native_profile_library_link "%.p.cmx" "%.p.cmxa");;

rule "ocaml: cmx & o -> cmxa & a"
  ~prods:["%.cmxa"; x_a]
  ~deps:["%.cmx"; x_o]
  ~doc:"Just as you can build a .cma from a .cmo in absence of .mllib file, \
        you can build a .cmxa (native archive file for static linking only) \
        from a .cmx, which will include the local modules it depends upon, \
        transitivitely."
  (Ocaml_compiler.native_library_link "%.cmx" "%.cmxa");;

rule "ocaml: p.cmxa & p.a -> p.cmxs & p.so"
  ~prods:["%.p.cmxs"; x_p_dll]
  ~deps:["%.p.cmxa"]
  (Ocaml_compiler.native_shared_library_link ~tags:["profile";"linkall"] "%.p.cmxa" "%.p.cmxs");;

rule "ocaml: cmxa -> cmxs & so"
  ~prods:["%.cmxs"; x_dll]
  ~deps:["%.cmxa"]
  ~doc:"This rule allows to build a .cmxs from a .cmxa, to avoid having \
        to duplicate a .mllib file into a .mldylib."
  (Ocaml_compiler.native_shared_library_link ~tags:["linkall"] "%.cmxa" "%.cmxs");;

rule "ocaml dependencies ml"
  ~prod:"%.ml.depends"
  ~dep:"%.ml"
  ~doc:"OCamlbuild will use ocamldep to approximate dependencies \
        of a source file. The ocamldep tool being purely syntactic, \
        it only computes an over-approximation of the dependencies.

\
        If you manipulate a module Foo that is in fact a submodule Bar.Foo \
        (after 'open Bar'), ocamldep may believe that your module depends \
        on foo.ml -- when such a file also exists in your project. This can \
        lead to spurious circular dependencies. In that case, you can use \
        OCamlbuild_plugin.non_dependency in your myocamlbuild.ml \
        to manually remove the spurious dependency. See the plugins API."
  (Ocaml_tools.ocamldep_command "%.ml" "%.ml.depends");;

rule "ocaml dependencies mli"
  ~prod:"%.mli.depends"
  ~dep:"%.mli"
  (Ocaml_tools.ocamldep_command "%.mli" "%.mli.depends");;

rule "ocamllex"
  ~prod:"%.ml"
  ~dep:"%.mll"
  (Ocaml_tools.ocamllex "%.mll");;

rule "ocaml: mli -> odoc"
  ~prod:"%.odoc"
  ~deps:["%.mli"; "%.mli.depends"]
  ~doc:".odoc are intermediate files storing the result of ocamldoc processing \
        on a source file. See the various .docdir/... targets for ocamldoc."
  (Ocaml_tools.document_ocaml_interf "%.mli" "%.odoc");;

rule "ocaml: ml -> odoc"
  ~prod:"%.odoc"
  ~deps:["%.ml"; "%.ml.depends"]
  (Ocaml_tools.document_ocaml_implem "%.ml" "%.odoc");;

rule "ocamldoc: document ocaml project odocl & *odoc -> docdir (html)"
  ~prod:"%.docdir/index.html"
  ~stamp:"%.docdir/html.stamp"
  ~dep:"%.odocl"
  ~doc:"the target foo.docdir/index.html will call ocamldoc to produce \
        the html documentation for the module paths listed \
        in foo.odocl, one per line. \
        See also the man|latex|texinfo|dot target below."
  (Ocaml_tools.document_ocaml_project
      ~tags:["html"]
      ~ocamldoc:Ocaml_tools.ocamldoc_l_dir "%.odocl" "%.docdir/index.html" "%.docdir");;

rule "ocamldoc: document ocaml project odocl & *odoc -> docdir (man)"
  ~prod:"%.docdir/man"
  ~stamp:"%.docdir/man.stamp"
  ~dep:"%.odocl"
  ~doc:"The target foo.docdir/man will call ocamldoc to produce \
        documentation, in man format, for the module paths \
        listed in foo.odocl, one per line."
  (Ocaml_tools.document_ocaml_project
      ~tags:["man"]
      ~ocamldoc:Ocaml_tools.ocamldoc_l_dir "%.odocl" "%.docdir/man" "%.docdir");;

rule "ocamldoc: document ocaml project odocl & *odoc -> latex"
  ~prod:"%(dir).docdir/%(file).%(ext:\"tex\" or \"ltx\")"
  ~dep:"%(dir).odocl"
  ~doc:"The target foo.docdir/bar.tex (or bar.ltx) will call ocamldoc \
        to produce documentation, in LaTeX format, for the module paths \
        listed in foo.odocl, one per line."
  (Ocaml_tools.document_ocaml_project
      ~tags:["tex"]
      ~ocamldoc:Ocaml_tools.ocamldoc_l_file
        "%(dir).odocl" "%(dir).docdir/%(file).%(ext)" "%(dir).docdir");;

rule "ocamldoc: document ocaml project odocl & *odoc -> texinfo"
  ~prod:"%(dir).docdir/%(file).texi"
  ~dep:"%(dir).odocl"
  ~doc:"The target foo.docdir/bar.texi will call ocamldoc to produce \
        documentation, in TeXinfo format, for the module paths \
        listed in foo.odocl, one per line."
  (Ocaml_tools.document_ocaml_project
      ~tags:["texi"]
      ~ocamldoc:Ocaml_tools.ocamldoc_l_file
      "%(dir).odocl" "%(dir).docdir/%(file).texi" "%(dir).docdir");;

rule "ocamldoc: document ocaml project odocl & *odoc -> dot"
  ~prod:"%(dir).docdir/%(file).dot"
  ~dep:"%(dir).odocl"
  ~doc:"The target foo.docdir/bar.dot will generate a dot graph of module \
        dependencies."
  (Ocaml_tools.document_ocaml_project
      ~tags:["dot"]
      ~ocamldoc:Ocaml_tools.ocamldoc_l_file
      "%(dir).odocl" "%(dir).docdir/%(file).dot" "%(dir).docdir");;

(* To use menhir give the -use-menhir option at command line,
   Or put true: use_menhir in your tag file. *)
if !Options.use_menhir || Configuration.has_tag "use_menhir" then begin
  (* Automatic handling of menhir modules, given a
     description file %.mlypack                         *)
  rule "ocaml: modular menhir (mlypack)"
    ~prods:["%.mli" ; "%.ml"]
    ~deps:["%.mlypack"]
    ~doc:"Menhir supports building a parser by composing several .mly files \
          together, containing different parts of the grammar description. \
          To use that feature with ocamlbuild, you should create a .mlypack \
          file with the same syntax as .mllib or .mlpack files: \
          a whitespace-separated list of the capitalized module names \
          of the .mly files you want to combine together."
    (Ocaml_tools.menhir_modular "%" "%.mlypack" "%.mlypack.depends");

  rule "ocaml: menhir modular dependencies"
    ~prod:"%.mlypack.depends"
    ~dep:"%.mlypack"
    (Ocaml_tools.menhir_modular_ocamldep_command "%.mlypack" "%.mlypack.depends");

  rule "ocaml: menhir"
    ~prods:["%.ml"; "%.mli"]
    ~deps:["%.mly"; "%.mly.depends"]
    ~doc:"Invokes menhir to build the .ml and .mli files derived from a .mly \
          grammar. If you want to use ocamlyacc instead, you must disable the \
          -use-menhir option that was passed to ocamlbuild."
    (Ocaml_tools.menhir "%.mly");

  rule "ocaml: menhir dependencies"
    ~prod:"%.mly.depends"
    ~dep:"%.mly"
    (Ocaml_tools.menhir_ocamldep_command "%.mly" "%.mly.depends");

end else
  rule "ocamlyacc"
    ~prods:["%.ml"; "%.mli"]
    ~dep:"%.mly"
    ~doc:"By default, ocamlbuild will use ocamlyacc to produce a .ml and .mly \
          from a .mly file of the same name. You can also enable the \
          -use-menhir option to use menhir instead. Menhir is a recommended \
          replacement for ocamlyacc, that supports more feature, lets you \
          write more readable grammars, and helps you understand conflicts."
    (Ocaml_tools.ocamlyacc "%.mly");;

rule "ocaml C stubs: c -> o"
  ~prod:x_o
  ~dep:"%.c"
  ~doc:"The OCaml compiler can be passed .c files and will call \
        the underlying C toolchain to produce corresponding .o files. \
        ocamlc or ocamlopt will be used depending on whether \
        the 'native' flag is set on the .c file."
  begin fun env _build ->
    let c = env "%.c" in
    let o = env x_o in
    let comp = if Tags.mem "native" (tags_of_pathname c) then !Options.ocamlopt else !Options.ocamlc in
    (* The workaround below is necessary on 4.03 and older OCaml version because
         ocamlc -c -o dir/foo.o dir/foo.c
       does not work as expected. See
         the original discussion: http://caml.inria.fr/mantis/view.php?id=6475
         the partial revert: https://github.com/ocaml/ocamlbuild/pull/51
         the 4.04 improvement: https://github.com/ocaml/ocaml/pull/464

       Whenever we support to drop support for OCaml 4.03 we can
       replace the lines below with just

         Cmd(S[comp; T(tags_of_pathname c++"c"++"compile");
               A"-c"; A"-o"; P o; Px c])
     *)
    let cc = Cmd(S[comp; T(tags_of_pathname c++"c"++"compile"); A"-c"; Px c]) in
    if Pathname.dirname o = Pathname.current_dir_name then cc
    else Seq[cc; mv (Pathname.basename o) o]
  end;;

rule "ocaml: ml & ml.depends & *cmi -> .inferred.mli"
  ~prod:"%.inferred.mli"
  ~deps:["%.ml"; "%.ml.depends"]
  ~doc:"The target foo.inferred.mli will produce a .mli that exposes all the \
        declarations in foo.ml, as obtained by direct invocation of `ocamlc -i`."
  (Ocaml_tools.infer_interface "%.ml" "%.inferred.mli");;

rule "ocaml: mltop -> top"
  ~prod:"%.top"
  ~dep:"%.mltop"
  ~doc:"If foo.mltop contains a list of module paths, then building foo.top \
        will call the ocamlmktop tool to build a custom toplevel with those \
        modules pre-linked."
  (Ocaml_compiler.byte_toplevel_link_mltop "%.mltop" "%.top");;

rule "preprocess: ml -> pp.ml"
  ~dep:"%.ml"
  ~prod:"%.pp.ml"
  ~doc:"The target foo.pp.ml should generate a source file equivalent \
        to foo.ml after syntactic preprocessors (camlp4, etc.) have been \
        applied.

\
        Warning: This option is currently known to malfunction \
        when used together with -use-ocamlfind (for syntax extensions \
        coming from ocamlfind packages). Direct compilation of the \
        corresponding file to produce a .cmx or .cmo will still work well."
  (Ocaml_tools.camlp4 "pp.ml" "%.ml" "%.pp.ml");;

flag ["ocaml"; "pp"] begin
  S (List.fold_right (fun x acc -> Sh x :: acc) !Options.ocaml_ppflags [])
end;;

flag ["ocaml"; "compile"] begin
  atomize !Options.ocaml_cflags
end;;

flag ["c"; "compile"] begin
  atomize !Options.ocaml_cflags
end;;

flag ["ocaml"; "link"] begin
  atomize !Options.ocaml_lflags
end;;

flag ["c"; "link"] begin
  atomize !Options.ocaml_lflags
end;;

flag ["ocaml"; "ocamlyacc"] (atomize !Options.ocaml_yaccflags);;
flag ["ocaml"; "menhir"] (atomize !Options.ocaml_yaccflags);;
flag ["ocaml"; "doc"] (atomize !Options.ocaml_docflags);;
flag ["ocaml"; "ocamllex"] (atomize !Options.ocaml_lexflags);;

(* menhir options *)
flag [ "ocaml" ; "menhir" ; "explain" ] (S[A "--explain"]);;
flag [ "ocaml" ; "menhir" ; "infer" ] (S[A "--infer"]);;
flag [ "ocaml" ; "menhir" ; "table" ] (S[A "--table"]);;
flag [ "ocaml" ; "menhir" ; "trace" ] (S[A "--trace"]);;
flag [ "ocaml" ; "menhir" ; "trace" ] (S[A "--canonical"]);;

(* Define two ocamlbuild flags [only_tokens] and [external_tokens(Foo)]
   which correspond to menhir's [--only-tokens] and [--external-tokens Foo].
   When they are used, these flags should be passed both to [menhir] and to
   [menhir --raw-depend]. *)
let () =
  List.iter begin fun mode ->
    flag [ mode; "only_tokens" ] (S[A "--only-tokens"]);
    pflag [ mode ] "external_tokens" ~doc_param:"TokenModule" (fun name ->
      S[A "--external-tokens"; A name]);
  end [ "menhir"; "menhir_ocamldep" ];;

(* Tell ocamllex to generate ml code *)
flag [ "ocaml" ; "ocamllex" ; "generate_ml" ] (S[A "-ml"]);;

flag ["ocaml"; "byte"; "link"] begin
  S (List.map (fun x -> A (x^".cma")) !Options.ocaml_libs)
end;;

flag ["ocaml"; "native"; "link"] begin
  S (List.map (fun x -> A (x^".cmxa")) !Options.ocaml_libs)
end;;

flag ["ocaml"; "byte"; "link"] begin
  S (List.map (fun x -> A (x^".cmo")) !Options.ocaml_mods)
end;;

flag ["ocaml"; "native"; "link"] begin
  S (List.map (fun x -> A (x^".cmx")) !Options.ocaml_mods)
end;;

(* findlib *)
let () =
  if !Options.use_ocamlfind then begin
    let doc_param = "my_package" in

    (* Ocamlfind will link the archives for us. *)
    flag ["ocaml"; "link"; "program"] & A"-linkpkg";
    flag ["ocaml"; "link"; "toplevel"] & A"-linkpkg";
    flag ["ocaml"; "link"; "output_obj"] & A"-linkpkg";

    (* "program" will make sure that -linkpkg is passed when compiling
       whole-programs (.byte and .native); but it is occasionally
       useful to pass -linkpkg when building archives for example
       (.cma and .cmxa); the "linkpkg" flag allows user to request it
       explicitly. *)
    flag ["ocaml"; "link"; "library"; "linkpkg"] & A"-linkpkg";
    pflag ["ocaml"; "link"] "dontlink" ~doc_param
          (fun pkg -> S[A"-dontlink"; A pkg]);

    let all_tags = [
      ["ocaml"; "byte"; "compile"];
      ["ocaml"; "native"; "compile"];
      ["ocaml"; "byte"; "link"];
      ["ocaml"; "native"; "link"];
      ["ocaml"; "ocamldep"];
      ["ocaml"; "doc"];
      ["ocaml"; "mktop"];
      ["ocaml"; "infer_interface"];
      (* PR#6794: ocamlbuild should pass -package flags when building C files *)
      ["c"; "compile"];
    ] in

    (* tags package(X), predicate(X) and syntax(X), ppopt(X), ppxopt(X) *)
    List.iter begin fun tags ->
      pflag tags "package" ~doc_param (fun pkg -> S [A "-package"; A pkg]);
      pflag tags "predicate" ~doc_param:"archive"
        (fun pkg -> S [A "-predicates"; A pkg]);
      if List.mem "ocaml" tags then begin
        pflag tags "syntax" ~doc_param:"camlp4o"
          (fun pkg -> S [A "-syntax"; A pkg]);
        pflag tags "ppopt" ~doc_param:"pparg"
          (fun arg -> S [A "-ppopt"; A arg]);
        pflag tags "ppxopt" ~doc_param:"package,arg"
          (fun arg -> S [A "-ppxopt"; A arg]);
      end;
    end all_tags
  end else begin
    try
      (* Note: if there is no -pkg option, ocamlfind won't be called *)
      let pkgs = List.map Findlib.query !Options.ocaml_pkgs in
      flag ["ocaml"; "byte"; "compile"] (Findlib.compile_flags_byte pkgs);
      flag ["ocaml"; "native"; "compile"] (Findlib.compile_flags_native pkgs);
      flag ["ocaml"; "byte"; "link"] (Findlib.link_flags_byte pkgs);
      flag ["ocaml"; "native"; "link"] (Findlib.link_flags_native pkgs);
      (* PR#6794: ocamlbuild should pass -package flags when building C files *)
      flag ["c"; "compile"] (Findlib.include_flags pkgs)
    with Findlib.Findlib_error e ->
      Findlib.report_error e
  end

(* parameterized tags *)
let () =
  pflag ["ocaml"; "compile"] "for-pack" ~doc_param:"PackModule"
    (fun param -> S [A "-for-pack"; A param]);
  pflag ["ocaml"; "pack"] "for-pack" ~doc_param:"PackModule"
    (fun param -> S [A "-for-pack"; A param]);
  pflag ["ocaml"; "native"; "compile"] "inline" ~doc_param:"5"
    (fun param -> S [A "-inline"; A param]);
  pflag ["ocaml"; "compile"] "color" (fun setting -> S[A "-color"; A setting]);
  pflag ["ocaml"; "infer_interface"] "color" (fun setting -> S[A "-color"; A setting]);
  List.iter (fun pp ->
    let doc_param = "my_" ^ pp in
    pflag ["ocaml"; "compile"] pp ~doc_param
      (fun param -> S [A ("-" ^ pp); A param]);
    pflag ["ocaml"; "ocamldep"] pp ~doc_param
      (fun param -> S [A ("-" ^ pp); A param]);
    pflag ["ocaml"; "doc"] pp ~doc_param
      (fun param -> S [A ("-" ^ pp); A param]);
    pflag ["ocaml"; "infer_interface"] pp ~doc_param
      (fun param -> S [A ("-" ^ pp); A param])
  ) ["pp"; "ppx"];
  pflag ["ocaml";"compile";] "warn" ~doc_param:"A@10-28@40-42-45"
    (fun param -> S [A "-w"; A param]);
  pflag ["ocaml";"infer_interface";] "warn" ~doc_param:"A@10-28@40-42-45"
    (fun param -> S [A "-w"; A param]);
  pflag ["ocaml";"compile";] "warn_error" ~doc_param:"+10+40"
    (fun param -> S [A "-warn-error"; A param]);
  pflag ["ocaml";"infer_interface";] "warn_error" ~doc_param:"+10+40"
    (fun param -> S [A "-warn-error"; A param]);
  pflag ["ocaml"; "ocamldep"] "open" ~doc_param:"MyPervasives"
    (fun param -> S [A "-open"; A param]);
  pflag ["ocaml"; "compile"] "open" ~doc_param:"MyPervasives"
    (fun param -> S [A "-open"; A param]);
  pflag ["ocaml"; "infer_interface"] "open" ~doc_param:"MyPervasives"
    (fun param -> S [A "-open"; A param]);
  pflag ["ocaml"; "link"] "runtime_variant" ~doc_param:"_pic"
    (fun param -> S [A "-runtime-variant"; A param]);
  pflag ["ocaml"; "native"; "compile"] "optimize" ~doc_param:"2"
    (fun param ->
       match param with
       | "classic"
       | "2"
       | "3" -> S [A ("-O" ^ param)]
       | _ ->
         let e = Printf.sprintf
             "Invalid parameter for \"optimize\" tag: %s (can be classic, 2, or 3)."
             param
         in
         raise (Invalid_argument e));
  pflag ["ocaml"; "native"; "compile"] "optimization_rounds" ~doc_param:"2"
    (fun param -> S [A "-rounds"; A param]);
  pflag ["ocaml"; "native"; "compile"] "inline_toplevel" ~doc_param:"160"
    (fun param -> S [A "-inline-toplevel"; A param]);
  pflag ["ocaml"; "native"; "compile"] "inline_branch_factor" ~doc_param:"0.1"
    (fun param -> S [A "-inline-branch-factor"; A param];);
  pflag ["ocaml"; "native"; "compile"] "inline_alloc_cost" ~doc_param:"7"
    (fun param -> S [A "-inline-alloc-cost"; A param]);
  pflag ["ocaml"; "native"; "compile"] "inline_branch_cost" ~doc_param:"5"
    (fun param -> S [A "-inline-branch-cost"; A param]);
  pflag ["ocaml"; "native"; "compile"] "inline_call_cost" ~doc_param:"5"
    (fun param -> S [A "-inline-call-cost"; A param]);
  pflag ["ocaml"; "native"; "compile"] "inline_indirect_cost" ~doc_param:"4"
    (fun param -> S [A "-inline-indirect-cost"; A param]);
  pflag ["ocaml"; "native"; "compile"] "inline_prim_cost" ~doc_param:"3"
    (fun param -> S [A "-inline-prim-cost"; A param]);
  pflag ["ocaml"; "native"; "compile"] "inline_lifting_benefit"
    ~doc_param:"1300" (fun param -> S [A "-inline-lifting-benefit"; A param]);
  pflag ["ocaml"; "native"; "compile"] "inline_max_depth" ~doc_param:"1"
    (fun param -> S [A "-inline-max-depth"; A param]);
  pflag ["ocaml"; "native"; "compile"] "inline_max_unroll" ~doc_param:"0"
    (fun param -> S [A "-inline-max-unroll"; A param]);
  pflag ["ocaml"; "native"; "compile"] "unbox_closures_factor" ~doc_param:"10"
    (fun param -> S [A "-unbox-closures-factor"; A param]);
  pflag ["ocaml"; "native"; "compile"] "afl_inst_ratio" ~doc_param:"50"
    (fun param -> S [A "-afl-inst-ratio"; A param]);
  ()

let camlp4_flags camlp4s =
  List.iter begin fun camlp4 ->
    flag ["ocaml"; "pp"; camlp4] (A camlp4)
  end camlp4s;;

let p4_series =  ["camlp4o"; "camlp4r"; "camlp4of"; "camlp4rf"; "camlp4orf"; "camlp4oof"];;
let p4_opt_series = List.map (fun f -> f ^ ".opt") p4_series;;

camlp4_flags p4_series;;
camlp4_flags p4_opt_series;;

let camlp4_flags' camlp4s =
  List.iter begin fun (camlp4, flags) ->
    flag ["ocaml"; "pp"; camlp4] flags
  end camlp4s;;

camlp4_flags' ["camlp4orr", S[A"camlp4of"; A"-parser"; A"reloaded"];
               "camlp4rrr", S[A"camlp4rf"; A"-parser"; A"reloaded"]];;

flag ["ocaml"; "pp"; "camlp4:no_quot"] (A"-no_quot");;

ocaml_lib ~extern:true "dynlink";;
ocaml_lib ~extern:true "unix";;
ocaml_lib ~extern:true "str";;
ocaml_lib ~extern:true "bigarray";;
ocaml_lib ~extern:true "nums";;
ocaml_lib ~extern:true "dbm";;
ocaml_lib ~extern:true "graphics";;
ocaml_lib ~extern:true ~tag_name:"use_toplevel" "toplevellib";;
ocaml_lib ~extern:true ~dir:"+ocamldoc" "ocamldoc";;
ocaml_lib ~extern:true ~dir:"+ocamlbuild" ~tag_name:"use_ocamlbuild" "ocamlbuildlib";;

let camlp4dir =
  let where cmd =
    (* may raise a Failure exception *)
    String.chomp
      (My_unix.run_and_read
         (cmd ^ " -where 2>/dev/null")) in
  try where "camlp4" with _ -> "+camlp4"
;;

ocaml_lib ~extern:true ~dir:camlp4dir ~tag_name:"use_camlp4" "camlp4lib";;
ocaml_lib ~extern:true ~dir:camlp4dir ~tag_name:"use_old_camlp4" "camlp4";;
ocaml_lib ~extern:true ~dir:camlp4dir ~tag_name:"use_camlp4_full" "camlp4fulllib";;
flag ["ocaml"; "compile"; "use_camlp4_full"]
     (S[A"-I"; A(camlp4dir^"/Camlp4Parsers");
        A"-I"; A(camlp4dir^"/Camlp4Printers");
        A"-I"; A(camlp4dir^"/Camlp4Filters")]);;
flag ["ocaml"; "use_camlp4_bin"; "link"; "byte"] (A(camlp4dir^"/Camlp4Bin.cmo"));;
flag ["ocaml"; "use_camlp4_bin"; "link"; "native"] (A(camlp4dir^"/Camlp4Bin.cmx"));;

flag ["ocaml"; "debug"; "compile"; "byte"] (A "-g");;
flag ["ocaml"; "debug"; "link"; "byte"; "program"] (A "-g");;
flag ["ocaml"; "debug"; "pack"; "byte"] (A "-g");;
flag ["ocaml"; "debug"; "compile"; "native"] (A "-g");;
flag ["ocaml"; "debug"; "link"; "native"; "program"] (A "-g");;
flag ["ocaml"; "debug"; "pack"; "native"] (A "-g");;
flag ["c";     "debug"; "compile"] (A "-g");
flag ["c";     "debug"; "link"] (A "-g");
flag ["ocaml"; "link"; "native"; "output_obj"] (A"-output-obj");;
flag ["ocaml"; "link"; "byte"; "output_obj"] (A"-output-obj");;
flag ["ocaml"; "dtypes"; "compile"] (A "-dtypes");;
flag ["ocaml"; "annot"; "compile"] (A "-annot");;
flag ["ocaml"; "annot"; "pack"] (A "-annot");;
flag ["ocaml"; "bin_annot"; "compile"] (A "-bin-annot");;
flag ["ocaml"; "bin_annot"; "pack"] (A "-bin-annot");;
flag ["ocaml"; "safe_string"; "compile"] (A "-safe-string");;
flag ["ocaml"; "safe_string"; "infer_interface"] (A "-safe-string");;
flag ["ocaml"; "unsafe_string"; "compile"] (A "-unsafe-string");;
flag ["ocaml"; "unsafe_string"; "infer_interface"] (A "-unsafe-string");;
flag ["ocaml"; "short_paths"; "compile"] (A "-short-paths");;
flag ["ocaml"; "short_paths"; "infer_interface"] (A "-short-paths");;
flag ["ocaml"; "rectypes"; "compile"] (A "-rectypes");;
flag ["ocaml"; "rectypes"; "infer_interface"] (A "-rectypes");;
flag ["ocaml"; "rectypes"; "doc"] (A "-rectypes");;
flag ["ocaml"; "rectypes"; "pack"] (A "-rectypes");;
flag ["ocaml"; "principal"; "compile"] (A "-principal");;
flag ["ocaml"; "principal"; "infer_interface"] (A "-principal");;
flag ["ocaml"; "linkall"; "link"] (A "-linkall");;
flag ["ocaml"; "noautolink"; "link"] (A "-noautolink");;
flag ["ocaml"; "link"; "profile"; "native"] (A "-p");;
flag ["ocaml"; "link"; "program"; "custom"; "byte"] (A "-custom");;
flag ["ocaml"; "link"; "library"; "custom"; "byte"] (A "-custom");;
flag ["ocaml"; "link"; "toplevel"; "custom"; "byte"] (A "-custom");;
flag ["ocaml"; "compile"; "profile"; "native"] (A "-p");;

flag ["ocamlmklib"; "linkall"] (A "-linkall");;
flag ["ocamlmklib"; "custom"] (A "-custom");;
flag ["ocamlmklib"; "failsafe"] (A "-failsafe");;
flag ["ocamlmklib"; "debug"] (A "-g");;

flag ["ocaml"; "compile"; "no_alias_deps";] (A "-no-alias-deps");;
flag ["ocaml"; "infer_interface"; "no_alias_deps";] (A "-no-alias-deps");;

flag ["ocaml"; "compile"; "strict_formats";] (A "-strict-formats");;
flag ["ocaml"; "infer_interface"; "strict_formats";] (A "-strict-formats");;

flag ["ocaml"; "compile"; "noassert"] (A "-noassert");;
flag ["ocaml"; "compile"; "unsafe"] (A "-unsafe");;

begin
  let above_403 =
    match split_ocaml_version with
      | Some (major, minor, _patch, _rest) -> major > 4 || minor >= 3
      | None -> false
  in
  (* starting from 4.03, ocamlc also supports -opaque *)
  if above_403 then
    flag ["ocaml"; "compile"; "opaque";] (A "-opaque")
  else
    flag ["ocaml"; "native"; "compile"; "opaque";] (A "-opaque")
end;;

flag ["ocaml"; "native"; "compile"; "no_float_const_prop";] (A "-no-float-const-prop");

["compile"; "link"; "pack"] |> List.iter begin fun phase ->
  flag ["ocaml"; phase; "keep_docs";] (A "-keep-docs");
  flag ["ocaml"; phase; "keep_locs";] (A "-keep-locs");
end;;
["compile"; "infer_interface"] |> List.iter begin fun phase ->
  flag ["ocaml"; phase; "absname"] (A "-absname");
end;;

flag ["ocaml"; "byte"; "compile"; "compat_32";] (A "-compat-32");;
flag ["ocaml";"compile";"native";"asm"] & S [A "-S"];;

(* AFL instrumentation *)
flag ["ocaml"; "compile"; "native"; "afl_instrument"] (A "-afl-instrument");;

(* threads, with or without findlib *)
flag ["ocaml"; "compile"; "thread"] (A "-thread");;
flag ["ocaml"; "link"; "thread"] (A "-thread");;
if !Options.use_ocamlfind then
  (* PR#6794: Needed as we pass -package when compiling C files *)
  flag ["c"; "compile"; "thread"] (A "-thread")
else begin
  flag ["ocaml"; "doc"; "thread"] (S[A"-I"; A"+threads"]);
  flag ["ocaml"; "link"; "thread"; "native"; "program"] (A "threads.cmxa");
  flag ["ocaml"; "link"; "thread"; "byte"; "program"] (A "threads.cma");
  flag ["ocaml"; "link"; "thread"; "native"; "toplevel"] (A "threads.cmxa");
  flag ["ocaml"; "link"; "thread"; "byte"; "toplevel"] (A "threads.cma");
end;;

flag ["ocaml"; "compile"; "nostdlib"] (A"-nostdlib");;
flag ["ocaml"; "infer_interface"; "nostdlib"] (A"-nostdlib");;
flag ["ocaml"; "link"; "nostdlib"] (A"-nostdlib");;

flag ["ocaml"; "compile"; "nopervasives"] (A"-nopervasives");;
flag ["ocaml"; "infer_interface"; "nopervasives"] (A"-nopervasives");;
flag ["ocaml"; "compile"; "nolabels"] (A"-nolabels");;
flag ["ocaml"; "infer_interface"; "nolabels"] (A"-nolabels");;

(*flag ["ocaml"; "ocamlyacc"; "quiet"] (A"-q");;*)
flag ["ocaml"; "ocamllex"; "quiet"] (A"-q");;

let ocaml_warn_flag c =
  flag ~deprecated:true
    ["ocaml"; "compile"; sprintf "warn_%c" (Char.uppercase_ascii c)]
    (S[A"-w"; A (sprintf "%c" (Char.uppercase_ascii c))]);
  flag ~deprecated:true
    ["ocaml"; "compile"; sprintf "warn_error_%c" (Char.uppercase_ascii c)]
    (S[A"-warn-error"; A (sprintf "%c" (Char.uppercase_ascii c))]);
  flag ~deprecated:true
    ["ocaml"; "compile"; sprintf "warn_%c" (Char.lowercase_ascii c)]
    (S[A"-w"; A (sprintf "%c" (Char.lowercase_ascii c))]);
  flag ~deprecated:true
    ["ocaml"; "compile"; sprintf "warn_error_%c" (Char.lowercase_ascii c)]
    (S[A"-warn-error"; A (sprintf "%c" (Char.lowercase_ascii c))]);;

List.iter ocaml_warn_flag ['A'; 'C'; 'D'; 'E'; 'F'; 'K'; 'L'; 'M'; 'P'; 'R'; 'S'; 'U'; 'V'; 'X'; 'Y'; 'Z'];;

flag ~deprecated:true
  ["ocaml"; "compile"; "strict-sequence"] (A "-strict-sequence");;

flag ["ocaml"; "compile"; "strict_sequence"] (A "-strict-sequence");;
flag ["ocaml"; "infer_interface"; "strict_sequence"] (A "-strict-sequence");;

flag ["ocaml"; "doc"; "docdir"; "html"] (A"-html");;
flag ["ocaml"; "doc"; "docdir"; "man"] (A"-man");;
flag ["ocaml"; "doc"; "docfile"; "dot"] (A"-dot");;
flag ["ocaml"; "doc"; "docfile"; "tex"] (A"-latex");;
flag ["ocaml"; "doc"; "docfile"; "texi"] (A"-texi");;

(* Inlining and flambda optimization options *)
flag ["ocaml"; "native"; "compile"; "inlining_report"]
  (A"-inlining-report");;

flag ["ocaml"; "native"; "compile"; "remove_unused_arguments"]
  (A"-remove-unused-arguments");;

flag ["ocaml"; "native"; "compile"; "unbox_closures"]
  (A"-unbox-closures");;

flag ["ocaml"; "native"; "compile"; "no_unbox_free_vars_of_closures"]
  (A"-no-unbox-free-vars-of-closures");;

flag ["ocaml"; "native"; "compile"; "no_unbox_specialized_args"]
  (A"-no-unbox-specialized-args");;

(* Kept for backward compatibility. *)
flag ["ocaml"; "doc"; "docdir"; "manpage"] (A"-man");;

pflag ["ocaml"; "doc"; "dot"] "dot_colors"
  (fun param -> S [A "-dot-colors"; A param]);;
flag ["ocaml"; "doc"; "dot"; "dot_include_all"] (A"-dot-include-all");;
flag ["ocaml"; "doc"; "dot"; "dot_reduce"] (A"-dot-reduce");;
flag ["ocaml"; "doc"; "dot"; "dot_types"] (A"-dot-types");;

flag ["ocaml"; "doc"; "man"; "man_mini"] (A"-man-mini");;
pflag ["ocaml"; "doc"; "man"] "man_suffix"
  (fun param -> S [A "-man-suffix"; A param]);;
pflag ["ocaml"; "doc"; "man"] "man_section"
  (fun param -> S [A "-man-section"; A param]);;

ocaml_lib "ocamlbuildlib";;
ocaml_lib "ocamlbuildlightlib";;

begin
  let ccflag ~lang ~phase ~flag =
    pflag [lang; phase] flag (fun param -> S [A ("-"^flag); A param])
  in
  ["c"; "ocaml"] |> List.iter (fun lang ->
  ["compile"; "link"] |> List.iter (fun phase ->
  ["cc"; "ccopt"; "cclib"] |> List.iter (fun flag ->
    ccflag ~lang ~phase ~flag)))
end;;

begin
  let ocamlmklib_pflag ?doc_param flag =
    pflag ["ocamlmklib"] flag ?doc_param
      (fun param -> S [A ("-"^flag); A param]) in
  ocamlmklib_pflag "cclib";
  ocamlmklib_pflag "ccopt";
  ocamlmklib_pflag "rpath";
  ocamlmklib_pflag "ldopt";
end;;

end in ()
