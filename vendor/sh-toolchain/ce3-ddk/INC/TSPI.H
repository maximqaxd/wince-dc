/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

Module Name:  

tspi.h

Abstract:

Windows CE version of TSPI.h

Notes: 


--*/
// The  Telephony  API  is jointly copyrighted by Intel and Microsoft.  You are
// granted  a royalty free worldwide, unlimited license to make copies, and use
// the   API/SPI  for  making  applications/drivers  that  interface  with  the
// specification provided that this paragraph and the Intel/Microsoft copyright
// statement is maintained as is in the text and source code files.
//
// Copyright 1992, 1993 Intel/Microsoft, all rights reserved.

#ifndef TSPI_H
#define TSPI_H


#include <windows.h>

#include "tapi.h"

// tspi.h  is  only  of  use  in  conjunction  with tapi.h.  Very few types are
// defined  in  tspi.h.   Most  types of procedure formal parameters are simply
// passed through from corresponding procedures in tapi.h.  A working knowledge
// of the TAPI interface is required for an understanding of this interface.

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

#ifndef DECLARE_OPAQUE32
#define DECLARE_OPAQUE32(name)  struct name##__ { int unused; }; \
                                typedef const struct name##__ FAR* name
#endif  // DECLARE_OPAQUE32

#ifndef TSPIAPI
# ifdef UNDER_CE
# define TSPIAPI
# else
#  define TSPIAPI __export __pascal
# endif                                    
#endif
 
DECLARE_OPAQUE32(HDRVCALL);
DECLARE_OPAQUE32(HDRVLINE);
DECLARE_OPAQUE32(HDRVPHONE);

typedef HDRVCALL * LPHDRVCALL;
typedef HDRVLINE * LPHDRVLINE;
typedef HDRVPHONE FAR * LPHDRVPHONE;

DECLARE_OPAQUE32(HTAPICALL);
DECLARE_OPAQUE32(HTAPILINE);
DECLARE_OPAQUE32(HTAPIPHONE);

typedef HTAPICALL * LPHTAPICALL;
typedef HTAPILINE * LPHTAPILINE;
typedef HTAPIPHONE FAR * LPHTAPIPHONE;

DECLARE_OPAQUE32(HPROVIDER);
typedef HPROVIDER * LPHPROVIDER;



typedef void (CALLBACK * LINEEVENT) (
    HTAPILINE   htLine,
    HTAPICALL   htCall,
    DWORD       dwMsg,
    DWORD       dwParam1,
    DWORD       dwParam2,
    DWORD       dwParam3);

typedef void (CALLBACK * PHONEEVENT)(
    HTAPIPHONE  htPhone,
    DWORD       dwMsg,
    DWORD       dwParam1,
    DWORD       dwParam2,
    DWORD       dwParam3
    );


#define TSPI_MESSAGE_BASE 500
    // The lowest-numbered TSPI-specific message ID number

#define LINE_NEWCALL                ((long) TSPI_MESSAGE_BASE + 0)
#define LINE_CALLDEVSPECIFIC        ((long) TSPI_MESSAGE_BASE + 1)
#define LINE_CALLDEVSPECIFICFEATURE ((long) TSPI_MESSAGE_BASE + 2)

typedef DWORD DRV_REQUESTID;

typedef void (CALLBACK * ASYNC_COMPLETION) (
    DRV_REQUESTID  dwRequestID,
    LONG           lResult);


// ----------------------------------------------------------------------------
// TSPIAPI TSPI_line functions
// ----------------------------------------------------------------------------
    
LONG TSPIAPI TSPI_lineAccept(
    DRV_REQUESTID  dwRequestID,
    HDRVCALL       hdCall,
    LPCSTR         lpsUserUserInfo,
    DWORD          dwSize);

LONG TSPIAPI TSPI_lineAddToConference(
    DRV_REQUESTID  dwRequestID,
    HDRVCALL       hdConfCall,
    HDRVCALL       hdConsultCall);

