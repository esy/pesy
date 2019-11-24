type fileOperation;

type validationError =
  | StaleDuneFile(string)
  | StaleOpamFile((string, string));

exception ShouldNotBeNull(string);
exception FatalError(string);
exception ShouldNotBeHere(unit);
exception InvalidRootName(string);
exception GenericException(string);
exception ResolveRelativePathFailure(string);
exception InvalidBinProperty(string);
exception BuildValidationFailures(list(validationError));

let gen: (string, string) => list(fileOperation);
let log: list(fileOperation) => unit;
let validateDuneFiles: (string, string) => string;
