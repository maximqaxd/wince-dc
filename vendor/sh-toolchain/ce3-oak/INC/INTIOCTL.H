// Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.
#ifndef _INTIOCTL_H_
#define _INTIOCTL_H_

// Internal IOCTLs for testing purposes

// function code starts from 1001 to avoid conflict with OS function codes
#define IOCTL_SET_TRUST_MODE CTL_CODE(FILE_DEVICE_HAL, 1001, METHOD_BUFFERED, FILE_ANY_ACCESS)
BOOL InternalHalTesting (DWORD dwIoControlCode, LPVOID lpInBuf, DWORD nInBufSize,
	LPVOID lpOutBuf, DWORD nOutBufSize, LPDWORD lpBytesReturned);

#include <pshpack1.h>

#define	MAX_MODULE_LENGTH	30		// should be enough for testing purpose

typedef struct _TRUST_MODULE_INFO {
	USHORT wTrustLvl;
	WCHAR  szModule[MAX_MODULE_LENGTH + 1];
} TRUST_MODULE_INFO, *PTRUST_MODULE_INFO;

typedef struct _TRUST_TEST_DATA {
	DWORD dwLength;		// length of the data structure in bytes
	USHORT wDfltTrust;	// default trust level
	USHORT wFunction;	// Function to perform, 3 available
#define	TRUST_TEST_RESET	0
#define	TRUST_TEST_DEFAULT	1
    // TRUST_TEST_DATA is followed by the PUBLICKEYBLOB
#define	TRUST_TEST_DYNAMIC	2
    // TRUST_TEST_DATA is followed by one or more TRUST_MODULE_INFO structs
} TRUST_TEST_DATA, *PTRUST_TEST_DATA;



#include <poppack.h>

#endif // _INTIOCTL_H_
