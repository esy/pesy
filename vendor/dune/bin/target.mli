open Stdune

type t =
  | File      of Path.t
  | Alias     of Alias.t

val request
  :  Dune.Main.build_system
  -> t list
  -> (unit, unit) Dune.Build.t

val resolve_target
  : Common.t
  -> setup:Dune.Main.build_system
  -> string
  -> (t list, Path.t * string) result

type resolve_input =
  | Path of Path.t
  | String of string

val resolve_targets_mixed
  :  log:Dune.Log.t
  -> Common.t
  -> Dune.Main.build_system
  -> resolve_input list
  -> (t list, Path.t * string) result list

val resolve_targets_exn
  :  log:Dune.Log.t
  -> Common.t
  -> Dune.Main.build_system
  -> string list
  -> t list
