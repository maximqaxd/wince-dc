/*****************************************************************************/
/**								Microsoft Windows							**/
/**   Copyright (c) 1997-2000 Microsoft Corporation.  All rights reserved.  **/
/*****************************************************************************/

/*
	globals.h

  DESCRIPTION:
	global Dhcp data


*/

#ifndef _GLOBALS_H_
#define _GLOBALS_H_

#include "dhcp.h"


extern PFNVOID *v_apSocketFns;
extern PFNVOID *v_apAfdFns;

EXTERNAL_LOCK(v_GlobalListLock)
extern DhcpInfo	*v_pWaitingList;
extern DhcpInfo	*v_pEstablishedList;
extern HANDLE v_hListEvent;

extern PFNSetDHCPNTE	pfnSetDHCPNTE;
extern PFNIPSetNTEAddr pfnIPSetNTEAddr;

#endif	// _GLOBALS_H_


