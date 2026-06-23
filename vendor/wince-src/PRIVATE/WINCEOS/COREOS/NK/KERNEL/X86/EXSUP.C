/*
 *	Copyright (c) 1994-2000 Microsoft Corporation.  All rights reserved.
 *
 * Structure Exception Handling support.
 * Taken from NT sources.
 */

#include <kernel.h>
#include "exsup.h"

#define SavedEsp  -8
#define XPointers -4

#define FRAME_EBP_OFFSET 16

struct contextinfo {
    CONTEXT CRec;
};

extern void _unwind_handler(void);
extern void _local_unwind2(void);
extern void _except_finish(void);

/* __ABNORMAL_TERMINATION - return TRUE if __finally clause entered via
 * _local_unwind2.
 */
#pragma warning(disable:4035)
__declspec(naked) BOOL __abnormal_termination(void)
{
    __asm {
        xor     eax, eax;              /* assume FALSE */

        mov     ecx, fs:[0];
        cmp     [ecx.ExHandler], offset _unwind_handler;
        jne     short at_done;         /* UnwindHandler first? */

        mov     edx, [ecx+12];         /* establisher of local_unwind2 */
        mov     edx, [edx.TryLevel];   /* is TryLevel the same as the */
        cmp     [ecx+8], edx;          /* local_unwind level? */
        jne     short at_done;         /* no - then FALSE */

        inc     eax                    /* currently in _abnormal_termination */
at_done:
        ret
    }
}
#pragma warning(default:4035)

/*
 * The structure struct _EXCEPTION_REGISTRATION was changed in C9,
 * and a few fields were added. To keep the existing code happy,
 * the link field was maintained and the new fields are used here
 * as negative offsets from a pointer to such structure.
 */
#define SavedEsp  -8
#define XPointers -4

#define FRAME_EBP_OFFSET 16

/* _EXCEPT_HANDLER3 - Try to find an exception handler listed in the scope
 * table associated with the given registration record, that wants to accept
 * the current exception. If we find one, run it (and never return).
 * RETURNS: (*if* it returns)
 *  ExceptionContinueExecution - dismiss the exception.
 *  ExceptionContinueSearch - pass the exception up to enclosing handlers
 */
#pragma warning(disable:4035)
EXCEPTION_DISPOSITION __cdecl  _except_handler3(
       PEXCEPTION_RECORD XRecord,
       void *Registration,
       PCONTEXT Context,
       PDISPATCHER_CONTEXT Dispatcher)
{
    EXCEPTION_POINTERS ExcPtrs;

    UnusedParameter(Dispatcher);

    __asm {
        /* DF in indeterminate state at time of exception, so clear it */
        cld

        mov     ebx, Registration;
        mov     eax, XRecord;

        test    [eax.ExceptionFlags], EXCEPTION_UNWIND_CONTEXT;
        jnz     short lh_unwinding;

        /* build the EXCEPTION_POINTERS locally, store its address in the
         * registration record. this is the pointer that is returned by
         * the _exception_info intrinsic.
         */
        mov     dword ptr ExcPtrs.ExceptionRecord, eax;
        mov     eax, Context;
        mov     dword ptr ExcPtrs.ContextRecord, eax;
        lea     eax, ExcPtrs;
        mov     [ebx.XPointers], eax;

        mov     esi, [ebx.TryLevel];
        mov     edi, [ebx.ScopeTable];
lh_top:
        cmp     esi, -1;
        je      short lh_bagit;
        lea     ecx, [esi+esi*2];            /*ecx= TryLevel*3*/
        cmp     dword ptr [(edi+ecx*4).Filter], 0;
        je      short lh_continue;           /*term handler, so keep looking*/

        /*filter may trash *all* registers, so save ebp and ScopeTable offset*/
        push    esi;
        push    ebp;

        lea	ebp, FRAME_EBP_OFFSET[ebx];
        call    [(edi+ecx*4).Filter];        /*call the filter*/

        pop     ebp;
        pop     esi;
        /*ebx may have been trashed by the filter, so we must reload*/
        mov     ebx, Registration;

        /* Accept <0, 0, >0 instead of just -1, 0, +1 */
        or	eax, eax;
        jz	short lh_continue;
        js	short lh_dismiss;

        /*;assert(eax==FILTER_ACCEPT)*/
        mov     eax, Context
        mov     [eax].CRec.Esi, esi
        mov     [eax].CRec.Ebx, ebx
        mov     [eax].CRec.Eip, offset _except_finish
        mov     eax, ExceptionExecuteHandler
        jmp     short lh_return

lh_continue:
        /* reload the scope table base, possibly trashed by call to filter */
        mov     edi, [ebx.ScopeTable];
        lea     ecx, [esi+esi*2];
        mov     esi, [edi+ecx*4+0];         /* load the enclosing TryLevel */
        jmp     short lh_top;

lh_dismiss:
        mov     eax, ExceptionContinueExecution;   /* dismiss the exception */
        jmp     short lh_return;

lh_bagit:
        mov     eax, ExceptionContinueSearch;
        jmp     short lh_return;

lh_unwinding:
        test    [eax.ExceptionFlags], EXCEPTION_TARGET_UNWIND
        mov     eax, ExceptionContinueSearch;
        jnz     short lh_return;

        push    ebp;
        lea	ebp, FRAME_EBP_OFFSET[ebx];
        push    -1;
        push    ebx;
        call    _local_unwind2;
        add     esp, 8;
        pop     ebp;
	/* the return value is not really relevent in an unwind context */
        mov     eax, ExceptionContinueSearch;

lh_return:
    }
}
#pragma warning(default:4035)