LONG TSPIAPI TSPI_lineAnswer(
    DRV_REQUESTID  dwRequestID,
    HDRVCALL       hdCall,
    LPCSTR         lpsUserUserInfo,
    DWORD          dwSize);

LONG TSPIAPI TSPI_lineBlindTransfer(
    DRV_REQUESTID  dwRequestID,
    HDRVCALL       hdCall,
    LPCWSTR         lpszDestAddress,
    DWORD          dwCountryCode);

LONG TSPIAPI TSPI_lineClose(
    HDRVLINE  hdLine);

LONG TSPIAPI TSPI_lineCloseCall(
    HDRVCALL  hdCall);

LONG TSPIAPI TSPI_lineCompleteTransfer(
    DRV_REQUESTID  dwRequestID,
    HDRVCALL       hdCall,
    HDRVCALL       hdConsultCall,
    HTAPICALL      htConfCall,
    LPHDRVCALL     lphdConfCall,
    DWORD          dwTransferMode);

LONG TSPIAPI TSPI_lineConditionalMediaDetection(
    HDRVLINE          hdLine,
    DWORD             dwMediaModes,
    LPLINECALLPARAMS  const lpCallParams);

LONG TSPIAPI TSPI_lineConfigDialogEdit(
    DWORD dwDeviceID,
    HWND hwndOwner, 
    LPCWSTR lpszDeviceClass,
    LPVOID const lpDeviceConfigIn, 
    DWORD dwSize,
    LPVARSTRING lpDeviceConfigOut
    );

LONG TSPIAPI TSPI_lineDevSpecific(
    DRV_REQUESTID  dwRequestID,
    HDRVLINE       hdLine,
    DWORD          dwAddressID,
    HDRVCALL       hdCall,
    LPVOID         lpParams,
    DWORD          dwSize);

LONG TSPIAPI TSPI_lineDial(
    DRV_REQUESTID  dwRequestID,
    HDRVCALL       hdCall,
    LPCWSTR         lpszDestAddress,
    DWORD          dwCountryCode);

LONG TSPIAPI TSPI_lineDrop(
    DRV_REQUESTID  dwRequestID,
    HDRVCALL       hdCall,
    LPCSTR         lpsUserUserInfo,
    DWORD          dwSize);

LONG TSPIAPI TSPI_lineForward(
    DRV_REQUESTID     dwRequestID,
    HDRVLINE          hdLine,
    DWORD             bAllAddresses,
    DWORD             dwAddressID,
    LPLINEFORWARDLIST const lpForwardList,
    DWORD             dwNumRingsNoAnswer,
    HTAPICALL         htConsultCall,
    LPHDRVCALL        lphdConsultCall,
    LPLINECALLPARAMS  const lpCallParams);

LONG TSPIAPI TSPI_lineGenerateDigits(
    HDRVCALL       hdCall,
    DWORD          dwEndToEndID,
    DWORD          dwDigitMode,
    LPCWSTR         lpszDigits,
    DWORD          dwDuration);

LONG TSPIAPI TSPI_lineGenerateTone(
    HDRVCALL            hdCall,
    DWORD               dwEndToEndID,
    DWORD               dwToneMode,
    DWORD               dwDuration,
    DWORD               dwNumTones,
    LPLINEGENERATETONE  const lpTones);

LONG TSPIAPI TSPI_lineGetAddressCaps(
    DWORD              dwDeviceID,
    DWORD              dwAddressID,
    DWORD              dwTSPIVersion,
    DWORD              dwExtVersion,
    LPLINEADDRESSCAPS  lpAddressCaps);

LONG TSPIAPI TSPI_lineGetAddressID(
    HDRVLINE       hdLine,
    LPDWORD        lpdwAddressID,
    DWORD          dwAddressMode,
    LPCWSTR         lpsAddress,
    DWORD          dwSize);

LONG TSPIAPI TSPI_lineGetAddressStatus(
    HDRVLINE             hdLine,
    DWORD                dwAddressID,
    LPLINEADDRESSSTATUS  lpAddressStatus);

