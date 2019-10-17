BIN_DIR="${cur__root}/<ORIG_DIR>"
BIN_DUNE_FILE="${BIN_DIR}/dune"
# FOR BINARY IN DIRECTORY <DIR>
<DIR>_MAIN_MODULE="${<DIR>_MAIN_MODULE:-$DEFAULT_MAIN_MODULE_NAME}"

<DIR>_MAIN_MODULE_NAME="${<DIR>_MAIN_MODULE%%.*}"
# https://stackoverflow.com/a/965072
if [ "$<DIR>_MAIN_MODULE_NAME"=="$<DIR>_MAIN_MODULE" ]; then
  # If they did not specify an extension, we'll assume it is .re
  <DIR>_MAIN_MODULE_FILENAME="${<DIR>_MAIN_MODULE}.re"
else
  <DIR>_MAIN_MODULE_FILENAME="${<DIR>_MAIN_MODULE}"
fi

if [ -f  "${BIN_DIR}/${<DIR>_MAIN_MODULE_FILENAME}" ]; then
  true
else
  BUILD_STALE_PROBLEM="true"
  notifyUser
  echo ""
  if [ "${MODE}" == "build" ]; then
    printf "    □  Generate %s main module\\n" "${<DIR>_MAIN_MODULE_FILENAME}"
  else
    printf "    %s☒%s  Generate %s main module\\n" "${BOLD}${GREEN}" "${RESET}" "${<DIR>_MAIN_MODULE_FILENAME}"
    mkdir -p "${BIN_DIR}"
    printf "print_endline(\"Hello!\");" > "${BIN_DIR}/${<DIR>_MAIN_MODULE_FILENAME}"
  fi
fi

