// Copyright (c) 1999-2000 Microsoft Corporation.  All rights reserved.
#ifndef __EXSUP_H__
#define __EXSUP_H__

/* The following data structures are understood by the
 * compiler and the kernel. Normal user code (either C
 * or some other language) should NOT modify
 * them. Different (versions of the) compiler(s) should
 * continue to support minimal definitions the kernel
 * understands.
 */
#include <windef.h>
typedef unsigned long ADDRESS;
typedef void *PTR;
#define UnusedParameter(x) x = x
#define CRTAPI

/* Exception Codes
 */

#define EXC_ACCESS_VIOLATION         0xC0000005
#define EXC_DATATYPE_MISALIGNMENT    0x80000002
#define EXC_BREAKPOINT               0x80000003
#define EXC_SINGLE_STEP              0x80000004
#define EXC_ARRAY_BOUNDS_EXCEEDED    0xC000008C
#define EXC_FLT_DENORMAL_OPERAND     0xC000008D
#define EXC_FLT_DIVIDE_BY_ZERO       0xC000008E
#define EXC_FLT_INEXACT_RESULT       0xC000008F
#define EXC_FLT_INVALID_OPERATION    0xC0000090
#define EXC_FLT_OVERFLOW             0xC0000091
#define EXC_FLT_STACK_CHECK          0xC0000092
#define EXC_FLT_UNDERFLOW            0xC0000093
#define EXC_INT_DIVIDE_BY_ZERO       0xC0000094
#define EXC_INT_OVERFLOW             0xC0000095
#define EXC_PRIV_INSTRUCTION         0xC0000096
#define EXC_IN_PAGE_ERROR            0xC0000006
#define EXC_ILLEGAL_INSTRUCTION      0xC000001D
#define EXC_NONCONTINUABLE_EXCEPTION 0xC0000025
#define EXC_STACK_OVERFLOW           0xC00000FD
#define EXC_INVALID_DISPOSITION      0xC0000026
#define EXC_GUARD_PAGE               0x80000001

/*filter return codes */
#define FILTER_ACCEPT              1
#define FILTER_DISMISS            -1
#define FILTER_CONTINUE_SEARCH     0

/*handler flags settings.. */
#define EXCEPTION_EXECUTE_HANDLER	 1
#define EXCEPTION_CONTINUE_SEARCH	 0
#define EXCEPTION_CONTINUE_EXECUTION	-1

#define TRYLEVEL_NONE        -1
#define TRYLEVEL_INVALID     -2

#define EXCEPTION_UNWIND_CONTEXT (EXCEPTION_UNWINDING|EXCEPTION_EXIT_UNWIND)

#define EXCEPTION_MAXIMUM_PARAMETERS 15

typedef struct _EXCEPTION_REGISTRATION EXCEPTION_REGISTRATION;
typedef EXCEPTION_REGISTRATION *PEXCEPTION_REGISTRATION;

typedef struct _C9_EXCEPTION_REGISTRATION C9_EXCEPTION_REGISTRATION;
typedef C9_EXCEPTION_REGISTRATION *PC9_EXCEPTION_REGISTRATION;

typedef struct _SCOPETABLE_ENTRY SCOPETABLE_ENTRY;
typedef SCOPETABLE_ENTRY *PSCOPETABLE_ENTRY;


#if 0
typedef EXCEPTION_DISPOSITION __cdecl EXCEPTION_ROUTINE(
                                PEXCEPTION_RECORD,
                                PEXCEPTION_REGISTRATION,
                                PCONTEXT,
                                PEXCEPTION_REGISTRATION);
typedef EXCEPTION_ROUTINE *PEXCEPTION_ROUTINE;
#endif







struct _EXCEPTION_REGISTRATION {
    PEXCEPTION_REGISTRATION PreviousRegistration;
    PEXCEPTION_ROUTINE ExHandler; /* normally _except_handler3 */
    PSCOPETABLE_ENTRY ScopeTable;
    int TryLevel;
};

struct _C9_EXCEPTION_REGISTRATION {
    ADDRESS SavedEsp;
    PEXCEPTION_POINTERS XPointers;
    EXCEPTION_REGISTRATION XRegistration;
};

#define EXCEPTION_CHAIN_END ((PEXCEPTION_REGISTRATION)-1)

/* The filter and the handler are called by _except_handler3
 * with ebp pointing just after the exception registration record.
 */
struct _SCOPETABLE_ENTRY {
    int EnclosingLevel;                 /* lexical level of enclosing scope */
    int (__cdecl *Filter)(void);        /* NULL for a termination handler */
    void (__cdecl *SpecificHandler)(void); /* xcpt or termination handler */
};
/*struct _SCOPETABLE_ENTRY Scopetable[NUMTRYS]; */


/* Even if the linker over-optimizes ABS symbols away..
 */
extern PTR _except_list;

/* Prototypes */
extern void __cdecl NK_global_unwind2(PEXCEPTION_REGISTRATION stop);
extern void __cdecl NK_local_unwind2(PEXCEPTION_REGISTRATION xr, int stop);
EXCEPTION_DISPOSITION __cdecl  NK_except_handler3(
       PEXCEPTION_RECORD XRecord,
       PEXCEPTION_REGISTRATION Registration,
       PCONTEXT Context,
       PEXCEPTION_REGISTRATION Dispatcher);
EXCEPTION_DISPOSITION __cdecl  NKCallExceptionFilter(
       PEXCEPTION_RECORD XRecord,
       PEXCEPTION_REGISTRATION Registration,
       PCONTEXT Context);

#endif /* __EXSUP_H__ */
