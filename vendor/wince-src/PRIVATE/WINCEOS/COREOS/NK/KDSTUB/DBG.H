/*++

Copyright (c) 1999-2000 Microsoft Corporation.  All rights reserved.

Module Name:

    dbg.h

Abstract:

    dbg module header file

Environment:

    WinCE

--*/

#ifndef _DBGH_
#define _DBGH_


#include "kernel.h"


extern PCALLSTACK pStk;
extern PPROCESS pLastProc;
extern PTHREAD pWalkThread;


//
// Counted String
//

DBGPARAM dpCurSettings;

typedef struct _STRING {
    USHORT Length;
    USHORT MaximumLength;
#ifdef MIDL_PASS
    [size_is(MaximumLength), length_is(Length) ]
#endif // MIDL_PASS
    PCHAR Buffer;
} STRING;
typedef STRING *PSTRING;

typedef STRING ANSI_STRING;
typedef PSTRING PANSI_STRING;

typedef STRING OEM_STRING;
typedef PSTRING POEM_STRING;

//
// CONSTCounted String
//

typedef STRING CANSI_STRING;
typedef PSTRING PCANSI_STRING;


typedef struct _NameandBase
{
    PCHAR   szName;
    ULONG   ImageBase;
    ULONG   ImageSize;
} NAMEANDBASE, *PNAMEANDBASE;


void kdbgWtoA(LPWSTR pWstr, LPCHAR pAstr);
DWORD kdbgwcslen(LPWSTR pWstr);

//
// Define NK function prototypes.
//
BOOL 
GetNameandImageBase(
    PPROCESS pProc, 
    DWORD dwAddress, 
    PNAMEANDBASE pnb, 
    BOOL fRedundant,
    DWORD BreakCode
    );


ULONG
GetProcessInfo(
    LPBYTE lpbBuf, 
    ULONG dwSize
    ); 
    

ULONG 
GetSpecificProcessInfo(
    PPROCESS pProc, 
    LPBYTE lpbBuf, 
    ULONG dwSize
    ); 


BOOL 
GetProcessAndThreadInfo(
    IN PSTRING pParamBuf
    );


ULONG 
VerifyAddress(
    PVOID Addr
    );


BOOL
TranslateRA(
    PULONG lpAddr, 
    PTHREAD pThd,
    DWORD dwStackFrameAddr
    );


BOOL
TranslateAddress(
    PULONG lpAddr
    );


BOOL
SwitchToThread(
    PTHREAD pThd
    );


void 
ClearThreadSwitch(
    PCONTEXT pCtxf
    );


ULONG 
SH3strtoul(
    LPBYTE lpStr, 
    ULONG radix
    );


void 
SetKernLoadAddress(
    void
    );


#endif // _DBGH_

