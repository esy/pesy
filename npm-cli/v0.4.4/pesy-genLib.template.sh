
# Perform validation:

LIB_DIR="${cur__root}/<ORIG_DIR>"
LIB_DUNE_FILE="${LIB_DIR}/dune"

# TODO: Error if there are multiple libraries all using the default namespace.
if [ -d "${LIB_DIR}" ]; then
  true
else
  BUILD_STALE_PROBLEM="true"
  notifyUser
  if [ "${MODE}" == "build" ]; then
    printf "    □  Your project is missing the <ORIG_DIR> directory described in package.json buildDirs\\n"
  else
    printf "    %s☒%s  Your project is missing the <ORIG_DIR> directory described in package.json buildDirs\\n" "${BOLD}${GREEN}" "${RESET}"
    mkdir -p "${LIB_DIR}"
  fi
fi

LIB_DUNE_CONTENTS=""
LIB_DUNE_EXISTING_CONTENTS=""
if [ -f "${LIB_DUNE_FILE}" ]; then
  LIB_DUNE_EXISTING_CONTENTS=$(<"${LIB_DUNE_FILE}")
fi
LIB_DUNE_CONTENTS=$(printf "%s\\n%s" "${LIB_DUNE_CONTENTS}" "; !!!! This dune file is generated from the package.json file by pesy. If you modify it by hand")
LIB_DUNE_CONTENTS=$(printf "%s\\n%s" "${LIB_DUNE_CONTENTS}" "; !!!! your changes will be undone! Instead, edit the package.json and then rerun 'esy pesy' at the project root.")
LIB_DUNE_CONTENTS=$(printf "%s\\n%s %s" "${LIB_DUNE_CONTENTS}" "; !!!! If you want to stop using pesy and manage this file by hand, change package.json's 'esy.build' command to: refmterr dune build -p " "${cur__name}")
LIB_DUNE_CONTENTS=$(printf "%s\\n%s" "${LIB_DUNE_CONTENTS}" "(library")
LIB_DUNE_CONTENTS=$(printf "%s\\n %s" "${LIB_DUNE_CONTENTS}" "  ; The namespace that other packages/libraries will access this library through")
LIB_DUNE_CONTENTS=$(printf "%s\\n %s" "${LIB_DUNE_CONTENTS}" "  (name ${<DIR>_NAMESPACE})")
LIB_DUNE_CONTENTS=$(printf "%s\\n %s" "${LIB_DUNE_CONTENTS}" "  ; Other libraries list this name in their package.json 'require' field to use this library.")
LIB_DUNE_CONTENTS=$(printf "%s\\n %s" "${LIB_DUNE_CONTENTS}" "  (public_name <LIB_NAME>)")
if [ ! -z "${<DIR>_REQUIRE}" ]; then
  LIB_DUNE_CONTENTS=$(printf "%s\\n %s\\n" "${LIB_DUNE_CONTENTS}" "  (libraries ${<DIR>_REQUIRE})")
fi
if [ ! -z "${<DIR>_WRAPPED}" ]; then
  LIB_DUNE_CONTENTS=$(printf "%s\\n%s\\n" "${LIB_DUNE_CONTENTS}" "   (wrapped ${<DIR>_WRAPPED})  ; From package.json wrapped field")
fi
if [ ! -z "${<DIR>_C_NAMES}" ]; then
  LIB_DUNE_CONTENTS=$(printf "%s\\n %s\\n" "${LIB_DUNE_CONTENTS}" "  (c_names ${<DIR>_C_NAMES})  ; From package.json cNames field")
fi
if [ -z "${<DIR>_JSOO_FLAGS}" ] && [ -z "${<DIR>_JSOO_FILES}" ]; then
  # No jsoo flags whatsoever
  true
else
  LIB_DUNE_CONTENTS=$(printf "%s\\n %s" "${LIB_DUNE_CONTENTS}" "  (js_of_ocaml ")
  if [ ! -z "${<DIR>_JSOO_FLAGS}" ]; then
    LIB_DUNE_CONTENTS=$(printf "%s\\n %s\\n" "${LIB_DUNE_CONTENTS}" "    (flags (${<DIR>_JSOO_FLAGS}))  ; From package.json jsooFlags field")
  fi
  if [ ! -z "${<DIR>_JSOO_FILES}" ]; then
    LIB_DUNE_CONTENTS=$(printf "%s\\n %s\\n" "${LIB_DUNE_CONTENTS}" "    (javascript_files ${<DIR>_JSOO_FILES})  ; From package.json jsooFiles field")
  fi
  LIB_DUNE_CONTENTS=$(printf "%s\\n%s" "${LIB_DUNE_CONTENTS}" "   )")
