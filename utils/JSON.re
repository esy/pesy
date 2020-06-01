/* TODO: Making parsing more lenient? Eg. allow string where single element list is valid */
include Yojson.Basic;
open Printf;
open Yojson.Basic;
type t = Yojson.Basic.t;

/* private */
exception MissingJSONMember(string);

/* public */
exception InvalidJSONValue(string);
exception NullJSONValue(unit);

let ofString = jstr => from_string(jstr);
let fromFile = path => from_file(path);
let member = (j, m) =>
  try (Util.member(m, j)) {
  | _ =>
    raise(
      MissingJSONMember(Printf.sprintf("%s was missing in the json", m)),
    )
  };
let toKeyValuePairs = (json: Yojson.Basic.t) =>
  switch (json) {
  | `Assoc(jsonKeyValuePairs) => jsonKeyValuePairs
  | `Null => raise(NullJSONValue())
  | _ => raise(InvalidJSONValue("Expected key value pairs"))
  };
let rec toValue = (json: Yojson.Basic.t) =>
  switch (json) {
  | `Bool(b) => FieldTypes.Bool(b)
  | `String(s) => FieldTypes.String(s)
  | `List(jl) => FieldTypes.List(List.map(j => toValue(j), jl))
  | `Null => raise(NullJSONValue())
  | _ =>
    raise(
      InvalidJSONValue(
        sprintf(
          "Value must be either string, bool or list of string. Found %s",
          to_string(json),
        ),
      ),
    )
  };
let debug = t => to_string(t);
