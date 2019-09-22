(** Workspaces definitions *)

open! Stdune
open! Import

module Context : sig
  module Target : sig
    type t =
      | Native
      | Named of string
  end
  module Common : sig
    type t =
      { loc          : Loc.t
      ; profile      : string
      ; targets      : Target.t list
      ; env          : Dune_env.Stanza.t option
      ; toolchain    : string option
      ; name         : string
      ; host_context : string option
      }
  end
  module Opam : sig
    type t =
      { base    : Common.t
      ; switch  : string
      ; root    : string option
      ; merlin  : bool
      }
  end

  module Default : sig
    type t = Common.t
  end

  type t = Default of Default.t | Opam of Opam.t

  val loc : t -> Loc.t

  val name : t -> string

  val env : t -> Dune_env.Stanza.t option

  val host_context : t -> string option
end

(** Representation of a workspace. The list of context is
    topologically sorted, i.e. a context always comes before the
    contexts where it is used as host context. *)
type t = private
  { merlin_context : string option
  ; contexts       : Context.t list
  ; env            : Dune_env.Stanza.t option
  }

val load : ?x:string -> ?profile:string -> Path.t -> t

(** Default name of workspace files *)
val filename : string

(** Default configuration *)
val default : ?x:string -> ?profile:string -> unit -> t
