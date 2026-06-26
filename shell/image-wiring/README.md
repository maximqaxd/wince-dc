# image-wiring — how to wire dcshell into the bootable CE image

The image is built from sources in the **DC SDK tree** (`C:\wcedreamcast\release\<flavor>\`),
which is the one external dependency and is **not** in this repo. Three source files there
must carry the DCWin wiring. `setenv.bat` is already vendored (`toolchain\setenv.bat`); the
other two (`platform.bib`, `gemini.reg`) are SDK-derived, so only the **snippets we add** are
vendored here — apply them on a fresh PC after placing the SDK.

`makeimg` regenerates `ce.bib`/`reginit.ini` from these `IF`-guarded sources every run, so
editing `ce.bib`/`reginit.ini` directly is futile — edit the sources below.

## 1. `platform.bib` — MODULES block

Add the 4 shell exes to the `MODULES` section, right after the `sysstart.exe` line. See
[platform.bib.snippet](platform.bib.snippet) for the exact lines + surrounding context.

```
   sysstart.exe    $(_FLATRELEASEDIR)$(RELEASEDIR_OS)sysstart.exe    NK  SH
   dcshell.exe     $(_FLATRELEASEDIR)$(RELEASEDIR_OS)dcshell.exe     NK  SH
   dcwcalc.exe     $(_FLATRELEASEDIR)$(RELEASEDIR_OS)dcwcalc.exe     NK  SH
   dcwclock.exe    $(_FLATRELEASEDIR)$(RELEASEDIR_OS)dcwclock.exe    NK  SH
   dcwexp.exe      $(_FLATRELEASEDIR)$(RELEASEDIR_OS)dcwexp.exe      NK  SH
   dcwtask.exe     $(_FLATRELEASEDIR)$(RELEASEDIR_OS)dcwtask.exe     NK  SH
IF IMGNOSHELL
   toolhelp.dll    $(_FLATRELEASEDIR)$(RELEASEDIR_OS)toolhelp.dll    NK  SH
ENDIF
```

`toolhelp.dll` (process enumeration for the Task Manager) is normally bundled with
`shell.exe` inside `IF IMGNOSHELL !`. We drop `shell.exe` (IMGNOSHELL=1) but still
need toolhelp, so we re-add it under `IF IMGNOSHELL` — present in both configs,
never duplicated. The Task Manager LoadLibrary()s it (graceful if absent).

## 2. `gemini.reg` — `[HKEY_LOCAL_MACHINE\init]`

`sysstart.exe` reads `"Autorun"` and launches it last. Set it to `dcshell.exe`. See
[gemini.reg.snippet](gemini.reg.snippet).

```
[HKEY_LOCAL_MACHINE\init]
IF CONFIGTOOL !
    "Autorun"="dcshell.exe"
ENDIF
```

## 3. `setenv.bat` — image env flags (already vendored)

In `toolchain\setenv.bat` (tracked in this repo). The flags that matter for DCWin:

```
set IMGNOSHELL=1        :: drop the devkit CESH shell.exe (faults in emulation; talks to DA over P2)
set IMGDIRECTINPUT=1    :: put dinput.dll in the image (needed for the DI keyboard + controller pointer)
set IMGDIRECTDRAW=1     :: ddraw.dll (the display layer)
set IMGNODEBUGGER=1     :: no KdStub
```

## 4. Deploy the built exes + build

```bat
shell\build-dcshell.bat retail
copy /Y reference\shell-obj\dcshell.exe  C:\wcedreamcast\release\retail\OS\
copy /Y reference\shell-obj\dcwcalc.exe  C:\wcedreamcast\release\retail\OS\
copy /Y reference\shell-obj\dcwclock.exe C:\wcedreamcast\release\retail\OS\
copy /Y reference\shell-obj\dcwexp.exe   C:\wcedreamcast\release\retail\OS\
toolchain\build-image.bat retail
powershell -File toolchain\wrap-image.ps1 -NkBin C:\wcedreamcast\release\retail\NK.bin -Out C:\wcedreamcast\release\retail\0winceos.bin
powershell -File toolchain\make-gdi.ps1   -Image C:\wcedreamcast\release\retail\0winceos.bin
```

Do the same under `release\debug\` for a debug image. See `../BUILD.md` for the full loop.

> Only the *added* lines are reproduced here — the full `platform.bib`/`gemini.reg` are SDK
> property and stay in the SDK tree, not the repo.
