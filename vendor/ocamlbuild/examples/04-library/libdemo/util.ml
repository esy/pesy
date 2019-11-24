
let rec join = function
    | []      -> ""
    | [x]     -> x
    | [x;y]   -> x ^ " and " ^ y
    | x::xs   -> x ^ ", "    ^ join xs


