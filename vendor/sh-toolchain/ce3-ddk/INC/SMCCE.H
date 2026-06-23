/*++

Copyright (c) 1998-2000 Microsoft Corporation.  All rights reserved.

Module Name:

    smcce.h

Abstract:

    This files inlcudes the Windows CE specific data structure
    for the smart card library

Environment:

    Kernel mode only.

Notes:

Revision History:

    - Created Oct 1998  

--*/
#include <ntstatus.h>
#include <windows.h>
#include <types.h>
#include <tchar.h>
#include <ceddk.h>


#ifndef RtlMoveMemory
#define RtlMoveMemory(Destination,Source,Length) memmove((Destination),(Source),(Length))
#define RtlCopyMemory(Destination,Source,Length) memcpy((Destination),(Source),(Length))
#define RtlFillMemory(Destination,Length,Fill) memset((Destination),(Fill),(Length))
#define RtlZeroMemory(Destination,Length) memset((Destination),0,(Length))
#endif

typedef struct _OS_DEP_DATA {

	// Pointer to the device object (Must be set by driver)
	struct _SMARTCARD_EXTENSION *pSmartCardExtension;
	CRITICAL_SECTION CritSect;  // used for serializing entry into driver
	HANDLE hChangeEvent;        // signaled on card-insertion event
	HANDLE hCancelEvent;        // signaled on CANCEL_BLOCKING ioctls

} OS_DEP_DATA, *POS_DEP_DATA;



