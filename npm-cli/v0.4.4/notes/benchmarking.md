The results of benchmarking a sample "generateEverything.sh" script:


DELTAms 5
DELTAms 11  - detect shell
DELTAms 6  - setup variables
DELTAms 10  - define functions
DELTAms 11  - define build variables
DELTAms 8  - print directories
DELTAms 9  - check lib dir
DELTAms 30  - create lib build file
DELTAms 12  - verify lib build existing contents
DELTAms 12  - setup bin vars
DELTAms 8  - check bin main mod name
DELTAms 10  - check bin main module exists
DELTAms 28  - read existing dune bin contents
DELTAms 23  - create bin dune contents
DELTAms 5  - check bin against required bin
DELTAms 8  - check root dune exists
DELTAms 4  - check opam file exists
DELTAms -989  - check root dune-project file exists
DELTAms 82  - perform actual build

  Build Succeeded! To test a binary:

      esy x ChalkConsole.exe


DELTAms 5  - check build failure
DELTAms 8  - end
DELTAms 5  - end



For the following manually instrumented profiling of that script file (requires
gdate be installed). I've since improved the detect shell step (avoided using
grep subprocess).

All that really matters is the time it takes to run the genEverything script.
It doesn't matter how long it takes to generate the genEverything script.  The
biggest impacts for optimization of running genEverything would be:
1. Omitting fields in the generated dune files that aren't present in the
   package.json config.

Note that each measurement has an additional 5ms overhead apparently (so
subtract about 5ms for each measurement to get the real time).

Some smaller improvements:
- md5 takes about 7ms to execute which blocks running of genEverything.sh, but
  just checking for if md5 program exists is time consuming.
  If we have a previous build hash, we don't need to check 

Run it in a project by doing `esy b ./_build/genEverything.sh`


