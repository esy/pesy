open Printf;

/*  @esy-ocaml/foo-package -> foo-package */
let esyCommand =
  Sys.unix
    ? "esy"
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

      let v =
        List.fold_right(
          (e, acc) => {
            let (_, v) = e;
            acc ++ (Sys.unix ? ":" : ";") ++ v;
          },
          pathVars,
          "",
        );

      /* Unix.putenv("PATH", v); /\* This didn't work! *\/ */

      let paths = Str.split(Str.regexp(Sys.unix ? ":" : ";"), v);

      let npmPaths =
        List.filter(
          path => Sys.file_exists(Filename.concat(path, "esy.cmd")),
          paths,
        );
      switch (npmPaths) {
      | [] =>
        fprintf(stderr, "No npm bin path found");
        exit(-1);
      | [h, ..._] => Filename.concat(h, "esy.cmd")
      };
    };
