# wince-dc - self-contained build of the Dreamcast WinCE modules (SH-4) + the bootable
# image. No external SDK: everything is vendored under vendor/. We don't use CMake's
# compiler model (the 1999 Renesas-SH cl.exe isn't a CMake-known compiler), so the
# project language is NONE and every step is an explicit custom command driving the
# vendored cl.exe / shasm.exe / link.exe and the makeimg image pipeline.
cmake_minimum_required(VERSION 3.20)
project(wince_dc NONE)

set(REPO       ${CMAKE_CURRENT_SOURCE_DIR})
set(SH_BIN     ${REPO}/vendor/sh-toolchain/bin/I386/SH)
set(HOST_BIN   ${REPO}/vendor/sh-toolchain/bin/I386)
set(CL         ${SH_BIN}/cl.exe)
set(SHASM      ${SH_BIN}/shasm.exe)
set(LINK       ${HOST_BIN}/link.exe)
set(SDK        ${REPO}/vendor/wcesdk)
set(SDK_INC    ${SDK}/inc)
set(SDK_LIB    ${SDK}/lib/retail)
set(CE3_ASM_INC ${REPO}/vendor/sh-toolchain/ce3-ppc2k/include)
set(OUTDIR     ${CMAKE_BINARY_DIR}/modules)         # built .dll/.exe land here
file(MAKE_DIRECTORY ${OUTDIR})

# SH-4 CE compile flags (same as the old build-*.bat CF).
set(CFLAGS /nologo /c /W3 -DUNDER_CE=212 -D_WIN32_WCE=212 -DUNICODE -D_UNICODE -DSH4=1 -DSHx=1)

# cl.exe/link.exe load their backend DLLs (c1*.dll, mspdb*.dll, ...) from the toolchain
# bins via PATH (the old .bats put SH_BIN;HOST_BIN on PATH). Prepend them per-command via
# `cmake -E env --modify path_list_prepend` (handles the ';' correctly, unlike a raw env).
set(ENVPFX ${CMAKE_COMMAND} -E env
    --modify "PATH=path_list_prepend:${SH_BIN}"
    --modify "PATH=path_list_prepend:${HOST_BIN}" --)

# add_sh_module(NAME TYPE dll|exe ENTRY <sym> [DEF <file>] OUT <file>
#               SOURCES <c...> [ASM <src...>] [LIBS <name.lib...>] [INCLUDES <dir...>])
# Includes are passed as /I (no INCLUDE env), libs as full paths (no LIB env), so there
# are no semicolon-list pitfalls; cl/link find their backend DLLs next to themselves.
function(add_sh_module NAME)
  cmake_parse_arguments(M "" "TYPE;ENTRY;DEF;OUT" "SOURCES;ASM;LIBS;INCLUDES" ${ARGN})
  set(objs "")
  set(incflags /I${SDK_INC})
  foreach(d ${M_INCLUDES})
    list(APPEND incflags /I${d})
  endforeach()
  foreach(src ${M_SOURCES})
    get_filename_component(base ${src} NAME_WE)
    set(obj ${OUTDIR}/${NAME}_${base}.obj)
    add_custom_command(OUTPUT ${obj}
      COMMAND ${ENVPFX} "${CL}" ${CFLAGS} ${incflags} /Fo${obj} ${src}
      DEPENDS ${src} VERBATIM)
    list(APPEND objs ${obj})
  endforeach()
  foreach(src ${M_ASM})
    get_filename_component(base ${src} NAME_WE)
    set(obj ${OUTDIR}/${NAME}_${base}.obj)
    add_custom_command(OUTPUT ${obj}
      COMMAND ${ENVPFX} "${SHASM}" -cpu=SH4 -DSH4=1 -DSHx=1 -nologo -I${CE3_ASM_INC} -object=${obj} ${src}
      DEPENDS ${src} VERBATIM)
    list(APPEND objs ${obj})
  endforeach()
  set(out ${OUTDIR}/${M_OUT})
  set(lf /nologo /machine:SH4 /subsystem:windowsce,2.12 /entry:${M_ENTRY})
  if(M_TYPE STREQUAL "dll")
    list(APPEND lf /dll)
  endif()
  if(M_DEF)
    list(APPEND lf /def:${M_DEF})
  endif()
  set(libs "")
  foreach(l ${M_LIBS})
    list(APPEND libs ${SDK_LIB}/${l})
  endforeach()
  add_custom_command(OUTPUT ${out}
    COMMAND ${ENVPFX} "${LINK}" ${lf} /out:${out} ${objs} ${libs}
    DEPENDS ${objs} VERBATIM)
  add_custom_target(${NAME} ALL DEPENDS ${out})
  set_property(GLOBAL APPEND PROPERTY WINCE_MODULES ${out})
