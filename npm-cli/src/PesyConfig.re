type t = {
  azureProject: string,
  github: string,
  ignoreDirs: option(list(string)),
};

let decoder = json => {
  open Json.Decode;
  let azureProject = json |> field("azure-project", string);
  let github = json |> field("github", string);
  let ignoreDirs = json |> (field("ignore-dirs", list(string)) |> optional);
  {ignoreDirs, github, azureProject};
};

let ofJson = json =>
  try(json |> Json.Decode.field("pesy", decoder) |> Result.return) {
  | Json.Decode.DecodeError(msg) => Error(msg)
  };

/* Makes a config. Must fail if mandatory fields are not present */
let make = manifest =>
  switch (manifest |> Json.parse) {
  | None => Error("Json parser failed: " ++ manifest)
  | Some(json) => ofJson(json)
  };

let getAzureProject = config => config.azureProject;
let getGithub = config => config.github;
let getIgnoreDirs = config => config.ignoreDirs;

module Build: {
  type t;
  let ofString: string => result(t, string);
  let ofJson: Js.Json.t => result(t, string);
} = {
  type t = bool; // We only track if 'buildDirs' property exists in the package.json

  open Json.Decode;

  let ofJson = json =>
    switch (json |> (field("buildDirs", dict(id)) |> optional)) {
    | Some(_) => Result.return(true)
    | None =>
      let str = Json.stringify(json);
      Result.fail(
        {j|buildDirs was not found in the provided string: $str |j},
      );
    };

  let ofString = str => {
    switch (Json.parse(str)) {
    | Some(json) => ofJson(json)
    | None =>
      {j| Could not parse the provided string as json:
$str
|j} |> Result.fail
    };
  };
};
