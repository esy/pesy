(** High-level API for compiling OCaml files *)

open! Stdune
open Import

(** Represent a compilation context.

    A compilation context contains all the necessary information to
    preprocess and compile OCaml source files. Exactly one compilation
    context is associated to each library, executable and executables
    stanza.
*)
type t

(** Create a compilation context. *)
val create
  :  super_context         : Super_context.t
  -> scope                 : Scope.t
  -> expander              : Expander.t
  -> obj_dir               : Path.Build.t Obj_dir.t
  -> ?dir_kind             : Dune_lang.File_syntax.t
  -> modules               : Modules.t
  -> flags                 : Ocaml_flags.t
  -> requires_compile      : Lib.t list Or_exn.t
  -> requires_link         : Lib.t list Or_exn.t Lazy.t
  -> ?preprocessing        : Preprocessing.t
  -> ?no_keep_locs         : bool
  -> opaque                : bool
  -> ?stdlib               : Dune_file.Library.Stdlib.t
  -> js_of_ocaml           : Dune_file.Js_of_ocaml.t option
  -> dynlink               : bool
  -> ?sandbox              : bool
  -> package               : Package.t option
  -> ?vimpl                : Vimpl.t
  -> unit
  -> t

(** Return a compilation context suitable for compiling the alias module. *)
val for_alias_module : t -> t

val super_context        : t -> Super_context.t
val expander             : t -> Expander.t
val context              : t -> Context.t
val scope                : t -> Scope.t
val dir                  : t -> Path.Build.t
val dir_kind             : t -> Dune_lang.File_syntax.t
val obj_dir              : t -> Path.Build.t Obj_dir.t
val modules              : t -> Modules.t
val flags                : t -> Ocaml_flags.t
val requires_link        : t -> Lib.t list Or_exn.t
val requires_compile     : t -> Lib.t list Or_exn.t
val includes             : t -> Command.Args.dynamic Command.Args.t Cm_kind.Dict.t
val preprocessing        : t -> Preprocessing.t
val no_keep_locs         : t -> bool
val opaque               : t -> bool
val stdlib               : t -> Dune_file.Library.Stdlib.t option
val js_of_ocaml          : t -> Dune_file.Js_of_ocaml.t option
val dynlink              : t -> bool
val sandbox              : t -> bool option
val package              : t -> Package.t option
val vimpl                : t -> Vimpl.t option

val for_wrapped_compat : t -> t
