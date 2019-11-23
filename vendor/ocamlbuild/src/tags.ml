(***********************************************************************)
(*                                                                     *)
(*                             ocamlbuild                              *)
(*                                                                     *)
(*  Nicolas Pouillard, Berke Durak, projet Gallium, INRIA Rocquencourt *)
(*                                                                     *)
(*  Copyright 2007 Institut National de Recherche en Informatique et   *)
(*  en Automatique.  All rights reserved.  This file is distributed    *)
(*  under the terms of the GNU Library General Public License, with    *)
(*  the special exception on linking described in file ../LICENSE.     *)
(*                                                                     *)
(***********************************************************************)


(* Original author: Nicolas Pouillard *)

let compare_tags str1 str2 =
  let len1, len2 = String.length str1, String.length str2 in
  if len1 = 0 || len2 = 0 then String.compare str1 str2
  else
    (* if the last character is ')', the tag is an applied
       parametrized tag; place it first for visibility *)
    begin match str1.[len1 - 1] = ')', str2.[len2 - 1] = ')' with
      | true, false -> -1
      | false, true -> 1
      | true, true | false, false -> String.compare str1 str2
    end

include Set.Make(struct
    type t = string
    let compare = compare_tags
  end)

(**
  does_match {foo, bar, baz} {foo} => ok
  does_match {foo, bar, baz} {foo, boo} => ko
  does_match {foo, bar, baz} {} => ok
  does_match {foo, bar, baz} {foo, bar, baz} => ok
*)
let does_match x y = subset y x

let of_list l = List.fold_right add l empty

open Format

let print f s =
  let () = fprintf f "@[<0>" in
  let _ =
    fold begin fun elt first ->
      if not first then fprintf f ",@ ";
      pp_print_string f elt;
      false
    end s true in
  fprintf f "@]"

module Operators = struct
  let ( ++ ) x y = add y x
  let ( -- ) x y = remove y x
  let ( +++ ) x = function Some y -> add y x | None -> x
  let ( --- ) x = function Some y -> remove y x | None -> x
end
