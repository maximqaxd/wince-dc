/*****************************************************************************/
/**								Microsoft Windows							**/
/**   Copyright (c) 1997-2000 Microsoft Corporation.  All rights reserved.  **/
/*****************************************************************************/

/*
	dhcp.c

  DESCRIPTION:



*/

#include "dhcpp.h"
#include "dhcp.h"
#include "protocol.h"
#include "netui.h"

BOOL		v_fWorkerThread;
DhcpInfo	*v_pWaitingList;
DhcpInfo	*v_pEstablishedList;
DhcpInfo	*v_pCurrent;
DhcpInfo	*v_pDelete;
DEFINE_LOCK_STRUCTURE(v_GlobalListLock)

#define DEFAULT_MAX_RETRIES		2
#define DEFAULT_RETRY_DIALOGUE	2
#define DEFAULT_INIT_DELAY		10000		// 10 secs

// local externs
extern void ProcessT1(DhcpInfo *pDhcp);
extern void ProcessT2(DhcpInfo *pDhcp);

void mul64_32_64(const FILETIME *lpnum1, DWORD num2, LPFILETIME lpres) {
	__int64 num1;
	num1 = (__int64)lpnum1->dwLowDateTime * (__int64)num2;
	num1 += ((__int64)lpnum1->dwHighDateTime * (__int64)num2)<<32;
	lpres->dwHighDateTime = (DWORD)(num1>>32);
	lpres->dwLowDateTime = (DWORD)(num1&0xffffffff);
}

void add64_32_64(const FILETIME *lpnum1, DWORD num2, LPFILETIME lpres) {
	DWORD bottom = lpnum1->dwLowDateTime + num2;
	lpres->dwHighDateTime = lpnum1->dwHighDateTime +
		(bottom < lpnum1->dwLowDateTime ? 1 : 0);
	lpres->dwLowDateTime = bottom;
}

#ifdef THUMB

#pragma optimize("",off)
#endif
void add64_64_64(const FILETIME *lpnum1, LPFILETIME lpnum2, LPFILETIME lpres) {
	__int64 num1, num2;
	num1 = (((__int64)lpnum1->dwHighDateTime)<<32)+(__int64)lpnum1->dwLowDateTime;
	num2 = (((__int64)lpnum2->dwHighDateTime)<<32)+(__int64)lpnum2->dwLowDateTime;
	num1 += num2;
	lpres->dwHighDateTime = (DWORD)(num1>>32);
	lpres->dwLowDateTime = (DWORD)(num1&0xffffffff);
}

void sub64_64_64(const FILETIME *lpnum1, LPFILETIME lpnum2, LPFILETIME lpres) {
	__int64 num1, num2;
	num1 = (((__int64)lpnum1->dwHighDateTime)<<32)+(__int64)lpnum1->dwLowDateTime;
	num2 = (((__int64)lpnum2->dwHighDateTime)<<32)+(__int64)lpnum2->dwLowDateTime;
	num1 -= num2;
	lpres->dwHighDateTime = (DWORD)(num1>>32);
	lpres->dwLowDateTime = (DWORD)(num1&0xffffffff);
}
#ifdef THUMB

#pragma optimize("",on)
#endif

// Unsigned divide
// Divides a 64 bit number by a *31* bit number.  Doesn't work for 32 bit divisors!

void div64_32_64(const FILETIME *lpdividend, DWORD divisor, LPFILETIME lpresult) {
	DWORD bitmask;
	DWORD top;
	FILETIME wholetop = *lpdividend;
	top = 0;
	lpresult->dwHighDateTime = 0;
	for (bitmask = 0x80000000; bitmask; bitmask >>= 1) {
		top = (top<<1) + ((wholetop.dwHighDateTime&bitmask) ? 1 : 0);
		if (top >= divisor) {
			top -= divisor;
			lpresult->dwHighDateTime |= bitmask;
		}
	}
	lpresult->dwLowDateTime = 0;
	for (bitmask = 0x80000000; bitmask; bitmask >>= 1) {
		top = (top<<1) + ((wholetop.dwLowDateTime&bitmask) ? 1 : 0);
		if (top >= divisor) {
			top -= divisor;
			lpresult->dwLowDateTime |= bitmask;
		}
	}
}


//	Len, SubnetMask(1), DomainName(15), Router(3), NB Name Server(44), 
//	NB Node Type(46), NBT Scope (47), DomainServer(6)
unsigned char v_aDftReqOptions[] = 
{7, 0x01, 0x03, 0x06, 0x0f, 0x2c, 0x2e, 0x2f, 0x0};

unsigned int v_cXid;
int	v_fXidInit;

unsigned int GetXid(char *pMacAddr) {
	unsigned int i;

	if (! v_fXidInit) {
		memcpy(&i, pMacAddr + 2, 4);
		v_cXid ^= i;
		v_fXidInit++;
	}

//	CTEGetLock(&v_MiscLock, 0);
	if (i = v_cXid++)
		;
	else	// if it is 0, we increment again!
		i = v_cXid++;
	
//	CTEFreeLock(&v_MiscLock, 0);

	return i;
}	// GetXid()


DhcpInfo *NewDhcpInfo(int cName) {
	DhcpInfo *pDhcp;

	pDhcp = LocalAlloc(LPTR, sizeof(*pDhcp) + cName);
	if (NULL != pDhcp) {
        pDhcp->ARPEvent = CreateEvent(NULL, TRUE, FALSE, NULL); // manual reset
        if (NULL == pDhcp->ARPEvent) {
            DEBUGMSG(ZONE_WARN, (L"DHCP:NewDhcpInfo - CreateEvent failed!\n"));
            LocalFree(pDhcp);
            return NULL;
        }
    
		CTEInitLock(&pDhcp->Lock);
		// technically speaking should be net_long(INADDR_BROADCAST);
		pDhcp->DhcpServer = INADDR_BROADCAST;

        pDhcp->Flags = AUTO_IP_ENABLED_FL;
        pDhcp->AutoInterval = DHPC_IPAUTOCONFIG_DEFAULT_INTERVAL;
        pDhcp->AutoMask = DHCP_IPAUTOCONFIGURATION_DEFAULT_MASK;
        pDhcp->AutoSubnet = DHCP_IPAUTOCONFIGURATION_DEFAULT_SUBNET;
	}
	DEBUGMSG(ZONE_WARN, (TEXT("*NewDhcpInfo: %X\n"), pDhcp));
	return pDhcp;

}	// NewDhcpInfo()


void FreeDhcpInfo(DhcpInfo *pDhcp) {
	DEBUGMSG(ZONE_WARN, (TEXT("*FreeDhcpInfo: %X\n"), pDhcp));
	CTEStopFTimer(&pDhcp->Timer);
	CTEDeleteLock(&pDhcp->Lock);
	if (pDhcp->pSendOptions) {
		LocalFree(pDhcp->pSendOptions);
	}
    if (pDhcp->ARPEvent) {
        CloseHandle(pDhcp->ARPEvent);
    }
	LocalFree(pDhcp);
}	// FreeDhcpInfo()

// assumes pDhcp->Lock is NOT held
// it gets the v_GlobalListLock, tries to find a double-ptr to the dhcp
// and gets its lock
// always returns with v_GlobalListLock held
DhcpInfo **_FindDhcp(DhcpInfo *pDhcp, PTSTR pName) {
	DhcpInfo	**ppTemp;
	
	CTEGetLock(&v_GlobalListLock, 0);

	ppTemp = &v_pEstablishedList;
	for ( ; *ppTemp; ppTemp = &(*ppTemp)->pNext) {
		if (pDhcp) {
			if (*ppTemp == pDhcp)
				break;
		} else if (0 == _tcscmp((*ppTemp)->Name, pName))
			break;
	}	// for()
	if (*ppTemp)
		CTEGetLock(&(*ppTemp)->Lock, 0);

	return ppTemp;

}	// _FindDhcp()


BOOL StringToAddr(TCHAR *AddressString, DWORD *AddressValue) {
	TCHAR	*pStr = AddressString;
	PUCHAR	AddressPtr = (PUCHAR)AddressValue;
	int		i;
	int		Value;

	// Parse the four pieces of the address.
	for (i=0; *pStr && (i < 4); i++) {
		Value = 0;
		while (*pStr && TEXT('.') != *pStr) {
			if ((*pStr < TEXT('0')) || (*pStr > TEXT('9'))) {
				DEBUGMSG (ZONE_INIT,
						  (TEXT("Unable to convert %s to address\r\n"),
						   AddressString));
				return FALSE;
			}
			Value *= 10;
			Value += *pStr - TEXT('0');
			pStr++;
		}
		if (Value > 255) {
			DEBUGMSG (ZONE_INIT,
					  (TEXT("Unable to convert %s to address\r\n"),
					   AddressString));
			return FALSE;
		}
		AddressPtr[i] = Value;
		if (TEXT('.') == *pStr) {
			pStr++;
		}
	}

	// Did we get all of the pieces?
	if (i != 4) {
		DEBUGMSG (ZONE_INIT,
				  (TEXT("Unable to convert %s to address\r\n"),
				   AddressString));
		return FALSE;
	}

	DEBUGMSG (ZONE_INIT,
			  (TEXT("Converted %s to address %X\r\n"),
			   AddressString, *AddressValue));

	return TRUE;
}	// StringToAddr()


