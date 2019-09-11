open! Stdune

module Virtual : sig
  type t =
    { virtual_modules : Ordered_set_lang.t
    }
end

module Implementation : sig
  type t =
    { existing_virtual_modules : Module.Name.Set.t
    ; allow_new_public_modules : bool
    }
end

type kind =
  | Virtual of Virtual.t
  | Implementation of Implementation.t
  | Exe_or_normal_lib

val eval
  :  modules:(Module.Source.t Module.Name.Map.t)
  -> buildable:Dune_file.Buildable.t
  -> private_modules:Ordered_set_lang.t
  -> kind:kind
  -> Module.Name_map.t
