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

/* JIT loader (CE 3.0 .NET/AppDomain JIT) - not used at boot. */
DWORD ModuleJit(LPCWSTR p1, LPWSTR p2, HANDLE *p3)  { return 0; }
DWORD InitializeJit(void)                           { return 0; }

/* Kernel debugger init (PKD / KdStub). nknodbg has no debugger. */
BOOL PKDInit(void)                                  { return FALSE; }
