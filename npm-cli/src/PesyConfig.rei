type t;
/* Json decoder */
let decoder: Json.Decode.decoder(t);
/* Extracts the Azure Project name from the config */
let getAzureProject: t => string;
/* Creates pesy config from a give manifest file */
let make: string => result(t, string);
