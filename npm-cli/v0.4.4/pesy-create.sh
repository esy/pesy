#!/bin/bash

set -e
set -u

uppers="ABCDEFGHIJKLMNOPQRSTUVWXYZ"
lowers="abcdefghijklmnopqrstuvwxyz"
wordBoundaries="._-"

#usage: camelcasify "some_stringHere" -> "some-string-here"
#usage: camelcasify "domHTML" -> "dom-html"
#usage: camelcasify "domHtML" -> "dom-ht-ml"
#usage: camelcasify "dom-HTML" -> "dom-html"
lowerHyphenate(){
  OUTPUT=""
  i=0
  prevWasUpper="true" # Causes leading uppercase
  while ([ $i -lt ${#1} ]) do
    CUR=${1:$i:1}
    case $uppers in
        *$CUR*)
          CUR=${uppers%$CUR*};
          if [[ "${prevWasUpper}" == "false" ]]; then
            OUTPUT="${OUTPUT}-${lowers:${#CUR}:1}"
          else
            # No hyphen
            OUTPUT="${OUTPUT}${lowers:${#CUR}:1}"
          fi
          prevWasUpper="true" # Causes leading uppercase
          ;;
        *)
          OUTPUT="${OUTPUT}$CUR"
          prevWasUpper="false" # Causes leading uppercase
          ;;
    esac
    i=$((i+1))
  done
  echo "${OUTPUT}"
}

function printDirectory() {
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
}


# https://stackoverflow.com/a/677212
# We want to check if esy has been installed globally, but only when running
# naked on the command line (just `pesy`).
if hash esy 2>/dev/null; then
  true
else
  printf "\\n%sERROR: You haven't installed esy globally. First install esy then try again.%s" "${RED}${BOLD}" "${RESET}"
  printf "\\n"
  printf "\\n    npm install -g esy"
  printf "\\n\\n"
  exit 1
fi

HAS_JSON="false"
if [ -f  "${PWD}/package.json" ]; then
  HAS_JSON="true"
else
  if [ -f  "${PWD}/esy.json" ]; then
    HAS_JSON="true"
  fi
fi
if [[ "$HAS_JSON" == "true" ]]; then
  printf "\\n%sIncorrect Usage:%s" "${RED}${BOLD}" "${RESET}"
  printf "\\n"
  printf "\\npesy should only be run via 'esy pesy', unless you want to create a new project"
  printf "\\nin a fresh directory. This does not appear to be a fresh directory because"
  printf "\\nit contains a package.json or esy.json file.\n\n"
  printf "\\nType esy --help for more assistance.\n\n"
  exit 1
fi

CUR_DIR_NAME=${PWD##*/}
CUR_DIR_NAME_KEBAB=$(lowerHyphenate "${CUR_DIR_NAME}")
printf "\\n%sCreate new package in the CURRENT DIRECTORY %s?%s\\n" "${BOLD}" "${PWD}" "${RESET}"
printf "\\n"
printf "Enter package name (lowercase/hyphens) [default %s]:" "${CUR_DIR_NAME_KEBAB}"
read ANSWER
if [[ "${ANSWER}" == "" ]]; then
  ANSWER_KEBAB="${CUR_DIR_NAME_KEBAB}"
else
  ANSWER_KEBAB=$(lowerHyphenate "${ANSWER}")
  if [[ "$ANSWER" != "${ANSWER_KEBAB}" ]]; then
    printf "\\n%sPackage names should only consist of lower case and hyphens. Pesy is going to adjust the name to%s:%s" "${YELLOW}${BOLD}" "${RESET}" "${BOLD}${ANSWER_KEBAB}${RESET}"
  fi
fi
PACKAGE_NAME_FULL="${ANSWER_KEBAB}"
# Strip off any scope like @esy-ocaml/foo-package.
PACKAGE_NAME="${PACKAGE_NAME_FULL##*/}"

# https://stackoverflow.com/a/8952274
source "${PESY_DIR}/pesy-name-utils.sh"

# Gnu uppercasing extensions to sed don't exist on mac.
PACKAGE_NAME_UPPER_CAMEL=$(upperCamelCasify "${PACKAGE_NAME}")
PUBLIC_LIB_NAME="${PACKAGE_NAME}.lib"

VERSION="0.0.0"
sed  -e "s;<PACKAGE_NAME_FULL>;${PACKAGE_NAME_FULL};g; s;<VERSION>;${VERSION};g; s;<PUBLIC_LIB_NAME>;${PUBLIC_LIB_NAME};g; s;<PACKAGE_NAME_UPPER_CAMEL>;${PACKAGE_NAME_UPPER_CAMEL};g" "${PESY_DIR}/pesy-package.template.json"  >> "${PWD}/package.json"


mkdir -p "${PWD}/executable/"
printf "%s;\\n" "${PACKAGE_NAME_UPPER_CAMEL}.Util.foo()"                   >> "${PWD}/executable/${PACKAGE_NAME_UPPER_CAMEL}App.re"

mkdir -p "${PWD}/library/"
printf "let foo = () => print_endline(\"Hello\");\\n"                   >> "${PWD}/library/Util.re"

mkdir -p "${PWD}/test/"
printf "%s;\\n" "${PACKAGE_NAME_UPPER_CAMEL}.Util.foo()"                   >> "${PWD}/test/Test${PACKAGE_NAME_UPPER_CAMEL}.re"
printf "print_endline(\"Add Your Test Cases Here\");\\n"                   >> "${PWD}/test/Test${PACKAGE_NAME_UPPER_CAMEL}.re"

if [ -f  "${PWD}/azure-pipelines.yml" ]; then
  printf "%s-azure-pipelines.yml already exists. Skipping azure-pipelines.yml generation.%s\\n" "${YELLOW}" "${RESET}"
else
  mkdir -p "${PWD}/.ci"
  cp "${PESY_DIR}/azure-ci-template/azure-pipelines.yml" "${PWD}/azure-pipelines.yml"
  sed  -e "s;<PACKAGE_NAME_FULL>;${PACKAGE_NAME_FULL};g; s;<PACKAGE_NAME>;${PACKAGE_NAME};g; s;<PUBLIC_LIB_NAME>;${PUBLIC_LIB_NAME};g; s;<PACKAGE_NAME_UPPER_CAMEL>;${PACKAGE_NAME_UPPER_CAMEL};g" "${PESY_DIR}/azure-ci-template/esy-build-steps.template.yml"  >> "${PWD}/.ci/esy-build-steps.yml"
  cp "${PESY_DIR}/azure-ci-template/publish-build-cache.yml" "${PWD}/.ci/publish-build-cache.yml"
  cp "${PESY_DIR}/azure-ci-template/restore-build-cache.yml" "${PWD}/.ci/restore-build-cache.yml"
fi

if [ -f  "${PWD}/README.md" ]; then
  printf "%s-README.md already exists. Skipping README generation.%s\\n" "${YELLOW}" "${RESET}"
else
  sed  -e "s;<PACKAGE_NAME_FULL>;${PACKAGE_NAME_FULL};g; s;<PACKAGE_NAME>;${PACKAGE_NAME};g; s;<PUBLIC_LIB_NAME>;${PUBLIC_LIB_NAME};g; s;<PACKAGE_NAME_UPPER_CAMEL>;${PACKAGE_NAME_UPPER_CAMEL};g" "${PESY_DIR}/pesy-README.template.md"  >> "${PWD}/README.md"
fi

if [ -f  "${PWD}/.gitignore" ]; then
  printf "%s-.gitignore already exists. Skipping .gitignore generation.%s\\n" "${YELLOW}" "${RESET}"
else
  sed  -e "s;<PACKAGE_NAME>;${PACKAGE_NAME};g; s;<PUBLIC_LIB_NAME>;${PUBLIC_LIB_NAME};g; s;<PACKAGE_NAME_UPPER_CAMEL>;${PACKAGE_NAME_UPPER_CAMEL};g" "${PESY_DIR}/pesy-gitignore.template"  >> "${PWD}/.gitignore"
fi

printf "\\n%s%s package.json created. Running 'esy install' and 'esy pesy'\\n\\n%s" "${BOLD}"  "${PACKAGE_NAME_FULL}@${VERSION}" "${RESET}"
