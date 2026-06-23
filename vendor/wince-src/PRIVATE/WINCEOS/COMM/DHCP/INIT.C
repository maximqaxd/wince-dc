/*****************************************************************************/
/**								Microsoft Windows							**/
/**   Copyright (c) 1997-2000 Microsoft Corporation.  All rights reserved.  **/
/*****************************************************************************/

/*
	init.c

  DESCRIPTION:
	initialization routines for Dhcp

*/

#include "dhcpp.h"
#include "dhcp.h"
#include "protocol.h"

#ifdef DEBUG
DBGPARAM dpCurSettings = {
    TEXT("DHCP"), {
    TEXT("Init"),   TEXT("Timer"),      TEXT("AutoIP"),     TEXT("Unused"),
    TEXT("Recv"),   TEXT("Send"),       TEXT("Unused"),     TEXT("Unused"),
    TEXT("Unused"), TEXT("Buffer"),     TEXT("Interface"),  TEXT("Misc"),
    TEXT("Alloc"),  TEXT("Function"),   TEXT("Warning"),    TEXT("Error") },
    0xe035
}; 
#endif

PFNVOID *v_apSocketFns, *v_apAfdFns;

PFNSetDHCPNTE	pfnSetDHCPNTE;
PFNIPSetNTEAddr pfnIPSetNTEAddr;

// Handle by which to notify applications of changes to IP data structures.
extern HANDLE	g_hAddrChangeEvent;

extern STATUS DhcpNotify(uint Code, PTSTR pAdapter, void *Nte, 
						 unsigned Context, char *pAddr, unsigned cAddr);


int DhcpRegister(
	PFNSetDHCPNTE		pfnSetNTE,
	PFNIPSetNTEAddr		pfnSetAddr, 
	PFN_DHCP_NOTIFY		*ppDhcpNotify)
{
	BOOL	fStatus = TRUE;

	DEBUGMSG(ZONE_INIT | ZONE_WARN, 
		(TEXT("+DhcpRegister:\r\n")));

	pfnSetDHCPNTE = pfnSetNTE;
	pfnIPSetNTEAddr = pfnSetAddr;
	*ppDhcpNotify = DhcpNotify;

	DEBUGMSG(ZONE_INIT | ZONE_WARN, 
		(TEXT("-DhcpRegister: Ret = %d\r\n"), fStatus));

	return fStatus;
}	// DhcpRegister()


BOOL DllEntry (HANDLE hinstDLL, DWORD Op, PVOID lpvReserved) {
	BOOL Status = TRUE;

	switch (Op) {
	case DLL_PROCESS_ATTACH:
	    DEBUGREGISTER(hinstDLL);
		DEBUGMSG (ZONE_INIT|ZONE_WARN, 
			(TEXT("Dhcp: dllentry() %d\r\n"), hinstDLL));

		break;

	case DLL_PROCESS_DETACH:
		break;

	case DLL_THREAD_DETACH :
		break;

	case DLL_THREAD_ATTACH :
		break;
	
	default :
		break;
	}
	return Status;
}	// dllentry()

