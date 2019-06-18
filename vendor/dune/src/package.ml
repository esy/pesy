open! Stdune

module Name = struct
  module T = Interned.Make(struct
      let initial_size = 16
      let resize_policy = Interned.Conservative
      let order = Interned.Natural
    end)()

  include T

  let of_string = make

  let opam_fn (t : t) = to_string t ^ ".opam"

  let meta_fn (t : t) = "META." ^ to_string t

  let version_fn (t : t) = to_string t ^ ".version"

  let pp fmt t = Format.pp_print_string fmt (to_string t)

  let decode = Dune_lang.Decoder.(map string ~f:of_string)

  let encode t = Dune_lang.Encoder.(string (to_string t))

  module Infix = Comparable.Operators(T)
end


type t =
  { name                   : Name.t
  ; path                   : Path.t
  ; version_from_opam_file : string option
  }

let hash { name; path; version_from_opam_file } =
  Hashtbl.hash
    ( Name.hash name
    , Path.hash path
    , Option.hash String.hash version_from_opam_file
    )

let to_dyn { name; path; version_from_opam_file } =
  let open Dyn in
  Record
    [ "name", Name.to_dyn name
    ; "path", Path.to_dyn path
    ; "version_from_opam_file"
    , Option (Option.map ~f:(fun s -> String s) version_from_opam_file)
    ]

let pp fmt t = Dyn.pp fmt (to_dyn t)

let opam_file t = Path.relative t.path (Name.opam_fn t.name)

let meta_file t = Path.relative t.path (Name.meta_fn t.name)
