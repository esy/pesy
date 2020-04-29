type t('a, 'b) = Js.Promise.t(result('a, 'b));
let (>>=) = (rp, f) => {
  rp
  |> Js.Promise.then_(
       fun
       | Ok(x) => f(x)
       | Error(msg) => Js.Promise.resolve(Error(msg)),
     );
};
let (>>|) = (rp, f) => {
  rp
  |> Js.Promise.then_(
       fun
       | Error(msg) => Js.Promise.resolve(Error(msg))
       | Ok(x) => f(x) |> Result.return |> Js.Promise.resolve,
     );
};
let ok = x => x |> Result.return |> Js.Promise.resolve; // Js.Promise.resolve << Result.return;
let fail = x => x |> Result.fail |> Js.Promise.resolve;
/**
     Js.Promise.resolve << Result.fail;
     let fail: '_weak1 => Js.Promise.t(Belt.Result.t('a, '_weak1))
     is not included in let fail: 'b => t('a, 'b)
   */;
