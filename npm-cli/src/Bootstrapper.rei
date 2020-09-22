let run:
  (
    Cmd.t,
    Path.t,
    Template.Kind.t,
    bool /* bootstrapOnly */,
    bool /* bootstrapCIOnly */,
    option(bool)
  ) =>
  ResultPromise.t(unit, string);
