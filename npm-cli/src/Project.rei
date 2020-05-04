type t;
let ofPath: Path.t => ResultPromise.t(t, string);
let pesyConfig: t => result(PesyConfig.t, string);
let lockFileHash: t => string;
