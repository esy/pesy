open Bindings;
open ResultPromise;

let runCommand = (cmd, args, projectPath, message) => {
  Js.log("");
  Js.log(message);
  Cmd.spawn(~args, ~cwd=projectPath, ~cmd);
};

let bootstrap = Template.Kind.gen;

let runningEsy = (~esy, ~projectPath, ()) => {
  runCommand(
    esy,
    [|"i"|],
    projectPath,
    ("Running" |> Chalk.dim) ++ (" esy install" |> Chalk.whiteBright),
  )
  >>= (
    () => {
      Process.Stdout.write(
        Process.Stdout.v,
        "\n"
        ++ ("Checking" |> Chalk.dim)
        ++ " if project has all dependencies installed already => ",
      );
      Esy.status(projectPath, esy)
      >>= (
        status =>
          if (!EsyStatus.isProjectReadyForDev(status)) {
            Js.log("no" |> Chalk.red);
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
          } else {
            Js.log("yes!" |> Chalk.green);
            ResultPromise.ok();
          }
      );
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
      Js.log("");
      "Ready for development!" |> Chalk.green |> Js.log;
    }
  );
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

  bootstrapOnly
    ? bootstrapped
      >>| (
        () => {
          Js.log("");
          "You may have to run " ++ Chalk.green("esy") |> Js.log;
        }
      )
    : bootstrapped >>= runningEsy(~esy, ~projectPath);
};

let run = (esy, projectPath, template, bootstrapOnly) => {
  Fs.mkdir(~p=true, projectPath)
  |> Js.Promise.then_(() => {
       Process.chdir(projectPath);
       run(esy, projectPath, template, bootstrapOnly);
     });
};