LONG TSPIAPI TSPI_lineGetCallAddressID(
    HDRVCALL  hdCall,
    LPDWORD   lpdwAddressID);

LONG TSPIAPI TSPI_lineGetCallInfo(
    HDRVCALL        hdCall,
    LPLINECALLINFO  lpCallInfo);

LONG TSPIAPI TSPI_lineGetCallStatus(
    HDRVCALL          hdCall,
    LPLINECALLSTATUS  lpCallStatus);

LONG TSPIAPI TSPI_lineGetDevCaps(
    DWORD          dwDeviceID,
    DWORD          dwTSPIVersion,
    DWORD          dwExtVersion,
    LPLINEDEVCAPS  lpLineDevCaps);

LONG TSPIAPI TSPI_lineGetDevConfig(
    DWORD dwDeviceID,
    LPVARSTRING lpDeviceConfig,
    LPCWSTR lpszDeviceClass);

LONG TSPIAPI TSPI_lineGetExtensionID(
    DWORD              dwDeviceID,
    DWORD              dwTSPIVersion,
    LPLINEEXTENSIONID  lpExtensionID);

LONG TSPIAPI TSPI_lineGetIcon(
    DWORD    dwDeviceID,
    LPCWSTR   lpszDeviceClass,
    LPHICON  lphIcon);

LONG TSPIAPI TSPI_lineGetID(
    HDRVLINE       hdLine,
    DWORD          dwAddressID,
    HDRVCALL       hdCall,
    DWORD          dwSelect,
    LPVARSTRING    lpDeviceID,
    LPCWSTR        lpszDeviceClass);

LONG TSPIAPI TSPI_lineGetLineDevStatus(
    HDRVLINE         hdLine,
    LPLINEDEVSTATUS  lpLineDevStatus);

LONG TSPIAPI TSPI_lineGetNumAddressIDs(
    HDRVLINE    hdLine,
    LPDWORD     lpdwNumAddressIDs);

LONG TSPIAPI TSPI_lineHold(
    DRV_REQUESTID  dwRequestID,
    HDRVCALL       hdCall);

LONG TSPIAPI TSPI_lineMakeCall(
    DRV_REQUESTID     dwRequestID,
    HDRVLINE          hdLine,
    HTAPICALL         htCall,
    LPHDRVCALL        lphdCall,
    LPCWSTR            lpszDestAddress,
    DWORD             dwCountryCode,
    LPLINECALLPARAMS  const lpCallParams);

LONG TSPIAPI TSPI_lineMonitorDigits(
    HDRVCALL       hdCall,
    DWORD          dwDigitModes);

LONG TSPIAPI TSPI_lineMonitorMedia(
    HDRVCALL       hdCall,
    DWORD          dwMediaModes);

LONG TSPIAPI TSPI_lineNegotiateExtVersion(
    DWORD    dwDeviceID,
    DWORD    dwTSPIVersion,
    DWORD    dwLowVersion,
    DWORD    dwHighVersion,
    LPDWORD  lpdwExtVersion);

LONG TSPIAPI TSPI_lineNegotiateTSPIVersion(
    DWORD    dwDeviceID,
    DWORD    dwLowVersion,
    DWORD    dwHighVersion,
    LPDWORD  lpdwTSPIVersion);

LONG TSPIAPI TSPI_lineOpen(
    DWORD       dwDeviceID,
    HTAPILINE   htLine,
    LPHDRVLINE  lphdLine,
    DWORD       dwTSPIVersion,
    LINEEVENT   lpfnEventProc);

LONG TSPIAPI TSPI_linePickup(
    DRV_REQUESTID  dwRequestID,
    HDRVLINE       hdLine,
    DWORD          dwAddressID,
    HTAPICALL      htCall,
    LPHDRVCALL     lphdCall,
    LPCWSTR         lpszDestAddress,
    LPCWSTR         lpszGroupID);

