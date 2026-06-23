// The  Telephony  API  is jointly copyrighted by Intel and Microsoft.  You are
// granted  a royalty free worldwide, unlimited license to make copies, and use
// the   API/SPI  for  making  applications/drivers  that  interface  with  the
// specification provided that this paragraph and the Intel/Microsoft copyright
// statement is maintained as is in the text and source code files.
//
// Copyright (c) 1992-1998 Intel/Microsoft, all rights reserved.

// @CESYSGEN IF CE_MODULES_TAPI

#ifndef TAPI_H
#define TAPI_H

#include <windows.h>

#pragma pack(1)
// Type definitions of the data types used in tapi

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif   /* __cplusplus */


#define DECLARE_OPAQUE32(name)  struct name##__ { int unused; }; \
                                typedef const struct name##__ FAR* name

DECLARE_OPAQUE32(HCALL);
typedef HCALL * LPHCALL;

DECLARE_OPAQUE32(HLINE);
typedef HLINE * LPHLINE;

DECLARE_OPAQUE32(HLINEAPP);
typedef HLINEAPP * LPHLINEAPP;

typedef HICON * LPHICON;
// typedef of the LINE callback procedure
typedef void (CALLBACK * LINECALLBACK)  (DWORD hDevice,
                                              DWORD dwMessage,
                                              DWORD dwInstance,
                                              DWORD dwParam1,
                                              DWORD dwParam2,
                                              DWORD dwParam3);


// Messages for Phones and Lines

#define LINE_ADDRESSSTATE        0L
#define LINE_CALLINFO            1L
#define LINE_CALLSTATE           2L
#define LINE_CLOSE               3L
#define LINE_DEVSPECIFIC         4L
#define LINE_DEVSPECIFICFEATURE  5L
#define LINE_GATHERDIGITS        6L
#define LINE_GENERATE            7L
#define LINE_LINEDEVSTATE        8L
#define LINE_MONITORDIGITS       9L
#define LINE_MONITORMEDIA        10L
#define LINE_MONITORTONE         11L
#define LINE_REPLY               12L
#define LINE_REQUEST             13L

#define LINE_CREATE              19L

#define LINE_REMOVE              21L


#define TAPIMAXDESTADDRESSSIZE   200L 
#define TAPIMAXAPPNAMESIZE        40L
#define TAPIMAXCALLEDPARTYSIZE    40L
#define TAPIMAXCOMMENTSIZE        80L
#define TAPIMAXDEVICECLASSSIZE    40L
#define TAPIMAXDEVICEIDSIZE       40L


#define STRINGFORMAT_ASCII   0x00000001
#define STRINGFORMAT_DBCS    0x00000002
#define STRINGFORMAT_UNICODE 0x00000003
#define STRINGFORMAT_BINARY  0x00000004

typedef struct varstring_tag {
    DWORD    dwTotalSize;
    DWORD    dwNeededSize;
    DWORD    dwUsedSize;

    DWORD    dwStringFormat;
    DWORD    dwStringSize;
    DWORD    dwStringOffset;
} VARSTRING, *LPVARSTRING;

// Data types and values for Lines


#define LINEADDRCAPFLAGS_FWDNUMRINGS        0x00000001
#define LINEADDRCAPFLAGS_PICKUPGROUPID      0x00000002    
#define LINEADDRCAPFLAGS_SECURE             0x00000004
#define LINEADDRCAPFLAGS_BLOCKIDDEFAULT     0x00000008
#define LINEADDRCAPFLAGS_BLOCKIDOVERRIDE    0x00000010
#define LINEADDRCAPFLAGS_DIALED             0x00000020
#define LINEADDRCAPFLAGS_ORIGOFFHOOK        0x00000040
#define LINEADDRCAPFLAGS_DESTOFFHOOK        0x00000080
#define LINEADDRCAPFLAGS_FWDCONSULT         0x00000100
#define LINEADDRCAPFLAGS_SETUPCONFNULL      0x00000200
#define LINEADDRCAPFLAGS_AUTORECONNECT      0x00000400
#define LINEADDRCAPFLAGS_COMPLETIONID       0x00000800
#define LINEADDRCAPFLAGS_TRANSFERHELD       0x00001000
#define LINEADDRCAPFLAGS_TRANSFERMAKE       0x00002000
#define LINEADDRCAPFLAGS_CONFERENCEHELD     0x00004000
#define LINEADDRCAPFLAGS_CONFERENCEMAKE     0x00008000
#define LINEADDRCAPFLAGS_PARTIALDIAL        0x00010000
#define LINEADDRCAPFLAGS_FWDSTATUSVALID     0x00020000
#define LINEADDRCAPFLAGS_FWDINTEXTADDR      0x00040000
#define LINEADDRCAPFLAGS_FWDBUSYNAADDR      0x00080000
#define LINEADDRCAPFLAGS_ACCEPTTOALERT      0x00100000
#define LINEADDRCAPFLAGS_CONFDROP           0x00200000
#define LINEADDRCAPFLAGS_PICKUPCALLWAIT     0x00400000

typedef struct lineaddresscaps_tag {
    DWORD    dwTotalSize;
    DWORD    dwNeededSize;
    DWORD    dwUsedSize;

    DWORD    dwLineDeviceID;

    DWORD    dwAddressSize;
    DWORD    dwAddressOffset;

    DWORD    dwDevSpecificSize;
    DWORD    dwDevSpecificOffset;

    DWORD    dwAddressSharing;
    DWORD    dwAddressStates;
    DWORD    dwCallInfoStates;
    DWORD    dwCallerIDFlags;
    DWORD    dwCalledIDFlags;
    DWORD    dwConnectedIDFlags;
    DWORD    dwRedirectionIDFlags;
    DWORD    dwRedirectingIDFlags;
    DWORD    dwCallStates;
    DWORD    dwDialToneModes;
    DWORD    dwBusyModes;
    DWORD    dwSpecialInfo;
    DWORD    dwDisconnectModes;

    DWORD    dwMaxNumActiveCalls;
    DWORD    dwMaxNumOnHoldCalls;
    DWORD    dwMaxNumOnHoldPendingCalls;
    DWORD    dwMaxNumConference;
    DWORD    dwMaxNumTransConf;

    DWORD    dwAddrCapFlags;
    DWORD    dwCallFeatures;
    DWORD    dwRemoveFromConfCaps;
    DWORD    dwRemoveFromConfState;
    DWORD    dwTransferModes;
    DWORD    dwParkModes;

    DWORD    dwForwardModes;
    DWORD    dwMaxForwardEntries;
    DWORD    dwMaxSpecificEntries;
    DWORD    dwMinFwdNumRings;
    DWORD    dwMaxFwdNumRings;

    DWORD    dwMaxCallCompletions;
    DWORD    dwCallCompletionConds;
    DWORD    dwCallCompletionModes;
    DWORD    dwNumCompletionMessages;
    DWORD    dwCompletionMsgTextEntrySize;
    DWORD    dwCompletionMsgTextSize;
    DWORD    dwCompletionMsgTextOffset;
} LINEADDRESSCAPS, *LPLINEADDRESSCAPS;

