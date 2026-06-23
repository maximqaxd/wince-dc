; ----------------------------------------------------------------
;
; Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.
;
; ----------------------------------------------------------------

.486p


PAGE_SIZE EQU 1000h
PAGE_ATTRIB_RW		EQU	000001100011b
PAGE_ATTRIB_RW_NOCACHE	EQU	000001111011b
PAGE_ATTRIB_RO_USER	EQU	000000100101b
PAGE_ATTRIB_RW_USER	EQU	000001100111b

KGDT_R0_CODE	EQU 8h
KGDT_R0_DATA	EQU 10h

PG_MASK	    EQU	    80000000h
WP_MASK	    EQU	    00010000h

PageRange MACRO Start, NumPages, Attrib
        LOCAL CurPhys
        .ERRNZ Start AND (PAGE_SIZE-1)
        CurPhys = Start
        REPEAT NumPages
            dd  CurPhys OR Attrib
            CurPhys = CurPhys + PAGE_SIZE
        ENDM
        ENDM


LIN_TO_PHYS_OFFSET EQU 80000000h

OFFSET32 EQU <OFFSET FLAT:>

.KDATA segment para public 'STACK'
	public _KDBase
	public _KStack
	db	1000h dup (?)	; reserve an extra page for the debugger
_KDBase	db	7fch dup (?)
_KStack dd	?
	public	_KData
_KData	db	088h dup (?)
	public _CurMSec, _DiffMSec
_CurMSec dd	?
_DiffMSec dd	?
 	db (800h - ($ - _KData)) dup (?)

	public	_g_PageDir
	public	_g_PageTablePool
	public	_g_ShadowPageDir
_g_PageDir dd	1024 dup (?)
;
; DANGER -- Don't change this number (16) without also changing PTE_POOL_SIZE in NKI386.H!!!
;
_g_PageTablePool   dd	16 * 1024 dup (?)	
;
;   "Shadow" page directory.  NOTE:  This MUST be aligned on a page boundary (same alignment as the page directory)
;
_g_ShadowPageDir    dd	512 dup (?)
		    dd	512 dup (?)	; Align this to a full page
ZERO_BLOCK_SIZE EQU $-_g_PageDir
.KDATA ends


PageTbl SEGMENT PAGE READONLY USE32 'PageTbl'
SyscallPage label word
	db	2048 dup (0CDh, 020h)	; page filled with "int 20" instructions
SyscallDir label dword
	dd	1024 dup (SyscallPage + LIN_TO_PHYS_OFFSET + PAGE_ATTRIB_RO_USER)
PageTbl ENDS

_TEXT segment para public 'TEXT'
EXTRN _SystemInitialization:NEAR
extrn _KernelInit:near
extrn _Reschedule:near
extrn _OEMInitPageTable:near
extrn _IdentifyCpu:near

	public	_NullSection
	align	4
_NullSection dd	512 dup (0)


