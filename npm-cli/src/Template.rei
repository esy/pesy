module Kind: {
  type t;

  let path: string => t;
  let gitRemote: string => t;
  let gen: (Path.t, t) => ResultPromise.t(unit, string);
  /** Parses a string into either Path or GitRemote. Otherwise fails */
  let ofString: string => result(t, string);
  let cmdlinerConv: Cmdliner.Arg.conv(t); // = (Cmdliner.Arg.parser(t), Cmdliner.Arg.printer(t));
};

/** Copies a template */
let copy: (Path.t, Path.t) => ResultPromise.t(unit, string);

/** Substitutes values in the template */
let substitute: Path.t => ResultPromise.t(unit, string);
