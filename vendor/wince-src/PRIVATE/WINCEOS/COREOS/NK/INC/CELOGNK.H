//------------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//  Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.
//  
//------------------------------------------------------------------------------
//
//  Module Name:  
//  
//      celognk.h
//  
//  Abstract:  
//
//      Kernel's interface to the event logging functions.
//      
//------------------------------------------------------------------------------
#ifndef __CELOGNK_H_
#define __CELOGNK_H_

#ifdef CELOG

//------------------------------------------------------------------------------
//  Logging operation (NKPROF.LIB)
//

// Include the public SDK structures.                
#include <celog.h>

// Include exported profiler functions
#include <profiler.h>

// Redefine CELOGDATA. We want core logging in NKPROF even in ship-build                    
#undef CELOGDATA
#undef CELOGDATAFLAGGED



extern void CeLogInit(void);
extern void CeLogCreateThread(void);

#define CELOGINIT()                       CeLogInit()
#define CELOGCREATETHREAD()               CeLogCreateThread()
#define CELOGDATA(T,I,D,L,Y,Z)            CeLogData(T,I,D,L,Y,Z,0,FALSE) 
#define CELOGDATAFLAGGED(T,I,D,L,Y,Z,F)   CeLogData(T,I,D,L,Y,Z,F,TRUE) 


// The kernel wrappers

//----------------------------------------------------------
_inline void CELOG_ThreadSwitch(PTHREAD pThread) {
    CEL_THREAD_SWITCH cl;

    if (pThread) {
        cl.hThread = pThread->hTh;
    } else {
        cl.hThread = 0;
    }

    CELOGDATA(TRUE, CELID_THREAD_SWITCH, &cl, sizeof(cl), 0, CELZONE_RESCHEDULE);
}

//----------------------------------------------------------
void CeLogThreadMigrate(HANDLE hProcess, DWORD dwReserved);

#define CELOG_ThreadMigrate(hProcess, dwReserved) \
        CeLogThreadMigrate(hProcess, dwReserved)

//----------------------------------------------------------
_inline void CELOG_ThreadCreate(PTHREAD pThread, HANDLE hProcess, LPCWSTR lpProcName) {

    if (pThread) {
        BYTE pTmp[sizeof(CEL_THREAD_CREATE) + 2*MAX_PATH];
        PCEL_THREAD_CREATE pcl = (PCEL_THREAD_CREATE) pTmp;
        WORD wLen = 0;

        pcl->hProcess = hProcess;
        pcl->hThread  = pThread->hTh;
        
        // If the process name was passed in, we assume this is the primary
        // thread of a process, and take the process' name and module handle.
        // Otherwise, look up the thread's function name and module handle.
        if (lpProcName) {
            wLen = strlenW(lpProcName) + 1;
            kstrcpyW(pcl->szName, lpProcName); 
            pcl->hModule = hProcess;
        } else {
            GetThreadName(pThread, &(pcl->hModule), pcl->szName);
            if (pcl->szName[0] != 0) {
                wLen = strlenW(pcl->szName) + 1;
            }
        }
        
        CELOGDATA(TRUE, CELID_THREAD_CREATE, (PVOID) pcl, 
                  (WORD)(sizeof(CEL_THREAD_CREATE) + (wLen * sizeof(WCHAR))),
                  0, CELZONE_THREAD);
    }

}

//----------------------------------------------------------
_inline void CELOG_ThreadCloseHandle(HANDLE hThread) {
    
    CEL_THREAD_CLOSE cl;
    
    cl.hThread  = hThread;

    CELOGDATA(TRUE, CELID_THREAD_CLOSE, &cl, sizeof(CEL_THREAD_CLOSE), 0, CELZONE_THREAD);
}

//----------------------------------------------------------
_inline void CELOG_ThreadTerminate(HANDLE hThread) {
    
    CEL_THREAD_TERMINATE cl;
    
    cl.hThread  = hThread;

    CELOGDATA(TRUE, CELID_THREAD_TERMINATE, &cl, sizeof(CEL_THREAD_TERMINATE), 0, CELZONE_THREAD);
}

