open Bindings;
open ResultPromise;

let prepareAzureCacheURL = (projStr, github) => {
  let proj =
    projStr
    |> AzurePipelines.ProjectName.ofString
    |> AzurePipelines.ProjectName.validate;
  AzurePipelines.getDefinitionID(proj, github)
  >>= (definitionID => AzurePipelines.getBuildID(proj, definitionID))
  >>= (id => AzurePipelines.getDownloadURL(proj, id));
};

let getLatestAzureBuildId = (proj, github) => {
  AzurePipelines.getDefinitionID(proj, github)
  >>= (definitionID => AzurePipelines.getBuildID(proj, definitionID));
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

let getUnzipCmd = () => {
  Process.platform == "win32"
    ? Cmd.ofPathStr(
        Path.resolve([|
          dirname,
          "..",
          "esy",
          "node_modules",
          "esy-bash",
          ".cygwin",
          "bin",
          "unzip.exe",
        |]),
      )
      |> Js.Promise.then_(cmd => {
           switch (cmd) {
           | Ok(cmd) => Ok(cmd) |> Js.Promise.resolve
           | Error(_) =>
             Cmd.ofPathStr(
               Path.resolve([|
                 dirname,
                 "..",
                 "@esy-nightly",
                 "esy",
                 "node_modules",
                 "esy-bash",
                 ".cygwin",
                 "bin",
                 "unzip.exe",
               |]),
             )
           }
         })
    : Cmd.make(~cmd="unzip", ~env=Process.env);
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

let readFileContentAsString = filePath =>
  Fs.readFile(. filePath)
  |> Js.Promise.then_(bytes =>
       Bindings.Buffer.toString(bytes) |> Js.String.trim |> Js.Promise.resolve
     );

type checksumMatch =
  | ChecksumMatch
  | ChecksumNotMatch;

type cacheState =
  | CacheFound(checksumMatch)
  | CacheNotFound;

let verifyChecksum = (~checksumFilePath, ~cacheZipPath) =>
  Js.Promise.all2((
    readFileContentAsString(checksumFilePath),
    Crypto.sha256File(cacheZipPath),
  ))
  |> Js.Promise.then_(((checksumToMatch, calculatedChecksum)) => {
       Js.Promise.resolve(
         if (checksumToMatch == calculatedChecksum) {
           ChecksumMatch;
         } else {
           ChecksumNotMatch;
         },
       )
     });

let checkCacheState = (~checksumFilePath, ~cacheZipPath) =>
  Js.Promise.all2((Fs.exists(cacheZipPath), Fs.exists(checksumFilePath)))
  |> Js.Promise.then_(((cacheZipFound, checksumFileFound)) =>
       if (cacheZipFound && checksumFileFound) {
         verifyChecksum(~checksumFilePath, ~cacheZipPath)
         |> Js.Promise.then_(checksumMatch =>
              Js.Promise.resolve(CacheFound(checksumMatch))
            );
       } else {
         Js.Promise.resolve(CacheNotFound);
       }
     );

let rec downloadCacheFromGithub =
        (~github, ~templateTag, ~cacheDir, ~cacheZipPath, ~checksumFilePath) => {
  let os =
    switch (Process.platform) {
    | "darwin" => Some("Darwin")
    | "linux" => Some("Linux")
    | "win32" => Some("Windows_NT")
    | _ => None
    };
  Js.log2(
    "Fetching prebuilts from Github" |> Chalk.dim,
    github |> Chalk.whiteBright,
  );
  Rimraf.run(cacheDir)
  |> Js.Promise.then_(_ => Fs.mkdir(cacheDir))
  |> (
    _ =>
      Resolve.resolve(
        {j|https://github.com/$github/releases/download/$templateTag/cache-$os-install-v1-checksum|j},
      )
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
                write(
                  v,
                  "Downloading checksum file " ++ (x |> Chalk.green) ++ "\r",
                )
              )
          );
        };
        let error = error => {
          Js.log(error);
          resolve(. Error("Checksum file download failed"));
        };
        let end_ = () => {
          Js.log("\nChecksum file downloaded. " |> Chalk.green);
          resolve(. Ok());
        };
        download(downloadUrl, checksumFilePath, ~progress, ~error, ~end_);
      })
  )
  >>= (
    _ =>
      Resolve.resolve(
        {j|https://github.com/$github/releases/download/$templateTag/cache-$os-install-v1|j},
      )
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
                write(
                  v,
                  "Downloading cache files " ++ (x |> Chalk.green) ++ "\r",
                )
              )
          );
        };
        let error = error => {
          Js.log(error);
          resolve(. Error("Cache files download failed"));
        };
        let end_ = () => {
          Js.log("\nCache files downloaded" |> Chalk.green);
          resolve(. Ok());
        };
        download(downloadUrl, cacheZipPath, ~progress, ~error, ~end_);
      })
  )
  >>= (_ => getUnzipCmd())
  >>= (
    cmd =>
      Cmd.output(
        ~cwd=cacheDir,
        ~cmd,
        ~args=[|"-d _export", "-o", cacheZipPath|],
      )
  )
  >>= (
    _ => {
      Process.Stdout.(write(v, "Verifying checksum... "));
      verifyChecksum(~checksumFilePath, ~cacheZipPath)
      |> Js.Promise.then_(cacheState =>
           switch (cacheState) {
           | ChecksumMatch =>
             Process.Stdout.(
               write(v, Chalk.green("checksum match") ++ "\n")
             );
             ResultPromise.ok();
           | ChecksumNotMatch =>
             Process.Stdout.(
               write(v, Chalk.red("checksum does not match") ++ "\n")
             );
             Js.Promise.make((~resolve, ~reject as _) => {
               Utils.askYesNoQuestion(
                 ~question=
                   "Looks like there is an issue with the downloaded files. Do you want to re-download these files ?",
                 ~onYes=
                   () =>
                     downloadCacheFromGithub(
                       ~github,
                       ~cacheDir,
                       ~cacheZipPath,
                       ~checksumFilePath,
                     ),
                 ~onNo=
                   () =>
                     resolve(.
                       Result.fail(
                         "Issue with the downloaded files (checksum does not match)",
                       ),
                     ),
                 (),
               )
             });
           }
         );
    }
  );
};