#define LINEADDRESSMODE_ADDRESSID        0x00000001
#define LINEADDRESSMODE_DIALABLEADDR     0x00000002


#define LINEADDRESSSHARING_PRIVATE        0x00000001
#define LINEADDRESSSHARING_BRIDGEDEXCL    0x00000002
#define LINEADDRESSSHARING_BRIDGEDNEW     0x00000004
#define LINEADDRESSSHARING_BRIDGEDSHARED  0x00000008
#define LINEADDRESSSHARING_MONITORED      0x00000010

#define LINEADDRESSSTATE_OTHER            0x00000001
#define LINEADDRESSSTATE_DEVSPECIFIC      0x00000002
#define LINEADDRESSSTATE_INUSEZERO        0x00000004
#define LINEADDRESSSTATE_INUSEONE         0x00000008
#define LINEADDRESSSTATE_INUSEMANY        0x00000010
#define LINEADDRESSSTATE_NUMCALLS         0x00000020
#define LINEADDRESSSTATE_FORWARD          0x00000040
#define LINEADDRESSSTATE_TERMINALS        0x00000080

typedef struct lineaddressstatus_tag {
    DWORD    dwTotalSize;
    DWORD    dwNeededSize;
    DWORD    dwUsedSize;

    DWORD    dwNumInUse;
    DWORD    dwNumActiveCalls;
    DWORD    dwNumOnHoldCalls;
    DWORD    dwNumOnHoldPendCalls;
    DWORD    dwAddressFeatures;

    DWORD    dwNumRingsNoAnswer;
    DWORD    dwForwardNumEntries;
    DWORD    dwForwardSize;
    DWORD    dwForwardOffset;

    DWORD    dwTerminalModesSize;
    DWORD    dwTerminalModesOffset;

    DWORD    dwDevSpecificSize;
    DWORD    dwDevSpecificOffset;
} LINEADDRESSSTATUS, *LPLINEADDRESSSTATUS;

#define LINEADDRFEATURE_FORWARD           0x00000001
#define LINEADDRFEATURE_MAKECALL          0x00000002
#define LINEADDRFEATURE_PICKUP            0x00000004
#define LINEADDRFEATURE_SETMEDIACONTROL   0x00000008
#define LINEADDRFEATURE_SETTERMINAL       0x00000010
#define LINEADDRFEATURE_SETUPCONF         0x00000020
#define LINEADDRFEATURE_UNCOMPLETECALL    0x00000040
#define LINEADDRFEATURE_UNPARK            0x00000080


#define LINEANSWERMODE_NONE    0x00000001
#define LINEANSWERMODE_DROP    0x00000002
#define LINEANSWERMODE_HOLD    0x00000004

#define LINEBEARERMODE_VOICE            0x00000001
#define LINEBEARERMODE_SPEECH           0x00000002
#define LINEBEARERMODE_MULTIUSE         0x00000004
#define LINEBEARERMODE_DATA             0x00000008
#define LINEBEARERMODE_ALTSPEECHDATA    0x00000010
#define LINEBEARERMODE_NONCALLSIGNALING 0x00000020
#define LINEBEARERMODE_PASSTHROUGH      0x00000040

#define LINEBUSYMODE_STATION    0x00000001
#define LINEBUSYMODE_TRUNK      0x00000002
#define LINEBUSYMODE_UNKNOWN    0x00000004
#define LINEBUSYMODE_UNAVAIL    0x00000008

#define LINECALLCOMPLCOND_BUSY        0x00000001
#define LINECALLCOMPLCOND_NOANSWER    0x00000002

#define LINECALLCOMPLMODE_CAMPON      0x00000001
#define LINECALLCOMPLMODE_CALLBACK    0x00000002
#define LINECALLCOMPLMODE_INTRUDE     0x00000004
#define LINECALLCOMPLMODE_MESSAGE     0x00000008

#define LINECALLFEATURE_ACCEPT                   0x00000001
#define LINECALLFEATURE_ADDTOCONF                0x00000002
#define LINECALLFEATURE_ANSWER                   0x00000004
#define LINECALLFEATURE_BLINDTRANSFER            0x00000008
#define LINECALLFEATURE_COMPLETECALL             0x00000010
#define LINECALLFEATURE_COMPLETETRANSF           0x00000020
#define LINECALLFEATURE_DIAL                     0x00000040
#define LINECALLFEATURE_DROP                     0x00000080
#define LINECALLFEATURE_GATHERDIGITS             0x00000100
#define LINECALLFEATURE_GENERATEDIGITS           0x00000200
#define LINECALLFEATURE_GENERATETONE             0x00000400
#define LINECALLFEATURE_HOLD                     0x00000800
#define LINECALLFEATURE_MONITORDIGITS            0x00001000
#define LINECALLFEATURE_MONITORMEDIA             0x00002000
#define LINECALLFEATURE_MONITORTONES             0x00004000
#define LINECALLFEATURE_PARK                     0x00008000
#define LINECALLFEATURE_PREPAREADDCONF           0x00010000
#define LINECALLFEATURE_REDIRECT                 0x00020000
#define LINECALLFEATURE_REMOVEFROMCONF           0x00040000
#define LINECALLFEATURE_SECURECALL               0x00080000
#define LINECALLFEATURE_SENDUSERUSER             0x00100000
#define LINECALLFEATURE_SETCALLPARAMS            0x00200000
#define LINECALLFEATURE_SETMEDIACONTROL          0x00400000
#define LINECALLFEATURE_SETTERMINAL              0x00800000
#define LINECALLFEATURE_SETUPCONF                0x01000000
#define LINECALLFEATURE_SETUPTRANSFER            0x02000000
#define LINECALLFEATURE_SWAPHOLD                 0x04000000
#define LINECALLFEATURE_UNHOLD                   0x08000000

