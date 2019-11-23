(******************************************************************************)
(*                                                                            *)
(*                                   Menhir                                   *)
(*                                                                            *)
(*                       François Pottier, Inria Paris                        *)
(*              Yann Régis-Gianas, PPS, Université Paris Diderot              *)
(*                                                                            *)
(*  Copyright Inria. All rights reserved. This file is distributed under the  *)
(*  terms of the GNU General Public License version 2, as described in the    *)
(*  file LICENSE.                                                             *)
(*                                                                            *)
(******************************************************************************)

(* This code is copied from the library Fix by François Pottier,
   with manual tweaks.
   We prefer to avoid any dependency on an external library. *)

module type TYPE = sig
  type t
end

module type IMPERATIVE_MAPS = sig
  type key
  type 'data t
  val create: unit -> 'data t
  val add: key -> 'data -> 'data t -> unit
  val find: key -> 'data t -> 'data
  val clear: 'data t -> unit
  val iter: (key -> 'data -> unit) -> 'data t -> unit
end

module type MEMOIZER = sig
  (* A type of keys. *)
  type key
  (* A memoization combinator for this type. *)
  val memoize: (key -> 'a) -> (key -> 'a)
  (* A recursive memoization combinator for this type. *)
  val fix: ((key -> 'a) -> (key -> 'a)) -> (key -> 'a)
  (* [defensive_fix] works like [fix], except it additionally detects circular
     dependencies, which can arise if the second-order function supplied by
     the user does not follow a well-founded recursion pattern. When the user
     invokes [f x], where [f] is the function returned by [defensive_fix], if
     a cyclic dependency is detected, then [Cycle (zs, z)] is raised, where
     the list [zs] begins with [z] and continues with a series of intermediate
     keys, leading back to [z]. Note that undetected divergence remains
     possible; this corresponds to an infinite dependency chain, without a
     cycle. *)
  exception Cycle of key list * key
  val defensive_fix: ((key -> 'a) -> (key -> 'a)) -> (key -> 'a)
end

(* [Make] constructs a memoizer for a type [key] that is
   equipped with an implementation of imperative maps. *)

module Make
  (M : IMPERATIVE_MAPS)
     : MEMOIZER with type key = M.key

(* [ForOrderedType] is a special case of [Make] where it
   suffices to pass an ordered type [T] as an argument.
   A reference to a persistent map is used to hold the
   memoization table. *)

module ForOrderedType
  (T : Map.OrderedType)
     : MEMOIZER with type key = T.t

(* [ForHashedType] is a special case of [Make] where it
   suffices to pass a hashed type [T] as an argument. A
   hash table is used to hold the memoization table. *)

module ForHashedType
  (T : Hashtbl.HashedType)
     : MEMOIZER with type key = T.t

(* [ForType] is a special case of [Make] where it suffices
   to pass an arbitrary type [T] as an argument. A hash table
   is used to hold the memoization table. OCaml's built-in
   generic equality and hash functions are used. *)

module ForType
  (T : TYPE)
     : MEMOIZER with type key = T.t

(* Memoizers for some common types. *)

module Int
     : MEMOIZER with type key = int

module String
     : MEMOIZER with type key = string
