PREFIX_ARG := $(if $(PREFIX),--prefix $(PREFIX),)
LIBDIR_ARG := $(if $(LIBDIR),--libdir $(LIBDIR),)
INSTALL_ARGS := $(PREFIX_ARG) $(LIBDIR_ARG)
BIN := ./_boot/default/bin/main/main_dune.exe

-include Makefile.dev

default: boot.exe
	./boot.exe

release: boot.exe
	./boot.exe --release

boot.exe: bootstrap.ml
	ocaml bootstrap.ml

install:
	$(BIN) install $(INSTALL_ARGS) dune --build-dir _boot

uninstall:
	$(BIN) uninstall $(INSTALL_ARGS) dune --build-dir _boot

reinstall: uninstall reinstall

test:
	$(BIN) runtest

test-js:
	$(BIN) build @runtest-js

test-coq:
	$(BIN) build @runtest-coq

test-all:
	$(BIN) build @runtest @runtest-js @runtest-coq

check:
	$(BIN) build @check

promote:
	$(BIN) promote

accept-corrections: promote

all-supported-ocaml-versions:
	$(BIN) build @install @runtest --workspace dune-workspace.dev --root .

clean:
	rm -f ./boot.exe $(wildcard ./bootstrap.cmi ./bootstrap.cmo ./bootstrap.exe)
	$(BIN) clean || true
	rm -rf _boot

distclean: clean
	rm -f src/setup.ml

doc:
	cd doc && sphinx-build . _build

livedoc:
	cd doc && sphinx-autobuild . _build \
	  -p 8888 -q  --host $(shell hostname) -r '\.#.*'

update-jbuilds: $(BIN)
	$(BIN) build @doc/runtest --auto-promote

# If the first argument is "run"...
ifeq (dune,$(firstword $(MAKECMDGOALS)))
  # use the rest as arguments for "run"
  RUN_ARGS := $(wordlist 2,$(words $(MAKECMDGOALS)),$(MAKECMDGOALS))
  # ...and turn them into do-nothing targets
  $(eval $(RUN_ARGS):;@:)
endif

dune: $(BIN)
	$(BIN) $(RUN_ARGS)

.PHONY: default install uninstall reinstall clean test doc
.PHONY: promote accept-corrections opam-release dune check

opam-release:
	dune-release distrib --skip-build --skip-lint --skip-tests
	dune-release publish distrib --verbose
	dune-release opam pkg
	dune-release opam submit
