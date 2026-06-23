/*
 *		NK Statistical Profiler API's
 *
 *		Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.
 *
 * Module Name:
 *
 *		profiler.c
 *
 * Abstract:
 *
 *		This file implements the NK statistical profiler API's
 *
 *
 * Revision History:
 *
 */
extern void * *Win32Methods;
extern int bAllKMode;

#define WIN32_CALL(type, api, args) (bAllKMode ? (*(type (*)args)(Win32Methods[W32_ ## api])) : (IMPLICIT_DECL(type, SH_WIN32, W32_ ## api, args)))
#define KILLTHREADIFNEEDED() {if (bAllKMode) (*(void (*)(void))(Win32Methods[151]))();}

#include <windows.h>
#include <coredll.h>
#define C_ONLY
#include "kernel.h"
#include "xtime.h"
#include "profiler.h"

// AMC CodeTEST code
#ifdef WINCECODETEST

// amc_ctrl_port and amc_data_port definitions
#include "tagaddrs.h"

// CodeTEST profiler base address/port address functions
#define W32_GetProfileBaseAddress 154
#define W32_SetProfilePortAddress 155

#define GetProfileBaseAddress PRIV_WIN32_CALL(LPVOID, GetProfileBaseAddress, (void))
#define SetProfilePortAddress PRIV_WIN32_CALL(void, SetProfilePortAddress, (LPVOID))

// Page alignment macros
#define PA(A) ((((DWORD)(A)) / PAGE_SIZE) * PAGE_SIZE)
#define EX(A) (((DWORD)(A)) - PA(A))
#endif // WINCECODETEST

//---------------------------------------------------------------------------
//	ProfileInit - Perform initialization tasks for profiling
//
//---------------------------------------------------------------------------
DWORD ProfileInit(void)
{
#ifdef WINCECODETEST
    amc_ctrl_port = (LPVOID)((DWORD)GetProfileBaseAddress( ) + EX(AMC_CTRL_PORT_ADDR));
	amc_data_port = amc_ctrl_port - EX(AMC_CTRL_PORT_ADDR) + EX(AMC_DATA_PORT_ADDR);
	RETAILMSG(1, (TEXT("CodeTEST: ProfileInit: amc_ctrl_port = 0x%08X\n"), amc_ctrl_port));
#endif

    return 0;
}

//---------------------------------------------------------------------------
//	ProfileStart - clear profile counters and enable profiler ISR
//
//	INPUT:	dwUSecInterval - non-zero = sample interval in uSec
//								 zero= reset uSec counter
//				bPerProcess - TRUE - profile current process only
//							  FALSE - profile all samples
//---------------------------------------------------------------------------
DWORD ProfileStart(DWORD dwUSecInterval, DWORD dwOptions)
{
	DWORD dwProfileLen[4];

#ifdef WINCECODETEST
	switch (dwOptions)
	{
	default:
#endif
		// call ProfileSyscall with op= XTIME_PROFILE_DATA
		// and size=0 to clear counters and enable ISR
		dwProfileLen[0]=XTIME_PROFILE_DATA;
		dwProfileLen[1]=0;
		dwProfileLen[2]= dwUSecInterval;
		dwProfileLen[3]= dwOptions;
		ProfileSyscall(dwProfileLen);
		return 0;

#ifdef WINCECODETEST
	case PROFILE_CODETEST:
        RETAILMSG(1, (TEXT("CodeTEST: ProfileStart\n")));

        // Start profiling by redirecting tags to the instrumentation ports
		// (kernel).
		dwProfileLen[0] = XTIME_CODETEST;
		dwProfileLen[1] = 1;
		ProfileSyscall(dwProfileLen);

		// Start profiling by redirecting flags to the instrumentation ports
		// (coredll).  Note: AMC_CTRL_PORT_ADDR must be on page boundary
		SetProfilePortAddress((LPVOID)(PA(AMC_CTRL_PORT_ADDR) / 256));
		
		return 0;   	
	}
#endif
}


//---------------------------------------------------------------------------
//	ProfileStop - Stop profile and display hit report
//
//
//---------------------------------------------------------------------------
void ProfileStop(void)
{
	DWORD dwProfileLen[4];

#ifdef WINCECODETEST
    // Stop profiling by redirecting tags to unused memory (kernel)
	dwProfileLen[0] = XTIME_CODETEST;
	dwProfileLen[1] = 0;
	ProfileSyscall(dwProfileLen);

	// Stop profiling by redirecting tags to unused memory (coredll)
	SetProfilePortAddress(0);

	RETAILMSG(1, (TEXT("CodeTEST: ProfileStop\n")));
#endif

	dwProfileLen[0]=XTIME_PROFILE_DATA;	// call ProfileSyscall with op= XTIME_PROFILE_DATA
	dwProfileLen[1]=1;					// and size=1 to disable ISR and return profile data size
	ProfileSyscall(dwProfileLen);
	return;
}


#ifdef CELOG

//
// This variable is only non-NULL if CELOGCORE is included into COREDLL
//
VOID (*pCeLogData)(BOOL fTimeStamp, WORD wID, PVOID pData, WORD wLen, 
                   DWORD dwZoneUser, DWORD dwZoneCE, WORD wFlag, BOOL bFlagged);
VOID (*pCeLogSetZones)(DWORD dwZoneUser, DWORD dwZoneCE, DWORD dwZoneProcess);

extern DWORD bProfilingKernel;

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
VOID xxx_CeLogData(
    BOOL fTimeStamp, 
    WORD wID, 
    PVOID pData, 
    WORD wLen, 
    DWORD dwZoneUser, 
    DWORD dwZoneCE,
    WORD wFlag,
    BOOL bFlagged)
{
    if (pCeLogData) {
        //
        // Call the replacement function.
        //
        pCeLogData(fTimeStamp, wID, pData, wLen, dwZoneUser, dwZoneCE,
                   wFlag, bFlagged);
        return;
    }
    
    if (bProfilingKernel) {
        CeLogData(fTimeStamp, wID, pData, wLen, dwZoneUser, dwZoneCE,
                  wFlag, bFlagged);
        KILLTHREADIFNEEDED();
    }
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
VOID xxx_CeLogSetZones(
    DWORD dwZoneUser, 
    DWORD dwZoneCE, 
    DWORD dwZoneProcess)
{
    if (pCeLogSetZones) {
        //
        // Call the replacement function.
        //
        pCeLogSetZones(dwZoneUser, dwZoneCE, dwZoneProcess);
        return;
    }
    
    if (bProfilingKernel) {
        CeLogSetZones(dwZoneUser, dwZoneCE, dwZoneProcess);
        KILLTHREADIFNEEDED();
    }
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
VOID xxx_CeLogReSync()
{
    if (bProfilingKernel) {
        CeLogReSync();
        KILLTHREADIFNEEDED();
    }
}


#else // CELOG

// Stubs for non-profiling COREDLL, provided in case logging functions are 
// called externally.

VOID xxx_CeLogData(
    BOOL fTimeStamp, 
    WORD wID, 
    PVOID pData, 
    WORD wLen, 
    DWORD dwZoneUser, 
    DWORD dwZoneCE,
    WORD wFlag,
    BOOL bFlagged)
{}

VOID xxx_CeLogSetZones(
    DWORD dwZoneUser, 
    DWORD dwZoneCE, 
    DWORD dwZoneProcess)
{}

VOID xxx_CeLogReSync()
{}


#endif // CELOG

