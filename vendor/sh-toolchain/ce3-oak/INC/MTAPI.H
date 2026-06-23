/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

Module Name:  

mtapi.h

Abstract:  

Notes: 


--*/
#ifndef MTAPI_H
#define MTAPI_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef WINCEMACRO
#error WINCEMACRO not defined __FILE__
#endif

#define lineClose IMPLICIT_DECL (LONG, SH_TAPI, 2,			 		\
			(HLINE hLine))

#define lineConfigDialogEdit IMPLICIT_DECL (LONG, SH_TAPI, 3,		\
			(DWORD dwDeviceID, HWND hwndOwner, 						\
			 LPCTSTR lpszDeviceClass, LPVOID const lpDeviceConfigIn, \
			 DWORD dwSize, LPVARSTRING lpDeviceConfigOut))

#define lineDeallocateCall IMPLICIT_DECL (LONG, SH_TAPI, 4,			\
			(HCALL hCall))

#define lineDrop IMPLICIT_DECL (LONG, SH_TAPI, 5,					\
			(HCALL hCall, LPCTSTR lpsUserUserInfo, DWORD dwSize))

#define lineGetDevCaps IMPLICIT_DECL (LONG, SH_TAPI, 6,				\
			(HLINEAPP hLineApp, DWORD dwDeviceID, 					\
			 DWORD dwAPIVersion, DWORD dwExtVersion, 				\
			 LPLINEDEVCAPS lpLineDevCaps))

#define lineGetDevConfig IMPLICIT_DECL (LONG, SH_TAPI, 7,			\
			 (DWORD dwDeviceID, LPVARSTRING lpDeviceConfig, 		\
			  LPCTSTR lpszDeviceClass))
    

#define lineGetTranslateCaps IMPLICIT_DECL (LONG, SH_TAPI, 8,		\
			(HLINEAPP hLineApp, DWORD dwAPIVersion, 				\
			 LPLINETRANSLATECAPS lpTranslateCaps))

#define TAPIlineInitialize IMPLICIT_DECL (LONG, SH_TAPI, 9,			\
			(LPHLINEAPP lphLineApp, HINSTANCE hInstance, 			\
			 HWND hWind, LPCTSTR lpszAppName,				 		\
			 LPDWORD lpdwNumDevs))

#define lineMakeCall IMPLICIT_DECL (LONG, SH_TAPI, 10,				\
			(HLINE hLine, LPHCALL lphCall, LPCTSTR lpszDestAddress, 	\
			 DWORD dwCountryCode, LPLINECALLPARAMS const lpCallParams))

#define lineNegotiateAPIVersion IMPLICIT_DECL (LONG, SH_TAPI, 11,	\
			(HLINEAPP hLineApp, DWORD dwDeviceID, 					\
			 DWORD dwAPILowVersion, DWORD dwAPIHighVersion, 		\
			 LPDWORD lpdwAPIVersion, LPLINEEXTENSIONID lpExtensionID))

#define lineOpen IMPLICIT_DECL (LONG, SH_TAPI, 12,					\
			(HLINEAPP hLineApp, DWORD dwDeviceID, LPHLINE lphLine,	\
			 DWORD dwAPIVersion, DWORD dwExtVersion, 				\
			 DWORD dwCallbackInstance, DWORD dwPrivileges, 			\
			 DWORD dwMediaModes, LPLINECALLPARAMS const lpCallParams))

#define lineSetDevConfig IMPLICIT_DECL (LONG, SH_TAPI, 13,			\
			 (DWORD dwDeviceID, LPVOID const lpDeviceConfig, 		\
			  DWORD dwSize, LPCTSTR lpszDeviceClass))

#define lineSetStatusMessages IMPLICIT_DECL (LONG, SH_TAPI, 14,		\
			 (HLINE hLine, DWORD dwLineStates, 						\
			  DWORD dwAddressStates))

#define TAPIlineShutdown IMPLICIT_DECL (LONG, SH_TAPI, 15,			\
			(HLINEAPP hLineApp))

#define lineTranslateAddress IMPLICIT_DECL (LONG, SH_TAPI, 16,		\
			(HLINEAPP hLineApp, DWORD dwDeviceID, 					\
			 DWORD dwAPIVersion, LPCTSTR lpszAddressIn, 			\
			 DWORD dwCard, DWORD dwTranslateOptions,				\
			 LPLINETRANSLATEOUTPUT lpTranslateOutput))