let rec downloadCacheFromAzure =
        (
          ~azureProject,
          ~github,
          ~cacheDir,
          ~checksumZipPath,
          ~cacheZipPath,
          ~checksumFilePath,
          ~exportPath,
        ) => {
  Js.log2(
    "Fetching prebuilts for azure project" |> Chalk.dim,
    azureProject |> AzurePipelines.ProjectName.toString |> Chalk.whiteBright,
  );
  Rimraf.run(cacheDir)
  |> Js.Promise.then_(_ => Fs.mkdir(cacheDir))
  |> Js.Promise.then_(() => getLatestAzureBuildId(azureProject, github))
  >>= (
    buildId => {
      AzurePipelines.getChecksumDownloadURL(azureProject, buildId)
      >>= (
        downloadUrl =>
          Js.Promise.make((~resolve, ~reject as _) => {
            let progress = bytes => {
              bytes
              |> toHumanReadableBytes
              |> (
                x =>
                  Process.Stdout.(
                    write(
                      v,
                      "Downloading checksum file" ++ (x |> Chalk.green) ++ "\r",
                    )
                  )
              );
            };
            let error = error => {
              Js.log(error);
              resolve(. Error("Checksum file download failed"));
            };
            let end_ = () => {
              Js.log("\nChecksum file downloaded. " |> Chalk.green);
              resolve(. Ok());
            };
            download(downloadUrl, checksumZipPath, ~progress, ~error, ~end_);
          })
      )
      >>= (_ => getUnzipCmd())
      >>= (
        cmd => {
          Cmd.output(
            ~cwd=cacheDir,
            ~cmd,
            ~args=[|"-j", "-o", checksumZipPath|],
          )
          >>= (
            _ => {
              Js.log("Extracting files...");
              Cmd.output(
                ~cwd=cacheDir,
                ~cmd,
                ~args=[|"-j", "-o", cacheZipPath|],
              );
            }
          )
          >>= (
            _ =>
              Cmd.output(
                ~cwd=cacheDir,
                ~cmd,
                ~args=[|"-d _export", "-o", exportPath|],
              )
          );
        }
      )
      >>= (_ => AzurePipelines.getDownloadURL(azureProject, buildId))
      >>= (
        downloadUrl =>
          Js.Promise.make((~resolve, ~reject as _) => {
            let progress = bytes => {
              bytes
              |> toHumanReadableBytes
              |> (
                x =>
                  Process.Stdout.(
                    write(
                      v,
                      "Downloading cache files " ++ (x |> Chalk.green) ++ "\r",
                    )
                  )
              );
            };
            let error = error => {
              Js.log(error);
              resolve(. Error("Cache files download failed"));
            };
            let end_ = () => {
              Js.log("\nCache files Downloaded. " |> Chalk.green);
              resolve(. Ok());
            };
            download(downloadUrl, cacheZipPath, ~progress, ~error, ~end_);
          })
      )
      >>= (
        _ => {
          Process.Stdout.(write(v, "Verifying checksum... "));

          verifyChecksum(~checksumFilePath, ~cacheZipPath)
          |> Js.Promise.then_(cacheState =>
               switch (cacheState) {
               | ChecksumMatch =>
                 Process.Stdout.(
                   write(v, Chalk.green("checksum match") ++ "\n")
                 );
                 ResultPromise.ok();
               | ChecksumNotMatch =>
                 Process.Stdout.(
                   write(v, Chalk.red("checksum does not match") ++ "\n")
                 );
                 Js.Promise.make((~resolve, ~reject as _) => {
                   Utils.askYesNoQuestion(
                     ~question=
                       "Looks like there is an issue with the downloaded files. Do you want to re-download these files ?",
                     ~onYes=
                       () =>
                         downloadCacheFromAzure(
                           ~azureProject,
                           ~github,
                           ~cacheDir,
                           ~checksumZipPath,
                           ~cacheZipPath,
                           ~checksumFilePath,
                           ~exportPath,
                         ),
                     ~onNo=
                       () =>
                         resolve(.
                           Result.fail(
                             "Issue with the downloaded files (checksum does not match)",
                           ),
                         ),
                     (),
                   )
                 });
               }
             );
        }
      );
    }
  );
};

