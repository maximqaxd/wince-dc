/*++

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1995-1998  Microsoft Corporation

Module Name:

ras.h

Abstract:

Remote Access Service structures and defines

Notes: 

--*/

// @CESYSGEN IF CE_MODULES_PPP
#ifndef _RAS_H_
#define _RAS_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef UNLEN
#include <lmcons.h>
#endif
#include <tapi.h>

#include "pshpack4.h"

#define RAS_MaxEntryName      20
#define RAS_MaxDeviceName     32
#define RAS_MaxDeviceType     16
#define RAS_MaxParamKey       32
#define RAS_MaxParamValue     128
#define RAS_MaxPhoneNumber    128
#define RAS_MaxCallbackNumber 48
#define RAS_MaxIpAddress      15
#define RAS_MaxIpxAddress     21

// Ras extensions

#define	RAS_MaxAreaCode			(10)			// TBD
#define	RAS_MaxPadType			(32)			// TBD
#define	RAS_MaxX25Address		(200)			// TBD
#define	RAS_MaxFacilities		(200)			// TBD
#define	RAS_MaxUserData			(200)			// TBD

// RAS IP Address

typedef	struct	tagRasIpAddr
{
	BYTE	a;
	BYTE	b;
	BYTE	c;
	BYTE	d;
}
RASIPADDR;

// RAS Entry Definition

typedef	struct	tagRasEntry	
{
	DWORD		dwSize;
	DWORD		dwfOptions;
	DWORD		dwCountryID;
	DWORD		dwCountryCode;
	TCHAR		szAreaCode[ RAS_MaxAreaCode + 1 ];
	TCHAR		szLocalPhoneNumber[ RAS_MaxPhoneNumber + 1 ];
	DWORD		dwAlternatesOffset;
	RASIPADDR	ipaddr;
	RASIPADDR	ipaddrDns;
	RASIPADDR	ipaddrDnsAlt;
	RASIPADDR	ipaddrWins;
	RASIPADDR	ipaddrWinsAlt;
	DWORD		dwFrameSize;
	DWORD		dwfNetProtocols;
	DWORD		dwFramingProtocol;
	TCHAR		szScript[ MAX_PATH ];
	TCHAR		szAutoDialDll[ MAX_PATH ];
	TCHAR		szAutoDialFunc[ MAX_PATH ];
	TCHAR		szDeviceType[ RAS_MaxDeviceType + 1 ];
	TCHAR		szDeviceName[ RAS_MaxDeviceName + 1 ];
	TCHAR		szX25PadType[ RAS_MaxPadType + 1 ];
	TCHAR		szX25Address[ RAS_MaxX25Address + 1 ];
	TCHAR		szX25Facilities[ RAS_MaxFacilities + 1 ];
	TCHAR		szX25UserData[ RAS_MaxUserData + 1 ];
	DWORD		dwChannels;
	DWORD		dwReserved1;
	DWORD		dwReserved2;
}
RASENTRY, *LPRASENTRY;

/* Describes RAS Device Information
*/
typedef struct  tagRasDevInfoW {
	DWORD       dwSize;
	WCHAR       szDeviceType[RAS_MaxDeviceType+1];
	WCHAR       szDeviceName[RAS_MaxDeviceName+1];
} RASDEVINFOW;
typedef RASDEVINFOW * LPRASDEVINFOW;

/* CE is unicode only
*/
#define RASDEVINFO		RASDEVINFOW
#define LPRASDEVINFO	LPRASDEVINFOW

/* RASENTRY 'dwfOptions' bit flags.
*/
#define RASEO_UseCountryAndAreaCodes  0x00000001
#define RASEO_SpecificIpAddr          0x00000002
#define RASEO_SpecificNameServers     0x00000004
#define RASEO_IpHeaderCompression     0x00000008
#define RASEO_RemoteDefaultGateway    0x00000010
#define RASEO_DisableLcpExtensions    0x00000020
#define RASEO_TerminalBeforeDial      0x00000040
#define RASEO_TerminalAfterDial       0x00000080
#define RASEO_ModemLights             0x00000100
#define RASEO_SwCompression           0x00000200
#define RASEO_RequireEncryptedPw      0x00000400
#define RASEO_RequireMsEncryptedPw    0x00000800
#define RASEO_RequireDataEncryption   0x00001000
#define RASEO_NetworkLogon            0x00002000
#define RASEO_UseLogonCredentials     0x00004000
#define RASEO_PromoteAlternates       0x00008000
#define RASEO_SecureLocalFiles        0x00010000
#define RASEO_DialAsLocalCall         0x00020000


/* RASENTRY 'dwfNetProtocols' bit flags. (session negotiated protocols)
*/
#define RASNP_Netbeui       0x00000001  // Negotiate NetBEUI
#define RASNP_Ipx           0x00000002  // Negotiate IPX
#define RASNP_Ip            0x00000004  // Negotiate TCP/IP


