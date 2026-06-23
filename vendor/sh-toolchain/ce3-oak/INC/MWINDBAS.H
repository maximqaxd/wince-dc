/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

Module Name:  

mwindbas.h

Abstract:

private portion of windbase.h

Notes: 


--*/
   
#ifndef _MACRO_WINDBASE_H_
#define _MACRO_WINDBASE_H_

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

#ifndef WINCEMACRO
#error WINCEMACRO not defined __FILE__
#endif

#ifdef WIN32_FS_CALL
// SDK functions
#define CeOidGetInfoEx WIN32_FS_CALL(BOOL, 12, (PCEGUID pguid, CEOID oid, CEOIDINFO *oidInfo))
#define CeFindFirstDatabaseEx WIN32_FS_CALL(HANDLE, 13, (PCEGUID pguid, DWORD dwClassID))
#define CeCreateDatabaseEx WIN32_FS_CALL(CEOID, 14, (PCEGUID pguid, CEDBASEINFO *pInfo))
#define CeSetDatabaseInfoEx WIN32_FS_CALL(BOOL, 15, (PCEGUID pguid, CEOID oidDbase, CEDBASEINFO *pNewInfo))
#define CeOpenDatabaseEx WIN32_FS_CALL(HANDLE, 16, (PCEGUID pguid, PCEOID poid, LPWSTR lpszName, CEPROPID propid, DWORD dwFlags, CENOTIFYREQUEST *pRequest))
#define CeDeleteDatabaseEx WIN32_FS_CALL(BOOL, 28, (PCEGUID pguid, CEOID oid))
#define CeMountDBVol WIN32_FS_CALL(BOOL, 58, (PCEGUID pguid, LPWSTR lpszVol, DWORD dwFlags))
#define CeUnmountDBVol WIN32_FS_CALL(BOOL, 60, (PCEGUID pguid))
#define CeFlushDBVol WIN32_FS_CALL(BOOL, 61, (PCEGUID pguid))
#define CeEnumDBVolumes WIN32_FS_CALL(BOOL, 59, (PCEGUID pguid, LPWSTR lpBuf, DWORD dwSize))
#define GetStoreInformation WIN32_FS_CALL(BOOL, 32, (LPSTORE_INFORMATION lpsi))
#define CeFreeNotification WIN32_FS_CALL(BOOL, 62, (PCENOTIFYREQUEST pRequest, PCENOTIFICATION pNotify))
#define CeFindNextDatabaseEx WIN32_DBFIND_CALL(CEOID, 2, (HANDLE hEnum, PCEGUID pguid))
#define CeSeekDatabase WIN32_DBFILE_CALL(CEOID, 2, (HANDLE hDatabase, DWORD dwSeekType, DWORD dwValue, LPDWORD lpdwIndex))
#define CeDeleteRecord WIN32_DBFILE_CALL(BOOL, 3, (HANDLE hDatabase, CEOID oidRecord))
#define CeReadRecordPropsEx WIN32_DBFILE_CALL(CEOID, 4, (HANDLE hDbase, DWORD dwFlags, LPWORD lpcPropID, CEPROPID *rgPropID, LPBYTE *lplpBuffer, LPDWORD lpcbBuffer, HANDLE hHeap))
#define CeWriteRecordProps WIN32_DBFILE_CALL(CEOID, 5, (HANDLE hDbase, CEOID oidRecord, WORD cPropID, CEPROPVAL *rgPropVal))

