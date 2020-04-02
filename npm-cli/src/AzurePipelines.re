open Bindings;
module P = Js.Promise;

module ProjectName: {
  type validated;
  type unvalidated;
  type t('a);
  let ofString: string => t(unvalidated);
  let validate: t(unvalidated) => t(validated);
  let toString: t(validated) => string;
} = {
  type validated;
  type unvalidated;
  type t('a) = string;
  let ofString = x => x;
  let validate = x => {
    let start = x.[0] == '/' ? 1 : 0;
    let len = Js.String.length(x);
    let end_ = x.[len - 1] == '/' ? len - 1 : len;
    Js.String.substring(~from=start, ~to_=end_, x);
  };
  let toString = x => x;
};

module E = {
  type t =
    | InvalidJSONType(string)
    | MissingField(string)
    | InvalidFirstArrayElement;

  let toString =
    fun
    | InvalidJSONType(value) => {j|Field $value in Azure's response was undefined|j}
    | MissingField(k) => {j| Response from Azure did not contain build $k |j}
    | InvalidFirstArrayElement => "Unexpected array value in Azure response";
};

module RESTResponse = {
  type buildIdObject = {id: int};

  let guard = (jsonParser, str) =>
    try(jsonParser(str)) {
    | e =>
      let msg = Printexc.to_string(e);
      Error({j|Failed to parse Azure response
$msg
$str
|j});
    };

  let getBuildId' = json => {
    open Json.Decode;
    let buildIdObject = json => {id: field("id", int, json)};
    let valueArray = field("value", array(buildIdObject), json);
    valueArray[0].id;
  };

  let getBuildId =
    guard(responseText =>
      switch (Json.parse(responseText)) {
      | Some(json) => Ok(getBuildId'(json))
      | None => Error("getBuildId(): responseText could not be parsed")
      }
    );

  type downloadUrlObject = {downloadUrl: string};

  let getDownloadURL' = json => {
    open Json.Decode;
    let downloadUrlObject = json => {
      downloadUrl: field("downloadUrl", string, json),
    };

    switch (json |> optional(field("resource", downloadUrlObject))) {
    | Some(resource) => Ok(resource.downloadUrl)
    | None =>
      Error(
        {j|getDownloadURL(): responseObject.resource as not of the form { downloadURL: "..." }. Instead got
$responseText |j},
      )
    };
  };

  let getDownloadURL =
    guard(responseText =>
      switch (Json.parse(responseText)) {
      | Some(json) => getDownloadURL'(json)
      | None => Error("getDownloadURL(): could not parse responseText")
      }
    );
};

let os =
  switch (Process.platform) {
  | "darwin" => Some("Darwin")
  | "linux" => Some("Linux")
  | "win32" => Some("Windows")
  | _ => None
  };

let artifactName =
  switch (os) {
  | Some(os) => Some({j|cache-$os-install-v1|j})
  | None => None
  };

let master = "branchName=refs%2Fheads%2Fmaster";

let filter = "deletedFilter=excludeDeleted&statusFilter=completed&resultFilter=succeeded";

let latest = "queryOrder=finishTimeDescending&$top=1";
let restBase = "https://dev.azure.com";

let getBuildID = projName => {
  let proj = projName |> ProjectName.toString;
  Https.getCompleteResponse(
    {j|$restBase/$proj/_apis/build/builds?$filter&$master&$latest&api-version=4.1|j},
  )
  |> P.then_(r =>
       P.resolve(
         switch (r) {
         | Ok(response) => RESTResponse.getBuildId(response)
         | Error(e) =>
           switch (e) {
           | Https.E.Failure(url) => Error({j| Could not download $url |j})
           }
         },
       )
     );
};

let getDownloadURL = (projName, latestBuildID) => {
  let proj = projName |> ProjectName.toString;
  let latestBuildID = Js.Int.toString(latestBuildID);
  switch (artifactName) {
  | Some(artifactName) =>
    Https.getCompleteResponse(
      {j|$restBase/$proj/_apis/build/builds/$latestBuildID/artifacts?artifactname=$artifactName&api-version=4.1|j},
    )
    |> P.then_(
         fun
         | Error(Https.E.Failure(url)) =>
           Error({j| Failed to download $url |j}) |> P.resolve
         | Ok(responseText) =>
           responseText |> RESTResponse.getDownloadURL |> P.resolve,
       )
  | None =>
    Error("We detected a platform for which we couldn't find cached builds")
    |> P.resolve
  };
};