```sh

#!/bin/bash

set -e
set -u

BOLD=`tput bold`  || BOLD=''   # Select bold mode
BLACK=`tput setaf 0` || BLACK=''
RED=`tput setaf 1` || RED=''
GREEN=`tput setaf 2` || GREEN=''
YELLOW=`tput setaf 3` || YELLOW=''
RESET=`tput sgr0` || RESET=''

PREV_TIME="1$(/opt/homebrew/bin/gdate +%N)"




# Some operation:
NOW="1$(/opt/homebrew/bin/gdate +%N)"
DELTA_MS=$(( $(($NOW-$PREV_TIME)) / 1000000))
echo "DELTAms ${DELTA_MS}"
PREV_TIME="1$(/opt/homebrew/bin/gdate +%N)"
MODE="update"
[[ $SHELL =~ "noprofile" ]] && MODE="build"

# Some operation:
NOW="1$(/opt/homebrew/bin/gdate +%N)"
DELTA_MS=$(( $(($NOW-$PREV_TIME)) / 1000000))
echo "DELTAms ${DELTA_MS}  - detect shell"
PREV_TIME="1$(/opt/homebrew/bin/gdate +%N)"


LAST_EXE_NAME=""
NOTIFIED_USER="false"
BUILD_STALE_PROBLEM="false"

DEFAULT_MAIN_MODULE_NAME="Index"

NOW="1$(/opt/homebrew/bin/gdate +%N)"
DELTA_MS=$(( $(($NOW-$PREV_TIME)) / 1000000))
echo "DELTAms ${DELTA_MS}  - setup variables"
PREV_TIME="1$(/opt/homebrew/bin/gdate +%N)"



function notifyUser() {
  if [ "${NOTIFIED_USER}" == "false" ]; then
    echo ""
    if [ "${MODE}" == "build" ]; then
      printf "  %sAlmost there!%s %sWe just need to prepare a couple of files:%s\\n\\n" "${YELLOW}${BOLD}" "${RESET}" "${BOLD}" "${RESET}"
    else
      printf "  %sPreparing for build:%s\\n\\n" "${YELLOW}${BOLD}" "${RESET}"
    fi
    NOTIFIED_USER="true"
  else
    # do nothing
    true
  fi
}


function printDirectory() {
  if [ "${MODE}" != "build" ]; then
    DIR=$1
    NAME=$2
    NAMESPACE=$3
    REQUIRE=$4
    IS_LAST=$5
    printf "│\\n"
    PREFIX=""
    if [[ "$IS_LAST" == "last" ]]; then
      printf "└─%s/\\n" "$DIR"
      PREFIX="    "
    else
      printf "├─%s/\\n" "$DIR"
      PREFIX="│   "
    fi
    printf "%s%s\\n" "$PREFIX" "$NAME"
    printf "%s%s\\n" "$PREFIX" "$NAMESPACE"
    if [ -z "$REQUIRE" ]; then
      true
    else
      if [ "$REQUIRE" != " " ]; then
        printf   "%s%s\\n" "$PREFIX" "$REQUIRE"
      fi
    fi
  fi
}


NOW="1$(/opt/homebrew/bin/gdate +%N)"
DELTA_MS=$(( $(($NOW-$PREV_TIME)) / 1000000))
echo "DELTAms ${DELTA_MS}  - define functions"
PREV_TIME="1$(/opt/homebrew/bin/gdate +%N)"



PACKAGE_NAME="chalk-console"
PACKAGE_NAME_UPPER_CAMEL="ChalkConsole"
NAMESPACE="ChalkConsole"
PUBLIC_LIB_NAME="chalk-console.lib"
#Default Namespace
lib_NAMESPACE="ChalkConsole"
#Default Requires
lib_REQUIRE=""
#Default Flags
lib_FLAGS=""
lib_OCAMLC_FLAGS=""
lib_OCAMLOPT_FLAGS=""
lib_PREPROCESS=""
lib_C_NAMES=""
#Default Requires
bin_REQUIRE=""
#Default Flags
bin_FLAGS=""
bin_OCAMLC_FLAGS=""
bin_OCAMLOPT_FLAGS=""
bin_PREPROCESS=""
bin_C_NAMES=""
lib_NAMESPACE=""ChalkConsole""
bin_MAIN_MODULE=""TestChalkConsole""
lib_REQUIRE=" console.lib chalk.lib "
bin_REQUIRE=" console.lib chalk.lib chalk-console.lib "


NOW="1$(/opt/homebrew/bin/gdate +%N)"
DELTA_MS=$(( $(($NOW-$PREV_TIME)) / 1000000))
echo "DELTAms ${DELTA_MS}  - define build variables"
PREV_TIME="1$(/opt/homebrew/bin/gdate +%N)"

printDirectory "lib" "library name: chalk-console.lib" "namespace:    $lib_NAMESPACE" "require:     $lib_REQUIRE" not-last
printDirectory "bin" "name:    ChalkConsole.exe" "main:    ${bin_MAIN_MODULE:-$DEFAULT_MAIN_MODULE_NAME}" "require:$bin_REQUIRE" last


NOW="1$(/opt/homebrew/bin/gdate +%N)"
DELTA_MS=$(( $(($NOW-$PREV_TIME)) / 1000000))
echo "DELTAms ${DELTA_MS}  - print directories"
PREV_TIME="1$(/opt/homebrew/bin/gdate +%N)"


# Perform validation:

LIB_DIR="${cur__root}/lib"
LIB_DUNE_FILE="${LIB_DIR}/dune"

# TODO: Error if there are multiple libraries all using the default namespace.
if [ -d "${LIB_DIR}" ]; then
  true
else
  BUILD_STALE_PROBLEM="true"
  notifyUser
  if [ "${MODE}" == "build" ]; then
    printf "    □  Your project is missing the lib directory described in package.json buildDirs\\n"
  else
    printf "    %s☒%s  Your project is missing the lib directory described in package.json buildDirs\\n" "${BOLD}${GREEN}" "${RESET}"
    mkdir -p "${LIB_DIR}"
  fi
fi

NOW="1$(/opt/homebrew/bin/gdate +%N)"
DELTA_MS=$(( $(($NOW-$PREV_TIME)) / 1000000))
echo "DELTAms ${DELTA_MS}  - check lib dir"
PREV_TIME="1$(/opt/homebrew/bin/gdate +%N)"

LIB_DUNE_CONTENTS=""
LIB_DUNE_EXISTING_CONTENTS=""
if [ -f "${LIB_DUNE_FILE}" ]; then
  LIB_DUNE_EXISTING_CONTENTS=$(<"${LIB_DUNE_FILE}")
fi
LIB_DUNE_CONTENTS="(library"
LIB_DUNE_CONTENTS=$(printf "%s\\n %s" "${LIB_DUNE_CONTENTS}" "  ; !!!! This dune file is generated from the package.json file. Do NOT modify by hand.")
LIB_DUNE_CONTENTS=$(printf "%s\\n %s" "${LIB_DUNE_CONTENTS}" "  ; !!!! Instead, edit the package.json and then rerun 'esy pesy' at the project root.")
LIB_DUNE_CONTENTS=$(printf "%s\\n %s" "${LIB_DUNE_CONTENTS}" "  ; The namespace other code see this as")
LIB_DUNE_CONTENTS=$(printf "%s\\n %s" "${LIB_DUNE_CONTENTS}" "  (name ${lib_NAMESPACE})")
LIB_DUNE_CONTENTS=$(printf "%s\\n %s" "${LIB_DUNE_CONTENTS}" "  (public_name chalk-console.lib)")
LIB_DUNE_CONTENTS=$(printf "%s\\n %s\\n" "${LIB_DUNE_CONTENTS}" "  (libraries ${lib_REQUIRE})")
LIB_DUNE_CONTENTS=$(printf "%s\\n %s\\n" "${LIB_DUNE_CONTENTS}" "  (c_names ${lib_C_NAMES})  ; From package.json cNames field")
LIB_DUNE_CONTENTS=$(printf "%s\\n %s\\n" "${LIB_DUNE_CONTENTS}" "  (flags (:standard ${lib_FLAGS}))  ; From package.json flags field")
LIB_DUNE_CONTENTS=$(printf "%s\\n %s\\n" "${LIB_DUNE_CONTENTS}" "  (ocamlc_flags (:standard ${lib_OCAMLC_FLAGS}))  ; From package.json ocamlcFlags field")
LIB_DUNE_CONTENTS=$(printf "%s\\n %s\\n" "${LIB_DUNE_CONTENTS}" "  (ocamlopt_flags (:standard ${lib_OCAMLOPT_FLAGS}))) ; From package.json ocamloptFlags")


NOW="1$(/opt/homebrew/bin/gdate +%N)"
DELTA_MS=$(( $(($NOW-$PREV_TIME)) / 1000000))
echo "DELTAms ${DELTA_MS}  - create lib build file"
PREV_TIME="1$(/opt/homebrew/bin/gdate +%N)"

if [ "${LIB_DUNE_EXISTING_CONTENTS}" == "${LIB_DUNE_CONTENTS}" ]; then
  true
else
  notifyUser
  BUILD_STALE_PROBLEM="true"
  if [ "${MODE}" == "build" ]; then
    printf "    □  Update lib/dune build config\\n"
  else
    printf "    %s☒%s  Update lib/dune build config\\n" "${BOLD}${GREEN}" "${RESET}"
    printf "%s" "$LIB_DUNE_CONTENTS" > "${LIB_DUNE_FILE}"
  fi
fi

NOW="1$(/opt/homebrew/bin/gdate +%N)"
DELTA_MS=$(( $(($NOW-$PREV_TIME)) / 1000000))
echo "DELTAms ${DELTA_MS}  - verify lib build existing contents"
PREV_TIME="1$(/opt/homebrew/bin/gdate +%N)"


BIN_DIR="${cur__root}/bin"
BIN_DUNE_FILE="${BIN_DIR}/dune"
# FOR BINARY IN DIRECTORY bin
bin_MAIN_MODULE="${bin_MAIN_MODULE:-$DEFAULT_MAIN_MODULE_NAME}"

bin_MAIN_MODULE_NAME="${bin_MAIN_MODULE%%.*}"


NOW="1$(/opt/homebrew/bin/gdate +%N)"
DELTA_MS=$(( $(($NOW-$PREV_TIME)) / 1000000))
echo "DELTAms ${DELTA_MS}  - setup bin vars"
PREV_TIME="1$(/opt/homebrew/bin/gdate +%N)"

# https://stackoverflow.com/a/965072
if [ "$bin_MAIN_MODULE_NAME"=="$bin_MAIN_MODULE" ]; then
  # If they did not specify an extension, we'll assume it is .re
  bin_MAIN_MODULE_FILENAME="${bin_MAIN_MODULE}.re"
else
  bin_MAIN_MODULE_FILENAME="${bin_MAIN_MODULE}"
fi

NOW="1$(/opt/homebrew/bin/gdate +%N)"
DELTA_MS=$(( $(($NOW-$PREV_TIME)) / 1000000))
echo "DELTAms ${DELTA_MS}  - check bin main mod name"
PREV_TIME="1$(/opt/homebrew/bin/gdate +%N)"


if [ -f  "${BIN_DIR}/${bin_MAIN_MODULE_FILENAME}" ]; then
  true
else
  BUILD_STALE_PROBLEM="true"
  notifyUser
  echo ""
  if [ "${MODE}" == "build" ]; then
    printf "    □  Generate %s main module\\n" "${bin_MAIN_MODULE_FILENAME}"
  else
    printf "    %s☒%s  Generate %s main module\\n" "${BOLD}${GREEN}" "${RESET}" "${bin_MAIN_MODULE_FILENAME}"
    mkdir -p "${BIN_DIR}"
    printf "print_endline(\"Hello!\");" > "${BIN_DIR}/${bin_MAIN_MODULE_FILENAME}"
  fi
fi



NOW="1$(/opt/homebrew/bin/gdate +%N)"
DELTA_MS=$(( $(($NOW-$PREV_TIME)) / 1000000))
echo "DELTAms ${DELTA_MS}  - check bin main module exists"
PREV_TIME="1$(/opt/homebrew/bin/gdate +%N)"

if [ -d "${BIN_DIR}" ]; then
  LAST_EXE_NAME="ChalkConsole.exe"
  BIN_DUNE_EXISTING_CONTENTS=""
  if [ -f "${BIN_DUNE_FILE}" ]; then
    BIN_DUNE_EXISTING_CONTENTS=$(<"${BIN_DUNE_FILE}")
  else
    BIN_DUNE_EXISTING_CONTENTS=""
  fi

  NOW="1$(/opt/homebrew/bin/gdate +%N)"
  DELTA_MS=$(( $(($NOW-$PREV_TIME)) / 1000000))
  echo "DELTAms ${DELTA_MS}  - read existing dune bin contents"
  PREV_TIME="1$(/opt/homebrew/bin/gdate +%N)"


  BIN_DUNE_CONTENTS="(executable"
  BIN_DUNE_CONTENTS=$(printf "%s\\n %s" "${BIN_DUNE_CONTENTS}" "  ; !!!! This dune file is generated from the package.json file. Do NOT modify by hand.")
  BIN_DUNE_CONTENTS=$(printf "%s\\n %s" "${BIN_DUNE_CONTENTS}" "  ; !!!! Instead, edit the package.json and then rerun 'esy pesy' at the project root.")
  BIN_DUNE_CONTENTS=$(printf "%s\\n %s" "${BIN_DUNE_CONTENTS}" "  ; The entrypoint module")
  BIN_DUNE_CONTENTS=$(printf "%s\\n %s" "${BIN_DUNE_CONTENTS}" "  (name ${bin_MAIN_MODULE_NAME})  ;  From package.json main field")
  BIN_DUNE_CONTENTS=$(printf "%s\\n %s" "${BIN_DUNE_CONTENTS}" "  (public_name ChalkConsole.exe)  ;  From package.json name field")
  BIN_DUNE_CONTENTS=$(printf "%s\\n %s\\n" "${BIN_DUNE_CONTENTS}" "  (libraries ${bin_REQUIRE}) ;  From package.json require field (array of strings)")
  BIN_DUNE_CONTENTS=$(printf "%s\\n %s\\n" "${BIN_DUNE_CONTENTS}" "  (flags (:standard ${bin_FLAGS})) ;  From package.json flags field")
  BIN_DUNE_CONTENTS=$(printf "%s\\n %s\\n" "${BIN_DUNE_CONTENTS}" "  (ocamlc_flags (:standard ${bin_OCAMLC_FLAGS}))  ; From package.json ocamlcFlags field")
  BIN_DUNE_CONTENTS=$(printf "%s\\n %s\\n" "${BIN_DUNE_CONTENTS}" "  (ocamlopt_flags (:standard ${bin_OCAMLOPT_FLAGS})))  ; From package.json ocamloptFlags field")


  
  NOW="1$(/opt/homebrew/bin/gdate +%N)"
  DELTA_MS=$(( $(($NOW-$PREV_TIME)) / 1000000))
  echo "DELTAms ${DELTA_MS}  - create bin dune contents"
  PREV_TIME="1$(/opt/homebrew/bin/gdate +%N)"

  if [ "${BIN_DUNE_EXISTING_CONTENTS}" == "${BIN_DUNE_CONTENTS}" ]; then
    true
  else
    notifyUser
    BUILD_STALE_PROBLEM="true"
    if [ "${MODE}" == "build" ]; then
      printf "    □  Update bin/dune build config\\n"
    else
      printf "    %s☒%s  Update bin/dune build config\\n" "${BOLD}${GREEN}" "${RESET}"
      printf "%s" "${BIN_DUNE_CONTENTS}" > "${BIN_DUNE_FILE}"
      mkdir -p "${BIN_DIR}"
    fi
  fi
  
  NOW="1$(/opt/homebrew/bin/gdate +%N)"
  DELTA_MS=$(( $(($NOW-$PREV_TIME)) / 1000000))
  echo "DELTAms ${DELTA_MS}  - check bin against required bin"
  PREV_TIME="1$(/opt/homebrew/bin/gdate +%N)"

else
  BUILD_STALE_PROBLEM="true"
  notifyUser
  if [ "${MODE}" == "build" ]; then
    printf "    □  Generate missing the bin directory described in package.json buildDirs\\n"
  else
    printf "    %s☒%s  Generate missing the bin directory described in package.json buildDirs\\n" "${BOLD}${GREEN}" "${RESET}"
    mkdir -p "${BIN_DIR}"
  fi
fi

if [ -f  "${cur__root}/dune" ]; then
  true
else
  BUILD_STALE_PROBLEM="true"
  notifyUser
  if [ "${MODE}" == "build" ]; then
    printf "    □  Update ./dune to ignore node_modules\\n"
  else
    printf "    %s☒%s  Update ./dune to ignore node_modules\\n" "${BOLD}${GREEN}" "${RESET}"
    printf "(ignored_subdirs (node_modules))" > "${cur__root}/dune"
  fi
fi

NOW="1$(/opt/homebrew/bin/gdate +%N)"
DELTA_MS=$(( $(($NOW-$PREV_TIME)) / 1000000))
echo "DELTAms ${DELTA_MS}  - check root dune exists"
PREV_TIME="1$(/opt/homebrew/bin/gdate +%N)"



if [ -f  "${cur__root}/${PACKAGE_NAME}.opam" ]; then
  true
else
  BUILD_STALE_PROBLEM="true"
  notifyUser
  if [ "${MODE}" == "build" ]; then
    printf "    □  Add %s\\n" "${PACKAGE_NAME}.opam"
  else
    printf "    %s☒%s  Add %s\\n" "${BOLD}${GREEN}" "${RESET}" "${PACKAGE_NAME}.opam" 
    touch "${cur__root}/${PACKAGE_NAME}.opam"
  fi
fi


NOW="1$(/opt/homebrew/bin/gdate +%N)"
DELTA_MS=$(( $(($NOW-$PREV_TIME)) / 1000000))
echo "DELTAms ${DELTA_MS}  - check opam file exists"
PREV_TIME="1$(/opt/homebrew/bin/gdate +%N)"


if [ -f  "${cur__root}/dune-project" ]; then
  true
else
  BUILD_STALE_PROBLEM="true"
  notifyUser
  if [ "${MODE}" == "build" ]; then
    printf "    □  Add a ./dune-project\\n"
  else
    printf "    %s☒%s  Add a ./dune-project\\n" "${BOLD}${GREEN}" "${RESET}"
    printf "(lang dune 1.0)\\n (name %s)" "${PACKAGE_NAME}" > "${cur__root}/dune-project"
  fi
fi

NOW="1$(/opt/homebrew/bin/gdate +%N)"
DELTA_MS=$(( $(($NOW-$PREV_TIME)) / 1000000))
echo "DELTAms ${DELTA_MS}  - check root dune-project file exists"
PREV_TIME="1$(/opt/homebrew/bin/gdate +%N)"


if [ "${MODE}" == "build" ]; then
  if [ "${BUILD_STALE_PROBLEM}" == "true" ]; then
    printf "\\n  %sTo perform those updates and build run:%s\n\n" "${BOLD}${YELLOW}" "${RESET}"
    printf "    esy pesy\\n\\n\\n\\n"
    exit 1
  else
    # If you list a refmterr as a dev dependency, we'll use it!

    BUILD_FAILED=""
    if hash refmterr 2>/dev/null; then
      refmterr dune build -p "${PACKAGE_NAME}" || BUILD_FAILED="true"
    else
      dune build -p "${PACKAGE_NAME}" || BUILD_FAILED="true"
    fi

    NOW="1$(/opt/homebrew/bin/gdate +%N)"
    DELTA_MS=$(( $(($NOW-$PREV_TIME)) / 1000000))
    echo "DELTAms ${DELTA_MS}  - perform actual build"
    PREV_TIME="1$(/opt/homebrew/bin/gdate +%N)"

    if [ -z "$BUILD_FAILED" ]; then
      printf "\\n%s  Build Succeeded!%s " "${BOLD}${GREEN}" "${RESET}"
      if [ -z "$LAST_EXE_NAME" ]; then
        printf "\\n\\n"
        true
      else
        # If we built an EXE
        printf "%sTo test a binary:%s\\n\\n" "${BOLD}" "${RESET}"
        printf "      esy x %s\\n\\n\\n" "${LAST_EXE_NAME}"
      fi
      true
      NOW="1$(/opt/homebrew/bin/gdate +%N)"
      DELTA_MS=$(( $(($NOW-$PREV_TIME)) / 1000000))
      echo "DELTAms ${DELTA_MS}  - check build failure"
      PREV_TIME="1$(/opt/homebrew/bin/gdate +%N)"
    else
      exit 1
    fi
  fi
else
  # In update mode.
  if [ "${BUILD_STALE_PROBLEM}" == "true" ]; then
    printf "\\n  %sUpdated!%s %sNow run:%s\\n\\n" "${BOLD}${GREEN}" "${RESET}" "${BOLD}" "${RESET}"
    printf "    esy build\\n\\n\\n"
  else
    printf "\\n  %sAlready up to date!%s %sNow run:%s\\n\\n" "${BOLD}${GREEN}" "${RESET}" "${BOLD}" "${RESET}"
    printf "      esy build\\n\\n\\n"
  fi
fi

      NOW="1$(/opt/homebrew/bin/gdate +%N)"
      DELTA_MS=$(( $(($NOW-$PREV_TIME)) / 1000000))
      echo "DELTAms ${DELTA_MS}  - end"
      PREV_TIME="1$(/opt/homebrew/bin/gdate +%N)"

      NOW="1$(/opt/homebrew/bin/gdate +%N)"
      DELTA_MS=$(( $(($NOW-$PREV_TIME)) / 1000000))
      echo "DELTAms ${DELTA_MS}  - end"
      PREV_TIME="1$(/opt/homebrew/bin/gdate +%N)"

```



Although we quickly call into genEverything if the package.json hasn't changed,
there appears to be an 80ms overhead in pesy until we get to that point.


DELTAms 4 - baseline margin of error amount
DELTAms 5 - check help
DELTAms 12 - check env
DELTAms 13 - check create
DELTAms 7 - record pesy dir

super-project-now@0.0.0
DELTAms 6 - output title
DELTAms 7 - mkdir cur__target_dir
DELTAms 7 - check if md5 exists
DELTAms 12 - compute md5
DELTAms 14 - get previous md5
