type t;

let ofPath: Path.t => ResultPromise.t(t, string);
let checksum: t => string;