if [ -d "${BIN_DIR}" ]; then
  LAST_EXE_NAME="<EXE_NAME>"
  BIN_DUNE_EXISTING_CONTENTS=""
  if [ -f "${BIN_DUNE_FILE}" ]; then
    BIN_DUNE_EXISTING_CONTENTS=$(<"${BIN_DUNE_FILE}")
  else
    BIN_DUNE_EXISTING_CONTENTS=""
  fi
  BIN_DUNE_CONTENTS=""
  BIN_DUNE_CONTENTS=$(printf "%s\\n%s" "${BIN_DUNE_CONTENTS}" "; !!!! This dune file is generated from the package.json file by pesy. If you modify it by hand")
  BIN_DUNE_CONTENTS=$(printf "%s\\n%s" "${BIN_DUNE_CONTENTS}" "; !!!! your changes will be undone! Instead, edit the package.json and then rerun 'esy pesy' at the project root.")
  BIN_DUNE_CONTENTS=$(printf "%s\\n%s %s" "${BIN_DUNE_CONTENTS}" "; !!!! If you want to stop using pesy and manage this file by hand, change package.json's 'esy.build' command to: refmterr dune build -p " "${cur__name}")
  BIN_DUNE_CONTENTS=$(printf "%s\\n%s" "${BIN_DUNE_CONTENTS}" "(executable")
  BIN_DUNE_CONTENTS=$(printf "%s\\n %s" "${BIN_DUNE_CONTENTS}" "  ; The entrypoint module")
  BIN_DUNE_CONTENTS=$(printf "%s\\n %s" "${BIN_DUNE_CONTENTS}" "  (name ${<DIR>_MAIN_MODULE_NAME})  ;  From package.json main field")
  BIN_DUNE_CONTENTS=$(printf "%s\\n %s" "${BIN_DUNE_CONTENTS}" "  ; The name of the executable (runnable via esy x <EXE_NAME>) ")
  BIN_DUNE_CONTENTS=$(printf "%s\\n %s" "${BIN_DUNE_CONTENTS}" "  (public_name <EXE_NAME>)  ;  From package.json name field")

  if [ -z "${<DIR>_JSOO_FLAGS}" ] && [ -z "${<DIR>_JSOO_FILES}" ]; then
    # No jsoo flags whatsoever
    true
  else
    BIN_DUNE_CONTENTS=$(printf "%s\\n %s" "${BIN_DUNE_CONTENTS}" "  (js_of_ocaml ")
    if [ ! -z "${<DIR>_JSOO_FLAGS}" ]; then
      BIN_DUNE_CONTENTS=$(printf "%s\\n %s\\n" "${BIN_DUNE_CONTENTS}" "    (flags (${<DIR>_JSOO_FLAGS}))  ; From package.json jsooFlags field")
    fi
    if [ ! -z "${<DIR>_JSOO_FILES}" ]; then
      BIN_DUNE_CONTENTS=$(printf "%s\\n %s\\n" "${BIN_DUNE_CONTENTS}" "    (javascript_files ${<DIR>_JSOO_FILES})  ; From package.json jsooFiles field")
    fi
    BIN_DUNE_CONTENTS=$(printf "%s\\n%s" "${BIN_DUNE_CONTENTS}" "   )")
  fi
  if [ ! -z "${<DIR>_REQUIRE}" ]; then
    BIN_DUNE_CONTENTS=$(printf "%s\\n %s\\n" "${BIN_DUNE_CONTENTS}" "  (libraries ${<DIR>_REQUIRE}) ;  From package.json require field (array of strings)")
  fi
  if [ ! -z "${<DIR>_FLAGS}" ]; then
    BIN_DUNE_CONTENTS=$(printf "%s\\n %s\\n" "${BIN_DUNE_CONTENTS}" "  (flags (${<DIR>_FLAGS})) ;  From package.json flags field")
  fi
  if [ ! -z "${<DIR>_OCAMLC_FLAGS}" ]; then
    BIN_DUNE_CONTENTS=$(printf "%s\\n %s\\n" "${BIN_DUNE_CONTENTS}" "  (ocamlc_flags (${<DIR>_OCAMLC_FLAGS}))  ; From package.json ocamlcFlags field")
  fi
  if [ ! -z "${<DIR>_OCAMLOPT_FLAGS}" ]; then
    BIN_DUNE_CONTENTS=$(printf "%s\\n %s\\n" "${BIN_DUNE_CONTENTS}" "  (ocamlopt_flags (${<DIR>_OCAMLOPT_FLAGS}))  ; From package.json ocamloptFlags field")
  fi
  if [ ! -z "${<DIR>_MODES}" ]; then
    BIN_DUNE_CONTENTS=$(printf "%s\\n %s\\n" "${BIN_DUNE_CONTENTS}" "  (modes (${<DIR>_MODES}))  ; From package.json modes field")
  fi
  if [ ! -z "${<DIR>_RAWBUILDCONFIG}" ]; then
    BIN_DUNE_CONTENTS=$(printf "%s\\n %s\\n" "${BIN_DUNE_CONTENTS}" "  ${<DIR>_RAWBUILDCONFIG} ")
  fi
  if [ ! -z "${<DIR>_PREPROCESS}" ]; then
    BIN_DUNE_CONTENTS=$(printf "%s\\n %s\\n" "${BIN_DUNE_CONTENTS}" "  (preprocess (${<DIR>_PREPROCESS}))  ; From package.json preprocess field")
  fi
  BIN_DUNE_CONTENTS=$(printf "%s\\n%s\\n" "${BIN_DUNE_CONTENTS}" ")")
  if [ ! -z "${<DIR>_IGNOREDSUBDIRS}" ]; then
    BIN_DUNE_CONTENTS=$(printf "%s\\n%s\\n" "${BIN_DUNE_CONTENTS}" "(ignored_subdirs (${<DIR>_IGNOREDSUBDIRS})) ;  From package.json ignoredSubdirs field")
  fi
  if [ ! -z "${<DIR>_INCLUDESUBDIRS}" ]; then
    BIN_DUNE_CONTENTS=$(printf "%s\\n%s\\n" "${BIN_DUNE_CONTENTS}" "(include_subdirs ${<DIR>_INCLUDESUBDIRS}) ;  From package.json includeSubdirs field")
  fi

  if [ ! -z "${<DIR>_RAWBUILDCONFIGFOOTER}" ]; then
    BIN_DUNE_CONTENTS=$(printf "%s\\n %s\\n" "${BIN_DUNE_CONTENTS}" "${<DIR>_RAWBUILDCONFIGFOOTER}")
  fi

  if [ "${BIN_DUNE_EXISTING_CONTENTS}" == "${BIN_DUNE_CONTENTS}" ]; then
    true
  else
    notifyUser
    BUILD_STALE_PROBLEM="true"
    if [ "${MODE}" == "build" ]; then
      printf "    □  Update <ORIG_DIR>/dune build config\\n"
    else
      printf "    %s☒%s  Update <ORIG_DIR>/dune build config\\n" "${BOLD}${GREEN}" "${RESET}"
      printf "%s" "${BIN_DUNE_CONTENTS}" > "${BIN_DUNE_FILE}"
      mkdir -p "${BIN_DIR}"
    fi
  fi
else
  BUILD_STALE_PROBLEM="true"
  notifyUser
  if [ "${MODE}" == "build" ]; then
    printf "    □  Generate missing the <ORIG_DIR> directory described in package.json buildDirs\\n"
  else
    printf "    %s☒%s  Generate missing the <ORIG_DIR> directory described in package.json buildDirs\\n" "${BOLD}${GREEN}" "${RESET}"
    mkdir -p "${BIN_DIR}"
  fi
fi
