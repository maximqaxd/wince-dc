        TTL ARM Interrupt and Exception Processing
;-------------------------------------------------------------------------------
;++
;
; Copyright (c) 1997-2000 Microsoft Corporation.  All rights reserved.
;
; Module Name:
;
;    armtrap.s
;
; Abstract:
;
;    This module implements the code necessary to field and process ARM
;    interrupt and exception conditions.
;
;    WARNING: This module executes in KSEG0 and, in general, cannot
;       tolerate a TLB Miss. Registers k0 and k1 are used during initial
;       interrupt and exception processing, and therefore, extreme care must
;       be exercised when modifying this module.
;
; Environment:
;
;    Kernel mode only.
;
;--
;-------------------------------------------------------------------------------
        OPT     2       ; disable listing
        INCLUDE ksarm.h
        OPT     1       ; reenable listing




;;; P2 h/w defines
  IF {TRUE}
HKEEP_FPGA_REGS_BASE    EQU     0xA4000000      ; housekeeping FPGA reg base
LED_ALPHA       EQU     HKEEP_FPGA_REGS_BASE+0x00060000
LED_DISCRETE    EQU     HKEEP_FPGA_REGS_BASE+0x00040000
SPIN_DELAY      EQU     0x40000
  ENDIF

BANK_SIZE EQU   0x00100000      ; 1MB per bank in MemoryMap table
BANK_SHIFT EQU  20

        MACRO
        BREAKPOINT
        DCD     0xE6000010      ; special undefined instruction
        MEND

; Resister save frame for Code Aborts, Data Aborts and IRQs.
FrameR0 EQU     0
FrameR1 EQU     4
FrameR2 EQU     8
FrameR3 EQU     12
FrameR12 EQU    16
FrameLR EQU     20

; Def(1) to allow ARM to test 920 kernel as 720. Undef(0) for real life.
ARM920_HACK EQU 0

; High memory layout:
;       FFFD0000 - first level page table (uncached) (2nd half is r/o)
;       FFFD4000 - second level page tables (uncached)
;       FFFE0000 - disabled for protection
;       FFFF0000 - exception vectors
;       FFFF03E0 - exception vector jump table
;       FFFF0400 - not used (r/o)
;       FFFF1000 - disabled for protection
;       FFFF2000 - r/o (physical overlaps with vectors)
;       FFFF2400 - Interrupt stack (1k)
;       FFFF2800 - r/o (physical overlaps with Abort stack)
;       FFFF3000 - disabled for protection
;       FFFF4000 - r/o (physical memory overlaps with vectors & intr. stack & FIQ stack)
;       FFFF4900 - Abort stack (2k - 256 bytes)
;       FFFF5000 - disabled for protection
;       FFFF6000 - r/o (physical memory overlaps with vectors & intr. stack)
;       FFFF6800 - FIQ stack (256 bytes)
;       FFFF6900 - r/o (physical memory overlaps with Abort stack)
;       FFFF7000 - disabled
;       FFFFC000 - kernel stack
;       FFFFC800 - KDataStruct
;       FFFFCC00 - r/o for protection (2nd level page table for 0xFFF00000)

        ^ 0xFFFD0000
FirstPT         # 0x4000
PageTables      # 0x4000
                # 0x8000
                # 0x10000       ; not mapped
ExVector        # 0x1000
                # 0x1400        ; not mapped
                # 0x0400        ; 1K interrupt stack
IntStack        # 0x2000        ; not mapped                    (ffff2800)
                # 0x0100        ; not mapped (FIQ stack)        (ffff4800)
                # 0x0700        ; 2K-256 abort stack            (ffff4900)
AbortStack      # 0x1800        ; not mapped                    (ffff5000)
                # 0x0100        ; not mapped (FIQ stack)        (ffff6800)
FIQStack        # 0xC000-0x6900 ; not mapped                    (ffff6900)
KDBase          # 0x07E0        ; 2K-32 kernel stack
KStack          # 0x0020        ; temporary register save area
KData           # 0x400         ; kernel data area


;* .KDATA area is used to reserve physical memory for the above structures.
        AREA |.KDATA|,DATA,NOINIT
KDataArea
PTs     %       0x4000          ; space for first-level page table
        %       0x4000          ; space for 2nd-level page tables
Vectors %       0x0400          ; space for exception vectors
        %       0x0400          ; space for interrupt stack
        %       0x0100          ; space for FIQ stack
        %       0x0700          ; space for Abort stack
KPage   %       0x0c00          ; space for kernel stack & KDataStruct
HighPT  %       0x0400          ; space for 2nd level page table to map 0xFFF00000
KDEnd   %       0


        MACRO
        mtc15   $cpureg, $cp15reg
        mcr     p15,0,$cpureg,$cp15reg,c0,0
        MEND

        MACRO
        mfc15   $cpureg, $cp15reg
        mrc     p15,0,$cpureg,$cp15reg,c0,0
        MEND



        MACRO
        WRITELED $val
  IF {FALSE}
        stmfd   sp!, {r0-r1}
        ldr     r0, =LED_ALPHA
        ldr     r1, $val
        str     r1, [r0]
        ldmfd   sp!, {r0-r1}
  ENDIF
        MEND

        MACRO
        WRITELED_REG $reg
  IF {FALSE}
        stmfd   sp!, {r0}
        ldr     r0, =LED_ALPHA
        str     $reg, [r0]
        ldmfd   sp!, {r0}
  ENDIF
        MEND



  IF Thumbing

    MACRO
    CALL $Fn
    ldr     r12, =$Fn
    mov     lr, pc
    bx      r12
    MEND

    MACRO
    CALLEQ $Fn
    ldreq   r12, =$Fn
    moveq   lr, pc
    bxeq    r12
    MEND

    MACRO
    RETURN
    bx      lr
    MEND

    MACRO
    RETURN_EQ
    bxeq    lr
    MEND

    MACRO
    RETURN_NE
    bxne    lr
    MEND

  ELSE

    MACRO
    CALL $Fn
    bl      $Fn
    MEND

    MACRO
    CALLEQ $Fn
    bleq    $Fn
    MEND

    MACRO
    RETURN
    mov     pc, lr
    MEND

    MACRO
    RETURN_EQ
    moveq   pc, lr
    MEND

    MACRO
    RETURN_NE
    movne   pc, lr
    MEND

  ENDIF

        AREA |.data|, DATA

        TEXTAREA
        IMPORT  ObjectCall
        IMPORT  ServerCallReturn
        IMPORT  LoadPageTable
        IMPORT  HandleException
        IMPORT  NextThread
        IMPORT  ExceptionDispatch
        IMPORT  OEMInterruptHandler
        IMPORT  OEMInterruptHandlerFIQ
        IMPORT  ARMInit
        IMPORT  KernelInit
        IMPORT  idleconv
        IMPORT  TLBClear
        IMPORT  OEMARMCacheMode

  IF CELOG
        IMPORT  CeLogInterrupt
        IMPORT  CELOG_ThreadMigrateARM
  ENDIF

  IF ARM920_HACK <> 1
  IF _TGTCPU = "ARM720"
        IMPORT  OEMDataAbortHandler
  ENDIF
  ENDIF

  IF {FALSE}
        IMPORT  WhereAmI
        IMPORT  WriteHex
        IMPORT  WriteByte
        IMPORT  PutHex
  ENDIF



;-------------------------------------------------------------------------------
; KernelStart - kernel main entry point
;
;       The OEM layer will setup any platform or CPU specific configuration that is
; required for the kernel to have access to ROM and DRAM and jump here to start up
; the system. Any processor specific cache or MMU initialization should be completed.
; The MMU and caches should not enabled.
;
;       This routine will initialize the first-level page table based up the contents of
; the MemoryMap array and enable the MMU and caches.
;
; NOTE: Until the MMU is enabled, kernel symbolic addresses are not valid and must be
;       translated via the MemoryMap array to find the correct physical address.
;
;       Entry   (r0) = pointer to MemoryMap array in physical memory
;       Exit    returns if MemoryMap is invalid
;-------------------------------------------------------------------------------
        LEAF_ENTRY KernelStart


        mov     r11, r0                         ; (r11) = &MemoryMap (save pointer)
        ldr     r9, =PTs                        ; (r9) = "virtual address" of 1st level table

        ldr     r7, =0x1FF00000                 ; VA needs 512MB, 1MB aligned.
        ldr     r8, =0xFFF00000                 ; PA needs   4GB, 1MB aligned.

        and     r6, r9, r7                      ; (r6) = KDATA Virtual Address (1MB)

        mov     r1, r11                         ; (r1) = ptr to MemoryMap array

