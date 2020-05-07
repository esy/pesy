type t;

/** Given a path, it tries to returns a [project] if it
   is a valid one */
let ofPath: (Esy.t, Path.t) => ResultPromise.t(t, string);

/** Given [project], it returns it's pesy config */
let pesyConfig: t => result(PesyConfig.t, string);

/** Given a [project], it returns it's esy lock file checksum */
let lockFileHash: t => string;

/** Given a [project], it returns the path it is setup */
let path: t => Path.t;

let hasBuildDirsConfig: t => bool;
