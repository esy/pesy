#
# Pure OCaml, no packages, no _tags, code in several directories
#

# bin-annot is required for Merlin and other IDE-like tools
# The -I flag introduces sub-directories to search for code

.PHONY: all clean byte native profile debug test

OCB_FLAGS = -tag bin_annot -I src -I lib
OCB = ocamlbuild $(OCB_FLAGS)

all: native byte # profile debug

clean:
	$(OCB) -clean

native:
	$(OCB) main.native

byte:
	$(OCB) main.byte

profile:
	$(OCB) -tag profile main.native

debug:
	$(OCB) -tag debug main.byte

test: native
	./main.native "OCaml" "OCamlBuild" "users"
