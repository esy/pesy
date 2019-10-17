#!/bin/bash

set -e
set -u

# Also removes /
# https://stackoverflow.com/a/8952274
uppers="ABCDEFGHIJKLMNOPQRSTUVWXYZ"
lowers="abcdefghijklmnopqrstuvwxyz"
wordBoundaries="._-/"
upperCamelCasify(){ #usage: camelcasify "some_string-Here" -> "SomeStringHere"
  OUTPUT=""
  i=0
  prevWasBoundary="true" # Causes leading uppercase
  while ([ $i -lt ${#1} ]) do
    CUR=${1:$i:1}
    if [[ "${prevWasBoundary}" == "true" ]]; then
      # Then upper case this one.
      case "$lowers" in
          *$CUR*)
            CUR=${lowers%$CUR*}
            OUTPUT="${OUTPUT}${uppers:${#CUR}:1}"
            ;;
          *)
            OUTPUT="${OUTPUT}$CUR"
            ;;
      esac
      prevWasBoundary="false"
    else
      case $wordBoundaries in
          *$CUR*)
            prevWasBoundary="true"
            ;;
          *)
            OUTPUT="${OUTPUT}$CUR"
            ;;
      esac
    fi
    i=$((i+1))
  done
  echo "${OUTPUT}"
}