LONG TSPIAPI TSPI_linePrepareAddToConference(
    DRV_REQUESTID     dwRequestID,
    HDRVCALL          hdConfCall,
    HTAPICALL         htConsultCall,
    LPHDRVCALL        lphdConsultCall,
    LPLINECALLPARAMS  const lpCallParams);

LONG TSPIAPI TSPI_lineRedirect(
    DRV_REQUESTID  dwRequestID,
    HDRVCALL       hdCall,
    LPCWSTR         lpszDestAddress,
    DWORD          dwCountryCode);

LONG TSPIAPI TSPI_lineRemoveFromConference(
    DRV_REQUESTID  dwRequestID,
    HDRVCALL       hdCall);

LONG TSPIAPI TSPI_lineSecureCall(
    DRV_REQUESTID  dwRequestID,
    HDRVCALL       hdCall);

LONG TSPIAPI TSPI_lineSelectExtVersion(
    HDRVLINE  hdLine,
    DWORD     dwExtVersion);

LONG TSPIAPI TSPI_lineSendUserUserInfo(
    DRV_REQUESTID  dwRequestID,
    HDRVCALL       hdCall,
    LPCSTR         lpsUserUserInfo,
    DWORD          dwSize);

LONG TSPIAPI TSPI_lineSetCallParams(
    DRV_REQUESTID     dwRequestID,
    HDRVCALL          hdCall,
    DWORD             dwBearerMode,
    DWORD             dwMinRate,
    DWORD             dwMaxRate,
    LPLINEDIALPARAMS  const lpDialParams);

LONG TSPIAPI TSPI_lineSetDefaultMediaDetection(
    HDRVLINE       hdLine,
    DWORD          dwMediaModes);

LONG TSPIAPI TSPI_lineSetDevConfig(
    DWORD         dwDeviceID,
    LPVOID        const lpDeviceConfig,
    DWORD         dwSize,
    LPCWSTR       lpszDeviceClass);

LONG TSPIAPI TSPI_lineSetMediaMode(
    HDRVCALL       hdCall,
    DWORD          dwMediaMode);

LONG TSPIAPI TSPI_lineSetStatusMessages(
    HDRVLINE       hdLine,
    DWORD          dwLineStates,
    DWORD          dwAddressStates);

LONG TSPIAPI TSPI_lineSetTerminal(
    DRV_REQUESTID  dwRequestID,
    HDRVLINE       hdLine,
    DWORD          dwAddressID,
    HDRVCALL       hdCall,
    DWORD          dwSelect,
    DWORD          dwTerminalModes,
    DWORD          dwTerminalID,
    DWORD           bEnable);

LONG TSPIAPI TSPI_lineSetupConference(
    DRV_REQUESTID     dwRequestID,
    HDRVCALL          hdCall,
    HDRVLINE          hdLine,
    HTAPICALL         htConfCall,
    LPHDRVCALL        lphdConfCall,
    HTAPICALL         htConsultCall,
    LPHDRVCALL        lphdConsultCall,
    DWORD             dwNumParties,
    LPLINECALLPARAMS  const lpCallParams);

LONG TSPIAPI TSPI_lineSetupTransfer(
    DRV_REQUESTID     dwRequestID,
    HDRVCALL          hdCall,
    HTAPICALL         htConsultCall,
    LPHDRVCALL        lphdConsultCall,
    LPLINECALLPARAMS  const lpCallParams);

LONG TSPIAPI TSPI_lineSwapHold(
    DRV_REQUESTID  dwRequestID,
    HDRVCALL       hdActiveCall,
    HDRVCALL       hdHeldCall);

LONG TSPIAPI TSPI_lineUnhold(
    DRV_REQUESTID  dwRequestID,
    HDRVCALL       hdCall);



// ----------------------------------------------------------------------------
// TSPIAPI TSPI_phone functions
// ----------------------------------------------------------------------------

LONG
TSPIAPI
TSPI_phoneClose(
    HDRVPHONE           hdPhone
    );

