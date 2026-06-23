/******************************************************************************
Copyright (c) 1996-2000 Microsoft Corporation.  All rights reserved.

tapihelp.h : TAPI helper functions include file


******************************************************************************/
#ifndef _TAPIHELP_H_
#define _TAPIHELP_H_

#define MIN_TAPI_VERSION    0x00010004    // Version 1.4
#define MAX_TAPI_VERSION    0x00010005    // Version 1.5

#define DEV_CLASS_COMM_DATAMODEM TEXT("comm/datamodem")


//
// Library globals.
//
extern HLINEAPP v_hLineApp;
extern DWORD    v_dwNumDevs;
extern DWORD	v_dwVersion;
extern DWORD	v_dwTapiHelpDebug;

//
// Debug zones
//
#define TAPIDBGZONE(n)		(0x00000001<<n)

#define TAPI_ZONE_INFO		TAPIDBGZONE(0)
#define	TAPI_ZONE_ALLOC		TAPIDBGZONE(12)
#define	TAPI_ZONE_FUNCTION	TAPIDBGZONE(13)
#define TAPI_ZONE_WARN		TAPIDBGZONE(14)
#define TAPI_ZONE_ERROR		TAPIDBGZONE(15)


//
// Function declarations
//
LONG			TapiInitialize(HINSTANCE hInstance, LINECALLBACK lpfnCallback,
							   LPCTSTR szAppName, DWORD dwDebugFlag);
LPTSTR 			TapiGetLineName(DWORD dwDeviceID, LPWORD pwDeviceType,
								LPBOOL pfActive);
DWORD 			TapiGetVersion(DWORD dwDeviceID);
LPLINEDEVCAPS	TapiLineGetDevCaps(DWORD dwDeviceID, DWORD dwAPIVersion);
DWORD			TapiFindDeviceID (LPTSTR szDevName, DWORD dwMaxLen);
LPVARSTRING		TapiLineGetDevConfig (DWORD dwDeviceID, LPCTSTR szDeviceClass);
DWORD			TapiLineGetID (HCALL hCall);
LPTSTR			TapiLineTranslateAddress(DWORD dwDeviceID, DWORD dwCountryCode,
										 LPCTSTR szAreaCode,
										 LPCTSTR szLocalPhoneNumber,
										 BOOL fDialAble, DWORD dwTranslateOpts);
LPLINETRANSLATECAPS TapiLineGetTranslateCaps ();
LPTSTR			TapiGetDefaultAreaCountryCode(LPDWORD pdwCountryCode);
LPTSTR			TapiGetDefaultLocationName();


#endif // _TAPIHELP_H_
