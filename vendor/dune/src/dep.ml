open Stdune

module Trace = struct
  type t = (string * Digest.t) list
end

module T = struct
  type t =
    | Env of Env.Var.t
    | File of Path.t
    | Alias of Alias.t
    | Glob of File_selector.t
    | Universe

  let env e = Env e
  let file f = File f
  let alias a = Alias a
  let universe = Universe
  let glob g = Glob g

  let compare x y =
    match x, y with
    | Env x, Env y -> Env.Var.compare x y
    | Env _, _ -> Lt
    | _, Env _ -> Gt
    | File x, File y -> Path.compare x y
    | File _, _ -> Lt
    | _, File _ -> Gt
    | Alias x, Alias y -> Alias.compare x y
    | Alias _, _ -> Lt
    | _, Alias _ -> Gt
    | Glob x, Glob y -> File_selector.compare x y
    | Glob _, _ -> Lt
    | _, Glob _ -> Gt
    | Universe, Universe -> Ordering.Eq

  let unset = lazy (Digest.string "unset")

  let trace_file fn = (Path.to_string fn, Cached_digest.file fn)

  let trace t ~env ~eval_pred =
    match t with
    | Universe -> ["universe", Digest.string "universe"]
    | File fn -> [trace_file fn]
    | Alias a -> [trace_file (Path.build (Alias.stamp_file a))]
    | Glob dir_glob ->
      eval_pred dir_glob
      |> Path.Set.to_list
      |> List.map ~f:trace_file
    | Env var ->
      let value =
        begin match Env.get env var with
        | None -> Lazy.force unset
        | Some v -> Digest.string v
        end
      in
      [var, value]

  let encode t =
    let open Dune_lang.Encoder in
    match t with
    | Glob g -> pair string File_selector.encode ("glob", g)
    | Env e -> pair string string ("Env", e)
    | File f -> pair string Dpath.encode ("File", f)
    | Alias a -> pair string Alias.encode ("Alias", a)
    | Universe -> string "Universe"

  let to_dyn _ = Dyn.opaque
end

include T

module O = Comparable.Make(T)

module Map = O.Map
module Set = struct
  include O.Set

  let has_universe t = mem t Universe

  let of_files = List.fold_left ~init:empty ~f:(fun acc f -> add acc (file f))

  let of_files_set =
    Path.Set.fold ~init:empty ~f:(fun f acc -> add acc (file f))

  let trace t ~env ~eval_pred =
    List.concat_map (to_list t) ~f:(trace ~env ~eval_pred)

  let add_paths t paths =
    Path.Set.fold paths ~init:t ~f:(fun p set -> add set (File p))

  let encode t = Dune_lang.Encoder.list encode (to_list t)

  let paths t ~eval_pred =
    fold t ~init:Path.Set.empty ~f:(fun d acc ->
      match d with
      | Alias a -> Path.Set.add acc (Path.build (Alias.stamp_file a))
      | File f -> Path.Set.add acc f
      | Glob g -> Path.Set.union acc (eval_pred g)
      | Universe
      | Env _ -> acc)

  let parallel_iter t ~f = Fiber.parallel_iter ~f (to_list t)

  let parallel_iter_files t ~f ~eval_pred =
    paths t ~eval_pred
    |> Path.Set.to_list
    |> Fiber.parallel_iter ~f

  let dirs t =
    fold t ~init:Path.Set.empty ~f:(fun f acc ->
      match f with
      | Alias a -> Path.Set.add acc (Path.build (Alias.dir a))
      | Glob g -> Path.Set.add acc (File_selector.dir g)
      | File f -> Path.Set.add acc (Path.parent_exn f)
      | Universe
      | Env _ -> acc)
end

type eval_pred = File_selector.t -> Path.Set.t