let run = (esy, project) => {
  let projectHash = Project.lockFileHash(project);
  let projectPath = Project.path(project);
  let cacheDir = Path.join([|Os.tmpdir(), "pesy-" ++ projectHash|]);
  let checksumZipPath = Path.join([|cacheDir, "checksum.zip"|]);
  let checksumFilePath = Path.join([|cacheDir, "checksum.txt"|]);
  let cacheZipPath = Path.join([|cacheDir, "cache.zip"|]);
  let exportPath = Path.join([|cacheDir, "_export"|]);

  checkCacheState(~checksumFilePath, ~cacheZipPath)
  |> Js.Promise.then_(cacheState =>
       switch (cacheState) {
       | CacheFound(ChecksumMatch) => ResultPromise.ok()
       | CacheNotFound
       | CacheFound(ChecksumNotMatch) =>
         Js.Promise.resolve(Project.pesyConfig(project))
         >>= (
           pesyConfig => {
             let github = PesyConfig.getGithub(pesyConfig);
             let templateVersion = PesyConfig.getTemplateTag(pesyConfig);
             let azureProject = PesyConfig.getAzureProject(pesyConfig);
             switch (templateVersion, azureProject) {
             | (None, None) =>
               downloadCacheFromGithub(
                 ~github,
                 ~templateTag="latest",
                 ~cacheDir,
                 ~cacheZipPath,
                 ~checksumFilePath,
               )
             | (Some(templateTag), _) =>
               downloadCacheFromGithub(
                 ~github,
                 ~templateTag,
                 ~cacheDir,
                 ~cacheZipPath,
                 ~checksumFilePath,
               )
             | (None, Some(azureProject)) =>
               let azureProject =
                 azureProject
                 |> AzurePipelines.ProjectName.ofString
                 |> AzurePipelines.ProjectName.validate;
               downloadCacheFromAzure(
                 ~azureProject,
                 ~github,
                 ~cacheDir,
                 ~checksumZipPath,
                 ~cacheZipPath,
                 ~checksumFilePath,
                 ~exportPath,
               );
             };
           }
         )
       }
     )
  >>= (
    _stdout =>
      Esy.importDependencies(
        ~projectPath,
        ~exportsPath=Path.join([|cacheDir, "_export"|]),
        esy,
      )
      |> Js.Promise.then_(
           fun
           | Ok(r) => ResultPromise.ok(r)
           | Error(_) =>
             /* import-dependencies can sometimes fail on individual cache entries which is fail. We can simply retry */
             Esy.importDependencies(
               ~projectPath,
               ~exportsPath=Path.join([|cacheDir, "_export"|]),
               esy,
             ),
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
  );
};
