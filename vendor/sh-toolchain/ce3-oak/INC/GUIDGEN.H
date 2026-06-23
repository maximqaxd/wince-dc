/*++

Copyright (c) 1998-2000 Microsoft Corporation.  All rights reserved.

Module Name:

    guidgen.h

Abstract:

    GUID generator for Windows CE

--*/
#if ! defined (__guidgen_H__)
#define __guidgen_H__		1

#if defined (__cplusplus)
extern "C" {
#endif

int CeGenerateGUID (GUID *);
int CeGenerateLUID (LUID *);
int CeGenRandom    (void *pb, int cb);

#if defined (__cplusplus)
};
#endif
#endif
