open Belt.Result;

let (>>=) = flatMap;

let (>>|) = map;

let return = x => Ok(x);

let fail = x => Error(x);
