open Bindings;
open ResultPromise;

let runCommand = (cmd, args, projectPath, message) => {
  Js.log("");
  Js.log(message);
  Cmd.spawn(~args, ~cwd=projectPath, ~cmd);
};

/** TODO: Bootstrapper should ideally work in a temporary workspace, make sure
the project of generating files and substituting values in them works fine and
only them present the project to the user for further esy command.

Bonus: We don't overwrite if files of the same name exist and only append pesy
config to package.json/esy.json. Why? So that users can easily upgrade to a pesy
config. (Opposite of pesy eject) */

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
            Js.log("No" |> Chalk.red);

            Js.Promise.make((~resolve, ~reject as _) => {
              Utils.askYesNoQuestion(
                ~question="Warm up local cache from CI?",
                ~onYes=
                  () => {
                    Js.log(
                      ("Running" |> Chalk.dim)
                      ++ (" pesy warm" |> Chalk.whiteBright),
                    );
                    Project.ofPath(esy, projectPath)
                    >>= (project => Warmup.run(esy, project))
                    |> Js.Promise.then_(warmupResult => {
                         switch (warmupResult) {
                         | Ok () => ()
                         | Error(msg) =>
                           Js.log2("Skipping warmup because: ", msg)
                         };
                         ResultPromise.ok();
                       })
                    |> catch
                    |> Js.Promise.then_(() =>
                         resolve(. Result.return()) |> Js.Promise.resolve
                       )
                    |> ignore;
                  },
                ~onNo=() => resolve(. Result.return()),
                (),
              )
            });
          } else {
            Js.log("Yes!" |> Chalk.green);
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
        Js.log("");
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
