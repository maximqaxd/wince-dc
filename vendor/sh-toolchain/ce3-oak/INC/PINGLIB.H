/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

Module Name:  

pinglib.h

Abstract:  

    Packet INternet Groper utility for TCP/IP include file.


Notes:


--*/

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MAX_OPT_SIZE
#define MAX_OPT_SIZE				40
#endif
#define DEFAULT_SEND_SIZE           32
#define DEFAULT_COUNT               4
#define DEFAULT_TTL                 32
#define DEFAULT_TOS                 0
#define DEFAULT_TIMEOUT             1000L
#define MIN_INTERVAL                1000L

extern BOOL v_fStopPing;

typedef struct _PINGOPTIONS {
	DWORD	SendSize;			// How many bytes to send
	DWORD	Count;				// Number of times to Ping (-1 too loop)
	DWORD	Timeout;			// Timeout
	BOOL	DnsReq;				// Perform gethostbyaddr() call
	BYTE	Flags;
	BYTE	TTL;				// Time to live
	BYTE	TOS;				// Type of service
	BYTE	OptLength;			// Length of options buffer.
	BYTE	SendOptions[MAX_OPT_SIZE]; // Send options
} PINGOPTIONS, *PPINGOPTIONS;


typedef VOID (WINAPI *PFN_PINGDISPSTR)(LPTSTR szOutStr);

DWORD WINAPI DoPing (LPTSTR szHostName, PPINGOPTIONS pPingOptions,
					 PFN_PINGDISPSTR pPingDispStr);

#ifdef __cplusplus
}
#endif

