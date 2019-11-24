open! Import

module Int    = Int0
module String = String0

module T = struct
  type t = Caml.Lexing.position =
    { pos_fname : string;
      pos_lnum : int;
      pos_bol : int;
      pos_cnum : int;
    }
  [@@deriving_inline compare, hash, sexp]
  let compare : t -> t -> int =
    fun a__001_ ->
    fun b__002_ ->
      if Ppx_compare_lib.phys_equal a__001_ b__002_
      then 0
      else
        (match compare_string a__001_.pos_fname b__002_.pos_fname with
         | 0 ->
           (match compare_int a__001_.pos_lnum b__002_.pos_lnum with
            | 0 ->
              (match compare_int a__001_.pos_bol b__002_.pos_bol with
               | 0 -> compare_int a__001_.pos_cnum b__002_.pos_cnum
               | n -> n)
            | n -> n)
         | n -> n)
  let (hash_fold_t :
         Ppx_hash_lib.Std.Hash.state -> t -> Ppx_hash_lib.Std.Hash.state) =
    fun hsv ->
    fun arg ->
      let hsv =
        let hsv =
          let hsv = let hsv = hsv in hash_fold_string hsv arg.pos_fname in
          hash_fold_int hsv arg.pos_lnum in
        hash_fold_int hsv arg.pos_bol in
      hash_fold_int hsv arg.pos_cnum
  let (hash : t -> Ppx_hash_lib.Std.Hash.hash_value) =
    let func arg =
      Ppx_hash_lib.Std.Hash.get_hash_value
        (let hsv = Ppx_hash_lib.Std.Hash.create () in hash_fold_t hsv arg) in
    fun x -> func x
  let t_of_sexp : Ppx_sexp_conv_lib.Sexp.t -> t =
    let _tp_loc = "src/source_code_position0.ml.T.t" in
    function
    | Ppx_sexp_conv_lib.Sexp.List field_sexps as sexp ->
      let pos_fname_field = ref None
      and pos_lnum_field = ref None
      and pos_bol_field = ref None
      and pos_cnum_field = ref None
      and duplicates = ref []
      and extra = ref [] in
      let rec iter =
        function
        | (Ppx_sexp_conv_lib.Sexp.List ((Ppx_sexp_conv_lib.Sexp.Atom
                                           field_name)::_field_sexp::[]))::tail ->
          ((match field_name with
             | "pos_fname" ->
               (match !pos_fname_field with
                | None ->
                  let fvalue = string_of_sexp _field_sexp in
                  pos_fname_field := (Some fvalue)
                | Some _ -> duplicates := (field_name :: (!duplicates)))
             | "pos_lnum" ->
               (match !pos_lnum_field with
                | None ->
                  let fvalue = int_of_sexp _field_sexp in
                  pos_lnum_field := (Some fvalue)
                | Some _ -> duplicates := (field_name :: (!duplicates)))
             | "pos_bol" ->
               (match !pos_bol_field with
                | None ->
                  let fvalue = int_of_sexp _field_sexp in
                  pos_bol_field := (Some fvalue)
                | Some _ -> duplicates := (field_name :: (!duplicates)))
             | "pos_cnum" ->
               (match !pos_cnum_field with
                | None ->
                  let fvalue = int_of_sexp _field_sexp in
                  pos_cnum_field := (Some fvalue)
                | Some _ -> duplicates := (field_name :: (!duplicates)))
             | _ ->
               if !Ppx_sexp_conv_lib.Conv.record_check_extra_fields
               then extra := (field_name :: (!extra))
               else ());
           iter tail)
        | (Ppx_sexp_conv_lib.Sexp.List ((Ppx_sexp_conv_lib.Sexp.Atom
                                           field_name)::[]))::tail ->
          ((let _ = field_name in
            if !Ppx_sexp_conv_lib.Conv.record_check_extra_fields
            then extra := (field_name :: (!extra))
            else ());
           iter tail)
        | (Ppx_sexp_conv_lib.Sexp.Atom _|Ppx_sexp_conv_lib.Sexp.List _ as
           sexp)::_
          ->
          Ppx_sexp_conv_lib.Conv_error.record_only_pairs_expected _tp_loc
            sexp
        | [] -> () in
      (iter field_sexps;
       (match !duplicates with
        | _::_ ->
          Ppx_sexp_conv_lib.Conv_error.record_duplicate_fields _tp_loc
            (!duplicates) sexp
        | [] ->
          (match !extra with
           | _::_ ->
             Ppx_sexp_conv_lib.Conv_error.record_extra_fields _tp_loc
               (!extra) sexp
           | [] ->
             (match ((!pos_fname_field), (!pos_lnum_field),
                     (!pos_bol_field), (!pos_cnum_field))
              with
              | (Some pos_fname_value, Some pos_lnum_value, Some
                                                              pos_bol_value, Some pos_cnum_value) ->
                {
                  pos_fname = pos_fname_value;
                  pos_lnum = pos_lnum_value;
                  pos_bol = pos_bol_value;
                  pos_cnum = pos_cnum_value
                }
              | _ ->
                Ppx_sexp_conv_lib.Conv_error.record_undefined_elements
                  _tp_loc sexp
                  [((Ppx_sexp_conv_lib.Conv.(=) (!pos_fname_field) None),
                    "pos_fname");
                   ((Ppx_sexp_conv_lib.Conv.(=) (!pos_lnum_field) None),
                    "pos_lnum");
                   ((Ppx_sexp_conv_lib.Conv.(=) (!pos_bol_field) None),
                    "pos_bol");
                   ((Ppx_sexp_conv_lib.Conv.(=) (!pos_cnum_field) None),
                    "pos_cnum")]))))
    | Ppx_sexp_conv_lib.Sexp.Atom _ as sexp ->
      Ppx_sexp_conv_lib.Conv_error.record_list_instead_atom _tp_loc sexp
  let sexp_of_t : t -> Ppx_sexp_conv_lib.Sexp.t =
    function
    | { pos_fname = v_pos_fname; pos_lnum = v_pos_lnum; pos_bol = v_pos_bol;
        pos_cnum = v_pos_cnum } ->
      let bnds = [] in
      let bnds =
        let arg = sexp_of_int v_pos_cnum in
        (Ppx_sexp_conv_lib.Sexp.List
           [Ppx_sexp_conv_lib.Sexp.Atom "pos_cnum"; arg])
        :: bnds in
      let bnds =
        let arg = sexp_of_int v_pos_bol in
        (Ppx_sexp_conv_lib.Sexp.List
           [Ppx_sexp_conv_lib.Sexp.Atom "pos_bol"; arg])
        :: bnds in
      let bnds =
        let arg = sexp_of_int v_pos_lnum in
        (Ppx_sexp_conv_lib.Sexp.List
           [Ppx_sexp_conv_lib.Sexp.Atom "pos_lnum"; arg])
        :: bnds in
      let bnds =
        let arg = sexp_of_string v_pos_fname in
        (Ppx_sexp_conv_lib.Sexp.List
           [Ppx_sexp_conv_lib.Sexp.Atom "pos_fname"; arg])
        :: bnds in
      Ppx_sexp_conv_lib.Sexp.List bnds
  [@@@end]
end

include T
include Comparator.Make(T)

(* This is the same function as Ppx_here.lift_position_as_string. *)
let make_location_string ~pos_fname ~pos_lnum ~pos_cnum ~pos_bol =
  String.concat
    [ pos_fname
    ; ":"; Int.to_string pos_lnum
    ; ":"; Int.to_string (pos_cnum - pos_bol)
    ]

let to_string {Caml.Lexing.pos_fname; pos_lnum; pos_cnum; pos_bol} =
  make_location_string ~pos_fname ~pos_lnum ~pos_cnum ~pos_bol

let sexp_of_t t = Sexp.Atom (to_string t)
