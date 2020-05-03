open Bindings;
module R = Result;

module PesyConfig: {
  type t;
  /* Json decoder */
  let decoder: Json.Decode.decoder(t);
  /* Extracts the Azure Project name from the config */
  let getAzureProject: t => string;
  /* Creates pesy config from a give manifest file */
  let make: string => result(t, string);
} = {
  type t = {azureProject: string};
  let decoder = json =>
    json
    |> Json.Decode.(field("azure-project", string))
    |> (azureProject => {azureProject: azureProject});
  /* Makes a config. Must fail if mandatory fields are not present */
  let make = manifest =>
    switch (manifest |> Json.parse) {
    | None => Error("Json parser failed: " ++ manifest)
    | Some(json) =>
      try(json |> Json.Decode.field("pesy", decoder) |> R.return) {
      | Json.Decode.DecodeError(msg) => Error(msg)
      }
    };
  let getAzureProject = config => config.azureProject;
};

let prepareAzureCacheURL = projStr => {
  let proj =
    projStr
    |> AzurePipelines.ProjectName.ofString
    |> AzurePipelines.ProjectName.validate;
  AzurePipelines.getBuildID(proj)
  |> Js.Promise.then_(
       fun
       | Ok(id) => AzurePipelines.getDownloadURL(proj, id)
       | Error(msg) => Error(msg) |> Js.Promise.resolve,
     );
};

let download = (url, file, ~progress, ~end_, ~error) => {
  open Bindings;
  let stream = RequestProgress.requestProgress(Request.request(url));
  RequestProgress.onProgress(stream, state =>
    progress(state##size##transferred)
  );
  /* RequestProgress.onData(stream, data); */
  RequestProgress.onEnd(stream, end_);
  RequestProgress.onError(stream, error);
  RequestProgress.pipe(stream, Fs.createWriteStream(file));
};

let giga = 1024 * 1024 * 1024;
let mega = 1024 * 1024;
let kilo = 1024;

let divideBy = (n, x) => float_of_int(x) /. float_of_int(n);
let toHumanReadableBytes =
  fun
  | x when x > giga => x |> divideBy(giga) |> Printf.sprintf("%.2f GB  ")
  | x when x > mega => x |> divideBy(mega) |> Printf.sprintf("%.2f MB  ")
  | x when x > kilo => x |> divideBy(kilo) |> Printf.sprintf("%.2f KB  ")
  | x => (x |> string_of_int) ++ " bytes";

let run = projectPath =>
  ResultPromise.(
    {
      Esy.make()
      >>= (
        esy =>
          Esy.manifestPath(projectPath, esy)
          >>= (
            rootPackageConfigPath =>
              Fs.readFile(. rootPackageConfigPath)
              |> Js.Promise.then_(manifestBytes => {
                   switch (PesyConfig.make(manifestBytes |> Buffer.toString)) {
                   | Error(msg) => Error(msg) |> Js.Promise.resolve
                   | Ok(pesyConfig) =>
                     let azureProject =
                       PesyConfig.getAzureProject(pesyConfig);
                     Js.log2(
                       "Fetching prebuilts for azure project" |> Chalk.dim,
                       azureProject |> Chalk.whiteBright,
                     );
                     prepareAzureCacheURL(azureProject)
                     >>= (
                       downloadUrl =>
                         Js.Promise.make((~resolve, ~reject as _) =>
                           download(
                             downloadUrl,
                             "cache.zip",
                             ~progress=
                               bytes => {
                                 bytes
                                 |> toHumanReadableBytes
                                 |> (
                                   x =>
                                     Process.Stdout.write(
                                       Process.Stdout.v,
                                       "Downloading "
                                       ++ (x |> Chalk.green)
                                       ++ "\r",
                                     )
                                 )
                               },
                             ~error=
                               error => {
                                 Js.log(error);
                                 resolve(. Error("Download failed"));
                               },
                             ~end_=
                               () => {
                                 Js.log(
                                   ("Downloaded. " |> Chalk.green)
                                   ++ (
                                     "Hydrating esy cache..."
                                     |> Chalk.whiteBright
                                   ),
                                 );
                                 resolve(. Ok());
                               },
                           )
                         )
                     )
                     >>= (
                       () =>
                         Cmd.make(~env=Process.env, ~cmd="unzip")
                         >>= (
                           cmd =>
                             Cmd.output(
                               ~cwd=projectPath,
                               ~cmd,
                               ~args=[|"-o", "cache.zip"|],
                             )
                             >>| (_ => ())
                         )
                     )
                     >>= (
                       _stdout =>
                         Esy.importDependencies(
                           ~projectPath,
                           ~exportsPath=
                             Path.join([|
                               projectPath,
                               "cache-Darwin-install-v1",
                               "_export",
                             |]),
                           esy,
                         )
                     )
                     >>= (
                       ((stdout, stderr)) => {
                         Js.log(
                           ("Running " |> Chalk.dim)
                           ++ ("esy import-dependencies" |> Chalk.bold),
                         );
                         Process.Stdout.write(
                           Process.Stdout.v,
                           stdout |> Chalk.dim,
                         );
                         Process.Stdout.write(
                           Process.Stdout.v,
                           stderr |> Chalk.dim,
                         );
                         Rimraf.run(Path.join([|projectPath, "cache.zip"|]));
                       }
                     )
                     >>= (
                       () =>
                         Rimraf.run(
                           Path.join([|
                             projectPath,
                             "cache-Darwin-install-v1",
                           |]),
                         )
                     );
                   }
                 })
          )
      );
    }
  );
