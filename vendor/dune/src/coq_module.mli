(* This file is licensed under The MIT License *)
(* (c) MINES ParisTech 2018-2019               *)
(*     Written by: Emilio Jesús Gallego Arias  *)

open! Stdune

module Name : sig

  type t

  val make : string -> t
  val compare : t -> t -> Ordering.t

  val pp : t Fmt.t

end

type t

(** A Coq module [a.b.foo] defined in file [a/b/foo.v] *)
val make
  :  source:Path.t
  (** file = .v source file; module name has to be the same so far *)
  -> prefix:string list
  (** Library-local qualified prefix *)
  -> name:Name.t
  (** Name of the module *)
  -> t

(** Coq does enforce some invariants wrt module vs file names *)

val source : t -> Path.t
val prefix : t -> string list
val name : t -> string
val obj_file : obj_dir:Path.t -> ext:string -> t -> Path.t
val pp : t Fmt.t

(** Parses a form "a.b.c" to a module *)
val parse : dir:Path.t -> loc:Loc.t -> string -> t
module Eval : Ordered_set_lang.S with type value := t
