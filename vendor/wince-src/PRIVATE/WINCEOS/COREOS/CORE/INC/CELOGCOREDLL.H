// Copyright (c) 1999-2000 Microsoft Corporation.  All rights reserved.
#ifdef CELOG

#include <celog.h>
#define MAX_STACK_FRAME 10

// Redefine CELOGDATA. We want core logging in COREDLL even in ship-build                    
#undef CELOGDATA
#undef CELOGDATAFLAGGED
#define CELOGDATA(T,I,D,L,Y,Z)            CeLogData(T,I,D,L,Y,Z,0,FALSE) 
#define CELOGDATAFLAGGED(T,I,D,L,Y,Z,F)   CeLogData(T,I,D,L,Y,Z,F,TRUE) 


//----------------------------------------------------------
_inline void CELOG_HeapCreate(DWORD dwOptions, DWORD dwInitSize, DWORD dwMaxSize, HANDLE hHeap) {
    CEL_HEAP_CREATE cl;
    CEL_MEMTRACK_HEAPCREATE cl2;

    cl.dwOptions = dwOptions;
    cl.dwInitSize = dwInitSize;
    cl.dwMaxSize = dwMaxSize;
    cl.hHeap = hHeap;

    CELOGDATA(TRUE, CELID_HEAP_CREATE, &cl, sizeof(CEL_HEAP_CREATE), 0, CELZONE_HEAP);

    cl2.dwPID = GetCurrentProcessId ();
    cl2.dwTID = GetCurrentThreadId ();
    cl2.dwOptions = dwOptions;
    cl2.dwInitSize = dwInitSize;
    cl2.dwMaxSize = dwMaxSize;
    cl2.hHeap = hHeap;

    CELOGDATA(TRUE, CELID_MEMTRACK_HEAPCREATE, &cl2, sizeof(CEL_MEMTRACK_HEAPCREATE), 0, CELZONE_MEMTRACKING);
}

//----------------------------------------------------------
_inline void CELOG_HeapAlloc(HANDLE hHeap, DWORD dwFlags, DWORD dwBytes, DWORD lpMem) {
    
    if (lpMem) {
        CEL_HEAP_ALLOC cl;
    	union {
	        CEL_MEMTRACK_ALLOC cl;
	        BYTE b[sizeof(CEL_MEMTRACK_ALLOC) + MAX_STACK_FRAME * sizeof(DWORD)];
	    } u;
        USHORT dwSize = sizeof(CEL_MEMTRACK_ALLOC);

        cl.hHeap   = hHeap;
        cl.dwFlags = dwFlags;
        cl.dwBytes = dwBytes;
        cl.lpMem   = lpMem;

        CELOGDATA(TRUE, CELID_HEAP_ALLOC, &cl, sizeof(CEL_HEAP_ALLOC), 0, CELZONE_HEAP);

        u.cl.hHeap = hHeap;
        u.cl.dwFlags = dwFlags;
        u.cl.dwBytes = dwBytes;
        u.cl.lpMem = lpMem;
		u.cl.dwTID = GetCurrentThreadId ();
		u.cl.dwPID = GetCurrentProcessId ();
		u.cl.dwCallerPID = (DWORD)GetCallerProcess ();

#if defined (_X86_)
        {
            DWORD FrameBase = 0;
            DWORD *x86BasePtr = 0;
            DWORD *pRetAddr = (DWORD *)&(u.cl.adwStackTrace[0]);
            int i;

            dwSize = offsetof(CEL_MEMTRACK_REALLOC, adwStackTrace);
            __asm mov FrameBase, ebp;
            
            x86BasePtr = (DWORD*) FrameBase;

            for (i = 0 ; i < MAX_STACK_FRAME ; ++i) {
                //
                // TLS is carved off the top of the thread's stack so we shouldn't ever
                // read past (or into) this area.
                //
                if ((DWORD) x86BasePtr >= (DWORD) UTlsPtr() || (DWORD) x86BasePtr < FrameBase)
                    break;

                __try {
                    *pRetAddr = *(x86BasePtr + 1);
                    x86BasePtr = *(DWORD **)x86BasePtr;
                    ++pRetAddr;
                    dwSize += sizeof(DWORD);
                } __except (EXCEPTION_EXECUTE_HANDLER) {
                    break;
                }
            }
        }
#endif

        CELOGDATA(TRUE, CELID_MEMTRACK_ALLOC, &u.cl, dwSize, 0, CELZONE_MEMTRACKING);
    }
}

