/*++

Copyright (c) 1997-2000 Microsoft Corporation.  All rights reserved.

Module Name: optimizer.h

--*/

//
//	#define or -D__OPT_DISABLE to disable optimization tuning
//

#ifndef __OPT_DISABLE

#include <bldver.h>

#if __OPT_PLAT_FLAG

//
//	If __OPT_VER_OFF, disable optimization, but only for
//	specified version. We need to revisit optimization
//	structure for the next version.
//
#if defined (__OPT_VER_OFF)

#if __OPT_VER_OFF == (CE_MAJOR_VER * 100 + CE_MINOR_VER)
#pragma optimize ("", off)
#pragma message ("Optimization turned off")
#else
#error Revisit optimization tuning for the next release.
#endif

#undef __OPT_VER_OFF

#endif 	// __OPT_VER_OFF

//
//	If __OPT_VER_RESTORE, restore optimization to default
//	settings, but only for specified version. We need
//	to revisit optimization structure for the next version.
//
#if defined (__OPT_VER_RESTORE)

#if __OPT_VER_RESTORE == (CE_MAJOR_VER * 100 + CE_MINOR_VER)
#pragma optimize ("", on)
#pragma message ("Optimization settings restored")
#else
#error Revisit optimization tuning for the next release.
#endif

#undef __OPT_VER_RESTORE

#endif 	// __OPT_VER_RESTORE

#endif	// __OPT_PLAT_FLAG

#endif // _OPTIMIZER_H_