endfunction()

# ---- SPI transport ----------------------------------------------------------------
add_sh_module(dcspi TYPE dll ENTRY DllMain DEF ${REPO}/drivers/dcspi/dcspi.def OUT dcspi.dll
  SOURCES ${REPO}/drivers/dcspi/dcspi.c
  LIBS coredll.lib corelibc.lib)

# ---- universal microstk link shim (mppp.dll) --------------------------------------
add_sh_module(mppp TYPE dll ENTRY dllentry DEF ${REPO}/net/netif/netif.def OUT mppp.dll
  SOURCES ${REPO}/net/netif/netif.c ${REPO}/net/netif/bba_hw.c ${REPO}/net/netif/ras.c
          ${REPO}/net/netif/w5500.c ${REPO}/net/netif/flashrom.c ${REPO}/net/netif/fblog.c
  ASM     ${REPO}/net/netif/g2lock.src
  INCLUDES ${REPO}/drivers/dcspi
  LIBS coredll.lib corelibc.lib)

# ---- DCWin desktop shell + PVR2 compositor ----------------------------------------
add_sh_module(dcshell TYPE exe ENTRY WinMainCRTStartup OUT dcshell.exe
  SOURCES ${REPO}/shell/dcshell.c ${REPO}/shell/dcgfx.c ${REPO}/shell/dcinput.c
  LIBS coredll.lib corelibc.lib ddraw.lib d3dim.lib dinput.lib dxguid.lib)

# ---- DCWin client apps (each links the client lib) --------------------------------
foreach(app dcwcalc dcwclock dcwexp dcwtask dcwmem)
  add_sh_module(${app} TYPE exe ENTRY WinMainCRTStartup OUT ${app}.exe
    SOURCES ${REPO}/shell/${app}.c ${REPO}/shell/dcwlib.c
    LIBS coredll.lib corelibc.lib)
endforeach()
add_sh_module(dcwnet TYPE exe ENTRY WinMainCRTStartup OUT dcwnet.exe
  SOURCES ${REPO}/shell/dcwnet.c ${REPO}/shell/dcwlib.c
  LIBS coredll.lib corelibc.lib winsock.lib)

# ====================================================================================
# Bootable image pipeline (opt-in: `cmake --build build --target gdi`).
#   modules -> deploy into image/OS -> makeimg -> NK.bin -> wrap -> 0winceos.bin -> GDI
# The read-only vendored release tree (vendor/wcesdk/image) is seeded into a writable
# build work-dir; our freshly-built modules overlay image/OS before makeimg runs.
# ====================================================================================
set(IMG_SEED ${SDK}/image)
set(IMG_WORK ${CMAKE_BINARY_DIR}/image)
set(NKBIN    ${IMG_WORK}/NK.bin)
set(WINCEOS  ${CMAKE_BINARY_DIR}/0winceos.bin)
set(DISCDIR  ${CMAKE_BINARY_DIR}/disc)

get_property(MODS GLOBAL PROPERTY WINCE_MODULES)