LONG
TSPIAPI
TSPI_phoneDevSpecific(
    DRV_REQUESTID       dwRequestID,
    HDRVPHONE           hdPhone,
    LPVOID              lpParams,
    DWORD               dwSize
    );

LONG
TSPIAPI
TSPI_phoneGetButtonInfo(
    HDRVPHONE           hdPhone,
    DWORD               dwButtonLampID,
    LPPHONEBUTTONINFO   lpButtonInfo
    );

LONG
TSPIAPI
TSPI_phoneGetData(
    HDRVPHONE           hdPhone,
    DWORD               dwDataID,
    LPVOID              lpData,
    DWORD               dwSize
    );

LONG
TSPIAPI
TSPI_phoneGetDevCaps(
    DWORD               dwDeviceID,
    DWORD               dwTSPIVersion,
    DWORD               dwExtVersion,
    LPPHONECAPS         lpPhoneCaps
    );

LONG
TSPIAPI
TSPI_phoneGetDisplay(
    HDRVPHONE           hdPhone,
    LPVARSTRING         lpDisplay
    );

LONG
TSPIAPI
TSPI_phoneGetExtensionID(
    DWORD               dwDeviceID,
    DWORD               dwTSPIVersion,
    LPPHONEEXTENSIONID  lpExtensionID
    );

LONG
TSPIAPI
TSPI_phoneGetGain(
    HDRVPHONE           hdPhone,
    DWORD               dwHookSwitchDev,
    LPDWORD             lpdwGain
    );

LONG
TSPIAPI
TSPI_phoneGetHookSwitch(
    HDRVPHONE           hdPhone,
    LPDWORD             lpdwHookSwitchDevs
    );

LONG
TSPIAPI
TSPI_phoneGetIcon(
    DWORD               dwDeviceID,
    LPCWSTR             lpszDeviceClass,
    LPHICON             lphIcon
    );

LONG
TSPIAPI
TSPI_phoneGetID(
    HDRVPHONE           hdPhone,
    LPVARSTRING         lpDeviceID,
    LPCWSTR             lpszDeviceClass
    );

LONG
TSPIAPI
TSPI_phoneGetLamp(
    HDRVPHONE           hdPhone,
    DWORD               dwButtonLampID,
    LPDWORD             lpdwLampMode
    );

LONG
TSPIAPI
TSPI_phoneGetRing(
    HDRVPHONE           hdPhone,
    LPDWORD             lpdwRingMode,
    LPDWORD             lpdwVolume
    );

LONG
TSPIAPI
TSPI_phoneGetStatus(
    HDRVPHONE           hdPhone,
    LPPHONESTATUS       lpPhoneStatus
    );

LONG
TSPIAPI
TSPI_phoneGetVolume(
    HDRVPHONE           hdPhone,
    DWORD               dwHookSwitchDev,
    LPDWORD             lpdwVolume
    );

LONG
TSPIAPI
TSPI_phoneNegotiateExtVersion(
    DWORD               dwDeviceID,
    DWORD               dwTSPIVersion,
    DWORD               dwLowVersion,
    DWORD               dwHighVersion,
    LPDWORD             lpdwExtVersion
    );

LONG
TSPIAPI
TSPI_phoneNegotiateTSPIVersion(
    DWORD               dwDeviceID,
    DWORD               dwLowVersion,
    DWORD               dwHighVersion,
    LPDWORD             lpdwTSPIVersion
    );

LONG
TSPIAPI
TSPI_phoneOpen(
    DWORD               dwDeviceID,
    HTAPIPHONE          htPhone,
    LPHDRVPHONE         lphdPhone,
    DWORD               dwTSPIVersion,
    PHONEEVENT          lpfnEventProc
    );

LONG
TSPIAPI
TSPI_phoneSelectExtVersion(
    HDRVPHONE           hdPhone,
    DWORD               dwExtVersion
    );

