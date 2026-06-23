/*		Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved. */
#include "kernel.h"

#ifdef DEBUG

#ifndef DP_SETTINGS
  /* To override this put: set DP_SETTINGS in your environment */
  #define DP_SETTINGS   0x100
#endif

DBGPARAM dpCurSettings = {
	TEXT("NK Kernel"), {
	TEXT("Schedule"),   TEXT("Memory"),    TEXT("ObjDisp"),   TEXT("Debugger"),
	TEXT("NextThread"), TEXT("Loader"),    TEXT("VirtMem"),   TEXT("Loader2"),
	TEXT("ThreadIDs"),  TEXT("GetInfo"),   TEXT("PhysMem"),   TEXT("SEH"),
	TEXT("OpenExe"),    TEXT("MemTracker"), TEXT("Paging"), TEXT("APIEntry") },
	DP_SETTINGS    // Turn on bit 0 for schedule, 1 for memory, etc...
//	0xdde6		   // useful for when things don't even boot
//	0xc934		   // useful for when things boot somewhat
//	0x4100
};

#endif

