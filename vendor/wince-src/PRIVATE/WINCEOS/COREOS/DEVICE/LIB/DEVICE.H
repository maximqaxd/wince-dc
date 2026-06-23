// --------------------------------------------------------------------------
//
// Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.
//
// Module:
//
//     device.h
//
//
// --------------------------------------------------------------------------

#ifndef __DEVICE_H__
#define __DEVICE_H__ 1

#include <windows.h>
#include <memory.h>
#include <linklist.h>

typedef DWORD (* pInitFn)(DWORD);
typedef DWORD (* pInitExFn)(HANDLE,PCWSTR,LPVOID);
typedef BOOL (* pDeinitFn)(DWORD);
typedef BOOL (* pOpenFn)(DWORD,DWORD,DWORD);
typedef BOOL (* pCloseFn)(DWORD);
typedef DWORD (* pReadFn)(DWORD,LPVOID,DWORD);
typedef DWORD (* pWriteFn)(DWORD,LPCVOID,DWORD);
typedef DWORD (* pSeekFn)(DWORD,long,DWORD);
typedef BOOL (* pControlFn)(DWORD,DWORD,PBYTE,DWORD,PBYTE,DWORD,PDWORD);
typedef void (* pPowerupFn)(DWORD);
typedef void (* pPowerdnFn)(DWORD);

#define DEVFLAGS_FSD_NEEDTOWAIT       0x0001
#define DEVFLAGS_AUTO_DEREGISTER      0x0002

//
// Structure to track file system drivers
//
typedef struct _fsd_t {
    struct _fsd_t * next;
    int       cFSDDevices;
    HINSTANCE hFSDLib;
    pInitFn   pfnFSDInit;
    pDeinitFn pfnFSDDeinit;
    pInitExFn pfnFSDInitEx;
    pDeinitFn pfnFSDDeinitEx;
    HINSTANCE hFSDLibEx;
} fsd_t, * pfsd_t;

//
// Structure to track device drivers
//
typedef struct fsdev_t {
	LIST_ENTRY list;
	DWORD index;
	DWORD dwData;
	DWORD dwLoadOrder;
	pInitFn fnInit;
	pDeinitFn fnDeinit;
	pOpenFn fnOpen;
	pCloseFn fnClose;
	pReadFn fnRead;
	pWriteFn fnWrite;
	pSeekFn fnSeek;
	pControlFn fnControl;
	pPowerupFn fnPowerup;
	pPowerdnFn fnPowerdn;
	HINSTANCE hLib;
	BOOL PwrOn;
    WCHAR type[3];
    WORD wFlags;
    DWORD dwFSDData;
    DWORD dwRefCnt;
    pfsd_t  pfsd;
} fsdev_t;

//
// Note on reference counts:
// lpdwDevRefCnt points to the ref count of the associated fsdev_t. While fsdev_t.dwRefCnt is not 0, then
// the fsdev_t will not be freed.
// dwOpenRefCnt is the ref count for the fsopendev_t structure. While it is not 0, the fsopendev_t will
// not be freed.
// There were situations where the fsopendev_t structure was freed while a thread was blocked inside
// a DeviceIoControl call. dwOpenRefCnt will prevent this.
//
typedef struct fsopendev_t {
	struct fsopendev_t *nextptr;	// next one in linked list
	DWORD dwOpenData;
	fsdev_t *lpDev;
	DWORD *lpdwDevRefCnt;	// since we set lpDev to NULL on device deregister
    DWORD dwOpenRefCnt;     // ref count for this structure 
	HANDLE KHandle;			// kernel handle pointing to this structure
	HPROCESS hProc;			// process owning this handle
} fsopendev_t;

BOOL FS_DevReadFile(fsopendev_t *fsodev, LPVOID buffer, DWORD nBytesToRead, LPDWORD lpNumBytesRead,
	LPOVERLAPPED lpOverlapped);
BOOL FS_DevWriteFile(fsopendev_t *fsodev, LPCVOID buffer, DWORD nBytesToWrite, LPDWORD lpNumBytesWritten,
	LPOVERLAPPED lpOverlapped);
DWORD FS_DevGetFileSize(fsopendev_t *fsodev, LPDWORD lpFileSizeHigh);
DWORD FS_DevSetFilePointer(fsopendev_t *fsodev, LONG lDistanceToMove, PLONG lpDistanceToMoveHigh,
	DWORD dwMoveMethod);
BOOL FS_DevGetFileInformationByHandle(fsopendev_t *fsodev, LPBY_HANDLE_FILE_INFORMATION lpFileInfo);
BOOL FS_DevFlushFileBuffers(fsopendev_t *fsodev);
BOOL FS_DevGetFileTime(fsopendev_t *fsodev, LPFILETIME lpCreation, LPFILETIME lpLastAccess, LPFILETIME lpLastWrite);
BOOL FS_DevSetFileTime(fsopendev_t *fsodev, CONST FILETIME *lpCreation, CONST FILETIME *lpLastAccess, CONST FILETIME *lpLastWrite);
BOOL FS_DevSetEndOfFile(fsopendev_t *fsodev);
BOOL FS_DevDeviceIoControl(fsopendev_t *fsodev, DWORD dwIoControlCode, LPVOID lpInBuf, DWORD nInBufSize, LPVOID lpOutBuf, DWORD nOutBufSize, LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped);
void FS_DevProcNotify(DWORD flags, HPROCESS proc, HTHREAD thread);
HANDLE FS_RegisterDevice(LPCWSTR lpszType, DWORD dwIndex, LPCWSTR lpszLib, DWORD dwInfo);
BOOL FS_DeregisterDevice(HANDLE hDevice);
HANDLE FS_ActivateDevice(LPCWSTR lpszDevKey, DWORD dwClientInfo);
BOOL FS_DeactivateDevice(HANDLE hDevice);
void FS_CloseAllDeviceHandles(HPROCESS proc);
HANDLE FS_CreateDeviceHandle(LPCWSTR lpNew, DWORD dwAccess, DWORD dwShareMode, HPROCESS hProc);
BOOL FS_DevCloseFileHandle(fsopendev_t *fsodev);
BOOL FS_LoadFSD(HANDLE hDevice, LPCWSTR lpFSDName);
BOOL FS_LoadFSDEx(HANDLE hDevice, LPCWSTR lpFSDName, DWORD dwFlag);
BOOL FS_CeResyncFilesys(HANDLE hDevice);
BOOL FS_GetDeviceByIndex(DWORD dwIndex, LPWIN32_FIND_DATA lpFindFileData);
HANDLE RegisterDeviceEx( LPCWSTR lpszType, DWORD dwIndex, LPCWSTR lpszLib, DWORD dwInfo, DWORD dwLoadOrder);

#define ENTER_DEVICE_FUNCTION	{													\
	                                EnterCriticalSection(&g_devcs);   				\
	    							__try

#define EXIT_DEVICE_FUNCTION        __except (EXCEPTION_EXECUTE_HANDLER) {			\
           							    SetLastError(ERROR_INVALID_PARAMETER);  	\
	                                }												\
	    							LeaveCriticalSection(&g_devcs);    				\
	    						}

#endif
