// Copyright (c) 1995-2000 Microsoft Corporation
#include "ksmips.h"

		LEAF_ENTRY(ThreadStopFunc)
	.set noreorder
		break 3
		nop
		j ra
		nop
	.end ThreadStopFunc

