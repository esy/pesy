type t('a, 'b) = Js.Promise.t(result('a, 'b));
let (>>=): (t('a, 'b), 'a => t('c, 'b)) => t('c, 'b);
let (>>|): (t('a, 'b), 'a => 'c) => t('c, 'b);
let ok: 'a => t('a, 'b);
let fail: 'b => t('a, 'b);
