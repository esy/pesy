open Stdune

val gen_rules :
  Super_context.t -> (dir:Path.Build.t -> Build_system.Subdir_set.t)

val init_meta : Super_context.t -> dir:Path.Build.t -> unit

val packages : Super_context.t -> Package.Name.Set.t Path.Build.Map.t
