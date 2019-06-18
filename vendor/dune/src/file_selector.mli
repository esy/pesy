(** A File_selector.t is a predicate that is to be evaluated in a particular
    directory *)

open Stdune

type t

val dir : t -> Path.t

val create : dir:Path.t -> string Predicate.t -> t

val equal : t -> t -> bool

val hash : t -> int

val compare : t -> t -> Ordering.t

val pp : t Fmt.t

val to_dyn : t -> Dyn.t

val encode : t Dune_lang.Encoder.t

val to_sexp : t -> Sexp.t

val test : t -> Path.t -> bool
