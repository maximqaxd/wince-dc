@REM
@REM Copyright (c) Microsoft Corporation.  All rights reserved.
@REM
@REM
@REM Use of this source code is subject to the terms of the Microsoft end-user
@REM license agreement (EULA) under which you licensed this SOFTWARE PRODUCT.
@REM If you did not accept the terms of the EULA, you are not authorized to use
@REM this source code. For a copy of the EULA, please see the LICENSE.RTF on your
@REM install media.
@REM
@echo off
prep /om /fv %1.exe
if errorlevel 1 goto done
profile %1 %2 %3 %4 %5 %6 %7 %8 %9
if errorlevel 1 goto done
prep /m %1
if errorlevel 1 goto done
plist %1
:done
