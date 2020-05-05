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

module Build: {
  type t;

  let ofString: string => result(t, string);
} = {
  type t = bool; // We only track if 'buildDirs' property exists in the package.json

  open Json.Decode;

  let ofString = str => {
    switch (Json.parse(str)) {
    | Some(json) =>
      switch (field("buildDirs", dict(id) |> optional, json)) {
      | Some(_) => Result.return(true)
      | None =>
        Result.fail(
          {j|buildDirs was not found in the provided string: $str |j},
        )
      }
    | None =>
      {j| Could not parse the provided string as json:
$str
|j} |> Result.fail
    };
  };
};
