open Bindings;

let isEmpty = path =>
  !Node.Fs.existsSync(path)
  || Belt.Array.length(Node.Fs.readdirSync(path)) == 0;

let forEachEnt = (f, dir) => {
  Js.Promise.(
    Fs.readdir(dir)
    |> then_(files => files |> Array.map(f) |> all)
    |> then_(_ => ResultPromise.ok())
  );
};

let rec scan = (f, dir) => {
  Js.Promise.(
    forEachEnt(
      entry => {
        let entryPath = Path.join([|dir, entry|]);
        f(entryPath)
        |> then_(_ =>
             Fs.isDirectory(entryPath)
             |> then_(isDir =>
                  if (isDir) {
                    scan(f, entryPath);
                  } else {
                    ResultPromise.ok();
                  }
                )
           );
      },
      dir,
    )
  );
};