//----------------------------------------------------------
_inline void CELOG_ThreadDelete(HANDLE hThread) {
    
    CEL_THREAD_DELETE cl;
    
    cl.hThread  = hThread;

    CELOGDATA(TRUE, CELID_THREAD_DELETE, &cl, sizeof(CEL_THREAD_DELETE), 0, CELZONE_THREAD);
}

//----------------------------------------------------------
_inline void CELOG_ProcessCreate(HANDLE hProcess, LPCWSTR lpProcName, DWORD dwVmBase) {
    BYTE pTmp[(MAX_PATH*sizeof(WCHAR))+max(sizeof(CEL_PROCESS_CREATE), sizeof(CEL_MEMTRACK_CREATEPROC))];
    PCEL_PROCESS_CREATE pcl = (PCEL_PROCESS_CREATE) pTmp;
    PCEL_MEMTRACK_CREATEPROC pcl2 = (PCEL_MEMTRACK_CREATEPROC) pTmp;
    WORD wLen = 0;
    
    pcl->hProcess = hProcess;
    
    if (lpProcName) {
        wLen = strlenW(lpProcName) + 1;
        kstrcpyW(pcl->szName, lpProcName); 
    }
    
    CELOGDATA(TRUE, CELID_PROCESS_CREATE, (PVOID) pcl, (WORD) (sizeof(CEL_PROCESS_CREATE) + (wLen * sizeof(WCHAR))), 0, CELZONE_PROCESS);

    pcl2->hProcess = hProcess;
    pcl2->dwVmBase = dwVmBase;

    if (wLen)
        kstrcpyW(pcl2->szName, lpProcName); 

    CELOGDATA(TRUE, CELID_MEMTRACK_CREATEPROC, (PVOID) pcl2, (WORD) (sizeof(CEL_MEMTRACK_CREATEPROC) + (wLen * sizeof(WCHAR))), 0, CELZONE_MEMTRACKING);
}

//----------------------------------------------------------
_inline void CELOG_ProcessCloseHandle(HANDLE hProcess) {
    
    CEL_PROCESS_CLOSE cl;
    
    cl.hProcess = hProcess;
    
    CELOGDATA(TRUE, CELID_PROCESS_CLOSE, &cl, sizeof(CEL_PROCESS_CLOSE), 0, CELZONE_PROCESS);
}

//----------------------------------------------------------
_inline void CELOG_ProcessTerminate(HANDLE hProcess) {
    
    CEL_PROCESS_TERMINATE cl;
    
    cl.hProcess = hProcess;
    
    CELOGDATA(TRUE, CELID_PROCESS_TERMINATE, &cl, sizeof(CEL_PROCESS_TERMINATE), 0, CELZONE_PROCESS | CELZONE_MEMTRACKING);
}

//----------------------------------------------------------
_inline void CELOG_ProcessDelete(HANDLE hProcess) {
    
    CEL_PROCESS_DELETE cl;
    
    cl.hProcess = hProcess;
    
    CELOGDATA(TRUE, CELID_PROCESS_DELETE, &cl, sizeof(CEL_PROCESS_DELETE), 0, CELZONE_PROCESS);
}

//----------------------------------------------------------
_inline void CELOG_Sleep(DWORD dwTimeout) {
    CEL_SLEEP cl;

    cl.dwTimeout = dwTimeout;

    CELOGDATA(TRUE, CELID_SLEEP, &cl, sizeof(cl), 0, CELZONE_SYNCH);
}

//----------------------------------------------------------
_inline void CELOG_EventCreate(HANDLE hEvent, BOOL fManReset, BOOL fInitState, LPCWSTR lpEventName) {
    
    BYTE pTmp[MAX_PATH+sizeof(CEL_EVENT_CREATE)];
    PCEL_EVENT_CREATE pcl = (PCEL_EVENT_CREATE) pTmp;
    WORD wLen = 0;
    
    pcl->hEvent = hEvent;
    pcl->fManual = fManReset;
    pcl->fInitialState = fInitState;
     
    if (lpEventName) {
       wLen = strlenW(lpEventName) + 1;
       kstrcpyW(pcl->szName, lpEventName); 
    }

    CELOGDATA(TRUE, CELID_EVENT_CREATE, (PVOID) pcl, (WORD) (sizeof(CEL_EVENT_CREATE) + (wLen * sizeof(WCHAR))), 0, CELZONE_SYNCH);
}

