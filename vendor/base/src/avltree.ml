(* A few small things copied from other parts of Base because they depend on us, so we
   can't use them. *)

open! Import

module Int = struct
  type t = int

  let max (x : t) y = if x > y then x else y
end

(* Its important that Empty have no args. It's tempting to make this type a record
   (e.g. to hold the compare function), but a lot of memory is saved by Empty being an
   immediate, since all unused buckets in the hashtbl don't use any memory (besides the
   array cell) *)
type ('k, 'v) t =
  | Empty
  | Node of { mutable left   : ('k, 'v) t
            ;         key    : 'k
            ; mutable value  : 'v
            ; mutable height : int
            ; mutable right  : ('k, 'v) t
            }
  | Leaf of {         key    : 'k
            ; mutable value  : 'v
            }


let empty = Empty

let height = function
  | Empty -> 0
  | Leaf _ -> 1
  | Node { left = _; key = _; value = _; height; right = _ } -> height

let invariant compare =
  let legal_left_key key = function
    | Empty -> ()
    | Leaf { key = left_key; value = _; }
    | Node { left = _; key = left_key; value = _; height = _; right = _ } ->
      assert (compare left_key key < 0)
  in
  let legal_right_key key = function
    | Empty -> ()
    | Leaf { key = right_key; value = _; }
    | Node { left = _; key = right_key; value = _; height = _; right = _ } ->
      assert (compare right_key key > 0)
  in
  let rec inv = function
    | Empty | Leaf _ -> ()
    | Node { left; key = k; value = _; height = h; right } ->
      let (hl, hr) = (height left, height right) in
      inv left;
      inv right;
      legal_left_key k left;
      legal_right_key k right;
      assert (h = Int.max hl hr + 1);
      assert (abs (hl - hr) <= 2)
  in inv

let invariant t ~compare = invariant compare t

(* In the following comments,
   't is balanced' means that 'invariant t' does not
   raise an exception.  This implies of course that each node's height field is
   correct.
   't is balanceable' means that height of the left and right subtrees of t
   differ by at most 3. *)

(* @pre: left and right subtrees have correct heights
   @post: output has the correct height *)
let update_height = function
  | Node ({ left; key = _; value = _; height = old_height; right } as x) ->
    let new_height = (Int.max (height left) (height right)) + 1 in
    if new_height <> old_height then x.height <- new_height
  | Empty | Leaf _ -> assert false

(* @pre: left and right subtrees are balanced
   @pre: tree is balanceable
   @post: output is balanced (in particular, height is correct) *)
