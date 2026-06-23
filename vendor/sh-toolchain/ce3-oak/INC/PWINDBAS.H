/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

Module Name:  

pwindbas.h

Abstract:

Private portion of windbase.h

Notes: 


--*/
   
#ifndef _PRIV_WINDBASE__
#define _PRIV_WINDBASE__

#ifdef __cplusplus
extern "C" {
#endif

// @CESYSGEN IF FILESYS_FSMAIN
  

/*
@func BOOL | CeGetReplChangeMask | Gets current replication change mask
@parm LPDWORD | lpmask | gets set to current replication mask
@comm Obtains the current replication change bit mask.  Currently only 2
		bits are supported.  Returns TRUE for success.
*/ 
BOOL CeGetReplChangeMask (LPDWORD lpmask);

/*
@func BOOL | CeSetReplChangeMask | Sets current replication change mask
@parm DWORD | mask | The new replication change mask
@comm Sets a new replication change bit mask.  Currently only 2 bits are supported.
		Returns TRUE for success.
*/ 
BOOL CeSetReplChangeMask (DWORD mask);

/*
@func BOOL | CeGetReplChangeBits | Gets the change bits for a particular oid
@parm CEOID | oid | oid to get change bits for
@parm LPDWORD | lpbits | Gets set to current bits for that oid
@parm DWORD | dwFlags | If set to REPL_CHANGE_WILLCLEAR, then the next call to
	CeClearReplChangeBits will actually clear these bits unless there has been
	a subsequent change to the item
@comm Obtains the current change bits for an oid.  If the REPL_CHANGE_WILLCLEAR flag
	is set, then the CeClearReplChangeBits api can be used next to actually clear
	some of the bits.  Currently only 2 bits are supported.
*/ 
BOOL CeGetReplChangeBitsEx (PCEGUID pguid, CEOID oid, LPDWORD lpbits, DWORD dwFlags);
#define CeGetReplChangeBits(oid,lpbits,dwFlags)	CeGetReplChangeBitsEx(NULL,oid,lpbits,dwFlags)

/*
@func BOOL | CeSetReplChangeBits | Sets selected change bits for an oid
@parm CEOID | oid | oid to get change bits for
@parm DWORD | mask | Bits to set
@comm This clears the bits which are set in the mask.  Currently only 2 bits are supported.
*/
BOOL CeSetReplChangeBitsEx (PCEGUID pguid, CEOID oid, DWORD mask);
#define CeSetReplChangeBits(oid,mask) CeSetReplChangeBitsEx(NULL,oid,mask)

/*
@func BOOL | CeClearReplChangeBits | Clears selected change bits for an oid
@parm CEOID | oid | oid to get change bits for
@parm DWORD | mask | Bits to clear
@comm If the oid hasn't changed since the last call to CeGetReplChangeBits with the
	REPL_CHANGE_WILLCLEAR flag, it clears the bits which are set in the mask.  Currently
	only 2 bits are supported.
*/ 
BOOL CeClearReplChangeBitsEx (PCEGUID pguid, CEOID oid, DWORD mask);
#define CeClearReplChangeBits(oid,mask) CeClearReplChangeBitsEx(NULL,oid,mask)

/*
@func BOOL | CeGetReplOtherBits | Retrives the replication private bits for an oid
@parm CEOID | oid | oid to get bits for
@parm LPDWORD | lpbits | Gets current bits
@comm Gets the current private bits for an oid.  Currently only 1 bit is supported.
*/ 
BOOL CeGetReplOtherBitsEx (PCEGUID pguid, CEOID oid, LPDWORD lpbits);
#define CeGetReplOtherBits(oid,lpbits) CeGetReplOtherBitsEx(NULL,oid,lpbits)

/*
@func BOOL | CeSetReplOtherBits | Sets the replication private bits for an oid
@parm CEOID | oid | oid to set bits for
@parm DWORD | bits | New bits
@comm Sets the current private bits for an oid.  Currently only 1 bit is supported.
*/ 
BOOL CeSetReplOtherBitsEx (PCEGUID pguid, CEOID oid, DWORD bits);
#define CeSetReplOtherBits(oid,bits) CeSetReplOtherBitsEx(NULL,oid,bits)

#define OID_REPL				2
#define MAKEREPLOID(bits28) (((OID_REPL)<<28)|((bits28)&0x0fffffff))
#define ISREPLOID(dwOid) ((dwOid>>28)==OID_REPL)
#define GETREPLOIDBITS(dwOid) ((dwOid)&0x0fffffff)

void DumpFileSystemHeap(void);

/*
@doc INTERNAL
@func BOOL | GetSystemMemoryDivision | Gets information on object store and system memory
@parm LPDWORD | lpdwStorePages | receives number of pages dedicated to the store
@parm LPDWORD | lpdwRamPages | receives number of pages dedicated to system memory
@parm LPDWORD | lpdwPageSize | receives the number of bytes in a page
@comm Gets information on object store and system memory
*/ 
BOOL GetSystemMemoryDivision (LPDWORD lpdwStorePages, LPDWORD lpdwRamPages, LPDWORD lpdwPageSize);

#define SYSMEM_CHANGED	0
#define SYSMEM_MUSTREBOOT 1
#define SYSMEM_REBOOTPENDING 2
#define SYSMEM_FAILED 3

/*
@doc INTERNAL
@func DWORD | SetSystemMemoryDivision | Sets number of pages for the object store
@parm DWORD | dwStorePages | Number of pages to allocate for the store
@comm Returs SYSMEM_CHANGED on success.  Returns SYSMEM_MUSTREBOOT if a reboot is neccessary to
	get the change to take effect.  Returns SYSMEM_REBOOTPENDING if a previous call returned
	SYSMEM_MUSTREBOOT, in which case the boundary cannot be adjusted further until the reboot
	happens.  Returns SYSMEM_FAILED if the dwStorePages is out of range (you cannot shrink
	system memory or the object store memory beyond a certain point, 256K each plus space for
	existing files and records.  If SYSMEM_FAILED is returned, you can get further information
	by calling GetLastError.
*/ 
DWORD SetSystemMemoryDivision (DWORD dwStorePages);

/*
  @func BOOL | CeRegisterFileSystemNotification | For internal use of the shell
  @parm HWND | hWnd | Window handle to which notifications will be posted
  @comm Posts a DB_* message to hWnd on any change in the file system.  Subsequent calls
  	override earlier calls.  Returns FALSE for failure.
  */
BOOL CeRegisterFileSystemNotification (HWND hWnd);
  
  /*
  @func BOOL | CeRegisterReplNotification | For internal use of replication
  @parm HWND | hWnd | Window handle for notifications
  @comm Posts a message of the form DB_CEOID_* when something changes in the file system or
   the system database.  Pass in the NULL hWnd to cancel notification.  Only one window
   may be registered at a time.  Subsequent calls override earlier calls.  Returns FALSE
   for failure.
  */ 
BOOL CeRegisterReplNotification (CENOTIFYREQUEST *pRequest);

// Functions
BOOL RegisterAFS (int index, HANDLE h, DWORD dw, DWORD version);
BOOL RegisterHiddenAFS (int index, HANDLE h, DWORD dw, DWORD version);
BOOL DeregisterAFS (int index);

int RegisterAFSName(LPCWSTR pName);
BOOL DeregisterAFSName(int index);

// @CESYSGEN ENDIF

// @CESYSGEN IF FILESYS_FSDBASE

/*
@func VOID | CeChangeDatabaseLCID | Changes the LCID for all database sorts
@parm DWORD | lcid | The lcid wanted
@comm Changes what the LCID used for all database sorts is
*/
VOID CeChangeDatabaseLCID (PCEGUID pguid, DWORD LCID);

BOOL DBCanonicalize(LCID Locale, LPCWSTR lpSrc, DWORD dwSrcLen, LPWSTR lpDst);

// @CESYSGEN ENDIF

#ifdef __cplusplus
}
#endif

    
#endif //_PRIV_WINDBASE_H_
    