//----------------------------------------------------------
// Event Set, Reset, Pulse
_inline void CELOG_Event(HANDLE hEvent, DWORD type) {
    
   switch (type) {
   
   case EVENT_SET: {
      
      CEL_EVENT_SET cl;
      
      cl.hEvent = hEvent;
  
      CELOGDATA(TRUE, CELID_EVENT_SET, &cl, sizeof(CEL_EVENT_SET), 0, CELZONE_SYNCH);
      break;
   }

   case EVENT_RESET: {
      
      CEL_EVENT_RESET cl;
      
      cl.hEvent = hEvent;
  
      CELOGDATA(TRUE, CELID_EVENT_RESET, &cl, sizeof(CEL_EVENT_RESET), 0, CELZONE_SYNCH);
      break;
   }
   
   case EVENT_PULSE: {
      
      CEL_EVENT_PULSE cl;
      
      cl.hEvent = hEvent;
  
      CELOGDATA(TRUE, CELID_EVENT_PULSE, &cl, sizeof(CEL_EVENT_PULSE), 0, CELZONE_SYNCH);
      break;
   }
   
   default:;
   }
}

//----------------------------------------------------------
_inline void CELOG_EventCloseHandle(HANDLE hEvent) {
    
    CEL_EVENT_CLOSE cl;
    
    cl.hEvent = hEvent;

    CELOGDATA(TRUE, CELID_EVENT_CLOSE, &cl, sizeof(CEL_EVENT_CLOSE), 0, CELZONE_SYNCH);
}

//----------------------------------------------------------
_inline void CELOG_EventDelete(HANDLE hEvent) {
    
    CEL_EVENT_DELETE cl;
    
    cl.hEvent = hEvent;

    CELOGDATA(TRUE, CELID_EVENT_DELETE, &cl, sizeof(CEL_EVENT_DELETE), 0, CELZONE_SYNCH);
}

//----------------------------------------------------------
_inline void CELOG_WaitForMultipleObjects(
    DWORD dwNumObjects,
    CONST HANDLE* lpHandles,
    BOOL fWaitAll,
    DWORD dwTimeout
    )
{
    BYTE pTmp[(MAXIMUM_WAIT_OBJECTS * sizeof(HANDLE)) + sizeof(CEL_WAIT_MULTI)];
    PCEL_WAIT_MULTI pcl = (PCEL_WAIT_MULTI) pTmp;
    
    // Impose a limit on the number of handles logged
    if (dwNumObjects > MAXIMUM_WAIT_OBJECTS) {
       dwNumObjects = MAXIMUM_WAIT_OBJECTS;
    }
    
    pcl->fWaitAll = fWaitAll;
    pcl->dwTimeout = dwTimeout;
    memcpy(pcl->hHandles, lpHandles, dwNumObjects * sizeof(HANDLE));

    CELOGDATA(TRUE, CELID_WAIT_MULTI, (LPVOID) pcl, (WORD) (sizeof(CEL_WAIT_MULTI) + (dwNumObjects * sizeof(HANDLE))), 0, CELZONE_SYNCH);
}

//----------------------------------------------------------
_inline void CELOG_MutexCreate(HANDLE hMutex, Name* pName) {
    
    BYTE pTmp[MAX_PATH+sizeof(CEL_MUTEX_CREATE)];
    PCEL_MUTEX_CREATE pcl = (PCEL_MUTEX_CREATE) pTmp;
    WORD wLen = 0;
    
    pcl->hMutex = hMutex;
    if (pName && pName->name) {
       wLen = strlenW(pName->name) + 1;
       kstrcpyW(pcl->szName, pName->name);
    }

    CELOGDATA(TRUE, CELID_MUTEX_CREATE, (PVOID) pcl, (WORD) (sizeof(CEL_MUTEX_CREATE) + (wLen * sizeof(WCHAR))), 0, CELZONE_SYNCH);
}