typedef struct linedialparams_tag {
    DWORD    dwDialPause;
    DWORD    dwDialSpeed;
    DWORD    dwDigitDuration;
    DWORD    dwWaitForDialtone;
} LINEDIALPARAMS, *LPLINEDIALPARAMS;

typedef struct linecallinfo_tag {
    DWORD    dwTotalSize;
    DWORD    dwNeededSize;
    DWORD    dwUsedSize;

    HLINE    hLine;
    DWORD    dwLineDeviceID;
    DWORD    dwAddressID;

    DWORD    dwBearerMode;
    DWORD    dwRate;
    DWORD    dwMediaMode;

    DWORD    dwAppSpecific;
    DWORD    dwCallID;
    DWORD    dwRelatedCallID;
    DWORD    dwCallParamFlags;
    DWORD    dwCallStates;

    DWORD    dwMonitorDigitModes;
    DWORD    dwMonitorMediaModes;
    LINEDIALPARAMS    DialParams;

    DWORD    dwOrigin;
    DWORD    dwReason;
    DWORD    dwCompletionID;
    DWORD    dwNumOwners;
    DWORD    dwNumMonitors;

    DWORD    dwCountryCode;
    DWORD    dwTrunk;

    DWORD    dwCallerIDFlags;
    DWORD    dwCallerIDSize;
    DWORD    dwCallerIDOffset;
    DWORD    dwCallerIDNameSize;
    DWORD    dwCallerIDNameOffset;

    DWORD    dwCalledIDFlags;
    DWORD    dwCalledIDSize;
    DWORD    dwCalledIDOffset;
    DWORD    dwCalledIDNameSize;
    DWORD    dwCalledIDNameOffset;

    DWORD    dwConnectedIDFlags;
    DWORD    dwConnectedIDSize;
    DWORD    dwConnectedIDOffset;
    DWORD    dwConnectedIDNameSize;
    DWORD    dwConnectedIDNameOffset;

    DWORD    dwRedirectionIDFlags;
    DWORD    dwRedirectionIDSize;
    DWORD    dwRedirectionIDOffset;
    DWORD    dwRedirectionIDNameSize;
    DWORD    dwRedirectionIDNameOffset;

    DWORD    dwRedirectingIDFlags;
    DWORD    dwRedirectingIDSize;
    DWORD    dwRedirectingIDOffset;
    DWORD    dwRedirectingIDNameSize;
    DWORD    dwRedirectingIDNameOffset;

    DWORD    dwAppNameSize;
    DWORD    dwAppNameOffset;

    DWORD    dwDisplayableAddressSize;
    DWORD    dwDisplayableAddressOffset;

    DWORD    dwCalledPartySize;
    DWORD    dwCalledPartyOffset;

    DWORD    dwCommentSize;
    DWORD    dwCommentOffset;

    DWORD    dwDisplaySize;
    DWORD    dwDisplayOffset;

    DWORD    dwUserUserInfoSize;
    DWORD    dwUserUserInfoOffset;

    DWORD    dwHighLevelCompSize;
    DWORD    dwHighLevelCompOffset;

    DWORD    dwLowLevelCompSize;
    DWORD    dwLowLevelCompOffset;

    DWORD    dwChargingInfoSize;
    DWORD    dwChargingInfoOffset;

    DWORD    dwTerminalModesSize;
    DWORD    dwTerminalModesOffset;

    DWORD    dwDevSpecificSize;
    DWORD    dwDevSpecificOffset;
} LINECALLINFO, *LPLINECALLINFO;

#define LINECALLINFOSTATE_OTHER             0x00000001
#define LINECALLINFOSTATE_DEVSPECIFIC       0x00000002
#define LINECALLINFOSTATE_BEARERMODE        0x00000004
#define LINECALLINFOSTATE_RATE              0x00000008
#define LINECALLINFOSTATE_MEDIAMODE         0x00000010
#define LINECALLINFOSTATE_APPSPECIFIC       0x00000020
#define LINECALLINFOSTATE_CALLID            0x00000040
#define LINECALLINFOSTATE_RELATEDCALLID     0x00000080
#define LINECALLINFOSTATE_ORIGIN            0x00000100
#define LINECALLINFOSTATE_REASON            0x00000200
#define LINECALLINFOSTATE_COMPLETIONID      0x00000400
#define LINECALLINFOSTATE_NUMOWNERINCR      0x00000800
#define LINECALLINFOSTATE_NUMOWNERDECR      0x00001000
#define LINECALLINFOSTATE_NUMMONITORS       0x00002000
#define LINECALLINFOSTATE_TRUNK             0x00004000
#define LINECALLINFOSTATE_CALLERID          0x00008000
#define LINECALLINFOSTATE_CALLEDID          0x00010000
#define LINECALLINFOSTATE_CONNECTEDID       0x00020000
#define LINECALLINFOSTATE_REDIRECTIONID     0x00040000
#define LINECALLINFOSTATE_REDIRECTINGID     0x00080000
#define LINECALLINFOSTATE_DISPLAY           0x00100000
#define LINECALLINFOSTATE_USERUSERINFO      0x00200000
#define LINECALLINFOSTATE_HIGHLEVELCOMP     0x00400000
#define LINECALLINFOSTATE_LOWLEVELCOMP      0x00800000
#define LINECALLINFOSTATE_CHARGINGINFO      0x01000000
#define LINECALLINFOSTATE_TERMINAL          0x02000000
#define LINECALLINFOSTATE_DIALPARAMS        0x04000000
#define LINECALLINFOSTATE_MONITORMODES      0x08000000

typedef struct linecalllist_tag {
    DWORD    dwTotalSize;
    DWORD    dwNeededSize;
    DWORD    dwUsedSize;
    
    DWORD    dwCallsNumEntries;
    DWORD    dwCallsSize;
    DWORD    dwCallsOffset;
} LINECALLLIST, *LPLINECALLLIST;

