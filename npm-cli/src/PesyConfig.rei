type t;

/* Json decoder */
let decoder: Json.Decode.decoder(t);

/* Extracts the Azure Project name from the config */
let getAzureProject: t => string;

/* Extracts ignoreDirs from the config */
let getIgnoreDirs: t => option(list(string));

/* Creates pesy config from a give manifest file */
let make: string => result(t, string);

/* Same as make, but accepts a json instead of string */
let ofJson: Js.Json.t => result(t, string);

/** Build Config */
module Build: {
  /** Abstract type */
  type t;

  /** Parses the build config from a strig */
  let ofString: string => result(t, string);

  /** Same as ofString - parses build config, but from a Json.t */
  let ofJson: Js.Json.t => result(t, string);
};
