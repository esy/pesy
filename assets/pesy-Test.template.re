module TestFramework =
  Rely.Make({
    let config =
      Rely.TestFrameworkConfig.initialize({
        snapshotDir: "test/_snapshots",
        projectDir: "",
      });
  });

open TestFramework;

describe("my first test suite", ({test, _}) =>
  test("1 + 1 should equal 2", ({expect}) =>
    expect.int(1 + 1).toBe(2)
  )
);

cli();