#define LINECALLORIGIN_OUTBOUND    0x00000001
#define LINECALLORIGIN_INTERNAL    0x00000002
#define LINECALLORIGIN_EXTERNAL    0x00000004
#define LINECALLORIGIN_UNKNOWN     0x00000010
#define LINECALLORIGIN_UNAVAIL     0x00000020
#define LINECALLORIGIN_CONFERENCE  0x00000040

#define LINECALLPARAMFLAGS_SECURE           0x00000001
#define LINECALLPARAMFLAGS_IDLE             0x00000002
#define LINECALLPARAMFLAGS_BLOCKID          0x00000004
#define LINECALLPARAMFLAGS_ORIGOFFHOOK      0x00000008
#define LINECALLPARAMFLAGS_DESTOFFHOOK      0x00000010

typedef struct linecallparams_tag {     // Defaults:
    DWORD    dwTotalSize;               // ---------
    
    DWORD    dwBearerMode;              // voice
    DWORD    dwMinRate;                 // (3.1kHz)
    DWORD    dwMaxRate;                 // (3.1kHz)
    DWORD    dwMediaMode;               // interactiveVoice

    DWORD    dwCallParamFlags;          // 0
    DWORD    dwAddressMode;             // addressID
    DWORD    dwAddressID;               // (any available)

    LINEDIALPARAMS  DialParams;         // (0, 0, 0, 0)
    
    DWORD    dwOrigAddressSize;         // 0
    DWORD    dwOrigAddressOffset;
    DWORD    dwDisplayableAddressSize;
    DWORD    dwDisplayableAddressOffset;

    DWORD    dwCalledPartySize;         // 0
    DWORD    dwCalledPartyOffset;

    DWORD    dwCommentSize;             // 0
    DWORD    dwCommentOffset;

    DWORD    dwUserUserInfoSize;        // 0
    DWORD    dwUserUserInfoOffset;

    DWORD    dwHighLevelCompSize;       // 0
    DWORD    dwHighLevelCompOffset;

    DWORD    dwLowLevelCompSize;        // 0
    DWORD    dwLowLevelCompOffset;

    DWORD    dwDevSpecificSize;         // 0
    DWORD    dwDevSpecificOffset;
} LINECALLPARAMS, *LPLINECALLPARAMS;


#define LINECALLPARTYID_BLOCKED         0x00000001
#define LINECALLPARTYID_OUTOFAREA       0x00000002
#define LINECALLPARTYID_NAME            0x00000004
#define LINECALLPARTYID_ADDRESS         0x00000008
#define LINECALLPARTYID_PARTIAL         0x00000010
#define LINECALLPARTYID_UNKNOWN         0x00000020
#define LINECALLPARTYID_UNAVAIL         0x00000040

#define LINECALLPRIVILEGE_NONE          0x00000001
#define LINECALLPRIVILEGE_MONITOR       0x00000002
#define LINECALLPRIVILEGE_OWNER         0x00000004

#define LINECALLSELECT_LINE     0x00000001
#define LINECALLSELECT_ADDRESS  0x00000002
#define LINECALLSELECT_CALL     0x00000004

#define LINECALLSTATE_IDLE                  0x00000001
#define LINECALLSTATE_OFFERING              0x00000002
#define LINECALLSTATE_ACCEPTED              0x00000004
#define LINECALLSTATE_DIALTONE              0x00000008
#define LINECALLSTATE_DIALING               0x00000010
#define LINECALLSTATE_RINGBACK              0x00000020
#define LINECALLSTATE_BUSY                  0x00000040
#define LINECALLSTATE_SPECIALINFO           0x00000080
#define LINECALLSTATE_CONNECTED             0x00000100
#define LINECALLSTATE_PROCEEDING            0x00000200
#define LINECALLSTATE_ONHOLD                0x00000400
#define LINECALLSTATE_CONFERENCED           0x00000800
#define LINECALLSTATE_ONHOLDPENDCONF        0x00001000
#define LINECALLSTATE_ONHOLDPENDTRANSFER    0x00002000
#define LINECALLSTATE_DISCONNECTED          0x00004000
#define LINECALLSTATE_UNKNOWN               0x00008000


#define LINECONNECTEDMODE_ACTIVE            0x00000001
#define LINECONNECTEDMODE_INACTIVE          0x00000002

#define LINEOFFERINGMODE_ACTIVE             0x00000001
#define LINEOFFERINGMODE_INACTIVE           0x00000002


typedef struct linecallstatus_tag {
    DWORD    dwTotalSize;
    DWORD    dwNeededSize;
    DWORD    dwUsedSize;

    DWORD    dwCallState;
    DWORD    dwCallStateMode;
    DWORD    dwCallPrivilege;
    DWORD    dwCallFeatures;

    DWORD    dwDevSpecificSize;
    DWORD    dwDevSpecificOffset;
} LINECALLSTATUS, *LPLINECALLSTATUS;


#define LINEDEVCAPFLAGS_CROSSADDRCONF   0x00000001
#define LINEDEVCAPFLAGS_HIGHLEVCOMP     0x00000002
#define LINEDEVCAPFLAGS_LOWLEVCOMP      0x00000004
#define LINEDEVCAPFLAGS_MEDIACONTROL    0x00000008
#define LINEDEVCAPFLAGS_MULTIPLEADDR    0x00000010
#define LINEDEVCAPFLAGS_CLOSEDROP       0x00000020
#define LINEDEVCAPFLAGS_DIALBILLING     0x00000040
#define LINEDEVCAPFLAGS_DIALQUIET       0x00000080
#define LINEDEVCAPFLAGS_DIALDIALTONE    0x00000100

typedef struct lineextensionid_tag {
    DWORD    dwExtensionID0;
    DWORD    dwExtensionID1;
    DWORD    dwExtensionID2;
    DWORD    dwExtensionID3;
} LINEEXTENSIONID, *LPLINEEXTENSIONID;