LPTSTR AddrToString(DWORD Addr, TCHAR *pString) {
	TCHAR			Buffer[16], *p;
	int				i;
	unsigned char	*aAddr;

	DEBUGMSG(ZONE_WARN, (TEXT("+AddrToString: %X\r\n"),
		Addr));
	
    p = Buffer;
    Addr = net_long(Addr);  // put it in the desired format
	aAddr = (unsigned char *)&Addr;

	for (i = 0; i < 4; i++) {
		do {
			*p++ = aAddr[i] % 10 + TEXT('0');
		} while (aAddr[i] /= 10);
		*p++ = TEXT('.');
	}
	*(--p) = '\0';	// get rid of last '.' & end str

	for (i = 0; i <= 16 && p > Buffer; i++) {
			pString[i] = *(--p);
	}
	pString[i] = TEXT('\0');

	DEBUGMSG(ZONE_WARN, (TEXT("-AddrToString: %s\r\n"), pString));
	return pString;
}	// AddrToString()

// our list is typically < 10 elts so this n^2 algo shld work
void InsSort(int cBuf, uchar *pBuf) {
	uchar	c, *p, *pBig, *pEnd = pBuf + cBuf;

	while ((pBig = pBuf) < pEnd) {
		for (p = pBuf; p < pEnd; p++) {
			if (*p > *pBig)
				pBig = p;
		}
		if (pBig < --pEnd) {
			c = *pBig;
			*pBig = *pEnd;
			*pEnd = c;
		}
	}
}	// InsSort


// returns 0, if not valid #
int Convert(TCHAR *szName, DWORD cName, uchar *pc) {
	uchar c = 0;

	for ( ; cName--  > 0 && *szName != TEXT('\0'); szName++) {
		if (*szName < TEXT('0') || *szName > TEXT('9'))
			return 0;
		c *= 10;
		c += *szName - TEXT('0');
	}
	*pc = c;
	return 1;
}


STATUS GetReqOptions(HKEY hRoot, DhcpInfo *pDhcp) {
	HKEY	hKey;
	LONG	hRes;
	TCHAR	szName[4];
	DWORD	i, j, cName, cBuf, Type;
	uchar	*p, aBuf[OPTIONS_LEN], aPrevReqOptions[MAX_DHCP_REQ_OPTIONS+1];

	pDhcp->Flags &= ~(OPTION_CHANGE_FL | USER_OPTIONS_FL);

	p = aBuf + 1;
	hRes = RegOpenKeyEx(hRoot, TEXT("DhcpOptions"), 0, 0, &hKey);

	if (ERROR_SUCCESS == hRes) {
		cName = sizeof(szName);
		hRes = RegQueryValueEx(hKey, NULL, NULL, NULL, NULL, &cName);
		if (ERROR_SUCCESS == hRes && cName) {
			pDhcp->Flags |= USER_OPTIONS_FL;
			// RegDeleteValue(hKey, NULL);
		
			i = 0;
			while (i < MAX_DHCP_REQ_OPTIONS) {
				cName = sizeof(szName)/sizeof(szName[0]);
				hRes = RegEnumValue(hKey, i++, szName, &cName, NULL, NULL, 
					NULL, NULL);
				if (ERROR_SUCCESS == hRes) {
					if (Convert(szName, 4, p))
						p++;
				} else
					break;
			}	// while()
			
			aBuf[0] = (uchar)(p - aBuf - 1);
			InsSort(aBuf[0], &aBuf[1]);

			memcpy(pDhcp->ReqOptions, aBuf, aBuf[0] + 1);
			
		} else {
			memcpy(pDhcp->ReqOptions, v_aDftReqOptions, 
				v_aDftReqOptions[0] + 1);
		}
		RegCloseKey (hKey);
		
	} else {
		memcpy(pDhcp->ReqOptions, v_aDftReqOptions, v_aDftReqOptions[0] + 1);
	}

	// RFC 2131 requires that if a client requests parameters in a DISCOVER, the 
	// list must be included in subsequent REQUEST messages. So, if our option 
	// list changes, set a flag which will drop us back to INIT state.
	cBuf = sizeof(aPrevReqOptions);
	if (GetRegBinaryValue(hRoot,TEXT("PrevReqOptions"),aPrevReqOptions,&cBuf)) {
		ASSERT(cBuf == (DWORD)(aPrevReqOptions[0] + 1));
		if (memcmp(aPrevReqOptions,pDhcp->ReqOptions,cBuf)) {
			DEBUGMSG(ZONE_WARN,(TEXT("\tGetReqOptions, detected ReqOptions change\r\n")));
			pDhcp->Flags |= OPTION_CHANGE_FL;
		}
	}

	hRes = RegOpenKeyEx(hRoot, TEXT("DhcpSendOptions"), 0, 0, &hKey);
	if (ERROR_SUCCESS == hRes) {
		cName = sizeof(szName);
		hRes = RegQueryValueEx(hKey, NULL, NULL, NULL, NULL, &cName);
		if (ERROR_SUCCESS == hRes && cName) {
			pDhcp->Flags |= SEND_OPTIONS_FL;
			// RegDeleteValue(hKey, NULL);
		
			i = j = 0;
			while (j < OPTIONS_LEN - MAX_DHCP_REQ_OPTIONS - 4 - 3 - 9 - 6) {
				cName = sizeof(szName)/sizeof(szName[0]);
				cBuf = OPTIONS_LEN - j - 22;
				hRes = RegEnumValue(hKey, i++, szName, &cName, NULL, &Type, 
					&aBuf[j], &cBuf);
				if (ERROR_SUCCESS == hRes && REG_BINARY == Type) {
					j += cBuf;
				} else if (ERROR_NO_MORE_ITEMS == hRes)
					break;
			}	// while()

			if (j > 0) {
				if (p = LocalAlloc(LPTR, j)) {
					if (pDhcp->pSendOptions)
						LocalFree(pDhcp->pSendOptions);
					pDhcp->pSendOptions = p;
					memcpy(p, aBuf, j);
					pDhcp->cSendOptions = j;
				}
			} else {
				ASSERT(j == 0);
				// note cSendOptions only valid when pSendOptions != NULL
				LocalFree(pDhcp->pSendOptions);
				pDhcp->pSendOptions = NULL;
			}
		}

		RegCloseKey (hKey);
	}

	return ERROR_SUCCESS;
}	// GetReqOptions


// Read IP address strings from the registry and convert them to DWORDs
BOOL GetRegIPAddr(HKEY hKey, LPTSTR lpVal, LPDWORD lpdwIPAddrs, DWORD dwNumIPAddrs) {
	TCHAR	Buffer[MAX_REG_STR];
    LPTSTR  lpBuf = Buffer;

    Buffer[0] = TEXT('\0');
    // GetRegMultiSZValue works on REG_SZ strings as well as REG_MULTI_SZ
    if (!GetRegMultiSZValue(hKey, lpVal, lpBuf, sizeof(Buffer))) {
        return FALSE;
    }

    while (dwNumIPAddrs) {
        if (TEXT('0') == *lpBuf) {
            *lpdwIPAddrs = 0;
        } else {
            StringToAddr(lpBuf, lpdwIPAddrs);
            lpBuf += _tcslen(lpBuf) + 1;
        }
        lpdwIPAddrs++;
        dwNumIPAddrs--;
    }
    return TRUE;

}   // GetRegIPAddr


HKEY
OpenDHCPKey(
    DhcpInfo * pDhcp
    )
{
	TCHAR	Buffer[MAX_REG_STR];
	HKEY	hKey;
	LONG	hRes;

	_tcscpy (Buffer, COMM_REG_KEY);
	_tcscat (Buffer, pDhcp->Name);
	_tcscat (Buffer, TEXT("\\Parms\\TcpIp"));

	hRes = RegOpenKeyEx (HKEY_LOCAL_MACHINE, Buffer, 0, 0, &hKey);
    if (hRes) {
        return NULL;
    }
    return hKey;
}

