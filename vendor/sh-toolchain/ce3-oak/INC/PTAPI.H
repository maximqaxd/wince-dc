/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

Module Name:  

ptapi.h

Abstract:

Private portion of tapi.h

Notes: 


--*/
#ifndef PTAPI_H
#define PTAPI_H

#ifdef __cplusplus
extern "C" {
#endif

LONG WINAPI lineClose(HLINE hLine);

LONG WINAPI lineDeallocateCall(HCALL hCall);

LONG WINAPI lineDrop(HCALL hCall, LPCSTR lpsUserUserInfo, DWORD dwSize);

LONG WINAPI lineGetDevCaps(HLINEAPP hLineApp, DWORD dwDeviceID, 
					DWORD dwAPIVersion, DWORD dwExtVersion, 
					LPLINEDEVCAPS lpLineDevCaps);

LONG WINAPI lineGetDevConfig(DWORD dwDeviceID, LPVARSTRING lpDeviceConfig, 
					  LPCWSTR lpszDeviceClass);


LONG WINAPI lineGetTranslateCaps(HLINEAPP hLineApp, DWORD dwAPIVersion, 
						  LPLINETRANSLATECAPS lpTranslateCaps);

LONG WINAPI lineInitialize(LPHLINEAPP lphLineApp, HINSTANCE hInstance, 
					LINECALLBACK lpfnCallback, LPCWSTR lpszAppName, 
					LPDWORD lpdwNumDevs);

LONG WINAPI lineMakeCall(HLINE hLine, LPHCALL lphCall, LPCWSTR lpszDestAddress, 
				  DWORD dwCountryCode, LPLINECALLPARAMS const lpCallParams);

LONG WINAPI lineNegotiateAPIVersion(HLINEAPP hLineApp, DWORD dwDeviceID, 
							 DWORD dwAPILowVersion, DWORD dwAPIHighVersion, 
							 LPDWORD lpdwAPIVersion,
							 LPLINEEXTENSIONID lpExtensionID);

LONG WINAPI lineOpen(HLINEAPP hLineApp, DWORD dwDeviceID, LPHLINE lphLine,
			  DWORD dwAPIVersion, DWORD dwExtVersion, 
			  DWORD dwCallbackInstance, DWORD dwPrivileges, 
			  DWORD dwMediaModes, LPLINECALLPARAMS const lpCallParams);

LONG WINAPI lineSetDevConfig(DWORD dwDeviceID, LPVOID const lpDeviceConfig, 
					  DWORD dwSize, LPCWSTR lpszDeviceClass);

LONG WINAPI lineSetStatusMessages(HLINE hLine, DWORD dwLineStates, 
						   DWORD dwAddressStates);

LONG WINAPI lineShutdown(HLINEAPP hLineApp);

LONG WINAPI lineTranslateAddress(HLINEAPP hLineApp, DWORD dwDeviceID, 
						  DWORD dwAPIVersion, LPCWSTR lpszAddressIn, 
						  DWORD dwCard, DWORD dwTranslateOptions,
						  LPLINETRANSLATEOUTPUT lpTranslateOutput);

LONG WINAPI lineTranslateDialog(HLINEAPP hLineApp, DWORD dwDeviceID, 
						 DWORD dwAPIVersion, HWND hwndOwner,
						 LPCWSTR lpszAddressIn);

LONG WINAPI lineConfigDialogEdit(DWORD dwDeviceID, HWND hwndOwner, 
						  LPCWSTR lpszDeviceClass,
						  LPVOID const lpDeviceConfigIn, 
						  DWORD dwSize, LPVARSTRING lpDeviceConfigOut);

LONG WINAPI lineGetID(HLINE hLine, DWORD dwAddressID, HCALL hCall,          
			   DWORD dwSelect, LPVARSTRING lpDeviceID,               
			   LPCWSTR lpszDeviceClass);

LONG WINAPI lineAddProvider(LPCWSTR lpszProviderFilename,
               HWND hwndOwner, LPDWORD lpdwPermanentProviderID);

//
// New for v3.0
//

LONG WINAPI lineAccept(HCALL hCall, LPCSTR lpsUserUserInfo, DWORD dwSize);

LONG WINAPI lineAddToConference(HCALL hConfCall, HCALL hConsultCall);

LONG WINAPI lineAnswer(HCALL hCall, LPCSTR lpsUserUserInfo, DWORD dwSize);

LONG WINAPI lineBlindTransfer(HCALL hCall, LPCWSTR lpszDestAddress,
               DWORD dwCountryCode);

LONG WINAPI lineCompleteTransfer(HCALL hCall, HCALL hConsultCall,
               LPHCALL lphConfCall, DWORD dwTransferMode);

LONG WINAPI lineDevSpecific(HLINE hLine, DWORD dwAddressID, HCALL hCall,
               LPVOID lpParams, DWORD dwSize);

LONG WINAPI lineDial(HCALL hCall, LPCWSTR lpszDestAddress, DWORD dwCountryCode);

LONG WINAPI lineForward(HLINE hLine, DWORD bAllAddresses, DWORD dwAddressID,
               LPLINEFORWARDLIST const lpForwardList, DWORD dwNumRingsNoAnswer,
               LPHCALL lphConsultCall, LPLINECALLPARAMS const lpCallParams);

LONG WINAPI lineGenerateDigits(HCALL hCall, DWORD dwDigitMode,
               LPCWSTR lpszDigits, DWORD dwDuration);

LONG WINAPI lineGenerateTone(HCALL hCall, DWORD dwToneMode, DWORD dwDuration,
               DWORD dwNumTones, LPLINEGENERATETONE const lpTones);

LONG WINAPI lineGetAddressCaps(HLINEAPP hLineApp, DWORD dwDeviceID,
               DWORD dwAddressID, DWORD dwAPIVersion, DWORD dwExtVersion,
               LPLINEADDRESSCAPS lpAddressCaps);

LONG WINAPI lineGetAddressID(HLINE hLine, LPDWORD lpdwAddressID,
               DWORD dwAddressMode, LPCWSTR lpsAddress, DWORD dwSize);

LONG WINAPI lineGetAddressStatus(HLINE hLine, DWORD dwAddressID,
               LPLINEADDRESSSTATUS lpAddressStatus);


LONG WINAPI lineGetAppPriority(LPCWSTR lpszAppFilename, DWORD dwMediaMode,
               LPLINEEXTENSIONID lpExtensionID, DWORD dwRequestMode,
               LPVARSTRING lpExtensionName, LPDWORD lpdwPriority);

LONG WINAPI lineGetCallInfo(HCALL hCall, LPLINECALLINFO lpCallInfo);

LONG WINAPI lineGetCallStatus(HCALL hCall, LPLINECALLSTATUS lpCallStatus);

LONG WINAPI lineGetConfRelatedCalls(HCALL hCall, LPLINECALLLIST lpCallList);
 
LONG WINAPI lineGetIcon(DWORD dwDeviceID, LPCWSTR lpszDeviceClass,
               LPHICON lphIcon);

LONG WINAPI lineGetLineDevStatus(HLINE hLine, LPLINEDEVSTATUS lpLineDevStatus);

LONG WINAPI lineGetMessage(HLINEAPP hLineApp, LPLINEMESSAGE lpMessage,
               DWORD dwTimeout);

LONG WINAPI lineGetNewCalls(HLINE hLine, DWORD dwAddressID, DWORD dwSelect,
               LPLINECALLLIST lpCallList);

LONG WINAPI lineGetNumRings(HLINE hLine, DWORD dwAddressID,
               LPDWORD lpdwNumRings);

LONG WINAPI lineGetProviderList(DWORD dwAPIVersion,
               LPLINEPROVIDERLIST lpProviderList);

LONG WINAPI lineGetStatusMessages(HLINE hLine, LPDWORD lpdwLineStates,
               LPDWORD lpdwAddressStates);

LONG WINAPI lineHandoff(HCALL hCall, LPCWSTR lpszFileName, DWORD dwMediaMode);

LONG WINAPI lineHold(HCALL hCall);

LONG WINAPI lineInitializeEx(LPHLINEAPP lphLineApp, HINSTANCE hInstance,
               LINECALLBACK lpfnCallback, LPCWSTR lpszFriendlyAppName,
               LPDWORD lpdwNumDevs, LPDWORD lpdwAPIVersion,
               LPLINEINITIALIZEEXPARAMS lpLineInitializeExParams);

LONG WINAPI lineMonitorDigits(HCALL hCall, DWORD dwDigitModes);

LONG WINAPI lineMonitorMedia(HCALL hCall, DWORD dwMediaModes);

LONG WINAPI lineNegotiateExtVersion(HLINEAPP hLineApp, DWORD dwDeviceID,
               DWORD dwAPIVersion, DWORD dwExtLowVersion,
               DWORD dwExtHighVersion, LPDWORD lpdwExtVersion);

LONG WINAPI linePickup(HLINE hLine, DWORD dwAddressID, LPHCALL lphCall,
               LPCWSTR lpszDestAddress, LPCWSTR lpszGroupID);

LONG WINAPI linePrepareAddToConference(HCALL hConfCall, LPHCALL lphConsultCall,
               LPLINECALLPARAMS const lpCallParams);

LONG WINAPI lineRedirect(HCALL hCall, LPCWSTR lpszDestAddress,
               DWORD dwCountryCode);

LONG WINAPI lineReleaseUserUserInfo(HCALL hCall);

LONG WINAPI lineRemoveFromConference(HCALL hCall);

LONG WINAPI lineSendUserUserInfo(HCALL hCall, LPCSTR lpsUserUserInfo,
               DWORD dwSize);

LONG WINAPI lineSetAppPriority(LPCWSTR lpszAppFilename, DWORD dwMediaMode,
               LPLINEEXTENSIONID lpExtensionID, DWORD dwRequestMode,
               LPCWSTR lpszExtensionName, DWORD dwPriority);

LONG WINAPI lineSetCallParams(HCALL hCall, DWORD dwBearerMode, DWORD dwMinRate,
               DWORD dwMaxRate, LPLINEDIALPARAMS const lpDialParams);

LONG WINAPI lineSetCallPrivilege(HCALL hCall, DWORD dwCallPrivilege);

LONG WINAPI lineSetMediaMode(HCALL hCall, DWORD dwMediaModes);

LONG WINAPI lineSetNumRings(HLINE hLine, DWORD dwAddressID, DWORD dwNumRings);

LONG WINAPI lineSetTerminal(HLINE hLine, DWORD dwAddressID, HCALL hCall,
               DWORD dwSelect, DWORD dwTerminalModes, DWORD dwTerminalID,
               DWORD bEnable);

LONG WINAPI lineSetTollList(HLINEAPP hLineApp, DWORD dwDeviceID,
               LPCWSTR lpszAddressIn, DWORD dwTollListOption);


LONG WINAPI lineSetupConference(HCALL hCall, HLINE hLine, LPHCALL lphConfCall,
               LPHCALL lphConsultCall, DWORD dwNumParties,
               LPLINECALLPARAMS const lpCallParams);

LONG WINAPI lineSetupTransfer(HCALL hCall, LPHCALL lphConsultCall,
               LPLINECALLPARAMS const lpCallParams);

LONG WINAPI lineSwapHold(HCALL hActiveCall, HCALL hHeldCall);

LONG WINAPI lineUnhold(HCALL hCall);


LONG WINAPI phoneClose(HPHONE hPhone);

LONG WINAPI phoneConfigDialog(DWORD dwDeviceID, HWND hwndOwner,
               LPCWSTR lpszDeviceClass);

LONG WINAPI phoneDevSpecific(HPHONE hPhone, LPVOID lpParams, DWORD dwSize);

LONG WINAPI phoneGetDevCaps(HPHONEAPP hPhoneApp, DWORD dwDeviceID,
               DWORD dwAPIVersion, DWORD dwExtVersion, LPPHONECAPS lpPhoneCaps);

LONG WINAPI phoneGetGain(HPHONE hPhone, DWORD dwHookSwitchDev, LPDWORD lpdwGain);

LONG WINAPI phoneGetHookSwitch(HPHONE hPhone, LPDWORD lpdwHookSwitchDevs);

LONG WINAPI phoneGetIcon(DWORD dwDeviceID, LPCWSTR lpszDeviceClass,
               LPHICON lphIcon);


LONG WINAPI phoneGetID(HPHONE hPhone, LPVARSTRING lpDeviceID,
               LPCWSTR lpszDeviceClass);

LONG WINAPI phoneGetMessage(HPHONEAPP hPhoneApp, LPPHONEMESSAGE lpMessage,
               DWORD dwTimeout);

LONG WINAPI phoneGetRing(HPHONE hPhone, LPDWORD lpdwRingMode,
               LPDWORD lpdwVolume);

LONG WINAPI phoneGetStatus(HPHONE hPhone, LPPHONESTATUS lpPhoneStatus);

LONG WINAPI phoneGetStatusMessages(HPHONE hPhone, LPDWORD lpdwPhoneStates,
               LPDWORD lpdwButtonModes, LPDWORD lpdwButtonStates);

LONG WINAPI phoneGetVolume(HPHONE hPhone, DWORD dwHookSwitchDev,
               LPDWORD lpdwVolume);

LONG WINAPI phoneInitializeEx(LPHPHONEAPP lphPhoneApp, HINSTANCE hInstance,
               PHONECALLBACK lpfnCallback, LPCWSTR lpszFriendlyAppName,
               LPDWORD lpdwNumDevs, LPDWORD lpdwAPIVersion,
               LPPHONEINITIALIZEEXPARAMS lpPhoneInitializeExParams);


LONG WINAPI phoneNegotiateAPIVersion(HPHONEAPP hPhoneApp, DWORD dwDeviceID,
               DWORD dwAPILowVersion, DWORD dwAPIHighVersion,
               LPDWORD lpdwAPIVersion, LPPHONEEXTENSIONID lpExtensionID);

LONG WINAPI phoneNegotiateExtVersion(HPHONEAPP hPhoneApp, DWORD dwDeviceID,
               DWORD dwAPIVersion, DWORD dwExtLowVersion,
               DWORD dwExtHighVersion, LPDWORD lpdwExtVersion);

LONG WINAPI phoneOpen(HPHONEAPP hPhoneApp, DWORD dwDeviceID, LPHPHONE lphPhone,
               DWORD dwAPIVersion, DWORD dwExtVersion,
               DWORD dwCallbackInstance, DWORD dwPrivilege);

LONG WINAPI phoneSetGain(HPHONE hPhone, DWORD dwHookSwitchDev, DWORD dwGain);

LONG WINAPI phoneSetHookSwitch(HPHONE hPhone, DWORD dwHookSwitchDevs,
               DWORD dwHookSwitchMode);

LONG WINAPI phoneSetRing(HPHONE hPhone, DWORD dwRingMode, DWORD dwVolume);

LONG WINAPI phoneSetStatusMessages(HPHONE hPhone, DWORD dwPhoneStates,
               DWORD dwButtonModes, DWORD dwButtonStates);

LONG WINAPI phoneSetVolume(HPHONE hPhone, DWORD dwHookSwitchDev, DWORD dwVolume);

LONG WINAPI phoneShutdown(HPHONEAPP hPhoneApp);


#ifdef __cplusplus
}
#endif

#endif // PTAPI_H

