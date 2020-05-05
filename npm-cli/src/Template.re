open Utils;

module Helpers: {
  /** Contains functions that compute package name kebabs, upper camel
   cased and other forms that aid in generating Dune config for the subpackages */

  /** Substitute variable in a file name. Different from [substituteTemplateValues]
   since placeholders in file name contains underscore */
  let substituteFileNames: (Path.t, string) => string;

  /** strips -template suffix */
  let stripTemplateExtension: string => string;

  /** Substitiute variables in the provided string (file contents) */
  let substituteTemplateValues: (Path.t, string) => string;
} = {
  open Bindings;

  /** If the package name were [fooBar], it would be turned to [foo-bar] */
  /* let packageNameKebab: Path.t => string; */
  let packageNameKebab = projectPath => kebab(basename(projectPath));

  /** If the package name were [@opam/foo], it turns [foo] */
  /* let packageNameKebabSansScope: Path.t => string; */
  let packageNameKebabSansScope = projectPath =>
    projectPath |> packageNameKebab |> removeScope;

  /** foo-bar turns FooBar */
  /* let packageNameUpperCamelCase: Path.t => string; */
  let packageNameUpperCamelCase = projectPath =>
    projectPath |> packageNameKebabSansScope |> upperCamelCasify;

  /** Set to a 0.0.0 */
  /* let templateVersion: string; */
  let templateVersion = "0.0.0";

  /** Produces value for (library name <packageLibName> ...) */
  /* let packageLibName: Path.t => string; */
  let packageLibName = projectPath =>
    packageNameKebabSansScope(projectPath) ++ "/library";

  /** Produces value for (library name <packageLibName> ...) */
  /* let packageTestName: Path.t => string; */
  let packageTestName = projectPath =>
    packageNameKebabSansScope(projectPath) ++ "/test";

  let substituteTemplateValues = (projectPath, s) =>
    s
    |> Js.String.replaceByRe(
         [%bs.re "/<PACKAGE_NAME_FULL>/g"],
         packageNameKebab(projectPath),
       )
    |> Js.String.replaceByRe([%bs.re "/<VERSION>/g"], templateVersion)
    |> Js.String.replaceByRe(
         [%bs.re "/<PUBLIC_LIB_NAME>/g"],
         packageLibName(projectPath),
       )
    |> Js.String.replaceByRe(
         [%bs.re "/<TEST_LIB_NAME>/g"],
         packageTestName(projectPath),
       )
    |> Js.String.replaceByRe(
         [%bs.re "/<PACKAGE_NAME>/g"],
         packageNameKebab(projectPath),
       )
    |> Js.String.replaceByRe(
         [%bs.re "/<PACKAGE_NAME_UPPER_CAMEL>/g"],
         packageNameUpperCamelCase(projectPath),
       );
  let stripTemplateExtension = s => {
    s |> Js.String.replace("-template", "");
  };

  let substituteFileNames = (projectPath, s) => {
    s
    |> Js.String.replaceByRe(
         [%bs.re "/__PACKAGE_NAME_FULL__/g"],
         packageNameKebab(projectPath),
       )
    |> Js.String.replaceByRe(
         [%bs.re "/__PACKAGE_NAME__/g"],
         packageNameKebab(projectPath),
       )
    |> Js.String.replaceByRe(
         [%bs.re "/__PACKAGE_NAME_UPPER_CAMEL__/g"],
         packageNameUpperCamelCase(projectPath),
       );
  };
};

open Bindings;
let copy = (templatePath, projectPath) => {
  Dir.scan(
    src => {
      let dest = Js.String.replace(templatePath, projectPath, src);
      Fs.isDirectory(src)
      |> Js.Promise.then_(isDir =>
           if (isDir) {
             Fs.mkdir(~dryRun=false, ~p=true, dest);
           } else {
             Fs.copy(~src, ~dest, ~dryRun=false, ());
           }
         )
      |> Js.Promise.then_(_ => ResultPromise.ok());
    },
    templatePath,
  );
};

