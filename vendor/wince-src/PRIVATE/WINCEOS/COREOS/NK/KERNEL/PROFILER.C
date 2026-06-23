/*++
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

Module Name:  

Abstract:  
 This file implements the NK kernel profiler support.

Functions:


Notes: 

--*/
#define C_ONLY
#include "kernel.h"
#include "romldr.h"
#include "xtime.h"
#include <profiler.h>

#define PROFILEMSG(cond,printf_exp)   \
   ((cond)?(NKDbgPrintfW printf_exp),1:0)

#ifdef KCALL_PROFILE

#include "kcall.h"
KPRF_t KPRFInfo[MAX_KCALL_PROFILE];

#ifdef NKPROF
#define KCALL_NextThread	10	// Next thread's entry in array
#endif
#endif

int (*PProfileInterrupt)(void)=NULL;	// pointer to profiler ISR, 
										// set in platform profiler code

BOOL bProfileTimerRunning=FALSE;		// this variable is used in OEMIdle(), so must be defined even in a non-profiling kernel

#ifdef NKPROF
BOOL bProfileObjCall=FALSE;				// object call profiling flag
//volatile BOOL bProfileKCall=FALSE;		// kcall profiling flag
BOOL bProfileKCall=FALSE;		// kcall profiling flag
BOOL bProfileBuffer=FALSE;				// profile to buffer flag
extern void SC_DumpKCallProfile(DWORD bReset);
extern BOOL ProfilerAllocBuffer(void);
extern BOOL ProfilerFreeBuffer(void);
#endif

#ifdef WINCECODETEST
extern void CT_Profile(DWORD Start);
#endif

// SC_ProfileSyscall - profiler syscall support
void SC_ProfileSyscall(LPXT lpXt)
{
#ifdef NKPROF
	static int scPauseContinueCalls = 0;
	static BOOL bStart = FALSE;

	if (lpXt->dwOp == XTIME_PROFILE_DATA)
	{
		if(!lpXt->dwTime[0])			// if starting profiling
		{
			if(lpXt->dwTime[2] & PROFILE_CONTINUE)
			{
				if(bStart)
				{
					++scPauseContinueCalls;
					// start profiler timer on 0 to 1 transition
					if(1 == scPauseContinueCalls)
					{
						OEMProfileTimerEnable(lpXt->dwTime[1]);
						bProfileTimerRunning = TRUE;
					}
				}
			}
			else if(lpXt->dwTime[2] & PROFILE_PAUSE)
			{
				if(bStart)
				{
					--scPauseContinueCalls;
					// start profiler timer on 1 to 0 transition
					if(!scPauseContinueCalls)
					{
						OEMProfileTimerDisable();
						bProfileTimerRunning = FALSE;
					}
				}
			}
			else 
			{
				OEMProfileTimerDisable();		// disable profiler timer
				bProfileTimerRunning = FALSE;
				bStart = TRUE;
				++scPauseContinueCalls;
				
				dwProfileCount=0;			// clear profile count
				ClearProfileHits();			// reset all profile counts
				
				
				// check object call profile flag
				if (lpXt->dwTime[2] & PROFILE_OBJCALL)
					bProfileObjCall=TRUE;
				else
					bProfileObjCall=FALSE;

				// check kcall profile flag
				if (lpXt->dwTime[2] & PROFILE_KCALL)
				{
					bProfileKCall=TRUE;
					return;
				}
				else
				{
					bProfileKCall=FALSE;
				}

				// check profile to buffer flag
				if (lpXt->dwTime[2] & PROFILE_BUFFER)
				{
					bProfileBuffer=TRUE;
					if (!ProfilerAllocBuffer())
					{
						bProfileBuffer=FALSE;
					}
				}
				else
				{
					bProfileBuffer=FALSE;
				}
				
				
				if (bProfileObjCall)
				{
					// We don't need special timers for this case.
					return;
				}
											// start profiler timer
				if(!(lpXt->dwTime[2] & PROFILE_STARTPAUSED))
				{
					// start profiler timer
					OEMProfileTimerEnable(lpXt->dwTime[1]);
					bProfileTimerRunning = TRUE;
				}
			}	
		}	
		else
		{
			if(bProfileTimerRunning)
			{
				OEMProfileTimerDisable();		// disable profiler timer
				bProfileTimerRunning = FALSE;
			}
			
			// stopping profile
			bStart = FALSE;
			if (bProfileKCall)			// if profile KCALL enabled
			{
                bProfileKCall=FALSE;
				// dump the KCall profile data
				SC_DumpKCallProfile(TRUE);
			}
			else						// else display profile hit report
			{
				ProfilerReport();
			}

			if(bProfileBuffer)
			{
				ProfilerFreeBuffer();
				bProfileBuffer = FALSE;
			}

			bProfileObjCall = FALSE;
			scPauseContinueCalls = 0;
		}
	}
#endif

#ifdef WINCECODETEST
	if (lpXt->dwOp == XTIME_CODETEST)
	{
		// Start/Stop CodeTEST profiling
		CT_Profile(lpXt->dwTime[0]);
	}
#endif
}

