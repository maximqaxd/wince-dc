/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

Module Name:  

serdbg.h

Abstract:

Serial debugging support

Notes: 


--*/

#ifndef __SERDBG_H__
#define __SERDBG_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef DEBUG
#define ZONE_INIT		DEBUGZONE(0)
#define ZONE_OPEN		DEBUGZONE(1)
#define ZONE_READ		DEBUGZONE(2)
#define ZONE_WRITE		DEBUGZONE(3)
#define ZONE_CLOSE		DEBUGZONE(4)
#define ZONE_IOCTL		DEBUGZONE(5)
#define ZONE_THREAD		DEBUGZONE(6)
#define ZONE_EVENTS		DEBUGZONE(7)
#define ZONE_CRITSEC	DEBUGZONE(8)
#define ZONE_FLOW		DEBUGZONE(9)
#define ZONE_IR			DEBUGZONE(10)
#define ZONE_USR_READ	DEBUGZONE(11)
#define ZONE_ALLOC		DEBUGZONE(12)
#define ZONE_FUNCTION	DEBUGZONE(13)
#define ZONE_WARN		DEBUGZONE(14)
#define ZONE_ERROR		DEBUGZONE(15)

// unofficial zones - The upper 16 zones don't show up with nice
// names in cesh, etc. because we only store mnemonics for the first
// 16 zones in DBGPARAM.  But for convenience, all of my serial drivers
// use the upper 16 bits consistently as defined below.
#define ZONE_RXDATA		DEBUGZONE(16)
#define ZONE_TXDATA		DEBUGZONE(17)

#ifdef __cplusplus
}
#endif


#endif


#endif /* __SERDBG_H__ */
