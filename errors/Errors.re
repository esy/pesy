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
exception ImportsParserFailure(unit);
exception LocalLibraryPathNotFound(string);
exception ForeignStubsIncorrectlyUsed;
exception CNamesIncorrectlyUsed;
exception InvalidDuneVersion(string);
