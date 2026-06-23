// Copyright (c) 1999-2000 Microsoft Corporation.  All rights reserved.
// Layout of Kernel data page. Note that this page is mapped such
// that it has the same physical address and virtual address to allow
// access to the data with and without relocations enabled.

    .struct KPAGE_BASE
KData:          .space  0x800-StackFrameHeaderLength
                                // 2K kernel stack
KStack:         .space  StackFrameHeaderLength
                                // 0x7xx: arg space
lpvTls:         .space  4       // 0x800: current TLS pointer
SystemHandles:  .space 4*32     // 0x804

ReschedFlag:    .space  1       // 0x884: != 0 if NextThread must be called
KNest:          .space  1       // == 1 iff not in kernel
                .space  1       // alignment padding
                .space  1       // alignment padding

                .space  4       // 0x888: (CurMSec) # of msecs since boot
                .space  4       // 0x88c: (DiffMSec) msecs since TimerCallBack
CurAKey:        .space  4       // 0x890: current thread's access key
CurThdPtr:      .space  4       // 0x894: ptr to current THREAD structure
dwPad:          .space  4       // 0x898: was process breakpoint
HandleBase:     .space  4       // 0x89c: base address of HDATA array.
SectionTable:   .space 4*64     // 0x8a0: virtual memory section array
IntrEvents:     .space SYSINTR_MAX_DEVICES*4
                                // 0x9a0: interrupt event pointers
IntrData:       .space SYSINTR_MAX_DEVICES*4
                                // 0xA20: interrupt handler data pointers
BPowerOff:      .space  1       // 0xaa0: power off flag
BProfileOn:     .space  1       // 0xaa1: TRUE if profiling enabled
                .space  2       // 0xaa2: byte padding
PtrAPIRet:      .space  4       // 0xaa4: api call return for kernel mode
PtrCurPrc:      .space  4       // 0xaa8: ptr to current PROCESS structure
NKGp:           .space  4       // 0xaac: kernel's Global pointer
kMSR:           .space  4       // 0xab0: Base kernel MSR value
kPFNMap:        .space  KPFN_CNT // 0xab4 - 0xad4: KVA to Physical mappings

//
//  Machine dependant fields. These are not shared via the KData structure:
//

SaveR6:         .space  4           // 0xad4: scratch save area
TBIndex:        .space  4           // 0xad8: Current tlb index  (PPC403 only)
                .space  0xB00-0xADC // 0xadc: padding

//
//  Kernel Info Table (DWORD KInfoTable[32])
//

KInfoTable:     .space 4*16         // 0xB00: misc. kernel info
PendEvents:     .space  4           // 0xB40: bitmask of pending events
                .space  4*15        // 0xB44: more misc. kernel info
dwKCRes:        .space 4            // 0xB80
                .space  4*11        // 0xB84: more misc kernel info

LowHandlers:    .space 4           // 0xBB0: Low Memory exception handler routines

                .org    KPAGE_BASE+0x1000
KData_size:;                    // end of kernel data page


    .set hWin32, SystemHandles
    .set hCurThread, SystemHandles+4
    .set hCurProc, SystemHandles+8