#define lineTranslateDialog IMPLICIT_DECL (LONG, SH_TAPI, 17,		\
			 (HLINEAPP hLineApp, DWORD dwDeviceID, 					\
			  DWORD dwAPIVersion, HWND hwndOwner, LPCTSTR lpszAddressIn))

#define lineGetID IMPLICIT_DECL (LONG, SH_TAPI, 18,			        \
             (HLINE hLine, DWORD dwAddressID, HCALL hCall,          \
              DWORD dwSelect, LPVARSTRING lpDeviceID,               \
              LPCTSTR lpszDeviceClass))

#define lineAddProvider IMPLICIT_DECL (LONG, SH_TAPI, 19,	        \
             (LPCTSTR lpszProviderFilename,							\
			  HWND hwndOwner,										\
			  LPDWORD lpdwPermanentProviderID))

#define lineSetCurrentLocation IMPLICIT_DECL (LONG, SH_TAPI, 20,	\
             (HLINEAPP hLineApp,                                    \
              DWORD dwLocation))
    
    
//
// New for v3.0
//


#define lineAccept IMPLICIT_DECL (LONG, SH_TAPI, 21,   \
             (HCALL hCall, LPCSTR lpsUserUserInfo, DWORD dwSize))

#define lineAddToConference IMPLICIT_DECL (LONG, SH_TAPI, 22,  \
             (HCALL hConfCall, HCALL hConsultCall))

#define lineAnswer IMPLICIT_DECL (LONG, SH_TAPI, 23,   \
             (HCALL hCall, LPCSTR lpsUserUserInfo, DWORD dwSize))

#define lineBlindTransfer IMPLICIT_DECL (LONG, SH_TAPI, 24,    \
             (HCALL hCall, LPCWSTR lpszDestAddress,           \
              DWORD dwCountryCode))

#define lineCompleteTransfer IMPLICIT_DECL (LONG, SH_TAPI, 25,  \
             (HCALL hCall, HCALL hConsultCall, \
              LPHCALL lphConfCall, DWORD dwTransferMode))

#define lineDevSpecific IMPLICIT_DECL (LONG, SH_TAPI, 26,  \
             (HLINE hLine, DWORD dwAddressID, HCALL hCall, \
               LPVOID lpParams, DWORD dwSize))

#define lineDial IMPLICIT_DECL (LONG, SH_TAPI, 27, \
             (HCALL hCall, LPCWSTR lpszDestAddress, DWORD dwCountryCode))

#define lineForward IMPLICIT_DECL (LONG, SH_TAPI, 28,   \
             (HLINE hLine, DWORD bAllAddresses, DWORD dwAddressID, \
               LPLINEFORWARDLIST const lpForwardList, DWORD dwNumRingsNoAnswer, \
               LPHCALL lphConsultCall, LPLINECALLPARAMS const lpCallParams))

#define lineGenerateDigits IMPLICIT_DECL (LONG, SH_TAPI, 29,    \
             (HCALL hCall, DWORD dwDigitMode,  \
               LPCWSTR lpszDigits, DWORD dwDuration))

#define lineGenerateTone IMPLICIT_DECL (LONG, SH_TAPI, 30, \
             (HCALL hCall, DWORD dwToneMode, DWORD dwDuration,  \
               DWORD dwNumTones, LPLINEGENERATETONE const lpTones))

#define lineGetAddressCaps IMPLICIT_DECL (LONG, SH_TAPI, 31,    \
             (HLINEAPP hLineApp, DWORD dwDeviceID, \
               DWORD dwAddressID, DWORD dwAPIVersion, DWORD dwExtVersion, \
               LPLINEADDRESSCAPS lpAddressCaps))

#define lineGetAddressID IMPLICIT_DECL (LONG, SH_TAPI, 32, \
             (HLINE hLine, LPDWORD lpdwAddressID, \
               DWORD dwAddressMode, LPCWSTR lpsAddress, DWORD dwSize))

#define lineGetAddressStatus IMPLICIT_DECL (LONG, SH_TAPI, 33,  \
             (HLINE hLine, DWORD dwAddressID,  \
               LPLINEADDRESSSTATUS lpAddressStatus))


