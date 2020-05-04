type t;
let ofPath: (Esy.t, Path.t) => ResultPromise.t(t, string);
let pesyConfig: t => result(PesyConfig.t, string);
let lockFileHash: t => string;
let path: t => Path.t;
