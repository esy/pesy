#########################################################################
#                                                                       #
#                                 OCaml                                 #
#                                                                       #
#   Nicolas Pouillard, Berke Durak, projet Gallium, INRIA Rocquencourt  #
#                                                                       #
#   Copyright 2007 Institut National de Recherche en Informatique et    #
#   en Automatique.  All rights reserved.  This file is distributed     #
#  under the terms of the GNU Library General Public License, with      #
#  the special exception on linking described in file ../LICENSE.       #
#                                                                       #
#########################################################################

# see 'check-if-preinstalled' target
CHECK_IF_PREINSTALLED ?= true

# this configuration file is generated from configure.make
include Makefile.config

ifeq ($(OCAML_NATIVE_TOOLS), true)
OCAMLC    ?= ocamlc.opt
OCAMLOPT  ?= ocamlopt.opt
OCAMLDEP  ?= ocamldep.opt
OCAMLLEX  ?= ocamllex.opt
else
OCAMLC    ?= ocamlc
OCAMLOPT  ?= ocamlopt
OCAMLDEP  ?= ocamldep
OCAMLLEX  ?= ocamllex
endif

CP        ?= cp
COMPFLAGS ?= -w L -w R -w Z -I src -I +unix -safe-string -bin-annot -strict-sequence
LINKFLAGS ?= -I +unix -I src

PACK_CMO= $(addprefix src/,\
  const.cmo \
  loc.cmo \
  discard_printf.cmo \
  signatures.cmi \
  my_std.cmo \
  my_unix.cmo \
  tags.cmo \
  display.cmo \
  log.cmo \
  shell.cmo \
  bool.cmo \
  glob_ast.cmo \
  glob_lexer.cmo \
  glob.cmo \
  lexers.cmo \
  param_tags.cmo \
  command.cmo \
  ocamlbuild_config.cmo \
  ocamlbuild_where.cmo \
  slurp.cmo \
  options.cmo \
  pathname.cmo \
  configuration.cmo \
  flags.cmo \
  hygiene.cmo \
  digest_cache.cmo \
  resource.cmo \
  rule.cmo \
  solver.cmo \
  report.cmo \
  tools.cmo \
  fda.cmo \
  findlib.cmo \
  ocaml_arch.cmo \
  ocaml_utils.cmo \
  ocaml_dependencies.cmo \
  ocaml_compiler.cmo \
  ocaml_tools.cmo \
  ocaml_specific.cmo \
  exit_codes.cmo \
  plugin.cmo \
  hooks.cmo \
  main.cmo \
  )

EXTRA_CMO=$(addprefix src/,\
  ocamlbuild_plugin.cmo \
  ocamlbuild_executor.cmo \
  ocamlbuild_unix_plugin.cmo \
  )

PACK_CMX=$(PACK_CMO:.cmo=.cmx)
EXTRA_CMX=$(EXTRA_CMO:.cmo=.cmx)
EXTRA_CMI=$(EXTRA_CMO:.cmo=.cmi)

INSTALL_LIB=\
  src/ocamlbuildlib.cma \
  src/ocamlbuild.cmo \
  src/ocamlbuild_pack.cmi \
  $(EXTRA_CMO:.cmo=.cmi)

INSTALL_LIB_OPT=\
  src/ocamlbuildlib.cmxa src/ocamlbuildlib$(EXT_LIB) \
  src/ocamlbuild.cmx src/ocamlbuild$(EXT_OBJ) \
  src/ocamlbuild_pack.cmx \
  $(EXTRA_CMO:.cmo=.cmx) $(EXTRA_CMO:.cmo=$(EXT_OBJ))

INSTALL_LIBDIR=$(DESTDIR)$(LIBDIR)
INSTALL_BINDIR=$(DESTDIR)$(BINDIR)
INSTALL_MANDIR=$(DESTDIR)$(MANDIR)

INSTALL_SIGNATURES=\
  src/signatures.mli \
  src/signatures.cmi \
  src/signatures.cmti

ifeq ($(OCAML_NATIVE), true)
all: byte native man
else
all: byte man
endif

byte: ocamlbuild.byte src/ocamlbuildlib.cma
                 # ocamlbuildlight.byte ocamlbuildlightlib.cma
native: ocamlbuild.native src/ocamlbuildlib.cmxa

allopt: all # compatibility alias

distclean:: clean

