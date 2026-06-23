# wince-dc — Windows CE 3.0 on the Sega Dreamcast (Path B)

Research project: build a **fuller Windows CE** for the Dreamcast from the leaked CE source
+ the Sega "Dragon" SDK's own image tools + a real SH-4 compiler — going beyond the stripped
CE 2.12 game runtime (toward a shell / multitasking desktop).

> Self-contained project dir. Sibling dirs in `C:\dev\Dreamcast` (ReactOS `sh4pe-toolchain`,
> `DreamShell`, `KallistiOS`, `img4dc`) are **separate** efforts — not part of this repo.

## Status (2026-06-23)
- ✅ **Toolchain validated** — two SH-4 PE compilers work on this host:
  gweslab `cl.exe` (Renesas SH, **defaults to SH-4 / `0x1A6`**) and the authentic
  wce212 `SHCL.EXE` (Hitachi SH, the 2.12 vintage).
- ✅ **Image pipeline validated end-to-end** — `makeimg` reproduces the shipped CE ROM
  (29 modules); `wrap-image.ps1` rebuilds the bootable `0winceos.bin` with a
  **byte-identical header** and exact size. No Platform Builder, no CD key, no extraction.
- ⏭️ **Next** — compile the leaked CE 3.0 `NK` (SHX) kernel and swap it into the image.

## Layout
| path | what |
|------|------|
| `toolchain/` | `setenv.bat`, `build-image.bat`, `wrap-image.ps1`, `unwrap-image.ps1`, `build-kernel.bat`, README |
| `docs/` | `01-findings.md`, `02-toolchain-setup.md`, `03-build-pipeline.md` |
| `bsp/` | Dreamcast BSP scaffold (oal/ drivers/ inc/ files/ cesysgen/) — for the kernel-swap phase |
| `reference/` | build artifacts + `MANIFEST.md` (binaries gitignored) |
| `loader/` | placeholder (DC bootstrap / dc-load / KOS IPL) |

Vendored **in the repo** (`vendor/`): `wince-src/` (leaked CE 3.0 source) and `sh-toolchain/`
(SH-4 compiler + CE3 headers). The only external dependency is the **DC SDK** at
`C:\wcedreamcast` (set `WCEDREAMCASTROOT` to relocate) — see `RESUME.md`.

Resuming on another machine? Start with `RESUME.md`, then `CLAUDE.md` and `handoff/SESSION-LOG.md`.

## Quickstart
```bat
cd wince-dc\toolchain
build-image.bat retail
powershell -File wrap-image.ps1 ^
  -NkBin C:\wcedreamcast\release\retail\NK.bin ^
  -Out   C:\wcedreamcast\release\retail\0winceos.bin
```
Then master to GD-R (SDK GD Workshop / `img4dc`) or load via `dc-load`/BBA.

See `docs/03-build-pipeline.md` for the validated chain and `toolchain/README.md` for details.
