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

(* Sigs. *)

module type TYPE = sig
  type t
end

module type PERSISTENT_MAPS = sig
  type key
  type 'data t
  val empty: 'data t
  val add: key -> 'data -> 'data t -> 'data t
  val find: key -> 'data t -> 'data
  val iter: (key -> 'data -> unit) -> 'data t -> unit
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

(* -------------------------------------------------------------------------- *)

(* Glue. *)

module INT = struct
  type t = int
end

module STRING = struct
  type t = string
end

module TrivialHashedType (T : TYPE) = struct
  include T
  let equal = (=)
  let hash = Hashtbl.hash
end

module PersistentMapsToImperativeMaps (M : PERSISTENT_MAPS) = struct

  type key =
    M.key

  type 'data t =
    'data M.t ref

  let create () =
    ref M.empty

  let clear t =
    t := M.empty

  let add k d t =
    t := M.add k d !t

  let find k t =
    M.find k !t

  let iter f t =
    M.iter f !t

end

module Adapt (T : Hashtbl.S) = struct

  include T
    (* types:  [key], ['data t] *)
    (* values: [clear], [iter]  *)

  let create () =
    T.create 1023

  let add key data table =
    T.add table key data

  let find table key =
    T.find key table

end

module HashTablesAsImperativeMaps (H : Hashtbl.HashedType) =
  Adapt(Hashtbl.Make(H))

(* -------------------------------------------------------------------------- *)

(* Memoize. *)

(* [rev_take accu n xs] is [accu @ rev (take n xs)], where [take n xs]
   takes the first [n] elements of the list [xs]. The length of [xs] must
   be at least [n]. *)

let rec rev_take accu n xs =
  match n, xs with
  | 0, _ ->
      accu
  | _, [] ->
      (* The list is too short. This cannot happen. *)
      assert false
  | _, x :: xs ->
      rev_take (x :: accu) (n - 1) xs

module Make (M : IMPERATIVE_MAPS) = struct

  type key = M.key

  let add x y table =
    M.add x y table;
    y

  (* [memoize] could be defined as a special case of [fix] via the declaration
     [let memoize f = fix (fun _ x -> f x)]. The following direct definition is
     perhaps easier to understand and may give rise to more efficient code. *)

  let memoize (f : key -> 'a) : key -> 'a =
    let table = M.create() in
    fun x ->
      try
	M.find x table
      with Not_found ->
        add x (f x) table

  let fix (ff : (key -> 'a) -> (key -> 'a)) : key -> 'a =
    let table = M.create() in
    let rec f x =
      try
	M.find x table
      with Not_found ->
	add x (ff f x) table
    in
    f

  (* In the implementation of [defensive_fix], we choose to use two tables.
     A permanent table, [table] maps keys to values. Once a pair [x, y] has
     been added to this table, it remains present forever: [x] is stable,
     and a call to [f x] returns [y] immediately. A transient table, [marked],
     is used only while a call is in progress. This table maps keys to integers:
     for each key [x], it records the depth of the stack at the time [x] was
     pushed onto the stack. Finally, [stack] is a list of the keys currently
     under examination (most recent key first), and [depth] is the length of
     the list [stack]. Recording integer depths in the table [marked] allows
     us to identify the desired cycle, a prefix of the list [stack], without
     requiring an equality test on keys. *)

  exception Cycle of key list * key

  let defensive_fix (ff : (key -> 'a) -> (key -> 'a)) : key -> 'a =
    (* Create the permanent table. *)
    let table = M.create() in
    (* Define the main recursive function. *)
    let rec f stack depth marked (x : key) : 'a =
      try
	M.find x table
      with Not_found ->
        match M.find x marked with
        | i ->
            (* [x] is marked, and was pushed onto the stack at a time when the
               stack depth was [i]. We have found a cycle. Fail. Cut a prefix
               of the reversed stack, which represents the cycle that we have
               detected, and reverse it on the fly. *)
            raise (Cycle (rev_take [] (depth - i) stack, x))
        | exception Not_found ->
           (* [x] is not marked. Mark it while we work on it. There is no need
              to unmark [x] afterwards; inserting it into [table] indicates
              that it has stabilized. There also is no need to catch and
              re-raise the exception [Cycle]; we just let it escape. *)
            M.add x depth marked;
            let stack = x :: stack
            and depth = depth + 1 in
            let y = ff (f stack depth marked) x in
	    add x y table
    in
    fun x ->
      (* Create the transient table. *)
      let marked = M.create()
      and stack = []
      and depth = 0 in
      (* Answer this query. *)
      f stack depth marked x

end

module ForOrderedType (T : Map.OrderedType) =
  Make(PersistentMapsToImperativeMaps(Map.Make(T)))

module ForHashedType (T : Hashtbl.HashedType) =
  Make(HashTablesAsImperativeMaps(T))

module ForType (T : TYPE) =
  ForHashedType(TrivialHashedType(T))

module Int =
  ForType(INT)

module String =
  ForType(STRING)
