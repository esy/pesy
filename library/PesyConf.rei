type fileOperation;

let gen: (string, string) => list(fileOperation);
let log: list(fileOperation) => unit;

exception ShouldNotBeNull(string);
exception FatalError(string);
exception ShouldNotBeHere(unit);
exception InvalidRootName(string);
exception GenericException(string);
exception ResolveRelativePathFailure(string);
exception InvalidBinProperty(string);
