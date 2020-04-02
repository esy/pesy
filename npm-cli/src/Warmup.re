open Bindings;
open Utils;
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

module EsyStatus: {
  type t;
  let make: string => result(t, string);
  let getRootPackageConfigPath: t => string;
} = {
  type t = {rootPackageConfigPath: string};
  let make = statusOutput =>
    switch (Json.parse(statusOutput)) {
    | Some(json) =>
      Json.Decode.(
        try(
          field("rootPackageConfigPath", string, json)
          |> (
            rootPackageConfigPath => {
              rootPackageConfigPath: rootPackageConfigPath,
            }
          )
          |> R.return
        ) {
        | DecodeError(msg) => Error(msg)
        }
      )
    | None => Error({j| Json parser failed
$statusOutput |j})
    };

  let getRootPackageConfigPath = config => config.rootPackageConfigPath;
};

module Esy: {
  type t;
  let make: unit => Js.Promise.t(result(t, string));
  let manifestPath: (string, t) => Js.Promise.t(result(string, string));
  let importDependencies:
    (string, t) => Js.Promise.t(result((string, string), string));
} = {
  open ResultPromise;
  type t = Cmd.t;
  let make = () => Cmd.make(~cmd="esy", ~env=Process.env);
  let manifestPath = (path, cmd) => {
    Cmd.output(~cmd, ~cwd=path, ~args=[|"status"|])
    >>= (
      ((output, _stderr)) =>
        Result.(
          EsyStatus.make(output)
          >>| (status => EsyStatus.getRootPackageConfigPath(status))
        )
        |> Js.Promise.resolve
    );
  };
  let importDependencies = (path, cmd) =>
    Cmd.output(~cmd, ~cwd=path, ~args=[|"import-dependencies"|]);
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
                           Path.join([|
                             projectPath,
                             "cache-Darwin-install-v1",
                           |]),
                           esy,
                         )
                     )
                     >>= (
                       ((stdout, stderr)) => {
                         Js.log(
                           ("Running " |> Chalk.whiteBright)
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