void SC_TurnOnProfiling(void) {
	DEBUGMSG(ZONE_ENTRY,(L"SC_TurnOnProfiling entry\r\n"));
	SET_PROFILE(pCurThread);
#if SHx
	ProfileFlag = 1;	// profile status
#endif
	DEBUGMSG(ZONE_ENTRY,(L"SC_TurnOnProfiling exit\r\n"));
}

void SC_TurnOffProfiling(void) {
	DEBUGMSG(ZONE_ENTRY,(L"SC_TurnOffProfiling entry\r\n"));
	CLEAR_PROFILE(pCurThread);
#if SHx
	ProfileFlag = 0;	// profile status
#endif
	DEBUGMSG(ZONE_ENTRY,(L"SC_TurnOffProfiling exit\r\n"));
}

#ifdef KCALL_PROFILE
void GetKCallProfile(KPRF_t *pkprf, int loop, BOOL bReset) {
	KCALLPROFON(26);
	memcpy(pkprf,&KPRFInfo[loop],sizeof(KPRF_t));
	if (bReset && (loop != 26))
		memset(&KPRFInfo[loop],0,sizeof(KPRF_t));
	KCALLPROFOFF(26);
	if (bReset && (loop == 26))
		memset(&KPRFInfo[loop],0,sizeof(KPRF_t));
}
#endif

//
// Convert the number of ticks to microseconds.
//
static DWORD 
local_ScaleDown(DWORD dwIn)
{
    LARGE_INTEGER liFreq;

    SC_QueryPerformanceFrequency(&liFreq);

	return ((DWORD) (((__int64) dwIn * 1000000) / liFreq.LowPart));
}


