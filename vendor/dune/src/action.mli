open! Stdune
open! Import

module Outputs : module type of struct include Action_intf.Outputs end

(** result of the lookup of a program, the path to it or information about the
    failure and possibly a hint how to fix it *)
module Prog : sig
  module Not_found : sig
    type t = private
      { context : string
      ; program : string
      ; hint    : string option
      ; loc     : Loc.t option
      }

    val create
      :  ?hint:string
      -> context:string
      -> program:string
      -> loc:Loc.t option
      -> unit
      -> t

    val raise : t -> _
  end

  type t = (Path.t, Not_found.t) result
end

include Action_intf.Ast
  with type program = Prog.t
  with type path    = Path.t
  with type target  = Path.Build.t
  with type string  = string

include Action_intf.Helpers
  with type program := Prog.t
  with type path    := Path.t
  with type target  := Path.Build.t
  with type string  := string
  with type t       := t

val decode : t Dune_lang.Decoder.t

module For_shell : sig
  include Action_intf.Ast
    with type program := string
    with type path    := string
    with type target  := string
    with type string  := string

  val encode : t Dune_lang.Encoder.t
end

(** Convert the action to a format suitable for printing *)
val for_shell : t -> For_shell.t

(** Return the list of directories the action chdirs to *)
val chdirs : t -> Path.Set.t

(** Ast where programs are not yet looked up in the PATH *)
module Unresolved : sig
  type action = t

  module Program : sig
    type t =
      | This   of Path.t
      | Search of Loc.t option * string

    val of_string : dir:Path.t -> loc:Loc.t option -> string -> t
  end

  include Action_intf.Ast
    with type program := Program.t
    with type path    := Path.t
    with type target  := Path.Build.t
    with type string  := string

  val resolve : t -> f:(Loc.t option -> string -> Path.t) -> action
end with type action := t

(** Return a sandboxed version of an action *)
val sandbox
  :  t
  -> sandboxed:(Path.Build.t -> Path.Build.t)
  -> deps:Dep.Set.t
  -> targets:Path.Build.t list
  -> eval_pred:Dep.eval_pred
  -> t
