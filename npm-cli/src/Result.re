open Belt.Result;

let (>>=) = flatMap;

let (>>|) = map;

let return = x => Ok(x);

let fail = x => Error(x);

let all = (~f, rps) =>
  List.fold_left(
    (acc, r) =>
      switch (r, acc) {
      | (Ok(v), Ok(acc)) => Ok([v, ...acc])
      | (Error(msg), Ok(_)) => Error(msg)
      | (Ok(_), Error(msg)) => Error(msg)
      | (Error(msg1), Error(msg2)) => Error(f(msg1, msg2))
      },
    Ok([]),
    rps,
  );

let all2 = (~f: ('a, 'a) => 'a, rps) =>
  switch (rps) {
  | (Ok(v1), Ok(v2)) => Ok((v1, v2))
  | (Error(msg), Ok(_)) => Error(msg)
  | (Ok(_), Error(msg)) => Error(msg)
  | (Error(msg1), Error(msg2)) => Error(f(msg1, msg2))
  };
