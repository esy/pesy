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
    try(json |> Json.Decode.field("pesy", decoder) |> Result.return) {
    | Json.Decode.DecodeError(msg) => Error(msg)
    }
  };
let getAzureProject = config => config.azureProject;