#define lineGetAppPriority IMPLICIT_DECL (LONG, SH_TAPI, 34,    \
             (LPCWSTR lpszAppFilename, DWORD dwMediaMode,  \
               LPLINEEXTENSIONID lpExtensionID, DWORD dwRequestMode, \
               LPVARSTRING lpExtensionName, LPDWORD lpdwPriority))

#define lineGetCallInfo IMPLICIT_DECL (LONG, SH_TAPI, 35,  \
             (HCALL hCall, LPLINECALLINFO lpCallInfo))

#define lineGetCallStatus IMPLICIT_DECL (LONG, SH_TAPI, 36, \
             (HCALL hCall, LPLINECALLSTATUS lpCallStatus))

#define lineGetConfRelatedCalls IMPLICIT_DECL (LONG, SH_TAPI, 37,  \
             (HCALL hCall, LPLINECALLLIST lpCallList))
 
#define lineGetIcon IMPLICIT_DECL (LONG, SH_TAPI, 38,   \
             (DWORD dwDeviceID, LPCWSTR lpszDeviceClass, \
               LPHICON lphIcon))

#define lineGetLineDevStatus IMPLICIT_DECL (LONG, SH_TAPI, 39,  \
             (HLINE hLine, LPLINEDEVSTATUS lpLineDevStatus))

#define lineGetMessage IMPLICIT_DECL (LONG, SH_TAPI, 40, \
             (HLINEAPP hLineApp, \
               LPLINEMESSAGE lpMessage,  \
               DWORD dwTimeout))

#define lineGetNewCalls IMPLICIT_DECL (LONG, SH_TAPI, 41,  \
             (HLINE hLine, DWORD dwAddressID, DWORD dwSelect,   \
               LPLINECALLLIST lpCallList))

#define lineGetNumRings IMPLICIT_DECL (LONG, SH_TAPI, 42,  \
             (HLINE hLine, DWORD dwAddressID,  \
               LPDWORD lpdwNumRings))

#define lineGetProviderList IMPLICIT_DECL (LONG, SH_TAPI, 43,   \
             (DWORD dwAPIVersion, \
              LPLINEPROVIDERLIST lpProviderList))

#define lineGetStatusMessages IMPLICIT_DECL (LONG, SH_TAPI, 44, \
             (HLINE hLine, LPDWORD lpdwLineStates, \
               LPDWORD lpdwAddressStates))

#define lineHandoff IMPLICIT_DECL (LONG, SH_TAPI, 45, \
             (HCALL hCall, LPCWSTR lpszFileName, DWORD dwMediaMode))

#define lineHold IMPLICIT_DECL (LONG, SH_TAPI, 46, \
             (HCALL hCall))

#define TAPIlineInitializeEx IMPLICIT_DECL (LONG, SH_TAPI, 47, \
             (LPHLINEAPP lphLineApp, HINSTANCE hInstance, \
               LINECALLBACK lpfnCallback, LPCWSTR lpszFriendlyAppName, \
               LPDWORD lpdwNumDevs, LPDWORD lpdwAPIVersion, \
               LPLINEINITIALIZEEXPARAMS lpLineInitializeExParams))

#define lineMonitorDigits IMPLICIT_DECL (LONG, SH_TAPI, 48, \
             (HCALL hCall, DWORD dwDigitModes))

#define lineMonitorMedia IMPLICIT_DECL (LONG, SH_TAPI, 49, \
             (HCALL hCall, DWORD dwMediaModes))

#define lineNegotiateExtVersion IMPLICIT_DECL (LONG, SH_TAPI, 50, \
             (HLINEAPP hLineApp, DWORD dwDeviceID, \
               DWORD dwAPIVersion, DWORD dwExtLowVersion, \
               DWORD dwExtHighVersion, LPDWORD lpdwExtVersion))

#define linePickup IMPLICIT_DECL (LONG, SH_TAPI, 51,    \
             (HLINE hLine, DWORD dwAddressID, LPHCALL lphCall,  \
               LPCWSTR lpszDestAddress, LPCWSTR lpszGroupID))

#define linePrepareAddToConference IMPLICIT_DECL (LONG, SH_TAPI, 52, \
             (HCALL hConfCall, LPHCALL lphConsultCall, \
               LPLINECALLPARAMS const lpCallParams))

