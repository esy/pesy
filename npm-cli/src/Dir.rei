let isEmpty: Path.t => bool;

let forEachEnt:
  (string => ResultPromise.t(unit, string), Path.t) =>
  ResultPromise.t(unit, string);

let scan:
  (Path.t => ResultPromise.t(unit, string), Path.t) =>
  ResultPromise.t(unit, string);