typedef struct linedevcaps_tag {
    DWORD    dwTotalSize;
    DWORD    dwNeededSize;
    DWORD    dwUsedSize;

    DWORD    dwProviderInfoSize;
    DWORD    dwProviderInfoOffset;

    DWORD    dwSwitchInfoSize;
    DWORD    dwSwitchInfoOffset;

    DWORD    dwPermanentLineID;
    DWORD    dwLineNameSize;
    DWORD    dwLineNameOffset;
    DWORD    dwStringFormat;
    
    DWORD    dwAddressModes;
    DWORD    dwNumAddresses;
    DWORD    dwBearerModes;
    DWORD    dwMaxRate;
    DWORD    dwMediaModes;

    DWORD    dwGenerateToneModes;
    DWORD    dwGenerateToneMaxNumFreq;
    DWORD    dwGenerateDigitModes;
    DWORD    dwMonitorToneMaxNumFreq;
    DWORD    dwMonitorToneMaxNumEntries;
    DWORD    dwMonitorDigitModes;
    DWORD    dwGatherDigitsMinTimeout;
    DWORD    dwGatherDigitsMaxTimeout;

    DWORD    dwMedCtlDigitMaxListSize;
    DWORD    dwMedCtlMediaMaxListSize;
    DWORD    dwMedCtlToneMaxListSize;
    DWORD    dwMedCtlCallStateMaxListSize;

    DWORD    dwDevCapFlags;
    DWORD    dwMaxNumActiveCalls;
    DWORD    dwAnswerMode;
    DWORD    dwRingModes;
    DWORD    dwLineStates;

    DWORD    dwUUIAcceptSize;
    DWORD    dwUUIAnswerSize;
    DWORD    dwUUIMakeCallSize;
    DWORD    dwUUIDropSize;
    DWORD    dwUUISendUserUserInfoSize;
    DWORD    dwUUICallInfoSize;
    
    LINEDIALPARAMS    MinDialParams;
    LINEDIALPARAMS    MaxDialParams;
    LINEDIALPARAMS    DefaultDialParams;

    DWORD    dwNumTerminals;
    DWORD    dwTerminalCapsSize;
    DWORD    dwTerminalCapsOffset;
    DWORD    dwTerminalTextEntrySize;
    DWORD    dwTerminalTextSize;
    DWORD    dwTerminalTextOffset;

    DWORD    dwDevSpecificSize;
    DWORD    dwDevSpecificOffset;
} LINEDEVCAPS, *LPLINEDEVCAPS;
    
#define LINEDEVSTATE_OTHER              0x00000001
#define LINEDEVSTATE_RINGING            0x00000002
#define LINEDEVSTATE_CONNECTED          0x00000004
#define LINEDEVSTATE_DISCONNECTED       0x00000008
#define LINEDEVSTATE_MSGWAITON          0x00000010
#define LINEDEVSTATE_MSGWAITOFF         0x00000020
#define LINEDEVSTATE_INSERVICE          0x00000040
#define LINEDEVSTATE_OUTOFSERVICE       0x00000080
#define LINEDEVSTATE_MAINTENANCE        0x00000100
#define LINEDEVSTATE_OPEN               0x00000200
#define LINEDEVSTATE_CLOSE              0x00000400
#define LINEDEVSTATE_NUMCALLS           0x00000800
#define LINEDEVSTATE_NUMCOMPLETIONS     0x00001000
#define LINEDEVSTATE_TERMINALS          0x00002000
#define LINEDEVSTATE_ROAMMODE           0x00004000
#define LINEDEVSTATE_BATTERY            0x00008000
#define LINEDEVSTATE_SIGNAL             0x00010000
#define LINEDEVSTATE_DEVSPECIFIC        0x00020000
#define LINEDEVSTATE_REINIT             0x00040000
#define LINEDEVSTATE_LOCK               0x00080000
#define LINEDEVSTATE_CAPSCHANGE         0x00100000
#define LINEDEVSTATE_CONFIGCHANGE       0x00200000
#define LINEDEVSTATE_TRANSLATECHANGE    0x00400000
#define LINEDEVSTATE_COMPLCANCEL        0x00800000
#define LINEDEVSTATE_REMOVED            0x01000000

typedef struct linedevstatus_tag {
    DWORD    dwTotalSize;
    DWORD    dwNeededSize;
    DWORD    dwUsedSize;

    DWORD    dwNumOpens;
    DWORD    dwOpenMediaModes;
    DWORD    dwNumActiveCalls;
    DWORD    dwNumOnHoldCalls;
    DWORD    dwNumOnHoldPendCalls;
    DWORD    dwLineFeatures;
    DWORD    dwNumCallCompletions;
    DWORD    dwRingMode;
    DWORD    dwSignalLevel;
    DWORD    dwBatteryLevel;
    DWORD    dwRoamMode;
    
    DWORD    dwDevStatusFlags;
    
    DWORD    dwTerminalModesSize;
    DWORD    dwTerminalModesOffset;
    
    DWORD    dwDevSpecificSize;
    DWORD    dwDevSpecificOffset;
} LINEDEVSTATUS, *LPLINEDEVSTATUS;


#define LINEDEVSTATUSFLAGS_CONNECTED  0x00000001
#define LINEDEVSTATUSFLAGS_MSGWAIT    0x00000002
#define LINEDEVSTATUSFLAGS_INSERVICE  0x00000004
#define LINEDEVSTATUSFLAGS_LOCKED     0x00000008


#define LINEDIALTONEMODE_NORMAL    0x00000001
#define LINEDIALTONEMODE_SPECIAL   0x00000002
#define LINEDIALTONEMODE_INTERNAL  0x00000004
#define LINEDIALTONEMODE_EXTERNAL  0x00000008
#define LINEDIALTONEMODE_UNKNOWN   0x00000010
#define LINEDIALTONEMODE_UNAVAIL   0x00000020


#define LINEDISCONNECTMODE_NORMAL       0x00000001
#define LINEDISCONNECTMODE_UNKNOWN      0x00000002
#define LINEDISCONNECTMODE_REJECT       0x00000004
#define LINEDISCONNECTMODE_PICKUP       0x00000008
#define LINEDISCONNECTMODE_FORWARDED    0x00000010
#define LINEDISCONNECTMODE_BUSY         0x00000020
#define LINEDISCONNECTMODE_NOANSWER     0x00000040
#define LINEDISCONNECTMODE_BADADDRESS   0x00000080
#define LINEDISCONNECTMODE_UNREACHABLE  0x00000100
#define LINEDISCONNECTMODE_CONGESTION   0x00000200
#define LINEDISCONNECTMODE_INCOMPATIBLE 0x00000400
#define LINEDISCONNECTMODE_UNAVAIL      0x00000800
#define LINEDISCONNECTMODE_NODIALTONE   0x00001000

