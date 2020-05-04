type t('a, 'b) = Js.Promise.t(result('a, 'b));
let (>>=): (t('a, 'b), 'a => t('c, 'b)) => t('c, 'b);
let (>>|): (t('a, 'b), 'a => 'c) => t('c, 'b);
let ok: 'a => t('a, 'b);
let fail: 'b => t('a, 'b);
let catch: t(unit, string) => Js.Promise.t(unit);
let all: array(t('a, 'b)) => t(array('a), 'b);
let all2: (t('a, 'b), t('c, 'b)) => t(('a, 'c), 'b);
