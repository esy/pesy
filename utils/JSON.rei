include (module type of Yojson.Basic);
type t = Yojson.Basic.t;
exception NullJSONValue(unit);
exception InvalidJSONValue(string);
let ofString: string => t;
let fromFile: string => t;
let member: (t, string) => t;
let isNull: t => bool;
let jsonNullValue: unit => t;
let toKeyValuePairs: t => list((string, t));
let toListKVPairs: t => list(list((string, t)));
let toValue: t => FieldTypes.t;
let debug: t => string;