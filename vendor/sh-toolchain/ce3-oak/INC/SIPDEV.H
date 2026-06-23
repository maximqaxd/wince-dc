// Copyright (c) 1999-2000 Microsoft Corporation.  All rights reserved.
#ifndef __INCLUDE_SIPDEV__
#define __INCLUDE_SIPDEV__

#ifdef __cplusplus
extern "C" {
#endif


//	Device io control commands for the sip.
//	Also used as dwCode for WM_SIPTHREADCALL.
#define SIPDEV_SHOW_IM			1
#define SIPDEV_GET_INFO 		2
#define SIPDEV_SET_INFO 		3
#define SIPDEV_GET_CURRENT_IM	4
#define SIPDEV_SET_CURRENT_IM	5
#define SIPDEV_REGISTER_UI		6
#define	SIPDEV_DEFAULT_RECT		7
#define	SIPDEV_ENUMERATE		8
#define SIPDEV_IMM_SET_ACTIVE_CONTEXT	9


struct SIP_IMM_SET_ACTIVE_CONTEXT
	{
	HWND	hwnd;
	BOOL	bOpen;
	DWORD	dwConversion;
	DWORD	dwSentence;
	HKL		hkl;
	};



#ifdef __cplusplus
};
#endif

#endif//__INCLUDE_SIPDEV__
