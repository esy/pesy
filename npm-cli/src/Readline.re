type io = {
  .
  "input": in_channel,
  "output": out_channel,
};
type rlType = {
  .
  [@bs.meth] "close": unit => unit,
  [@bs.meth] "write": string => unit,
  [@bs.meth] "on": (string, string => unit) => unit,
  [@bs.meth] "question": (string, string => unit) => unit,
};
[@bs.module "readline"] external createInterface: io => rlType = "";