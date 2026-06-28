# Build-time image work-dir preparation (run via `cmake -P`).
#   -DSEED=<read-only image tree>  -DWORK=<writable work-dir>
#   -DDEBUG_DIR=<checked-DLL overlay | "none">  -DAUTORUN=<exe baked into HKLM\init>
#   -DMODS=<our built modules, '|'-separated>
# Mirrors SEED into WORK, overlays the debug DLLs (debug image only), drops our
# freshly-built modules into WORK/OS, then rewrites the Autorun registry value.

file(COPY "${SEED}/" DESTINATION "${WORK}")                 # retail baseline

if(DEBUG_DIR AND NOT DEBUG_DIR STREQUAL "none")
  file(COPY "${DEBUG_DIR}/" DESTINATION "${WORK}")          # overlay checked coredll/gwes/...
endif()

string(REPLACE "|" ";" _mods "${MODS}")
foreach(m ${_mods})
  file(COPY "${m}" DESTINATION "${WORK}/OS")                # our SH-4 modules win
endforeach()

# Rewrite the Autorun value. AUTORUN is given with forward slashes (e.g. dcshell.exe, or
# /CD-ROM/DC.EXE for the disc binary); convert '/' to a reg-escaped '\\' so the value is
# valid .reg syntax (single backslashes would be parsed as escapes and break regcomp).
# Use REGEX only to FIND the line, then a LITERAL string(REPLACE) (no escape handling).
string(REPLACE "/" "\\\\" AUTORUN_REG "${AUTORUN}")
file(READ "${WORK}/gemini.reg" _r)
string(REGEX MATCH "\"Autorun\"=\"[^\"]*\"" _old "${_r}")
if(_old)
  string(REPLACE "${_old}" "\"Autorun\"=\"${AUTORUN_REG}\"" _r "${_r}")
  file(WRITE "${WORK}/gemini.reg" "${_r}")
else()
  message(WARNING "prep-image: no \"Autorun\"= line found in gemini.reg")
endif()
