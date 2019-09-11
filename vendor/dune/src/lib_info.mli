(** Raw library descriptions *)

(** This module regroup all information about a library. We call such
    descriptions "raw" as the names, such as the names of dependencies
    are plain unresolved library names.

    The [Lib] module takes care of resolving library names to actual
    libraries. *)

open Stdune

module Status : sig
  type t =
    | Installed
    | Public  of Dune_project.Name.t * Package.t
    | Private of Dune_project.t

  val pp : t Fmt.t

  val is_private : t -> bool

  (** For local libraries, return the project name they are part of *)
  val project_name : t -> Dune_project.Name.t option
end

module Deps : sig
  type t =
    | Simple  of (Loc.t * Lib_name.t) list
    | Complex of Dune_file.Lib_dep.t list

  val of_lib_deps : Dune_file.Lib_deps.t -> t
end

(** For values like modules that need to be evaluated to be fetched *)
module Source : sig
  type 'a t =
    | Local
    | External of 'a
end

module Enabled_status : sig
  type t =
    | Normal
    | Optional
    | Disabled_because_of_enabled_if
end

type 'path t

val name : _ t -> Lib_name.t
val loc : _ t -> Loc.t
val archives : 'path t -> 'path list Mode.Dict.t
val foreign_archives : 'path t -> 'path list Mode.Dict.t
val foreign_objects : 'path t -> 'path list Source.t
val plugins : 'path t -> 'path list Mode.Dict.t
val src_dir : 'path t -> 'path
val status : _ t -> Status.t
val variant : _ t -> Variant.t option
val default_implementation : _ t -> (Loc.t * Lib_name.t) option
val kind : _ t -> Lib_kind.t
val synopsis : _ t -> string option
val jsoo_runtime : 'path t -> 'path list
val jsoo_archive : 'path t -> 'path option
val obj_dir : 'path t -> 'path Obj_dir.t
val virtual_ : _ t -> Modules.t Source.t option
val main_module_name : _ t -> Dune_file.Library.Main_module_name.t
val wrapped : _ t -> Wrapped.t Dune_file.Library.Inherited.t option
val special_builtin_support : _ t -> Dune_file.Library.Special_builtin_support.t option
val modes : _ t -> Mode.Dict.Set.t
val implements : _ t -> (Loc.t * Lib_name.t) option
val known_implementations : _ t -> (Loc.t * Lib_name.t) Variant.Map.t
val requires : _ t -> Deps.t
val ppx_runtime_deps : _ t -> (Loc.t * Lib_name.t) list
val pps : _ t -> (Loc.t * Lib_name.t) list
val sub_systems : _ t -> Sub_system_info.t Sub_system_name.Map.t
val enabled : _ t -> Enabled_status.t
val orig_src_dir : 'path t -> 'path option
val version : _ t -> string option

(** Directory where the source files for the library are located. Returns
    the original src dir when it exists *)
val best_src_dir : 'path t -> 'path

type external_ = Path.t t
type local = Path.Build.t t

val of_library_stanza
  :  dir:Path.Build.t
  -> lib_config:Lib_config.t
  -> known_implementations:(Loc.t * Lib_name.t) Variant.Map.t
  -> Dune_file.Library.t
  -> local

val user_written_deps : _ t -> Dune_file.Lib_deps.t

val of_dune_lib
  :  Sub_system_info.t Dune_package.Lib.t
  -> external_

val of_local : local -> external_

val as_local_exn : external_ -> local
