/*++

Copyright (c) 1996-2000 Microsoft Corporation.  All rights reserved.

Module Name: patcher.h

++*/

#ifndef PATCHER_H
#define PATCHER_H


/* Defines used to generate API calls to the ROM Patcher system handle */
#define PATCHER_CALL(type, api, args) \
	IMPLICIT_DECL(type, SH_PATCHER, api, args)

#define PatchExe PATCHER_CALL(BOOL, 2, (PPROCESS pproc, LPWSTR lpwsName))
#define PatchDll PATCHER_CALL(BOOL, 3, (PPROCESS pproc, PMODULE pmod, LPWSTR lpwsName))
#define FreeDllPatch PATCHER_CALL(void, 4, (PPROCESS pproc, PMODULE pmod))

#endif

