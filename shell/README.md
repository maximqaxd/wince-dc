# dcshell — a desktop shell/launcher for Dreamcast Windows CE 2.12

The piece the SDK doesn't have: there's **no `explorer.exe`** anywhere (SDK, leak, or any
PB), and GWES/shell source was never released — so the shell is the one component you **write**,
not acquire (see `../docs/06`/`../docs/08` and the NT4/PB analyses). It's the *easy* part: the
stock closed 2.12 already has **multitasking** (the kernel is preemptive — it boots
gwes+shell+wdevice+microstk+sysstart concurrently) and a **full GUI/GDI API** (gwes.exe +
`coredll` exports `CreateWindowExW`/`GetMessageW`/`BitBlt`/`ExtTextOutW`/`CreateProcessW`/
`FindFirstFileW`…). `dcshell` is just an ordinary CE GUI app on top.

## What it is (v1)
Full-screen launcher: lists `*.exe` in `\Windows` and `\`, Up/Down (or tap) to select,
Enter/tap to `CreateProcess` it (= multitasking), with a title + live clock.

## The DC GDI subset (important)
The stock `coredll` GDI is **text+blit only** — present: `ExtTextOutW`, `CreateFontIndirectW`,
`BitBlt`, `GetStockObject`, `SelectObject`, `SetBkColor`/`SetTextColor`, the `RECT` helpers,
`BeginPaint`. **Absent:** `FillRect`, `Rectangle`, `CreatePen`, `CreateSolidBrush`, `DrawText`,
`TextOut`, `Polygon`, `Ellipse`, `LineTo`. So `dcshell` fills rectangles via the standard
`ExtTextOutW(..., ETO_OPAQUE, &rect, L"", 0, ...)` idiom (paints `rect` with the bg color) and
does all text with `ExtTextOutW`. Stick to that palette when extending it.

## Build
```
shell\build-dcshell.bat        :: -> reference\shell-obj\dcshell.exe  (SH-4 0x1A6, subsystem 2.12)
```
Uses the vendored gweslab SH compiler + the **DC SDK** `inc`/`lib` (apps use the 2.12 SDK API,
not the CE3 leak headers). Links `coredll.lib` + `corelibc.lib` (the latter has
`WinMainCRTStartup`). Verified: links clean, 0 unresolved.

## Wire it into a bootable image
Test on the **stock kernel** (it boots fully today; our from-source `nk.exe` is still mid-bring-up).
First restore the stock kernel if needed: copy `C:\wcedreamcast\release\retail\nknodbg.exe.stock`
back over `nknodbg.exe`. Then:
1. Copy `reference\shell-obj\dcshell.exe` → `C:\wcedreamcast\release\retail\OS\dcshell.exe`.
2. Add a MODULES line to a `.bib` (e.g. `release\retail\gemini.bib`):
   ```
   dcshell.exe   $(_FLATRELEASEDIR)$(RELEASEDIR_OS)dcshell.exe   NK  SH
   ```
3. Launch it at boot — add to `[HKEY_LOCAL_MACHINE\init]` in `release\retail\gemini.reg`:
   ```
   "Launch90"="dcshell.exe"
   "Depend90"=hex:1e,00          ; depend on gwes (0x1e=30) so windowing is up first
   ```
   (Or replace the title that `sysstart` launches, to make dcshell the foreground app.)
4. `toolchain\build-image.bat retail` → `wrap-image.ps1` → `make-gdi.ps1` → load
   `reference\disc-gdi\disc.gdi` in Flycast (SerialConsole on).

## Reference
NT4 explorer source (sparse-cloned, gitignored) at
`vendor\nt4-ref\private\windows\shell\cabinet\` (`cabwnd.c`, `cabinet.c`, `base\`) — **read for
design** (taskbar/desktop/file-view logic) only; it can't be ported (full Win32/COM/shell32
namespace vs the CE subset — see `../docs/08`-adjacent analysis).

## Roadmap
v1 = launcher (done, builds). Next: icon grid + `BitBlt` icons, a window-list taskbar via
`EnumWindows`/`SetForegroundWindow`, a simple file browser (`FindFirstFile` tree), wallpaper via
`BitBlt`. All within the GDI subset above.