void SC_DumpKCallProfile(DWORD bReset) {
#ifdef KCALL_PROFILE
	extern DWORD local_ScaleDown(DWORD);
	int loop;
	KPRF_t kprf;
	DWORD min = 0xffffffff, max = 0, total = 0, calls = 0;

#ifdef NKPROF
	calls = local_ScaleDown(1000);
    PROFILEMSG(1,(L"Resolution: %d.%3.3d usec per tick\r\n",calls/1000,calls%1000));
	KCall((PKFN)GetKCallProfile,&kprf,KCALL_NextThread,bReset);
	PROFILEMSG(1,(L"NextThread: Calls=%u Min=%u Max=%u Ave=%u\r\n",
		kprf.hits,local_ScaleDown(kprf.min),
		local_ScaleDown(kprf.max),kprf.hits ? local_ScaleDown(kprf.total)/kprf.hits : 0));
	for (loop = 0; loop < MAX_KCALL_PROFILE; loop++) {
		if (loop != KCALL_NextThread) {
			KCall((PKFN)GetKCallProfile,&kprf,loop,bReset);
			if (kprf.max > max)
				max = kprf.max;
			total+=kprf.total;
			calls+= kprf.hits;
		}
	}
	PROFILEMSG(1,(L"Other Kernel calls: Max=%u Avg=%u\r\n",max,calls ? local_ScaleDown(total)/calls : 0));
#else
	calls = local_ScaleDown(1000);
	PROFILEMSG(1,(L"Resolution: %d.%3.3d usec per tick\r\n",calls/1000,calls%1000));
	PROFILEMSG(1,(L"Index Entrypoint                        Calls      uSecs    Min    Max    Ave\r\n"));
	for (loop = 0; loop < MAX_KCALL_PROFILE; loop++) {
		KCall((PKFN)GetKCallProfile,&kprf,loop,bReset);
		PROFILEMSG(1,(L"%5d %-30s %8d %10d %6d %6d %6d\r\n",
			loop, pKCallName[loop], kprf.hits,local_ScaleDown(kprf.total),local_ScaleDown(kprf.min),
			local_ScaleDown(kprf.max),kprf.hits ? local_ScaleDown(kprf.total)/kprf.hits : 0));
		if (kprf.min && (kprf.min < min))
			min = kprf.min;
		if (kprf.max > max)
			max = kprf.max;
		calls += kprf.hits;
		total += kprf.total;
	}
	PROFILEMSG(1,(L"\r\n"));
	PROFILEMSG(1,(L"      %-30s %8d %10d %6d %6d %6d\r\n",
		L"Summary", calls,local_ScaleDown(total),local_ScaleDown(min),local_ScaleDown(max),calls ? local_ScaleDown(total)/calls : 0));
#endif
#endif
}



#ifdef CELOG

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// CELOG RESYNC API:  This function generates a series of calls to CeLogData, 
// to log all currently-existing processes and threads.
//

// This buffer is used during resync to minimize stack usage
BYTE g_pCeLogSyncBuffer[(MAX_PATH * sizeof(WCHAR))
                        + max(sizeof(CEL_PROCESS_CREATE), sizeof(CEL_THREAD_CREATE))];

// Helper func to generate a celog process create event, minimizing stack usage.
// At most one of lpProcNameA, lpProcNameW can be non-null!
_inline void CELOG_SyncProcess(HANDLE hProcess, LPSTR lpProcNameA, LPWSTR lpProcNameW)
{
    PCEL_PROCESS_CREATE pcl = (PCEL_PROCESS_CREATE) &g_pCeLogSyncBuffer;
    WORD wLen = 0;

    pcl->hProcess = hProcess;

    if (lpProcNameW) {
        wLen = strlenW(lpProcNameW) + 1;
        kstrcpyW(pcl->szName, lpProcNameW); 
    } else if (lpProcNameA) {
        wLen = strlen(lpProcNameA) + 1;
        KAsciiToUnicode(pcl->szName, lpProcNameA, MAX_PATH);
    }

    CELOGDATA(TRUE, CELID_PROCESS_CREATE, (PVOID) pcl, (WORD) (sizeof(CEL_PROCESS_CREATE) + (wLen * sizeof(WCHAR))), 0, CELZONE_PROCESS);
}

// Helper func to generate a celog thread create event, minimizing stack usage.
_inline void CELOG_SyncThread(PTHREAD pThread, HANDLE hProcess)
{
    if (pThread) {
        PCEL_THREAD_CREATE pcl = (PCEL_THREAD_CREATE) &g_pCeLogSyncBuffer;
        WORD wLen = 0;

        pcl->hProcess = hProcess;
        pcl->hThread  = pThread->hTh;
        
        // Look up the thread's function name and module handle.
        GetThreadName(pThread, &(pcl->hModule), pcl->szName);
        if (pcl->szName[0] != 0) {
            wLen = strlenW(pcl->szName) + 1;
        }
        
        CELOGDATA(TRUE, CELID_THREAD_CREATE, (PVOID) pcl, 
                  (WORD)(sizeof(CEL_THREAD_CREATE) + (wLen * sizeof(WCHAR))),
                  0, CELZONE_THREAD);
    }
}


