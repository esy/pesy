
module Ppx_sexp_conv_lib = struct
  module Conv_error = Sexplib0.Sexp_conv_error
  module Conv       = Sexplib0.Sexp_conv

  module Sexp       = Sexplib0.Sexp
end

include Sexplib0.Sexp_conv