# The 1999 romimage/fmerge can't parse forward-slash paths (it mangles the path join),
# so feed it native backslash paths for the env values.
file(TO_NATIVE_PATH "${IMG_WORK}" IMG_WORK_NAT)
file(TO_NATIVE_PATH "${SDK}"      SDK_NAT)

# makeimg environment - faithful replica of toolchain/setenv.bat (no PATH here; the
# tools dir is prepended separately via --modify so the ';' stays intact).
set(IMG_ENV
  "CEVersion=212" "CESubsystem=windowsce,2.12" "CEConfigName=Dreamcast"
  # forward slashes on purpose: a trailing '\' would escape the closing quote on the
  # command line and blank the var (romimage then collapses every path to flat). Win32
  # accepts mixed separators, so $(_FLATRELEASEDIR)/OS/foo.dll resolves fine.
  "RELEASEDIR_STDCORE=/CoreOS_Standard/" "RELEASEDIR_HTMLCORE=/CoreOS_HTML/"
  "RELEASEDIR_OS=/OS/" "RELEASEDIR_SAMPLES=/Samples/" "RELEASEDIR_APPS=/Applications/"
  "IMGNODEBUGGER=1" "IMGNOSHELL=1" "IMGDIRECTDRAW=1" "IMGDIRECT3DIM=1" "IMGDDHAL=1"
  "IMGDIRECTSOUND=1" "IMGDIRECTINPUT=1" "IMGMICROSTK=1" "IMGNOCOMM=1" "IMGNOPCMCIA=1"
  "IMGNOCEDDK=1" "INITNOCOMM=1" "INITMICROSTK=1" "INITSERIAL=1" "INITMODEM=1"
  "INITDIRECTSHOW=1" "COUNTRY=USA" "_HOSTCPUTYPE=i386" "_TGTPROJ=DRAGON"
  "_DEPTREES=winceos gemini mcputech dshowdm6"
  "WCEDREAMCASTROOT=${SDK_NAT}" "_PUBLICROOT=${SDK_NAT}" "_FLATRELEASEDIR=${IMG_WORK_NAT}")

# Seed the work-dir, overlay our modules into OS\, run makeimg -> NK.bin.
add_custom_command(OUTPUT ${NKBIN}
  COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different ${IMG_SEED} ${IMG_WORK}
  COMMAND ${CMAKE_COMMAND} -E copy ${MODS} ${IMG_WORK}/OS/
  COMMAND ${CMAKE_COMMAND} -E env --modify "PATH=path_list_prepend:${SDK}/tools"
          ${IMG_ENV} -- "${SDK}/tools/makeimg.exe"
  WORKING_DIRECTORY ${IMG_WORK}
  DEPENDS ${MODS} VERBATIM
  COMMENT "makeimg: building CE ROM (NK.bin)")

# Wrap NK.bin into the bootable 0winceos.bin (plants the Flycast SH-4 MMU magic).
add_custom_command(OUTPUT ${WINCEOS}
  COMMAND powershell -NoProfile -ExecutionPolicy Bypass -File ${REPO}/toolchain/wrap-image.ps1
          -NkBin ${NKBIN} -Out ${WINCEOS} -DcSdk ${SDK}
  DEPENDS ${NKBIN} VERBATIM
  COMMENT "wrap-image: NK.bin -> 0winceos.bin")

add_custom_target(image DEPENDS ${WINCEOS})

# Build the Flycast/GDEMU-loadable GDI (Half-Life DC pipeline: buildgdi + ip.bin).
add_custom_target(gdi
  COMMAND powershell -NoProfile -ExecutionPolicy Bypass -File ${REPO}/toolchain/make-gdi.ps1
          -Image ${WINCEOS} -OutDir ${DISCDIR}
  DEPENDS ${WINCEOS}
  COMMENT "make-gdi: 0winceos.bin -> disc.gdi"
  USES_TERMINAL)