// OAK functions  
#define CeGetReplChangeMask WIN32_FS_CALL(BOOL, 33, (LPDWORD lpmask))
#define CeSetReplChangeMask WIN32_FS_CALL(BOOL, 34, (DWORD mask))
#define CeGetReplChangeBitsEx WIN32_FS_CALL(BOOL, 35, (PCEGUID pguid, CEOID oid, LPDWORD lpbits, DWORD dwFlags))
#define CeSetReplChangeBitsEx WIN32_FS_CALL(BOOL, 51, (PCEGUID pguid, CEOID oid, DWORD mask))
#define CeClearReplChangeBitsEx WIN32_FS_CALL(BOOL, 36, (PCEGUID pguid, CEOID oid, DWORD mask))
#define CeGetReplOtherBitsEx WIN32_FS_CALL(BOOL, 37, (PCEGUID pguid, CEOID oid, LPDWORD lpbits))
#define CeSetReplOtherBitsEx WIN32_FS_CALL(BOOL, 38, (PCEGUID pguid, CEOID oid, DWORD bits))
#define CeChangeDatabaseLCID WIN32_FS_CALL(VOID, 56, (PCEGUID pguid, DWORD LCID))
#define CeRegisterFileSystemNotification WIN32_FS_CALL(BOOL, 10, (HWND hWnd))
#define CeRegisterReplNotification WIN32_FS_CALL(BOOL, 11, (CENOTIFYREQUEST *pRequest))
#define RegisterAFS WIN32_FS_CALL (BOOL, 45, (int index, HANDLE h, DWORD dw, DWORD version))
#define RegisterHiddenAFS WIN32_FS_CALL (BOOL, 55, (int index, HANDLE h, DWORD dw, DWORD version))
#define DeregisterAFS WIN32_FS_CALL (BOOL, 46, (int index))
#define GetSystemMemoryDivision WIN32_FS_CALL(BOOL, 39, (LPDWORD lpdwStorePages, LPDWORD lpdwRamPages, LPDWORD lpdwPageSize))
#define SetSystemMemoryDivision WIN32_FS_CALL(DWORD, 40, (DWORD dwStorePages))
#define RegisterAFSName WIN32_FS_CALL(int,52,(LPCWSTR pName))
#define DeregisterAFSName WIN32_FS_CALL(BOOL,53,(int index))
#define DumpFileSystemHeap WIN32_FS_CALL(void,57,(void))

#elif defined(COREDLL)