5       ldr     r2, [r1], #4                    ; (r2) = virtual address to map Bank at
        ldr     r3, [r1], #4                    ; (r3) = physical address to map from
        ldr     r4, [r1], #4                    ; (r4) = num MB to map

        cmp     r4, #0                          ; End of table?
        moveq   pc, lr                          ; RETURN!!!! INVALID MEMORY MAP. NO MAPPING FOR KDATA AREA!

        and     r2, r2, r7                      ; VA needs 512MB, 1MB aligned.
        and     r3, r3, r8                      ; PA needs 4GB, 1MB aligned.

        ;
        ; For each MB in this table entry, compare with KDATA Virtal Address...
        ;
8
        cmp     r2, r6                          ; KDATA VA = Section VA?
        beq     %F9                             ; Found it!!! (r3) is the PA

        add     r2, r2, #0x00100000             ; (r2) = VA + 1MB
        add     r3, r3, #0x00100000             ; (r3) = PA + 1MB

        sub     r4, r4, #1                      ; Decrement number of MB left
        cmp     r4, #0
        bne     %B8                             ; Map next MB

        b       %B5                             ; Get next element


9
        mov     r4, r9
        mov     r1, r3

;       Found MemoryMap entry for .KDATA section which contains the kernel data page,
; the first-level page table, exception vector page, etc.
;
;       (r1) = base address of bank
;       (r4) = virtual address of FirstPT
;       (r11) = ptr to MemoryMap array

15      mov     r10, #BANK_SIZE
        sub     r10, r10, #1                    ; (r10) = mask for offset
        and     r10, r10, r4                    ; (r10) = offset into bank for FirstPT
        orr     r10, r10, r1                    ; (r10) = ptr to FirstPT

;       Zero out page tables & kernel data page

        mov     r0, #0                          ; (r0-r3) = 0's to store
        mov     r1, #0
        mov     r2, #0
        mov     r3, #0
        mov     r4, r10                         ; (r4) = first address to clear
        add     r5, r10, #KDEnd-PTs             ; (r5) = last address + 1
18      stmia   r4!, {r0-r3}
        stmia   r4!, {r0-r3}
        cmp     r4, r5
        blo     %B18


