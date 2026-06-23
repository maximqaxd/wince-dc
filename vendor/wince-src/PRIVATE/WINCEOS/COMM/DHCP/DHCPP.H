/*****************************************************************************/
/**								Microsoft Windows							**/
/**  Copyright (c) 1997-2000 Microsoft Corporation.  All rights reserved.   **/
/*****************************************************************************/

/*
	dhcpp.h

  DESCRIPTION:
	private dhcp.h


*/

#ifndef _DHCPP_H_
#define _DHCPP_H_

//
//  System include files.
//

#include <windows.h>
#include <types.h>

// DEBUG ZONES
#ifdef DEBUG
#define ZONE_INIT		DEBUGZONE(0)
#define ZONE_TIMER		DEBUGZONE(1)
#define ZONE_AUTOIP 	DEBUGZONE(2)
#define ZONE_UNUSED3	DEBUGZONE(3)
#define ZONE_RECV		DEBUGZONE(4)
#define ZONE_SEND		DEBUGZONE(5)
#define ZONE_UNUSED6	DEBUGZONE(6)
#define ZONE_UNUSED7	DEBUGZONE(7)
#define ZONE_UNUSED8	DEBUGZONE(8)
#define ZONE_BUFFER		DEBUGZONE(9)
#define ZONE_INTERFACE	DEBUGZONE(10)
#define ZONE_MISC		DEBUGZONE(11)
#define ZONE_ALLOC		DEBUGZONE(12)
#define ZONE_FUNCTION	DEBUGZONE(13)
#define ZONE_WARN		DEBUGZONE(14)
#define ZONE_ERROR		DEBUGZONE(15)
#endif

#include <types.h>
#include <winsock.h>
#include <memory.h>
#include "ptype.h"

typedef unsigned char UCHAR, *PUCHAR;
typedef unsigned short USHORT, ushort;
typedef unsigned int uint;
typedef void *PVOID;

#define PUNICODE_STRING PWSTR

#include <string.h>
#include <stdarg.h>
#include "ndis.h"

#include <cxport.h>

#include "tdi.h"
#include <tdistat.h>
#include <tdipeg.h>
#include <ipexport.h>
#include <tdiinfo.h>
#include <tcpinfo.h>

#include "type.h"

#include "linklist.h"
#include "wsock.h"
#include "wstype.h"

#include "helper.h"	// should be in comm\inc
#include "netbios.h"

//
//  Project include files.
//

#undef BEGIN_INIT_DATA
#undef END_INIT_DATA

//
//  Local include files.
//

//#include "const.h"


typedef int STATUS;

#ifndef SUCCESS
#define SUCCESS	0
#endif

#ifndef net_short
#define net_short(x) ((((x)&0xff) << 8) | (((x)&0xff00) >> 8))

#define net_long(x) (((((ulong)(x))&0xffL)<<24) | \
                     ((((ulong)(x))&0xff00L)<<8) | \
                     ((((ulong)(x))&0xff0000L)>>8) | \
                     ((((ulong)(x))&0xff000000L)>>24))
#endif // net_short


#ifdef MEMTRACKING
// Memory types for AddTrackedItem.  Do a MAKELONG(AFDMEM_BUFFER,Info)
// for first user DWORD.
#define	AFDMEM_BUFFER	0
#define AFDMEM_SOCKET	1
#define AFDMEM_QUEUE	2
#define AFDMEM_CONN		3
#define AFDMEM_ENDP		4
#define AFDMEM_TRACK	5
#define AFDMEM_ACCEPTC	6
#define AFDMEM_STACK	7
#define AFDMEM_STACKMAP	8
#endif

#define MAX_REG_STR				128
#define MAX_DHCP_REQ_OPTIONS	32
#define MAX_DOMAIN_NAME			51	// -1 b/c first byte is length

#define DHCP_SERVER_PORT	67
#define DHCP_CLIENT_PORT	68

//
// Registry configurable options (DhcpInfo->Flags)
//
#define DHCP_ENABLED_FL		0x0001
#define USER_OPTIONS_FL		0x0002
#define OPTION_CHANGE_FL	0x0004
#define SAVE_OPTIONS_FL		0x0008
#define NO_MAC_COMPARE_FL	0x0010
#define SEND_OPTIONS_FL     0x0020
#define AUTO_IP_ENABLED_FL  0x0040

