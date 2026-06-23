# RESUME — set up on a new PC

This repo is self-contained **except** the Dreamcast WinCE SDK (copyrighted; fetched separately).

## Steps
1. **Clone** this repo (it includes the leaked CE 3.0 source and the SH-4 toolchain under
   `vendor/`, ~43 MB).
2. **Get the DC SDK** ("wcedreamcast") and place it at `C:\wcedreamcast`
   (or anywhere — then `set WCEDREAMCASTROOT=<that path>`).
3. **Verify:** `powershell -File toolchain\bootstrap.ps1`
   - downloads the SDK if you pass `-SdkUrl <url>`; otherwise just checks everything is present.
   - all lines should read `OK` and end with `READY.`
4. **Build:**
   ```bat
   cd toolchain
   build-image.bat retail      :: makeimg -> NK.bin  (validated, reproducible)
   build-kernel.bat retail     :: compile leaked NK/SHX  (work in progress)
   ```

## Then resume the actual work
Read, in order: **`CLAUDE.md`** -> **`handoff/SESSION-LOG.md`** -> **`docs/04-kernel-build.md`**.
The current task is the kernel-compile header grind; the exact frontier and method are in
`docs/04-kernel-build.md`. The assistant's project memory is in `handoff/memory/` (you can copy
those `.md` files into a new machine's `~/.claude` project memory if you want them auto-loaded).

## Requirements
- Windows + PowerShell (the SH compiler and `makeimg` are Win32). A Git Bash is handy for `grep`.
- ~50 MB free for the clone; the DC SDK adds tens of MB.