BOOL CeLogReSync()
{
    DWORD   dwProc;
    
    // KCall so nobody changes ProcArray out from under us
    if (!InSysCall()) {
        return KCall((PKFN)CeLogReSync);
    }
    
    KCALLPROFON(74);

    // Since we're in a KCall, we must limit stack usage, so we can't call 
    // CELOG_ProcessCreate or CELOG_ThreadCreate -- instead use CELOG_Sync*.
    
    for (dwProc = 0; dwProc < 32; dwProc++) {

        if (ProcArray[dwProc].dwVMBase) {

            THREAD* pThread;
            
            // Log the process name.  Since we are in a KCall, we must use the 
            // table of contents for everything besides nk.exe, because we
            // cannot reach into the memory owned by other processes.
            if (dwProc == 0) {
                // NK.EXE
                
                CELOG_SyncProcess(ProcArray[dwProc].hProc, NULL,
                                  ProcArray[dwProc].lpszProcName);

            } else if ((ProcArray[dwProc].oe.filetype == FT_ROMIMAGE)
                       && (ProcArray[dwProc].oe.tocptr)) {
                // Get the name from the TOC
                
                CELOG_SyncProcess(ProcArray[dwProc].hProc,
                                  ProcArray[dwProc].oe.tocptr->lpszFileName, NULL);
            
            } else {
                // We can't do anything if the process isn't nk.exe and isn't
                // in the table of contents.
                
                CELOG_SyncProcess(ProcArray[dwProc].hProc, NULL, NULL);
            }

            // Log the existence of each of the process' threads as a ThreadCreate
            pThread = ProcArray[dwProc].pTh;
            while (pThread != NULL) {
                CELOG_SyncThread(pThread, pThread->pOwnerProc->hProc);
                pThread = pThread->pNextInProc;
            }
        }
    }
    
    KCALLPROFOFF(74);
    
    return TRUE;
}

#endif // CELOG



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
//  The following functions are used for GetThreadName.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------


