type t = {checksum: string};

let ofPath = path => {
  Bindings.(
    Fs.readFile(. path)
    |> Js.Promise.then_(lockfileBytes => {
         let lockFileContents = lockfileBytes |> Buffer.toString;
         (
           switch (Json.parse(lockFileContents)) {
           | Some(json) =>
             Json.Decode.(
               try(
                 {checksum: field("checksum", string, json)} |> Result.return
               ) {
               | DecodeError(msg) => Error(msg)
               }
             )
           | None =>
             Error(
               {j| Json parser failed on the lockfile:
$lockFileContents |j},
             )
           }
         )
         |> Js.Promise.resolve;
       })
  );
};

let checksum = v => v.checksum;
