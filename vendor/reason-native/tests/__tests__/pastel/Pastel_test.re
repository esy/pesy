/**
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */;
open TestFramework;

describe("Pastel", ({describe, test}) => {
  test(
    "ANSI escape sequences should be applied inside pastels and persisted outside of them",
    ({expect}) =>
    Pastel.useMode(
      Terminal,
      () => {
        let testCase =
          <Pastel dim=true>
            "oo"
            <Pastel color=Green> "hello \027[4mworld!" </Pastel>
            "unpasteled o_O"
          </Pastel>
          ++ " reset to prevent underlines on the rest of stdout \027[0m";
        expect.string(testCase).toMatchSnapshot();
      },
    )
  );

  let testMode = (mode, name) => {
    module PastelWithSpecifiedMode =
      Pastel.Make({});
    PastelWithSpecifiedMode.setMode(mode);
    PastelWithSpecifiedMode.(
      describe(
        name,
        ({describe}) => {
          describe("Nested styles", ({test}) =>
            test("Single nesting", ({expect}) => {
              let output =
                <PastelWithSpecifiedMode dim=true>
                  "oo"
                  <PastelWithSpecifiedMode color=Green>
                    "hello world!"
                  </PastelWithSpecifiedMode>
                  "unpasteled o_O"
                </PastelWithSpecifiedMode>;
              expect.string(output).toMatchSnapshot();
            })
          );
          describe("Pastel.length", ({test}) => {
            test("Length with no colors", ({expect}) => {
              expect.int(length("")).toBe(0);
              expect.int(length("123")).toBe(3);
              expect.int(length("123 567 901")).toBe(11);
            });
            test("Length with colors", ({expect}) => {
              expect.int("" |> red |> length).toBe(0);
              expect.int("123" |> bg.red |> white |> length).toBe(3);
              expect.int("123 567 901" |> underline |> bg.white |> length).
                toBe(
                11,
              );
            });
          });
          describe("Pastel.length fuzz testing", ({test}) => {
            let allPastels = [|
              PastelWithSpecifiedMode.modifier.bold,
              PastelWithSpecifiedMode.modifier.dim,
              PastelWithSpecifiedMode.modifier.italic,
              PastelWithSpecifiedMode.modifier.underline,
              PastelWithSpecifiedMode.modifier.inverse,
              PastelWithSpecifiedMode.modifier.hidden,
              PastelWithSpecifiedMode.modifier.strikethrough,
              PastelWithSpecifiedMode.color.black,
              PastelWithSpecifiedMode.color.red,
              PastelWithSpecifiedMode.color.green,
              PastelWithSpecifiedMode.color.yellow,
              PastelWithSpecifiedMode.color.blue,
              PastelWithSpecifiedMode.color.magenta,
              PastelWithSpecifiedMode.color.cyan,
              PastelWithSpecifiedMode.color.white,
              PastelWithSpecifiedMode.color.blackBright,
              PastelWithSpecifiedMode.color.redBright,
              PastelWithSpecifiedMode.color.greenBright,
              PastelWithSpecifiedMode.color.yellowBright,
              PastelWithSpecifiedMode.color.blueBright,
              PastelWithSpecifiedMode.color.magentaBright,
              PastelWithSpecifiedMode.color.cyanBright,
              PastelWithSpecifiedMode.color.whiteBright,
              PastelWithSpecifiedMode.bg.black,
              PastelWithSpecifiedMode.bg.red,
              PastelWithSpecifiedMode.bg.green,
              PastelWithSpecifiedMode.bg.yellow,
              PastelWithSpecifiedMode.bg.blue,
              PastelWithSpecifiedMode.bg.magenta,
              PastelWithSpecifiedMode.bg.cyan,
              PastelWithSpecifiedMode.bg.white,
              PastelWithSpecifiedMode.bg.blackBright,
              PastelWithSpecifiedMode.bg.redBright,
              PastelWithSpecifiedMode.bg.greenBright,
              PastelWithSpecifiedMode.bg.yellowBright,
              PastelWithSpecifiedMode.bg.blueBright,
              PastelWithSpecifiedMode.bg.magentaBright,
              PastelWithSpecifiedMode.bg.cyanBright,
              PastelWithSpecifiedMode.bg.whiteBright,
            |];
            let n = Array.length(allPastels);
            let getRandomPastel = () => allPastels[Random.int(n)];
            /* Get a random word, apply some pastels, then make sure length works */
            for (i in 1 to 1000) {
              test(
                "length fuzz testing: " ++ string_of_int(i),
                ({expect}) => {
                  let parts =
                    Array.make(Random.int(30), "")
                    |> Array.map(_ => Lorem.getRandomWord())
                    |> Array.to_list;
                  let noColorWord = parts |> String.concat("");
                  let colorWord =
                    parts
                    |> List.map(part => {
                         let part =
                           Array.make(Random.int(5), "")
                           |> Array.to_list
                           |> List.map(_ => getRandomPastel())
                           |> List.fold_left(
                                (part, pastel) => pastel(part),
                                part,
                              );
                         part;
                       })
                    |> String.concat("");
                  let expectedLength = noColorWord |> String.length;
                  let actualNoColor =
                    noColorWord |> PastelWithSpecifiedMode.length;
                  let actualColor =
                    colorWord |> PastelWithSpecifiedMode.length;
                  expect.int(actualNoColor).toBe(expectedLength);
                  expect.int(actualColor).toBe(expectedLength);
                },
              );
            };
          });
        },
      )
    );
  };

  testMode(Terminal, "Pastel terminal mode");
  testMode(HumanReadable, "Pastel Human Readable mode");
  testMode(Disabled, "Pastel disabled mode");
});
