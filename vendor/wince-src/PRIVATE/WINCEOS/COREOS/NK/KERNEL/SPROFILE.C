/* Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved. */

/*+
	sprofile.c - system call profiling information.
*/

#ifdef SYSCALL_PROFILING
	#include "kernel.h"

	#define UNKNOWNMETHOD -1
	char *MethodToName(long iMethod, char ClassName[4]);
	int strncmp(const char *s1, const char *s2, unsigned int l);
	// tables to map iMethod function name
	// class name "USER" from \coreos\gwe\gwe\server\apiset.cpp
	char *USERNames[] = {	
    "GweNotifyCallback",                     /* 0  */
    "(reserved for NK)", /* RESERVED for NK */                        /* 1  */
    "RegisterClassWStub",                    /* 2  */
    "UnregisterClassW",                      /* 3  */
    "CWindowManager::CreateWindowExW",       /* 4  */
    "PostMessageW",                          /* 5  */
    "PostQuitMessage",                       /* 6  */
    "SendMessageW",                          /* 7  */
    "GetMessageW",                           /* 8  */
    "TranslateMessage",                      /* 9  */
    "DispatchMessageW",                      /* 10 */
    "GetCapture",                            /* 11 */
    "SetCapture",                            /* 12 */
    "ReleaseCapture",                        /* 13 */
    "CWindow::SetWindowPos",                  /* 14 */
    "CWindow::GetWindowRect",                 /* 15 */
    "CWindow::GetClientRect",                 /* 16 */
    "CWindow::InvalidateRect",                /* 17 */
    "CWindow::GetWindow",                     /* 18 */
    "GetSystemMetrics",                      /* 19 */
    "ImageList_GetDragImage",                /* 20 */
    "ImageList_GetIconSize",                 /* 21 */
    "ImageList_SetIconSize",                 /* 22 */
    "ImageList_GetImageInfo",                /* 23 */
    "ImageList_Merge",                       /* 24 */
    "CreateColorBitmap",                     /* 25 */
    "CreateMonoBitmap",                      /* 26 */
    "ImageList_CopyDitherImage",             /* 27 */
    "ImageList_DrawIndirect",                /* 28 */
    "ImageList_DragShowNolock",              /* 29 */
    "CWindowManager::WindowFromPoint",       /* 30 */
    "CWindow::ChildWindowFromPoint",           /* 31 */
    "CWindow::ClientToScreen",                 /* 32 */
    "CWindow::ScreenToClient",                 /* 33 */
    "CWindow::SetWindowText",                 /* 34 */
    "CWindow::GetWindowText",                 /* 35 */
    "CWindow::SetWindowLong",                 /* 36 */
    "CWindow::GetWindowLong",                 /* 37 */
    "CWindow::BeginPaint",                    /* 38 */
    "CWindow::EndPaint",                      /* 39 */
    "CWindow::GetDC",                         /* 40 */
    "CWindow::ReleaseDC",                     /* 41 */
    "CWindow::DefWindowProc",                 /* 42 */
    "CWindow::GetClassLong",                  /* 43 */
    "CWindow::SetClassLong",                  /* 44 */
    "CWindow::DestroyWindow",                 /* 45 */
    "CWindow::ShowWindow",                    /* 46 */
    "CWindow::UpdateWindow",                  /* 47 */
    "CWindow::SetParent",                     /* 48 */
    "CWindow::GetParent",                      /* 49 */
    "MessageBoxW",                           /* 50 */
    "SetFocus",                              /* 51 */
    "GetFocus",                              /* 52 */
    "GetActiveWindow",                       /* 53 */
    "CWindow::GetWindowDC",                   /* 54 */
    "GetSysColor",                           /* 55 */
    "AdjustWindowRectEx",                    /* 56 */
    "CWindow::IsWindow",                       /* 57 */
    "CreatePopupMenu",                       /* 58 */
    "InsertMenuW",                           /* 59 */
    "AppendMenuW",                           /* 60 */
    "RemoveMenu",                            /* 61 */
    "DestroyMenu",                           /* 62 */
    "TrackPopupMenuEx",                      /* 63 */
    "LoadMenuW",                             /* 64 */
    "EnableMenuItem",                        /* 65 */
    "CWindow::MoveWindow",                    /* 66 */
    "CWindow::GetUpdateRgn",                  /* 67 */
    "CWindow::GetUpdateRect",                 /* 68 */
    "CWindow::BringWindowToTop",              /* 69 */
    "CWindow::GetWindowTextLengthW",          /* 70 */
    "CWindow::IsChild",                       /* 71 */
    "CWindow::IsWindowVisible",               /* 72 */
    "CWindow::ValidateRect",                   /* 73 */
    "LoadBitmapW",                           /* 74 */
    "CheckMenuItem",                         /* 75 */
    "CheckMenuRadioItem",                    /* 76 */
    "DeleteMenu",                            /* 77 */
    "LoadIconW",                             /* 78 */
    "DrawIconEx",                            /* 79 */
    "DestroyIcon",                           /* 80 */
    "GetAsyncKeyState",                      /* 81 */
    "LoadStringW",                           /* 82 */
    "DialogBoxIndirectParamW",               /* 83 */
    "EndDialog",                             /* 84 */
    "GetDlgItem",                            /* 85 */
    "GetDlgCtrlID",                          /* 86 */
    "GetKeyState",                           /* 87 */
    "KeybdInquire",                          /* 88 */
    "KeybdInitStates",                       /* 89 */
    "PostKeybdMessage",                      /* 90 */
    "KeybdVKeyToUnicode",                    /* 91 */
    "keybd_event",                           /* 92 */
    "mouse_event",                           /* 93 */
    "CWindow::SetScrollInfo",                 /* 94 */
    "CWindow::SetScrollPos",                  /* 95 */
    "CWindow::SetScrollRange",                /* 96 */
    "CWindow::GetScrollInfo",                 /* 97 */
    "PeekMessageW",                          /* 98 */
    "MapVirtualKeyW",                        /* 99 */
    "GetMessageWNoWait",                     /* 100 */
    "GetClassNameW",                          /* 101 */
    "CWindowManager::MapWindowPoints",       /* 102 */
    "LoadImageW",                            /* 103 */
    "GetForegroundWindow",                   /* 104 */
    "SetForegroundWindow",                   /* 105 */
    "CWindowManager::RegisterTaskBar",       /* 106 */
    "SetActiveWindow",                       /* 107 */
    "CWindowManager::CallWindowProcW",       /* 108 */
    "GetClassInfoW",                         /* 109 */
    "GetNextDlgTabItem",                     /* 110 */
    "CreateDialogIndirectParamW",            /* 111 */
    "IsDialogMessage",                       /* 112 */
    "SetDlgItemInt",                         /* 113 */
    "GetDlgItemInt",                         /* 114 */
    "CWindowManager::FindWindowW",           /* 115 */
    "CreateCaret",                           /* 116 */
    "DestroyCaret",                          /* 117 */
    "HideCaret",                             /* 118 */
    "ShowCaret",                             /* 119 */
    "SetCaretPos",                           /* 120 */
    "GetCaretPos",                           /* 121 */
    "TouchGetCalibrationPointCount",         /* 122 */
    "TouchGetCalibrationPoint",              /* 123 */
    "TouchReadCalibrationPoint",             /* 124 */
    "TouchAcceptCalibration",                /* 125 */
    "ExtractIconExW",                        /* 126 */
    "SetTimer",                              /* 127 */
    "KillTimer",                             /* 128 */
    "GetNextDlgGroupItem",                   /* 129 */
	  "CheckRadioButton",                      /* 130 */
	  "CWindow::EnableWindow",  				/* 131 */
		"CWindow::IsWindowEnabled",			    /* 132 */
    "CreateMenu",                            /* 133 */
    "GetSubMenu",                            /* 134 */
    "LoadAcceleratorsW",						/* 135 */
    "CreateAcceleratorTableW",				/* 136 */
    "DestroyAcceleratorTable",				/* 137 */
    "TranslateAcceleratorW",					/* 138 */
    "sndPlaySoundW",							/* 139 */
    "waveOutGetVolume",						/* 140 */
    "waveOutSetVolume",                      /* 141 */
    "ImageList_Create",                      /* 142 */
    "ImageList_Destroy",                     /* 143 */
    "ImageList_GetImageCount",               /* 144 */
    "ImageList_Add",                         /* 145 */
    "ImageList_ReplaceIcon",                 /* 146 */
    "ImageList_SetBkColor",                  /* 147 */
    "ImageList_GetBkColor",                  /* 148 */
    "ImageList_SetOverlayImage",             /* 149 */
    "ImageList_Draw",                        /* 150 */
    "ImageList_Replace",                     /* 151 */
    "ImageList_AddMasked",                   /* 152 */
    "ImageList_DrawEx",                      /* 153 */
    "ImageList_Remove",                      /* 154 */
    "ImageList_GetIcon",                     /* 155 */
    "ImageList_LoadImage",                   /* 156 */
    "ImageList_BeginDrag",                   /* 157 */
    "ImageList_EndDrag",                     /* 158 */
    "ImageList_DragEnter",                   /* 159 */
    "ImageList_DragLeave",                   /* 160 */
    "ImageList_DragMove",                    /* 161 */
    "ImageList_SetDragCursorImage",          /* 162 */
    "AudioUpdateFromRegistry"               /* 163 */
		};
  // from gwe\gdi\server\gdiinit.cpp, classname "GDI"
  char *GDINames[] = {
    "GDINotifyCallback",
    "Fn_UnUsed", // reserved by kernel
    "AddFontResourceW",
    "BitBlt",
    "CombineRgn",
    "CreateCompatibleDC",
    "CreateDIBPatternBrushPt",
    "CreateDIBSectionStub",
    "CreateFontIndirectW",
    "CreateRectRgnIndirect",
    "CreatePenIndirect",
    "CreateSolidBrush",
    "DeleteDC",
    "DeleteObject",
    "DrawEdge",
    "DrawFocusRect",
    "DrawTextW",
    "Ellipse",
    "EnumFontFamiliesW",
    "EnumFontsW",
    "ExcludeClipRect",
    "ExtTextOutW",
    "FillRect",
    "Fn_UnUsed",  //Was FrameRgn
    "GetBkColor",
    "GetBkMode",
    "GetClipRgn",
    "GetCurrentObject",
    "GetDeviceCaps",
    "GetNearestColor",
    "GetObjectW",
    "GetObjectType",
    "GetPixel",
    "GetRegionData",
    "GetRgnBox",
    "GetStockObject",
    "PatBlt",
    "GetTextColor",
    "GetTextExtentExPointW",
    "GetTextFaceW",
    "GetTextMetricsW",
    "MaskBlt",
    "OffsetRgn",
    "Polygon",
    "Polyline",
    "PtInRegion",
    "Rectangle",
    "RectInRegion",
    "RemoveFontResourceW",
    "RestoreDC",
    "RoundRect",
    "SaveDC",
    "SelectClipRgn",
    "SelectObject",
    "SetBkColor",
    "SetBkMode",
    "SetBrushOrgEx",
    "SetPixel",
    "SetTextColor",
    "StretchBlt",
    "CreateBitmap",
    "CreateCompatibleBitmap",
    "GetSysColorBrush",
    "IntersectClipRect",
    "GetClipBox",
    "CeRemoveFontResource",
    "EnableEUDC",
    "CloseEnhMetaFile",
    "CreateEnhMetaFileW",
    "DeleteEnhMetaFile",
    "PlayEnhMetaFile",
    "CreatePalette",
    "SelectPalette",
    "RealizePalette",
    "GetPaletteEntries",
    "SetPaletteEntries",
    "GetSystemPaletteEntries",
    "GetNearestPaletteIndex",
    "CreatePen",
    "StartDocW",
    "EndDoc",
    "StartPage",
    "EndPage",
    "AbortDoc",
    "SetAbortProc",
    "CreateDCW",
    "CreateRectRgn", 
    "FillRgn",
    "SetROP2",
    "SetRectRgn",
    "RectVisible",
    "CreatePatternBrush",
    "CreateBitmapFromPointer",
    "SetViewportOrgEx",
    "TransparentImage",
    "SetObjectOwner",
	"InvertRect"
  };
  // from coreos\init.cpp, class name "INT"
  char *INTNames[] = { 
		"InitNotifyCallback",
    "0",
    "SC_Init_Initialized"
	};
  // from coreos\nktest\psltest\server\psl.c, class name "TEST"
  char *TESTNames[] = { 
    "0",
    "0",
    "TestAPI"
	};
  // from coreos\filesys\filesys.c, class name "W32A"
  char *W32ANames[] = { 
		"FS_ProcNotify",
		"0",
		"FS_CreateDirectoryW",
		"FS_RemoveDirectoryW",
		"FS_MoveFileW",
		"FS_CopyFileW",
		"FS_DeleteFileW",
		"FS_GetFileAttributes",
		"FS_FindFirstFileW",
		"FS_CreateFileW",
		"FS_CeRegisterFileSystemNotification",
		"FS_CeRegisterReplNotification",
		"FS_CeOidGetInfo",
		"FS_CeFindFirstDatabase",
		"FS_CeCreateDatabase",
		"FS_CeSetDatabaseInfo",
		"FS_CeOpenDatabase",
    "prgRegCloseKey",
    "prgRegCreateKeyExW",
    "prgRegDeleteKeyW",
    "prgRegDeleteValueW",
    "prgRegEnumValueW",
    "prgRegEnumKeyExW",
    "prgRegOpenKeyExW",
    "prgRegQueryInfoKeyW",
    "prgRegQueryValueExW",
    "prgRegSetValueExW",
		"FS_CreateContainerW",
		"FS_CeDeleteDatabase",
		"FS_RegisterDevice",
		"FS_DeregisterDevice",
		"FS_SetFileAttributesW",
		"FS_GetStoreInformation",
		"FS_CeGetReplChangeMask",
		"FS_CeSetReplChangeMask",
		"FS_CeGetReplChangeBits",
		"FS_CeClearReplChangeBits",
		"FS_CeGetReplOtherBits",
		"FS_CeSetReplOtherBits",
		"FS_GetSystemMemoryDivision",
		"FS_SetSystemMemoryDivision"
	};
  // from coreos\filesys\filesys.c, class name "W32H"
  char *W32HNames[] = { 
    "FS_CloseFileHandle",
    "0",
    "FS_ReadFile",
    "FS_WriteFile",
    "FS_GetFileSize",
    "FS_SetFilePointer",
    "FS_GetFileInformationByHandle",
		"FS_FlushFileBuffers",
		"FS_GetFileTime",
		"FS_SetFileTime",
		"FS_SetEndOfFile",
		"FS_DeviceIoControl"
	};