/* RASENTRY 'dwFramingProtocols' (framing protocols used by the server)
*/
#define RASFP_Ppp           0x00000001  // Point-to-Point Protocol (PPP)
#define RASFP_Slip          0x00000002  // Serial Line Internet Protocol (SLIP)
#define RASFP_Ras           0x00000004  // Microsoft proprietary protocol


/* RASENTRY 'szDeviceType' strings
*/
#define RASDT_Direct        TEXT("direct")    // Direct Connect (WINCE Extension)
#define RASDT_Modem         TEXT("modem")     // Modem
#define RASDT_Isdn          TEXT("isdn")      // ISDN
#define RASDT_X25           TEXT("x25")       // X.25



DECLARE_HANDLE( HRASCONN );
typedef HRASCONN *LPHRASCONN;

// Identifies an active RAS connection.  (See RasEnumConnections)

typedef struct tagRASCONN
{
    DWORD    dwSize;
    HRASCONN hrasconn;
    TCHAR    szEntryName[ RAS_MaxEntryName + 1 ];
} 
RASCONN, *LPRASCONN;

// Enumerates intermediate states to a connection.  (See RasDial)

#define RASCS_PAUSED 0x1000
#define RASCS_DONE   0x2000

typedef enum tagRASCONNSTATE
{
    RASCS_OpenPort = 0,
    RASCS_PortOpened,
    RASCS_ConnectDevice,
    RASCS_DeviceConnected,
    RASCS_AllDevicesConnected,
    RASCS_Authenticate,
    RASCS_AuthNotify,
    RASCS_AuthRetry,
    RASCS_AuthCallback,
    RASCS_AuthChangePassword,
    RASCS_AuthProject,
    RASCS_AuthLinkSpeed,
    RASCS_AuthAck,
    RASCS_ReAuthenticate,
    RASCS_Authenticated,
    RASCS_PrepareForCallback,
    RASCS_WaitForModemReset,
    RASCS_WaitForCallback,
    RASCS_Projected,

    RASCS_Interactive 			= RASCS_PAUSED,
    RASCS_RetryAuthentication,
    RASCS_CallbackSetByCaller,
    RASCS_PasswordExpired,

    RASCS_Connected 			= RASCS_DONE,
    RASCS_Disconnected
} 
RASCONNSTATE, *LPRASCONNSTATE;


// Describes the status of a RAS connection.  (See RasConnectionStatus)

typedef struct tagRASCONNSTATUS
{
    DWORD        dwSize;
    RASCONNSTATE rasconnstate;
    DWORD        dwError;
    TCHAR        szDeviceType[ RAS_MaxDeviceType + 1 ];
    TCHAR        szDeviceName[ RAS_MaxDeviceName + 1 ];
}
RASCONNSTATUS, *LPRASCONNSTATUS;


// Describes connection establishment parameters.  (See RasDial)

typedef struct tagRASDIALPARAMS
{
    DWORD dwSize;
    TCHAR szEntryName[ RAS_MaxEntryName + 1 ];
    TCHAR szPhoneNumber[ RAS_MaxPhoneNumber + 1 ];
    TCHAR szCallbackNumber[ RAS_MaxCallbackNumber + 1 ];
    TCHAR szUserName[ UNLEN + 1 ];
    TCHAR szPassword[ PWLEN + 1 ];
    TCHAR szDomain[ DNLEN + 1 ];
}
RASDIALPARAMS, *LPRASDIALPARAMS;


// Describes extended connection establishment options.  (See RasDial)

typedef struct tagRASDIALEXTENSIONS
{
    DWORD dwSize;
    DWORD dwfOptions;
    HWND  hwndParent;
    DWORD reserved;
} 
RASDIALEXTENSIONS, *LPRASDIALEXTENSIONS;

// 'dwfOptions' bit flags.

#define RDEOPT_UsePrefixSuffix           0x00000001
#define RDEOPT_PausedStates              0x00000002
#define RDEOPT_IgnoreModemSpeaker        0x00000004
#define RDEOPT_SetModemSpeaker           0x00000008
#define RDEOPT_IgnoreSoftwareCompression 0x00000010
#define RDEOPT_SetSoftwareCompression    0x00000020


// Describes an enumerated RAS phone book entry name.  (See RasEntryEnum)

typedef struct tagRASENTRYNAME
{
    DWORD dwSize;
    TCHAR szEntryName[ RAS_MaxEntryName + 1 ];
} 
RASENTRYNAME, *LPRASENTRYNAME;


// Describes results of a GET_WINS/DNS I/O control. If
// the requested address has been negotiated the boolean
// valid is TRUE and the IpAddress contains the address of
// the requested server.  If Valid is false IpAddress is
// zero.

