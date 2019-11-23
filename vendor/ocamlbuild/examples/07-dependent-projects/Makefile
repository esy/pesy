#
# Pure OCaml, project depends on library code from a local project
#
# For more details, see:
# https://ocaml.org/learn/tutorials/ocamlbuild/Working_on_dependent_projects_with_ocamlbuild.html
#

# - The -I flag introduces sub-directories

.PHONY: all clean byte native profile debug test

OCB_FLAGS = -I src -I libdemo
OCB = ocamlbuild $(OCB_FLAGS)

all: native byte

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
	./main.native groucho harpo chico
