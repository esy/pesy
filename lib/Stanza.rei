type t;
let create: (string, t) => t;
let createAtom: string => t;
let createExpression: list(t) => t;
let toSexp: t => Sexp.t;
let ofSexp: Sexp.t => t;
let ofString: string => t;
