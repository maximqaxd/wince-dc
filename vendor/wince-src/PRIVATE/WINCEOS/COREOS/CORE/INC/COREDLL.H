// --------------------------------------------------------------------------
//
// Copyright (c) 1994-2000 Microsoft Corporation.  All rights reserved.
//
// Module:
//
//     coredll.h
//
// Purpose:
//
//    Internal include file used by the core dll.
//
// --------------------------------------------------------------------------

#ifndef __COREDLL_H__
#define __COREDLL_H__


#ifdef DEBUG
#define DBGFIXHP    DEBUGZONE(0)
#define DBGLMEM     DEBUGZONE(1)
#define DBGMOV      DEBUGZONE(2)
#define DBGSBLK     DEBUGZONE(3)
#define DBGVIRTMEM  DEBUGZONE(4)
#define DBGDEVICE   DEBUGZONE(5)
#define DBGSTDIO    DEBUGZONE(8)
#define DBGSTDIOHF  DEBUGZONE(9)
#define DBGSHELL    DEBUGZONE(10)
#define DBGIMM		DEBUGZONE(11)
#endif

#ifdef __cplusplus
extern "C" {
#endif

BOOL WINAPI LMemInit(VOID);
extern HANDLE hInstCoreDll;

#ifdef __cplusplus
}
#endif

#endif /* __COREDLL_H__ */

