#!/bin/bash -xue

PATH=~/ocaml/bin:$PATH; export PATH
OPAMYES="true"; export OPAMYES

OPAM_VERSION="2.0.3"

ODOC="odoc.1.4.0"

TARGET="$1"; shift

case "$TARGET" in
  prepare)
    echo -en "travis_fold:start:ocaml\r"
    if [ ! -e ~/ocaml/cached-version -o "$(cat ~/ocaml/cached-version)" != "$OCAML_VERSION.$OCAML_RELEASE" ] ; then
      rm -rf ~/ocaml
      mkdir -p ~/ocaml/src
      cd ~/ocaml/src
      wget http://caml.inria.fr/pub/distrib/ocaml-$OCAML_VERSION/ocaml-$OCAML_VERSION.$OCAML_RELEASE.tar.gz
      tar -xzf ocaml-$OCAML_VERSION.$OCAML_RELEASE.tar.gz
      cd ocaml-$OCAML_VERSION.$OCAML_RELEASE
      ./configure -prefix ~/ocaml
      make world.opt
      make install
      cd ../..
      rm -rf src
      echo "$OCAML_VERSION.$OCAML_RELEASE" > ~/ocaml/cached-version
    fi
    echo -en "travis_fold:end:ocaml\r"
    if [ $WITH_OPAM -eq 1 ] ; then
      echo -en "travis_fold:start:opam.init\r"
      if [ "$TRAVIS_OS_NAME" = "osx" ] ; then
        brew update
        brew install aspcud
        PREFIX=/Users/travis
      else
        PREFIX=/home/travis
      fi
      if [ ! -e ~/ocaml/bin/opam -o ! -e ~/.opam/lock -o "$OPAM_RESET" = "1" ] ; then
        mkdir ~/ocaml/src
        cd ~/ocaml/src
        wget https://github.com/ocaml/opam/releases/download/$OPAM_VERSION/opam-full-$OPAM_VERSION.tar.gz
        tar -xzf opam-full-$OPAM_VERSION.tar.gz
        cd opam-full-$OPAM_VERSION
        ./configure --prefix=$PREFIX/ocaml
        make lib-ext
        make all
        make install
        cd ../..
        rm -rf src
        rm -rf ~/.opam
        opam init --disable-sandboxing
        eval $(opam config env)
        _boot/install/default/bin/dune runtest && \
        opam install ocamlfind utop ppxlib $ODOC menhir ocaml-migrate-parsetree js_of_ocaml-ppx js_of_ocaml-compiler
        opam remove dune jbuilder \
             `opam list --depends-on jbuilder --installed --short` \
             `opam list --depends-on dune     --installed --short`
        if opam info dune &> /dev/null; then
            opam remove dune `opam list --depends-on dune --installed --short`
        fi
      fi
      cp -a ~/.opam ~/.opam-start
      echo -en "travis_fold:end:opam.init\r"
    fi
  ;;
  build)
    UPDATE_OPAM=0
    RUNTEST_NO_DEPS=runtest-no-deps.out
    echo -en "travis_fold:start:dune.bootstrap\r"
    ocaml bootstrap.ml
    echo -en "travis_fold:end:dune.bootstrap\r"
    ./boot.exe --subst
    echo -en "travis_fold:start:dune.boot\r"
    ./boot.exe
    if [ $WITH_OPAM -eq 1 ] ; then
      echo -en "travis_fold:start:opam.deps\r"
      DATE=$(date +%Y%m%d)
      eval $(opam config env)
      for pkg in $(opam pin list --short); do
        UPDATE_OPAM=1
        opam pin remove $pkg --no-action
        opam remove $pkg || true
      done
      if [ ! -e ~/.opam/last-update ] || [ $(cat ~/.opam/last-update) != $DATE ] ; then
        opam update
        echo $DATE> ~/.opam/last-update
        UPDATE_OPAM=1
        opam upgrade
      fi
      if ! ./_boot/install/default/bin/dune build @runtest-no-deps &> $RUNTEST_NO_DEPS ; then
        cat $RUNTEST_NO_DEPS;
        exit 1;
      fi
      opam list
      opam pin add dune . --no-action
      opam install ocamlfind utop ppxlib $ODOC ocaml-migrate-parsetree js_of_ocaml-ppx js_of_ocaml-compiler coq
      echo -en "travis_fold:end:opam.deps\r"
    fi
    echo -en "travis_fold:end:dune.boot\r"
    if [ $WITH_OPAM -eq 1 ] ; then
      cat $RUNTEST_NO_DEPS;
      _boot/install/default/bin/dune runtest && \
      _boot/install/default/bin/dune build @test/blackbox-tests/runtest-js && \
      _boot/install/default/bin/dune build @test/blackbox-tests/runtest-coq && \
      ! _boot/install/default/bin/dune build @test/fail-with-background-jobs-running
      RESULT=$?
      if [ $UPDATE_OPAM -eq 0 ] ; then
        rm -rf ~/.opam
        mv ~/.opam-start ~/.opam
      fi
      exit $RESULT
    fi
  ;;
  *)
    echo "bad command $TARGET">&2; exit 1
esac
