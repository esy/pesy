(** META file parsing/printing *)

open! Stdune
open! Import

type t =
  { name    : Lib_name.t option
  ; entries : entry list
  }

and entry =
  | Comment of string
  | Rule    of rule
  | Package of t

and rule =
  { var        : string
  ; predicates : predicate list
  ; action     : action
  ; value      : string
  }

and action = Set | Add

and predicate =
  | Pos of string
  | Neg of string

val parse_entries : Lexing.lexbuf -> entry list

(** Add version fields to all package in [t] that don't have and have
    at least one rule.  [get_version] is used to obtain the version.
    It receives as argument the package path. *)
val add_versions : t -> get_version:(Lib_name.t list -> string option) -> t

module Simplified : sig
  module Rules : sig
    type t =
      { set_rules : rule list
      ; add_rules : rule list
      }
  end

  type t =
    { name : Lib_name.t option
    ; vars : Rules.t String.Map.t
    ; subs : t list
    }

  val to_dyn : t -> Dyn.t
end

val load : Path.t -> name:Lib_name.t option -> Simplified.t

(** Builtin META files for libraries distributed with the compiler. For when ocamlfind is
    not installed. *)
val builtins : stdlib_dir:Path.t -> version:Ocaml_version.t -> Simplified.t Lib_name.Map.t

val pp : Format.formatter -> entry list -> unit
