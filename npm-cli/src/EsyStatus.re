type t = {
  isProjectReadyForDev: bool,
  rootPackageConfigPath: string,
};
let make = statusOutput =>
  switch (Json.parse(statusOutput)) {
  | Some(json) =>
    Json.Decode.(
      try(
        {
          rootPackageConfigPath: field("rootPackageConfigPath", string, json),
          isProjectReadyForDev: field("isProjectReadyForDev", bool, json),
        }
        |> Result.return
      ) {
      | DecodeError(msg) => Error(msg)
      }
    )
  | None => Error({j| Json parser failed
$statusOutput |j})
  };

let getRootPackageConfigPath = config => config.rootPackageConfigPath;
let isProjectReadyForDev = config => config.isProjectReadyForDev;