//----------------------------------------------------------
_inline void CELOG_MutexRelease(HANDLE hMutex) {
    
    CEL_MUTEX_RELEASE cl;
    
    cl.hMutex = hMutex;

    CELOGDATA(TRUE, CELID_MUTEX_RELEASE, &cl, sizeof(CEL_MUTEX_RELEASE), 0, CELZONE_SYNCH);
}

//----------------------------------------------------------
_inline void CELOG_MutexCloseHandle(HANDLE hMutex) {
    
    CEL_MUTEX_CLOSE cl;
    
    cl.hMutex = hMutex;

    CELOGDATA(TRUE, CELID_MUTEX_CLOSE, &cl, sizeof(CEL_MUTEX_CLOSE), 0, CELZONE_SYNCH);
}

//----------------------------------------------------------
_inline void CELOG_MutexDelete(HANDLE hMutex) {
    
    CEL_MUTEX_DELETE cl;
    
    cl.hMutex = hMutex;

    CELOGDATA(TRUE, CELID_MUTEX_DELETE, &cl, sizeof(CEL_MUTEX_DELETE), 0, CELZONE_SYNCH);
}

//----------------------------------------------------------
_inline void CELOG_SemaphoreCreate(HANDLE hSem, LONG lInitCount, LONG lMaxCount, LPCWSTR szName) {
    
    BYTE pTmp[MAX_PATH+sizeof(CEL_SEM_CREATE)];
    PCEL_SEM_CREATE pcl = (PCEL_SEM_CREATE) pTmp;
    WORD wLen = 0;
    
    pcl->hSem = hSem;
    pcl->dwInitCount = (DWORD) lInitCount;
    pcl->dwMaxCount = (DWORD) lMaxCount;
    
    if (szName) {
       wLen = strlenW(szName) + 1;
       kstrcpyW(pcl->szName, szName);
    }

    CELOGDATA(TRUE, CELID_SEM_CREATE, (PVOID) pcl, (WORD) (sizeof(CEL_SEM_CREATE) + (wLen * sizeof(WCHAR))), 0, CELZONE_SYNCH);
}

//----------------------------------------------------------
_inline void CELOG_SemaphoreRelease(HANDLE hSem, LONG lReleaseCount, LONG lPreviousCount) {
    
    CEL_SEM_RELEASE cl;
    
    cl.hSem = hSem;
    cl.dwReleaseCount = (DWORD) lReleaseCount;
    cl.dwPreviousCount = (DWORD) lPreviousCount;

    CELOGDATA(TRUE, CELID_SEM_RELEASE, &cl, sizeof(CEL_SEM_RELEASE), 0, CELZONE_SYNCH);
}

//----------------------------------------------------------
_inline void CELOG_SemaphoreCloseHandle(HANDLE hSem) {
    
    CEL_SEM_CLOSE cl;
    
    cl.hSem = hSem;

    CELOGDATA(TRUE, CELID_SEM_CLOSE, &cl, sizeof(CEL_SEM_CLOSE), 0, CELZONE_SYNCH);
}

//----------------------------------------------------------
_inline void CELOG_SemaphoreDelete(HANDLE hSem) {
    
    CEL_SEM_DELETE cl;
    
    cl.hSem = hSem;

    CELOGDATA(TRUE, CELID_SEM_DELETE, &cl, sizeof(CEL_SEM_DELETE), 0, CELZONE_SYNCH);
}

//----------------------------------------------------------
// Only logged if we block when entering
_inline void CELOG_CSEnter(HANDLE hCS, HANDLE hOwnerThread) {
    
    CEL_CRITSEC_ENTER cl;
    
    cl.hCS = hCS;
    cl.hOwnerThread = hOwnerThread;

    CELOGDATA(TRUE, CELID_CS_ENTER, &cl, sizeof(CEL_CRITSEC_ENTER), 0, CELZONE_CRITSECT);
}