typedef	struct	tagRASPPPAddr
{
	DWORD	dwSize;
	DWORD	dwError;
	BOOL	Valid;
	DWORD	IpAddress;
}
RASPPPADDR;

// 	RAS Control Structures
//
// 	Control Request Enumeration

typedef	enum	tagRasCntlEnum
{
	RASCNTL_SET_DEBUG,							// set debug control params
	RASCNTL_LOCK_STATUS,						// get session's lock status
	RASCNTL_PRINT_CS,							// print cs info
	RASCNTL_STATISTICS,							// Get statistics
	RASCNTL_ENUMDEV								// Enum Devices

	// Add others here
}
RasCntlEnum_t;

// 
// RasDial message notifications are sent with Message ID set to
// WM_RASDIALEVENT
//

#define RASDIALEVENT    	"RasDialEvent"

#define WM_RASDIALEVENT 	0xCCCD


#ifdef UNDER_CE

// This structure is used by the RnaApp application on WINCE
// to signal when a connection has occured.

typedef struct tagRNAAppInfo {
	DWORD	dwSize;					// The size of this structure
	DWORD	hWndRNAApp;				// The handle of the RNAApp window
	DWORD	Context;				// Context value specified on CmdLine
	DWORD	ErrorCode;				// Last error code
	TCHAR	RasEntryName[RAS_MaxEntryName+1];
} RNAAPP_INFO, *PRNAAPP_INFO;


#define RNA_RASCMD		WM_USER+1
#define	RNA_ADDREF		1
#define RNA_DELREF		2
#endif

#include "poppack.h"

#ifndef RASAPI
#define RASAPI
#endif

DWORD RASAPI RasDial (LPRASDIALEXTENSIONS dialExtensions,
	       LPTSTR phoneBookPath,
	       LPRASDIALPARAMS rasDialParam,
	       DWORD NotifierType,
	       LPVOID notifier,
	       LPHRASCONN pRasConn);
DWORD RASAPI RasHangup (HRASCONN Session);	// Kept for historical compatibility
DWORD RASAPI RasHangUp (HRASCONN Session);
DWORD RASAPI RasEnumEntries (LPWSTR Reserved,
		      LPWSTR lpszPhoneBookPath,
		      LPRASENTRYNAME lprasentryname,
		      LPDWORD lpcb,
		      LPDWORD lpcEntries);
DWORD RASAPI RasGetEntryDialParams (LPWSTR lpszPhoneBook,
			     LPRASDIALPARAMS lpRasDialParams,	
			     LPBOOL lpfPassword);
DWORD RASAPI RasSetEntryDialParams (LPWSTR lpszPhoneBook,
			     LPRASDIALPARAMS lpRasDialParams,
			     BOOL fRemovePassword);
DWORD RASAPI RasGetEntryProperties (LPWSTR lpszPhoneBook,
			     LPWSTR szEntry,
			     LPRASENTRY lpEntry,
			     LPDWORD lpdwEntrySize,
			     LPBYTE lpb,
			     LPDWORD lpdwSize);
DWORD RASAPI RasSetEntryProperties (LPWSTR lpszPhoneBook,		
			     LPWSTR szEntry,
			     LPRASENTRY lpbEntry,
			     DWORD dwEntrySize,
			     LPBYTE lpb,
			     DWORD dwSize);
DWORD RASAPI RasValidateEntryName (LPWSTR lpszPhonebook,		
			    LPWSTR lpszEntry);
DWORD RASAPI RasDeleteEntry (LPWSTR lpszPhonebook,
		      LPWSTR lpszEntry);
DWORD RASAPI RasRenameEntry (LPWSTR lpszPhonebook,
		      LPWSTR lpszOldEntry,
		      LPWSTR lpszNewEntry);
DWORD RASAPI RasEnumConnections (LPRASCONN lprasconn,
			  LPDWORD lpcb,
			  LPDWORD lpcConnections);
DWORD RASAPI RasGetConnectStatus (HRASCONN rasconn,				
			   LPRASCONNSTATUS lprasconnstatus);
DWORD RASAPI RasGetEntryDevConfig (LPCTSTR szPhonebook,
			    LPCTSTR szEntry,
			    LPDWORD pdwDeviceID,
			    LPDWORD pdwSize,
			    LPVARSTRING pDeviceConfig);
DWORD RASAPI RasSetEntryDevConfig (LPCTSTR szPhonebook,
			    LPCTSTR szEntry,
			    DWORD dwDeviceID,
				LPVARSTRING lpDeviceConfig);

DWORD APIENTRY RasEnumDevicesW( LPRASDEVINFOW, LPDWORD, LPDWORD );
#define RasEnumDevices          RasEnumDevicesW

#ifdef __cplusplus
}
#endif


#ifdef WINCEOEM
#include <pras.h>	// internal defines 
#endif

#endif // _RAS_H_

// @CESYSGEN ENDIF