LONG
TSPIAPI
TSPI_phoneSetButtonInfo(
    DRV_REQUESTID       dwRequestID,
    HDRVPHONE           hdPhone,
    DWORD               dwButtonLampID,
    LPPHONEBUTTONINFO   const lpButtonInfo
    );

LONG
TSPIAPI
TSPI_phoneSetData(
    DRV_REQUESTID       dwRequestID,
    HDRVPHONE           hdPhone,
    DWORD               dwDataID,
    LPVOID              const lpData,
    DWORD               dwSize
    );

LONG
TSPIAPI
TSPI_phoneSetDisplay(
    DRV_REQUESTID       dwRequestID,
    HDRVPHONE           hdPhone,
    DWORD               dwRow,
    DWORD               dwColumn,
    LPCWSTR             lpsDisplay,
    DWORD               dwSize
    );

LONG
TSPIAPI
TSPI_phoneSetGain(
    DRV_REQUESTID       dwRequestID,
    HDRVPHONE           hdPhone,
    DWORD               dwHookSwitchDev,
    DWORD               dwGain
    );

LONG
TSPIAPI
TSPI_phoneSetHookSwitch(
    DRV_REQUESTID       dwRequestID,
    HDRVPHONE           hdPhone,
    DWORD               dwHookSwitchDevs,
    DWORD               dwHookSwitchMode
    );

LONG
TSPIAPI
TSPI_phoneSetLamp(
    DRV_REQUESTID       dwRequestID,
    HDRVPHONE           hdPhone,
    DWORD               dwButtonLampID,
    DWORD               dwLampMode
    );

LONG
TSPIAPI
TSPI_phoneSetRing(
    DRV_REQUESTID       dwRequestID,
    HDRVPHONE           hdPhone,
    DWORD               dwRingMode,
    DWORD               dwVolume
    );

LONG
TSPIAPI
TSPI_phoneSetStatusMessages(
    HDRVPHONE           hdPhone,
    DWORD               dwPhoneStates,
    DWORD               dwButtonModes,
    DWORD               dwButtonStates
    );

LONG
TSPIAPI
TSPI_phoneSetVolume(
    DRV_REQUESTID       dwRequestID,
    HDRVPHONE           hdPhone,
    DWORD               dwHookSwitchDev,
    DWORD               dwVolume
    );



// ----------------------------------------------------------------------------
// TSPIAPI TSPI_provider functions
// ----------------------------------------------------------------------------

// Returns a Vtbl of all the other TSPI functions
LONG TSPIAPI TSPI_lineGetProcTable(
    void *lplpTspiProcs
    );

LONG TSPIAPI TSPI_providerInstall(
    HWND   hwndOwner,
    DWORD  dwPermanentProviderID
    );

LONG TSPIAPI TSPI_providerInit(
    DWORD             dwTSPIVersion,
    DWORD             dwPermanentProviderID,
    DWORD             dwLineDeviceIDBase,
    DWORD             dwPhoneDeviceIDBase,
    DWORD             dwNumLines,
    DWORD             dwNumPhones,
    ASYNC_COMPLETION  lpfnCompletionProc,
    LPDWORD           lpdwTSPIOptions);

LONG TSPIAPI TSPI_providerShutdown(
    DWORD    dwTSPIVersion);

LONG TSPIAPI TSPI_providerCreateLineDevice(
    HKEY    hActiveKey,
    LPCWSTR lpszDevPath,
    LPCWSTR lpszDeviceName);

LONG TSPIAPI TSPI_providerDeleteLineDevice(
    DWORD   Identifier );

LONG TSPIAPI TSPI_providerCreatePhoneDevice(
    HKEY    hActiveKey,
    LPCWSTR lpszDevPath,
    LPCWSTR lpszDeviceName);

LONG TSPIAPI TSPI_providerDeletePhoneDevice(
    DWORD   Identifier );

LONG TSPIAPI TSPI_providerRemoveDevice(
    LPCWSTR lpszDeviceName);

#ifdef __cplusplus
}                       /* End of extern "C" { */
#endif  /* __cplusplus */

#endif  // TSPI_H