#define lineRedirect IMPLICIT_DECL (LONG, SH_TAPI, 53, \
             (HCALL hCall, LPCWSTR lpszDestAddress, \
               DWORD dwCountryCode))

#define lineReleaseUserUserInfo IMPLICIT_DECL (LONG, SH_TAPI, 54,  \
             (HCALL hCall))

#define lineRemoveFromConference IMPLICIT_DECL (LONG, SH_TAPI, 55, \
             (HCALL hCall))

#define lineSendUserUserInfo IMPLICIT_DECL (LONG, SH_TAPI, 56, \
             (HCALL hCall, LPCSTR lpsUserUserInfo, \
               DWORD dwSize))

#define lineSetAppPriority IMPLICIT_DECL (LONG, SH_TAPI, 57, \
             (LPCWSTR lpszAppFilename, DWORD dwMediaMode, \
               LPLINEEXTENSIONID lpExtensionID, DWORD dwRequestMode, \
               LPCWSTR lpszExtensionName, DWORD dwPriority))

#define lineSetCallParams IMPLICIT_DECL (LONG, SH_TAPI, 58, \
             (HCALL hCall, DWORD dwBearerMode, DWORD dwMinRate, \
               DWORD dwMaxRate, LPLINEDIALPARAMS const lpDialParams))

#define lineSetCallPrivilege IMPLICIT_DECL (LONG, SH_TAPI, 59, \
             (HCALL hCall, DWORD dwCallPrivilege))

#define lineSetMediaMode IMPLICIT_DECL (LONG, SH_TAPI, 60, \
             (HCALL hCall, DWORD dwMediaModes))

#define lineSetNumRings IMPLICIT_DECL (LONG, SH_TAPI, 61, \
             (HLINE hLine, DWORD dwAddressID, DWORD dwNumRings))

#define lineSetTerminal IMPLICIT_DECL (LONG, SH_TAPI, 62, \
             (HLINE hLine, DWORD dwAddressID, HCALL hCall, \
               DWORD dwSelect, DWORD dwTerminalModes, DWORD dwTerminalID, \
               DWORD bEnable))

#define lineSetTollList IMPLICIT_DECL (LONG, SH_TAPI, 63, \
             (HLINEAPP hLineApp, DWORD dwDeviceID, \
               LPCWSTR lpszAddressIn, DWORD dwTollListOption))


#define lineSetupConference IMPLICIT_DECL (LONG, SH_TAPI, 64, \
             (HCALL hCall, HLINE hLine, LPHCALL lphConfCall, \
               LPHCALL lphConsultCall, DWORD dwNumParties, \
               LPLINECALLPARAMS const lpCallParams))

#define lineSetupTransfer IMPLICIT_DECL (LONG, SH_TAPI, 65, \
             (HCALL hCall, LPHCALL lphConsultCall, \
               LPLINECALLPARAMS const lpCallParams))

#define lineSwapHold IMPLICIT_DECL (LONG, SH_TAPI, 66, \
             (HCALL hActiveCall, HCALL hHeldCall))

#define lineUnhold IMPLICIT_DECL (LONG, SH_TAPI, 67, (HCALL hCall))


#define phoneClose IMPLICIT_DECL (LONG, SH_TAPI, 68, (HPHONE hPhone))

#define phoneConfigDialog IMPLICIT_DECL (LONG, SH_TAPI, 69, \
             (DWORD dwDeviceID, HWND hwndOwner, \
               LPCWSTR lpszDeviceClass))

#define phoneDevSpecific IMPLICIT_DECL (LONG, SH_TAPI, 70, \
             (HPHONE hPhone, LPVOID lpParams, DWORD dwSize))

#define phoneGetDevCaps IMPLICIT_DECL (LONG, SH_TAPI, 71, \
             (HPHONEAPP hPhoneApp, DWORD dwDeviceID, \
               DWORD dwAPIVersion, DWORD dwExtVersion, LPPHONECAPS lpPhoneCaps))

#define phoneGetGain IMPLICIT_DECL (LONG, SH_TAPI, 72, \
             (HPHONE hPhone, DWORD dwHookSwitchDev, LPDWORD lpdwGain))

#define phoneGetHookSwitch IMPLICIT_DECL (LONG, SH_TAPI, 73, \
             (HPHONE hPhone, LPDWORD lpdwHookSwitchDevs))