STATUS GetDhcpConfig(DhcpInfo *pDhcp) {
	HKEY	hKey;
	BOOL	fStatus;
	uint	fDhcpEnabled;
	int		i;

	DEBUGMSG (ZONE_INIT, (TEXT("+GetDhcpConfig:\r\n")));

	// Open the Registry Key.
	hKey = OpenDHCPKey(pDhcp);
	
	if (hKey) {
		fStatus = GetRegDWORDValue(hKey, TEXT("EnableDHCP"), 
			&fDhcpEnabled);

		if (fStatus && fDhcpEnabled)
			pDhcp->Flags |= DHCP_ENABLED_FL;

		fStatus = GetRegIPAddr(hKey, TEXT("DhcpIPAddress"), &pDhcp->IPAddr, 1); 
		if (fStatus && pDhcp->IPAddr) {
			fStatus = GetRegIPAddr(hKey, TEXT("DhcpSubnetMask"), &pDhcp->SubnetMask, 1);
			if (fStatus && pDhcp->SubnetMask) {
				// Get the DhcpServer
				GetRegIPAddr(hKey, TEXT("DhcpServer"), &pDhcp->DhcpServer, 1);

				// Get the DhcpDftGateway
				GetRegIPAddr(hKey, TEXT("DhcpDefaultGateway"), &pDhcp->Gateway, 1);

				GetRegIPAddr(hKey, TEXT("DhcpDNS"), pDhcp->DNS, 2);
				GetRegIPAddr(hKey, TEXT("DhcpWINS"), pDhcp->WinsServer, 2);

				// Get Lease Info
				GetRegDWORDValue(hKey, TEXT("LeaseObtainedLow"), 
					&pDhcp->LeaseObtained.dwLowDateTime);
				GetRegDWORDValue(hKey, TEXT("LeaseObtainedHigh"), 
					&pDhcp->LeaseObtained.dwHighDateTime);

				GetRegDWORDValue(hKey, TEXT("Lease"), &pDhcp->Lease);
				// Get T1 & T2
				GetRegDWORDValue(hKey, TEXT("T1"), &pDhcp->T1);
				GetRegDWORDValue(hKey, TEXT("T2"), &pDhcp->T2);
			}
		}

		pDhcp->cMaxRetry = DEFAULT_MAX_RETRIES;
		pDhcp->InitDelay = DEFAULT_INIT_DELAY;
		pDhcp->cRetryDialogue = DEFAULT_RETRY_DIALOGUE;
		// Get MaxRetries, InitDelayInterval
		GetRegDWORDValue(hKey, TEXT("DhcpMaxRetry"), &pDhcp->cMaxRetry);
		GetRegDWORDValue(hKey, TEXT("DhcpInitDelayInterval"), &pDhcp->InitDelay);
		GetRegDWORDValue(hKey, TEXT("DhcpRetryDialogue"), &pDhcp->cRetryDialogue);

		i = 0;
		GetRegDWORDValue(hKey, TEXT("DhcpNoMacCompare"), &i);

		if (i)
			pDhcp->Flags |= NO_MAC_COMPARE_FL;
		else
			pDhcp->Flags &= ~NO_MAC_COMPARE_FL;

		GetReqOptions(hKey, pDhcp);

        // Get auto IP config state
        if (GetRegDWORDValue(hKey, TEXT("AutoCfg"), (LPDWORD)&i)) {
            // auto IP config is enabled by default
            if (!i) {
                pDhcp->Flags &= ~AUTO_IP_ENABLED_FL;    // user wants it disabled
            }
        }
        if (pDhcp->Flags & AUTO_IP_ENABLED_FL) {
            GetRegDWORDValue(hKey, TEXT("AutoSeed"), &pDhcp->AutoSeed);
            GetRegIPAddr(hKey, TEXT("AutoIP"), &pDhcp->AutoIPAddr, 1);
            GetRegIPAddr(hKey, TEXT("AutoSubnet"), &pDhcp->AutoSubnet, 1);
            GetRegIPAddr(hKey, TEXT("AutoMask"), &pDhcp->AutoMask, 1);
            GetRegDWORDValue(hKey, TEXT("AutoInterval"), &pDhcp->AutoInterval);
        }


		RegCloseKey(hKey);	

	}	// if (hKey)

	DEBUGMSG (ZONE_INIT, (TEXT("-GetDhcpConfig:\r\n")));
	return DHCP_SUCCESS;

}	// GetDhcpConfig()


STATUS SetDhcpConfig(DhcpInfo *pDhcp) {
	TCHAR	Buffer[MAX_REG_STR];
	HKEY	hKey;
	LONG	hRes;
	BOOL	fStatus;
	int		i;

	DEBUGMSG (ZONE_INIT, (TEXT("+SetDhcpConfig:\r\n")));

	// Open the Registry Key.
	hKey = OpenDHCPKey(pDhcp);
	
	if (hKey) {
		SetRegDWORDValue(hKey, TEXT("EnableDHCP"), (pDhcp->Flags & DHCP_ENABLED_FL) ? 1 : 0);

		AddrToString(pDhcp->IPAddr, Buffer);
		fStatus = SetRegSZValue(hKey, TEXT("DhcpIPAddress"), Buffer);
		DEBUGMSG (ZONE_WARN, (TEXT("\tSetDhcpConfig: set IPAddr %X\r\n"),
			pDhcp->IPAddr));
		if (fStatus) {
			AddrToString(pDhcp->SubnetMask, Buffer);
			SetRegSZValue(hKey, TEXT("DhcpSubnetMask"), Buffer);

			AddrToString(pDhcp->DhcpServer, Buffer);
			SetRegSZValue(hKey, TEXT("DhcpServer"), Buffer);

			AddrToString(pDhcp->Gateway, Buffer);
			SetRegSZValue(hKey, TEXT("DhcpDefaultGateway"), Buffer);

			Buffer[0] = Buffer[1] = TEXT('\0');
			i = 0;
			if (pDhcp->DNS[0]) {
				AddrToString(pDhcp->DNS[0], Buffer);
				i = _tcslen(Buffer) + 1;
				if (pDhcp->DNS[1]) {
					AddrToString(pDhcp->DNS[1], &Buffer[i]);
					i += _tcslen(&Buffer[i]) + 1;
				}
				Buffer[i] = TEXT('\0');
			}
			// we want: dns1\0dns2\0\0
			SetRegMultiSZValue(hKey, TEXT("DhcpDNS"), Buffer);

			Buffer[0] = Buffer[1] = TEXT('\0');
			i = 0;
			if (pDhcp->WinsServer[0]) {
				AddrToString(pDhcp->WinsServer[0], Buffer);
				i = _tcslen(Buffer) + 1;
				if (pDhcp->WinsServer[1]) {
					AddrToString(pDhcp->WinsServer[1], &Buffer[i]);
					i += _tcslen(&Buffer[i]) + 1;
				}
				Buffer[i] = TEXT('\0');
			}
			// we want: wins1\0wins2\0\0
			SetRegMultiSZValue(hKey, TEXT("DhcpWINS"), Buffer);

			// Set Lease Times
			SetRegDWORDValue(hKey, TEXT("LeaseObtainedLow"), 
				pDhcp->LeaseObtained.dwLowDateTime);
			SetRegDWORDValue(hKey, TEXT("LeaseObtainedHigh"), 
				pDhcp->LeaseObtained.dwHighDateTime);
			DEBUGMSG (ZONE_WARN, 
				(TEXT("\tSetDhcpConfig: set LeaseObtained %x %x\r\n"),
				pDhcp->LeaseObtained.dwHighDateTime, 
				pDhcp->LeaseObtained.dwLowDateTime));

			SetRegDWORDValue(hKey, TEXT("Lease"), pDhcp->Lease);
			// Set T1 & T2
			SetRegDWORDValue(hKey, TEXT("T1"), pDhcp->T1);
			SetRegDWORDValue(hKey, TEXT("T2"), pDhcp->T2);
			DEBUGMSG (ZONE_WARN, 
				(TEXT("\tSetDhcpConfig: T1 %x T2 %x Lease %x\r\n"),
				pDhcp->T1, pDhcp->T2, pDhcp->Lease));

			// Store the list of params requested from server so we can tell
			// if it has changed after a reboot.
			SetRegBinaryValue(hKey,TEXT("PrevReqOptions"),pDhcp->ReqOptions,pDhcp->ReqOptions[0]+1);
		}

        // Save auto IP config state
        if (pDhcp->Flags & AUTO_IP_ENABLED_FL) {
            SetRegDWORDValue(hKey, TEXT("AutoSeed"), pDhcp->AutoSeed);
            AddrToString(pDhcp->AutoIPAddr, Buffer);
            SetRegSZValue(hKey, TEXT("AutoIP"), Buffer);
            AddrToString(pDhcp->AutoSubnet, Buffer);
            SetRegSZValue(hKey, TEXT("AutoSubnet"), Buffer);
            AddrToString(pDhcp->AutoMask, Buffer);
            SetRegSZValue(hKey, TEXT("AutoMask"), Buffer);
            SetRegDWORDValue(hKey, TEXT("AutoInterval"), pDhcp->AutoInterval);
        } else {
            SetRegDWORDValue(hKey, TEXT("AutoCfg"), 0);
        }


		RegCloseKey (hKey);	
	}	// if (hKey)

	if (pDhcp->aDomainName[0]) {
		_tcscpy (Buffer, COMM_REG_KEY);
		_tcscat (Buffer, TEXT("TcpIp\\Parms"));
		hRes = RegOpenKeyEx (HKEY_LOCAL_MACHINE, Buffer, 0, 0, &hKey);
		
		if (ERROR_SUCCESS == hRes) {
			if (-1 != mbstowcs(Buffer, &pDhcp->aDomainName[1], 
				pDhcp->aDomainName[0])) {
				Buffer[pDhcp->aDomainName[0]]= (L'\0');
				SetRegSZValue(hKey, TEXT("DNSDomain"), Buffer);
			}
			RegCloseKey(hKey);
		}
	}
	DEBUGMSG (ZONE_INIT, (TEXT("-SetDhcpConfig:\r\n")));
	return DHCP_SUCCESS;
}	// SetDhcpConfig()


