/*++

Copyright (c) 1997-2000 Microsoft Corporation.  All rights reserved.

Module Name:

    tables.c

Abstract:

    This file contains all the FSDMGR tables.

--*/

#include "fsdmgrp.h"

#ifdef DEBUG
DBGPARAM dpCurSettings = {
        TEXTW("FSDMGR"), {
            TEXTW("Init"),              // 0x0001
            TEXTW("Errors"),            // 0x0002
            TEXTW("Power"),             // 0x0004
            TEXTW("Events"),            // 0x0008
            TEXTW("Disk I/O"),          // 0x0010
            TEXTW("APIs"),              // 0x0020
            TEXTW("Reserved"),          // 0x0040
            TEXTW("Reserved"),          // 0x0080
            TEXTW("Reserved"),          // 0x0100
            TEXTW("Reserved"),          // 0x0200
            TEXTW("Reserved"),          // 0x0400
            TEXTW("Reserved"),          // 0x0800
            TEXTW("Reserved"),          // 0x1000
            TEXTW("Reserved"),          // 0x2000
            TEXTW("Reserved"),          // 0x4000
            TEXTW("Reserved"),          // 0x8000
        },
        ZONEMASK_DEFAULT
};
#endif


CONST PCWSTR apwsAFSAPIs[NUM_AFS_APIS] = {
        TEXTW("CloseVolume"),
        NULL,
        TEXTW("CreateDirectoryW"),
        TEXTW("RemoveDirectoryW"),
        TEXTW("GetFileAttributesW"),
        TEXTW("SetFileAttributesW"),
        TEXTW("CreateFileW"),
        TEXTW("DeleteFileW"),
        TEXTW("MoveFileW"),
        TEXTW("FindFirstFileW"),
        NULL,                                           // originally TEXTW("CeRegisterFileSystemNotification")
        NULL,                                           // originally TEXTW("CeOidGetInfo")
        TEXTW("DeleteAndRenameFileW"),                  // originally TEXTW("PrestoChangoFileName")
        NULL,                                           // originally TEXTW("CloseAllFiles")
        TEXTW("GetDiskFreeSpaceW"),                     // originally TEXTW("GetDiskFreeSpace")
        TEXTW("Notify"),
        TEXTW("RegisterFileSystemFunction"),
};

CONST PFNAPI apfnAFSAPIs[NUM_AFS_APIS] = {
        (PFNAPI)FSDMGR_CloseVolume,
        (PFNAPI)NULL,
        (PFNAPI)FSDMGR_CreateDirectoryW,
        (PFNAPI)FSDMGR_RemoveDirectoryW,
        (PFNAPI)FSDMGR_GetFileAttributesW,
        (PFNAPI)FSDMGR_SetFileAttributesW,
        (PFNAPI)FSDMGR_CreateFileW,
        (PFNAPI)FSDMGR_DeleteFileW,
        (PFNAPI)FSDMGR_MoveFileW,
        (PFNAPI)FSDMGR_FindFirstFileW,
        (PFNAPI)NULL,
        (PFNAPI)NULL,
        (PFNAPI)FSDMGR_DeleteAndRenameFileW,
        (PFNAPI)FSDMGR_CloseAllFiles,
        (PFNAPI)FSDMGR_GetDiskFreeSpaceW,
        (PFNAPI)FSDMGR_Notify,
        (PFNAPI)FSDMGR_RegisterFileSystemFunction,
};

CONST PFNAPI apfnAFSStubs[NUM_AFS_APIS] = {
        (PFNAPI)FSDMGRStub_CloseVolume,
        (PFNAPI)NULL,
        (PFNAPI)FSDMGRStub_CreateDirectoryW,
        (PFNAPI)FSDMGRStub_RemoveDirectoryW,
        (PFNAPI)FSDMGRStub_GetFileAttributesW,
        (PFNAPI)FSDMGRStub_SetFileAttributesW,
        (PFNAPI)FSDMGRStub_CreateFileW,
        (PFNAPI)FSDMGRStub_DeleteFileW,
        (PFNAPI)FSDMGRStub_MoveFileW,
        (PFNAPI)FSDMGRStub_FindFirstFileW,
        (PFNAPI)NULL,
        (PFNAPI)NULL,
        (PFNAPI)FSDMGRStub_DeleteAndRenameFileW,
        (PFNAPI)NULL,
        (PFNAPI)FSDMGRStub_GetDiskFreeSpaceW,
        (PFNAPI)FSDMGRStub_Notify,
        (PFNAPI)FSDMGRStub_RegisterFileSystemFunction,
};

CONST DWORD asigAFSAPIs[NUM_AFS_APIS] = {
        FNSIG1(DW),                                     // CloseVolume
        FNSIG0(),                                       //
        FNSIG3(DW, PTR, PTR),                           // CreateDirectoryW
        FNSIG2(DW, PTR),                                // RemoveDirectoryW
        FNSIG2(DW, PTR),                                // GetFileAttributesW
        FNSIG3(DW, PTR, DW),                            // SetFileAttributesW
        FNSIG9(DW, DW,  PTR, DW, DW, PTR, DW, DW, DW),  // CreateFileW
        FNSIG2(DW, PTR),                                // DeleteFileW
        FNSIG3(DW, PTR, PTR),                           // MoveFileW
        FNSIG4(DW, DW,  PTR, PTR),                      // FindFirstFileW
        FNSIG2(DW, DW),                                 // CeRegisterFileSystemNotification
        FNSIG3(DW, DW,  PTR),                           // CeOidGetInfo
        FNSIG3(DW, PTR, PTR),                           // PrestoChangoFileName
        FNSIG2(DW, DW),                                 // CloseAllFiles
        FNSIG6(DW, PTR, PTR, PTR, PTR, PTR),            // GetDiskFreeSpace
        FNSIG2(DW, DW),                                 // Notify
        FNSIG2(DW, DW),                                 // CeRegisterFileSystemFunction
};