#define LINEERR_ALLOCATED                   0x80000001
#define LINEERR_BADDEVICEID                 0x80000002
#define LINEERR_BEARERMODEUNAVAIL           0x80000003
#define LINEERR_CALLUNAVAIL                 0x80000005
#define LINEERR_COMPLETIONOVERRUN           0x80000006
#define LINEERR_CONFERENCEFULL              0x80000007
#define LINEERR_DIALBILLING                 0x80000008
#define LINEERR_DIALDIALTONE                0x80000009
#define LINEERR_DIALPROMPT                  0x8000000A
#define LINEERR_DIALQUIET                   0x8000000B
#define LINEERR_INCOMPATIBLEAPIVERSION      0x8000000C
#define LINEERR_INCOMPATIBLEEXTVERSION      0x8000000D
#define LINEERR_INIFILECORRUPT              0x8000000E
#define LINEERR_INUSE                       0x8000000F
#define LINEERR_INVALADDRESS                0x80000010
#define LINEERR_INVALADDRESSID              0x80000011
#define LINEERR_INVALADDRESSMODE            0x80000012
#define LINEERR_INVALADDRESSSTATE           0x80000013
#define LINEERR_INVALAPPHANDLE              0x80000014
#define LINEERR_INVALAPPNAME                0x80000015
#define LINEERR_INVALBEARERMODE             0x80000016
#define LINEERR_INVALCALLCOMPLMODE          0x80000017
#define LINEERR_INVALCALLHANDLE             0x80000018
#define LINEERR_INVALCALLPARAMS             0x80000019
#define LINEERR_INVALCALLPRIVILEGE          0x8000001A
#define LINEERR_INVALCALLSELECT             0x8000001B
#define LINEERR_INVALCALLSTATE              0x8000001C
#define LINEERR_INVALCALLSTATELIST          0x8000001D
#define LINEERR_INVALCARD                   0x8000001E
#define LINEERR_INVALCOMPLETIONID           0x8000001F
#define LINEERR_INVALCONFCALLHANDLE         0x80000020
#define LINEERR_INVALCONSULTCALLHANDLE      0x80000021
#define LINEERR_INVALCOUNTRYCODE            0x80000022
#define LINEERR_INVALDEVICECLASS            0x80000023
#define LINEERR_INVALDEVICEHANDLE           0x80000024
#define LINEERR_INVALDIALPARAMS             0x80000025
#define LINEERR_INVALDIGITLIST              0x80000026
#define LINEERR_INVALDIGITMODE              0x80000027
#define LINEERR_INVALDIGITS                 0x80000028
#define LINEERR_INVALEXTVERSION             0x80000029
#define LINEERR_INVALGROUPID                0x8000002A
#define LINEERR_INVALLINEHANDLE             0x8000002B
#define LINEERR_INVALLINESTATE              0x8000002C
#define LINEERR_INVALLOCATION               0x8000002D
#define LINEERR_INVALMEDIALIST              0x8000002E
#define LINEERR_INVALMEDIAMODE              0x8000002F
#define LINEERR_INVALMESSAGEID              0x80000030
#define LINEERR_INVALPARAM                  0x80000032
#define LINEERR_INVALPARKID                 0x80000033
#define LINEERR_INVALPARKMODE               0x80000034
#define LINEERR_INVALPOINTER                0x80000035
#define LINEERR_INVALPRIVSELECT             0x80000036
#define LINEERR_INVALRATE                   0x80000037
#define LINEERR_INVALREQUESTMODE            0x80000038
#define LINEERR_INVALTERMINALID             0x80000039
#define LINEERR_INVALTERMINALMODE           0x8000003A
#define LINEERR_INVALTIMEOUT                0x8000003B 
#define LINEERR_INVALTONE                   0x8000003C
#define LINEERR_INVALTONELIST               0x8000003D
#define LINEERR_INVALTONEMODE               0x8000003E
#define LINEERR_INVALTRANSFERMODE           0x8000003F
#define LINEERR_LINEMAPPERFAILED            0x80000040
#define LINEERR_NOCONFERENCE                0x80000041
#define LINEERR_NODEVICE                    0x80000042
#define LINEERR_NODRIVER                    0x80000043
#define LINEERR_NOMEM                       0x80000044
#define LINEERR_NOREQUEST                   0x80000045
#define LINEERR_NOTOWNER                    0x80000046
#define LINEERR_NOTREGISTERED               0x80000047
#define LINEERR_OPERATIONFAILED             0x80000048
#define LINEERR_OPERATIONUNAVAIL            0x80000049
#define LINEERR_RATEUNAVAIL                 0x8000004A
#define LINEERR_RESOURCEUNAVAIL             0x8000004B
#define LINEERR_REQUESTOVERRUN              0x8000004C
#define LINEERR_STRUCTURETOOSMALL           0x8000004D
#define LINEERR_TARGETNOTFOUND              0x8000004E
#define LINEERR_TARGETSELF                  0x8000004F
#define LINEERR_UNINITIALIZED               0x80000050
#define LINEERR_USERUSERINFOTOOBIG          0x80000051
#define LINEERR_REINIT                      0x80000052
#define LINEERR_ADDRESSBLOCKED              0x80000053
#define LINEERR_BILLINGREJECTED             0x80000054
#define LINEERR_INVALFEATURE                0x80000055
#define LINEERR_NOMULTIPLEINSTANCE          0x80000056


typedef struct lineforward_tag {
    DWORD    dwForwardMode;

    DWORD    dwCallerAddressSize;
    DWORD    dwCallerAddressOffset;
    
    DWORD    dwDestCountryCode;
    DWORD    dwDestAddressSize;
    DWORD    dwDestAddressOffset;    
} LINEFORWARD, *LPLINEFORWARD;