//----------------------------------------------------------
// Only logged if another thread was blocked waiting for the CS
_inline void CELOG_CSLeave(HANDLE hCS, HANDLE hNewOwner) {
    
    CEL_CRITSEC_LEAVE cl;
    
    cl.hCS = hCS;
    cl.hOwnerThread = hNewOwner;

    CELOGDATA(TRUE, CELID_CS_LEAVE, &cl, sizeof(CEL_CRITSEC_LEAVE), 0, CELZONE_CRITSECT);
}

//----------------------------------------------------------
_inline void CELOG_VirtualAlloc(DWORD dwResult, DWORD dwAddress, DWORD dwSize, DWORD dwType, DWORD dwProtect) {
    
    CEL_VIRTUAL_ALLOC cl;

    cl.dwResult = dwResult;
    cl.dwAddress = dwAddress;
    cl.dwSize = dwSize;
    cl.dwType = dwType;
    cl.dwProtect = dwProtect;
    
    CELOGDATA(TRUE, CELID_VIRTUAL_ALLOC, &cl, sizeof(CEL_VIRTUAL_ALLOC), 0, CELZONE_VIRTMEM);
}

//----------------------------------------------------------
_inline void CELOG_VirtualCopy(DWORD dwDest, DWORD dwSource, DWORD dwSize, DWORD dwProtect) {
    
    CEL_VIRTUAL_COPY cl;

    cl.dwDest = dwDest;
    cl.dwSource = dwSource;
    cl.dwSize = dwSize;
    cl.dwProtect = dwProtect;
    
    CELOGDATA(TRUE, CELID_VIRTUAL_COPY, &cl, sizeof(CEL_VIRTUAL_COPY), 0, CELZONE_VIRTMEM);
}

//----------------------------------------------------------
_inline void CELOG_VirtualFree(DWORD dwAddress, DWORD dwSize, DWORD dwType) {
    
    CEL_VIRTUAL_FREE cl;

    cl.dwAddress = dwAddress;
    cl.dwSize = dwSize;
    cl.dwType = dwType;
    
    CELOGDATA(TRUE, CELID_VIRTUAL_FREE, &cl, sizeof(CEL_VIRTUAL_FREE), 0, CELZONE_VIRTMEM);
}

//----------------------------------------------------------
_inline void CELOG_SystemTLB(DWORD dwCount) {
    
    CEL_SYSTEM_TLB cl;
    
    cl.dwCount = dwCount;

    CELOGDATA(TRUE, CELID_SYSTEM_TLB, &cl, sizeof(CEL_SYSTEM_TLB), 0, CELZONE_TLB);
}

//----------------------------------------------------------
_inline void CELOG_SystemPage(DWORD dwAddress, BOOL fReadWrite) {
    
    CEL_SYSTEM_PAGE cl;
    
    cl.dwAddress = dwAddress;
    cl.fReadWrite = fReadWrite;

    CELOGDATA(TRUE, CELID_SYSTEM_PAGE, &cl, sizeof(CEL_SYSTEM_PAGE), 0, CELZONE_DEMANDPAGE);
}

//----------------------------------------------------------
_inline void CELOG_SystemInvert(HANDLE hThread, int nPriority) {
    
    CEL_SYSTEM_INVERT cl;
    
    cl.hThread = hThread;
    cl.nPriority = nPriority;

    CELOGDATA(TRUE, CELID_SYSTEM_INVERT, &cl, sizeof(CEL_SYSTEM_INVERT), 0, CELZONE_PRIORITYINV);
}

//----------------------------------------------------------
_inline void CELOG_ThreadSetPriority(HANDLE hThread, int nPriority) {
    
    CEL_THREAD_PRIORITY cl;
    
    cl.hThread = hThread;
    cl.nPriority = nPriority;

    CELOGDATA(TRUE, CELID_THREAD_PRIORITY, &cl, sizeof(CEL_THREAD_PRIORITY), 0, CELZONE_THREAD);
}