//
// Operational state flags (DhcpInfo->SFlags)
//
#define DHCPSTATE_T1_EXPIRE         0x0001
#define DHCPSTATE_T2_EXPIRE         0x0002
#define DHCPSTATE_LEASE_EXPIRE      0x0004
#define DHCPSTATE_SAW_DHCP_SRV      0x0008  // There is a DHCP server on the net, don't autoconfigure the IP address
#define DHCPSTATE_AUTO_CFG_IP       0x0010  // IP address was auto configured.
#define DHCPSTATE_RELEASE_REQUEST   0x0020  // User has requested the lease be let go.
#define DHCPSTATE_DIALOGUE_BOX      0x0040
#define DHCPSTATE_DHCPNTE_SET		0x0080	// we have set the dhcpnte...

#define ETH_ADDR_LEN 6

//
// IP Autoconfiguration defaults
//
#define DHCP_IPAUTOCONFIGURATION_DEFAULT_SUBNET  0x0000FEA9     //"169.254.0.0"
#define DHCP_IPAUTOCONFIGURATION_DEFAULT_MASK    0x0000FFFF     //"255.255.0.0"
#define DHPC_IPAUTOCONFIG_DEFAULT_INTERVAL       300            // 5 minutes in seconds

// define the reserved range of autonet addresses..
#define DHCP_RESERVED_AUTOCFG_SUBNET             0x00FFFEA9     //"169.254.255.0"
#define DHCP_RESERVED_AUTOCFG_MASK               0x00FFFFFF     //"255.255.255.0"

#define ARP_RESPONSE_WAIT   5000    // ms to wait for ARP response indicating collision

#define TEN_M	(10000000)	// 10 million (to convert to seconds in a FILETIME)

#pragma warning(disable:4200) // nonstandard extensions warning

typedef struct DhcpInfo {
	struct DhcpInfo *pNext;
	LPSOCK_INFO		Socket;	// the socket handle
	void		*Nte;
	unsigned	NteContext;
	ushort		Flags;      // configuration settings (*_*_FL defines)
    uint        SFlags; // operational state flags (DHCPSTATE_* defines)
	char		PhysAddr[ETH_ADDR_LEN];
	uint		IPAddr;
	uint		SubnetMask;
	uint		Gateway;		// IP addr of the default gateway
	uint		DNS[2];
	uint		DhcpServer;
	uint		WinsServer[2];	// IP addrs of primary & ssecondary WINS servers
	FILETIME	LeaseObtained;	// in 100 micro-secs
	int			T1;		// these are in sec's
	int			T2;
	int			Lease;	// total length of lease
	uint		cRetryDialogue;
	uint		cMaxRetry;		// times to retry INIT phase
	int			InitDelay;		// in ms
	DEFINE_LOCK_STRUCTURE(Lock)
	CTEFTimer	Timer;
	uchar		ReqOptions[MAX_DHCP_REQ_OPTIONS];
	uchar		*pSendOptions;
	int			cSendOptions;
	char		aDomainName[MAX_DOMAIN_NAME+1];
    uint        AutoIPAddr;
    uint        AutoSubnet;
    uint        AutoMask;
    uint        AutoSeed;
    uint        AutoInterval;   // Interval at which to check for a DHCP server
    HANDLE      ARPEvent;
    uint        ARPResult;
	TCHAR		Name[];
} DhcpInfo;

#pragma warning(default:4200) // nonstandard extensions warning

typedef int STATUS;

// dhcp.c
void mul64_32_64(const FILETIME *lpnum1, DWORD num2, LPFILETIME lpres);
void add64_32_64(const FILETIME *lpnum1, DWORD num2, LPFILETIME lpres);
void add64_64_64(const FILETIME *lpnum1, LPFILETIME lpnum2, LPFILETIME lpres);
void sub64_64_64(const FILETIME *lpnum1, LPFILETIME lpnum2, LPFILETIME lpres);
void div64_32_64(const FILETIME *lpdividend, DWORD divisor, LPFILETIME lpresult);
unsigned int GetXid(char * pMacAddr);
LPTSTR AddrToString(DWORD Addr, TCHAR *pString);
STATUS DhcpInitSock(DhcpInfo *pInfo, int SrcIP);
void TakeNetDown(DhcpInfo *pDhcp, BOOL fDiscardIPAddr, BOOL fRemoveAfdInterface);
BOOL SetDHCPNTE(DhcpInfo * pDhcp);
BOOL ClearDHCPNTE(DhcpInfo * pDhcp);
STATUS RequestDHCPAddr(PTSTR pAdapter, void *Nte, unsigned Context, 
					   char *pAddr, unsigned cAddr);
BOOL ReleaseDHCPAddr(PTSTR pAdapter);
void DhcpWorkerThread();
HKEY OpenDHCPKey(DhcpInfo * pDhcp);

// autonet.c
STATUS AutoIP(DhcpInfo	*pDhcp);

#include "globals.h"


#endif	// dhcpp.h