;-------------------------------------------------------------------------------
;-------------------------------------------------------------------------------
_KernelInitialize PROC NEAR PUBLIC

	cld
	;;;mov	dword ptr ds:[0a001010H], 0aaH

	mov     ebx, OFFSET32 _g_PageDir + LIN_TO_PHYS_OFFSET
        mov	ecx, ZERO_BLOCK_SIZE / 4
        mov	edi, ebx
        xor	eax, eax
        rep     stosd
        
        call    _OEMInitPageTable
        ;
        ; The highest page directory entry is used by the kernel
        ;
        mov	DWORD PTR [ebx][0FFCh], OFFSET32 SyscallDir + LIN_TO_PHYS_OFFSET + PAGE_ATTRIB_RO_USER

        ;
        ; Create the identity-mapped addresses (physical = linear), because
        ; Pentium-II requires that once paging is enabled, the next instruction
        ; must come from the same address as if paging wasn't enabled.
        ;       
        ; First, the mapping for the eip register.
        mov     ecx, OFFSET32 PagingEnabled
        mov     edx, ecx
        add     edx, LIN_TO_PHYS_OFFSET
        ;
        ; ECX = current linear addr, EDX = current physical addr.
        ;
        shr     ecx, 22                 ; Which 4MB?
        shl     ecx, 2                  ; DWORD index
        shr     edx, 22                 ; Which 4MB?
        shl     edx, 2                  ; DWORD index
        ; Next, the mapping for the gdt.
        sub     esp, 8
        sgdt    [esp]
        mov     esi, dword ptr 2[esp]
        add     esp, 8
        mov     edi, esi
        sub     esi, LIN_TO_PHYS_OFFSET
        ;
        ; ESI = linear gdtbase, EDX = physical gdtbase.
        ;
        shr     esi, 22                 ; Which 4MB?
        shl     esi, 2                  ; DWORD index
        shr     edi, 22                 ; Which 4MB?
        shl     edi, 2                  ; DWORD index

        ;
        ; Save off the original page directory entry for the physical address.
        ;
        push    DWORD PTR [ebx][edx]
        push    DWORD PTR [ebx][edi]
        ;
        ; Copy the page directory entry for the current linear 
        ; address to the physical address.
        ;
        mov     eax, DWORD PTR [ebx][ecx]
PDE_G   EQU     100h
        and     eax, not PDE_G                  ; only works if PDE_G implies PDE_PS
        mov     DWORD PTR [ebx][edx], eax
        mov     eax, DWORD PTR [ebx][esi]
        and     eax, not PDE_G                  ; only works if PDE_G implies PDE_PS
        mov     DWORD PTR [ebx][edi], eax
        ;
        ; Get the saved values off the stack.
        ;
        mov     cr3, ebx
        pop     ebx     ; Original pde value for gdtbase
        pop     ebp     ; Original pde value for PagingEnabled
        ;
        ; We are now identity-mapped, okay to enable paging.
        ;
	mov	esi, OFFSET32 PagingEnabled
        mov     eax, cr0
        or      eax, PG_MASK OR WP_MASK	; enable paging & write-protection for Ring0
        and	eax, not 060000000h 	; clear cache disable & cache write-thru bits
	align   16
        mov     cr0, eax
        jmp	esi
        align	4
PagingEnabled:
	wbinvd	; clear out the cache
	;;;mov	dword ptr ds:[0aa001010H], 0F0H
        ;
        ; Switch to virtual addressing for the stack and save of all these registers.
        mov     esp, OFFSET32 _KStack
        push    edx     ; eip pde index
        push    ebp     ; eip pde value
        push    edi     ; gdt pde index
        push    ebx     ; gdt pde value

        push	0
        popfd
	
	call	_SystemInitialization           ; Load new gdtr, among other things.
        ;
        ; Restore the original page directory entry for the physical addresses.
        ; We cannot restore these PDEs until _SystemInitialization has reloaded
        ; gdtr.
        ;
        mov     ebx, OFFSET32 _g_PageDir
        pop     eax                             ; gdt pde value
        pop     edx                             ; gdt pde index
        mov     DWORD PTR [ebx][edx], eax       ; Restore GDT phys addr mapping
        pop     eax                             ; PagingEnabled pde value
        pop     edx                             ; PagingEnalbled pde index
        mov     DWORD PTR [ebx][edx], eax       ; Restore PaginEnabled phys addr mapping
        ;
        ; Flush the tlb, since we are unmapping.
        mov     eax, cr3                        ; Flush the TLB
        mov     cr3, eax

	call	_IdentifyCpu
	call	_KernelInit
	xor	edi, edi
	jmp	_Reschedule

_KernelInitialize ENDP


_INTERRUPTS_ON proc NEAR PUBLIC
	sti
	ret
_INTERRUPTS_ON endp

_INTERRUPTS_OFF proc NEAR PUBLIC
	cli
	ret
_INTERRUPTS_OFF endp

_TEXT ENDS 

        END
