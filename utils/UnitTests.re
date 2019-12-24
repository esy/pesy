module TestFramework = {
  include Rely.Make({
    let config =
      Rely.TestFrameworkConfig.initialize({
        snapshotDir: "test/_snapshots",
        projectDir: "",
      });
  });
};

open TestFramework;
open Utils;

describe("Utils.filterNone", ({test, _}) => {
  test("simple case", ({expect}) =>
    expect.list(filterNone([Some(1), None, None, Some(2), None, Some(3)])).
      toEqual([
      1,
      2,
      3,
    ])
  )
});