// assumes caller owns pDhcp->Lock
void TakeNetDown(DhcpInfo *pDhcp, BOOL fDiscardIPAddr, BOOL fRemoveAfdInterface) {
	HKEY		hKey;

	RETAILMSG(1, (TEXT("*TakeNetDown: pDhcp 0x%x IP %X!\r\n"),
		pDhcp, pDhcp->IPAddr));

    pDhcp->ARPResult = ERROR_UNEXP_NET_ERR;
    SetEvent(pDhcp->ARPEvent);  // wake up any waiting threads

	(*pfnIPSetNTEAddr)((ushort)pDhcp->NteContext, NULL, 0, 0, 0);

	if (fRemoveAfdInterface) {
        CallAfd(AddInterface)(pDhcp->Name, pDhcp->Nte, pDhcp->NteContext, 
                              ADD_INTF_DELETE_FL, pDhcp->IPAddr, pDhcp->SubnetMask, 0, NULL, 0, NULL);    
    }

    if (fDiscardIPAddr) {
        pDhcp->IPAddr = 0;
        // Delete the IPAddr.
        hKey = OpenDHCPKey(pDhcp);
        if (hKey) {
            RegDeleteValue(hKey, TEXT("DhcpIPAddress"));
            RegCloseKey (hKey);	
        }
	}

}	// TakeNetDown()


// assumes caller has v_GlobalListLock
void PutInWorkerQ(DhcpInfo *pDhcp) {
	HANDLE		hWorkerThrd;
	DWORD		ThreadId;

	pDhcp->pNext = v_pWaitingList;
	v_pWaitingList = pDhcp;
	// if there is already a worker thread we're done
	if (! v_fWorkerThread) {
		v_fWorkerThread = TRUE;
		hWorkerThrd = CreateThread(NULL, 0, 
			(LPTHREAD_START_ROUTINE)DhcpWorkerThread, NULL, 0, 
			&ThreadId);
		if (hWorkerThrd) {
			CloseHandle(hWorkerThrd);
		} else {
			RETAILMSG(1, 
				(TEXT("!RequestDHCPAddr: can't create worker thread\r\n")));
		}
	}
}	// PutInWorkerQ

//
// Close the Dhcp interface's socket if open and set it to NULL
// (Afd doesn't check the socket parameter, so we have to)
//
void
CloseDhcpSocket(
    DhcpInfo *pDhcp
    )
{
    if (pDhcp->Socket) {
        CallSock(Close) (pDhcp->Socket);
        pDhcp->Socket = NULL;
    }
}

// called when our lease has expired!
void ProcessExpire(DhcpInfo *pDhcp) {
	DhcpInfo	**ppDhcp;

	DEBUGMSG(ZONE_TIMER|ZONE_WARN, 
		(TEXT("+ProcessExpire: pDhcp 0x%X\r\n"), pDhcp));
	
	if (*(ppDhcp = _FindDhcp(pDhcp, NULL))) {
		RETAILMSG(1, (TEXT("!Lease Time Expired, bringing net down %x\r\n"),
			pDhcp));
		
		*ppDhcp = pDhcp->pNext;

		// take the interface down!
		pDhcp->SFlags |= DHCPSTATE_LEASE_EXPIRE;
		TakeNetDown(pDhcp, TRUE, TRUE);
		ClearDHCPNTE(pDhcp);		
		CloseDhcpSocket(pDhcp);
		PutInWorkerQ(pDhcp);

		CTEFreeLock(&v_GlobalListLock, 0);
		CTEFreeLock(&pDhcp->Lock, 0);
	} else
		CTEFreeLock(&v_GlobalListLock, 0);

	DEBUGMSG(ZONE_TIMER, (TEXT("-ProcessExpire: pDhcp 0x%X\r\n"),
			pDhcp));
}	// ProcessExpire()


void CalcMidwayTime(DhcpInfo *pDhcp, int End, CTEEventRtn Rtn1, 
					CTEEventRtn Rtn2) {
	FILETIME	CurTime, EndTime, TempTime;

	// calc Expire time, if > 1 min restart T1 timer
	EndTime.dwLowDateTime = End;
	EndTime.dwHighDateTime = 0;
	mul64_32_64(&EndTime, TEN_M, &EndTime);
	add64_64_64(&pDhcp->LeaseObtained, &EndTime, &EndTime);

	GetCurrentFT(&CurTime);
	if (CompareFileTime(&CurTime, &EndTime) >= 0) {
		DEBUGMSG(ZONE_TIMER, 
			(TEXT("*CalcMidwayTime: late already! start 2nd timer %x %x\r\n"),
			EndTime.dwHighDateTime, EndTime.dwLowDateTime));
		// start next timer
		CTEStartFTimer(&pDhcp->Timer, EndTime, 
			(CTEEventRtn)Rtn2, pDhcp);
	} else {
		sub64_64_64(&EndTime, &CurTime, &TempTime);
		
		DEBUGMSG(ZONE_TIMER, 
			(TEXT("*CalcMidwayTime: CurTime %x %x Endtime %x %x dt %x %x\r\n"),
			CurTime.dwHighDateTime, CurTime.dwLowDateTime,
			EndTime.dwHighDateTime, EndTime.dwLowDateTime,
			TempTime.dwHighDateTime, TempTime.dwLowDateTime));

		// if more than 1 min remain, restart timer
		// otherwise start the next timer
		if (TempTime.dwHighDateTime > 0 ||
			(TempTime.dwHighDateTime == 0 &&
			TempTime.dwLowDateTime >= (60 * TEN_M))) {

			div64_32_64(&TempTime, 2, &TempTime);
			add64_64_64(&CurTime, &TempTime, &TempTime);

			DEBUGMSG(ZONE_TIMER, 
				(TEXT("*CalcMidwayTime: starting timer for %x %x\r\n"),
				TempTime.dwHighDateTime, TempTime.dwLowDateTime));

			// restart the timer
			CTEStartFTimer(&pDhcp->Timer, TempTime, 
				(CTEEventRtn)Rtn1, pDhcp);
		} else {
			DEBUGMSG(ZONE_TIMER, 
				(TEXT("*CalcMidwayTime: starting 2nd timer for %x %x\r\n"),
				EndTime.dwHighDateTime, EndTime.dwLowDateTime));
			// start next timer
			CTEStartFTimer(&pDhcp->Timer, EndTime, 
				(CTEEventRtn)Rtn2, pDhcp);
		}
	}

}	// CalcMidwayTime()


void CalcT1Time(DhcpInfo *pDhcp) {
	FILETIME Ft;

	Ft.dwLowDateTime = pDhcp->T1;
	Ft.dwHighDateTime = 0;
	mul64_32_64(&Ft, TEN_M, &Ft);
	add64_64_64(&pDhcp->LeaseObtained, &Ft, &Ft);
	// Start T1 timer
	DEBUGMSG(ZONE_TIMER, (TEXT("*CalcT1Time: init LO %x %x , T1 %x %x\r\n"),
		pDhcp->LeaseObtained.dwHighDateTime, pDhcp->LeaseObtained.dwLowDateTime,
		Ft.dwHighDateTime, Ft.dwLowDateTime));

	CTEStartFTimer(&pDhcp->Timer, Ft, (CTEEventRtn)ProcessT1, pDhcp);	

}	// CalcT1Time()


