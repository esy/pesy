module type Hashtbl = sig
  module Key : sig
    type t
    val equal : t -> t -> bool
    val hash : t -> int
    val to_dyn : t -> Dyn.t
  end

  module H : module type of Hashtbl.Make(Key)

  type value
  val value : value H.t
end

type ('input, 'output) t =
  (module Hashtbl with type value = 'output and type Key.t = 'input)

module type Key = sig
  type t
  val equal : t -> t -> bool
  val hash : t -> int
  val to_dyn : t -> Dyn.t
end

let create (type key) (type value)
      (module Key : Key with type t = key) size : (key, value) t =
  (module struct
    module Key = Key
    module H = Hashtbl.Make(Key)
    type nonrec value = value
    let value = H.create size
  end)

let find (type input) (type output) ((module T) : (input, output) t) x =
  T.H.find T.value x

let set (type input) (type output) ((module T) : (input, output) t) k v =
  T.H.set T.value k v

let add_exn (type input) (type output) (t : (input, output) t) k v =
  let (module T) = t in
  match find t k with
  | None -> set t k v
  | Some _ ->
    Code_error.raise "Table.add_exn: key already exists"
      ["key", T.Key.to_dyn k]

let add t k v =
  match find t k with
  | None -> set t k v; Result.Ok ()
  | Some e -> Error e

let clear (type input) (type output) ((module T) : (input, output) t) =
  T.H.clear T.value
