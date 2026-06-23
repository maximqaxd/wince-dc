// Copyright (c) 1999-2000 Microsoft Corporation.  All rights reserved.
// Layout of last page of storage

	.struct	KPAGE_BASE
KData:	.space	2016		// 2K kernel stack
KStack:	.space	8*4         // 0x7e0: arg space + temps

lpvTls:		.space  4		// 0x800: current TLS pointer
SystemHandles: .space 4*32  // 0x804
ReschedFlag: .space	1       // 0x884: != 0 if NextThread must be called
KNest:      .space  1       // == 1 iff not in kernel
			.space	1		// alignment padding
BPowerOff:	.space	1		// 0x887: power off flag
SaveT0:     .space  4       // 0x888:
SaveK1:     .space  4       // 0x88c:
BasePSR:    .space  4       // 0x890: base value for PSR
    		.space  4       // 0x894 (CurMSec) # of milliseconds since boot
   			.space  4       // 0x898: (DiffMSec) # of milliseconds since TimerCallBack
CurAKey:    .space  4       // 0x89c: current thread's access key
DbgEntry:   .space  4       // 0x8a0: kernel debugger entry point
FalseInt:   .space  4       // 0x8a4: false interrupt entry
ISRTable:   .space  4*6     // 0x8a8: first level intr service routines

SectionTable: .space 4*64   // 0x8c0: virtual memory section array
IntrEvents: .space SYSINTR_MAX_DEVICES*4 // 0x9c0: interrupt event pointers
IntrData:   .space SYSINTR_MAX_DEVICES*4 // 0xA40: interrupt handler data pointers
CurThdPtr:	.space	4		// 0xAc0: ptr to current THREAD structure
CurPrcPtr:	.space	4		// 0xAc4: ptr to current PROCESS structure
HandleBase:	.space	4		// 0xAc8: base address of HDATA array.
PtrAPIRet:	.space	4		// 0xacc: api call return for kernel mode
dwKCRes:	.space  4		// 0xad0
bPadding:		.space 36	// 0xad4
g_CurFPUOwner:	.space 4	// 0xaf8
            .space 0xb00-0xafc  // padding

KInfoTable: .space 4*16     // 0xB00: misc. kernel info
PendEvents:	.space	4		// 0xB40: bitmask of pending events
			.space	4*27	// 0xB44: more misc. kernel info
SaveK0:		.space	4		// 0xBB0
KData_size:;                // 0xBB4 end of kernel data page

#define KPAGE_USER  0x00005000  // KPage address in user space

hWin32 = SystemHandles
hCurThread = SystemHandles+4
hCurProc = SystemHandles+8