STATUS	DhcpInitSock(DhcpInfo *pInfo, int SrcIP) {
	SOCKADDR_IN	SockAddr;
	STATUS		Status = DHCP_SUCCESS;
	LPSOCK_INFO	Sock;

	DEBUGMSG(ZONE_INIT, (TEXT("+DhcpInitSock: pInfo 0x%X\r\n"),
			pInfo));

	memset ((char *)&SockAddr, 0, sizeof(SockAddr));
	SockAddr.sin_family = AF_INET;
	SockAddr.sin_port = net_short(DHCP_CLIENT_PORT);
	SockAddr.sin_addr.S_un.S_addr = SrcIP;	// net_long(0);

	// set flag that this is an internal socket
	Sock = (LPSOCK_INFO) CallAfd(Socket) 
					(0x80000000 | AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (Sock) {
		int i = 1;

		
		if (SOCKET_ERROR == CallSock(SetOption)(Sock, SOL_SOCKET, 
				SO_REUSEADDR, &i, sizeof(i), NULL) ||
			SOCKET_ERROR == CallSock(SetOption)(Sock, SOL_SOCKET, 
				SO_BROADCAST, &i, sizeof(i), NULL) ||
			SOCKET_ERROR == CallSock(Bind)(Sock, (SOCKADDR *)&SockAddr, 
			sizeof(SockAddr), (struct CRITICAL_SECTION *)0xffffffff))
			
		{
			Status = GetLastError();
			DEBUGMSG(ZONE_WARN | ZONE_ERROR, 
				(TEXT("\t!DhcpInitSock: failure %d\r\n"),
				Status));

			CallSock(Close) (Sock);
		}
	} else	// if (Sock)
		Status = GetLastError();

	if (DHCP_SUCCESS == Status)
		pInfo->Socket = Sock;

	DEBUGMSG(ZONE_INIT, (TEXT("-DhcpInitSock: pInfo 0x%X/0x%X Ret: %d\r\n"),
			pInfo, pInfo->Socket, Status));

	return Status;

}	// InitSocket()


//
// Function to call ARP's SetDHCPNTE - caller must have pDhcp->Lock
//
BOOL
SetDHCPNTE(
    DhcpInfo * pDhcp
    )
{
	BOOL	RetStatus = FALSE;

	if (0 == (pDhcp->SFlags & DHCPSTATE_DHCPNTE_SET)) {
		RetStatus = (*pfnSetDHCPNTE)(pDhcp->NteContext, NULL, NULL, NULL);
		if (RetStatus)
			pDhcp->SFlags |= DHCPSTATE_DHCPNTE_SET;
	}

	return RetStatus;
}


BOOL
ClearDHCPNTE(
    DhcpInfo * pDhcp
    )
{
	BOOL	RetStatus = FALSE;

	if (pDhcp->SFlags & DHCPSTATE_DHCPNTE_SET) {
		// even if this fails we're going to assume that we no longer have
		// the dhcpnte set
		pDhcp->SFlags &= ~DHCPSTATE_DHCPNTE_SET;
	    RetStatus = (*pfnSetDHCPNTE)(0x1fff, NULL, NULL, NULL);
	}

	return RetStatus;
}


void ProcessT2(DhcpInfo *pDhcp) {
	STATUS		Status = DHCP_SUCCESS;
	DhcpPkt		Pkt;
	int			cPkt;
	FILETIME	CurTime;
	int			fStatus;
	DhcpInfo	**ppDhcp;

	DEBUGMSG(ZONE_TIMER, (TEXT("+ProcessT2: pDhcp 0x%X\r\n"), pDhcp));	
	
	if (*_FindDhcp(pDhcp, NULL)) {
		CTEFreeLock(&v_GlobalListLock, 0);
		pDhcp->SFlags |= DHCPSTATE_T2_EXPIRE;

		if (DHCP_SUCCESS == (Status = DhcpInitSock(pDhcp, pDhcp->IPAddr))) {

			BuildDhcpPkt(pDhcp, &Pkt, DHCPREQUEST, (NEW_PKT_FL), pDhcp->ReqOptions, &cPkt);

            fStatus = SetDHCPNTE(pDhcp);
            GetCurrentFT(&CurTime);
            DEBUGMSG(ZONE_TIMER, (TEXT("\tProcess T2: CurTime: %x %x\r\n"),
                CurTime.dwHighDateTime, CurTime.dwLowDateTime));
            
            if (fStatus)
                Status = SendDhcpPkt(pDhcp, &Pkt, cPkt, DHCPACK, 
                    (ONCE_FL | BCAST_FL));
            else
                Status = DHCP_NOCARD;

            if (Status == DHCP_SUCCESS) {
                pDhcp->SFlags &= ~(DHCPSTATE_T2_EXPIRE | DHCPSTATE_T1_EXPIRE);
                memcpy(&pDhcp->LeaseObtained, &CurTime, sizeof(CurTime));
                SetDhcpConfig(pDhcp);
                CalcT1Time(pDhcp);

            } else if (Status == DHCP_NACK) {
                DEBUGMSG(ZONE_WARN, 
                    (TEXT("\t!ProcessT2: Take Net Down\r\n")));
                TakeNetDown(pDhcp, TRUE, TRUE);
                ClearDHCPNTE(pDhcp);
		        pDhcp->SFlags &= ~(DHCPSTATE_T2_EXPIRE | DHCPSTATE_T1_EXPIRE);
	            CloseDhcpSocket(pDhcp);
				CTEFreeLock(&pDhcp->Lock, 0);

				if (*(ppDhcp = _FindDhcp(pDhcp, NULL))) {
					*ppDhcp = pDhcp->pNext;

//					CallNetMsgBox(NULL, NMB_FL_EXCLAIM | NMB_FL_OK | NMB_FL_TOPMOST,
//						NETUI_GETNETSTR_LEASE_EXPIRED);

	                PutInWorkerQ(pDhcp);
	                CTEFreeLock(&pDhcp->Lock, 0);
				}
                CTEFreeLock(&v_GlobalListLock, 0);

                return;
            } else {
                CalcMidwayTime(pDhcp, pDhcp->Lease, (CTEEventRtn)ProcessT2, 
                    (CTEEventRtn)ProcessExpire);
            }
            if (fStatus)
                ClearDHCPNTE(pDhcp);
			CloseDhcpSocket(pDhcp);

		} else {	// (DHCP_SUCCESS == DhcpInitSock())
			DEBUGMSG(ZONE_WARN, 
				(TEXT("\t!ProcessT2: DhcpInitSock failed %d!!\r\n"),
				Status));
			CalcMidwayTime(pDhcp, pDhcp->Lease, (CTEEventRtn)ProcessT2, 
				(CTEEventRtn)ProcessExpire);
		}
		CTEFreeLock(&pDhcp->Lock, 0);
	} else	// if (*_FindDhcp())
		CTEFreeLock(&v_GlobalListLock, 0);

	DEBUGMSG(ZONE_TIMER, (TEXT("-ProcessT2: pDhcp 0x%X\r\n"),
			pDhcp));	
}	// ProcessT2()


void ProcessT1(DhcpInfo *pDhcp) {
	STATUS		Status = DHCP_SUCCESS;
	DhcpPkt		Pkt;
	int			cPkt;
	FILETIME	CurTime;
	int			fStatus;
	DhcpInfo	**ppDhcp;

	DEBUGMSG(ZONE_TIMER, (TEXT("+ProcessT1: pDhcp 0x%X\r\n"), pDhcp));	

	if (*_FindDhcp(pDhcp, NULL)) {
		CTEFreeLock(&v_GlobalListLock, 0);
		pDhcp->SFlags |= DHCPSTATE_T1_EXPIRE;
		if (DHCP_SUCCESS == (Status = DhcpInitSock(pDhcp, pDhcp->IPAddr))) {

			BuildDhcpPkt(pDhcp, &Pkt, DHCPREQUEST, (NEW_PKT_FL), pDhcp->ReqOptions, &cPkt);

            fStatus = SetDHCPNTE(pDhcp);
            GetCurrentFT(&CurTime);

            DEBUGMSG(ZONE_TIMER, (TEXT("\tProcess T1: CurTime: %x %x\r\n"),
                CurTime.dwHighDateTime, CurTime.dwLowDateTime));

            if (fStatus)
                Status = SendDhcpPkt(pDhcp, &Pkt, cPkt, DHCPACK, ONCE_FL);
            else
                Status = DHCP_NOCARD;

            if (Status == DHCP_SUCCESS) {
                pDhcp->SFlags &= ~(DHCPSTATE_T1_EXPIRE);
                memcpy(&pDhcp->LeaseObtained, &CurTime, sizeof(CurTime));
                SetDhcpConfig(pDhcp);
                CalcT1Time(pDhcp);

            } else if (Status == DHCP_NACK) {
                // Take Net Down !!!
                DEBUGMSG(ZONE_WARN, 
                    (TEXT("\t!ProcessT1: Take Net Down\r\n")));
                TakeNetDown(pDhcp, TRUE, TRUE);
				ClearDHCPNTE(pDhcp);
				pDhcp->SFlags &= ~(DHCPSTATE_T1_EXPIRE);
				CloseDhcpSocket(pDhcp);
				CTEFreeLock(&pDhcp->Lock, 0);

				if (*(ppDhcp = _FindDhcp(pDhcp, NULL))) {
					*ppDhcp = pDhcp->pNext;

	//					CallNetMsgBox(NULL, NMB_FL_EXCLAIM | NMB_FL_OK | NMB_FL_TOPMOST,
	//						NETUI_GETNETSTR_LEASE_EXPIRED);
					PutInWorkerQ(pDhcp);
					CTEFreeLock(&pDhcp->Lock, 0);
				}
                CTEFreeLock(&v_GlobalListLock, 0);
                return;
            } else {
                CalcMidwayTime(pDhcp, pDhcp->T2, (CTEEventRtn)ProcessT1, 
                    (CTEEventRtn)ProcessT2);
            }
            if (fStatus)
                ClearDHCPNTE(pDhcp);
			CloseDhcpSocket(pDhcp);

		} else {	// if (DHCP_SUCCESS == DhcpInitSock)
			DEBUGMSG(ZONE_WARN, 
				(TEXT("\t!ProcessT1: DhcpInitSock failed %d!!\r\n"),
				Status));
			CalcMidwayTime(pDhcp, pDhcp->T2, (CTEEventRtn)ProcessT1, 
				(CTEEventRtn)ProcessT2);
		}
		CTEFreeLock(&pDhcp->Lock, 0);
	} else	// if (FindDhcp())
		CTEFreeLock(&v_GlobalListLock, 0);

	DEBUGMSG(ZONE_TIMER, (TEXT("-ProcessT1: pDhcp 0x%X\r\n"),
			pDhcp));	
}	// ProcessT1()

#ifdef UNDER_CE
// Handles by which to notify applications of changes to IP data structures.
HANDLE	g_hAddrChangeEvent;

void
NotifyXxxChange(HANDLE hEvent)
//
// Pulse the event to let applications know a change has occurred
//
{
	if (hEvent)
	{
		





		PulseEvent(hEvent);
	}
}
#endif //UNDER_CE

//
// PutNetUp - Cause TCP/IP to use DHCP negotiated IP address for this net.
// The TCP/IP stack will call our ARPResult function if there is an IP address conflict.
// ARPResult sets the ARPEvent to indicate it was called.
//
// Return: DHCP_* error code from comm\inc\dhcp.h
//
STATUS PutNetUp(DhcpInfo *pDhcp) {
    int i, j;

    pDhcp->ARPResult = ERROR_SUCCESS;   // assume no IP address collision
    ResetEvent(pDhcp->ARPEvent);
    i = (*pfnIPSetNTEAddr)((ushort)pDhcp->NteContext, NULL, 
                           pDhcp->IPAddr, pDhcp->SubnetMask, pDhcp->Gateway);
    if (i == IP_PENDING)
        WaitForSingleObject(pDhcp->ARPEvent, ARP_RESPONSE_WAIT);
    else if (i != IP_SUCCESS) {
        DEBUGMSG(ZONE_WARN, (L"DHCP:PutNetUp - IPSetNTEAddr failed!\n"));
        return (i == IP_DUPLICATE_ADDRESS) ? DHCP_COLLISION : DHCP_FAILURE;
    }
    if (ERROR_SUCCESS == pDhcp->ARPResult) {
        // no IP address collision, proceed
        i = j = 0;
    	if (pDhcp->DNS[0]) {
            i++;
            if (pDhcp->DNS[1]) {
                i++;
            }
    	}

    	if (pDhcp->WinsServer[0]) {
            j++;
            if (pDhcp->WinsServer[1]) {
                j++;
            }
    	}
    	CallAfd(AddInterface)(pDhcp->Name, pDhcp->Nte, pDhcp->NteContext, 0, pDhcp->IPAddr, 
    		pDhcp->SubnetMask, i, pDhcp->DNS, j, pDhcp->WinsServer);	 

        NotifyXxxChange(g_hAddrChangeEvent);

        return DHCP_SUCCESS;
	} else {
        DEBUGMSG(ZONE_WARN, (L"DHCP:PutNetUp - ARPResult == %d, assuming IP addr collision\n", pDhcp->ARPResult));
        return DHCP_COLLISION;
    }
}	// PutNetUp()


STATUS DhcpSendDown(DhcpInfo *pDhcp, DhcpPkt *pPkt, int cPkt, int DestIP);


void DhcpDecline(DhcpInfo * pDhcp, DhcpPkt *pPkt) {
    int cPkt;

    BuildDhcpPkt(pDhcp, pPkt, DHCPDECLINE, RIP_PKT_FL|NEW_PKT_FL, NULL, &cPkt);
    DhcpSendDown(pDhcp, pPkt, cPkt, INADDR_BROADCAST);

    // AFD info not added at this point
    TakeNetDown(pDhcp, TRUE, FALSE);  
}


BOOL CanUseCachedLease(DhcpInfo * pDhcp) {
	FILETIME	BeginTime, EndTime;

    // we only try to use our old lease if we're not in T2 stage!
    EndTime.dwHighDateTime = 0;
    EndTime.dwLowDateTime = pDhcp->T2;
    mul64_32_64(&EndTime, TEN_M, &EndTime);
    add64_64_64(&pDhcp->LeaseObtained, &EndTime, &EndTime);				

    GetCurrentFT(&BeginTime);
    if (CompareFileTime(&EndTime, &BeginTime) >= 0) {
        return TRUE;
    }

    return FALSE;
}   // CanUseCachedLease


STATUS GetParams(DhcpInfo *pDhcp) {
	STATUS		Status = DHCP_SUCCESS;
	DhcpPkt		Pkt;
	int			cPkt, fDiscover, Flags, fDone;
	FILETIME	BeginTime;
	uint		cRetry, cDelay;

	DEBUGMSG(ZONE_INIT, (TEXT("+GetParams: pDhcp 0x%X/0x%X\n"),
			pDhcp, pDhcp->Socket));

	cRetry = fDone = 0;
	pDhcp->SFlags &= ~(DHCPSTATE_T1_EXPIRE | DHCPSTATE_T2_EXPIRE | 
		DHCPSTATE_LEASE_EXPIRE | DHCPSTATE_SAW_DHCP_SRV);
		
	GetDhcpConfig(pDhcp);

	if (pDhcp->Flags & DHCP_ENABLED_FL) {

        // If the IP address is currently auto cfg and the user has requested "ipconfig /renew"
        // then stop the 5 minute discover timer and force a discover to happen 
        if (pDhcp->SFlags & DHCPSTATE_AUTO_CFG_IP) {
            CTEStopFTimer(&pDhcp->Timer);
			pDhcp->SFlags &= ~DHCPSTATE_AUTO_CFG_IP;
            pDhcp->IPAddr = 0;  // force a discover
        }

		for ( ; ! fDone && cRetry <= pDhcp->cMaxRetry; cRetry++) {
			cDelay = (GetTickCount() % pDhcp->InitDelay);
			Sleep(cDelay);
			
			if (SetDHCPNTE(pDhcp)) {
				Status = DHCP_SUCCESS;
				GetCurrentFT(&BeginTime);

                fDiscover = (!pDhcp->IPAddr) || (pDhcp->Flags & OPTION_CHANGE_FL);

				if (fDiscover) {	// if no prev IP or lease exp. or option change
					pDhcp->IPAddr = 0;
					pDhcp->Flags &= ~OPTION_CHANGE_FL;
					BuildDhcpPkt(pDhcp, &Pkt, DHCPDISCOVER, (NEW_PKT_FL), pDhcp->ReqOptions, &cPkt);

					Status = SendDhcpPkt(pDhcp, &Pkt, cPkt, DHCPOFFER, 
						BCAST_FL | DFT_LOOP_FL);
				}

				if (DHCP_SUCCESS == Status) {
					Flags = RIP_PKT_FL;	// request IP addr must be filled in
					if (fDiscover) 
						Flags |= SID_PKT_FL;
					else {
						Flags |= NEW_PKT_FL;
					}
					BuildDhcpPkt(pDhcp, &Pkt, DHCPREQUEST, Flags, pDhcp->ReqOptions, &cPkt);
					Status = SendDhcpPkt(pDhcp, &Pkt, cPkt, DHCPACK, 
						BCAST_FL | DFT_LOOP_FL);
				}

				ClearDHCPNTE(pDhcp);


                if ((pDhcp->Flags & AUTO_IP_ENABLED_FL) &&
				   (!(pDhcp->SFlags & DHCPSTATE_SAW_DHCP_SRV))) {
                    // No response from a DHCP server, so try AutoIP.
                    DEBUGMSG(ZONE_AUTOIP, (TEXT("DHCP:GetParams - No response from DHCP server, trying AutoIP\n")));
                    Status = AutoIP(pDhcp);
                }

				if (DHCP_SUCCESS == Status) {
					memcpy(&pDhcp->LeaseObtained, &BeginTime, sizeof(BeginTime));
					SetDhcpConfig(pDhcp);
					fDone = TRUE;

                    // We need to PutNetUp() here only when we've negotiated a lease with a DHCP server.
                    // If we didn't auto configure and didn't see a DHCP server, then AutoIP() was able to
                    // ping the default gateway so it decided to use the cached lease already.
					if ((!(pDhcp->SFlags & DHCPSTATE_AUTO_CFG_IP)) &&
                        (pDhcp->SFlags & DHCPSTATE_SAW_DHCP_SRV)) {
						Status = PutNetUp(pDhcp); 
						switch (Status) {
						case DHCP_SUCCESS:
							CalcT1Time(pDhcp);
							break;

						case DHCP_COLLISION:
							DhcpDecline(pDhcp, &Pkt);
							// fall through

						default:
							fDone = FALSE;
							break;
						}
					}

				} else if (DHCP_NACK == Status) {
					pDhcp->IPAddr = 0;

				} else if (!fDiscover) {
					// didn't get a NAK see if we can use our old lease
					if (pDhcp->IPAddr) {
                        if (CanUseCachedLease(pDhcp)) {
                            DEBUGMSG(ZONE_WARN, 
                                (TEXT("\tGetParams: using cached DHCP lease\r\n")));                    
                            CallNetMsgBox(NULL, NMB_FL_OK, NETUI_GETNETSTR_CACHED_LEASE);

							Status = PutNetUp(pDhcp);
							switch (Status) {
							case DHCP_SUCCESS:
								// even if we're already in T1 stage this should work
								// it just sets up a timer to call ProcessT1 right away
								// ProcessT1 should then do the right thing
								CalcT1Time(pDhcp);
								fDone = TRUE;
								break;

							case DHCP_COLLISION:
								DhcpDecline(pDhcp, &Pkt);
								// fall through

							default:
								fDone = FALSE;
								break;
								}
						} else {
							// we're in T2 stage don't use old addr.
							pDhcp->IPAddr = 0;
						}
					}
				} else {	// else if (!fDiscover)
					DEBUGMSG(ZONE_WARN, (TEXT("\tGetParams: failed badly!\n")));
					// shall we try again?
					pDhcp->IPAddr = 0;
				}
			} else {	// if (SetDHCPNTE)
				DEBUGMSG(ZONE_WARN, 
					(TEXT("DHCP: GetParams: adapter disappeared\r\n")));
				Status = DHCP_NOCARD;
			}

			if (! fDone && ((pDhcp->SFlags & DHCPSTATE_DIALOGUE_BOX) == 0) && 
				DHCP_NOCARD != Status &&
				(cRetry == pDhcp->cRetryDialogue || cRetry == pDhcp->cMaxRetry)) {

				pDhcp->SFlags |= DHCPSTATE_DIALOGUE_BOX;
				CallNetMsgBox(NULL, NMB_FL_EXCLAIM | NMB_FL_OK | 
					NMB_FL_TOPMOST, NETUI_GETNETSTR_NO_IPADDR);
			}
		}	// for(...)
        if (fDone) {
            pDhcp->SFlags &= ~DHCPSTATE_DIALOGUE_BOX;
        }
	}	// if (pDhcp->Flags & DHCP_ENABLED_FL)

	DEBUGMSG(ZONE_INIT, (TEXT("-GetParams: pDhcp 0x%X Ret: %d\n"), pDhcp, Status));
	return Status;
}	// GetParams()


STATUS DelDHCPInterface(PTSTR pAdapter, void *Nte, unsigned Context) {
	STATUS		Status;
	DhcpInfo	**ppDhcp, *pDhcp;

	ppDhcp = _FindDhcp(NULL, pAdapter);
	if (pDhcp = *ppDhcp) {
		*ppDhcp = pDhcp->pNext;
		CTEFreeLock(&v_GlobalListLock, 0);

		TakeNetDown(pDhcp, FALSE, TRUE);
		// we probably don't need to clear, but since we now have a flag
		// to not clear if we don't have it set it should be safer
		ClearDHCPNTE(pDhcp);
		CloseDhcpSocket(pDhcp);
		
		CTEFreeLock(&pDhcp->Lock, 0);
		FreeDhcpInfo(pDhcp);

	} else {
		// let's check the worker Q
		ppDhcp = &v_pWaitingList;
		while (pDhcp = *ppDhcp) {
			if (0 == _tcscmp(pDhcp->Name, pAdapter)) {
				*ppDhcp = pDhcp->pNext;
				FreeDhcpInfo(pDhcp);
				break;
			}
			ppDhcp = &(pDhcp->pNext);
		}

		if (pDhcp) {
			Status = DHCP_SUCCESS;
		} else if (v_pCurrent && 
			(0 == _tcscmp(v_pCurrent->Name, pAdapter))) {

			ASSERT(NULL == v_pDelete);
			v_pDelete = v_pCurrent;
			Status = DHCP_SUCCESS;
		} else {
			DEBUGMSG(ZONE_ERROR, 
				(TEXT("DHCP: DelDHCPInterface: couldn't find Interface %s\r\n"),
				pAdapter));
			Status = DHCP_NOINTERFACE;
		}
		CTEFreeLock(&v_GlobalListLock, 0);
	}
	
	return Status;

}	// DelDHCPInterface()


STATUS RenewDHCPAddr(PTSTR pAdapter, void *Nte, unsigned Context, char *pAddr, 
					 unsigned cAddr) {
	STATUS		Status = DHCP_SUCCESS;
	DhcpInfo	**ppDhcp, *pDhcp;
	int			fNewSock = 0;
	DhcpPkt		Pkt;
	int			cPkt;
	FILETIME	CurTime;
	int			fStatus, fLockFreed = 0;

	ppDhcp = _FindDhcp(NULL, pAdapter);
	if (pDhcp = *ppDhcp) {
		CTEFreeLock(&v_GlobalListLock, 0);
		ASSERT(pDhcp->Nte == Nte);
		ASSERT(pDhcp->NteContext == Context);

		if (pDhcp->SFlags & DHCPSTATE_LEASE_EXPIRE) {
			ASSERT(0);
			// if this is the case, we shouldn't be on the established list!
			Status = DHCP_FAILURE;
		} else if (pDhcp->SFlags & DHCPSTATE_AUTO_CFG_IP) {
			fLockFreed = TRUE;
			TakeNetDown(pDhcp, FALSE, TRUE);
			CTEFreeLock(&pDhcp->Lock, 0);
            Status = RequestDHCPAddr(pAdapter, Nte, Context, pAddr, cAddr);
        } else {

			if (! pDhcp->Socket) {
				fNewSock++;
				Status = DhcpInitSock(pDhcp, pDhcp->IPAddr);
			}

			if (DHCP_SUCCESS == Status) {

				BuildDhcpPkt(pDhcp, &Pkt, DHCPREQUEST, (NEW_PKT_FL), pDhcp->ReqOptions, &cPkt);

				fStatus = TRUE;	// SetDHCPNTE(pDhcp);
				GetCurrentFT(&CurTime);

				if (fStatus)
					Status = SendDhcpPkt(pDhcp, &Pkt, cPkt, DHCPACK, 3);
				else
					Status = DHCP_NOCARD;

				if (Status == DHCP_SUCCESS) {
					CTEStopFTimer(&pDhcp->Timer);
					memcpy(&pDhcp->LeaseObtained, &CurTime, sizeof(CurTime));
					SetDhcpConfig(pDhcp);
					// this should set the FTimer based on the new settings.
					CalcT1Time(pDhcp);

				} else if (Status == DHCP_NACK) {
					// Take Net Down !!!
					DEBUGMSG(ZONE_WARN, 
						(TEXT("\t!ProcessT1: Take Net Down\r\n")));
					CTEStopFTimer(&pDhcp->Timer);
					TakeNetDown(pDhcp, TRUE, TRUE);
					ClearDHCPNTE(pDhcp);
					CloseDhcpSocket(pDhcp);
					CTEFreeLock(&pDhcp->Lock, 0);

					if (*(ppDhcp = _FindDhcp(pDhcp, NULL))) {
						*ppDhcp = pDhcp->pNext;

		//					CallNetMsgBox(NULL, NMB_FL_EXCLAIM | NMB_FL_OK | NMB_FL_TOPMOST,
		//						NETUI_GETNETSTR_LEASE_EXPIRED);
						PutInWorkerQ(pDhcp);
						CTEFreeLock(&pDhcp->Lock, 0);
					}
					CTEFreeLock(&v_GlobalListLock, 0);
					// Status should be DHCP_NACK
					goto Exit;

				}	// Status == DHCP_NACK
				if (fNewSock)
					CloseDhcpSocket(pDhcp);
			}	// DHCP_SUCCESS == Status
		}	// else	DHCPSTATE_LEASE_EXPIRE
		if (! fLockFreed)
			CTEFreeLock(&pDhcp->Lock, 0);
	} else {	// if (_FindDhcp(...
		// let's check the worker Q
		pDhcp = v_pWaitingList;
		while (pDhcp) {
			if (0 == _tcscmp(pDhcp->Name, pAdapter)) {
				break;
			}
			pDhcp = pDhcp->pNext;
		}
		if (! pDhcp) {
			if (v_pCurrent && (0 == _tcscmp(v_pCurrent->Name, pAdapter)))
				pDhcp = v_pCurrent;
		}
		
		CTEFreeLock(&v_GlobalListLock, 0);
		if (pDhcp) {
			// this means he is either on the waiting list or our current one
			// so there is nothing more to do; return success.
			Status = DHCP_SUCCESS;
		} else {
			Status = RequestDHCPAddr(pAdapter, Nte, Context, pAddr, cAddr);
		}
	}
Exit:
	return Status;

}	// RenewDHCPAddr()


STATUS RequestDHCPAddr(PTSTR pAdapter, void *Nte, unsigned Context, char *pAddr, 
					 unsigned cAddr) {
	DhcpInfo	**ppDhcp, *pDhcp;
	int			cLen;
	STATUS		Status = DHCP_SUCCESS;

	DEBUGMSG(ZONE_WARN, (TEXT("+RequestDHCPAddr: %s Context %d\r\n"),
		pAdapter, Context));

	ppDhcp = _FindDhcp(NULL, pAdapter);
	if (pDhcp = *ppDhcp) {
		DEBUGMSG(ZONE_WARN, 
			(TEXT("!RequestDHCPAddr: found previous pDhcp %X w. IP %X\r\n"),
			pDhcp, pDhcp->IPAddr));
		// we already have an object for this adapter
		// take out of established list
		*ppDhcp = pDhcp->pNext;
		pDhcp->Nte = Nte;
		pDhcp->NteContext = Context;
		if (pAddr)
			memcpy(pDhcp->PhysAddr, pAddr, cAddr);
		CTEFreeLock(&pDhcp->Lock, 0);
	} else {
		cLen = (_tcslen(pAdapter) + 1) << 1;
		if (pDhcp = NewDhcpInfo(cLen)) {
			_tcscpy(pDhcp->Name, pAdapter);
			pDhcp->Nte = Nte;
			pDhcp->NteContext = Context;
			if (pAddr)
				memcpy(pDhcp->PhysAddr, pAddr, cAddr);
		}
	}

	if (pDhcp) {
		PutInWorkerQ(pDhcp);
	} else
		Status = DHCP_NOMEM;

	CTEFreeLock(&v_GlobalListLock, 0);

	ASSERT(v_GlobalListLock.OwnerThread != (HANDLE)GetCurrentThreadId());

	DEBUGMSG(ZONE_WARN, (TEXT("-RequestDHCPAddr: Context %d\r\n"),
		Context));

	return Status;
}	// RequestDHCPAddr()


STATUS ReleaseDHCPAddr(PTSTR		 pAdapter) {



	DhcpInfo	**ppDhcp, *pDhcp;
    int			cPkt;
    STATUS		Status;
    DhcpPkt		Pkt;

	DEBUGMSG(ZONE_WARN, (TEXT("+ReleaseDHCPAddr: %s \n"), pAdapter));

	// Find the object to release
	Status = DHCP_NOINTERFACE;
	ppDhcp = _FindDhcp(NULL, pAdapter);
	if (pDhcp = *ppDhcp) {
		// Don't want T1 or T2 timer to fire while we are in the middle
		// of releasing...
		// CTEStopFTimer(&pDhcp->Timer);
		// this won't happen...ie: if it fires, it won't find us on the list

		// Open the socket
		if (DHCP_SUCCESS == (Status = DhcpInitSock(pDhcp, pDhcp->IPAddr))) {
			// Build and transmit the Dhcp Release packet
			BuildDhcpPkt(pDhcp, &Pkt, DHCPRELEASE, NEW_PKT_FL, NULL, &cPkt);
            //
            //	Note that there is no ACK packet for a DHCPRELEASE, so there is
            //	no way to know for certain if a release is successful.
            //
            Status = DhcpSendDown(pDhcp, &Pkt, cPkt, pDhcp->DhcpServer);

			// Inform TCP/IP that the IP address is no longer valid,
			// Remove pDhcp from the global list
			*ppDhcp = pDhcp->pNext;

			CTEFreeLock(&v_GlobalListLock, 0);

			TakeNetDown(pDhcp, TRUE, TRUE);
			// we didn't set it, but in case someone else had...
			ClearDHCPNTE(pDhcp);	
			CloseDhcpSocket(pDhcp);

			CTEFreeLock(&pDhcp->Lock, 0);			
			FreeDhcpInfo(pDhcp);
		} else {
			CTEFreeLock(&v_GlobalListLock, 0);
			CTEFreeLock(&pDhcp->Lock, 0);
		}
	} else {
		CTEFreeLock(&v_GlobalListLock, 0);
	}

	return Status;
}	// ReleaseDHCPAddr()


void DhcpWorkerThread() {
	DhcpInfo	*pInfo;
	STATUS		Status;
	DWORD		Size;
	
	CTEGetLock(&v_GlobalListLock, 0);
	while (pInfo = v_pWaitingList) {
		// take him off the list
		v_pWaitingList = pInfo->pNext;
		v_pCurrent = pInfo;
		pInfo->pNext = NULL;
		CTEFreeLock(&v_GlobalListLock, 0);


		// now try to get an address for him
		if (DHCP_SUCCESS == (Status = DhcpInitSock(pInfo, 0))) {
			
			CTEGetLock(&pInfo->Lock, 0);
			if (! pInfo->Nte) {
				Size = ETH_ADDR_LEN;
				(*pfnSetDHCPNTE)(pInfo->NteContext, &pInfo->Nte, pInfo->PhysAddr, &Size);
			}
			Status = GetParams(pInfo);
			CloseDhcpSocket(pInfo);

			CTEFreeLock(&pInfo->Lock, 0);

			CTEGetLock(&v_GlobalListLock, 0);
			if (v_pDelete == v_pCurrent) {
				if (DHCP_SUCCESS == Status) {
					TakeNetDown(pInfo, FALSE, TRUE);
					// we really shouldn't need to clear him, but to be safer
					ClearDHCPNTE(pInfo);
					Status = DHCP_FAILURE;
				}
				v_pDelete = NULL;
			}
			ASSERT(NULL == v_pDelete);
			v_pCurrent = NULL;
			
			if (DHCP_SUCCESS == Status) {
				DEBUGMSG(ZONE_INIT, 
					(TEXT("Put Adapter in Established list\r\n")));
			
				// cool put him on the established list
				pInfo->pNext = v_pEstablishedList;
				v_pEstablishedList = pInfo;

			} else {
                DEBUGMSG(ZONE_WARN, 
                    (TEXT("\tDhcpWorkerThread: couldn't get Addr for pInfo 0x%X, Context 0x%X\r\n"),
                    pInfo, pInfo->NteContext));
                FreeDhcpInfo(pInfo);
			}
		} else {
			CTEGetLock(&v_GlobalListLock, 0);
			if (v_pDelete == v_pCurrent) {
				v_pDelete = NULL;
			}
			ASSERT(NULL == v_pDelete);
			v_pCurrent = NULL;
			// releaes global list lock since callnetmsgbox blocks our thread
			CTEFreeLock(&v_GlobalListLock, 0);
			FreeDhcpInfo(pInfo);
			CallNetMsgBox(NULL, NMB_FL_EXCLAIM | NMB_FL_OK | NMB_FL_TOPMOST, 
				NETUI_GETNETSTR_NO_IPADDR);
			CTEGetLock(&v_GlobalListLock, 0);
		}
		// we should have the Global Lock before going back up
		DEBUGMSG(ZONE_WARN, 
			(TEXT("\tDhcpWorkerThread: loop 1: WaitList is 0x%X\r\n"),
			v_pWaitingList));
	}
	v_pCurrent = NULL;
	v_fWorkerThread = FALSE;

	DEBUGMSG(ZONE_WARN, 
		(TEXT("-DhcpWorkerThread: WaitList is 0x%X\r\n"),
		v_pWaitingList));
	
	ASSERT(v_GlobalListLock.OwnerThread == (HANDLE)GetCurrentThreadId());
	CTEFreeLock(&v_GlobalListLock, 0);
	ASSERT(v_GlobalListLock.OwnerThread != (HANDLE)GetCurrentThreadId());

}	// DhcpWorkerThread()


STATUS DhcpNotify(uint Code, PTSTR pAdapter, void *Nte, unsigned Context, 
			   char *pAddr, unsigned cAddr) {

	STATUS	Status;

	switch(Code) {
	case DHCP_NOTIFY_DEL_INTERFACE:
		Status = DelDHCPInterface(pAdapter, Nte, Context);
		break;

	case DHCP_REQUEST_ADDRESS:
		Status = RequestDHCPAddr(pAdapter, Nte, Context, pAddr, cAddr);
		break;

	case DHCP_REQUEST_RELEASE:
		Status = ReleaseDHCPAddr(pAdapter);
		break;

	case DHCP_REQUEST_RENEW:
		Status = RenewDHCPAddr(pAdapter, Nte, Context, pAddr, cAddr);
		break;

	// the following are unhandled cases no one should be calling them
	case DHCP_NOTIFY_ADD_INTERFACE:
	default:
		ASSERT(0);
		Status = DHCP_FAILURE;
		break;
	}

	ASSERT(v_GlobalListLock.OwnerThread != (HANDLE)GetCurrentThreadId());

	return Status;

}	// DhcpNotify()



