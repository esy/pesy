/** Contains functions that compute package name kebabs, upper camel
   cased and other forms that aid in generating Dune config for the subpackages */

/** If the package name were [fooBar], it would be turned to [foo-bar] */
let packageNameKebab: Path.t => string;

/** If the package name were [@opam/foo], it turns [foo] */
let packageNameKebabSansScope: Path.t => string;

/** foo-bar turns FooBar */
let packageNameUpperCamelCase: Path.t => string;

/** Set to a 0.0.0 */
let templateVersion: string;

/** Produces value for (library name <packageLibName> ...) */
let packageLibName: Path.t => string;

/** Produces value for (library name <packageLibName> ...) */
let packageTestName: Path.t => string;

/** Substitute variable in a file name. Different from [substituteTemplateValues]
   since placeholders in file name contains underscore */
let substituteFileNames: (Path.t, string) => string;

/** strips -template suffix */
let stripTemplateExtension: string => string;

/** Substitiute variables in the provided string (file contents) */
let substituteTemplateValues: (Path.t, string) => string;

/** Copies a template */
let copy: (Path.t, Path.t) => ResultPromise.t(unit, string);