# The executables

ocamlbuild.byte: src/ocamlbuild_pack.cmo $(EXTRA_CMO) src/ocamlbuild.cmo
	$(OCAMLC) $(LINKFLAGS) -o $@ unix.cma $^

ocamlbuildlight.byte: src/ocamlbuild_pack.cmo src/ocamlbuildlight.cmo
	$(OCAMLC) $(LINKFLAGS) -o $@ $^

ocamlbuild.native: src/ocamlbuild_pack.cmx $(EXTRA_CMX) src/ocamlbuild.cmx
	$(OCAMLOPT) $(LINKFLAGS) -o $@ unix.cmxa $^

# The libraries

src/ocamlbuildlib.cma: src/ocamlbuild_pack.cmo $(EXTRA_CMO)
	$(OCAMLC) -a -o $@ $^

src/ocamlbuildlightlib.cma: src/ocamlbuild_pack.cmo src/ocamlbuildlight.cmo
	$(OCAMLC) -a -o $@ $^

src/ocamlbuildlib.cmxa: src/ocamlbuild_pack.cmx $(EXTRA_CMX)
	$(OCAMLOPT) -a -o $@ $^

# The packs

# Build artifacts are first placed into tmp/ to avoid a race condition
# described in https://caml.inria.fr/mantis/view.php?id=4991.

src/ocamlbuild_pack.cmo: $(PACK_CMO)
	mkdir -p tmp
	$(OCAMLC) -pack $^ -o tmp/ocamlbuild_pack.cmo
	mv tmp/ocamlbuild_pack.cmi src/ocamlbuild_pack.cmi
	mv tmp/ocamlbuild_pack.cmo src/ocamlbuild_pack.cmo

src/ocamlbuild_pack.cmi: src/ocamlbuild_pack.cmo

src/ocamlbuild_pack.cmx: $(PACK_CMX)
	mkdir -p tmp
	$(OCAMLOPT) -pack $^ -o tmp/ocamlbuild_pack.cmx
	mv tmp/ocamlbuild_pack.cmx src/ocamlbuild_pack.cmx
	mv tmp/ocamlbuild_pack$(EXT_OBJ) src/ocamlbuild_pack$(EXT_OBJ)

# The lexers

src/lexers.ml: src/lexers.mll
	$(OCAMLLEX) src/lexers.mll
clean::
	rm -f src/lexers.ml
beforedepend:: src/lexers.ml

src/glob_lexer.ml: src/glob_lexer.mll
	$(OCAMLLEX) src/glob_lexer.mll
clean::
	rm -f src/glob_lexer.ml
beforedepend:: src/glob_lexer.ml

# The config file

configure:
	$(MAKE) -f configure.make all

# proxy rule for rebuilding configuration files directly from the main Makefile
Makefile.config src/ocamlbuild_config.ml:
	$(MAKE) -f configure.make $@

clean::
	$(MAKE) -f configure.make clean

distclean::
	$(MAKE) -f configure.make distclean

beforedepend:: src/ocamlbuild_config.ml

# man page

man: man/ocamlbuild.1

man/ocamlbuild.1: man/ocamlbuild.header.1 man/ocamlbuild.options.1 man/ocamlbuild.footer.1
	cat $^ > man/ocamlbuild.1

man/ocamlbuild.options.1: man/options_man.byte
	./man/options_man.byte > man/ocamlbuild.options.1

clean::
	rm -f man/ocamlbuild.options.1

distclean::
	rm -f man/ocamlbuild.1

man/options_man.byte: src/ocamlbuild_pack.cmo
	$(OCAMLC) $^ -I src man/options_man.ml -o man/options_man.byte

clean::
	rm -f man/options_man.cm*
	rm -f man/options_man.byte
ifdef EXT_OBJ
	rm -f man/options_man$(EXT_OBJ)
endif

# Testing

test-%: testsuite/%.ml all
	cd testsuite && ocaml $(CURDIR)/$<

test: test-internal test-findlibonly test-external

clean::
	rm -rf testsuite/_test_*

# Installation

# The binaries go in BINDIR. We copy ocamlbuild.byte and
# ocamlbuild.native (if available), and also copy the best available
# binary as BINDIR/ocamlbuild.

# The library is put in LIBDIR/ocamlbuild. We copy
# - the META file (for ocamlfind)
# - src/signatures.{mli,cmi,cmti} (user documentation)
# - the files in INSTALL_LIB and INSTALL_LIB_OPT (if available)