//----------------------------------------------------------
_inline void CELOG_ThreadSetQuantum(HANDLE hThread, DWORD dwQuantum) {
    
    CEL_THREAD_QUANTUM cl;
    
    cl.hThread = hThread;
    cl.dwQuantum = dwQuantum;

    CELOGDATA(TRUE, CELID_THREAD_QUANTUM, &cl, sizeof(CEL_THREAD_QUANTUM), 0, CELZONE_THREAD);
}

//----------------------------------------------------------
_inline void CELOG_ThreadSuspend(HANDLE hThread) {
    
    CEL_THREAD_SUSPEND cl;
    
    cl.hThread = hThread;

    CELOGDATA(TRUE, CELID_THREAD_SUSPEND, &cl, sizeof(CEL_THREAD_SUSPEND), 0, CELZONE_RESCHEDULE);
}

//----------------------------------------------------------
_inline void CELOG_ThreadResume(HANDLE hThread) {
    
    CEL_THREAD_RESUME cl;
    
    cl.hThread = hThread;

    CELOGDATA(TRUE, CELID_THREAD_RESUME, &cl, sizeof(CEL_THREAD_RESUME), 0, CELZONE_RESCHEDULE);
}

//----------------------------------------------------------
_inline void CELOG_ModuleLoad(HANDLE hProcess, HANDLE hModule, LPCWSTR szName, DWORD dwBaseOffset) {
    
    if (hModule) {
       BYTE pTmp[MAX_PATH+max(sizeof(CEL_MODULE_LOAD), sizeof(CEL_MEMTRACK_LOADMODULE))];
       PCEL_MODULE_LOAD pcl = (PCEL_MODULE_LOAD) pTmp;
       PCEL_MEMTRACK_LOADMODULE pcl2 = (PCEL_MEMTRACK_LOADMODULE) pTmp;
       WORD wLen = 0;
      
       pcl->hProcess = hProcess;
       pcl->hModule = hModule;
      
       if (szName) {
          wLen = strlenW(szName) + 1;
          kstrcpyW(pcl->szName, szName);
       }
   
       CELOGDATA(TRUE, CELID_MODULE_LOAD, (PVOID) pcl, (WORD) (sizeof(CEL_MODULE_LOAD) + (wLen * sizeof(WCHAR))), 0, CELZONE_LOADER);
       pcl2->hProcess = hProcess;
       pcl2->hModule  = hModule;
       pcl2->dwBase   = dwBaseOffset;
       
       if (wLen)
          kstrcpyW(pcl2->szName, szName);

       CELOGDATA(TRUE, CELID_MEMTRACK_LOADMODULE, (PVOID) pcl2, (WORD) (sizeof(CEL_MEMTRACK_LOADMODULE) + (wLen * sizeof(WCHAR))), 0, CELZONE_MEMTRACKING);
    }
}

//----------------------------------------------------------
_inline void CELOG_ModuleFree(HANDLE hProcess, HANDLE hModule, DWORD fReally) {
    
    CEL_MODULE_FREE cl;
    
    cl.hProcess = hProcess;
    cl.hModule = hModule;

    CELOGDATA(TRUE, CELID_MODULE_FREE, &cl, sizeof(CEL_MODULE_FREE), 0, CELZONE_LOADER);

    if (fReally) {
        CEL_MEMTRACK_FREEMODULE cl2;
    
    	cl2.hProcess = hProcess;
    	cl2.hModule  = hModule;

    	CELOGDATA(TRUE, CELID_MEMTRACK_FREEMODULE, &cl2, sizeof(CEL_MEMTRACK_FREEMODULE), 0, CELZONE_MEMTRACKING);
    }
}

//----------------------------------------------------------
#define CELOG_KCallEnter(nID) \
    CELOGDATA(TRUE, CELID_KCALL_ENTER, &nID, sizeof(int), 0, CELZONE_KCALL)

//----------------------------------------------------------
#define CELOG_KCallLeave(nID) \
    CELOGDATA(TRUE, CELID_KCALL_LEAVE, &nID, sizeof(int), 0, CELZONE_KCALL)


