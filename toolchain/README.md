# Build — WinCE-on-Dreamcast

The whole build is driven by **CMake** at the repo root (`CMakeLists.txt`). Everything it
needs is vendored under `vendor/` — the SH-4 compiler (`vendor/sh-toolchain`) and the Sega
"Dragon" CE 2.12 SDK (`vendor/wcesdk`: headers, libs, image tools, the OS modules + the
patched kernels). No external SDK, no `C:\wcedreamcast`, no `setenv.bat`.

CMake's own language model is bypassed (`project(... NONE)`) because the 1999 Renesas-SH
`cl.exe` isn't a CMake-known compiler; each step is an explicit custom command driving the
vendored `cl.exe` / `shasm.exe` / `link.exe` and the `makeimg` image pipeline. Toolchain
DLLs are found by prepending the toolchain bins to `PATH` per-command (`cmake -E env
--modify path_list_prepend`).

## Prerequisites
- **CMake ≥ 3.20 + a generator.** The VS-bundled pair works out of the box:
  `…\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe`
  and the Ninja next to it. No compiler toolchain from VS is used — only cmake+ninja.

## Configure + build
```sh
cmake -G Ninja -DCMAKE_MAKE_PROGRAM=<path-to-ninja> -S . -B build
cmake --build build            # builds all SH-4 modules (DLLs + EXEs) into build/modules/
```
Targets:
| target | output | what |
|--------|--------|------|
| (default / `all`) | `build/modules/*.dll`, `*.exe` | `dcspi.dll`, `mppp.dll`, `dcshell.exe`, the 6 `dcw*` apps |
| `image` | `build/0winceos.bin` | seed image work-dir → deploy our modules into `OS\` → `makeimg` → `NK.bin` → `wrap-image.ps1` |
| `gdi`   | `build/disc/disc.gdi` | `image` → `make-gdi.ps1` (Half-Life DC pipeline) — load this in Flycast |

```sh
cmake --build build --target gdi     # full chain: modules -> NK.bin -> 0winceos.bin -> disc.gdi
```

## How the image step works
`makeimg`/`romimage` are 1999 tools with three sharp edges the CMake build handles:
- **Native paths only** — forward-slash paths get mangled in the path join, so the env
  values (`_FLATRELEASEDIR`, `WCEDREAMCASTROOT`) are converted with `file(TO_NATIVE_PATH)`.
- **No trailing backslash in env values** — a trailing `\` escapes the closing quote on the
  command line and blanks the var, collapsing every module path to flat. `RELEASEDIR_*`
  therefore use forward slashes (`/OS/`); Win32 accepts the mixed separator.
- **Kernel `.map` is required** — `romimage` reads `nknodbg.map` (same basename as the
  kernel) to locate `pTOC`; both `nknodbg.map` and `nkscifkd.map` are vendored next to the
  kernels in `vendor/wcesdk/image/`.

The read-only `vendor/wcesdk/image` tree is seeded into `build/image` (writable) and our
freshly-built modules overlay `build/image/OS/` before `makeimg` runs. To change what's in
the image, edit the vendored `vendor/wcesdk/image/*.bib` / `*.reg`.

## Helper scripts (invoked by CMake; runnable standalone)
| script | does |
|--------|------|
| `wrap-image.ps1 -NkBin .. -Out .. -DcSdk vendor\wcesdk` | `NK.bin` → bootable `0winceos.bin` (DUMPNK + 0x800 Sega header + Flycast SH-4 MMU magic) |
| `make-gdi.ps1 -Image .. -OutDir ..` | `0winceos.bin` → Flycast-loadable `disc.gdi` |
| `make-gdi-real.ps1` | rebuild against a real 4x4 Evo GDI → GDEMU-bootable disc |
| `unwrap-image.ps1 -In .. -Out ..` | `0winceos.bin` → raw memory image (inspect) |
| `make-disc.ps1` | alternative CDI path (mkisofs + cdi4dc) |
| `bootstrap.ps1` | sanity-check the vendored toolchain + SDK are present |

See `../docs/03-build-pipeline.md` for the validated chain and the wrapper format.
