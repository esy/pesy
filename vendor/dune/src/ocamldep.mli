(** ocamldep management *)

open Stdune

(** Generate ocamldep rules for all the modules in the context. *)
val rules : Compilation_context.t -> Dep_graph.Ml_kind.t

(** Compute the dependencies of an auxiliary module. *)
val rules_for_auxiliary_module
  :  Compilation_context.t
  -> Module.t
  -> Dep_graph.Ml_kind.t

(** Get the dep graph for an already defined library *)
val graph_of_remote_lib
  :  obj_dir:Path.t
  -> modules:Module.t Module.Name.Map.t
  -> Dep_graph.Ml_kind.t