//----------------------------------------------------------
_inline void CELOG_HeapRealloc(HANDLE hHeap, DWORD dwFlags, DWORD dwBytes, DWORD lpMemOld, DWORD lpMem) {
    
    if (lpMem) {
        CEL_HEAP_REALLOC cl;
    	union {
	        CEL_MEMTRACK_REALLOC cl;
	        BYTE b[sizeof(CEL_MEMTRACK_REALLOC) + MAX_STACK_FRAME * sizeof(DWORD)];
	    } u;
        USHORT dwSize = sizeof(CEL_MEMTRACK_REALLOC);

        cl.hHeap = hHeap;
        cl.dwFlags = dwFlags;
        cl.dwBytes = dwBytes;
        cl.lpMemOld = lpMemOld;
        cl.lpMem = lpMem;
    
        CELOGDATA(TRUE, CELID_HEAP_REALLOC, &cl, sizeof(CEL_HEAP_REALLOC), 0, CELZONE_HEAP);

        u.cl.hHeap = hHeap;
        u.cl.dwFlags = dwFlags;
        u.cl.dwBytes = dwBytes;
        u.cl.lpMem = lpMem;
        u.cl.lpMemOld = lpMemOld;
		u.cl.dwTID = GetCurrentThreadId ();
		u.cl.dwPID = GetCurrentProcessId ();
		u.cl.dwCallerPID = (DWORD)GetCallerProcess ();

#if defined (_X86_)
        {
            DWORD FrameBase = 0;
            DWORD *x86BasePtr = 0;
            DWORD *pRetAddr = (DWORD *)&(u.cl.adwStackTrace[0]);
            int i;

            dwSize = offsetof(CEL_MEMTRACK_REALLOC, adwStackTrace);
            __asm mov FrameBase, ebp;
            
            x86BasePtr = (DWORD*) FrameBase;

            for (i = 0 ; i < MAX_STACK_FRAME ; ++i) {
                //
                // TLS is carved off the top of the thread's stack so we shouldn't ever
                // read past (or into) this area.
                //
                if ((DWORD) x86BasePtr >= (DWORD) UTlsPtr() || (DWORD) x86BasePtr < FrameBase)
                    break;

                __try {
                    *pRetAddr = *(x86BasePtr + 1);
                    x86BasePtr = *(DWORD **)x86BasePtr;
                    ++pRetAddr;
                    dwSize += sizeof(DWORD);
                } __except (EXCEPTION_EXECUTE_HANDLER) {
                    break;
                }
            }
        }
#endif

        CELOGDATA(TRUE, CELID_MEMTRACK_REALLOC, &u.cl, dwSize, 0, CELZONE_MEMTRACKING);
    }
}

//----------------------------------------------------------
_inline void CELOG_HeapFree(HANDLE hHeap, DWORD dwFlags, DWORD lpMem) {
   	CEL_HEAP_FREE cl;
   	union {
        CEL_MEMTRACK_FREE cl;
        BYTE b[sizeof(CEL_MEMTRACK_FREE) + MAX_STACK_FRAME * sizeof(DWORD)];
    } u;
    USHORT dwSize = sizeof(CEL_MEMTRACK_REALLOC);

    cl.hHeap = hHeap;
    cl.dwFlags = dwFlags;
    cl.lpMem = lpMem;
    
    CELOGDATA(TRUE, CELID_HEAP_FREE, &cl, sizeof(CEL_HEAP_FREE), 0, CELZONE_HEAP);

    u.cl.hHeap = hHeap;
    u.cl.dwFlags = dwFlags;
    u.cl.lpMem = lpMem;
	u.cl.dwTID = GetCurrentThreadId ();
	u.cl.dwPID = GetCurrentProcessId ();
	u.cl.dwCallerPID = (DWORD)GetCallerProcess ();

#if defined (_X86_)
    {
        DWORD FrameBase = 0;
        DWORD *x86BasePtr = 0;
        DWORD *pRetAddr = (DWORD *)&(u.cl.adwStackTrace[0]);
        int i;

        dwSize = offsetof(CEL_MEMTRACK_REALLOC, adwStackTrace);
        __asm mov FrameBase, ebp;

        x86BasePtr = (DWORD*) FrameBase;

        for (i = 0 ; i < MAX_STACK_FRAME ; ++i) {
            //
            // TLS is carved off the top of the thread's stack so we shouldn't ever
            // read past (or into) this area.
            //
            if ((DWORD) x86BasePtr >= (DWORD) UTlsPtr() || (DWORD) x86BasePtr < FrameBase)
                break;

            __try {
                *pRetAddr = *(x86BasePtr + 1);
                x86BasePtr = *(DWORD **)x86BasePtr;
                ++pRetAddr;
                dwSize += sizeof(DWORD);
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                break;
            }
        }
    }
#endif

    CELOGDATA(TRUE, CELID_MEMTRACK_FREE, &u.cl, dwSize, 0, CELZONE_MEMTRACKING);
}

//----------------------------------------------------------
_inline void CELOG_HeapDestroy(HANDLE hHeap) {
    
    CEL_HEAP_DESTROY cl;
    CEL_MEMTRACK_HEAPDESTROY cl2;

    cl.hHeap = hHeap;
    cl2.hHeap = hHeap;
    cl2.dwPID = GetCurrentProcessId ();
    cl2.dwTID = GetCurrentThreadId ();

    CELOGDATA(TRUE, CELID_HEAP_DESTROY, &cl, sizeof(CEL_HEAP_DESTROY), 0, CELZONE_HEAP);
    CELOGDATA(TRUE, CELID_MEMTRACK_HEAPDESTROY, &cl2, sizeof(CEL_MEMTRACK_HEAPDESTROY), 0, CELZONE_MEMTRACKING);
}

#else  // CELOG

#define CELOG_HeapCreate(dwOptions, dwInitSize, dwMaxSize, hHeap)      ((void)0)
#define CELOG_HeapAlloc(hHeap, dwFlags, dwBytes, lpMem)                ((void)0)
#define CELOG_HeapRealloc(hHeap, dwFlags, dwBytes, lpMemOld, lpMem)    ((void)0)
#define CELOG_HeapFree(hHeap, dwFlags, lpMem)                          ((void)0)
#define CELOG_HeapDestroy(hHeap)                                       ((void)0)

#endif  // CELOG
