open Jest;
open Expect;
open! Expect.Operators;

open Utils;

describe("kebab function", () => {
    test("sample 1", () =>
      "Esy Pesy" |> kebab
      |> expect |> toBe("esy- -pesy")
    );
    test("sample 2", () =>
      "Hello-World-2020!" |> kebab
      |> expect |> toBe("hello--world--2-0-2-0-!")
    );
  }
);

describe("removeScope function", () => {
    test("no forward slash", () =>
      "esy-pesy" |> removeScope
      |> expect |> toEqual("esy-pesy")
    );
    test("forward slash at the beginning", () =>
      "/esy-pesy" |> removeScope
      |> expect |> toEqual("esy-pesy")
    );
    test("forward slash in between", () =>
      "esy/pesy" |> removeScope
      |> expect |> toEqual("pesy")
    );
    test("forward slash at the end", () =>
      "esy-pesy/" |> removeScope
      |> expect |> toEqual("")
    );
    test("multiple forward slashes", () =>
      "/esy/pesy/lemon/squeezy" |> removeScope
      |> expect |> toEqual("squeezy")
    );
  }
);

describe("upperCamelCasify function", () => {
    test("should camelCasify", () =>
      "esy-pesy" |> upperCamelCasify
      |> expect |> toEqual("EsyPesy")
    );
    test("forward slash(/) at the beginning", () =>
      "/esy-pesy" |> upperCamelCasify
      |> expect |> toEqual("/esyPesy")
    );
    test("forward slash(/) in between", () =>
      "esy/pesy" |> upperCamelCasify
      |> expect |> toEqual("Esy/pesy")
    );
    test("forward slash(/) at the end", () =>
      "esy-pesy/" |> upperCamelCasify
      |> expect |> toEqual("EsyPesy/")
    );
  }
);