#define phoneGetIcon IMPLICIT_DECL (LONG, SH_TAPI, 74, \
             (DWORD dwDeviceID, LPCWSTR lpszDeviceClass, \
               LPHICON lphIcon))


#define phoneGetID IMPLICIT_DECL (LONG, SH_TAPI, 75, \
             (HPHONE hPhone, LPVARSTRING lpDeviceID, \
               LPCWSTR lpszDeviceClass))

#define phoneGetMessage IMPLICIT_DECL (LONG, SH_TAPI, 76, \
             (HPHONEAPP hPhoneApp, LPPHONEMESSAGE lpMessage, \
               DWORD dwTimeout))

#define phoneGetRing IMPLICIT_DECL (LONG, SH_TAPI, 77, \
             (HPHONE hPhone, LPDWORD lpdwRingMode, \
               LPDWORD lpdwVolume))

#define phoneGetStatus IMPLICIT_DECL (LONG, SH_TAPI, 78, \
             (HPHONE hPhone, LPPHONESTATUS lpPhoneStatus))

#define phoneGetStatusMessages IMPLICIT_DECL (LONG, SH_TAPI, 79, \
             (HPHONE hPhone, LPDWORD lpdwPhoneStates, \
               LPDWORD lpdwButtonModes, LPDWORD lpdwButtonStates))

#define phoneGetVolume IMPLICIT_DECL (LONG, SH_TAPI, 80, \
             (HPHONE hPhone, DWORD dwHookSwitchDev, \
               LPDWORD lpdwVolume))

#define TAPIphoneInitializeEx IMPLICIT_DECL (LONG, SH_TAPI, 81, \
             (LPHPHONEAPP lphPhoneApp, HINSTANCE hInstance, \
               PHONECALLBACK lpfnCallback, LPCWSTR lpszFriendlyAppName, \
               LPDWORD lpdwNumDevs, LPDWORD lpdwAPIVersion, \
               LPPHONEINITIALIZEEXPARAMS lpPhoneInitializeExParams))


#define phoneNegotiateAPIVersion IMPLICIT_DECL (LONG, SH_TAPI, 82, \
             (HPHONEAPP hPhoneApp, DWORD dwDeviceID, \
               DWORD dwAPILowVersion, DWORD dwAPIHighVersion, \
               LPDWORD lpdwAPIVersion, LPPHONEEXTENSIONID lpExtensionID))

#define phoneNegotiateExtVersion IMPLICIT_DECL (LONG, SH_TAPI, 83, \
             (HPHONEAPP hPhoneApp, DWORD dwDeviceID, \
               DWORD dwAPIVersion, DWORD dwExtLowVersion, \
               DWORD dwExtHighVersion, LPDWORD lpdwExtVersion))

#define phoneOpen IMPLICIT_DECL (LONG, SH_TAPI, 84, \
             (HPHONEAPP hPhoneApp, DWORD dwDeviceID, LPHPHONE lphPhone, \
               DWORD dwAPIVersion, DWORD dwExtVersion, \
               DWORD dwCallbackInstance, DWORD dwPrivilege))

#define phoneSetGain IMPLICIT_DECL (LONG, SH_TAPI, 85, \
             (HPHONE hPhone, DWORD dwHookSwitchDev, DWORD dwGain))

#define phoneSetHookSwitch IMPLICIT_DECL (LONG, SH_TAPI, 86, \
             (HPHONE hPhone, DWORD dwHookSwitchDevs, \
               DWORD dwHookSwitchMode))

#define phoneSetRing IMPLICIT_DECL (LONG, SH_TAPI, 87, \
             (HPHONE hPhone, DWORD dwRingMode, DWORD dwVolume))

#define phoneSetStatusMessages IMPLICIT_DECL (LONG, SH_TAPI, 88, \
             (HPHONE hPhone, DWORD dwPhoneStates, \
               DWORD dwButtonModes, DWORD dwButtonStates))

#define phoneSetVolume IMPLICIT_DECL (LONG, SH_TAPI, 89, \
             (HPHONE hPhone, DWORD dwHookSwitchDev, DWORD dwVolume))

#define TAPIphoneShutdown IMPLICIT_DECL (LONG, SH_TAPI, 90, (HPHONEAPP hPhoneApp))


#ifdef __cplusplus
}
#endif

#endif // MTAPI_H

