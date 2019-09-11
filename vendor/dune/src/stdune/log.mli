(** Log file *)

type t

val no_log : t

val create : ?display:Console.Display.t -> ?path:Path.t -> unit -> t

(** Print an information message in the log *)
val info  : t -> string -> unit
val infof : t -> ('a, Format.formatter, unit, unit) format4 -> 'a

(** Print an executed command in the log *)
val command
  :  t
  -> command_line:string
  -> output:string
  -> exit_status:Unix.process_status
  -> unit
