(** An environment node represents an evaluated (env ..) stanza in a
    directory. *)

open Stdune

type t

val make
  :  dir:Path.t
  -> inherit_from:t Lazy.t option
  -> scope:Scope.t
  -> config:Dune_env.Stanza.t option
  -> env:Env.t option
  -> t

val scope : t -> Scope.t

val external_ : t -> profile:string -> default:Env.t -> Env.t

val ocaml_flags : t -> profile:string -> expander:Expander.t -> Ocaml_flags.t

val c_flags
  : t
  -> profile:string
  -> expander:Expander.t
  -> default_context_flags:string list C.Kind.Dict.t
  -> (unit, string list) Build.t C.Kind.Dict.t

val local_binaries
  :  t
  -> profile:string
  -> expander:Expander.t
  -> File_binding.Expanded.t list

val artifacts
  :  t
  -> profile:string
  -> default:Artifacts.t
  -> expander:Expander.t
  -> Artifacts.t