let balance tree =
  match tree with
  | Empty | Leaf _ -> tree
  | Node ({ left; key = _; value = _; height = _; right } as root_node) ->
    let hl = height left and hr = height right in
    (* + 2 is critically important, lowering it to 1 will break the Leaf
       assumptions in the code below, and will force us to promote leaf nodes in
       the balance routine. It's also faster, since it will balance less often.
       Note that the following code is delicate.  The update_height calls must
       occur in the correct order, since update_height assumes its children have
       the correct heights.  *)
    if hl > hr + 2 then begin
      match left with
      (* It cannot be a leaf, because even if right is empty, a leaf
         is only height 1 *)
      | Empty | Leaf _ -> assert false
      | Node ({ left = left_node_left; key = _; value = _; height = _;
                right = left_node_right; }
              as left_node) ->
        if height left_node_left >= height left_node_right then begin
          root_node.left  <- left_node_right;
          left_node.right <- tree;
          update_height tree;
          update_height left;
          left
        end else begin
          (* if right is a leaf, then left must be empty. That means
             height is 2. Even if hr is empty we still can't get here. *)
          match left_node_right with
          | Empty | Leaf _ -> assert false
          | Node ({ left = lr_left; key = _; value = _; height = _; right = lr_right; }
                  as lr_node) ->
            left_node.right <- lr_left;
            root_node.left  <- lr_right;
            lr_node  .right <- tree;
            lr_node  .left  <- left;
            update_height left;
            update_height tree;
            update_height left_node_right;
            left_node_right
        end
    end else if hr > hl + 2 then begin
      (* see above for an explanation of why right cannot be a leaf *)
      match right with
      | Empty | Leaf _ -> assert false
      | Node ({ left = right_node_left; key = _; value = _; height = _;
                right = right_node_right }
              as right_node) ->
        if height right_node_right >= height right_node_left then begin
          root_node .right <- right_node_left;
          right_node.left  <- tree;
          update_height tree;
          update_height right;
          right
        end else begin
          (* see above for an explanation of why this cannot be a leaf *)
          match right_node_left with
          | Empty | Leaf _ -> assert false
          | Node ({ left = rl_left; key = _; value = _; height = _; right = rl_right }
                  as rl_node)
            ->
            right_node.left  <- rl_right;
            root_node .right <- rl_left;
            rl_node   .left  <- tree;
            rl_node   .right <- right;
            update_height right;
            update_height tree;
            update_height right_node_left;
            right_node_left
        end
    end else begin
      update_height tree;
      tree
    end
;;

(* @pre: tree is balanceable
   @pre: abs (height (right node) - height (balance tree)) <= 3
   @post: result is balanceable *)

(* @pre: tree is balanceable
   @pre: abs (height (right node) - height (balance tree)) <= 3
   @post: result is balanceable *)
let set_left node tree =
  let tree = balance tree in
  match node with
  | Node ({ left; key = _; value = _; height = _; right = _ } as r) ->
    if phys_equal left tree then ()
    else
      r.left <- tree;
    update_height node
  | _ -> assert false

(* @pre: tree is balanceable
   @pre: abs (height (left node) - height (balance tree)) <= 3
   @post: result is balanceable *)
let set_right node tree =
  let tree = balance tree in
  match node with
  | Node ({ left = _; key = _; value = _; height = _; right } as r) ->
    if phys_equal right tree then ()
    else
      r.right <- tree;
    update_height node
  | _ -> assert false

(* @pre: t is balanced.
   @post: result is balanced, with new node inserted
   @post: !added = true iff the shape of the input tree changed.  *)
let add =
  let rec add t replace added compare k v =
    match t with
    | Empty ->
      added := true;
      Leaf { key = k; value = v }
    | Leaf ({ key = k'; value = _ } as r) ->
      let c = compare k' k in
      (* This compare is reversed on purpose, we are pretending
         that the leaf was just inserted instead of the other way
         round, that way we only allocate one node. *)
      if c = 0 then begin
        added := false;
        if replace then r.value <- v;
        t
      end else begin
        added := true;
        if c < 0 then
          Node { left = t; key = k; value = v; height = 2; right = Empty }
        else
          Node { left = Empty; key = k; value = v; height = 2; right = t }
      end
    | Node ({left; key = k'; value = _; height = _; right } as r) ->
      let c = compare k k' in
      if c = 0 then begin
        added := false;
        if replace then r.value <- v;
      end else if c < 0 then
        set_left t (add left replace added compare k v)
      else
        set_right t (add right replace added compare k v);
      t
  in
  fun t ~replace ~compare ~added ~key ~data ->
    let t = add t replace added compare key data in
    if !added then balance t else t
;;

let rec first t =
  match t with
  | Empty -> None
  | Leaf { key = k; value = v }
  | Node { left = Empty; key = k; value = v; height = _; right = _ } -> Some (k, v)
  | Node { left = l; key = _; value = _; height = _; right = _ } -> first l
;;

let rec last t =
  match t with
  | Empty -> None
  | Leaf { key = k; value = v }
  | Node { left = _; key = k; value = v; height = _; right = Empty } -> Some (k, v)
  | Node { left = _; key = _; value = _; height = _; right = r } -> last r
;;


let[@inline always] rec findi_and_call_impl t ~compare k ~call_if_found ~if_found ~if_not_found =
  (* A little manual unrolling of the recursion.
     This is really worth 5% on average *)
  match t with
  | Empty -> if_not_found k
  | Leaf { key = k'; value = v } ->
    if compare k k' = 0 then call_if_found ~if_found ~key:k' ~data:v
    else if_not_found k
  | Node { left; key = k'; value = v; height = _; right } ->
    let c = compare k k' in
    if c = 0 then call_if_found ~if_found ~key:k' ~data:v
    else if c < 0 then begin
      match left with
      | Empty -> if_not_found k
      | Leaf { key = k'; value = v }->
        if compare k k' = 0 then call_if_found ~if_found ~key:k' ~data:v
        else if_not_found k
      | Node { left; key = k'; value = v; height = _; right } ->
        let c = compare k k' in
        if c = 0 then call_if_found ~if_found ~key:k' ~data:v
        else
          findi_and_call_impl (if c < 0 then left else right) ~compare k ~call_if_found ~if_found ~if_not_found
    end else begin
      match right with
      | Empty -> if_not_found k
      | Leaf { key = k'; value = v } ->
        if compare k k' = 0 then call_if_found ~if_found ~key:k' ~data:v
        else if_not_found k
      | Node { left; key = k'; value = v; height = _; right } ->
        let c = compare k k' in
        if c = 0 then call_if_found ~if_found ~key:k' ~data:v
        else
          findi_and_call_impl (if c < 0 then left else right) ~compare k ~call_if_found ~if_found ~if_not_found
    end
;;

let find_and_call =
  let call_if_found ~if_found ~key:_ ~data = if_found data in
  fun t ~compare k ~if_found ~if_not_found ->
    findi_and_call_impl t ~compare k ~call_if_found ~if_found ~if_not_found

let findi_and_call =
  let call_if_found ~if_found ~key ~data = if_found ~key ~data in
  fun t ~compare k ~if_found ~if_not_found ->
    findi_and_call_impl t ~compare k ~call_if_found ~if_found ~if_not_found

let find =
  let if_found v = Some v in
  let if_not_found _ = None in
  fun t ~compare k ->
    find_and_call t ~compare k ~if_found ~if_not_found

let mem =
  let if_found _ = true in
  let if_not_found _ = false in
  fun t ~compare k ->
    find_and_call t ~compare k ~if_found ~if_not_found

let remove =
  let rec min_elt tree =
    match tree with
    | Empty -> Empty
    | Leaf _ -> tree
    | Node { left = Empty; key = _; value = _; height = _; right = _ } -> tree
    | Node { left; key = _; value = _; height = _; right = _ } -> min_elt left
  in
  let rec remove_min_elt tree =
    match tree with
    | Empty -> assert false
    | Leaf _ -> Empty (* This must be the root *)
    | Node { left = Empty; key = _; value = _; height = _; right } -> right
    | Node { left = Leaf _; key = k; value = v; height = _; right = Empty } ->
      Leaf { key = k; value = v }
    | Node { left = Leaf _; key = _; value = _; height = _; right = _ } as node ->
      set_left node Empty; tree
    | Node { left; key = _; value = _; height = _; right = _ } as node ->
      set_left node (remove_min_elt left); tree
  in
  let merge t1 t2 =
    match (t1, t2) with
    | (Empty, t) -> t
    | (t, Empty) -> t
    | (_, _) ->
      let tree = min_elt t2 in
      match tree with
      | Empty -> assert false
      | Leaf { key = k; value = v } ->
        let t2 = balance (remove_min_elt t2) in
        Node { left = t1; key = k; value = v;
               height = Int.max (height t1) (height t2) + 1; right = t2
             }
      | Node _ as node ->
        set_right node (remove_min_elt t2);
        set_left node t1;
        node
  in
  let rec remove t removed compare k =
    match t with
    | Empty ->
      removed := false;
      Empty
    | Leaf { key = k'; value = _ } ->
      if compare k k' = 0 then begin
        removed := true;
        Empty
      end else begin
        removed := false;
        t
      end
    | Node { left; key = k'; value = _; height = _; right } ->
      let c = compare k k' in
      if c = 0 then begin
        removed := true;
        merge left right
      end else if c < 0 then begin
        set_left t (remove left removed compare k);
        t
      end else begin
        set_right t (remove right removed compare k);
        t
      end
  in
  fun t ~removed ~compare k -> balance (remove t removed compare k)
;;

let rec fold t ~init ~f =
  match t with
  | Empty -> init
  | Leaf { key; value = data } -> f ~key ~data init
  | Node { left = Leaf { key = lkey; value = ldata };
           key; value = data; height = _;
           right = Leaf { key = rkey; value = rdata }
         } ->
    f ~key:rkey ~data:rdata (f ~key ~data (f ~key:lkey ~data:ldata init))
  | Node { left = Leaf { key = lkey; value = ldata}; key; value = data; height = _;
           right = Empty }
    -> f ~key ~data (f ~key:lkey ~data:ldata init)
  | Node { left = Empty; key; value = data; height = _;
           right = Leaf { key = rkey; value = rdata }
         } ->
    f ~key:rkey ~data:rdata (f ~key ~data init)
  | Node { left; key; value = data; height = _; right = Leaf { key = rkey; value = rdata }
         } ->
    f ~key:rkey ~data:rdata (f ~key ~data (fold left ~init ~f))
  | Node { left = Leaf { key = lkey; value = ldata}; key; value = data; height = _; right } ->
    fold right ~init:(f ~key ~data (f ~key:lkey ~data:ldata init)) ~f
  | Node { left; key; value = data; height = _; right } ->
    fold right ~init:(f ~key ~data (fold left ~init ~f)) ~f

let rec iter t ~f =
  match t with
  | Empty                            -> ()
  | Leaf { key; value = data }       -> f ~key ~data
  | Node { left; key; value = data; height = _; right } ->
    iter left ~f;
    f ~key ~data;
    iter right ~f

let rec mapi_inplace t ~f =
  match t with
  | Empty -> ()
  | Leaf ({ key ; value } as t) ->
    t.value <- f ~key ~data:value
  | Node ({ left ; key ; value ; height = _ ; right } as t) ->
    mapi_inplace ~f left;
    t.value <- f ~key ~data:value;
    mapi_inplace ~f right
