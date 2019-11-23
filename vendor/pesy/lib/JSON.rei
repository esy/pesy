type t;
exception NullJSONValue(unit);
exception InvalidJSONValue(string);
let ofString: string => t;
let fromFile: string => t;
let member: (t, string) => t;
let toKeyValuePairs: t => list((string, t));
let toValue: t => FieldTypes.t;
let debug: t => string;