typedef struct lineforwardlist_tag {
    DWORD    dwTotalSize;

    DWORD    dwNumEntries;
    LINEFORWARD    ForwardList[1];
} LINEFORWARDLIST, *LPLINEFORWARDLIST;

typedef struct linegeneratetone_tag {
    DWORD    dwFrequency;
    DWORD    dwCadenceOn;
    DWORD    dwCadenceOff;
    DWORD    dwVolume;
} LINEGENERATETONE, *LPLINEGENERATETONE;

#define LINEMAPPER         0xFFFFFFFF
typedef struct linemediacontrolcallstate_tag {
    DWORD    dwCallStates;
    DWORD    dwMediaControl;
} LINEMEDIACONTROLCALLSTATE, 
*LPLINEMEDIACONTROLCALLSTATE;

typedef struct linemediacontroldigit_tag {
    DWORD            dwDigit;
    DWORD            dwDigitModes;
    DWORD            dwMediaControl;
} LINEMEDIACONTROLDIGIT, *LPLINEMEDIACONTROLDIGIT;

typedef struct linemediacontrolmedia_tag {
    DWORD            dwMediaModes;
    DWORD            dwDuration;
    DWORD            dwMediaControl;
} LINEMEDIACONTROLMEDIA, *LPLINEMEDIACONTROLMEDIA;

typedef struct linemediacontroltone_tag {
    DWORD    dwAppSpecific;
    DWORD    dwDuration;
    DWORD    dwFrequency1;
    DWORD    dwFrequency2;
    DWORD    dwFrequency3;
    DWORD    dwMediaControl;
} LINEMEDIACONTROLTONE, *LPLINEMEDIACONTROLTONE;

#define LINEMEDIACONTROL_NONE           0x00000001
#define LINEMEDIACONTROL_START          0x00000002
#define LINEMEDIACONTROL_RESET          0x00000004
#define LINEMEDIACONTROL_PAUSE          0x00000008
#define LINEMEDIACONTROL_RESUME         0x00000010
#define LINEMEDIACONTROL_RATEUP         0x00000020
#define LINEMEDIACONTROL_RATEDOWN       0x00000040
#define LINEMEDIACONTROL_RATENORMAL     0x00000080
#define LINEMEDIACONTROL_VOLUMEUP       0x00000100
#define LINEMEDIACONTROL_VOLUMEDOWN     0x00000200
#define LINEMEDIACONTROL_VOLUMENORMAL   0x00000400



#define LINEMEDIAMODE_UNKNOWN           0x00000002
#define LINEMEDIAMODE_INTERACTIVEVOICE  0x00000004
#define LINEMEDIAMODE_AUTOMATEDVOICE    0x00000008
#define LINEMEDIAMODE_DATAMODEM         0x00000010
#define LINEMEDIAMODE_G3FAX             0x00000020
#define LINEMEDIAMODE_TDD               0x00000040
#define LINEMEDIAMODE_G4FAX             0x00000080
#define LINEMEDIAMODE_DIGITALDATA       0x00000100
#define LINEMEDIAMODE_TELETEX           0x00000200
#define LINEMEDIAMODE_VIDEOTEX          0x00000400
#define LINEMEDIAMODE_TELEX             0x00000800
#define LINEMEDIAMODE_MIXED             0x00001000
#define LINEMEDIAMODE_ADSI              0x00002000
#define LINEMEDIAMODE_VOICEVIEW         0x00004000

#define LAST_LINEMEDIAMODE              0x00004000

typedef struct linemonitortone_tag {
    DWORD    dwAppSpecific;
    DWORD    dwDuration;
    DWORD    dwFrequency1;
    DWORD    dwFrequency2;
    DWORD    dwFrequency3;
} LINEMONITORTONE, *LPLINEMONITORTONE;

typedef struct linereqmakecall_tag {
    char    szDestAddress[TAPIMAXDESTADDRESSSIZE];
    char    szAppName[TAPIMAXAPPNAMESIZE];
    char    szCalledParty[TAPIMAXCALLEDPARTYSIZE];
    char    szComment[TAPIMAXCOMMENTSIZE];
} LINEREQMAKECALL, *LPLINEREQMAKECALL;


typedef struct linetranslateoutput_tag {
    DWORD dwTotalSize;
    DWORD dwNeededSize;
    DWORD dwUsedSize;

    DWORD dwDialableStringSize;
    DWORD dwDialableStringOffset;
    DWORD dwDisplayableStringSize;
    DWORD dwDisplayableStringOffset;

    DWORD dwCurrentCountry;
    DWORD dwDestCountry;
    DWORD dwTranslateResults;
} LINETRANSLATEOUTPUT, *LPLINETRANSLATEOUTPUT;

typedef struct linetranslatecaps_tag {
    DWORD dwTotalSize;
    DWORD dwNeededSize;
    DWORD dwUsedSize;

    DWORD dwNumLocations;
    DWORD dwLocationListSize;
    DWORD dwLocationListOffset;
    
    DWORD dwCurrentLocationID;
    
    DWORD dwNumCards;
    DWORD dwCardListSize;
    DWORD dwCardListOffset;

    DWORD dwCurrentPreferredCardID;
} LINETRANSLATECAPS, *LPLINETRANSLATECAPS;

typedef struct linelocationentry_tag {
    DWORD dwPermanentLocationID;
    DWORD dwLocationNameSize;
    DWORD dwLocationNameOffset;
    DWORD dwCountryCode;
    DWORD dwCityCodeSize;
    DWORD dwCityCodeOffset;
    DWORD dwPreferredCardID;
    // From here down are new fields in TAPI32
    DWORD  dwLocalAccessCodeSize;
    DWORD  dwLocalAccessCodeOffset;
    DWORD  dwLongDistanceAccessCodeSize;
    DWORD  dwLongDistanceAccessCodeOffset;
    DWORD  dwTollPrefixListSize;
    DWORD  dwTollPrefixListOffset;
    DWORD  dwCountryID;
    DWORD  dwOptions;
    DWORD  dwCancelCallWaitingSize;
    DWORD  dwCancelCallWaitingOffset;
} LINELOCATIONENTRY, *LPLINELOCATIONENTRY;

