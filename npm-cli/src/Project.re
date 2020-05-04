type t = {
  manifest: string,
  hash: string,
};
let ofPath = projectPath =>
  Esy.manifestPath(projectPath, esy)
  >>= (
    rootPackageConfigPath =>
      Fs.readFile(. rootPackageConfigPath)
      |> Js.Promise.then_(bytes =>
           bytes |> Result.return |> Js.Promise.resolve
         )
      >>= (
        manifestBytes =>
          PesyConfig.make(manifestBytes |> Buffer.toString)
          |> Js.Promise.resolve
      )
  );
