/*
 * kstubs.c - kernel-side functions absent from the WINCEOS leak snapshot.
 * Not OAL proper, but the kernel core references them and the leak's compress/
 * JIT/kernel-debugger sources aren't present. Honest stubs for an nknodbg-style
 * (no-debugger) boot. See OAL-NOTES.md.
 */
#include <windows.h>

extern volatile DWORD CurMSec;          /* kernel tick (bumped by Timer0ISR) */

/* GetTickCount backing - the leak has no SC_GetTickCount body; return CurMSec. */
DWORD SC_GetTickCount(void)
{
    return CurMSec;
}

/* ROM compression. The leak ships no compress.c/nocompr; with COMPRESSION off in
 * the image these aren't hit. Stub as "no compression". */
DWORD CECompress(LPBYTE bufIn, DWORD cbIn, LPBYTE bufOut, DWORD cbOut,
                 WORD step, DWORD pagesize)
{
    return 0;       /* 0 bytes produced => caller stores uncompressed */
}

DWORD CEDecompress(LPBYTE bufIn, DWORD cbIn, LPBYTE bufOut, DWORD cbOut,
                   DWORD skip, WORD step, DWORD pagesize)
{
    return (DWORD)-1;
}

/* JIT loader (CE 3.0). ModuleJit is a real kernel function (also in the syscall
 * table); InitializeJit(PFNOPEN,PFNCLOSE) returns 0 => no JIT present. */
DWORD ModuleJit(LPCWSTR p1, LPWSTR p2, HANDLE *p3)  { return 0; }
int   InitializeJit(void *pfnOpen, void *pfnClose)  { return 0; }

/* Kernel debugger: PKDInit is a function-POINTER the kernel null-checks
 * (KernelInit2: `if (PKDInit) PKDInit(...)`). It MUST be a NULL pointer for a
 * no-debugger kernel - a stub FUNCTION makes the check non-null and the kernel
 * then calls garbage (the cause of the PC=0xE3007FFC fault inside "KdInit"). */
BOOLEAN (*PKDInit)(LPVOID *, LPVOID *, LPVOID *, LPVOID, LPVOID *, LPVOID *) = NULL;
