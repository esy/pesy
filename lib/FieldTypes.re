open Printf;
type t =
  | Bool(bool)
  | String(string)
  | List(list(t));
exception ConversionException(string);
let toBool =
  fun
  | Bool(b) => b
  | String(s) =>
    raise(
      ConversionException(sprintf("Expected string. Actual string (%s)", s)),
    )
  | List(l) => raise(ConversionException("Expected string. Actual list"));
let toString =
  fun
  | String(s) => s
  | Bool(b) =>
    raise(
      ConversionException(
        sprintf("Expected string. Actual bool (%s)", string_of_bool(b)),
      ),
    )
  | List(_) => raise(ConversionException("Expected string. Actual list"));
let toList =
  fun
  | List(l) => l
  | Bool(b) =>
    raise(
      ConversionException(
        sprintf("Expected list. Actual bool (%s)", string_of_bool(b)),
      ),
    )
  | String(_) => raise(ConversionException("Expected list. Actual string"));
