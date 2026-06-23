// Copyright (c) 1997-2000  Microsoft Corporation.

#include "ksppc.h"

    LEAF_ENTRY(ThreadStopFunc)

    twi 31, r0, 0x16
    blr

