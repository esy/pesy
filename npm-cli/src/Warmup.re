open Bindings;
open ResultPromise;

let prepareAzureCacheURL = projStr => {
  let proj =
    projStr
    |> AzurePipelines.ProjectName.ofString
    |> AzurePipelines.ProjectName.validate;
  AzurePipelines.getBuildID(proj)
  >>= (id => AzurePipelines.getDownloadURL(proj, id));
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

let run = (esy, project) => {
  let projectHash = Project.lockFileHash(project);
  let projectPath = Project.path(project);
  let cacheDir = Path.join([|Os.tmpdir(), "pesy-" ++ projectHash|]);
  let cachePath = Path.join([|cacheDir, "cache.zip"|]);
  Fs.exists(cachePath)
  |> Js.Promise.then_(cacheZipFound =>
       if (cacheZipFound) {
         ResultPromise.ok();
       } else {
         Project.pesyConfig(project)
         |> Js.Promise.resolve
         >>= (
           pesyConfig => {
             let azureProject = PesyConfig.getAzureProject(pesyConfig);
             Js.log2(
               "Fetching prebuilts for azure project" |> Chalk.dim,
               azureProject |> Chalk.whiteBright,
             );
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
         );
       }
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
        >>| (
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
