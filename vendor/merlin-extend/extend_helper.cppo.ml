open Parsetree
open Extend_protocol

#if OCAML_VERSION < (4, 3, 0)
# define CONST_STRING Asttypes.Const_string
#else
# define CONST_STRING Parsetree.Pconst_string
#endif

(** Default implementation for [Reader_def.print_outcome] using
    [Oprint] from compiler-libs *)
let print_outcome_using_oprint ppf = function
  | Reader.Out_value          x -> !Oprint.out_value ppf x
  | Reader.Out_type           x -> !Oprint.out_type ppf x
  | Reader.Out_class_type     x -> !Oprint.out_class_type ppf x
  | Reader.Out_module_type    x -> !Oprint.out_module_type ppf x
  | Reader.Out_sig_item       x -> !Oprint.out_sig_item ppf x
  | Reader.Out_signature      x -> !Oprint.out_signature ppf x
  | Reader.Out_type_extension x -> !Oprint.out_type_extension ppf x
  | Reader.Out_phrase         x -> !Oprint.out_phrase ppf x

(** Generate an extension node that will be reported as a syntax error by
    Merlin. *)
let syntax_error msg loc : extension =
  let str = Location.mkloc "merlin.syntax-error" loc in
  let payload = PStr [{
      pstr_loc = Location.none;
      pstr_desc = Pstr_eval ({
          pexp_loc = Location.none;
          pexp_desc = Pexp_constant (CONST_STRING (msg, None));
          pexp_attributes = [];
#if OCAML_VERSION >= (4, 8, 0)
          pexp_loc_stack = []
#endif
        }, []);
    }]
  in
  (str, payload)
;;


(** Physical locations might be too precise for some features.

    For instance in:
       let x = f    in    y
                 ^1    ^2

    Merlin cannot distinguish position ^1 from ^2 in the normal AST,
    because IN doesn't appear in abstract syntax. This is a problem when
    completing, because a different environment should be selected for both
    positions.

    One can add relaxed_location attributes to make some locations closer to
    the concrete syntax.

    Here is the same line annotated with physical and relaxed locations:
       let x = f    in    y
              [ ]        [ ]  -- physical locations for f and y nodes
              [     ][     ]  -- relaxed locations for f and y nodes
*)
let relaxed_location loc : attribute =
  let str = Location.mkloc "merlin.relaxed-location" loc in
#if OCAML_VERSION >= (4, 8, 0)
  {
    attr_name = str;
    attr_payload = PStr [];
    attr_loc = str.loc
  }
#else
  (str, PStr [])
#endif
;;


(** If some code should be ignored by merlin when reporting information to
    the user, put a hide_node attribute.

    This is useful for generated/desugared code which doesn't correspond to
    anything in concrete syntax (example use-case: encoding of some
    js_of_ocaml constructs).
*)
let hide_node : attribute =
  let str = Location.mknoloc "merlin.hide" in
#if OCAML_VERSION >= (4, 8, 0)
  {
    attr_name = str;
    attr_payload = PStr [];
    attr_loc = str.loc
  }
#else
  (str, PStr [])
#endif


(** The converse: when merlin should focus on a specific node of the AST.
    The main use case is also for js_of_ocaml.

    Assuming <code> is translated to:

    let module M = struct
      let prolog = ... (* boilerplate *)

      let code = <mapping-of-code>

      let epilog = ... (* boilerplate *)
    end
    in M.boilerplate

    To make merlin focus on [M.code] and ignore the boilerplate ([M.prolog]
    and [M.epilog]), add a [focus_node] attribute to the [M.code] item.
*)
let focus_node : attribute =
  let str = Location.mknoloc "merlin.focus" in
#if OCAML_VERSION >= (4, 8, 0)
  {
    attr_name = str;
    attr_payload = PStr [];
    attr_loc = str.loc
  }
#else
  (str, PStr [])
#endif

(* Projections for merlin attributes and extensions *)

let classify_extension (id, _ : extension) : [`Other | `Syntax_error] =
  match id.Location.txt with
  | "merlin.syntax-error" -> `Syntax_error
  | _ -> `Other

let classify_attribute (attr : attribute) : [`Other | `Relaxed_location | `Hide | `Focus] =
#if OCAML_VERSION >= (4, 8, 0)
  match attr.Parsetree.attr_name.Location.txt with
#else
  match (fst attr).Location.txt with
#endif
  | "merlin.relaxed-location" -> `Relaxed_location
  | "merlin.hide" -> `Hide
  | "merlin.focus" -> `Focus
  | _ -> `Other

let extract_syntax_error (id, payload : extension) : string * Location.t =
  if id.Location.txt <> "merlin.syntax-error" then
    invalid_arg "Merlin_extend.Reader_helper.extract_syntax_error";
  let msg = match payload with
    | PStr [{
        pstr_desc = Pstr_eval ({
            pexp_desc = Pexp_constant (CONST_STRING (msg, _)); _
          }, _); _
      }] -> msg
    | _ -> "Warning: extension produced an incorrect syntax-error node"
  in
  msg, id.Location.loc

let extract_relaxed_location : attribute -> Location.t = function
#if OCAML_VERSION >= (4, 8, 0)
  | { attr_name = {Location. txt = "merlin.relaxed-location"; loc}; _ } -> loc
#else
  | ({Location. txt = "merlin.relaxed-location"; loc} , _) -> loc
#endif
  | _ -> invalid_arg "Merlin_extend.Reader_helper.extract_relaxed_location"
