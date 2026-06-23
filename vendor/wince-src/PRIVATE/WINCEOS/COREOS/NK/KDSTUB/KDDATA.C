/*++

Copyright (c) 1990-2000 Microsoft Corporation.  All rights reserved.

Module Name:

    kddata.c

Abstract:

    This module contains global data for the portable kernel debgger.

--*/

#include "kdp.h"

BREAKPOINT_ENTRY KdpBreakpointTable[BREAKPOINT_TABLE_SIZE];
UCHAR KdpMessageBuffer[KDP_MESSAGE_BUFFER_SIZE];
