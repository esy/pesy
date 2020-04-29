open Bindings;
open ResultPromise;

let runCommand = (cmd, args, projectPath, message) => {
  Js.log("");
  Js.log(message);
  Cmd.spawn(~args, ~cwd=projectPath, ~cmd);
};

let bootstrap = projectPath =>
  fun
  | Some(template) => downloadGit(template, projectPath)
  | None => {
      let templatesDir =
        Path.resolve([|
          dirname,
          "templates",
          "pesy-reason-template-0.1.0-alpha.6",
        |]);
      Template.copy(templatesDir, projectPath);
    };

let run = (esy, projectPath, template, bootstrapOnly) => {
  let bootstrapped =
    bootstrap(projectPath, template)
    >>= (
      () => {
        // TODO: Hacky! Streamline what stays and what doesn't. Maybe add a ignore field to pesy config so that we know what to copy and what shouldn't be?
        Rimraf.run(
          Path.join([|projectPath, ".ci-self"|]),
        );
      }
    )
    >>= (
      _ => {
        Js.log("");
        Js.log("Setting up");
        Template.substitute(projectPath);
      }
    );
  let runningEsy = () => {
    runCommand(
      esy,
      [|"i"|],
      projectPath,
      ("Running" |> Chalk.dim) ++ (" esy install" |> Chalk.whiteBright),
    )
    >>= (
      () => {
        Js.log(
          ("Running" |> Chalk.dim) ++ (" pesy warm" |> Chalk.whiteBright),
        );
        Warmup.run(projectPath)
        |> Js.Promise.then_(warmupResult => {
             switch (warmupResult) {
             | Ok () => ()
             | Error(msg) => Js.log2("Skipping warmup because: ", msg)
             };
             ResultPromise.ok();
           });
      }
    )
    >>= (
      () /* (_stdout, _stderr) */ => {
        runCommand(
          esy,
          [|"pesy"|],
          projectPath,
          ("Running" |> Chalk.dim)
          ++ (" esy pesy" |> Chalk.whiteBright)
          ++ (" and " |> Chalk.dim)
          ++ ("building project dependencies" |> Chalk.whiteBright),
        );
      }
    )
    >>= (
      () /* (_stdout, _stderr) */ => {
        runCommand(
          esy,
          [|"b"|],
          projectPath,
          ("Running" |> Chalk.dim) ++ (" esy build" |> Chalk.whiteBright),
        );
      }
    )
    >>| (
      () /* (_stdout, _stderr) */ => {
        "Ready for development!" |> Chalk.green |> Js.log;
      }
    );
  };

  bootstrapOnly
    ? bootstrapped
      >>| (
        () => {
          Js.log("");
          "You may have to run " ++ Chalk.green("esy") |> Js.log;
        }
      )
    : bootstrapped >>= runningEsy;
};