// Return values from GetModulePointer
#define MODULEPOINTER_ERROR  0
#define MODULEPOINTER_NK     1
#define MODULEPOINTER_DLL    2
#define MODULEPOINTER_PROC   4

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
static DWORD
GetModulePointer(
    PTHREAD pth,
    unsigned int pc,
    DWORD* dwResult
    ) 
{
    if (!dwResult) {
       return MODULEPOINTER_ERROR;
    }
    *dwResult = (DWORD) NULL;


    if (pc & 0x80000000) {
        //
        // NK.EXE
        //
        return MODULEPOINTER_NK;

    } else if (pc > (DWORD) DllLoadBase && pc < 0x02000000) {
        //
        // It's a DLL address
        //
        PMODULE pModMax = NULL, pModTrav = NULL;
        DWORD dwMax = 0;

        //
        // Scan the modules list looking for a module with the highest base 
        // pointer that's less than the PC
        //
        pModTrav = pModList;

        while (pModTrav) {
            DWORD dwBase = ZeroPtr(pModTrav->BasePtr);

            if (dwBase > dwMax && dwBase < pc) {
                dwMax = dwBase;
                pModMax = pModTrav;
            }
            pModTrav = pModTrav->pMod;  // Next module.
        }
        
        if (pModMax != NULL) {
            if (pModMax->oe.filetype == FT_ROMIMAGE) {
                *dwResult = (DWORD) pModMax;
                return MODULEPOINTER_DLL;
            } 
        }
    } else {
        //
        // We assume it's a process
        //
        if (pth->pOwnerProc->oe.filetype == FT_ROMIMAGE) {
            *dwResult = (DWORD) (pth->pOwnerProc);
            return MODULEPOINTER_PROC;
        } 
    }
    
    return (MODULEPOINTER_ERROR);
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
static TOCentry*
GetTocPointer(
    PTHREAD pth,
    unsigned int pc
    ) 
{
    TOCentry* tocptr = NULL;
    DWORD dwModulePointer = (DWORD) NULL;
    DWORD dwMPFlag = GetModulePointer(pth, pc, &dwModulePointer);

    switch (dwMPFlag) {
        case MODULEPOINTER_NK: {
            // nk.exe: tocptr-> first entry in ROM
            tocptr=(TOCentry *)(pTOC+1);
            break;
        }
        
        case MODULEPOINTER_DLL: {
            PMODULE pMod = (PMODULE) dwModulePointer;
            tocptr = pMod->oe.tocptr;
            break;
        }
        
        case MODULEPOINTER_PROC: {
            PPROCESS pProc = (PPROCESS) dwModulePointer;    
            tocptr = pProc->oe.tocptr;
            break;
        }
    
        default: {
            // Error
            tocptr = NULL;
        }
    }
    
    return (tocptr);
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
static HANDLE
GetModHandle(
    PTHREAD pth,
    unsigned int pc
    ) 
{
    HANDLE hModule;
    DWORD dwModulePointer = (DWORD) NULL;
    DWORD dwMPFlag = GetModulePointer(pth, pc, &dwModulePointer);

    switch (dwMPFlag) {
        case MODULEPOINTER_NK: {
            // nk.exe is always the first process
            hModule = (HANDLE) ProcArray[0].hProc;
            break;
        }
        
        case MODULEPOINTER_DLL: {
            // A DLL doesn't have a handle, so return the pointer.
            hModule = (HANDLE) dwModulePointer;
            break;
        }

        case MODULEPOINTER_PROC: {
            hModule = (HANDLE) dwModulePointer;
            break;
        }
    
        default: {
            // Error
            hModule = INVALID_HANDLE_VALUE;
        }
    }
    
    return (hModule);
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
LPCSTR
GetModName(
    PTHREAD pth,
    unsigned int pc
    ) 
{
    LPCSTR pszModName = NULL;
    TOCentry* tocptr;

    tocptr = GetTocPointer(pth, pc);
    if (tocptr != NULL) {
        pszModName = tocptr->lpszFileName;
    }
    
    return (pszModName);
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// Note: this was called GetSymbol but that didn't want to compile...
static LPBYTE
GetFuncSymbol(LPBYTE lpSym, DWORD dwSymNum)
{
    while (dwSymNum > 0) {
        while (*lpSym++);
        dwSymNum--;
    }
    return lpSym;
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
LPCSTR
GetFunctionName(
    PTHREAD pth,
    unsigned int pc
    ) 
{
    LPCSTR      pszFunctionName = NULL;
    TOCentry    *tocptr;                // table of contents entry pointer
    PROFentry   *profptr=NULL;          // profile entry pointer
    SYMentry    *symptr;                // profiler symbol entry pointer
    unsigned int iMod;                  // module enty in TOC
    unsigned int iSym;                  // symbol counter

    if (!pTOC->ulProfileOffset) {
        //
        // No symbols available.
        //
        return (NULL);
    }
    
    if (pc & 0x80000000) {
        //
        // These addresses belong to NK.EXE.
        //
        if (pc >= 0xA0000000) {
            //
            // Hit a PSL exception address or perhaps a KDATA routine. No name.
            //
            return (NULL);
        }
        //
        // Mask off the caching bit.
        //
        pc &= 0xdfffffff;
    }
    
    //
    // Find the TOC entry if one exists. If not in ROM, then we can't get a name.
    //
    tocptr = GetTocPointer(pth, pc);
    if (tocptr == NULL) {
        return (NULL);
    }
    
    //
    // Compute the module number
    //
    iMod= ((DWORD)tocptr - (DWORD)(pTOC+1)) / sizeof(TOCentry);
    //
    // make profptr point to entry for this module
    //
    profptr  = (PROFentry *)pTOC->ulProfileOffset;
    profptr += iMod;
    
    //
    // If there are any symbols in this module, scan for the function.
    //
    if (profptr->ulNumSym) {
        unsigned int iClosestSym;            // index of nearest symbol entry
        SYMentry*    pClosestSym;            // ptr to nearest symbol entry
        
        iSym = 0;
        symptr=(SYMentry*)profptr->ulHitAddress;
        iClosestSym = iSym;
        pClosestSym = symptr;
        
        //
        // Scan the whole list of symbols, looking for the closest match.
        //
        while ((iSym < profptr->ulNumSym) && (pClosestSym->ulFuncAddress != pc)) {
            // Keep track of the closest symbol found
            if ((symptr->ulFuncAddress <= pc)
                && (symptr->ulFuncAddress > pClosestSym->ulFuncAddress)) {
                iClosestSym = iSym;
                pClosestSym = symptr;
            }
            
            iSym++;
            symptr++;
        }
        
        pszFunctionName = (LPCSTR) GetFuncSymbol((LPBYTE) profptr->ulSymAddress, iClosestSym);
    }

    return (pszFunctionName);
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void
GetThreadName(
    PTHREAD pth,
    HANDLE* hModule,
    WCHAR*  szThreadFunctionName
    )
{
   DWORD  dwProgramCounter = 0;

   //
   // Return a pointer to the thread's name if available.
   //
   if (pth != NULL) {

       // The thread's program counter is saved off when it is created.
       dwProgramCounter = pth->dwStartAddr;
       

       if (hModule != NULL) {
           *hModule = GetModHandle(pth, dwProgramCounter);
       }
       
       
       if (szThreadFunctionName != NULL) {
           
           if (dwProgramCounter == (DWORD) CreateNewProc) {
               //
               // Creating a new process, so use the proc name instead of the func
               //

               LPSTR lpszTemp;
                
               // First try to use the TOC to get the process name
               if (pth->pOwnerProc
                   && (pth->pOwnerProc->oe.filetype == FT_ROMIMAGE)
                   && (pth->pOwnerProc->oe.tocptr)
                   && (lpszTemp = pth->pOwnerProc->oe.tocptr->lpszFileName)) {
                   
                   KAsciiToUnicode(szThreadFunctionName, lpszTemp, MAX_PATH);
               
               } else if (!InSysCall()) {
                   // If we are not inside a KCall we can use the proc struct

                   LPWSTR lpszTempW;
                   
                   if (pth->pOwnerProc
                       && (lpszTempW = pth->pOwnerProc->lpszProcName)) {
                       
                       DWORD dwLen = strlenW(lpszTempW) + 1;
                       memcpy(szThreadFunctionName, lpszTempW, dwLen * sizeof(WCHAR));
                   } else {
                       szThreadFunctionName[0] = 0;
                   }
                   
               } else {
                   // Otherwise we have no way to get the thread's name
                   szThreadFunctionName[0] = 0;
               }
               
           } else {
               //
               // Look up the function name
               //
               
               LPSTR lpszTemp = (LPSTR) GetFunctionName(pth, dwProgramCounter);
               if (lpszTemp) {
                   KAsciiToUnicode(szThreadFunctionName, lpszTemp, MAX_PATH);
               } else {
                   szThreadFunctionName[0] = 0;
               }
           }
       }
       
   } else {
       if (hModule != NULL) {
           *hModule = INVALID_HANDLE_VALUE;
       }
       if (szThreadFunctionName != NULL) {
           szThreadFunctionName[0] = 0;
       }
   }
}

