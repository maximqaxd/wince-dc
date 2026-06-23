//------------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//  Copyright (c) 1995-1999  Microsoft Corporation
//  
//------------------------------------------------------------------------------
//
//  Module Name:  
//  
//      celog.h
//  
//  Abstract:  
//
//      Interface to the CELog event logging infrastructure.
//      
//------------------------------------------------------------------------------
#ifndef __CELOG_H__
#define __CELOG_H__

#ifdef __cplusplus
extern "C" {
#endif
    

//------------------------------------------------------------------------------
// Common header for all event data
//------------------------------------------------------------------------------

typedef struct  __CEL_HEADER {
    DWORD   Length : 16;                // Length in bytes (excluding 32 bit header and optional 32 bit timestamp)
    DWORD   ID : 14;                    // CELID - See Below
    DWORD   Reserved : 1;               // Reserved - must be zero
    DWORD   fTimeStamp : 1;             // Is there a 32 bit TimeStamp following the header?
} CEL_HEADER, *PCEL_HEADER;

#define CEL_HEADER_TIMESTAMP   0x80000000
#define CEL_HEADER_LENGTH_MASK 0x0000FFFF
#define CEL_HEADER_ID_MASK     0x3FFF0000


//------------------------------------------------------------------------------
// Structures and IDs for Microsoft defined events/data
//------------------------------------------------------------------------------

//-------------------------------------------------------
// Critical Section - Only logged if we block
//
#define CELID_CS_ENTER                  1
typedef struct  __CEL_CRITSEC_ENTER {
    HANDLE  hCS;
    HANDLE  hOwnerThread;                  // If CS already held, who owns it?
} CEL_CRITSEC_ENTER, *PCEL_CRITSEC_ENTER;

#define CELID_CS_LEAVE                  2
typedef struct  __CEL_CRITSEC_LEAVE {
    HANDLE  hCS;
    HANDLE  hOwnerThread;                  // Who gets the CS next?
} CEL_CRITSEC_LEAVE, *PCEL_CRITSEC_LEAVE;


//-------------------------------------------------------
// Events
//
#define CELID_EVENT_CREATE              3
typedef struct  __CEL_EVENT_CREATE {
    HANDLE  hEvent;
    DWORD   fManual       : 1;          // Boolean - Manual Flag
    DWORD   fInitialState : 1;          // Boolean - Initial State
    DWORD   dwReserved    : 30;
    WCHAR   szName[0];                  // OPTIONAL Name (length inferred from entry length)
} CEL_EVENT_CREATE, *PCEL_EVENT_CREATE;

#define CELID_EVENT_SET                 4
typedef struct  __CEL_EVENT_SET {
    HANDLE  hEvent;
} CEL_EVENT_SET, *PCEL_EVENT_SET;

#define CELID_EVENT_RESET               5
typedef struct  __CEL_EVENT_RESET {
    HANDLE  hEvent;
} CEL_EVENT_RESET, *PCEL_EVENT_RESET;

#define CELID_EVENT_PULSE               6
typedef struct  __CEL_EVENT_PULSE {
    HANDLE  hEvent;
} CEL_EVENT_PULSE, *PCEL_EVENT_PULSE;

#define CELID_EVENT_CLOSE               7
typedef struct  __CEL_EVENT_CLOSE {
    HANDLE  hEvent;
} CEL_EVENT_CLOSE, *PCEL_EVENT_CLOSE;

#define CELID_EVENT_DELETE              8
typedef struct  __CEL_EVENT_DELETE {
    HANDLE  hEvent;
} CEL_EVENT_DELETE, *PCEL_EVENT_DELETE;

//-------------------------------------------------------
// WaitForSingleObject / WaitForMultipleObjects
//

// NOTE: currently calls to WaitForSingleObject are
// logged as calls to WaitForMultipleObjects because
// WaitForSingleObject calls WaitForMultipleObjects.

#define CELID_WAIT_MULTI                9
typedef struct  __CEL_WAIT_MULTI {
    DWORD   dwTimeout;
    DWORD   fWaitAll   : 1;
    DWORD   dwReserved : 31;
    HANDLE  hHandles[0];                // List of handles, count indicated by length
} CEL_WAIT_MULTI, *PCEL_WAIT_MULTI;     // Max # handles = MAXIMUM_WAIT_OBJECTS

//-------------------------------------------------------
// Sleep
//
#define CELID_SLEEP                     10
typedef struct  __CEL_SLEEP {
    DWORD   dwTimeout;
} CEL_SLEEP, *PCEL_SLEEP;



//-------------------------------------------------------
// Semaphores
//
#define CELID_SEM_CREATE                15
typedef struct  __CEL_SEM_CREATE {
    HANDLE  hSem;
    DWORD   dwInitCount;
    DWORD   dwMaxCount;
    WCHAR   szName[0];                  // OPTIONAL Name (length inferred from entry length)
} CEL_SEM_CREATE, *PCEL_SEM_CREATE;

#define CELID_SEM_RELEASE               16
typedef struct  __CEL_SEM_RELEASE {
    HANDLE  hSem;
    DWORD   dwReleaseCount;
    DWORD   dwPreviousCount;
} CEL_SEM_RELEASE, *PCEL_SEM_RELEASE;

#define CELID_SEM_CLOSE                 17
typedef struct  __CEL_SEM_CLOSE {
    HANDLE  hSem;
} CEL_SEM_CLOSE, *PCEL_SEM_CLOSE;

#define CELID_SEM_DELETE                18
typedef struct  __CEL_SEM_DELETE {
    HANDLE  hSem;
} CEL_SEM_DELETE, *PCEL_SEM_DELETE;

//-------------------------------------------------------
// Heap
//
#define CELID_HEAP_CREATE               25
typedef struct  __CEL_HEAP_CREATE {
    DWORD   dwOptions;
    DWORD   dwInitSize;
    DWORD   dwMaxSize;
    HANDLE  hHeap;
} CEL_HEAP_CREATE, *PCEL_HEAP_CREATE;

#define CELID_HEAP_ALLOC                26
typedef struct  __CEL_HEAP_ALLOC {
    HANDLE  hHeap;
    DWORD   dwFlags;
    DWORD   dwBytes;
    DWORD   lpMem;
    BYTE    bReserved[0];
} CEL_HEAP_ALLOC, *PCEL_HEAP_ALLOC;

#define CELID_HEAP_REALLOC              27
typedef struct  __CEL_HEAP_REALLOC {
    HANDLE  hHeap;
    DWORD   dwFlags;
    DWORD   dwBytes;
    DWORD   lpMemOld;
    DWORD   lpMem;
} CEL_HEAP_REALLOC, *PCEL_HEAP_REALLOC;

#define CELID_HEAP_FREE                 28
typedef struct  __CEL_HEAP_FREE {
    HANDLE  hHeap;
    DWORD   dwFlags;
    DWORD   lpMem;
    BYTE    bReserved[0];
} CEL_HEAP_FREE, *PCEL_HEAP_FREE;

#define CELID_HEAP_DESTROY              29
typedef struct  __CEL_HEAP_DESTROY {
    HANDLE  hHeap;
} CEL_HEAP_DESTROY, *PCEL_HEAP_DESTROY;


//-------------------------------------------------------
// Virtual Memory
//
#define CELID_VIRTUAL_ALLOC             35
typedef struct  __CEL_VIRTUAL_ALLOC {
    DWORD   dwResult;
    DWORD   dwAddress;
    DWORD   dwSize;
    DWORD   dwType;
    DWORD   dwProtect;
    BYTE    bReserved[0];
} CEL_VIRTUAL_ALLOC, *PCEL_VIRTUAL_ALLOC;

#define CELID_VIRTUAL_COPY              36
typedef struct  __CEL_VIRTUAL_COPY {
    DWORD   dwDest;
    DWORD   dwSource;
    DWORD   dwSize;
    DWORD   dwProtect;
} CEL_VIRTUAL_COPY, *PCEL_VIRTUAL_COPY;

#define CELID_VIRTUAL_FREE              37
typedef struct  __CEL_VIRTUAL_FREE {
    DWORD   dwAddress;
    DWORD   dwSize;
    DWORD   dwType;
    BYTE    bReserved[0];
} CEL_VIRTUAL_FREE, *PCEL_VIRTUAL_FREE;


//-------------------------------------------------------
// Scheduler
//
#define CELID_THREAD_SWITCH             45
typedef struct  __CEL_THREAD_SWITCH {
    HANDLE  hThread;
} CEL_THREAD_SWITCH, *PCEL_THREAD_SWITCH;

#define CELID_THREAD_MIGRATE            46
typedef struct  __CEL_THREAD_MIGRATE {
    HANDLE  hProcess;
    BOOL    bProcessEntry;
} CEL_THREAD_MIGRATE, *PCEL_THREAD_MIGRATE;

#define CELID_THREAD_CREATE             47
typedef struct  __CEL_THREAD_CREATE {
    HANDLE  hThread;
    HANDLE  hProcess;
    HANDLE  hModule;
    WCHAR   szName[0];                  // OPTIONAL Name (length inferred from entry length)
} CEL_THREAD_CREATE, *PCEL_THREAD_CREATE;

#define CELID_THREAD_CLOSE              48
typedef struct __CEL_THREAD_CLOSE {
    HANDLE  hThread;
} CEL_THREAD_CLOSE, *PCEL_THREAD_CLOSE;

#define CELID_THREAD_TERMINATE          49
typedef struct __CEL_THREAD_TERMINATE {
    HANDLE  hThread;
} CEL_THREAD_TERMINATE, *PCEL_THREAD_TERMINATE;

#define CELID_THREAD_DELETE             50
typedef struct __CEL_THREAD_DELETE {
    HANDLE  hThread;
} CEL_THREAD_DELETE, *PCEL_THREAD_DELETE;

#define CELID_PROCESS_CREATE            51
typedef struct  __CEL_PROCESS_CREATE {
    HANDLE  hProcess;
    WCHAR   szName[0];                  // OPTIONAL Name (length inferred from entry length)
} CEL_PROCESS_CREATE, *PCEL_PROCESS_CREATE;

#define CELID_PROCESS_CLOSE             52
typedef struct __CEL_PROCESS_CLOSE {
    HANDLE  hProcess;
} CEL_PROCESS_CLOSE, *PCEL_PROCESS_CLOSE;

#define CELID_PROCESS_TERMINATE         53
typedef struct __CEL_PROCESS_TERMINATE {
    HANDLE  hProcess;
} CEL_PROCESS_TERMINATE, *PCEL_PROCESS_TERMINATE;

#define CELID_PROCESS_DELETE            54
typedef struct __CEL_PROCESS_DELETE {
    HANDLE  hProcess;
} CEL_PROCESS_DELETE, *PCEL_PROCESS_DELETE;


//-------------------------------------------------------
// Mutexes
//
#define CELID_MUTEX_CREATE              60
typedef struct  __CEL_MUTEX_CREATE {      
    HANDLE  hMutex;
    WCHAR   szName[0];                  // OPTIONAL Name (length inferred from entry length)
} CEL_MUTEX_CREATE, *PCEL_MUTEX_CREATE;

#define CELID_MUTEX_RELEASE             61
typedef struct __CEL_MUTEX_RELEASE {
    HANDLE  hMutex;
} CEL_MUTEX_RELEASE, *PCEL_MUTEX_RELEASE;

#define CELID_MUTEX_CLOSE               62
typedef struct __CEL_MUTEX_CLOSE {
    HANDLE  hMutex;
} CEL_MUTEX_CLOSE, *PCEL_MUTEX_CLOSE;

#define CELID_MUTEX_DELETE              63
typedef struct __CEL_MUTEX_DELETE {
    HANDLE  hMutex;
} CEL_MUTEX_DELETE, *PCEL_MUTEX_DELETE;


//-------------------------------------------------------
// Data types for logging raw data

#define CELID_RAW_LONG                  70
#define CELID_RAW_ULONG                 71
#define CELID_RAW_SHORT                 72
#define CELID_RAW_USHORT                73
#define CELID_RAW_WCHAR                 74
#define CELID_RAW_CHAR                  75
#define CELID_RAW_UCHAR                 76
#define CELID_RAW_FLOAT                 77
#define CELID_RAW_DOUBLE                78

// To log raw data, call CELOGDATA directly.  For example:
// CELOGDATA(TRUE, CELID_RAW_LONG, &lMyData, (WORD) (myDataLen * sizeof(LONG)), 
//           1, CELZONE_MISC)


//-------------------------------------------------------
// Miscellaneous

#define CELID_SYSTEM_TLB                80
typedef struct __CEL_SYSTEM_TLB {
    DWORD   dwCount;
} CEL_SYSTEM_TLB, *PCEL_SYSTEM_TLB;

#define CELID_SYSTEM_PAGE               81
typedef struct __CEL_SYSTEM_PAGE {
    DWORD   dwAddress;
    DWORD   fReadWrite : 1;
    DWORD   dwReserved : 31;
} CEL_SYSTEM_PAGE, *PCEL_SYSTEM_PAGE;

#define CELID_SYSTEM_INVERT             82
typedef struct __CEL_SYSTEM_INVERT {
    HANDLE  hThread;
    int     nPriority;
} CEL_SYSTEM_INVERT, *PCEL_SYSTEM_INVERT;

#define CELID_THREAD_PRIORITY           83
typedef struct __CEL_THREAD_PRIORITY {
    HANDLE  hThread;
    int     nPriority;
} CEL_THREAD_PRIORITY, *PCEL_THREAD_PRIORITY;

#define CELID_THREAD_QUANTUM            84
typedef struct __CEL_THREAD_QUANTUM {
    HANDLE  hThread;
    DWORD   dwQuantum;
} CEL_THREAD_QUANTUM, *PCEL_THREAD_QUANTUM;

#define CELID_MODULE_LOAD               85
typedef struct __CEL_MODULE_LOAD {
    HANDLE  hProcess;
    HANDLE  hModule;
    WCHAR   szName[0];                  // OPTIONAL Name (length inferred from entry length)
} CEL_MODULE_LOAD, *PCEL_MODULE_LOAD;

#define CELID_MODULE_FREE               86
typedef struct __CEL_MODULE_FREE {
    HANDLE  hProcess;
    HANDLE  hModule;
} CEL_MODULE_FREE, *PCEL_MODULE_FREE;

#define CELID_INTERRUPTS                87
typedef struct __CEL_INT_DATA {
    DWORD   dwTimeStamp;
    WORD    wSysIntr;
    WORD    wNestingLevel;
} CEL_INT_DATA, *PCEL_INT_DATA;

typedef struct __CEL_INTERRUPTS {
    DWORD   dwDiscarded;                // Number of interrupts not logged.
    CEL_INT_DATA IntData[0];            // variable number of interrupts
} CEL_INTERRUPTS, *PCEL_INTERRUPTS;


#define CELID_LOG_MARKER                0x1FFF
typedef struct __CEL_LOG_MARKER {
   DWORD   dwFrequency;                 // TimeStamp counter frequency
   DWORD   dwDefaultQuantum;
} CEL_LOG_MARKER, *PCEL_LOG_MARKER;


#define CELID_USER                      0x2000      // Min value for User ID
#define CELID_MAX                       0x3FFF      // Max value for User ID

//-------------------------------------------------------
// Logging zones.
//
#define CELZONE_INTERRUPT    0x00000001
#define CELZONE_RESCHEDULE   0x00000002
#define CELZONE_IDLE         0x00000004
#define CELZONE_TLB          0x00000008
#define CELZONE_DEMANDPAGE   0x00000010
#define CELZONE_THREAD       0x00000020
#define CELZONE_PROCESS      0x00000040
#define CELZONE_PRIORITYINV  0x00000080
#define CELZONE_CRITSECT     0x00000100
#define CELZONE_SYNCH        0x00000200
#define CELZONE_WAITOBJ      0x00000400
#define CELZONE_HEAP         0x00000800
#define CELZONE_VIRTMEM      0x00001000
#define CELZONE_CACHE        0x00002000
#define CELZONE_LOADER       0x00004000
#define CELZONE_MEMTRACKING  0x00008000

#define CELZONE_WINDOW       0x00100000
#define CELZONE_MESSAGE      0x00200000

#define CELZONE_MISC         0x80000000



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
typedef struct  __CEL_BUFFER {
    DWORD   dwMaskProcess;              // Process mask (1 bit per process slot)
    DWORD   dwMaskUser;                 // User zone mask
    DWORD   dwMaskCE;                   // Predefined zone mask (CELZONE_xxx)
    PDWORD  pWrite;                     // Pointer to next entry to be written
    DWORD   dwBytesLeft;                // Bytes left available in the buffer
    DWORD   dwSize;                     // Total number of bytes in the buffer
    DWORD   pBuffer[0];                 // Start of the data.
} CEL_BUFFER, *PCEL_BUFFER;


//------------------------------------------------------------------------------
//
// Functions for performing data logging.  Note that typically you should
// use the macros defined below, rather than calling this function
// directly.  In a ship build, CeLogData normally thunks to a stub, so 
// calling the function directly just increasing your code size.
//

// If macros for these functions are defined, we don't want to collide with them.

#ifndef CeLogData
void CeLogData( BOOL  fTimeStamp, // Should timestamp be used?
                WORD  wID,        // ID of event/data.
                PVOID pData,      // pointer to data buffer
                WORD  wLen,       // Len of data (in bytes)
                DWORD dwZoneUser, // User-defined zone
                DWORD dwZoneCE);  // Predefined zone
#endif // CeLogData


#ifndef CeLogSetZones
void CeLogSetZones(DWORD dwZoneUser,        // User-defined zones
                   DWORD dwZoneCE,          // Predefined zones
                   DWORD dwZoneProcess);    // Process zones
#endif // CeLogSetZones



    // Macro for CELogData.
#ifdef SHIP_BUILD
    
    #define CELOGDATA(Time, ID, Data, Len, Zone1, Zone2)  ((void)0)

#else // SHIP_BUILD
    
    #define CELOGDATA(Time, ID, Data, Len, Zone1, Zone2) \
            CeLogData(Time, ID, Data, Len, Zone1, Zone2) 

#endif // IF/ELSE SHIP_BUILD


//------------------------------------------------------------------------------
//
// Variables to control the logging.  If they are not set during OEMInit, they
// will default to reasonable values.
//

extern DWORD dwCeLogLargeBuf;
extern DWORD dwCeLogSmallBuf;
extern DWORD dwCeLogFlushTimeout;
extern DWORD dwCeLogLargeBufFlush;
extern int   nCeLogThreadPrio;



#ifdef __cplusplus
}
#endif

#endif __CELOG_H__
