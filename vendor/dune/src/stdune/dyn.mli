type t = Dyn0.t =
  | Unit
  | Int of int
  | Bool of bool
  | String of string
  | Bytes of bytes
  | Char of char
  | Float of float
  | Sexp of Sexp0.t
  | Option of t option
  | List of t list
  | Array of t array
  | Tuple of t list
  | Record of (string * t) list
  | Variant of string * t list
  | Map of (t * t) list
  | Set of t list

module Encoder : sig
  type dyn

  type 'a t = 'a -> dyn

  val unit       : unit                      t
  val char       : char                      t
  val string     : string                    t
  val int        : int                       t
  val float      : float                     t
  val bool       : bool                      t
  val pair       : 'a t -> 'b t -> ('a * 'b) t
  val triple     : 'a t -> 'b t -> 'c t -> ('a * 'b * 'c) t
  val list       : 'a t -> 'a list           t
  val array      : 'a t -> 'a array          t
  val option     : 'a t -> 'a option         t

  val via_sexp : ('a -> Sexp0.t) -> 'a t

  val record : (string * dyn) list -> dyn

  val unknown : _ t
  val opaque : _ t

  val constr : string -> dyn list -> dyn
end with type dyn := t

val pp : Format.formatter -> t -> unit

val opaque : t

val to_sexp : t Sexp.Encoder.t
