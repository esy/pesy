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
let copy = (templatePath, projectPath, ignoreDirs) => {
  let ignoreDirsTbl = Hashtbl.create(ignoreDirs |> List.length);
  List.iter(
    ignoreDir => Hashtbl.add(ignoreDirsTbl, ignoreDir, true),
    ignoreDirs,
  );
  Dir.scan(
    ignoreDirsTbl,
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
    | GitRemote(remote) =>
      /* TODO: sanitise remote templates */ downloadGit(remote, dest)
    | Path(source) => {
        /* Sanitisation. Checking if the source is a valid pesy
           project*/
        /* We may not reliably be able to run 'esy status' in a
           template project. Otherwise, we could have have simply obtained
           rootPackageConfigPath from it */
        /* Checking if root as a json file. Pesy supports multiple esy sandboxes
           Therefore, any one of them may contain a pesy config, for the whole
           template to be considered a valid pesy template */
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
             if (Array.length(jsonFiles) == 0) {
               ResultPromise.fail(
                 {j|No esy manifest found in the template ($source) |j},
               );
             } else {
               Js.Array.map(
                 jsonFile => {
                   Fs.readFile(. jsonFile)
                   |> Js.Promise.then_(jsonBytes => {
                        switch (
                          jsonBytes |> Bindings.Buffer.toString |> Json.parse
                        ) {
                        | None =>
                          ResultPromise.fail(
                            {j| JSON file $jsonFile failed to parse |j},
                          )
                        | Some(json) =>
                          let buildDirsExists =
                            switch (PesyConfig.Build.ofJson(json)) {
                            | Ok(_) => true
                            | Error(_) => false
                            };
                          let (pesyConfigExists, explicitIgnoreDirs) =
                            switch (PesyConfig.ofJson(json)) {
                            | Ok(pesyConfig) =>
                              switch (PesyConfig.getIgnoreDirs(pesyConfig)) {
                              | Some(explicitIgnoreDirs) => (
                                  true,
                                  explicitIgnoreDirs,
                                )
                              | None => (true, [])
                              }
                            | Error(_msg) => (false, [])
                            };
                          if (buildDirsExists || pesyConfigExists) {
                            ResultPromise.ok((jsonFile, explicitIgnoreDirs));
                          } else {
                            ResultPromise.fail(
                              {j| Neither 'pesy' nor 'buildDirs' was found in $jsonFile |j},
                            );
                          };
                        }
                      })
                 },
                 jsonFiles,
               )
               |> Js.Promise.all
               |> Js.Promise.then_(esyManifestFiles => {
                    let (pesyManifests, errors, explicitIgnoreDirs) =
                      Js.Array.reduce(
                        ((pesyManifests, errors, accExplicitIgnoreDirs)) =>
                          fun
                          | Ok((jsonFile, explicitIgnoreDirs)) => (
                              [jsonFile, ...pesyManifests],
                              errors,
                              explicitIgnoreDirs @ accExplicitIgnoreDirs,
                            )
                          | Error(msg) => (
                              pesyManifests,
                              [msg, ...errors],
                              accExplicitIgnoreDirs,
                            ),
                        ([], [], []),
                        esyManifestFiles,
                      );
                    if (List.length(pesyManifests) == 0) {
                      let errors =
                        Js.Array.joinWith("\n", Array.of_list(errors));
                      ResultPromise.fail(
                        {j|No esy manifest files found with pesy config (buildDirs or pesy).
Errors:
$errors
|j},
                      );
                    } else {
                      copy(
                        source,
                        dest,
                        explicitIgnoreDirs @ ["node_modules", ".git", "_esy"],
                      );
                    };
                  });
             }
           );
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
