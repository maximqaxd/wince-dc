// Copyright (c) 1999-2000 Microsoft Corporation.  All rights reserved.
//***************************************************************************
//  TITLE:	xtime.h - exception timing header file
//
//
//***************************************************************************
#ifndef __XTIME_H__
#define __XTIME_H__

//#define XTIME		1 			// build with this uncommented
							// to compile exception timing code

#ifdef WINCEPROFILE /// #ifndef SHIP_BUILD
#define PROFILE      1				// build with this uncommented
#endif

#define XT_GENERAL_EXCEPTION 			0
#define XT_TLB					1

#define XT_FIRST_THREAD_DISPATCH			2	// first thread dispatch #
#define XT_THREAD_TAKECRITSEC			2
#define XT_THREAD_RELEASECRITSEC			3
#define XT_THREAD_SUSPEND				4
#define XT_THREAD_RESUME				5
#define XT_THREAD_SLEEP				6
#define XT_THREAD_CREATEEVENT			7
#define XT_THREAD_SETTHREADPRIO			8
#define XT_THREAD_GETTHREADPRIO			9
#define XT_THREAD_SETLASTERROR			10
#define XT_THREAD_GETLASTERROR			11
#define XT_THREAD_TERMINATE				12
#define XT_THREAD_GETRETCODE			13
#define XT_THREAD_LOADSWITCH			14
#define XT_THREAD_TERMINATE_AND_SIGNAL		15
#define XT_THREAD_HEAPSTACK				16
#define XT_LAST_THREAD_DISPATCH			16	// last thread dispatch #

#define XT_FIRST_PROC_DISPATCH			17	// first thread dispatch #
#define XT_PROC_CREATETHREAD			17
#define XT_PROC_CREATEPROC				18
#define XT_PROC_ALLOCMODULE				19
#define XT_PROC_FREEMODULE				20
#define XT_PROC_GETDEFSTACKSIZE			21
#define XT_PROC_TERMINATE				22
#define XT_PROC_ISBADPTR				23
#define XT_PROC_SETDBGZONE				24
#define XT_PROC_SETDBGINFO				25
#define XT_PROC_GETDBGINFO				26
#define XT_LAST_PROC_DISPATCH			26
#define XT_NUM_PROC_DISPATCH			XT_LAST_PROC_DISPATCH-XT_FIRST_PROC_DISPATCH+1

#define XT_FIRST_EVENT_DISPATCH			27
#define XT_EVENT_FREE				27
#define XT_EVENT_WAIT				28
#define XT_EVENT_MODIFY				29
#define XT_LAST_EVENT_DISPATCH			29
#define XT_NUM_EVENT_DISPATCH			XT_LAST_EVENT_DISPATCH+XT_FIRST_EVENT_DISPATCH+1

#define XT_FIRST_PSL					30	// first PSL #
#define XT_CreateAPISet				30
#define XT_VirtualAlloc				31
#define XT_VirtualFree				32
#define XT_VirtualProtect				33
#define XT_VirtualQuery				34
#define XT_VirtualCopy				35
#define XT_LoadLibrary				36
#define XT_FreeLibrary				37
#define XT_GetProcAddress				38
#define XT_ThreadAttachAllDLLs			39
#define XT_ThreadDetachAllDLLs			40
#define XT_GetTickCount				41
#define XT_OutputDebugString			42
#define XT_CreateCallBack				43
//#define XT_NoSupported				44
#define XT_TlsCall					45
#define XT_GetSystemInfo				46
#define XT_StackHeap					47
#define XT_ropen					48
#define XT_rread					49
#define XT_rwrite					50
#define XT_rlseek					51
#define XT_rclose					52
#define XT_CreateSection				53
#define XT_DeleteSection				54
#define XT_RegisterDbgZones				55
#define XT_NoSupported				56
#define XT_NKDbgPrintfW				57
#define XT_ProfileSyscall				58
#define XT_FindResource				59
#define XT_LoadResource				60
#define XT_LockResource				61
#define XT_SizeofResource				62
#define XT_LAST_PSL					62	// last PSL #
#define XT_NUM_PSL					XT_LAST_PSL-XT_FIRST_PSL+1


