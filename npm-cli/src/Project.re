open ResultPromise;

type t = {
  path: Path.t,
  manifest: string,
  hash: string,
};

let ofPath = (esy, projectPath) =>
  Esy.manifestPath(projectPath, esy)
  >>= (
    rootPackageConfigPath =>
      Bindings.Fs.readFile(. rootPackageConfigPath)
      |> Js.Promise.then_(bytes =>
           bytes |> Result.return |> Js.Promise.resolve
         )
      >>= (
        manifestBytes => {
          [|
            rootPackageConfigPath
            |> Js.String.replace("package.json", "esy.json")
            |> Js.String.replace(".json", ".lock"),
            "index.json",
          |]
          |> Bindings.Path.join
          |> EsyLock.ofPath
          >>| (lock => EsyLock.checksum(lock))
          >>= (
            checksum => {
              let manifest = manifestBytes |> Bindings.Buffer.toString;
              ResultPromise.ok({hash: checksum, path: projectPath, manifest});
            }
          );
        }
      )
  );

let lockFileHash = v => v.hash;
let pesyConfig = v => PesyConfig.make(v.manifest);
let path = v => v.path;

let hasBuildDirsConfig = v =>
  switch (PesyConfig.Build.ofString(v.manifest)) {
  | Error(_) => false
  | Ok(_) => true
  };
