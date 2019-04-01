open Printf;

let esy_command = "esy";

/*  @esy-ocaml/foo-package -> foo-package */
let resolveEsyCommand = () =>
  Sys.unix
    ? try (
        Sys.command(sprintf("%s --version &>/dev/null", esy_command)) == 0
          ? Some(esy_command) : None
      ) {
      | _ => None
      }
    : {
      let pathVars =
        Array.to_list(Unix.environment())
        |> List.map(e =>
             switch (Str.split(Str.regexp("="), e)) {
             | [k, v, ...rest] => Some((k, v))
             | _ => None
             }
           )
        |> List.filter(
             fun
             | None => false
             | _ => true,
           )
        |> List.filter(e =>
             switch (e) {
             | Some((k, _)) => String.lowercase_ascii(k) == "path"
             | _ => false
             }
           )
        |> List.map(
             fun
             | Some(x) => x
             | None => ("", "") /* Why not filter_map? */
           );

      let paths =
        Str.split(
          Str.regexp( Sys.unix ? ": ": ";"),
          String.concat(
            Sys.unix ? ": ": ";",
              List.map(
              (e) => {
                let (_, v) = e;
                v;
              },
              pathVars,
            )
          )
        );

      /* Unix.putenv("PATH", v); /\* This didn't work! *\/ */

      let npmPaths =
        List.filter(
          path => Sys.file_exists(Filename.concat(path, "esy.cmd")),
          paths,
        );
      switch (npmPaths) {
      | [] => None
      | [h, ..._] => Some(Filename.concat(h, "esy.cmd"))
      };
    };