#define XT_FIRST_GWE					63	// first GWE #
#define XT_GweNotifyCallback			63
#define XT_RESERVED_for_NK				64
#define XT_RegisterClassWStub			65
#define XT_UnregisterClassW				66
#define XT_CWindowManager__CreateWindowExW	67
#define XT_PostMessageW				68
#define XT_PostQuitMessage				69
#define XT_SendMessageW				70
#define XT_GetMessageW				71
#define XT_TranslateMessage				72
#define XT_DispatchMessageW				73
#define XT_GetCapture				74
#define XT_SetCapture				75
#define XT_ReleaseCapture				76
#define XT_CWindow__SetWindowPos			77
#define XT_CWindow__GetWindowRect			78
#define XT_CWindow__GetClientRect			79
#define XT_CWindow__InvalidateRect			80
#define XT_CWindow__GetWindow			81
#define XT_GetSystemMetrics				82
#define XT_SetRect					83
#define XT_SetRectEmpty				84
#define XT_CopyRect					85
#define XT_InflateRect				86
#define XT_IntersectRect				87
#define XT_UnionRect					88
#define XT_OffsetRect				89
#define XT_IsRectEmpty				90
#define XT_EqualRect					91
#define XT_PtInRect					92
#define XT_CWindowManager__WindowFromPoint	93
#define XT_CWindow__ChildWindowFromPoint		94
#define XT_CWindow__ClientToScreen			95
#define XT_CWindow__ScreenToClient			96
#define XT_CWindow__SetWindowText			97
#define XT_CWindow__GetWindowText			98
#define XT_CWindow__SetWindowLong			99
#define XT_CWindow__GetWindowLong			100
#define XT_CWindow__BeginPaint			101
#define XT_CWindow__EndPaint			102
#define XT_CWindow__GetDC				103
#define XT_CWindow__ReleaseDC			104
#define XT_CWindow__DefWindowProc			105
#define XT_CWindow__GetClassLong			106
#define XT_CWindow__SetClassLong			107
#define XT_CWindow__DestroyWindow			108
#define XT_CWindow__ShowWindow			109
#define XT_CWindow__UpdateWindow			110
#define XT_CWindow__SetParent			111
#define XT_CWindow__GetParent			112
#define XT_MessageBoxW				113
#define XT_SetFocus					114
#define XT_GetFocus					115
#define XT_GetActiveWindow				116
#define XT_CWindow__GetWindowDC			117
#define XT_GetSysColor				118
#define XT_AdjustWindowRectEx			119
#define XT_CWindow__IsWindow			120
#define XT_CreatePopupMenu				121
#define XT_InsertMenuW				122
#define XT_AppendMenuW				123
#define XT_RemoveMenu				124
#define XT_DestroyMenu				125
#define XT_TrackPopupMenu				126
#define XT_LoadMenuW					127
#define XT_EnableMenuItem				128
#define XT_CWindow__MoveWindow			129
#define XT_CWindow__GetUpdateRgn			130
#define XT_CWindow__GetUpdateRect			131
#define XT_CWindow__BringWindowToTop		132
#define XT_CWindow__GetWindowTextLengthW		133
#define XT_CWindow__IsChild				134
#define XT_CWindow__IsWindowVisible		135
#define XT_CWindow__ValidateRect			136
#define XT_LoadBitmapW				137
#define XT_CheckMenuItem				138
#define XT_CheckMenuRadioItem			139
#define XT_DeleteMenu				140
#define XT_LoadIconW					141
#define XT_DrawIconEx				142
#define XT_DestroyIcon				143
#define XT_GetAsyncKeyState				144
#define XT_LoadStringW				145
#define XT_DialogBoxIndirectParamW			146
#define XT_EndDialog					147
#define XT_GetDlgItem				148
#define XT_GetDlgCtrlID				149
#define XT_GetKeyState				150
#define XT_KeybdInquire				151
#define XT_KeybdInitStates				152
#define XT_PostKeybdMessage				153
#define XT_KeybdVKeyToUnicode			154
#define XT_keybd_event				155
#define XT_mouse_event				156
#define XT_CWindow__SetScrollInfo			157
#define XT_CWindow__SetScrollPos			158
#define XT_CWindow__SetScrollRange			159
#define XT_CWindow__GetScrollInfo			160
#define XT_PeekMessageW				161
#define XT_MapVirtualKeyW				162
#define XT_GetMessageWNoWait			163
#define XT_GetClassNameW				164
#define XT_CWindowManager__MapWindowPoints	165
#define XT_LoadImageW				166
#define XT_GetForegroundWindow			167
#define XT_SetForegroundWindow			168
#define XT_CWindowManager__RegisterTaskBar	169
#define XT_SetActiveWindow				170
#define XT_CWindowManager__CallWindowProcW	171
#define XT_GetClassInfoW				172
#define XT_GetNextDlgTabItem			173
#define XT_CreateDialogIndirectParamW		174
#define XT_IsDialogMessage				175
#define XT_SetDlgItemInt				176
#define XT_GetDlgItemInt				177
#define XT_CWindowManager__FindWindowW		178
#define XT_CreateCaret				179
#define XT_DestroyCaret				180
#define XT_HideCaret					181
#define XT_ShowCaret					182
#define XT_SetCaretPos				183
#define XT_GetCaretPos				184
#define XT_TouchGetCalibrationPointCount		185
#define XT_TouchGetCalibrationPoint		186
#define XT_TouchReadCalibrationPoint		187
#define XT_TouchAcceptCalibration			188
#define XT_ExtractIconW				189
#define XT_SetTimer					190
#define XT_KillTimer					191
#define XT_GetNextDlgGroupItem			192
#define XT_LAST_GWE					192	// first GWE #
#define XT_NUM_GWE					XT_LAST_GWE-XT_FIRST_GWE+1

#define MAX_XTIME					193	// max exception timing items 

//
// SC_ProfileSyscall operations (passed as first word of data buffer)
//
#define XTIME_SC_DATA				0
#define XTIME_TIMER_RESET				1
#define XTIME_TIMER_READ				2
#define XTIME_PROFILE_DATA				3
#define XTIME_MODULE_NAME				4

#ifdef WINCECODETEST
#define XTIME_CODETEST					-1
#endif

//
// exception timing data buffer
//
#ifdef C_ONLY
typedef struct xt_struct {
	DWORD	dwOp;					// profile syscall operation
	DWORD	dwTime[MAX_XTIME];			// syscall elapsed time buffer
	DWORD	dwCount[MAX_XTIME];			// syscall count 
	DWORD	dwMax[MAX_XTIME];			// maximum time for syscall
	DWORD	dwMin[MAX_XTIME];			// minimum time for syscall
} XT, *LPXT;
#endif

#endif