;       Setup 2nd level page table to map the high memory area which contains the
; first level page table, 2nd level page tables, kernel data page, etc.

        add     r4, r10, #HighPT-PTs            ; (r4) = ptr to high page table
        orr     r0, r10, #0x051                 ; (r0) = PTE for 64K, kr/w kr/w r/o r/o page, uncached unbuffered
        str     r0, [r4, #0xD0*4]               ; store the entry into 8 consecutive slots
        str     r0, [r4, #0xD1*4]
        str     r0, [r4, #0xD2*4]
        str     r0, [r4, #0xD3*4]
        str     r0, [r4, #0xD4*4]
        str     r0, [r4, #0xD5*4]
        str     r0, [r4, #0xD6*4]
        str     r0, [r4, #0xD7*4]
        add     r8, r10, #Vectors-PTs           ; (r8) = ptr to vector page
        bl      OEMARMCacheMode                 ; places C and B bit values in r0 as set by OEM
        mov     r2, r0
        and     r2, r2, #0x0000000C             ; zero out all bits except for the valid C and B positions.
        orr     r0, r8, #0x002                  ; construct the PTE
        orr     r0, r0, r2
        str     r0, [r4, #0xF0*4]               ; store entry for exception vectors
        orr     r0, r0, #0x500                  ; (r0) = PTE for 4k r/o r/o kr/w kr/w C+B page
        str     r0, [r4, #0xF4*4]               ; store entry for abort stack
        str     r0, [r4, #0xF6*4]               ; store entry for FIQ stack  (access permissions overlap for abort and FIQ stacks, same 1k)
        orr     r0, r8, #0x042
        orr     r0, r0, r2                      ; (r0)= PTE for 4K r/o kr/w r/o r/o (C+B as set by OEM)
        str     r0, [r4, #0xF2*4]               ; store entry for interrupt stack
        add     r9, r10, #KPage-PTs             ; (r9) = ptr to kdata page
        orr     r0, r9, #0x002
        orr     r0, r0, r2                      ; (r0)=PTE for 4K (C+B as set by OEM)
        orr     r0, r0, #0x250                  ; (r0) = set perms kr/w kr/w kr/w+ur/o r/o
        str     r0, [r4, #0xFC*4]               ; store entry for kernel data page
        orr     r0, r4, #0x001                  ; (r0) = 1st level PTE for high memory section
        add     r1, r10, #0x4000
        str     r0, [r1, #-4]                   ; store PTE in last slot of 1st level table
  IF {FALSE}
        mov     r0, r4
        mov     r1, #256                        ; dump 256 words
        CALL    WriteHex
  ENDIF

;       Fill in first level page table entries to create "un-mapped" regions
; from the contents of the MemoryMap array.
;
;       (r9) = ptr to KData page
;       (r10) = ptr to 1st level page table
;       (r11) = ptr to MemoryMap array

        add     r10, r10, #0x2000               ; (r10) = ptr to 1st PTE for "unmapped space"

        mov         r0, #0x02
        orr     r0, r0, r2                      ; (r0)=PTE for 0: 1MB (C+B as set by OEM)
        orr     r0, r0, #0x400                  ; set kernel r/w permission
20      mov     r1, r11                         ; (r1) = ptr to MemoryMap array


25      ldr     r2, [r1], #4                    ; (r2) = virtual address to map Bank at
        ldr     r3, [r1], #4                    ; (r3) = physical address to map from
        ldr     r4, [r1], #4                    ; (r4) = num MB to map

        cmp     r4, #0                          ; End of table?
        beq     %F29

        ldr     r5, =0x1FF00000
        and     r2, r2, r5                      ; VA needs 512MB, 1MB aligned.

        ldr     r5, =0xFFF00000
        and     r3, r3, r5                      ; PA needs 4GB, 1MB aligned.

        add     r2, r10, r2, LSR #18
        add     r0, r0, r3                      ; (r0) = PTE for next physical page

28      str     r0, [r2], #4
        add     r0, r0, #0x00100000             ; (r0) = PTE for next physical page

        sub     r4, r4, #1                      ; Decrement number of MB left
        cmp     r4, #0
        bne     %B28                            ; Map next MB

        bic     r0, r0, #0xF0000000             ; Clear Section Base Address Field
        bic     r0, r0, #0x0FF00000             ; Clear Section Base Address Field
        b       %B25                            ; Get next element


29
        tst     r0, #8
        bic     r0, r0, #0x0C                   ; clear cachable & bufferable bits in PTE
        add     r10, r10, #0x0800               ; (r10) = ptr to 1st PTE for "unmapped uncached space"
        bne     %B20                            ; go setup PTEs for uncached space
        sub     r10, r10, #0x3000               ; (r10) = restore address of 1st level page table
  IF {FALSE}
        mov     r0, r10
        mov     r1, #4096                       ; dump 4096 words
        CALL    WriteHex
  ENDIF

; Setup the vector area.
;
;       (r8) = ptr to exception vectors

        add     r7, pc, #VectorInstructions - (.+8)
        ldmia   r7!, {r0-r3}                    ; load 4 instructions
        stmia   r8!, {r0-r3}                    ; store the 4 vector instructions
        ldmia   r7!, {r0-r3}                    ; load 4 instructions
        stmia   r8!, {r0-r3}                    ; store the 4 vector instructions
  IF {FALSE}
        sub     r0, r8, #8*4
        mov     r1, #8  ; dump 8 words
        CALL    WriteHex
  ENDIF
        add     r8, r8, #0x3E0-(8*4)
        ldmia   r7!, {r0-r3}
        stmia   r8!, {r0-r3}
        ldmia   r7!, {r0-r3}
        stmia   r8!, {r0-r3}
  IF {FALSE}
        sub     r0, r8, #8*4
        mov     r1, #8  ; dump 8 words
        CALL    WriteHex
  ENDIF

; The page tables and exception vectors are setup. Initialize the MMU and turn it on.

        mov     r1, #1
        mtc15   r1, c3                          ; setup access to domain 0
        mtc15   r10, c2
        bl      TLBClear
        mfc15   r1, c1
        orr     r1, r1, #0x007F                 ; changed to read-mod-write for ARM920 Enable: MMU, Align, DCache, WriteBuffer
        orr     r1, r1, #0x3200                 ; vector adjust, ICache, ROM protection
        ldr     r0, VirtualStart
        cmp     r0, #0                          ; make sure no stall on "mov pc,r0" below
        mtc15   r1, c1                          ; enable the MMU & Caches
        mov     pc, r0                          ;  & jump to new virtual address
        nop

; MMU & caches now enabled.
;
;       (r10) = physcial address of 1st level page table

VStart  ldr     sp, =KStack
        add     r4, sp, #KData-KStack           ; (r4) = ptr to KDataStruct
        mov     r0, #0x80000000
        str     r0, [r4, #hBase]                ; set HandleBase
        ldr     r0, =DirectRet
        str     r0, [r4, #pAPIReturn]           ; set DirectReturn address
        add     r1, r10, #0x4000                ; (r1) = physical addr of 2nd page table pool
        orr     r1, r1, #1                      ; (r1) = page table descriptor for pool entries
        str     r1, [r4, #ptDesc]               ; save for LoadPageTable()

; Initialize stacks for each mode.
        msr     cpsr_c, #ABORT_MODE:OR:0xC0     ; switch to Abort Mode w/IRQs disabled
        add     sp, r4, #AbortStack-KData
        mov     r1, sp
        msr     cpsr_c, #IRQ_MODE:OR:0xC0       ; switch to IRQ Mode w/IRQs disabled
        add     sp, r4, #IntStack-KData
        mov     r2, sp

        msr     cpsr_c, #FIQ_MODE:OR:0xC0       ; switch to FIQ Mode w/IRQs disabled
        add     sp, r4, #FIQStack-KData

        msr     cpsr_c, #UNDEF_MODE:OR:0xC0     ; switch to Undefined Mode w/IRQs disabled
        mov     sp, r4                          ; (sp_undef) = &KData
        mov     r3, sp
        msr     cpsr_c, #SVC_MODE:OR:0xC0       ; switch to Supervisor Mode w/IRQs disabled
        mfc15   r0, c0                          ; (r0) = processor ID
        CALL    ARMInit

        msr     cpsr_c, #SVC_MODE               ; switch to Supervisor Mode w/IRQs enabled
        CALL    KernelInit                      ; initialize scheduler, etc.
        mov     r0, #0                          ; no current thread
        mov     r1, #ID_RESCHEDULE
        b       FirstSchedule

VirtualStart DCD VStart

VectorInstructions
        BREAKPOINT                              ; reset
        ldr     pc, [pc, #0x3E0-8]              ; undefined instruction
        ldr     pc, [pc, #0x3E0-8]              ; SVC
        ldr     pc, [pc, #0x3E0-8]              ; Prefetch abort
        ldr     pc, [pc, #0x3E0-8]              ; data abort
        BREAKPOINT                              ; unused vector location
        ldr     pc, [pc, #0x3E0-8]              ; IRQ
        ldr     pc, [pc, #0x3E0-8]              ; FIQ
VectorTable
        DCD     -1
        DCD     UndefException
        DCD     SWIHandler
        DCD     PrefetchAbort

        IF ARM920_HACK = 1
        DCD DataAbortHandler
        ELSE
        IF _TGTCPU = "ARM720"
        DCD     OEMDataAbortHandler
        ELSE
        DCD     DataAbortHandler
        ENDIF
        ENDIF

        DCD     -1                              ; unused vector
        DCD     IRQHandler
        DCD     FIQHandler

        LTORG



;-------------------------------------------------------------------------------
;-------------------------------------------------------------------------------
        NESTED_ENTRY    UndefException
 IF Interworking :LOR: Thumbing
        stmdb   sp, {r0-r3, lr}
        mrs     r0, spsr
        tst     r0, #THUMB_STATE                ; Entered from Thumb Mode ?
        subne   lr, lr, #2                      ; Update return address
        subeq   lr, lr, #4                      ;   accordingly
        str     lr, [sp, #-4]                   ; update lr on stack
 ELSE
        sub     lr, lr, #4                      ; (lr) = address of undefined instruction
        stmdb   sp, {r0-r3, lr}
 ENDIF
        PROLOG_END
  IF {FALSE}
        ldr     r0, =LED_ALPHA
10      ldr     r1, =0x0DEF
        str     r1, [r0]
        mov     r1, #SPIN_DELAY
15      subs    r1, r1, #1
        bgt     %B15
        str     lr, [r0]
        mov     r1, #SPIN_DELAY
17      subs    r1, r1, #1
        bgt     %B17
  ENDIF

        mov     r1, #ID_UNDEF_INSTR
        b       CommonHandler
        ENTRY_END UndefException

        LEAF_ENTRY SWIHandler
  IF {FALSE}
        ldr     r0, =LED_ALPHA
10      ldr     r1, =0x99999999
        str     r1, [r0]
        mov     r1, #SPIN_DELAY
15      subs    r1, r1, #1
        bgt     %B15
        str     lr, [r0]
        mov     r1, #SPIN_DELAY
17      subs    r1, r1, #1
        bgt     %B17
        b       %B10
  ENDIF
        movs    pc, lr
        ENTRY_END SWIHandler



;-------------------------------------------------------------------------------
;-------------------------------------------------------------------------------
        NESTED_ENTRY FIQHandler
        sub     lr, lr, #4                      ; fix return address
        stmfd   sp!, {r0-r3, r12, lr}
        PROLOG_END

        CALL    OEMInterruptHandlerFIQ

        ldmfd   sp!, {r0-r3, r12, pc}^          ; restore regs & return for NOP
        ENTRY_END FIQHandler


        LTORG

;-------------------------------------------------------------------------------
;++
; The following code is never executed. Its purpose is to support unwinding
; through the calls to API dispatch and return routines.
;--
;-------------------------------------------------------------------------------
        NESTED_ENTRY    PrefetchAbortEH
        stmfd   sp!, {r0-r3}
        sub     sp, sp, #8
        PROLOG_END

        ALTERNATE_ENTRY PrefetchAbort

        sub     lr, lr, #0xF0000004
        cmp     lr, #0x00010400
        bhs     ProcessPrefAbort
        mrs     r12, spsr
        and     r12, r12, #0x0ff
        orr     r12, r12, lr, LSL #8            ; (r12) = old mode + 24 lower bits of LR.
        msr     cpsr_c, #SYSTEM_MODE            ; switch to System Mode

; Find the address of the API function to dispatch to by:
;  ObjectCall(int *pMode, RETADDR ra, void *args, long iMethod)
;
;       (r0-r3) = function parameters or return values
;       (r12) = abort address - 0xF0000000 (bits 31-8) (encodes API set & method index)
;               combined with the previous mode (bits 7-0)

        stmfd   sp!, {r0-r3}                    ; save function parameters onto the stack
        and     r0, r12, #0x1f                  ; (r0) = previous mode (User/Kernel)
        stmfd   sp!, {r0, r9}                   ; pass mode & static base to ObjectCall
        mov     r0, sp                          ; (r0) = ptr to mode & extra info
        mov     r3, #FIRST_METHOD - 0xF0000000
        rsb     r3, r3, r12, LSR #8
        mov     r3, r3, ASR #2                  ; (r3) = iMethod
        cmp     r3, #-1
        beq     SysRet                          ; system call return
        add     r2, sp, #8                      ; (r2) = ptr to function args
        mov     r1, lr                          ; (r1) = return address
        CALL    ObjectCall
        mov     r12, r0                         ; (r12) = API function address

        ldr     r0, [sp]                        ; (r0) = target mode
        add     sp, sp, #8                      ; restore stack (ignore extra field)

        cmp     r0, #USER_MODE
        ldmfd   sp!, {r0-r3}                    ; reload function paramters
        addne   lr, pc, #DirectRet-(.+8)
  IF Interworking :LOR: Thumbing
        bxne    r12
  ELSE
        movne   pc, r12                         ; invoke function in "kernel mode"
  ENDIF
        ldr     lr, =SYSCALL_RETURN             ; special return address for API calls
        msr     cpsr_c, #SVC_MODE:OR:0xC0       ; switch to Supervisor Mode w/IRQs disabled

        tst     r12, #0x01                      ; continuing in Thumb mode ??
        msrne   spsr_c, #USER_MODE | THUMB_STATE   ; Set Thumb mode bit
        msreq   spsr_c, #USER_MODE              ; else continuing in ARM mode

        movs    pc, r12                         ; switch to User Mode & jump to API function

; Direct system call return. Used for API calls made in kernel mode or to
; kernel mode functions.

DirectRet
        stmfd   sp!, {r0-r3}                    ; save return values
        mov     r0, #SYSTEM_MODE
        stmfd   sp!, {r0, r9}                   ; pass mode & extra info (unused)
        mov     r0, sp                          ; (r0) = ptr to mode & extra info

; System call return.
;
;       (r0) = ptr to mode & static base
;       (TOS) = mode
;       (TOS+4) = static base
;       (TOS+8) = saved r0-r3

SysRet
        CALL    ServerCallReturn
        mov     r12, r0                         ; (r12) = return address
        ldr     r0, [sp]                        ; (r0) = target mode
        add     sp, sp, #8                      ; retore stack (ignore extra field)

        cmp     r0, #USER_MODE                  ; return to user mode ??
        ldmfd   sp!, {r0-r3}                    ; reload return values

  IF Interworking :LOR: Thumbing
        bxne    r12
  ELSE
        movne   pc, r12                         ; return in "kernel mode"
  ENDIF

        msr     cpsr_c, #SVC_MODE:OR:0xC0       ; switch to SVC Mode w/IRQs disabled

        tst     r12, #0x01                      ; returning to Thumb mode ??
        msrne   spsr_c, #USER_MODE | THUMB_STATE
        msreq   spsr_c, #USER_MODE              ; set the Psr according to returning state

        movs    pc, r12                         ; switch to User Mode and return

        ENTRY_END PrefetchAbortEH

; Process a prefetch abort.

ProcessPrefAbort
        add     lr, lr, #0xF0000000             ; repair continuation address
        stmfd   sp!, {r0-r3, r12, lr}
  IF {FALSE}
        ldr     r0, =LED_ALPHA
10      ldr     r1, =0xC0DEAB00
        str     r1, [r0]
        mov     r1, #SPIN_DELAY
15      subs    r1, r1, #1
        bgt     %B15
        str     lr, [r0]
        mov     r1, #SPIN_DELAY
17      subs    r1, r1, #1
        bgt     %B17
  ENDIF
        mov     r0, lr                          ; (r0) = faulting address
        mfc15   r2, c13                         ; (r2) = process base address
        tst     r0, #0xFE000000                 ; slot 0 reference?
        orreq   r0, r0, r2                      ; (r0) = process slot based address
        CALL    LoadPageTable                   ; (r0) = !0 if entry loaded
        tst     r0, r0
        ldmnefd sp!, {r0-r3, r12, pc}^          ; restore regs & continue
        ldmfd   sp!, {r0-r3, r12}
        ldr     lr, =KData-4
        stmdb   lr, {r0-r3}
        ldmfd   sp!, {r0}
        str     r0, [lr]                        ; save resume address
        mov     r1, #ID_PREFETCH_ABORT          ; (r1) = exception ID
        ;;b     CommonHandler


        LTORG

;-------------------------------------------------------------------------------
; Common fault handler code.
;
;       The fault handlers all jump to this block of code to process an exception
; which cannot be handled in the fault handler or to reschedule.
;
;       (r1) = exception ID
;       original r0-r3 & lr saved at KData-0x14.
;-------------------------------------------------------------------------------
        ALTERNATE_ENTRY CommonHandler
        mrs     r2, spsr
        msr     cpsr_c, #SVC_MODE:OR:0xC0       ; switch to Supervisor mode w/IRQs disabled
        ldr     r3, =KData                      ; (r3) = ptr to KData page


; Save the processor state into a thread structure. If the previous state was
; User or System and the kernel isn't busy, then save the state into the current
; thread. Otherwise, create a temporary thread structure on the kernel stack.
;
;       (r1) = exception ID
;       (r2) = SPSR
;       (r3) = ptr to KData page
;       Interrupted r0-r3, and Pc saved at (r3-0x14)
;       In Supervisor Mode.
        ALTERNATE_ENTRY SaveAndReschedule
  IF {FALSE}
        ldrb    r0, [r3,#cNest]                 ; (r0) = kernel nest level
        subs    r0, r0, #1                      ; in a bit deeper (0 if first entry)
        strb    r0, [r3,#cNest]                 ; save new nest level
  ENDIF
        and     r0, r2, #0x1f                   ; (r0) = previous mode
        cmp     r0, #USER_MODE                  ; 'Z' set if from user mode
        cmpne   r0, #SYSTEM_MODE                ; 'Z' set if from System mode
        bne     %F50                            ; reentering kernel, save state on stack

        ldr     r0, [r3,#pCurThd]               ; (r0) = ptr to current thread
        add     r0, r0, #TcxR4                  ; (r0) = ptr to r4 save
        stmia   r0, {r4-r14}^                   ; save User bank registers
10      ldmdb   r3, {r3-r7}                     ; load saved r0-r3 & Pc
        stmdb   r0!, {r2-r6}                    ; save Psr, r0-r3
        sub     r0, r0, #THREAD_CONTEXT_OFFSET  ; (r0) = ptr to Thread struct
        str     r7, [r0,#TcxPc]                 ; save Pc
        mfc15   r2, c6                          ; (r2) = fault address
        mfc15   r3, c5                          ; (r3) = fault status

; Process an exception or reschedule request.

FirstSchedule
20      msr     cpsr_c, #SVC_MODE               ; enable interrupts
        CALL    HandleException
        ldr     r2, [r0, #TcxPsr]               ; (r2) = target status
        and     r1, r2, #0x1f                   ; (r1) = target mode
        cmp     r1, #USER_MODE
        cmpne   r1, #SYSTEM_MODE
        bne     %F30                            ; not going back to user or system mode
        add     r0, r0, #TcxR3
        ldmia   r0, {r3-r14}^                   ; reload user/system mode registers
        ldr     r1, =KData
        msr     cpsr_c, #SVC_MODE:OR:0xC0       ; disable all interrupts
        ldrb    r1, [r1, #bResched]             ; (r1) = nest level + reschedule flag
        cmp     r1, #1
        mov     r1, #ID_RESCHEDULE
        beq     %B20                            ; interrupted, reschedule again
        msr     spsr, r2
        ldr     lr, [r0, #TcxPc-TcxR3]
        ldmdb   r0, {r0-r2}
        movs    pc, lr                          ; return to user or system mode

; Return to a non-preemptible privileged mode.
;
;       (r0) = ptr to THREAD structure
;       (r2) = target mode

30
  IF Interworking :LOR: Thumbing

        tst     r2, #THUMB_STATE                ; returning to Thumb code ?
        addne   r1, r0, #THREAD_CONTEXT_OFFSET  ; create pointer to context
        bne     ThumbDispatch                   ; and branch to thumb dispatch code

  ENDIF

        msr     cpsr, r2                        ; switch to target mode
        add     r0, r0, #TcxR0
        ldmia   r0, {r0-r15}                    ; reload all registers & return

; Save registers for fault from a non-preemptible state.

50      sub     sp, sp, #TcxSizeof              ; allocate space for temp. thread structure
        cmp     r0, #SVC_MODE
        bne     %F55                            ; must mode switch to save state
        add     r0, sp, #TcxR4                  ; (r0) = ptr to r4 save area
        stmia   r0, {r4-r14}                    ; save SVC state registers
        add     r4, sp, #TcxSizeof              ; (r4) = old SVC stack pointer
        str     r4, [r0, #TcxSp-TcxR4]          ; update stack pointer value
        b       %B10

55
  IF Interworking :LOR: Thumbing
        bic     r0, r2, #THUMB_STATE            ; Ensure Thumb bit is clear
        msr     cpsr, r0                        ; and switch to mode exception came from
  ELSE
        msr     cpsr, r2                        ; switch to mode exception came from
  ENDIF

        add     r0, sp, #TcxR4                  ; (r0) = ptr to r4 save area
        stmia   r0, {r4-r14}                    ; save mode's register state
        msr     cpsr_c, #SVC_MODE:OR:0xC0       ; back to supervisor mode
        b       %B10                            ; go save remaining state

        LTORG


;-------------------------------------------------------------------------------
;-------------------------------------------------------------------------------
        NESTED_ENTRY    DataAbortHandler
        sub     lr, lr, #8                      ; repair continuation address
        stmfd   sp!, {r0-r3, r12, lr}
        PROLOG_END

        sub     r0, lr, #INTERLOCKED_START
        cmp     r0, #INTERLOCKED_END-INTERLOCKED_START
        bllo    CheckInterlockedRestart
        mfc15   r0, c6                          ; (r0) = FAR
        mfc15   r1, c5                          ; (r1) = FSR

  IF {FALSE}
        ldr     r2, =LED_DISCRETE
        str     r1, [r2]
        ldr     r2, =LED_ALPHA
10      ldr     r3, =0xDA2AAB00
        str     r3, [r2]
        mov     r3, #SPIN_DELAY
15      subs    r3, r3, #1
        bgt     %B15
        str     lr, [r2]
        mov     r3, #SPIN_DELAY
17      subs    r3, r3, #1
        bgt     %B17
        str     r0, [r2]
        mov     r3, #SPIN_DELAY
19      subs    r3, r3, #1
        bgt     %B19
;;;     b       %B10
  ENDIF

        mfc15   r2, c13                         ; (r2) = process base address
        tst     r0, #0xFE000000                 ; slot 0 reference?
        orreq   r0, r0, r2                      ; (r0) = process slot based address
        and     r1, r1, #0x0D                   ; type of data abort
        cmp     r1, #0x05                       ; translation error?
        movne   r0, #0
        CALLEQ  LoadPageTable                   ; (r0) = !0 if entry loaded
        tst     r0, r0
        ldmnefd sp!, {r0-r3, r12, pc}^          ; restore regs & continue
        ldr     lr, =KData-4
        ldmfd   sp!, {r0-r3, r12}
        stmdb   lr, {r0-r3}
        ldmfd   sp!, {r0}
        str     r0, [lr]                        ; save resume address
        mov     r1, #ID_DATA_ABORT              ; (r1) = exception ID
        b       CommonHandler

        ENTRY_END DataAbortHandler



;-------------------------------------------------------------------------------
;-------------------------------------------------------------------------------
        NESTED_ENTRY IRQHandler
        sub     lr, lr, #4                      ; fix return address
        stmfd   sp!, {r0-r3, r12, lr}
        PROLOG_END

  IF {FALSE}
        ldr     r0, =LED_ALPHA
10      ldr     r1, =0x09090a0a
        str     r1, [r0]
        mov     r1, #SPIN_DELAY
15      subs    r1, r1, #1
        bgt     %B15
        str     lr, [r0]
        mov     r1, #SPIN_DELAY
17      subs    r1, r1, #1
        bgt     %B17
  ENDIF
        ;
        ; Test interlocked API status.
        ;
        sub     r0, lr, #INTERLOCKED_START
        cmp     r0, #INTERLOCKED_END-INTERLOCKED_START
        bllo    CheckInterlockedRestart

  IF CELOG
        mov     r0,#0x80000000                  ; mark as ISR entry
        stmfd   sp!, {r0}
        CALL    CeLogInterrupt
        ldmfd   sp!, {r0}
        
        ;
        ; mark that we are going a level deeper
        ;
        
        ldr     r1, =KData
        ldrsb   r0, [r1,#cNest]                 ; (r0) = kernel nest level
        add     r0, r0, #1                      ; in a bit deeper (0 if first entry)
        strb    r0, [r1,#cNest]                 ; save new nest level
  ENDIF

	;
        ; CAREFUL! The stack frame is being altered here. It's ok since
        ; the only routine relying on this was the Interlock Check. Note that
        ; we re-push LR onto the stack so that the incoming argument area to
        ; OEMInterruptHandler will be correct.
        ;
        mrs     r1, spsr                        ; (r1) = saved status reg
        stmfd   sp!, {r1}                       ; save SPSR onto the IRQ stack
        mov     r0,lr                           ; parameter to OEMInterruptHandler

        msr     cpsr_c, #SVC_MODE:OR:0x80       ; switch to supervisor mode w/IRQs disabled
        stmfd   sp!, {lr}                       ; save LR onto the SVC stack
        stmfd   sp!, {r0}                       ; save IRQ LR (in R0) onto the SVC stack (param)

        ;
        ; Now we call the OEM's interrupt handler code. It is up to them to
        ; enable interrupts if they so desire. We can't do it for them since
        ; there's only on interrupt and they haven't yet defined their nesting.
        ;

	CALL    OEMInterruptHandler

        ldmfd   sp!, {r1}                       ; dummy pop (parameter)
        ldmfd   sp!, {lr}                       ; restore SVC LR from the SVC stack
        msr     cpsr_c, #IRQ_MODE:OR:0x80       ; switch back to IRQ mode w/IRQs disabled

        ;
        ; Restore the saved program status register from the stack.
        ;
        ldmfd   sp!, {r1}                       ; restore IRQ SPSR from the IRQ stack
        msr     spsr, r1                        ; (r1) = saved status reg

  IF CELOG
        ;
        ; call CeLogInterrupt with cNest << 16 | SYSINTR
        ;
        stmfd   sp!, {r0}
        
        ldr     lr, =KData
        ldrsb   r1, [lr,#cNest]                 ; (r1) = cNest value (1, 2, etc)
        sub     r1, r1, #1                      ; make one level less
        orr     r0, r0, r1, LSL #16             ; (r0) = cNest value << 16 + SYSINTR
        strb    r1, [lr,#cNest]        
        
        stmfd   sp!, {r0}
        CALL    CeLogInterrupt
        ldmfd   sp!, {r0}
        
        ldmfd   sp!, {r0}
  ENDIF

        ldr     lr, =KData                      ; (lr) = ptr to KDataStruct
        cmp     r0, #SYSINTR_RESCHED
        beq     %F10
        sub     r0, r0, #SYSINTR_DEVICES
        cmp     r0, #SYSINTR_MAX_DEVICES
        ;
        ; If not a device request (and not SYSINTR_RESCHED)
        ;        
        ldrhsb  r0, [lr, #bResched]             ; (r0) = reschedule flag
        bhs     %F20                            ; not a device request
        
        ldr     r2, [lr, #PendEvents]           ; (r2) = pending interrupt event mask
        mov     r1, #1
        orr     r2, r2, r1, LSL r0              ; (r2) = new pending mask
        str     r2, [lr, #PendEvents]           ; save it

        ;
        ; mark reschedule needed
        ;
10      ldrb    r0, [lr, #bResched]             ; (r0) = reschedule flag
        orr     r0, r0, #1                      ; set "reschedule needed bit"
        strb    r0, [lr, #bResched]             ; update flag

20      mrs     r1, spsr                        ; (r1) = saved status register value
        and     r1, r1, #0x1F                   ; (r1) = interrupted mode
        cmp     r1, #USER_MODE                  ; previously in user mode?
        cmpne   r1, #SYSTEM_MODE                ; if not, was it system mode?
        cmpeq   r0, #1                          ; user or system: is resched == 1
        ldmnefd sp!, {r0-r3, r12, pc}^          ; can't reschedule right now so return
        
        sub     lr, lr, #4
        ldmfd   sp!, {r0-r3, r12}
        stmdb   lr, {r0-r3}
        ldmfd   sp!, {r0}
        str     r0, [lr]                        ; save resume address
        mov     r1, #ID_RESCHEDULE              ; (r1) = exception ID

        b       CommonHandler

        ENTRY_END IRQHandler

        ;;;NESTED_ENTRY FIQResched



;-------------------------------------------------------------------------------
; CheckInterlockedRestart - check for restarting an InterlockedXXX API call
;
;       This routine is called by the Data Abort and IRQ handlers when the PC of
; the aborted or interrupted instruction is inside the interlocked api region
; contained at the tail of the kernel data page.  If the PC points to a MOV
; instruction then the operation has completed so no restart is needed.
; Otherwise, a backwards scan is made to look for a "ldr r12, [r0]" instruction
; which is the beginning of all of the interlocked api routines.
;
;       Entry   (r0) = interrupted PC - INTERLOCKED_START
;               (sp) = ptr to saved registers ({r0-r3, r12, lr}
;       Exit    LR value in register frame updated if necessary
;       Uses    r0, r1, r2
;-------------------------------------------------------------------------------
        LEAF_ENTRY CheckInterlockedRestart
  IF {FALSE}
        ldr     r1, =LED_ALPHA
        ldr     r2, =0xCEFF00AA
        str     r2, [r1]
  ENDIF
        add     r0, r0, #INTERLOCKED_START      ; (r0) = interrupted PC
        ldr     r1, [r0]                        ; (r1) = interrupted instruction
        ldr     r2, =0xE1A0                     ; (r2) = "mov rx, ry" opcode
        cmp     r2, r1, LSR #16                 ; is it a MOV instruction?
  IF Interworking :LOR: Thumbing
        ldrne   r2, =0xE12FFF1                  ; or is it a BX instruction ?
        cmpne   r2, r1, LSR #4
  ENDIF
        moveq   pc, lr                          ; Y: return to caller
        ldr     r2, =0xE590C000                 ; (r2) = "ldr r12, [r0]" instruction
        cmp     r1, r2                          ; at start of routine?
        moveq   pc, lr                          ;   Y: return to caller
10      ldr     r1, [r0, #-4]!                  ; (r1) = previous instruction
        cmp     r1, r2                          ; found start of routine?
        bne     %B10                            ;   N: keep backing up
        str     r0, [sp, #FrameLR]              ; update return address in stack frame

  IF {FALSE}
        ldr     r1, =LED_ALPHA
        ldr     r2, =0xCEBACBAC
        str     r2, [r1]
  ENDIF
        mov     pc, lr                          ; return



;-------------------------------------------------------------------------------
;-------------------------------------------------------------------------------
        LEAF_ENTRY ZeroPage
;       void ZeroPage(void *vpPage)
;
;       Entry   (r0) = (vpPage) = ptr to address of page to zero
;       Return  none
;       Uses    r0-r3, r12

        mov     r1, #0
        mov     r2, #0
        mov     r3, #0
        mov     r12, #0
10      stmia   r0!, {r1-r3, r12}               ; clear 16 bytes (64 bytes per loop)
        stmia   r0!, {r1-r3, r12}
        stmia   r0!, {r1-r3, r12}
        stmia   r0!, {r1-r3, r12}
        tst     r0, #0xFF0
        bne     %B10

        RETURN                                  ; return to caller



;-------------------------------------------------------------------------------
;-------------------------------------------------------------------------------
        LEAF_ENTRY GetHighPos

        add     r1, pc, #PosTable-(.+8)
        mov     r2, #-1
        and     r3, r0, #0xff
        ldrb    r3, [r1, +r3]
        teq     r3, #0
        bne     done

        mov     r0, r0, lsr #8
        add     r2, r2, #8
        and     r3, r0, #0xff
        ldrb    r3, [r1, +r3]
        teq     r3, #0
        bne     done

        mov     r0, r0, lsr #8
        add     r2, r2, #8
        and     r3, r0, #0xff
        ldrb    r3, [r1, +r3]
        teq     r3, #0
        bne     done

        mov     r0, r0, lsr #8
        add     r2, r2, #8
        and     r3, r0, #0xff
        ldrb    r3, [r1, +r3]
        teq     r3, #0
        bne     done

        add     r3, r3, #9
done
        add     r0, r3, r2
        RETURN                                  ; return to caller


PosTable
       DCB 0,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1,5,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1
       DCB 6,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1,5,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1
       DCB 7,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1,5,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1
       DCB 6,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1,5,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1
       DCB 8,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1,5,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1
       DCB 6,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1,5,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1
       DCB 7,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1,5,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1
       DCB 6,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1,5,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1



;-------------------------------------------------------------------------------
; INTERRUPTS_ON/_OFF - enable or disable interrupts
;-------------------------------------------------------------------------------
        LEAF_ENTRY INTERRUPTS_ON
        mrs     r0, cpsr                        ; (r0) = current status
        bic     r1, r0, #0xC0                   ; clear interrupt disable bit
        msr     cpsr, r1                        ; update status register

        RETURN                                  ; return to caller


;-------------------------------------------------------------------------------
;-------------------------------------------------------------------------------
        LEAF_ENTRY INTERRUPTS_OFF
        mrs     r0, cpsr                        ; (r0) = current status
        orr     r1, r0, #0xC0                   ; set interrupt disable bit
        msr     cpsr, r1                        ; update status register

        RETURN                                  ; return to caller



;-------------------------------------------------------------------------------
; SetCPUASID - setup processor specific address space information
;
;  SetCPUASID will update the virtual memory tables so that new current process
; is mapped into slot 0 and set the h/w address space ID register for the new
; process.
;
;       Entry   (r0) = ptr to THREAD structure
;       Return  nothing
;       Uses    r0-r3, r12
;-------------------------------------------------------------------------------
        LEAF_ENTRY SetCPUASID

  IF CELOG
        ldr     r1, [r0, #ThProc]               ; (r1) = ptr to new process
        ldr     r2, [r1, #PrcHandle]            ; (r2) = handle of new process
        ldr     r3, =KData                      ; (r3) = ptr to KDataStruct
        ldr     r1, [r3, #hCurProc]             ; (r1) = hCurProc
        cmp     r1, r2                          ; if new process == old process
        beq     %F31                            ; then skip CeLog call

        stmfd   sp!, {r0-r3, r12, lr}           ; save regs
        mov     r0, r2                          ; (r0) = handle of new process

        CALL    CELOG_ThreadMigrateARM

        ldmfd   sp!, {r0-r3, r12, lr}           ; restore regs
31
  ENDIF

        ldr     r1, [r0, #ThProc]               ; (r1) = ptr to thread's current process
        ldr     r12, =KData                     ; (r12) = ptr to KDataStruct
        ldr     r2, [r1, #PrcHandle]            ; (r2) = current proc's handle
        ldr     r3, [r1, #PrcVMBase]            ; (r3) = current proc's slot base address
        str     r1, [r12, #pCurPrc]             ; set pCurProc
        str     r2, [r12, #hCurProc]            ; set hCurProc
        add     r12, r12, #aSections            ; (r12) = ptr to SectionTable
        ldr     r2, [r12, r3, LSR #VA_SECTION-2]  ; (r2) = process's memory section
        mtc15   r3, c13                         ; set process base address register
        str     r2, [r12]                       ; copy to slot 0 for VirtualAlloc & friends
        ; Set DOMAIN Access.

        RETURN                                  ; return to caller

  IF CELOG
        LTORG
  ENDIF




;-------------------------------------------------------------------------------
;-------------------------------------------------------------------------------
        NESTED_ENTRY KCall
        PROLOG_END

; KCall - call kernel function
;
;       KCall invokes a kernel function in a non-preemtable state by switching
; to SVC mode.  This switches to the kernel stack and inhibits rescheduling.
;
;       Entry   (r0) = ptr to function to call
;               (r1) = first function arg
;               (r2) = second fucntion arg
;               (r3) = third function arg
;       Exit    (r0) = function return value
;       Uses    r0-r3, r12

        mrs     r12, cpsr                       ; (r12) = current status
        and     r12, r12, #0x1F                 ; (r12) = current mode
        cmp     r12, #SYSTEM_MODE
        mov     r12, r0                         ; (r12) = address of function to call
        mov     r0, r1                          ; \  ;
        mov     r1, r2                          ;  | ripple args down
        mov     r2, r3                          ; /

  IF Thumbing
        bxne    r12                             ; already non-preemptible, invoke routine directly
  ELSE
        movne   pc, r12                         ; already non-preemptible, invoke routine directly
  ENDIF

        msr     cpsr_c, #SVC_MODE
        mov     lr, pc                          ; (lr_svc) = PC+8

  IF Thumbing
        bx      r12                             ; invoke routine in non-preemptible state
  ELSE
        mov     pc, r12                         ; invoke routine in non-preemptible state
  ENDIF

        msr     cpsr_c, #SYSTEM_MODE            ; back to preemtible state
        ldr     r3, =KData                      ; (r3) = ptr to KDataStruct
        ldrb    r12, [r3, #bResched]            ; (r12) = reschedule flag
        cmp     r12, #1
        beq     %F1
        ldrb    r12, [r3, #dwKCRes]
        cmp     r12, #1

        RETURN_NE                               ; no reschedule needed so return

1       mov     r12, lr
        mrs     r2, cpsr
 IF Thumbing
        tst     r12, #0x01                      ; return to thumb mode ?
        orrne   r2, r2, #THUMB_STATE            ; then update the Psr
 ENDIF
        msr     cpsr_c, #SVC_MODE:OR:0xC0
        stmdb   r3, {r0-r3, r12}
        mov     r1, #ID_RESCHEDULE
        b       SaveAndReschedule
        ENTRY_END KCall



;-------------------------------------------------------------------------------
; InSysCall - check if in non-preemptible state
;
;       InSysCall is called to test if code is being called in a preemptible state
; or not.  Only user mode or system mode code is preemptible.  Since this code
; cannot be called in user mode, we just check for system mode.
;
;       Entry   none
;       Return  (r0) = 0 if preemtible, !=0 not preemptible
;-------------------------------------------------------------------------------
        LEAF_ENTRY InSysCall

        mrs     r0, cpsr                        ; (r0) = current status
        and     r0, r0, #0x1F                   ; (r0) = current mode
        eor     r0, r0, #SYSTEM_MODE            ; (r0) = 0 iff in system mode

        RETURN                                  ; return to caller



;-------------------------------------------------------------------------------
; CaptureContext is invoked in kernel context on the user thread's stack to
; build a context structure to be used for exception unwinding.
;
; Note: The Psr & Pc values will be updated by ExceptionDispatch from information
;       in the excinfo struct pointed at by the THREAD.
;
;       (sp) = aligned stack pointer
;       (r0-r14), etc. - CPU state at the time of exception
;-------------------------------------------------------------------------------
        ; Fake prolog for unwinding.
        NESTED_ENTRY xxCaptureContext

        mrs     r0, cpsr
        stmdb   sp!, {r0}                       ; Unwind the Psr register
        ; The following instruction is a special hint to the unwinder, giving
        ; the location of the exception PC.  See unwind.c:ArmVirtualUnwind().
        stmdb   sp!, {pc}                       ; PC of exception for unwinding
        stmdb   sp!, {lr}                       ; Restore lr
        sub     sp, sp, #(4 * 2)                ; Unlink past r12, sp
        stmdb   sp!, {r4-r11}                   ; Restore non-volatiles r4-r11
        sub     sp, sp, #(4 * 5)                ; Unlink past flags, r0-r3

    PROLOG_END

        ALTERNATE_ENTRY CaptureContext
        sub     sp, sp, #CtxSizeof              ; (sp) = CONTEXT_RECORD
        stmib   sp, {r0-r15}                    ; store all registers
        mrs     r1, cpsr                        ; (r1) = current status
        mov     r0, #CONTEXT_FULL
        str     r1, [sp,#CtxPsr]
        str     r0, [sp,#CtxFlags]              ; set ContextFlags
        mov     r0, sp                          ; (r0) = Arg0 = ptr to CONTEXT_RECORD
        CALL    ExceptionDispatch
        mov     r1, sp                          ; (r1) = ptr to CONTEXT_RECORD
        ldr     r2, [r1,#CtxPsr]                ; (r0) = new Psr
        and     r0, r2, #0x1f                   ; (r2) = destination mode
        cmp     r0, #USER_MODE
        bne     %F9                             ; not returning to user mode
; Returning to user mode.
        msr     cpsr_c, #SVC_MODE:OR:0xC0       ; switch to Supervisor Mode w/IRQs disabled
        msr     spsr, r2
        ldr     lr, [r1,#CtxPc]                 ; (lr) = address to continue at
        ldmib   r1, {r0-r14}^                   ; restore user mode registers
        nop                                     ; hazard from user mode bank switch
        movs    pc, lr                          ; return to user mode.

; Returning to a privileged mode.
;
;       (r1) = ptr to CONTEXT_RECORD
;       (r2) = new PSR

9
  IF Interworking :LOR: Thumbing

        tst     r2, #THUMB_STATE                ; returning to Thumb code ?
        beq     ARMDispatch                     ; if not then dispatch to ARM code

;
;   Switch to Thumb mode before continuing.
;   "push" r0 and PC onto the continuation stack, update the context, use
;   r0 to switch to Thumb, and then pop r0 & the continuation PC
;

ThumbDispatch

        bic     r2, r2, #THUMB_STATE            ; else update mode
        msr     cpsr, r2

        ldr     r2, [r1, #CtxR0]                ; load saved R0 value
        ldr     r3, [r1, #CtxPc]                ; and continuation PC
        ldr     r0, [r1, #CtxSp]                ; get the continuation SP
        stmdb   r0!, {r2, r3}                   ; "push" new R0 & PC values
        str     r0, [r1, #CtxSp]                ; and update new stack pointer

        add     r1, r1, #CtxR1                  ; get pointer to R1 context
        ldmia   r1, {r1-r14}                    ; load context except for R0 & PC

        orr     r0, pc, #0x01
        bx      r0                              ; and switch to Thumb code

  IF {FALSE}                                    ; CODE16 causes THUMB object type

        CODE16
        pop     {r0, pc}                        ; pop R0 and continue
        nop
        CODE32

  ENDIF

ThumbSwitch

        DCW     0xBD01                          ; pop {r0, pc}
        DCW     0x46C0                          ; nop

ARMDispatch

  ENDIF

        msr     cpsr, r2                        ; restore Psr & switch modes
        ldmib   r1, {r0-r15}                    ; restore all registers & return

        ENTRY_END xxCaptureContext



;-------------------------------------------------------------------------------
;++
; EXCEPTION_DISPOSITION
; RtlpExecuteHandlerForException (
;    IN PEXCEPTION_RECORD ExceptionRecord,
;    IN ULONG EstablisherFrame,
;    IN OUT PCONTEXT ContextRecord,
;    IN OUT PDISPATCHER_CONTEXT DispatcherContext,
;    IN PEXCEPTION_ROUTINE ExceptionRoutine,
;    IN ULONG ExceptionMode
;    )
;
; Routine Description:
;    This function allocates a call frame, stores the establisher frame
;    pointer in the frame, establishes an exception handler, and then calls
;    the specified exception handler as an exception handler. If a nested
;    exception occurs, then the exception handler of this function is called
;    and the establisher frame pointer is returned to the exception dispatcher
;    via the dispatcher context parameter. If control is returned to this
;    routine, then the frame is deallocated and the disposition status is
;    returned to the exception dispatcher.
;
; Arguments:
;    ExceptionRecord (r0) - Supplies a pointer to an exception record.
;
;    EstablisherFrame (r1) - Supplies the frame pointer of the establisher
;       of the exception handler that is to be called.
;
;    ContextRecord (r2) - Supplies a pointer to a context record.
;
;    DispatcherContext (r3) - Supplies a pointer to the dispatcher context
;       record.
;
;    ExceptionRoutine ((sp)) - supplies a pointer to the exception handler
;       that is to be called.
;
;    ExceptionMode (4(sp)) - PSR value for running ExceptionRoutine
;
; Return Value:
;    The disposition value returned by the specified exception handler is
;    returned as the function value.
;--
;-------------------------------------------------------------------------------
        EXCEPTION_HANDLER RtlpExceptionHandler

        NESTED_ENTRY RtlpExecuteHandlerForException

        mov     r12, sp
        stmfd   sp!, {r3}                       ; save ptr to DispatcherContext
                                                ;       for RtlpExceptionHandler
        stmfd   sp!, {r11,r12,lr}
        sub     sp, sp, #CstkSizeof             ; space for a CALLSTACK struct
        sub     r11, r12, #4                    ; (r11) = frame pointer

        PROLOG_END

        ldr     r12, [r11, #8]                  ; (r12) = handler's mode
        add     lr, pc, #reheRet-(.+8)          ; (lr) = return address
        tst     r12, #0xF                       ; user mode??

  IF Interworking :LOR: Thumbing
        ldrne   r12, [r11, #4]                  ; invoke handler in system mode
        bxne    r12
  ELSE
        ldrne   pc, [r11, #4]                   ; invoke handler in system mode
  ENDIF

        ldr     r12, =KData
        ldr     r12, [r12, #pCurThd]            ; (r12) = ptr to current thread
        ldr     r12, [r12, #ThPcstkTop]         ; (r12) = ptr to CALLSTACK

        ; Link CallStackReserve to thread's callstack list so the unwinder can
        ; unwind past SYSCALL_RETURN
        mov     r3, sp                          ; (r3) = ptr to CallStackReserve
        str     r12, [r3, #CstkNext]            ; CallStackReserve->pcstkNext = pth->pcstkTop
        ldr     r12, =KData
        ldr     r12, [r12, #pCurThd]            ; (r12) = pCurThread
        str     r3, [r12, #ThPcstkTop]          ; pCurThread->pcstkTop = CallStackReserve
        mov     r12, r3                         ; r12 = CallStackReserve
        ldr     r3, [r11, #0]                   ; restore r3

        str     lr, [r12, #CstkRa]              ; set return address
        mov     lr, #0
        str     lr, [r12, #CstkAkyLast]
        mov     lr, #SYSTEM_MODE
        str     lr, [r12, #CstkPrcLast]
        ldr     lr, =SYSCALL_RETURN
        ldr     r12, [r11,#4]                   ; (r12) = address to continue at

        msr     cpsr_c, #SVC_MODE:OR:0xC0       ; switch to SVC Mode w/IRQs disabled

        tst     r12, #0x01                      ; set continuation Psr based on
        msreq   spsr_c, #USER_MODE              ; Thumb or ARM continuation
        msrne   spsr_c, #USER_MODE | THUMB_STATE

        movs    pc, r12                         ; switch to user mode & jump to handler
        nop                                     ; NOP required to terminate unwinding

reheRet

  IF Thumbing
        ldmdb   r11, {r11, sp, lr}
        bx      lr
  ELSE
        ldmdb   r11, {r11, sp, pc}
  ENDIF

        ENTRY_END RtlpExecuteHandlerForException



;-------------------------------------------------------------------------------
;++
; EXCEPTION_DISPOSITION
; RtlpExceptionHandler (
;    IN PEXCEPTION_RECORD ExceptionRecord,
;    IN ULONG EstablisherFrame,
;    IN OUT PCONTEXT ContextRecord,
;    IN OUT PDISPATCHER_CONTEXT DispatcherContext
;    )
;
; Routine Description:
;    This function is called when a nested exception occurs. Its function
;    is to retrieve the establisher frame pointer from its establisher's
;    call frame, store this information in the dispatcher context record,
;    and return a disposition value of nested exception.
;
; Arguments:
;    ExceptionRecord (r0) - Supplies a pointer to an exception record.
;
;    EstablisherFrame (r1) - Supplies the frame pointer of the establisher
;       of this exception handler.
;
;    ContextRecord (r2) - Supplies a pointer to a context record.
;
;    DispatcherContext (r3) - Supplies a pointer to the dispatcher context
;       record.
;
; Return Value:
;    A disposition value ExceptionNestedException is returned if an unwind
;    is not in progress. Otherwise a value of ExceptionContinueSearch is
;    returned.
;--
;-------------------------------------------------------------------------------
        LEAF_ENTRY RtlpExceptionHandler

        ldr     r12, [r0, #ErExceptionFlags]    ; (r12) = exception flags
        tst     r12, #EXCEPTION_UNWIND          ; unwind in progress?
        movne   r0, #ExceptionContinueSearch    ;  Y: set dispostion value

        RETURN_NE                               ;  Y: & return

        ldr     r12, [r1, #-4]                  ; (r12) = ptr to DispatcherContext
        ldr     r12, [r12, #DcEstablisherFrame] ; (r12) = establisher frame pointer
        str     r12, [r3, #DcEstablisherFrame]  ; copy to current dispatcher ctx
        mov     r0, #ExceptionNestedException   ; set dispostion value

        RETURN                                  ; return to caller

        ENTRY_END RtlpExeptionHandler



;-------------------------------------------------------------------------------
;++
; EXCEPTION_DISPOSITION
; RtlpExecuteHandlerForUnwind (
;    IN PEXCEPTION_RECORD ExceptionRecord,
;    IN PVOID EstablisherFrame,
;    IN OUT PCONTEXT ContextRecord,
;    IN OUT PVOID DispatcherContext,
;    IN PEXCEPTION_ROUTINE ExceptionRoutine
;    )
;
; Routine Description:
;    This function allocates a call frame, stores the establisher frame
;    pointer and the context record address in the frame, establishes an
;    exception handler, and then calls the specified exception handler as
;    an unwind handler. If a collided unwind occurs, then the exception
;    handler of of this function is called and the establisher frame pointer
;    and context record address are returned to the unwind dispatcher via
;    the dispatcher context parameter. If control is returned to this routine,
;    then the frame is deallocated and the disposition status is returned to
;    the unwind dispatcher.
;
; Arguments:
;    ExceptionRecord (r0) - Supplies a pointer to an exception record.
;
;    EstablisherFrame (r1) - Supplies the frame pointer of the establisher
;       of the exception handler that is to be called.
;
;    ContextRecord (r2) - Supplies a pointer to a context record.
;
;    DispatcherContext (r3) - Supplies a pointer to the dispatcher context
;       record.
;
;    ExceptionRoutine ((sp)) - supplies a pointer to the exception handler
;       that is to be called.
;
; Return Value:
;    The disposition value returned by the specified exception handler is
;    returned as the function value.
;--
;-------------------------------------------------------------------------------
        EXCEPTION_HANDLER RtlpUnwindHandler

        NESTED_ENTRY RtlpExecuteHandlerForUnwind

        mov     r12, sp
        stmfd   sp!, {r3}                       ; save ptr to DispatcherContext
        stmfd   sp!, {r11,r12,lr}               ;       for RtlpUnwindHandler
        sub     sp, sp, #CstkSizeof             ; space for a CALLSTACK struct
        sub     r11, r12, #4                    ; (r11) = frame pointer

        PROLOG_END

        ldr     r12, [r2, #CtxPsr]              ; (r12) = handler's mode
        add     lr, pc, #rehuRet-(.+8)          ; (lr) = return address
        tst     r12, #0xF                       ; user mode??
  IF Thumbing
        ldrne   r12, [r11, #4]
        bxne    r12                             ; invoke Thumb mode handler
  ELSE
        ldrne   pc, [r11, #4]                   ; invoke handler in system mode
  ENDIF
        ldr     r12, =KData
        ldr     r12, [r12, #pCurThd]            ; (r12) = ptr to current thread
        ldr     r12, [r12, #ThPcstkTop]         ; (r12) = ptr to CALLSTACK

        ; Link CallStackReserve to thread's callstack list so the unwinder can
        ; unwind past SYSCALL_RETURN
        mov     r3, sp                          ; (r3) = ptr to CallStackReserve
        str     r12, [r3, #CstkNext]            ; CallStackReserve->pcstkNext = pth->pcstkTop
        ldr     r12, =KData
        ldr     r12, [r12, #pCurThd]            ; (r12) = pCurThread
        str     r3, [r12, #ThPcstkTop]          ; pCurThread->pcstkTop = CallStackReserve
        mov     r12, r3                         ; r12 = CallStackReserve
        ldr     r3, [r11, #0]                   ; restore r3

        str     lr, [r12, #CstkRa]              ; set return address
        mov     lr, #0
        str     lr, [r12, #CstkAkyLast]
        mov     lr, #SYSTEM_MODE
        str     lr, [r12, #CstkPrcLast]
        ldr     lr, =SYSCALL_RETURN
        ldr     r12, [r11,#4]                   ; (r12) = address to continue at

        msr     cpsr_c, #SVC_MODE:OR:0xC0       ; switch to SVC Mode w/IRQs disabled

        tst     r12, #0x01                      ; set continuation Psr based on
        msreq   spsr_c, #USER_MODE              ; Thumb or ARM continuation
        msrne   spsr_c, #USER_MODE | THUMB_STATE

        movs    pc, r12                         ; switch to user mode & jump to handler

rehuRet

  IF Thumbing
        ldmdb   r11, {r11, sp, lr}
        bx      lr
  ELSE
        ldmdb   r11, {r11, sp, pc}
  ENDIF

        ENTRY_END RtlpExecuteHandlerForUnwind



;-------------------------------------------------------------------------------
;++
; EXCEPTION_DISPOSITION
; RtlpUnwindHandler (
;    IN PEXCEPTION_RECORD ExceptionRecord,
;    IN PVOID EstablisherFrame,
;    IN OUT PCONTEXT ContextRecord,
;    IN OUT PVOID DispatcherContext
;    )
;
; Routine Description:
;    This function is called when a collided unwind occurs. Its function
;    is to retrieve the establisher dispatcher context, copy it to the
;    current dispatcher context, and return a disposition value of nested
;    unwind.
;
; Arguments:
;    ExceptionRecord (r0) - Supplies a pointer to an exception record.
;
;    EstablisherFrame (r1) - Supplies the frame pointer of the establisher
;       of this exception handler.
;
;    ContextRecord (r2) - Supplies a pointer to a context record.
;
;    DispatcherContext (r3) - Supplies a pointer to the dispatcher context
;       record.
;
; Return Value:
;    A disposition value ExceptionCollidedUnwind is returned if an unwind is
;    in progress. Otherwise, a value of ExceptionContinueSearch is returned.
;--
;-------------------------------------------------------------------------------
        LEAF_ENTRY RtlpUnwindHandler

        ldr     r12, [r0, #ErExceptionFlags]    ; (r12) = exception flags
        tst     r12, #EXCEPTION_UNWIND          ; unwind in progress?
        moveq   r0, #ExceptionContinueSearch    ;  N: set dispostion value
        RETURN_EQ                               ;  N: & return

        ldr     r12, [r1, #-4]                  ; (r12) = ptr to DispatcherContext
        ldmia   r12, {r0-r2,r12}                ; copy establisher frames disp. ctx
        stmia   r3, {r0-r2,r12}                 ; to the current dispatcher context
        mov     r0, #ExceptionCollidedUnwind    ; set dispostion value

        RETURN                                  ; return to caller

        ENTRY_END RtlpUnwindHandler




;-------------------------------------------------------------------------------
; This code is copied to the kernel's data page so that it is accessible from
; the kernel & user code. The kernel checks if it is interrupting an interlocked
; api by range checking the PC to be between UserKPage+0x380 & UserKPage+0x400.
;-------------------------------------------------------------------------------

        LEAF_ENTRY InterlockedAPIs
ILPopList
        ldr     r12, [r0]                       ; (r12) = ptr to first item in list
        cmp     r12, #0
        ldrne   r1, [r12]                       ; (r1) = new head of list
        strne   r1, [r0]                        ; update list head
        mov     r0, r12                         ; return ptr to first item
  IF Interworking :LOR: Thumbing
        bx      lr
  ELSE
        mov     pc, lr
  ENDIF

ILPushList
        ldr     r12, [r0]                       ; (r12) = old list head
        str     r12, [r1]                       ; store linkage
        str     r1, [r0]                        ; store new list head
        mov     r0, r12                         ; return old list head
  IF Interworking :LOR: Thumbing
        bx      lr
  ELSE
        mov     pc, lr
  ENDIF

ILCmpEx
        ldr     r12, [r0]
        cmp     r12, r2
        streq   r1, [r0]
        mov     r0, r12                         ; (r0) = return original value
  IF Interworking :LOR: Thumbing
        bx      lr
  ELSE
        mov     pc, lr
  ENDIF

ILExAdd
        ldr     r12, [r0]
        add     r2, r12, r1
        str     r2, [r0]
        mov     r0, r12                         ; (r0) = return original value
  IF Interworking :LOR: Thumbing
        bx      lr
  ELSE
        mov     pc, lr
  ENDIF

ILEx
        ldr     r12, [r0]
        str     r1, [r0]
        mov     r0, r12                         ; (r0) = return original value
  IF Interworking :LOR: Thumbing
        bx      lr
  ELSE
        mov     pc, lr
  ENDIF
        END_REGION InterlockedEnd

; NullSection - empty memory section for unallocated 32mb regions
        EXPORT  NullSection[DATA]
NullSection
        OPT     2                               ; disable listing
        %       512*4                           ; 512 Words of zeros
        OPT     1                               ; reenable listing

        END

