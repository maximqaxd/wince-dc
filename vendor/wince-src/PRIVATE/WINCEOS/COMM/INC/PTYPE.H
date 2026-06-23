// Copyright (c) 1999-2000 Microsoft Corporation.  All rights reserved.
#define HINSTANCE_ERROR	((HINSTANCE)32)		// from windows.h
#define HFILE_ERROR		((HFILE)-1)			// from windows.h

#define HFILE			int
#define time_t			int

#define PLUID			__int64

#define WSOCK_DEVICE_ID	0x003e				// from netvxd.h


FARPROC WSASetBlockingHook(FARPROC lpBlockFunc);	// from wsamisc.c
int WSACancelBlockingCall( VOID );


#include "cclib.h"	// the C library functions...
