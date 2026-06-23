; Copyright (c) 1995-2000 Microsoft Corporation
	OPT	2	; disable listing
	INCLUDE ksarm.h
	OPT	1	; reenable listing
	OPT	128	; disable listing of macro expansions

	TEXTAREA

	LEAF_ENTRY ThreadStopFunc
	DCD	0xE6000011	; special undefined instruction
	mov	pc, lr

	END
