module Mode = Mode;
module PesyConf = PesyConf;
module EsyCommand = EsyCommand;

open Utils;
open NoLwt;
open Printf;
open EsyCommand;

let generateBuildFiles = projectRoot => {
  let packageJSONPath = Path.(projectRoot / "package.json");
  PesyConf.gen(projectRoot, packageJSONPath);
};

let build = projectRoot => {
  let packageJSONPath = Path.(projectRoot / "package.json");
  /* Will throw if there validation errors. We catch then at Pesy.re
   * Else, returns the build target name
   */
  PesyConf.validateDuneFiles(projectRoot, packageJSONPath);
};
