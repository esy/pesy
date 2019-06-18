open Stdune

type t =
  { dir : Path.t
  ; predicate : string Predicate.t
  }

let dir t = t.dir

let compare x y =
  match Path.compare x.dir y.dir with
  | Ordering.Lt | Gt as a -> a
  | Eq -> Predicate.compare x.predicate y.predicate

let create ~dir predicate = { dir ; predicate }

let to_dyn { dir ; predicate } =
  let open Dyn in
  Record
    [ "dir", Path.to_dyn dir
    ; "predicate", Predicate.to_dyn predicate
    ]

let pp fmt t = Dyn.pp fmt (to_dyn t)

let encode { dir; predicate } =
  let open Dune_lang.Encoder in
  record
    [ "dir", Path_dune_lang.encode dir
    ; "predicate", Predicate.encode predicate
    ]

let equal x y = compare x y = Eq
let hash { dir; predicate} =
  Tuple.T2.hash Path.hash Predicate.hash (dir, predicate)

let to_sexp t = Dyn.to_sexp (to_dyn t)

let test t path = Predicate.test t.predicate (Path.basename path)
