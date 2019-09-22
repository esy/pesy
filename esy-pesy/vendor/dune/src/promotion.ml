open! Stdune

module File = struct
  type t =
    { src : Path.Build.t
    ; dst : Path.Source.t
    }

  let to_dyn { src; dst } =
    let open Dyn.Encoder in
    record
      [ "src", Path.Build.to_dyn src
      ; "dst", Path.Source.to_dyn dst
      ]

  let db : t list ref = ref []

  let register t = db := t :: !db

  let promote { src; dst } =
    let src_exists = Path.exists (Path.build src) in
    Console.print
      (Format.sprintf
         (if src_exists then
            "Promoting %s to %s.@."
          else
            "Skipping promotion of %s to %s as the file is missing.@.")
         (Path.to_string_maybe_quoted (Path.build src))
         (Path.Source.to_string_maybe_quoted dst));
    if src_exists then
      Io.copy_file ~src:(Path.build src) ~dst:(Path.source dst) ()
end

let clear_cache () =
  File.db := []

let () = Hooks.End_of_build.always clear_cache

module P = Persistent.Make(struct
    type t = File.t list
    let name = "TO-PROMOTE"
    let version = 1
  end)

let db_file = Path.relative Path.build_dir ".to-promote"

let dump_db db =
  if Path.build_dir_exists () then begin
    match db with
    | [] -> if Path.exists db_file then Path.unlink_no_err db_file
    | l -> P.dump db_file l
  end

let load_db () = Option.value ~default:[] (P.load db_file)

let group_by_targets db =
  List.map db ~f:(fun { File. src; dst } ->
    (dst, src))
  |> Path.Source.Map.of_list_multi
  (* Sort the list of possible sources for deterministic behavior *)
  |> Path.Source.Map.map ~f:(List.sort ~compare:Path.Build.compare)

type files_to_promote =
  | All
  | These of Path.Source.t list * (Path.Source.t -> unit)

let do_promote db files_to_promote =
  let by_targets = group_by_targets db  in
  let potential_build_contexts =
    match Path.readdir_unsorted Path.build_dir with
    | exception _ -> []
    | Error _ -> []
    | Ok files ->
      List.filter_map files ~f:(fun fn ->
        if fn = "" || fn.[0] = '.' || fn = "install" then
          None
        else
          let path = Path.(relative build_dir) fn in
          Option.some_if (Path.is_directory path) path)
  in
  let dirs_to_clear_from_cache = Path.root :: potential_build_contexts in
  let promote_one dst srcs =
    match srcs with
    | [] -> assert false
    | src :: others ->
      (* We remove the files from the digest cache to force a rehash
         on the next run. We do this because on OSX [mtime] is not
         precise enough and if a file is modified and promoted
         quickly, it will look like it hasn't changed even though it
         might have. *)
      List.iter dirs_to_clear_from_cache ~f:(fun dir ->
        Cached_digest.remove (Path.append_source dir dst));
      File.promote { src; dst };
      List.iter others ~f:(fun path ->
        Format.eprintf " -> ignored %s.@."
          (Path.to_string_maybe_quoted (Path.build path)))
  in
  match files_to_promote with
  | All ->
    Path.Source.Map.iteri by_targets ~f:promote_one;
    []
  | These (files, on_missing) ->
    let files =
      Path.Source.Set.of_list files |> Path.Source.Set.to_list
    in
    let by_targets =
      List.fold_left files ~init:by_targets ~f:(fun map fn ->
        match Path.Source.Map.find by_targets fn with
        | None ->
          on_missing fn;
          map
        | Some srcs ->
          promote_one fn srcs;
          Path.Source.Map.remove by_targets fn)
    in
    Path.Source.Map.to_list by_targets
    |> List.concat_map ~f:(fun (dst, srcs) ->
      List.map srcs ~f:(fun src -> { File.src; dst }))

let finalize () =
  let db =
    if !Clflags.auto_promote then
      do_promote !File.db All
    else
      !File.db
  in
  dump_db db

let promote_files_registered_in_last_run files_to_promote =
  let db = load_db () in
  let db = do_promote db files_to_promote in
  dump_db db
