let error_of_exn exn =
  match Location.error_of_exn exn with
  | Some (`Ok exn) -> Some exn
  | Some `Already_displayed -> None
  | None -> None

let get_load_paths () =
  Load_path.get_paths ()

let load_path_init l =
  Load_path.init l

let get_unboxed_types () =
  !Clflags.unboxed_types

let set_unboxed_types b =
  Clflags.unboxed_types := b