#define LINETRANSLATEOPTION_CARDOVERRIDE  0x00000001
#define LINETRANSLATEOPTION_CANCELCALLWAITING      0x00000002
#define LINETRANSLATEOPTION_FORCELOCAL             0x00000004
#define LINETRANSLATEOPTION_FORCELD                0x00000008

#define LINETRANSLATERESULT_CANONICAL              0x00000001
#define LINETRANSLATERESULT_INTERNATIONAL          0x00000002
#define LINETRANSLATERESULT_LONGDISTANCE           0x00000004
#define LINETRANSLATERESULT_LOCAL                  0x00000008
#define LINETRANSLATERESULT_INTOLLLIST             0x00000010
#define LINETRANSLATERESULT_NOTINTOLLLIST          0x00000020
#define LINETRANSLATERESULT_DIALBILLING            0x00000040
#define LINETRANSLATERESULT_DIALQUIET              0x00000080
#define LINETRANSLATERESULT_DIALDIALTONE           0x00000100
#define LINETRANSLATERESULT_DIALPROMPT             0x00000200


#define LINELOCATIONOPTION_PULSEDIAL               0x00000001

#define LINECARDOPTION_PREDEFINED              0x00000001
#define LINECARDOPTION_HIDDEN                  0x00000002


#define INITIALIZE_NEGOTIATION                     0xffffffff

// Tapi Address Translation procedures
LONG WINAPI lineSetCurrentLocation(
    HLINEAPP hLineApp,
    DWORD dwLocation);

LONG WINAPI lineTranslateAddress(
    HLINEAPP hLineApp,
    DWORD dwDeviceID,
    DWORD dwAPIVersion,
    LPCTSTR lpszAddressIn,
    DWORD dwCard,
    DWORD dwTranslateOptions,
    LPLINETRANSLATEOUTPUT lpTranslateOutput);

LONG WINAPI lineGetTranslateCaps(
    HLINEAPP hLineApp,
    DWORD dwAPIVersion,
    LPLINETRANSLATECAPS lpTranslateCaps);


// Tapi function prototypes

LONG WINAPI lineClose(
        HLINE hLine);

LONG WINAPI lineConfigDialog(
        DWORD dwDeviceID,
        HWND  hwndOwner,
        LPCTSTR lpszDeviceClass);

LONG WINAPI lineDeallocateCall(
        HCALL hCall);

LONG WINAPI lineDrop(
        HCALL hCall, 
        LPCTSTR lpsUserUserInfo, 
        DWORD dwSize);

LONG WINAPI lineGetDevCaps(
        HLINEAPP hLineApp, 
        DWORD dwDeviceID, 
        DWORD dwAPIVersion,
        DWORD dwExtVersion, 
        LPLINEDEVCAPS lpLineDevCaps);

LONG WINAPI lineGetDevConfig(
        DWORD dwDeviceID,
        LPVARSTRING lpDeviceConfig,
        LPCTSTR lpszDeviceClass);

LONG WINAPI lineGetID(
        HLINE hLine,
        DWORD dwAddressID, 
        HCALL hCall,
        DWORD dwSelect,        
        LPVARSTRING lpDeviceID, 
        LPCTSTR lpszDeviceClass);

LONG WINAPI lineInitialize(
        LPHLINEAPP lphLineApp,
        HINSTANCE hInstance,
        LINECALLBACK lpfnCallback,
        LPCTSTR lpszAppName,
        LPDWORD lpdwNumDevs);

LONG WINAPI lineMakeCall(
        HLINE hLine, 
        LPHCALL lphCall, 
        LPCTSTR lpszDestAddress, 
        DWORD dwCountryCode, 
        LPLINECALLPARAMS const lpCallParams);

LONG WINAPI lineNegotiateAPIVersion(
        HLINEAPP hLineApp,
        DWORD dwDeviceID,
        DWORD dwAPILowVersion,
        DWORD dwAPIHighVersion,
        LPDWORD lpdwAPIVersion,
        LPLINEEXTENSIONID lpExtensionID);

LONG WINAPI lineNegotiateExtVersion(
        HLINEAPP hLineApp,
        DWORD dwDeviceID,
        DWORD dwAPIVersion,
        DWORD dwExtLowVersion,
        DWORD dwExtHighVersion,
        LPDWORD lpdwExtVersion);

LONG WINAPI lineOpen(
        HLINEAPP hLineApp, 
        DWORD dwDeviceID,
        LPHLINE lphLine,
        DWORD dwAPIVersion,
        DWORD dwExtVersion,
        DWORD dwCallbackInstance,
        DWORD dwPrivileges,
        DWORD dwMediaModes,
        LPLINECALLPARAMS const lpCallParams);

LONG WINAPI lineSetDevConfig(
        DWORD dwDeviceID,
        LPVOID const lpDeviceConfig,
        DWORD dwSize,
        LPCTSTR lpszDeviceClass);

LONG WINAPI lineSetStatusMessages(
        HLINE hLine, 
        DWORD dwLineStates, 
        DWORD dwAddressStates);

LONG WINAPI lineShutdown(
        HLINEAPP hLineApp);

LONG WINAPI lineTranslateDialog(
	HLINEAPP hLineApp,
	DWORD dwDeviceID,
	DWORD dwAPIVersion,
	HWND hwndOwner,
	LPCTSTR lpszAddressIn);

LONG WINAPI lineConfigDialogEdit(
	DWORD dwDeviceID,
	HWND hwndOwner,
	LPCTSTR lpszDeviceClass,
	LPVOID const lpDeviceConfigIn,
	DWORD dwSize,
	LPVARSTRING lpDeviceConfigOut);

LONG WINAPI lineAddProvider(
    LPCTSTR lpszProviderFilename,
    HWND hwndOwner,
    LPDWORD lpdwPermanentProviderID);

LONG WINAPI lineSetCurrentLocation(
    HLINEAPP hLineApp,
    DWORD dwLocation);
    

#if defined(WINCEOEM) && defined(UNDER_CE)
#include <ptapi.h>
#ifdef WINCEMACRO
#include <mtapi.h>
#endif
#endif

#ifdef __cplusplus
}                       /* End of extern "C" { */
#endif   /* __cplusplus */

#pragma pack()

#endif // TAPI_H

// @CESYSGEN ENDIF
