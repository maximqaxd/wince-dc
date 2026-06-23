/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.
Portions Copyright Intel/Microsoft 1992-93.  All Rights Reserved.

Module Name:

tapicomm.h

Abstract:

Notes: 


--*/
#ifndef TAPICOMN_H
#define TAPICOMN_H

#ifdef __cplusplus
extern "C" {
#endif

//
// typedefs for TSPI functions
//

//
// Line functions
//
typedef
LONG
(TSPIAPI *
PFN_TSPI_lineAccept)(
    DRV_REQUESTID dwRequestID,
    HDRVCALL hdCall,
    LPCSTR lpsUserUserInfo,
    DWORD dwSize
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_lineAddToConference)(
    DRV_REQUESTID dwRequestID,
    HDRVCALL hdConfCall,
    HDRVCALL hdConsultCall
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_lineAnswer)(
    DRV_REQUESTID dwRequestID,
    HDRVCALL hdCall,
    LPCSTR lpsUserUserInfo,
    DWORD dwSize
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_lineBlindTransfer)(
    DRV_REQUESTID dwRequestID,
    HDRVCALL hdCall,
    LPCWSTR lpszDestAddress,
    DWORD dwCountryCode
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_lineClose)(
    HDRVLINE hdLine
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_lineCloseCall)(
    HDRVCALL hdCall
    );

typedef
LONG
(TSPIAPI * PFN_TSPI_lineCompleteCall) (
    DRV_REQUESTID  dwRequestID,
    HDRVCALL       hdCall,
    LPDWORD        lpdwCompletionID,
    DWORD          dwCompletionMode,
    DWORD          dwMessageID
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_lineCompleteTransfer)(
    DRV_REQUESTID dwRequestID,
    HDRVCALL hdCall,
    HDRVCALL hdConsultCall,
    HTAPICALL htConfCall,
    LPHDRVCALL lphdConfCall,
    DWORD dwTransferMode
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_lineConditionalMediaDetection)(
    HDRVLINE hdLine,
    DWORD dwMediaModes,
    LPLINECALLPARAMS const lpCallParams
    );

typedef    
LONG
(TSPIAPI *
 PFN_TSPI_lineConfigDialog) (
    DWORD   dwDeviceID,
    HWND    hwndOwner,
    LPCTSTR  lpszDeviceClass
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_lineConfigDialogEdit)(
    DWORD dwDeviceID,
    HWND hwndOwner,
    LPCWSTR lpszDeviceClass,
    LPVOID const lpDeviceConfigIn,
    DWORD dwSize,
    LPVARSTRING lpDeviceConfigOut
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_lineDevSpecific)(
    DRV_REQUESTID dwRequestID,
    HDRVLINE hdLine,
    DWORD dwAddressID,
    HDRVCALL hdCall,
    LPVOID lpParams,
    DWORD dwSize
    );

typedef    
LONG
(TSPIAPI *
PFN_TSPI_lineDevSpecificFeature) (
    DRV_REQUESTID  dwRequestID,
    HDRVLINE       hdLine,
    DWORD          dwFeature,
    LPVOID         lpParams,
    DWORD          dwSize
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_lineDial)(
    DRV_REQUESTID dwRequestID,
    HDRVCALL hdCall,
    LPCWSTR lpszDestAddress,
    DWORD dwCountryCode
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_lineDrop)(
    DRV_REQUESTID dwRequestID,
    HDRVCALL hdCall,
    LPCSTR lpsUserUserInfo,
    DWORD dwSize
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_lineDropOnClose) (
    HDRVCALL hdCall
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_lineDropNoOwner) (
    HDRVCALL hdCall
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_lineForward)(
    DRV_REQUESTID dwRequestID,
    HDRVLINE hdLine,
    DWORD bAllAddresses,
    DWORD dwAddressID,
    LPLINEFORWARDLIST const lpForwardList,
    DWORD dwNumRingsNoAnswer,
    HTAPICALL htConsultCall,
    LPHDRVCALL lphdConsultCall,
    LPLINECALLPARAMS const lpCallParams
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_lineGatherDigits) (
    HDRVCALL hdCall,
    DWORD    dwEndToEndID,
    DWORD    dwDigitModes,
    LPSTR    lpsDigits,
    DWORD    dwNumDigits,
    LPCTSTR  lpszTerminationDigits,
    DWORD    dwFirstDigitTimeout,
    DWORD    InterDigitTimeout
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_lineGenerateDigits)(
    HDRVCALL hdCall,
    DWORD dwEndToEndID,
    DWORD dwDigitMode,
    LPCWSTR lpszDigits,
    DWORD dwDuration
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_lineGenerateTone)(
    HDRVCALL hdCall,
    DWORD dwEndToEndID,
    DWORD dwToneMode,
    DWORD dwDuration,
    DWORD dwNumTones,
    LPLINEGENERATETONE const lpTones
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_lineGetAddressCaps)(
    DWORD dwDeviceID,
    DWORD dwAddressID,
    DWORD dwTSPIVersion,
    DWORD dwExtVersion,
    LPLINEADDRESSCAPS lpAddressCaps
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_lineGetAddressID)(
    HDRVLINE hdLine,
    LPDWORD lpdwAddressID,
    DWORD dwAddressMode,
    LPCWSTR lpsAddress,
    DWORD dwSize
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_lineGetAddressStatus)(
    HDRVLINE hdLine,
    DWORD dwAddressID,
    LPLINEADDRESSSTATUS lpAddressStatus
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_lineGetCallAddressID)(
    HDRVCALL hdCall,
    LPDWORD lpdwAddressID
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_lineGetCallInfo)(
    HDRVCALL hdCall,
    LPLINECALLINFO lpCallInfo
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_lineGetCallStatus)(
    HDRVCALL hdCall,
    LPLINECALLSTATUS lpCallStatus
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_lineGetDevCaps)(
    DWORD dwDeviceID,
    DWORD dwTSPIVersion,
    DWORD dwExtVersion,
    LPLINEDEVCAPS lpLineDevCaps
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_lineGetDevConfig)(
    DWORD dwDeviceID,
    LPVARSTRING lpDeviceConfig,
    LPCWSTR lpszDeviceClass
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_lineGetExtensionID)(
    DWORD dwDeviceID,
    DWORD dwTSPIVersion,
    LPLINEEXTENSIONID lpExtensionID
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_lineGetIcon)(
    DWORD dwDeviceID,
    LPCWSTR lpszDeviceClass,
    LPHICON lphIcon
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_lineGetID)(
    HDRVLINE hdLine,
    DWORD dwAddressID,
    HDRVCALL hdCall,
    DWORD dwSelect,
    LPVARSTRING lpDeviceID,
    LPCWSTR lpszDeviceClass
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_lineGetLineDevStatus)(
    HDRVLINE hdLine,
    LPLINEDEVSTATUS lpLineDevStatus
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_lineGetNumAddressIDs)(
    HDRVLINE hdLine,
    LPDWORD lpdwNumAddressIDs
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_lineHold)(
    DRV_REQUESTID dwRequestID,
    HDRVCALL hdCall
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_lineMakeCall)(
    DRV_REQUESTID dwRequestID,
    HDRVLINE hdLine,
    HTAPICALL htCall,
    LPHDRVCALL lphdCall,
    LPCWSTR lpszDestAddress,
    DWORD dwCountryCode,
    LPLINECALLPARAMS const lpCallParams
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_lineMonitorDigits)(
    HDRVCALL hdCall,
    DWORD dwDigitModes
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_lineMonitorMedia)(
    HDRVCALL hdCall,
    DWORD dwMediaModes
    );

typedef    
LONG
(TSPIAPI *
PFN_TSPI_lineMonitorTones) (
    HDRVCALL           hdCall,
    DWORD              dwToneListID,
    LPLINEMONITORTONE  const lpToneList,
    DWORD              dwNumEntries
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_lineNegotiateExtVersion)(
    DWORD dwDeviceID,
    DWORD dwTSPIVersion,
    DWORD dwLowVersion,
    DWORD dwHighVersion,
    LPDWORD lpdwExtVersion
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_lineNegotiateTSPIVersion)(
    DWORD dwDeviceID,
    DWORD dwLowVersion,
    DWORD dwHighVersion,
    LPDWORD lpdwTSPIVersion
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_lineOpen)(
    DWORD dwDeviceID,
    HTAPILINE htLine,
    LPHDRVLINE lphdLine,
    DWORD dwTSPIVersion,
    LINEEVENT lineEventProc
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_linePark) (
    DRV_REQUESTID  dwRequestID,
    HDRVCALL       hdCall,
    DWORD          dwParkMode,
    LPCTSTR        lpszDirAddress,
    LPVARSTRING    lpNonDirAddress
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_linePickup)(
    DRV_REQUESTID dwRequestID,
    HDRVLINE hdLine,
    DWORD dwAddressID,
    HTAPICALL htCall,
    LPHDRVCALL lphdCall,
    LPCWSTR lpszDestAddress,
    LPCWSTR lpszGroupID
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_linePrepareAddToConference)(
    DRV_REQUESTID dwRequestID,
    HDRVCALL hdConfCall,
    HTAPICALL htConsultCall,
    LPHDRVCALL lphdConsultCall,
    LPLINECALLPARAMS const lpCallParams
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_lineReleaseUserUserInfo)(
    DRV_REQUESTID dwRequestID,
    HDRVCALL hdCall
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_lineRedirect)(
    DRV_REQUESTID dwRequestID,
    HDRVCALL hdCall,
    LPCWSTR lpszDestAddress,
    DWORD dwCountryCode
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_lineRemoveFromConference)(
    DRV_REQUESTID dwRequestID,
    HDRVCALL hdCall
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_lineSecureCall) (
    DRV_REQUESTID  dwRequestID,
    HDRVCALL       hdCall
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_lineSelectExtVersion)(
    HDRVLINE hdLine,
    DWORD dwExtVersion
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_lineSendUserUserInfo)(
    DRV_REQUESTID dwRequestID,
    HDRVCALL hdCall,
    LPCSTR lpsUserUserInfo,
    DWORD dwSize
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_lineSetAppSpecific) (
    HDRVCALL hdCall,
    DWORD    dwAppSpecific
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_lineSetCallParams)(
    DRV_REQUESTID dwRequestID,
    HDRVCALL hdCall,
    DWORD dwBearerMode,
    DWORD dwMinRate,
    DWORD dwMaxRate,
    LPLINEDIALPARAMS const lpDialParams
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_lineSetCurrentLocation)(
    DWORD dwLocation
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_lineSetDefaultMediaDetection)(
    HDRVLINE hdLine,
    DWORD dwMediaModes
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_lineSetDevConfig)(
    DWORD dwDeviceID,
    LPVOID const lpDeviceConfig,
    DWORD dwSize,
    LPCWSTR lpszDeviceClass
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_lineSetMediaControl) (
    HDRVLINE                     hdLine,
    DWORD                        dwAddressID,
    HDRVCALL                     hdCall,
    DWORD                        dwSelect,
    LPLINEMEDIACONTROLDIGIT      const lpDigitList,
    DWORD                        dwDigitNumEntries,
    LPLINEMEDIACONTROLMEDIA      const lpMediaList,
    DWORD                        dwMediaNumEntries,
    LPLINEMEDIACONTROLTONE       const lpToneList,
    DWORD                        dwToneNumEntries,
    LPLINEMEDIACONTROLCALLSTATE  const lpCallStateList,
    DWORD                        dwCallStateNumEntries
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_lineSetMediaMode)(
    HDRVCALL hdCall,
    DWORD dwMediaMode
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_lineSetStatusMessages)(
    HDRVLINE hdLine,
    DWORD dwLineStates,
    DWORD dwAddressStates
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_lineSetTerminal)(
    DRV_REQUESTID dwRequestID,
    HDRVLINE hdLine,
    DWORD dwAddressID,
    HDRVCALL hdCall,
    DWORD dwSelect,
    DWORD dwTerminalModes,
    DWORD dwTerminalID,
    DWORD bEnable
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_lineSetupConference)(
    DRV_REQUESTID dwRequestID,
    HDRVCALL hdCall,
    HDRVLINE hdLine,
    HTAPICALL htConfCall,
    LPHDRVCALL lphdConfCall,
    HTAPICALL htConsultCall,
    LPHDRVCALL lphdConsultCall,
    DWORD dwNumParties,
    LPLINECALLPARAMS const lpCallParams
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_lineSetupTransfer)(
    DRV_REQUESTID dwRequestID,
    HDRVCALL hdCall,
    HTAPICALL htConsultCall,
    LPHDRVCALL lphdConsultCall,
    LPLINECALLPARAMS const lpCallParams
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_lineSwapHold)(
    DRV_REQUESTID dwRequestID,
    HDRVCALL hdActiveCall,
    HDRVCALL hdHeldCall
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_lineUncompleteCall) (
    DRV_REQUESTID  dwRequestID,
    HDRVLINE       hdLine,
    DWORD          dwCompletionID
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_lineUnhold)(
    DRV_REQUESTID dwRequestID,
    HDRVCALL      hdCall
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_lineUnpark) (
    DRV_REQUESTID  dwRequestID,
    HDRVLINE       hdLine,
    DWORD          dwAddressID,
    HTAPICALL      htCall,
    LPHDRVCALL     lphdCall,
    LPCTSTR        lpszDestAddress
    );


//
// Phone functions
//
typedef
LONG
(TSPIAPI *
PFN_TSPI_phoneClose)(
    HDRVPHONE hdPhone
    );


typedef
LONG
(TSPIAPI *
PFN_TSPI_phoneConfigDialog)(
    DWORD dwDeviceID,
    HWND hwndOwner,
    LPCWSTR lpszDeviceClass
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_phoneDevSpecific)(
    DRV_REQUESTID dwRequestID,
    HDRVPHONE     hdPhone,
    LPVOID        lpParams,
    DWORD         dwSize
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_phoneGetDevCaps)(
    DWORD       dwDeviceID,
    DWORD       dwTSPIVersion,
    DWORD       dwExtVersion,
    LPPHONECAPS lpPhoneCaps
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_phoneGetExtensionID)(
    DWORD dwDeviceID,
    DWORD dwTSPIVersion,
    LPPHONEEXTENSIONID lpExtensionID
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_phoneGetGain)(
    HDRVPHONE hdPhone,
    DWORD     dwHookSwitchDev,
    LPDWORD   lpdwGain
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_phoneGetHookSwitch)(
    HDRVPHONE hdPhone,
    LPDWORD   lpdwHookSwitchDevs
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_phoneGetIcon)(
    DWORD   dwDeviceID,
    LPCWSTR lpszDeviceClass,
    LPHICON lphIcon
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_phoneGetID)(
    HDRVPHONE   hdPhone,
    LPVARSTRING lpDeviceID,
    LPCWSTR     lpszDeviceClass
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_phoneGetRing)(
    HDRVPHONE hdPhone,
    LPDWORD   lpdwRingMode,
    LPDWORD   lpdwVolume
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_phoneGetStatus)(
    HDRVPHONE     hdPhone,
    LPPHONESTATUS lpPhoneStatus
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_phoneGetVolume)(
    HDRVPHONE hdPhone,
    DWORD     dwHookSwitchDev,
    LPDWORD   lpdwVolume
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_phoneNegotiateExtVersion)(
    DWORD dwDeviceID,
    DWORD dwAPIVersion,
    DWORD dwExtLowVersion,
    DWORD dwExtHighVersion,
    LPDWORD lpdwExtVersion
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_phoneNegotiateTSPIVersion)(
    DWORD   dwDeviceID,
    DWORD   dwLowVersion,
    DWORD   dwHighVersion,
    LPDWORD lpdwTSPIVersion
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_phoneOpen)(
    DWORD       dwDeviceID,
    HTAPIPHONE  htPhone,
    LPHDRVPHONE lphdPhone,
    DWORD       dwTSPIVersion,
    PHONEEVENT  lpfnEventProc
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_phoneSelectExtVersion)(
    HDRVPHONE hdPhone,
    DWORD     dwExtVersion
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_phoneSetGain)(
    DRV_REQUESTID dwRequestID,
    HDRVPHONE     hdPhone,
    DWORD         dwHookSwitchDev,
    DWORD         dwGain
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_phoneSetHookSwitch)(
    DRV_REQUESTID dwRequestID,
    HDRVPHONE     hdPhone,
    DWORD         dwHookSwitchDevs,
    DWORD         dwHookSwitchMode
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_phoneSetRing)(
    DRV_REQUESTID dwRequestID,
    HDRVPHONE     hdPhone,
    DWORD         dwRingMode,
    DWORD         dwVolume
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_phoneSetStatusMessages)(
    HDRVPHONE hdPhone,
    DWORD     dwPhoneStates,
    DWORD     dwButtonModes,
    DWORD     dwButtonStates
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_phoneSetVolume)(
    DRV_REQUESTID dwRequestID,
    HDRVPHONE     hdPhone,
    DWORD         dwHookSwitchDev,
    DWORD         dwVolume
    );

//
// Provider functions
//
typedef
LONG
(TSPIAPI *
PFN_TSPI_providerInstall)(
    HWND   hwndOwner,
    DWORD  dwPermanentProviderID
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_providerConfig)(
    HWND hwndOwner,
    DWORD dwPermanentProviderID
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_providerCreateLineDevice)(
    HKEY hActiveKey,
    LPCWSTR lpszDevPath,
    LPCWSTR lpszDeviceName
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_providerCreatePhoneDevice)(
    HKEY hActiveKey,
    LPCWSTR lpszDevPath,
    LPCWSTR lpszDeviceName
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_providerDeleteLineDevice)(
    DWORD dwIdentifier
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_providerDeletePhoneDevice)(
    DWORD dwIdentifier
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_providerEnumDevices)(
    DWORD dwPermanentProviderID,
    LPDWORD lpdwNumLines,
    LPDWORD lpdwNumPhones,
    HPROVIDER hProvider,
    LINEEVENT lpfnLineCreateProc,
    PHONEEVENT lpfnPhoneCreateProc
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_providerInit)(
    DWORD dwTSPIVersion,
    DWORD dwPermanentProviderID,
    DWORD dwLineDeviceIDBase,
    DWORD dwPhoneDeviceIDBase,
    DWORD dwNumLines,
    DWORD dwNumPhones,
    ASYNC_COMPLETION cbCompletionProc,
    LPDWORD lpdwTSPIOptions
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_providerShutdown)(
    DWORD dwTSPIVersion
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_providerRemove) (
    HWND   hwndOwner,
    DWORD  dwPermanentProviderID
    );

typedef
LONG
(TSPIAPI *
PFN_TSPI_providerRemoveDevice)(
    LPCWSTR lpszDeviceName
    );

typedef LONG (TSPIAPI * TSPI_LINEGETPROCTABLE)(void *lplpTspiProcs);

//
// vtbl for the tspi functions.
//
typedef struct tag_TSPI_PROCS {
    //
    // Old functions
    //
    PFN_TSPI_lineAccept                     TSPI_lineAccept;
    PFN_TSPI_lineAddToConference            TSPI_lineAddToConference;
    PFN_TSPI_lineAnswer                     TSPI_lineAnswer;
    PFN_TSPI_lineBlindTransfer              TSPI_lineBlindTransfer;
    PFN_TSPI_lineClose                      TSPI_lineClose;
    PFN_TSPI_lineCloseCall                  TSPI_lineCloseCall;
    PFN_TSPI_lineCompleteCall               TSPI_lineCompleteCall;
    PFN_TSPI_lineCompleteTransfer           TSPI_lineCompleteTransfer;
    PFN_TSPI_lineConditionalMediaDetection  TSPI_lineConditionalMediaDetection;
    PFN_TSPI_lineConfigDialog               TSPI_lineConfigDialog;
    PFN_TSPI_lineDevSpecific                TSPI_lineDevSpecific;
    PFN_TSPI_lineDevSpecificFeature         TSPI_lineDevSpecificFeature;
    PFN_TSPI_lineDial                       TSPI_lineDial;
    PFN_TSPI_lineDrop                       TSPI_lineDrop;
    PFN_TSPI_lineForward                    TSPI_lineForward;
    PFN_TSPI_lineGatherDigits               TSPI_lineGatherDigits;
    PFN_TSPI_lineGenerateDigits             TSPI_lineGenerateDigits;
    PFN_TSPI_lineGenerateTone               TSPI_lineGenerateTone;
    PFN_TSPI_lineGetAddressCaps             TSPI_lineGetAddressCaps;
    PFN_TSPI_lineGetAddressID               TSPI_lineGetAddressID;
    PFN_TSPI_lineGetAddressStatus           TSPI_lineGetAddressStatus;
    PFN_TSPI_lineGetCallAddressID           TSPI_lineGetCallAddressID;
    PFN_TSPI_lineGetCallInfo                TSPI_lineGetCallInfo;
    PFN_TSPI_lineGetCallStatus              TSPI_lineGetCallStatus;
    PFN_TSPI_lineGetDevCaps                 TSPI_lineGetDevCaps;
    PFN_TSPI_lineGetDevConfig               TSPI_lineGetDevConfig;
    PFN_TSPI_lineGetExtensionID             TSPI_lineGetExtensionID;
    PFN_TSPI_lineGetIcon                    TSPI_lineGetIcon;
    PFN_TSPI_lineGetID                      TSPI_lineGetID;
    PFN_TSPI_lineGetLineDevStatus           TSPI_lineGetLineDevStatus;
    PFN_TSPI_lineGetNumAddressIDs           TSPI_lineGetNumAddressIDs;
    PFN_TSPI_lineHold                       TSPI_lineHold;
    PFN_TSPI_lineMakeCall                   TSPI_lineMakeCall;
    PFN_TSPI_lineMonitorDigits              TSPI_lineMonitorDigits;
    PFN_TSPI_lineMonitorMedia               TSPI_lineMonitorMedia;
    PFN_TSPI_lineMonitorTones               TSPI_lineMonitorTones;
    PFN_TSPI_lineNegotiateExtVersion        TSPI_lineNegotiateExtVersion;
    PFN_TSPI_lineNegotiateTSPIVersion       TSPI_lineNegotiateTSPIVersion;
    PFN_TSPI_lineOpen                       TSPI_lineOpen;
    PFN_TSPI_linePark                       TSPI_linePark;
    PFN_TSPI_linePickup                     TSPI_linePickup;
    PFN_TSPI_linePrepareAddToConference     TSPI_linePrepareAddToConference;
    PFN_TSPI_lineRedirect                   TSPI_lineRedirect;
    PFN_TSPI_lineRemoveFromConference       TSPI_lineRemoveFromConference;
    PFN_TSPI_lineSecureCall                 TSPI_lineSecureCall;
    PFN_TSPI_lineSelectExtVersion           TSPI_lineSelectExtVersion;
    PFN_TSPI_lineSendUserUserInfo           TSPI_lineSendUserUserInfo;
    PFN_TSPI_lineSetAppSpecific             TSPI_lineSetAppSpecific;
    PFN_TSPI_lineSetCallParams              TSPI_lineSetCallParams;
    PFN_TSPI_lineSetDefaultMediaDetection   TSPI_lineSetDefaultMediaDetection;
    PFN_TSPI_lineSetDevConfig               TSPI_lineSetDevConfig;
    PFN_TSPI_lineSetMediaControl            TSPI_lineSetMediaControl;
    PFN_TSPI_lineSetMediaMode               TSPI_lineSetMediaMode;
    PFN_TSPI_lineSetStatusMessages          TSPI_lineSetStatusMessages;
    PFN_TSPI_lineSetTerminal                TSPI_lineSetTerminal;
    PFN_TSPI_lineSetupConference            TSPI_lineSetupConference;
    PFN_TSPI_lineSetupTransfer              TSPI_lineSetupTransfer;
    PFN_TSPI_lineSwapHold                   TSPI_lineSwapHold;
    PFN_TSPI_lineUncompleteCall             TSPI_lineUncompleteCall;
    PFN_TSPI_lineUnhold                     TSPI_lineUnhold;
    PFN_TSPI_lineUnpark                     TSPI_lineUnpark;

    PFN_TSPI_providerConfig                 TSPI_providerConfig;
    PFN_TSPI_providerInit                   TSPI_providerInit;
    PFN_TSPI_providerInstall                TSPI_providerInstall;
    PFN_TSPI_providerRemove                 TSPI_providerRemove;
    PFN_TSPI_providerShutdown               TSPI_providerShutdown;
    PFN_TSPI_providerEnumDevices            TSPI_providerEnumDevices;

    PFN_TSPI_lineDropOnClose                TSPI_lineDropOnClose;
    PFN_TSPI_lineDropNoOwner                TSPI_lineDropNoOwner;

    PFN_TSPI_providerCreateLineDevice       TSPI_providerCreateLineDevice;
    PFN_TSPI_providerDeleteLineDevice       TSPI_providerDeleteLineDevice;

    PFN_TSPI_lineSetCurrentLocation         TSPI_lineSetCurrentLocation;
    PFN_TSPI_lineConfigDialogEdit           TSPI_lineConfigDialogEdit;
    PFN_TSPI_lineReleaseUserUserInfo        TSPI_lineReleaseUserUserInfo;

    //
    // New functions
    //

    PFN_TSPI_phoneClose                     TSPI_phoneClose;
    PFN_TSPI_phoneConfigDialog              TSPI_phoneConfigDialog;
    PFN_TSPI_phoneDevSpecific               TSPI_phoneDevSpecific;
    PFN_TSPI_phoneGetDevCaps                TSPI_phoneGetDevCaps;
    PFN_TSPI_phoneGetExtensionID            TSPI_phoneGetExtensionID;
    PFN_TSPI_phoneGetGain                   TSPI_phoneGetGain;
    PFN_TSPI_phoneGetHookSwitch             TSPI_phoneGetHookSwitch;
    PFN_TSPI_phoneGetIcon                   TSPI_phoneGetIcon;
    PFN_TSPI_phoneGetID                     TSPI_phoneGetID;
    PFN_TSPI_phoneGetRing                   TSPI_phoneGetRing;
    PFN_TSPI_phoneGetStatus                 TSPI_phoneGetStatus;
    PFN_TSPI_phoneGetVolume                 TSPI_phoneGetVolume;
    PFN_TSPI_phoneNegotiateExtVersion       TSPI_phoneNegotiateExtVersion;
    PFN_TSPI_phoneNegotiateTSPIVersion      TSPI_phoneNegotiateTSPIVersion;
    PFN_TSPI_phoneOpen                      TSPI_phoneOpen;
    PFN_TSPI_phoneSelectExtVersion          TSPI_phoneSelectExtVersion;
    PFN_TSPI_phoneSetGain                   TSPI_phoneSetGain;
    PFN_TSPI_phoneSetHookSwitch             TSPI_phoneSetHookSwitch;
    PFN_TSPI_phoneSetRing                   TSPI_phoneSetRing;
    PFN_TSPI_phoneSetStatusMessages         TSPI_phoneSetStatusMessages;
    PFN_TSPI_phoneSetVolume                 TSPI_phoneSetVolume;

    PFN_TSPI_providerCreatePhoneDevice      TSPI_providerCreatePhoneDevice;
    PFN_TSPI_providerDeletePhoneDevice      TSPI_providerDeletePhoneDevice;
    PFN_TSPI_providerRemoveDevice           TSPI_providerRemoveDevice;
} TSPI_PROCS;

typedef TSPI_PROCS * LPTSPI_PROCS;

typedef struct {
    HINSTANCE         provider_instance;
    DWORD             dwNegotiatedTSPIVersion;
    DWORD             dwProviderPermanentID;
    TSPI_PROCS        tspi_procs;          
} TAPIDRIVER_INFO;

typedef TAPIDRIVER_INFO * LPTAPIDRIVER_INFO;
typedef LPTAPIDRIVER_INFO * LPLPTAPIDRIVER_INFO;

// The following macros are used to identify the type of the handle in the
// Asynchronous request table. They are also used at runtime to check if the
// APP passed the correct handle type to TAPI.
#define ANY_RT_HLINE           202
#define ANY_RT_HLINEAPP        203
#define ANY_RT_HCALL           204 
#define ANY_RT_HPHONE          205
#define ANY_RT_HPHONEAPP       206

#define TAPI_SUCCESS_RETURN    0L

#define IS_OPERATION_SYNCH(retval)  ( ((retval) > 0) ? FALSE : TRUE )  
 // Used to find out if an operation is synchronous or not

// The following macros are useful for indicating the completion type of the
// various TSPI functions.
#define OPERATION_IS_SYNCH   1
#define OPERATION_IS_ASYNCH  2

#define IsOnlyOneBitSetInWORD(target)    (! ( LOWORD((target)) & (LOWORD((target)) - 1) ))
// This macro returns TRUE iff one bit is set in the target parameter. If no 
// bit is set or more than one bits are set, then the function returns a FALSE.

#define IsOnlyOneBitSetInDWORD(target)    (! ( ((DWORD)(target)) & (((DWORD)(target)) - 1) ))

#define ALL_Fs             0xFFFFFFFF
#define WORD_Fs            0x0000FFFF
#undef LOWORD
#define LOWORD(l)          ((WORD)(((DWORD)(l)) & WORD_Fs))

// The following macro is used to find out if a selection of bits is part of
#define IS_INVAL_TAPI_WORD_MODE(bitarray, selection)       \
((!(LOWORD((selection)))) ||       \
 ((LOWORD(~(bitarray))) & (selection)))

#define IS_INVAL_TAPI_DWORD_MODE(bitarray, selection)       \
((!((selection))) ||       \
 ((~(bitarray)) & (selection)))

#define IS_INVAL_EXCL_TAPI_WORD_MODE(bitarray, selection)    \
(!IsOnlyOneBitSetInWORD((selection)) ||    \
 IS_INVAL_TAPI_WORD_MODE( (bitarray), (selection)))
                                
#define IS_INVAL_EXCL_TAPI_DWORD_MODE(bitarray, selection)  \
(!IsOnlyOneBitSetInDWORD((selection)) ||   \
 IS_INVAL_TAPI_DWORD_MODE( (bitarray), (selection)))

#define IS_INVAL_EXCL_TAPI_EXTN_WORD_MODE(bitarray, selection)  \
((!(IsOnlyOneBitSetInDWORD((selection)))) || \
 (LOWORD((selection)) &&                      \
  (IS_INVAL_EXCL_TAPI_WORD_MODE((bitarray),(selection)))))




#define FExtendedMediaMode(dwMediaMode) \
((dwMediaMode >= 0x01000000) && (dwMediaMode <= 0x80000000))
#define FOneBitOn(dw) IsOnlyOneBitSetInDWORD(dw)

#define FLineMediaMode(dwMediaMode) \
(       dwMediaMode == LINEMEDIAMODE_UNKNOWN                \
        || dwMediaMode == LINEMEDIAMODE_INTERACTIVEVOICE    \
        || dwMediaMode == LINEMEDIAMODE_AUTOMATEDVOICE      \
        || dwMediaMode == LINEMEDIAMODE_DATAMODEM               \
        || dwMediaMode == LINEMEDIAMODE_G3FAX                   \
        || dwMediaMode == LINEMEDIAMODE_TDD                     \
        || dwMediaMode == LINEMEDIAMODE_G4FAX                   \
        || dwMediaMode == LINEMEDIAMODE_DIGITALDATA         \
        || dwMediaMode == LINEMEDIAMODE_TELETEX             \
        || dwMediaMode == LINEMEDIAMODE_VIDEOTEX                \
        || dwMediaMode == LINEMEDIAMODE_MIXED                   \
        || dwMediaMode == LINEMEDIAMODE_ADSI                    \
        || dwMediaMode == LINEMEDIAMODE_VOICEVIEW)

#ifdef __cplusplus
}
#endif

#endif // TAPICOMN_H
