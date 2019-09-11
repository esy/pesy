open Stdune

let print pp = Format.printf "%a@." Pp.render_ignore_tags pp
let print_dyn dyn = print (Dyn.pp dyn)

let init =
  let init = lazy (
    Printexc.record_backtrace false;
    Path.set_root (Path.External.cwd ());
    Path.Build.set_build_dir (Path.Build.Kind.of_string "_build")
  ) in
  fun () -> Lazy.force init