# We support three installation methods:
# - standard {install,uninstall} targets
# - findlib-{install,uninstall} that uses findlib for the library install
# - producing an OPAM .install file and not actually installing anything

install-bin-byte:
	mkdir -p $(INSTALL_BINDIR)
	$(CP) ocamlbuild.byte $(INSTALL_BINDIR)/ocamlbuild.byte$(EXE)
ifneq ($(OCAML_NATIVE), true)
	$(CP) ocamlbuild.byte $(INSTALL_BINDIR)/ocamlbuild$(EXE)
endif

install-bin-native:
	mkdir -p $(INSTALL_BINDIR)
	$(CP) ocamlbuild.native $(INSTALL_BINDIR)/ocamlbuild$(EXE)
	$(CP) ocamlbuild.native $(INSTALL_BINDIR)/ocamlbuild.native$(EXE)

ifeq ($(OCAML_NATIVE), true)
install-bin: install-bin-byte install-bin-native
else
install-bin: install-bin-byte
endif

install-bin-opam:
	echo 'bin: [' >> ocamlbuild.install
	echo '  "ocamlbuild.byte" {"ocamlbuild.byte"}' >> ocamlbuild.install
ifeq ($(OCAML_NATIVE), true)
	echo '  "ocamlbuild.native" {"ocamlbuild.native"}' >> ocamlbuild.install
	echo '  "ocamlbuild.native" {"ocamlbuild"}' >> ocamlbuild.install
else
	echo '  "ocamlbuild.byte" {"ocamlbuild"}' >> ocamlbuild.install
endif
	echo ']' >> ocamlbuild.install
	echo >> ocamlbuild.install

install-lib-basics:
	mkdir -p $(INSTALL_LIBDIR)/ocamlbuild
	$(CP) META $(INSTALL_SIGNATURES) $(INSTALL_LIBDIR)/ocamlbuild

install-lib-basics-opam:
	echo '  "ocamlbuild.opam" {"opam"}' >> ocamlbuild.install
	echo '  "META"' >> ocamlbuild.install
	for lib in $(INSTALL_SIGNATURES); do \
	  echo "  \"$$lib\" {\"$$(basename $$lib)\"}" >> ocamlbuild.install; \
	done

install-lib-byte:
	mkdir -p $(INSTALL_LIBDIR)/ocamlbuild
	$(CP) $(INSTALL_LIB) $(INSTALL_LIBDIR)/ocamlbuild

install-lib-byte-opam:
	for lib in $(INSTALL_LIB); do \
	  echo "  \"$$lib\" {\"$$(basename $$lib)\"}" >> ocamlbuild.install; \
	done

install-lib-native:
	mkdir -p $(INSTALL_LIBDIR)/ocamlbuild
	$(CP) $(INSTALL_LIB_OPT) $(INSTALL_LIBDIR)/ocamlbuild

install-lib-native-opam:
	for lib in $(INSTALL_LIB_OPT); do \
	  echo "  \"$$lib\" {\"$$(basename $$lib)\"}" >> ocamlbuild.install; \
	done

ifeq ($(OCAML_NATIVE), true)
install-lib: install-lib-basics install-lib-byte install-lib-native
else
install-lib: install-lib-basics install-lib-byte
endif

install-lib-findlib:
ifeq ($(OCAML_NATIVE), true)
	ocamlfind install ocamlbuild \
	  META $(INSTALL_SIGNATURES) $(INSTALL_LIB) $(INSTALL_LIB_OPT)
else
	ocamlfind install ocamlbuild \
	  META $(INSTALL_SIGNATURES) $(INSTALL_LIB)
endif

install-lib-opam:
	echo 'lib: [' >> ocamlbuild.install
	$(MAKE) install-lib-basics-opam
	$(MAKE) install-lib-byte-opam
ifeq ($(OCAML_NATIVE), true)
	$(MAKE) install-lib-native-opam
endif
	echo ']' >> ocamlbuild.install
	echo >> ocamlbuild.install

install-man:
	mkdir -p $(INSTALL_MANDIR)/man1
	cp man/ocamlbuild.1 $(INSTALL_MANDIR)/man1/ocamlbuild.1

