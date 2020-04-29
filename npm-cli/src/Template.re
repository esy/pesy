open Utils;
open Bindings;

let packageNameKebab = projectPath => kebab(basename(projectPath));
let packageNameKebabSansScope = projectPath =>
  projectPath |> packageNameKebab |> removeScope;
let packageNameUpperCamelCase = projectPath =>
  projectPath |> packageNameKebabSansScope |> upperCamelCasify;
let templateVersion = "0.0.0";
let packageLibName = projectPath =>
  packageNameKebabSansScope(projectPath) ++ "/library";
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
