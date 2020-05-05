type t;

/* Json decoder */
let decoder: Json.Decode.decoder(t);

/* Extracts the Azure Project name from the config */
let getAzureProject: t => string;

/* Creates pesy config from a give manifest file */
let make: string => result(t, string);

/** Build Config */
module Build: {
  /** Abstract type */
  type t;

  /** Parses the build config from a strig */
  let ofString: string => result(t, string);
};