open Helpers;
let substitute = projectPath => {
  walk_sync(projectPath)
  |> Array.map(file_or_dir => Path.join([|projectPath, file_or_dir|]))
  |> Js.Array.filter(file_or_dir => {statSync(file_or_dir)->isFile})
  |> Array.map(file => {
       let substitutedName =
         file |> substituteFileNames(projectPath) |> stripTemplateExtension;
       Js.Promise.(
         Fs.(
           readFile(. file)
           |> then_(b =>
                Buffer.toString(b)
                |> substituteTemplateValues(projectPath)
                |> Buffer.from
                |> resolve
              )
           |> then_(s => writeFile(. substitutedName, s))
           |> then_(_ => {
                if (file != substitutedName) {
                  Node.Fs.unlinkSync(file);
                };
                substitutedName
                |> Js.String.replace(
                     projectPath ++ (Process.platform == "win32" ? "\\" : "/"),
                     "",
                   )
                |> resolve;
              })
         )
       );
     })
  |> Js.Promise.all
  |> Js.Promise.then_(files => {
       let shown = ref([]);
       files
       |> Array.to_list
       |> List.iter(fileName =>
            if (Js.String.search([%re "/\\.lock/"], fileName) == (-1)
                && !List.exists(x => x == fileName, shown^)) {
              ("    created " |> Chalk.green)
              ++ (fileName |> Chalk.whiteBright)
              |> Js.log;
              shown := [fileName, ...shown^];
            } else {
              ();
            }
          );
       ResultPromise.ok();
     });
};

module Kind = {
  type t =
    | Path(string)
    | GitRemote(string);
  let path = x => Path(x);
  let gitRemote = x => GitRemote(x);
  let gen = dest =>
    fun
    | GitRemote(remote) => downloadGit(remote, dest)
    | Path(source) => {
        /* Sanitisation. Checking if the source is a valid pesy
           project*/
        let sanityCheck =
          if (source != DefaultTemplate.path) {
            /* We may not reliably be able to run 'esy status' in a
               template project. Otherwise, we could have have simply obtained
               rootPackageConfigPath from it */
            // Checking if root as a json file
            Fs.readdir(source)
            |> Js.Promise.then_(directoryEntries =>
                 Js.Array.reduce(
                   (acc, directoryEntry) =>
                     if (Js.Re.test_([%re "/\\.json$/"], directoryEntry)) {
                       Js.Array.concat(
                         acc,
                         [|Path.join([|source, directoryEntry|])|],
                       );
                     } else {
                       acc;
                     },
                   [||],
                   directoryEntries,
                 )
                 |> Js.Promise.resolve
               )
            |> Js.Promise.then_(jsonFiles =>
                 Array.map(jsonFile => Fs.readFile(. jsonFile), jsonFiles)
                 |> Js.Promise.all
                 |> Js.Promise.then_(jsons =>
                      jsons
                      |> Js.Array.map(Bindings.Buffer.toString)
                      |> Js.Array.filter(jsonStr => {
                           switch (jsonStr |> Json.parse) {
                           | None => false
                           | Some(json) =>
                             let buildDirsExists =
                               switch (
                                 Json.Decode.(
                                   (
                                     field("buildDirs", id |> dict) |> optional
                                   )(
                                     json,
                                   )
                                 )
                               ) {
                               | Some(_) => true
                               | None => false
                               };
                             let pesyConfigExists =
                               switch (
                                 Json.Decode.(
                                   (field("pesy", id |> dict) |> optional)(
                                     json,
                                   )
                                 )
                               ) {
                               | Some(_) => true
                               | None => false
                               };
                             buildDirsExists || pesyConfigExists;
                           }
                         })
                      |> Js.Promise.resolve
                    )
                 |> Js.Promise.then_(manifestsWithPesy =>
                      if (Array.length(manifestsWithPesy) == 0) {
                        ResultPromise.fail(
                          "No esy manifest files found with pesy config (buildDirs or pesy)",
                        );
                      } else {
                        ResultPromise.ok();
                      }
                    )
               );
          } else {
            ResultPromise.ok();
          };
        ResultPromise.(sanityCheck >>= (() => copy(source, dest)));
      };
  let ofString = str =>
    /* TODO: sanitisation. For instance, if git remotes with three components (foo/bar/baz) are meaningless */
    if (Path.isAbsolute(str)) {
      Path(str) |> Result.return;
    } else {
      GitRemote(str) |> Result.return;
    };
  let cmdlinerConv = {
    let parser: Cmdliner.Arg.parser(t) =
      (str: string) =>
        switch (ofString(str)) {
        | Ok(kind) => `Ok(kind)
        | Error(msg) => `Error(msg)
        };

    let printer: Cmdliner.Arg.printer(t) =
      (ppf: Format.formatter, v: t) => (
        switch (v) {
        | GitRemote(remote) => Format.fprintf(ppf, "GitRemote(%s)", remote)
        | Path(path) => Format.fprintf(ppf, "Path(%s)", path)
        }: unit
      );

    (parser, printer);
  };
};
