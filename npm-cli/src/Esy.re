open Bindings;
open ResultPromise;

type t = Cmd.t;

let make = () => Cmd.make(~cmd="esy", ~env=Process.env);
let status = (projectPath, cmd) =>
  Cmd.output(~cmd, ~cwd=projectPath, ~args=[|"status"|])
  >>= (((output, _stderr)) => EsyStatus.make(output) |> Js.Promise.resolve);
let manifestPath = (path, cmd) =>
  status(path, cmd)
  >>| (esyStatus => EsyStatus.getRootPackageConfigPath(esyStatus));

let importDependencies = (~projectPath, ~exportsPath, cmd) =>
  Cmd.output(
    ~cmd,
    ~cwd=projectPath,
    ~args=[|"import-dependencies " ++ exportsPath|],
  );