#else


//------------------------------------------------------------------------------
//  Normal operation (NK.LIB)
//
#define CELOGINIT()                                                    ((void)0)
#define CELOGCREATETHREAD()                                            ((void)0)
#define CELOGDATA(Time, ID, Data, Len, Zone1, Zone2)                   ((void)0)
#define CELOGDATAFLAGGED(Time, ID, Data, Len, Zone1, Zone2, Flag)      ((void)0)

// The kernel wrappers
#define CELOG_ThreadSwitch(hThread)                                    ((void)0)
#define CELOG_ThreadMigrate(hProcess, dwReserved)                      ((void)0)
#define CELOG_Sleep(dwTimeout)                                         ((void)0)
#define CELOG_ThreadCreate(pThread, hProc, lpProcName)                 ((void)0)
#define CELOG_ThreadCloseHandle(hThread)                               ((void)0)
#define CELOG_ThreadTerminate(hThread)                                 ((void)0)
#define CELOG_ThreadDelete(hThread)                                    ((void)0)
#define CELOG_ProcessCreate(hProc, lpProcName, dwBase)                 ((void)0)
#define CELOG_ProcessCloseHandle(hProc)                                ((void)0)
#define CELOG_ProcessTerminate(hProc)                                  ((void)0)
#define CELOG_ProcessDelete(hProc)                                     ((void)0)
#define CELOG_EventCreate(hEvent, fmanReset, fInitState, lpEventName)  ((void)0)
#define CELOG_Event(hEvent, type)                                      ((void)0)
#define CELOG_EventCloseHandle(hEvent)                                 ((void)0)
#define CELOG_EventDelete(hEvent)                                      ((void)0)
#define CELOG_WaitForMultipleObjects(dwNumObjects, lpHandles, fWaitAll, dwTimeout) ((void)0)
#define CELOG_MutexCreate(hMutex, pName)                               ((void)0)
#define CELOG_MutexRelease(hMutex)                                     ((void)0)
#define CELOG_MutexCloseHandle(hMutex)                                 ((void)0)
#define CELOG_MutexDelete(hMutex)                                      ((void)0)
#define CELOG_SemaphoreCreate(hSem, lInitCount, lMaxCount, szName)     ((void)0)
#define CELOG_SemaphoreRelease(hSem, lReleaseCount, lPreviousCount)    ((void)0)
#define CELOG_SemaphoreCloseHandle(hSem)                               ((void)0)
#define CELOG_SemaphoreDelete(hSem)                                    ((void)0)
#define CELOG_CSEnter(lpcs, hOwnerThread)                              ((void)0)
#define CELOG_CSLeave(lpcs, hNewOwner)                                 ((void)0)
#define CELOG_VirtualAlloc(dwResult, dwAddress, dwSize, dwType, dwProtect) ((void)0)
#define CELOG_VirtualCopy(dwDest, dwSource, dwSize, dwProtect)         ((void)0)
#define CELOG_VirtualFree(dwAddress, dwSize, dwType)                   ((void)0)
#define CELOG_SystemTLB(dwCount)                                       ((void)0)
#define CELOG_SystemPage(dwAddress, fReadWrite)                        ((void)0)
#define CELOG_SystemInvert(hThread, nPriority)                         ((void)0)
#define CELOG_ThreadSetPriority(hThread, nPriority)                    ((void)0)
#define CELOG_ThreadSetQuantum(hThread, dwQuantum)                     ((void)0)
#define CELOG_ModuleLoad(hProcess, hModule, szName, dwOffset)  	     ((void)0)
#define CELOG_ModuleFree(hProcess, hModule, fReally)                   ((void)0)
#define CELOG_KCallEnter(nID)                                          ((void)0)
#define CELOG_KCallLeave(nID)                                          ((void)0)
#define CELOG_ThreadSuspend(hThread)                                   ((void)0)
#define CELOG_ThreadResume(hThread)                                    ((void)0)

#endif      


#endif