install-man-opam:
	echo 'man: [' >> ocamlbuild.install
	echo '  "man/ocamlbuild.1" {"man1/ocamlbuild.1"}' >> ocamlbuild.install
	echo ']' >> ocamlbuild.install
	echo >> ocamlbuild.install

install-doc-opam:
	echo 'doc: [' >> ocamlbuild.install
	echo '  "LICENSE"' >> ocamlbuild.install
	echo '  "Changes"' >> ocamlbuild.install
	echo '  "Readme.md"' >> ocamlbuild.install
	echo ']' >> ocamlbuild.install

uninstall-bin:
	rm $(BINDIR)/ocamlbuild
	rm $(BINDIR)/ocamlbuild.byte
ifeq ($(OCAML_NATIVE), true)
	rm $(BINDIR)/ocamlbuild.native
endif

uninstall-lib-basics:
	rm $(LIBDIR)/ocamlbuild/META
	for lib in $(INSTALL_SIGNATURES); do \
	  rm $(LIBDIR)/ocamlbuild/`basename $$lib`;\
	done

uninstall-lib-byte:
	for lib in $(INSTALL_LIB); do\
	  rm $(LIBDIR)/ocamlbuild/`basename $$lib`;\
	done

uninstall-lib-native:
	for lib in $(INSTALL_LIB_OPT); do\
	  rm $(LIBDIR)/ocamlbuild/`basename $$lib`;\
	done

uninstall-lib:
	$(MAKE) uninstall-lib-basics uninstall-lib-byte
ifeq ($(OCAML_NATIVE), true)
	$(MAKE) uninstall-lib-native
endif
	ls $(LIBDIR)/ocamlbuild # for easier debugging if rmdir fails
	rmdir $(LIBDIR)/ocamlbuild

uninstall-lib-findlib:
	ocamlfind remove ocamlbuild

uninstall-man:
	rm $(INSTALL_MANDIR)/man1/ocamlbuild.1

install: check-if-preinstalled
	$(MAKE) install-bin install-lib install-man
uninstall: uninstall-bin uninstall-lib uninstall-man

findlib-install: check-if-preinstalled
	$(MAKE) install-bin install-lib-findlib
findlib-uninstall: uninstall-bin uninstall-lib-findlib

opam-install: check-if-preinstalled
	$(MAKE) ocamlbuild.install

ocamlbuild.install:
	rm -f ocamlbuild.install
	touch ocamlbuild.install
	$(MAKE) install-bin-opam
	$(MAKE) install-lib-opam
	$(MAKE) install-man-opam
	$(MAKE) install-doc-opam

check-if-preinstalled:
ifeq ($(CHECK_IF_PREINSTALLED), true)
	if test -d $(shell ocamlc -where)/ocamlbuild; then\
	  >&2 echo "ERROR: Preinstalled ocamlbuild detected at"\
	       "$(shell ocamlc -where)/ocamlbuild";\
	  >&2 echo "Installation aborted; if you want to bypass this"\
	        "safety check, pass CHECK_IF_PREINSTALLED=false to make";\
	  exit 2;\
	fi
endif

# The generic rules

%.cmo: %.ml
	$(OCAMLC) $(COMPFLAGS) -c $<

%.cmi: %.mli
	$(OCAMLC) $(COMPFLAGS) -c $<

%.cmx: %.ml
	$(OCAMLOPT) -for-pack Ocamlbuild_pack $(COMPFLAGS) -c $<

clean::
	rm -rf tmp/
	rm -f src/*.cm* *.cm*
ifdef EXT_OBJ
	rm -f src/*$(EXT_OBJ) *$(EXT_OBJ)
endif
ifdef EXT_LIB
	rm -f src/*$(EXT_LIB) *$(EXT_LIB)
endif
	rm -f test/test2/vivi.ml

distclean::
	rm -f ocamlbuild.byte ocamlbuild.native
	rm -f ocamlbuild.install

# The dependencies

depend: beforedepend
	$(OCAMLDEP) -I src src/*.mli src/*.ml > .depend

$(EXTRA_CMI): src/ocamlbuild_pack.cmi
$(EXTRA_CMO): src/ocamlbuild_pack.cmo src/ocamlbuild_pack.cmi
$(EXTRA_CMX): src/ocamlbuild_pack.cmx src/ocamlbuild_pack.cmi

include .depend

.PHONY: all allopt beforedepend clean configure
.PHONY: install installopt installopt_really depend

