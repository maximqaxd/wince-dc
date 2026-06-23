/*++

Copyright (c) 1997-2000  Microsoft Corporation.  All rights reserved.

Module Name:

    odebug.c

Abstract:

    This module implements printing to extra serial port.


--*/
#include <windows.h>
#include "kdp.h"

int dwCurSetting = 0;

char *zonestr[] =  {
	"Move" 
	"Break"
	"API"
	"Trap"
	"Dbg"
	"Ctrl",
	"Undefined",
	"Undefined",
	"Undefined",
	"Undefined",
	"Undefined",
	"Undefined",
	"Undefined",
	"Undefined",
	"Undefined",
	"Undefined",	 
	"Undefined",
	"Undefined",
	"Undefined",
	"Undefined",
	"Undefined",
	"Undefined",
	"Undefined",
	"Undefined",
	"Undefined",
	"Undefined",
	"Undefined",
	"Undefined",
	"Undefined",
	"Undefined",
	"Undefined",
	"Undefined",
};

WCHAR wszBuf[512];
DWORD dwModeFile=TRUE;

int ppshfile;

extern void OEMWriteOtherDebugByte(unsigned char *ch);
extern int NKwvsprintfW(LPWSTR lpOut, LPCWSTR lpFmt, CONST VOID * lpParms, int maxchars);
DWORD WINAPI GetZoneStrings(LPBYTE Buffer);

DWORD WideToSingle(TCHAR *wszBuf)
{
	int i=0,j=0;
	while(wszBuf[i])  {
		((char *)wszBuf)[j] = (char)wszBuf[i];
		j++;
		i++;
	}
	((char *)wszBuf)[j] = 0;
	return j;
}	


VOID NKOtherPrintfW(LPWSTR lpszFmt, ...) 
{
	// Get it into a string
	NKwvsprintfW(wszBuf, lpszFmt,
		(LPVOID)(((DWORD)&lpszFmt)+sizeof(lpszFmt)), sizeof(wszBuf)/sizeof(WCHAR));

    U_rwrite (ppshfile, (char *)wszBuf, WideToSingle(wszBuf));
//    for (wLen=0; wszBuf[wLen]; wLen++)
//        OEMWriteOtherDebugByte(wszBuf[wLen] & 0x00ff);
}

WORD ProcessZone(LPBYTE Buffer)
{
    DWORD dwFlag;
	DWORD dwNewValue;

    dwFlag = (DWORD)*Buffer;
    dwNewValue = (DWORD)*(Buffer + 4);

    if (dwFlag == 0x1) {
        /* ON */
        dwCurSetting |= dwNewValue;
    }
    else {
        /* OFF */
        dwCurSetting = 0;
    }

    return 0;
}	

DWORD xstrlen(char *szStr)
{
	DWORD dwLen=0;
	for(;*szStr;szStr++,dwLen++);
	return dwLen;
}	

DWORD WINAPI GetZoneStrings(LPBYTE Buffer)
{
	int i;
	DWORD wLength;
	DWORD wRet;
	NKOtherPrintfW(TEXT("Inside GetZoneStrings\r\n"));
	memcpy( Buffer, &dwCurSetting, sizeof(DWORD));
	wRet = 4;
	for (i=0; i < 32; i++)  {
		wLength = xstrlen(zonestr[i])+1;
		memcpy( Buffer+wRet, zonestr[i], wLength);
		wRet += wLength;
	}
	Buffer[++wRet] = 0;
	return wRet;
}