CONST PCWSTR apwsFileAPIs[NUM_FILE_APIS] = {
        TEXTW("CloseFile"),
        NULL,
        TEXTW("ReadFile"),
        TEXTW("WriteFile"),
        TEXTW("GetFileSize"),
        TEXTW("SetFilePointer"),
        TEXTW("GetFileInformationByHandle"),
        TEXTW("FlushFileBuffers"),
        TEXTW("GetFileTime"),
        TEXTW("SetFileTime"),
        TEXTW("SetEndOfFile"),
        TEXTW("DeviceIoControl"),
        TEXTW("ReadFileWithSeek"),
        TEXTW("WriteFileWithSeek"),
};

CONST PFNAPI apfnFileAPIs[NUM_FILE_APIS] = {
        (PFNAPI)FSDMGR_CloseFile,
        (PFNAPI)NULL,
        (PFNAPI)FSDMGR_ReadFile,
        (PFNAPI)FSDMGR_WriteFile,
        (PFNAPI)FSDMGR_GetFileSize,
        (PFNAPI)FSDMGR_SetFilePointer,
        (PFNAPI)FSDMGR_GetFileInformationByHandle,
        (PFNAPI)FSDMGR_FlushFileBuffers,
        (PFNAPI)FSDMGR_GetFileTime,
        (PFNAPI)FSDMGR_SetFileTime,
        (PFNAPI)FSDMGR_SetEndOfFile,
        (PFNAPI)FSDMGR_DeviceIoControl,
        (PFNAPI)FSDMGR_ReadFileWithSeek,
        (PFNAPI)FSDMGR_WriteFileWithSeek,
};

CONST PFNAPI apfnFileStubs[NUM_FILE_APIS] = {
        (PFNAPI)FSDMGRStub_CloseFile,
        (PFNAPI)NULL,
        (PFNAPI)FSDMGRStub_ReadFile,
        (PFNAPI)FSDMGRStub_WriteFile,
        (PFNAPI)FSDMGRStub_GetFileSize,
        (PFNAPI)FSDMGRStub_SetFilePointer,
        (PFNAPI)FSDMGRStub_GetFileInformationByHandle,
        (PFNAPI)FSDMGRStub_FlushFileBuffers,
        (PFNAPI)FSDMGRStub_GetFileTime,
        (PFNAPI)FSDMGRStub_SetFileTime,
        (PFNAPI)FSDMGRStub_SetEndOfFile,
        (PFNAPI)FSDMGRStub_DeviceIoControl,
        (PFNAPI)FSDMGRStub_ReadFileWithSeek,
        (PFNAPI)FSDMGRStub_WriteFileWithSeek,
};

CONST DWORD asigFileAPIs[NUM_FILE_APIS] = {
        FNSIG1(DW),                                     // CloseFile
        FNSIG0(),                                       //
        FNSIG5(DW, PTR, DW,  PTR, PTR),                 // ReadFile
        FNSIG5(DW, PTR, DW,  PTR, PTR),                 // WriteFile
        FNSIG2(DW, PTR),                                // GetFileSize
        FNSIG4(DW, DW,  PTR, DW),                       // SetFilePointer
        FNSIG2(DW, PTR),                                // GetFileInformationByHandle
        FNSIG1(DW),                                     // FlushFileBuffers
        FNSIG4(DW, PTR, PTR, PTR),                      // GetFileTime
        FNSIG4(DW, PTR, PTR, PTR),                      // SetFileTime
        FNSIG1(DW),                                     // SetEndOfFile
        FNSIG8(DW, DW,  PTR, DW,  PTR, DW, PTR, PTR),   // DeviceIoControl
        FNSIG7(DW, PTR, DW,  PTR, PTR, DW, DW),         // ReadFileWithSeek
        FNSIG7(DW, PTR, DW,  PTR, PTR, DW, DW),         // WriteFileWithSeek
};

CONST PCWSTR apwsFindAPIs[NUM_FIND_APIS] = {
        TEXTW("FindClose"),
        NULL,
        TEXTW("FindNextFileW"),
};

CONST PFNAPI apfnFindAPIs[NUM_FIND_APIS] = {
        (PFNAPI)FSDMGR_FindClose,
        (PFNAPI)NULL,
        (PFNAPI)FSDMGR_FindNextFileW,
};

CONST PFNAPI apfnFindStubs[NUM_FIND_APIS] = {
        (PFNAPI)FSDMGRStub_FindClose,
        (PFNAPI)NULL,
        (PFNAPI)FSDMGRStub_FindNextFileW,
};

CONST DWORD asigFindAPIs[NUM_FIND_APIS] = {
        FNSIG1(DW),                                     // FindClose
        FNSIG0(),                                       //
        FNSIG2(DW, PTR)                                 // FindNextFileW
};
