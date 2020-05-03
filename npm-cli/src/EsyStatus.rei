type t;
let make: string => result(t, string);
let getRootPackageConfigPath: t => string;
let isProjectReadyForDev: t => bool;