fi
if [ ! -z "${<DIR>_FLAGS}" ]; then
  LIB_DUNE_CONTENTS=$(printf "%s\\n %s\\n" "${LIB_DUNE_CONTENTS}" "  (flags (${<DIR>_FLAGS}))  ; From package.json flags field")
fi
if [ ! -z "${<DIR>_OCAMLC_FLAGS}" ]; then
  LIB_DUNE_CONTENTS=$(printf "%s\\n %s\\n" "${LIB_DUNE_CONTENTS}" "  (ocamlc_flags (${<DIR>_OCAMLC_FLAGS}))  ; From package.json ocamlcFlags field")
fi
if [ ! -z "${<DIR>_OCAMLOPT_FLAGS}" ]; then
  LIB_DUNE_CONTENTS=$(printf "%s\\n %s\\n" "${LIB_DUNE_CONTENTS}" "  (ocamlopt_flags (${<DIR>_OCAMLOPT_FLAGS})) ; From package.json ocamloptFlags")
fi
if [ ! -z "${<DIR>_MODES}" ]; then
  LIB_DUNE_CONTENTS=$(printf "%s\\n %s\\n" "${LIB_DUNE_CONTENTS}" "  (modes (${<DIR>_MODES}))  ; From package.json modes field")
fi
if [ ! -z "${<DIR>_IMPLEMENTS}" ]; then
  LIB_DUNE_CONTENTS=$(printf "%s\\n %s\\n" "${LIB_DUNE_CONTENTS}" "  (implements ${<DIR>_IMPLEMENTS}) ; From package.json implements")
fi
if [ ! -z "${<DIR>_VIRTUALMODULES}" ]; then
  LIB_DUNE_CONTENTS=$(printf "%s\\n %s\\n" "${LIB_DUNE_CONTENTS}" "  (virtual_modules ${<DIR>_VIRTUALMODULES}) ; From package.json virtualModules")
fi
if [ ! -z "${<DIR>_RAWBUILDCONFIG}" ]; then
  LIB_DUNE_CONTENTS=$(printf "%s\\n %s\\n" "${LIB_DUNE_CONTENTS}" "  ${<DIR>_RAWBUILDCONFIG} ")
fi
if [ ! -z "${<DIR>_PREPROCESS}" ]; then
  LIB_DUNE_CONTENTS=$(printf "%s\\n %s\\n" "${LIB_DUNE_CONTENTS}" "  (preprocess (${<DIR>_PREPROCESS}))  ; From package.json preprocess field")
fi
LIB_DUNE_CONTENTS=$(printf "%s\\n%s\\n" "${LIB_DUNE_CONTENTS}" ")")

if [ ! -z "${<DIR>_IGNOREDSUBDIRS}" ]; then
  LIB_DUNE_CONTENTS=$(printf "%s\\n%s\\n" "${LIB_DUNE_CONTENTS}" "(ignored_subdirs (${<DIR>_IGNOREDSUBDIRS}))  ; From package.json ignoreSubdirs field")
fi
if [ ! -z "${<DIR>_INCLUDESUBDIRS}" ]; then
  LIB_DUNE_CONTENTS=$(printf "%s\\n%s\\n" "${LIB_DUNE_CONTENTS}" "(include_subdirs ${<DIR>_INCLUDESUBDIRS})  ; From package.json includeSubdirs field")
fi

if [ ! -z "${<DIR>_RAWBUILDCONFIGFOOTER}" ]; then
  LIB_DUNE_CONTENTS=$(printf "%s\\n %s\\n" "${LIB_DUNE_CONTENTS}" "${<DIR>_RAWBUILDCONFIGFOOTER}")
fi

if [ "${LIB_DUNE_EXISTING_CONTENTS}" == "${LIB_DUNE_CONTENTS}" ]; then
  true
else
  notifyUser
  BUILD_STALE_PROBLEM="true"
  if [ "${MODE}" == "build" ]; then
    printf "    □  Update <ORIG_DIR>/dune build config\\n"
  else
    printf "    %s☒%s  Update <ORIG_DIR>/dune build config\\n" "${BOLD}${GREEN}" "${RESET}"
    printf "%s" "$LIB_DUNE_CONTENTS" > "${LIB_DUNE_FILE}"
  fi
fi