DWORD WINAPI
CheckInitAdapters (LPVOID pvarg)
{
	DWORD	Index;
	HKEY	hKey;
	HKEY	hMiniportKey;
	HKEY	hTcpIpKey;
	TCHAR	szBaseKey[128];
	DWORD	dwBaseKeyLen;
	TCHAR	MiniportName[32];
	DWORD	MiniportNameLength;
	TCHAR	szTemp[128];
	DWORD	dwEnableDHCP;
	DWORD	dwIPInterfaceContext;

	// If we don't have a pointer to the function then ignore.
	if (!pfnSetDHCPNTE) {
		DEBUGMSG (ZONE_WARN, (TEXT("CheckInitAdapters: Don't have pfnSetDHCPNTE\r\n")));
		return (DWORD)-1;
	}
	
	_tcscpy (szBaseKey, TEXT("Comm"));
	dwBaseKeyLen = _tcslen(szBaseKey);
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szBaseKey, 0, KEY_READ,
					 &hKey) != ERROR_SUCCESS) {
		DEBUGMSG(ZONE_WARN, (TEXT("CheckInitAdapters: unable to open registry\r\n")));
		return (DWORD)-1;
	}
	for (Index = 0; TRUE; Index++) {
		MiniportNameLength = sizeof(MiniportName) / sizeof(MiniportName[0]);
		if (RegEnumKeyEx(hKey, Index, MiniportName,
						 &MiniportNameLength, NULL, NULL,
						 NULL, NULL) != ERROR_SUCCESS) {
			break;
		}
		DEBUGMSG (ZONE_INIT, (TEXT(" CheckInitAdapters: Found Key #%d '%s'\r\n"),
					  Index, MiniportName));
		

		// Check if this is really an ndis driver.
		if (RegOpenKeyEx (hKey, MiniportName, 0, KEY_READ,
						  &hMiniportKey) != ERROR_SUCCESS) {
			
			continue;
		}

		// Check the group
		if (!GetRegSZValue(hMiniportKey, TEXT("Group"), szTemp, 
						  sizeof(szTemp))) {
			DEBUGMSG (ZONE_INIT, (TEXT(" CheckInitAdapters: Couldn't find Group\r\n")));
			RegCloseKey (hMiniportKey);
			continue;
		}
		if (_tcscmp (szTemp, TEXT("NDIS"))) {
			DEBUGMSG (ZONE_INIT, (TEXT(" CheckInitAdapters: Group doesn't match\r\n")));
			RegCloseKey (hMiniportKey);
			continue;
		}

		if (RegOpenKeyEx (hMiniportKey, TEXT("Parms\\TcpIp"), 0, KEY_READ,
						  &hTcpIpKey) != ERROR_SUCCESS) {
			DEBUGMSG (ZONE_INIT, (TEXT(" CheckInitAdapters: Couldn't open TcpIp\r\n")));
			RegCloseKey (hMiniportKey);
			continue;
		}

		dwEnableDHCP = 0;
		if (!GetRegDWORDValue(hTcpIpKey, TEXT("EnableDHCP"), &dwEnableDHCP) ||
		   !dwEnableDHCP) {
			DEBUGMSG (ZONE_INIT, (TEXT(" CheckInitAdapters: Invalid EnableDHCP value=%d\r\n"),
						  dwEnableDHCP));
			RegCloseKey (hTcpIpKey);
			RegCloseKey (hMiniportKey);
			continue;
		}
		dwIPInterfaceContext = 0xFFFF;
		if (!GetRegDWORDValue(hTcpIpKey, TEXT("IPInterfaceContext"), &dwIPInterfaceContext) ||
		   (dwIPInterfaceContext == 0xFFFF)) {
			DEBUGMSG (ZONE_INIT, (TEXT(" CheckInitAdapters: Invalid IPInterfaceContext\r\n")));
			RegCloseKey (hTcpIpKey);
			RegCloseKey (hMiniportKey);
			continue;
		}

		RETAILMSG (1, (TEXT("Static Interface Context #%d\r\n"),
					   dwIPInterfaceContext));

		RequestDHCPAddr (MiniportName, NULL, dwIPInterfaceContext, NULL, 0);
		
		RegCloseKey (hTcpIpKey);
		RegCloseKey (hMiniportKey);
	}

	RegCloseKey (hKey);
	return 0;
}

	
//	IMPORTANT: The name of this fn must be the same as the HelperName
//	note: the parameters for registration is different: Unused, OpCode, 
//			pVTable, cVTable, pMTable, cMTable, pIndexNum
extern unsigned int v_cXid;

BOOL Dhcp(DWORD Unused, DWORD OpCode, 
			 PBYTE pName, DWORD cBuf1,
			 PBYTE pBuf1, DWORD cBuf2,
			 PDWORD pBuf2) {
	STATUS	Status = DHCP_SUCCESS;
	BOOL	fStatus = TRUE;
	PWSTR	pAdapterName = (PWSTR)pName;
	DWORD	ThreadID;
	HANDLE	hThread;
    unsigned int    cRand, cRand2;
    FILETIME	CurTime;
    __int64 cBigRand;

	switch(OpCode) {
	case DHCP_REGISTER:
		v_apAfdFns = (PFNVOID *)pName;
		v_apSocketFns = (PFNVOID *)pBuf1;
		*pBuf2 = AFD_DHCP_IDX;
		CTEInitLock(&v_GlobalListLock);
		hThread = CreateThread (NULL, 0, CheckInitAdapters, 0, 0, &ThreadID);
		CloseHandle (hThread);

      // Create event by which to signal applications that changes have occurred in the Address Table.
	   // (That is, to support the IP Helper API NotifyAddrChange )
	   g_hAddrChangeEvent = CreateEvent(NULL, TRUE, FALSE, TEXT("IP_ADDR_CHANGE_EVENT"));
      
        GetCurrentFT(&CurTime);
        cRand = (CurTime.dwHighDateTime & 0x788) | (CurTime.dwLowDateTime & 0xf800);
        cBigRand = CeGetRandomSeed();
		cRand2 = (uint)(cBigRand >> 32);
		cRand2 ^= (uint)(cBigRand & 0xffFFffFF);
        v_cXid = cRand ^ cRand2;
        DEBUGMSG(ZONE_INIT, (TEXT("\tDhcp:Register: FT %d, Xid %d\r\n"),
            cRand, v_cXid));
		return fStatus;

	case DHCP_PROBE:
		return TRUE;
		break;

	default:
		fStatus = FALSE;
		break;
	}
	
	if (Status != DHCP_SUCCESS) {
		SetLastError(Status);
		fStatus = FALSE;
	}

	return fStatus;
}	// Dhcp()