// from coreos\filesys\filesys.c, class name "W32D"
  char *W32DNames[] = { 
    "FS_DevCloseFileHandle",
    "0",
    "FS_DevReadFile",
    "FS_DevWriteFile",
    "FS_DevGetFileSize",
    "FS_DevSetFilePointer",
    "FS_DevGetFileInformationByHandle",
		"FS_DevFlushFileBuffers",
		"FS_DevGetFileTime",
		"FS_DevSetFileTime",
		"FS_DevSetEndOfFile",
		"FS_DevDeviceIoControl"
	};
  // from coreos\filesys\filesys.c, class name "FFHN"
  char *FFHNNames[] = { 
		"FS_FindClose",
    "0",
		"FS_FindNextFile"
	};
	// from coreos\filesys\filesys.c, class name "DBFA"	
  char *DBFANames[] = { 
		"FS_CloseDBFindHandle",
		"0",
		"FS_CeFindNextDatabase"
  };
  // from coreos\filesys\filesys.c, class name "DBOA"
  char *DBOANames[] = { 
		"FS_CloseDBHandle",
		"0",
		"FS_CeSeekDatabase",
		"FS_CeDeleteRecord",
		"FS_CeReadRecordProps",
		"FS_CeWriteRecordProps"
  };
  // from coreos\nk\kernel\kwin32.c, class name Wn32
  char *Wn32Names[] = { 
    "SC_Nop",
    "SC_NotSupported",
    "SC_CreateAPISet",
    "SC_VirtualAlloc",
    "SC_VirtualFree",
    "SC_VirtualProtect",
    "SC_VirtualQuery",
    "SC_VirtualCopy",
	"SC_LoadLibraryW",
	"SC_FreeLibrary",
	"SC_GetProcAddressW",
	"SC_ThreadAttachAllDLLs",
	"SC_ThreadDetachAllDLLs",
	"SC_GetTickCount",
	"OutputDebugStringW",
	"SC_NotSupported",
	"SC_TlsCall",
	"SC_GetSystemInfo",
	"0",
	"ropen",
	"rread",
	"rwrite",
	"rlseek",
	"rclose",
	"0",
	"0",
	"SC_RegisterDbgZones",
	"SC_NotSupported",
	"NKDbgPrintfW",
	"SC_ProfileSyscall",
	"SC_FindResource",
	"SC_LoadResource",
	"SC_NotSupported",
	"SC_SizeofResource",
	"OEMGetRealTime",
	"OEMSetRealTime",
	"SC_ProcessDetachAllDLLs",
	"SC_ExtractResource",
	"SC_GetRomFileInfo",
	"SC_GetRomFileBytes",
	"SC_CacheSync",
	"SC_AddTrackedItem",
	"SC_DeleteTrackedItem",
	"SC_PrintTrackedItem",
	"SC_GetKPhys",
	"SC_GiveKPhys"
  };
  // from coreos\nk\kernel\kwin32.c, class name KW32
  char *KW32Names[] = { 
    "SysCall_NOT_SUPPORTED",
    "SysCall_NOT_SUPPORTED",
    "SC_GetOwnerProcess",
    "SC_GetCallerProcess",
    "SC_PerformCallBack",
		"SC_InterruptInitialize",
		"SC_InterruptDone",
		"SC_InterruptDisable",
		"SC_GetCurrentProcessId",
		"SC_GetCurrentThreadId",
		"SC_IsPrimaryThread",
		"SC_KillAllOtherThreads",
		"SC_OtherThreadsRunning",
		"SC_SetProcPermissions",
		"SC_CallDebugger",
		"SC_SetKernelAlarm",
		"SC_RaiseException",
		"SC_GetCurrentPermissions",
		"SC_GetFSHeapInfo",
		"SC_SetKMode",
		"SC_GetProcAddrBits"
	};
  // from coreos\nk\kernel\schedule.c, class name THRD
  char *THRDNames[] = {
		"SysCall_THREAD_HANDLECLOSE",
		"SysCall_NOT_SUPPORTED",
		"SysCall_THREAD_TAKECRITSEC",
		"SysCall_THREAD_RELEASECRITSEC",
		"SysCall_THREAD_SUSPEND",
		"SysCall_THREAD_RESUME",
		"SysCall_THREAD_SLEEP",
		"SysCall_THREAD_CREATEEVENT",
		"SysCall_THREAD_SETPRIO",
		"SysCall_THREAD_GETPRIO",
		"SysCall_THREAD_SETLASTERROR",
		"SysCall_THREAD_GETLASTERROR",
		"SysCall_THREAD_TERMINATE",
		"SysCall_THREAD_GETRETCODE",
		"SysCall_THREAD_LOADSWITCH",
		"SysCall_THREAD_TERMINATE_AND_SIGNAL",
		"0",
		"SysCall_THREAD_TURNONPROFILING",
		"SysCall_THREAD_TURNOFFPROFILING",
		"SysCall_THREAD_TURNONSYSCALLPROFILING",
		"SysCall_THREAD_TURNOFFSYSCALLPROFILING"
	};
  // from coreos\nk\kernel\schedule.c, class name PROC
  char *PROCNames[] = {
		"SysCall_PROC_HANDLECLOSE",
		"SysCall_NOT_SUPPORTED",
		"SysCall_NOT_SUPPORTED",
		"SysCall_NOT_SUPPORTED",
		"SysCall_PROC_ALLOCMODULE",
		"SysCall_PROC_FREEMODULE",
		"SysCall_PROC_GETDEFSTACKSIZE",
		"SysCall_PROC_TERMINATE",
		"SC_PROC_IsBadPtr",
		"SysCall_PROC_SETDBGZONE",
		"SysCall_PROC_SETDBGINFO",
		"SysCall_PROC_GETDBGINFO",
    "SysCall_PROC_MAPPTR",
		"SysCall_PROC_MAPTOPROC",
		"SysCall_PROC_GETPROCFROMPTR",
		"SysCall_PROC_TERMINATEOTHER"
	};
  // from coreos\nk\kernel\schedule.c, class name EVNT
  char *EVNTNames[] = {
		"SysCall_EVENT_FREE",
		"SysCall_EVENT_WAIT",
		"SysCall_EVENT_MODIFY"
	};
  // from shell\notify\server\process.cpp, class name "NTFY"
  char *NTFYNames[] = { 
		"NotifyCallback",
		"PegSetUserNotification",
		"PegClearUserNotification",
		"PegRunAppAtTime",
		"PegRunAppAtEvent",
		"PegHandleAppNotifications"
	};
  // from shell\shell\server\shell.cpp, class name "SHEL"
  char *SHELNames[] = { 
    "ShellNotifyCallback",
    "ShellApiReservedForNK", /* Reserved for NK */
    "XShellExecuteEx",
		"XGetOpenFileName",
		"XGetSaveFileName",
		"SHGetFileInfo",
		"Shell_NotifyIcon",
		"SHAddToRecentDocs",
		"SHCreateShortcut",
		"SHCreateExplorerInstance"
  };
  // from coreos\nk\kernel\objdisp.c, class name "APIS"
  char *APISNames[] = {
	"SC_CloseAPISet",
    "SC_NotSupported",
    "SC_RegisterAPISet",
    "SC_CreateAPIHandle",
    "SC_VerifyAPIHandle"
	};


