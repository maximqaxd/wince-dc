# ROADMAP.md — DC shell / DCWin

Status as of 2026-06-26. The shell is a real **windowed, multitasking desktop** on the
Dreamcast (something CE never shipped here): a shell-as-compositor over a shared-memory
draw-command protocol. See `HANDOFF.md` for architecture, `BUILD.md` to build/test.

## Done ✅

- **DDraw display layer** (`dcgfx`): fullscreen 640×480×16 primary + persistent VRAM
  back buffer presented every frame (primary is volatile). Fills via COLORFILL Blt,
  bevels, Arial GDI fonts on a locked DC, 16×16 + 32×32 color-keyed icons built by
  DDraw Lock. `GfxLaunch` releases the exclusive primary to hand the display to a
  fullscreen app and reclaims it on exit.
- **DCWin compositor** (`dcshell`): named shared section, up to 4 windows, per-window
  draw-command lists with a gen seqlock, per-window input ring. Per-layer compositing
  (desktop → windows back-to-front → taskbar/Start → cursor). Render-on-change loop.
- **Client library + apps** (`dcwlib` + `dcwcalc`/`dcwclock`/`dcwexp`): Calculator,
  Clock, and a windowed **Explorer** (file browser; launches exes via a shell exec
  request). Separate processes, composited as windows.
- **Desktop UX**: NT4-ish chrome, desktop icon grid (My Dreamcast swirl, Explorer,
  Calculator, Clock), Start menu, taskbar window list with icons, Tab focus-cycle,
  file-type icons.
- **Launch from CD-ROM**: `HALFLIFE_DC.EXE` launches + renders fullscreen from `\CD-ROM`
  via display hand-off (crashes at level load — see Open).
- **Task Manager** (`dcwtask`, NEW): a DCWin client listing real CE processes via
  toolhelp (re-added `toolhelp.dll` to the image without the broken devkit
  `shell.exe`), free RAM via `GlobalMemoryStatus`, end-task via
  `OpenProcess`+`TerminateProcess` (criticals protected, SEH-guarded). The shell
  now reaps windows whose owner process died (OpenProcess liveness), so end-task
  on a windowed app doesn't leave a ghost.
- **DirectInput input** (`dcinput`): polled keyboard
  (replaces laggy WM_KEYDOWN; edge-detect + auto-repeat for nav keys) and the DC
  controller as a software pointer (analog stick → cursor, A/B/X/Y → click).
  `HandleClick` hit-tests Start menu / Start button / taskbar buttons / window
  close-box + title / desktop icons.

## Next ⏭ (priority order)

1. **Verify Task Manager + reaper on HW** (immediate). Load the disc, open Task Manager
   (desktop icon / Start). Confirm it lists processes (`DCWTASK:` logs if toolhelp fails),
   shows free RAM, and Del/Enter ends a non-critical task. Confirm
   `DCSHELL: dead-window reaper active` and that ending a windowed app (e.g. dcwclock)
   removes its window with no ghost.
2. **HL level-load crash**. Hypothesis: low heap. Before fullscreen hand-off, close all
   client windows (free their processes) so HL gets max RAM. The Task Manager / reaper
   work now gives the pieces to do this. Verify `\CD-ROM` paths. (`Exception 040`, wild
   TEA — looks like OOM, not a code bug.)
3. **Window move (drag)**. Title-bar drag → update `DcWindow.x/y`. We now have a pointer,
   so this is mostly hit-test + delta in the shell; clients already publish absolute coords.
4. **Window resize**. Border/corner grab; clients re-layout on size change (they already
   read `w/h` from the shared window).
5. **Text viewer app** (more `dcw*` clients; cheap, exercise the API).
6. **Nicer native 32×32 icon art** (current art is ASCII in `s_iconArt`); a distinct
   Task Manager icon (reuses `ICON_APP` today).
7. **Docs**: fold `CLAUDE-PATCH.md` into the top-level `CLAUDE.md`; the old `README.md`
   describes v1 (launcher) + a wrong GDI-subset model — rewrite or supersede it.

## Open issues / risks

- DCWin caps: 4 windows, 48 cmds/window, 16 input events/window (`dcwin.h`). Raise if apps
  need more, but the shared struct grows.
- No cross-process events (CE `SetEvent` absent here; kernel uses `EventModify`) — IPC is
  poll-based by design. Keep it that way.
- Phantom GDI exports remain a trap: never call FillRect/Rectangle/SetPixel/brush/pen/
  Create*Bitmap; SEH-probe any new GDI call before relying on it.
- `make-gdi.ps1` / `wrap-image.ps1` were authored for an old repo path — `make-gdi`
  defaults are fixed; double-check others if a script can't find a tool.