BOOL xxx_CeOidGetInfoEx (PCEGUID pguid, CEOID oid, CEOIDINFO *oidInfo);
#define CeOidGetInfoEx xxx_CeOidGetInfoEx
HANDLE xxx_CeFindFirstDatabaseEx (PCEGUID pguid, DWORD dwClassID);
#define CeFindFirstDatabaseEx xxx_CeFindFirstDatabaseEx
CEOID xxx_CeCreateDatabaseEx (PCEGUID pguid, CEDBASEINFO *pInfo);
#define CeCreateDatabaseEx xxx_CeCreateDatabaseEx
BOOL xxx_CeSetDatabaseInfoEx (PCEGUID pguid, CEOID oidDbase, CEDBASEINFO *pNewInfo);
#define CeSetDatabaseInfoEx xxx_CeSetDatabaseInfoEx
HANDLE xxx_CeOpenDatabaseEx (PCEGUID pguid, PCEOID poid, LPWSTR lpszName, CEPROPID propid, DWORD dwFlags, CENOTIFYREQUEST *pRequest);
#define CeOpenDatabaseEx xxx_CeOpenDatabaseEx
BOOL xxx_CeDeleteDatabaseEx (PCEGUID pguid, CEOID oid);
#define CeDeleteDatabaseEx xxx_CeDeleteDatabaseEx
BOOL xxx_CeMountDBVol (PCEGUID pguid, LPWSTR lpszVol, DWORD dwFlags);
#define CeMountDBVol xxx_CeMountDBVol
BOOL xxx_CeUnmountDBVol (PCEGUID pguid);
#define CeUnmountDBVol xxx_CeUnmountDBVol
BOOL xxx_CeFlushDBVol (PCEGUID pguid);
#define CeFlushDBVol xxx_CeFlushDBVol
BOOL xxx_CeEnumDBVolumes (PCEGUID pguid, LPWSTR lpBuf, DWORD dwSize);
#define CeEnumDBVolumes xxx_CeEnumDBVolumes
BOOL xxx_GetStoreInformation (LPSTORE_INFORMATION lpsi);
#define GetStoreInformation xxx_GetStoreInformation
BOOL xxx_CeFreeNotification (PCENOTIFYREQUEST pRequest, PCENOTIFICATION pNotify);
#define CeFreeNotification xxx_CeFreeNotification
CEOID xxx_CeFindNextDatabaseEx(HANDLE hEnum, PCEGUID pguid);
#define CeFindNextDatabaseEx xxx_CeFindNextDatabaseEx
CEOID xxx_CeSeekDatabase(HANDLE hDatabase, DWORD dwSeekType, DWORD dwValue, LPDWORD lpdwIndex);
#define CeSeekDatabase xxx_CeSeekDatabase
BOOL xxx_CeDeleteRecord(HANDLE hDatabase, CEOID oidRecord);
#define CeDeleteRecord xxx_CeDeleteRecord
CEOID xxx_CeReadRecordPropsEx(HANDLE hDbase, DWORD dwFlags, LPWORD lpcPropID, CEPROPID *rgPropID, LPBYTE *lplpBuffer, LPDWORD lpcbBuffer, HANDLE hHeap);
#define CeReadRecordPropsEx xxx_CeReadRecordPropsEx
CEOID xxx_CeWriteRecordProps(HANDLE hDbase, CEOID oidRecord, WORD cPropID, CEPROPVAL *rgPropVal);
#define CeWriteRecordProps xxx_CeWriteRecordProps
BOOL xxx_CeGetReplChangeMask (LPDWORD lpmask);
#define CeGetReplChangeMask xxx_CeGetReplChangeMask
BOOL xxx_CeSetReplChangeMask (DWORD mask);
#define CeSetReplChangeMask xxx_CeSetReplChangeMask
BOOL xxx_CeGetReplChangeBitsEx (PCEGUID pguid, CEOID oid, LPDWORD lpbits, DWORD dwFlags);
#define CeGetReplChangeBitsEx xxx_CeGetReplChangeBitsEx
BOOL xxx_CeSetReplChangeBitsEx (PCEGUID pguid, CEOID oid, DWORD mask);
#define CeSetReplChangeBitsEx xxx_CeSetReplChangeBitsEx
BOOL xxx_CeClearReplChangeBitsEx (PCEGUID pguid, CEOID oid, DWORD mask);
#define CeClearReplChangeBitsEx xxx_CeClearReplChangeBitsEx
BOOL xxx_CeGetReplOtherBitsEx (PCEGUID pguid, CEOID oid, LPDWORD lpbits);
#define CeGetReplOtherBitsEx xxx_CeGetReplOtherBitsEx
BOOL xxx_CeSetReplOtherBitsEx (PCEGUID pguid, CEOID oid, DWORD bits);
#define CeSetReplOtherBitsEx xxx_CeSetReplOtherBitsEx
VOID xxx_CeChangeDatabaseLCID (PCEGUID pguid, DWORD LCID);
#define CeChangeDatabaseLCID xxx_CeChangeDatabaseLCID
BOOL xxx_CeRegisterFileSystemNotification (HWND hWnd);
#define CeRegisterFileSystemNotification xxx_CeRegisterFileSystemNotification
BOOL xxx_CeRegisterReplNotification (CENOTIFYREQUEST *pRequest);
#define CeRegisterReplNotification xxx_CeRegisterReplNotification
BOOL xxx_RegisterAFS (int index, HANDLE h, DWORD dw, DWORD version);
#define RegisterAFS xxx_RegisterAFS
BOOL xxx_RegisterHiddenAFS (int index, HANDLE h, DWORD dw, DWORD version);
#define RegisterHiddenAFS xxx_RegisterHiddenAFS
BOOL xxx_DeregisterAFS (int index);
#define DeregisterAFS xxx_DeregisterAFS
BOOL xxx_GetSystemMemoryDivision (LPDWORD lpdwStorePages, LPDWORD lpdwRamPages, LPDWORD lpdwPageSize);
#define GetSystemMemoryDivision xxx_GetSystemMemoryDivision
DWORD xxx_SetSystemMemoryDivision (DWORD dwStorePages);
#define SetSystemMemoryDivision xxx_SetSystemMemoryDivision
int xxx_RegisterAFSName (LPCWSTR pName);
#define RegisterAFSName xxx_RegisterAFSName
BOOL xxx_DeregisterAFSName (int index);
#define DeregisterAFSName xxx_DeregisterAFSName
void xxx_DumpFileSystemHeap (void);
#define DumpFileSystemHeap xxx_DumpFileSystemHeap
		 
#endif

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif //_MACRO_WINDBASE_H_
    