char *MethodToName(long iMethod, char ClassName[4]) 
{
	if (UNKNOWNMETHOD == iMethod) {
		return NULL;
	} else if (!strncmp(ClassName, "GDI", 3)) {
		return GDINames[iMethod];
	} else if (!strncmp(ClassName, "USER", 3)) {
		return USERNames[iMethod];
	} else if (!strncmp(ClassName, "INT", 3)) {
		return INTNames[iMethod];
	} else if (!strncmp(ClassName, "TEST", 3)) {
		return TESTNames[iMethod];
	} else if (!strncmp(ClassName, "W32A", 4)) {
		return W32ANames[iMethod];
	} else if (!strncmp(ClassName, "W32D", 4)) {
		return W32DNames[iMethod];
	} else if (!strncmp(ClassName, "W32H", 4)) {
		return W32HNames[iMethod];
	} else if (!strncmp(ClassName, "FFHN", 3)) {
		return FFHNNames[iMethod];
	} else if (!strncmp(ClassName, "DBFA", 3)) {
		return DBFANames[iMethod];
	} else if (!strncmp(ClassName, "DBOA", 3)) {
		return DBOANames[iMethod];
	} else if (!strncmp(ClassName, "Wn32", 3)) {
		return Wn32Names[iMethod];
	} else if (!strncmp(ClassName, "KW32", 3)) {
		return KW32Names[iMethod];
	} else if (!strncmp(ClassName, "NTFY", 3)) {
		return NTFYNames[iMethod];
	} else if (!strncmp(ClassName, "SHEL", 3)) {
		return SHELNames[iMethod];	
	} else if (!strncmp(ClassName, "PROC", 3)) {
		return PROCNames[iMethod];	
	} else if (!strncmp(ClassName, "THRD", 3)) {
		return THRDNames[iMethod];	
	} else if (!strncmp(ClassName, "EVNT", 3)) {
		return EVNTNames[iMethod];	
	} else if (!strncmp(ClassName, "APIS", 3)) {
		return APISNames[iMethod];	
	} else {
		RETAILMSG(1,(L"Unknown class name %a\r\n", ClassName));
		return NULL;
	}
}

// Not normal strncmp returns
int strncmp(const char *s1, const char *s2, unsigned int l)
{
	unsigned int i;
	if (!s1 || !s2) {
		return 1;
	}
	for (i = 0; i < l; i++) {
		if (s1[i] != s2[i]) {
			return 1;
    }
  }
	return 0;
}

#endif /*SYSCALL_PROFILING*/
