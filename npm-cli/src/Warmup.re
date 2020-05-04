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

open ResultPromise;

let run = (esy, project) => {
  Project.pesyConfig(project)
  >>= (
    pesyConfig => {
      let azureProject = PesyConfig.getAzureProject(pesyConfig);
      Js.log2(
        "Fetching prebuilts for azure project" |> Chalk.dim,
        azureProject |> Chalk.whiteBright,
      );
      let projectHash = "-------";
      let cacheDir = Path.join([|Os.tmpdir(), "pesy-" ++ projectHash|]);
      let cachePath = Path.join([|cacheDir, "cache.zip"|]);
      Fs.mkdir(~p=true, cacheDir)
      |> Js.Promise.then_(() => prepareAzureCacheURL(azureProject));
    }
  )
  >>= (
    downloadUrl =>
      Js.Promise.make((~resolve, ~reject as _) => {
        let progress = bytes => {
          bytes
          |> toHumanReadableBytes
          |> (
            x =>
              Process.Stdout.(
                write(v, "Downloading " ++ (x |> Chalk.green) ++ "\r")
              )
          );
        };
        let error = error => {
          Js.log(error);
          resolve(. Error("Download failed"));
        };
        let end_ = () => {
          Js.log(
            ("Downloaded. " |> Chalk.green)
            ++ ("Hydrating esy cache..." |> Chalk.whiteBright),
          );
          resolve(. Ok());
        };
        download(downloadUrl, cachePath, ~progress, ~error, ~end_);
      })
  )
  >>= (() => Cmd.make(~env=Process.env, ~cmd="unzip"))
  >>= (cmd => Cmd.output(~cwd=cacheDir, ~cmd, ~args=[|"-o", cachePath|]))
  >>= (
    _stdout =>
      switch (AzurePipelines.artifactName) {
      | None =>
        ResultPromise.fail("Couldn't determine correct artifactName/os")
      | Some(artifactName) =>
        Esy.importDependencies(
          ~projectPath,
          ~exportsPath=Path.join([|cacheDir, artifactName, "_export"|]),
          esy,
        )
        >>= (
          ((stdout, stderr)) => {
            Js.log(
              ("Running " |> Chalk.dim)
              ++ ("esy import-dependencies" |> Chalk.bold),
            );
            Process.Stdout.(write(v, stdout |> Chalk.dim));
            Process.Stdout.(write(v, stderr |> Chalk.dim));
          }
        )
      }
  );
};
