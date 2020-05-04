open Belt.Result;

let (>>=) = flatMap;

let (>>|) = map;

let return = x => Ok(x);

let fail = x => Error(x);

let allArray =
  Array.fold_left(
    (
      acc,
      fun
      | Ok(v) => Ok(Array.append([|v|], acc))
      | Error(msg) => Error(msg),
    ),
    Ok([||]),
  );