void __declspec(naked) _except_finish(void)
{
    __asm {
        /*setup ebp for the local unwinder and the specific handler */
        lea     ebp, FRAME_EBP_OFFSET[ebx];
        /* reload the scope table base */
        mov     edi, [ebx.ScopeTable];

        /*the stop try level == accepting except level */
        push    esi;                        /* stop try level */
        push    ebx;                        /* Registration*  */
        call    _local_unwind2;
        add     esp, 8;

        lea     ecx, [esi+esi*2];           /* ecx=TryLevel*3 */

	/* set the current TryLevel to our enclosing level immediately
	 * before giving control to the handler. it is the enclosing
	 * level, if any, that guards the handler.
	 */
        mov     eax, [(edi+ecx*4).EnclosingLevel];
        mov     [ebx.TryLevel], eax;
        call    [(edi+ecx*4).SpecificHandler];
        /* assert(0); -- (NB! should not return) */
    }
}


/* _LOCAL_UNWIND2 - run all termination handlers listed in the scope table
 * associated with the given registration record, from the current lexical
 * level through enclosing levels up to, but not including the given 'stop'
 * level.
 */
__declspec(naked) void _local_unwind2(void)
{
    /* NOTE: frame (ebp) is setup by caller of __local_unwind2 */
    __asm {
        push    ebx;
        push    esi;
        push    edi;     /*call to the handler may trash, so we must save it */

        mov     eax, [esp+16]; /* xr */

        /* link in a handler to guard our unwind */
        push	eax;
        push    TRYLEVEL_INVALID;
        push    OFFSET _unwind_handler;
        push    dword ptr fs:[0];
        mov     fs:[0], esp;

lu_top:
        mov     eax, [esp+32];  /* xr */
        mov     ebx, [eax.ScopeTable];
        mov     esi, [eax.TryLevel];

        cmp     esi, -1;
        je      short lu_done;
        cmp     esi, [esp+36];  /* stop */
        je      short lu_done;

        lea     esi, [esi+esi*2];     /* esi*= 3 */

        mov     ecx, [(ebx+esi*4).EnclosingLevel];
        mov     [esp+8], ecx ;        /* save enclosing level */
        mov     [eax.TryLevel], ecx;

        cmp     dword ptr [(ebx+esi*4).Filter], 0;
        jnz     short lu_top;

        call    [(ebx+esi*4).SpecificHandler];

        jmp     short lu_top;

lu_done:
	/* cleanup stack
	 */
        pop     dword ptr fs:[0]
        add     esp, 4*3

        pop     edi;
        pop     esi;
        pop     ebx;
        ret
    }
}


// Exception handler function for _local_unwind2()
void __declspec(naked) _unwind_handler(void)
{
    /*
     * this is a special purpose handler used to guard our local unwinder.
     * its job is to catch collided unwinds.
     *
     */
    __asm {
        mov     ecx, dword ptr [esp+4];  /* xr */
        test    dword ptr [ecx.ExceptionFlags], EXCEPTION_UNWIND_CONTEXT;
        mov     eax, ExceptionContinueSearch;
        jz      short uh_return;

	/* We collided in a _local_unwind.  We set the dispatched to the
	 * establisher just before the local handler so we can unwind
	 * any future local handlers.
	 */

        mov     eax, [esp+8];	/* Our Establisher is the one */
                                /* in front of the local one  */
        mov     edx, [esp+16];  /* set Dispatcher to local_unwind2 */
        mov     [edx], eax;

	mov	eax, ExceptionCollidedUnwind;

uh_return:
	ret
    }
}

