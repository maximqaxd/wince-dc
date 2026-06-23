// --------------------------------------------------------------------------
//
// Copyright (c) 1994-2000 Microsoft Corporation.  All rights reserved.
//
// Module:
//
//    random.c
//
// Purpose:
//
//    Implementation of Random.
//
// --------------------------------------------------------------------------

#include <windows.h>

DWORD sr;

int RandBit() {
	if (!sr)
		if (!(sr = GetTickCount()))
			sr = 1;
    sr = ((((sr>>7) ^ (sr>>5) ^ (sr>>2) ^ (sr>>1) ^ sr) & 1) << 31) |
         (sr >> 1);
    return (sr & 1);
}

DWORD WINAPI Random () {
    DWORD res = 0, loop;
    for (loop = 0; loop < 32; loop++)
        res = ((res<<1) | RandBit());
    return res;
}

