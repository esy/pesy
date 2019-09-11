open Import

module Kind = struct
  type t = Git | Hg

  let of_dir_contents files =
    if String.Set.mem files ".git" then
      Some Git
    else if String.Set.mem files ".hg" then
      Some Hg
    else
      None

  let to_dyn t =
    Dyn.Variant
      ((match t with
         | Git -> "Git"
         | Hg -> "Hg"),
       [])

  let equal = (=)

  let decode =
    Stanza.Decoder.enum
      [ "git", Git
      ; "hg", Hg
      ]
end

module T = struct
  type t =
    { root : Path.t
    ; kind : Kind.t
    }

  let to_dyn { root; kind } =
    Dyn.Encoder.record
      [ "root", Path.to_dyn root
      ; "kind", Kind.to_dyn kind
      ]

  let equal { root = ra; kind = ka } { root = rb; kind = kb } =
    Path.equal ra rb && Kind.equal ka kb

  (* No need to hash the kind as there is only only kind per directory *)
  let hash t = Path.hash t.root

  let decode =
    let open Stanza.Decoder in
    record
      (let+ root = field "root" Dpath.decode
       and+ kind = field "kind" Kind.decode
       in
       { root; kind })
end

include T

let git, hg =
  let get prog = lazy (
    match Bin.which ~path:(Env.path Env.initial) prog with
    | Some x -> x
    | None -> Utils.program_not_found prog ~loc:None)
  in
  (get "git", get "hg")

let select git hg t =
  match t.kind with
  | Git -> git t
  | Hg -> hg t

let prog t =
  Lazy.force (
    match t.kind with
    | Git -> git
    | Hg -> hg)

let run t args =
  let open Fiber.O in
  let+ s =
    Process.run_capture Strict (prog t) args ~dir:t.root ~env:Env.initial
  in
  String.trim s

let run_lines t args =
  Process.run_capture_lines Strict (prog t) args ~dir:t.root ~env:Env.initial

let hg_describe t =
  let open Fiber.O in
  let* s =
    run t [ "log"; "--rev"; "."; "-T"; "{latesttag} {latesttagdistance}" ]
  in
  let+ id =
    run t [ "id"; "-i" ]
  in
  let id, dirty_suffix =
    match String.drop_suffix id ~suffix:"+" with
    | Some id -> id, "-dirty"
    | None -> id, ""
  in
  let s =
    let s, dist = Option.value_exn (String.rsplit2 s ~on:' ') in
    match s with
    | "null" -> id
    | _ ->
      match int_of_string dist with
      | 1 -> s
      | n -> sprintf "%s-%d-%s" s (n - 1) id
      | exception _ -> sprintf "%s-%s-%s" s dist id
  in
  s ^ dirty_suffix

let make_fun name ~output ~doc ~git ~hg =
  let memo =
    Memo.create
      name
      ~doc
      ~input:(module T)
      ~output
      ~visibility:(Public decode)
      Async
      (select git hg)
  in
  Staged.stage (Memo.exec memo)

let describe =
  Staged.unstage @@
  make_fun "vcs-describe"
    ~doc:"Obtain a nice description of the tip from the vcs"
    ~output:(Simple (module String))
    ~git:(fun t -> run t ["describe"; "--always"; "--dirty"])
    ~hg:hg_describe

let commit_id =
  Staged.unstage @@
  make_fun "vcs-commit-id"
    ~doc:"The hash of the head commit"
    ~output:(Simple (module String))
    ~git:(fun t -> run t ["rev-parse"; "HEAD"])
    ~hg:(fun t -> run t ["id"; "-i"])

let files =
  let f args t =
    let open Fiber.O in
    let+ l = run_lines t args in
    List.map l ~f:Path.in_source
  in
  Staged.unstage @@
  make_fun "vcs-files"
    ~doc:"Return the files committed in the repo"
    ~output:(Simple (module struct
                type t = Path.t list
                let to_dyn = Dyn.Encoder.list Path.to_dyn
              end))
    ~git:(f ["ls-tree"; "-r"; "--name-only"; "HEAD"])
    ~hg:(f ["files"])
