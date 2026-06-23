/*****************************************************************************/
/**								Microsoft Windows							**/
/**Copyright (c) 1997-2000 Microsoft Corporation.  All rights reserved.     **/
/*****************************************************************************/

/*
	dhcp.h

  DESCRIPTION:


*/

#ifndef _DHCP_H_
#define _DHCP_H_

#ifndef FAR
#define FAR
#endif

#include "ipexport.h"

typedef unsigned int (*PFNSetDHCPNTE)(unsigned Context, void **ppNTE,
									  char *pAddr, DWORD *cAddr);
typedef unsigned int (*PFNIPSetNTEAddr)(unsigned short Context, 
										void *NTEp, IPAddr Addr, IPMask Mask, 
										IPAddr GWAddr);

typedef DWORD (*PFN_DHCP_NOTIFY)(uint Code, PTSTR pAdapter, void *Nte, unsigned Context,
								   char *pAddr, unsigned cAddr);

typedef int  (*PFNDhcpRegister)(
	PFNSetDHCPNTE		pfnSetNTE,
	PFNIPSetNTEAddr		pfnSetAddr, 
	PFN_DHCP_NOTIFY		*ppDhcpNotify);

extern int DhcpRegister(
	PFNSetDHCPNTE		pfnSetNTE,
	PFNIPSetNTEAddr		pfnSetAddr, 
	PFN_DHCP_NOTIFY		*ppDhcpNotify);


// Network Control Block structure



//	Return Codes for Dhcp
#define	DHCP_SUCCESS		0
#define DHCP_FAILURE		1
#define DHCP_NOMEM			2
#define DHCP_NACK			3
#define DHCP_NOCARD			4
#define DHCP_COLLISION		5
#define DHCP_NOINTERFACE	6	// specified interface doesn't exist


//	OpCodes for the Dhcp Function

#define	DHCP_REGISTER			0x01	// this is a must for all Helper funcs
#define DHCP_PROBE				0x02	// reserved for all helper funcs

// note; need a notification mechanism

#define DHCP_RENEW				0x08
#define DHCP_RELEASE			0x09


//	Codes for PFN_DHCP_NOTIFY
#define DHCP_NOTIFY_ADD_INTERFACE		0x0001	// unused
#define DHCP_NOTIFY_DEL_INTERFACE		0x0002

#define DHCP_REQUEST_ADDRESS			0x1001
#define DHCP_REQUEST_RELEASE			0x1002
#define DHCP_REQUEST_RENEW				0x1003


#endif	// _DHCP_H_
