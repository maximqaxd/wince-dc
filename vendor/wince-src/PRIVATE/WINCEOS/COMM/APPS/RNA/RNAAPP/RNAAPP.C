/******************************************************************************
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

RnaApp.c : Remote networking worker program

	@doc EX_RNAAPP

	@topic     RNAApp |
		This program is used to create and maintain a RAS connection.


	NOTE: This is an internal program used by the Remote Networking application.
	Its' command line parameters and existance in Windows CE may change from release
	to release as we add more features or change internal architecture of the components.
		
	The RNAApp.exe program will attempt to create a RAS connection. This
	is a worker app used by RemNet and tends to change with every release.
	In general apps should use RasDial() themselves so they they can maintain
	their own state

	Usage:
	RNAAPP [-n -p -m -cNUM] -eENTRYNAME

	Option		Description
	-n			Disables most error message boxes, used when calling
				application will display the error values
	-p			By default it will prompt for username/password for
				dial-up entries, if this is specified it will skip the initial
				prompt
	-m			Minimize on connection.
	-cNUM		Set the context value to NUM (this should be a HEX value,
				eg. -c54AF)
	-eENTRYNAME	The name of the RASEntry to use.  If the name contains any
				embedded spaces then this should be enclosed in double quotes.

	Examples:
	rnaapp -m -eDirect
	rnaapp -e"Name with spaces"


	RNAApp will send a broadcast message to all applications when a
	connection succeeds (or fails).  This broadcast message is sent as follows:

	
	SendNotifyMessage( HWND_BROADCAST, WM_NETCONNECT, (WPARAM)TRUE,
		(LPARAM)&v_RNAAppInfo );

	The wParam of the message is a boolean that indicates either connection
	success (TRUE) or termination of the connection (FALSE).	

	The lParam of the message is a pointer to the following structure
	(defined in ras.h):
	
	typedef struct tagRNAAppInfo {
		DWORD	dwSize;					// The size of this structure
		DWORD	hWndRNAApp;				// The handle of the RNAApp window
		DWORD	Context;				// Context value specified on CmdLine
		DWORD	ErrorCode;				// Last error code
		TCHAR	RasEntryName[RAS_MaxEntryName+1];
	} RNAAPP_INFO, *PRNAAPP_INFO;

	The structure elements are defined as follows:

	dwSize		Size of the structure, to be certain of tracking version
				changes you should compare this against the
				sizeof(RNAAPP_INFO).
	hWndRNAApp	The window handle of the RNAApp program (see below).
	Context		The context specified on the command line.
	ErrorCode	The error code (only valid if wParam == FALSE). See below for
				the list of error codes.
	RasEntryName The RAS entry name specified on the command line.

	Error Codes (defined in raserror.h)
	ERROR_PORT_NOT_AVAILABLE
		Port was not available.
	ERROR_PORT_DISCONNECTED
		After sucessfully connecting the port was disconnected.
	ERROR_NO_CARRIER
		No carrier was detected by the modem.
	ERROR_NO_DIALTONE
		No dialtone was detected by the modem (not all modems support this).
	ERROR_DEVICE_NOT_READY
		The device is not ready (for PCMCIA modems the device might not be
		inserted).
	ERROR_LINE_BUSY
		The modem detected a busy signal.
	ERROR_NO_ANSWER
		No one answered the phone
	ERROR_POWER_OFF
		The serial device returned indication that power had been turned off.
	ERROR_POWER_OFF_CD
		The serial device returned indication that power had been turned off,
		and that Carrier Detect was currently asserted.  This is an indication
		that we are still in the docking station.
	ERROR_USER_DISCONNECTION
		The user has disconnected the connection (by pressing the
		disconnect/cancel button)
	ERROR_DISCONNECTION
		Disconnected for an unknow reason.
	ERROR_INVALID_PARAMETER
		Invalid or missing parameter.  The -e parameter is required.
	ERROR_STATE_MACHINES_ALREADY_STARTED
		The system cannot establish another RAS connection
	ERROR_CANNOT_FIND_PHONEBOOK_ENTRY
		Unable to find specified RAS entry.
	ERROR_EVENT_INVALID
		Internal error.
				

	Sending messages to the RNAApp.
		The calling application can send messages to RNAApp.  These should
		be sent in the form:
		
		SendMessage (hWnd, RNA_RASCMD, <CMD>, <INFO>);

		Where <CMD> is one of the following:
		RNA_ADDREF	Add a reference to the current connection. <INFO> should
					be 0.
		RNA_DELREF	Delete a reference to the current connection.  If the
					reference count is decremented to zero then the
					connection is dropped as if the user had selected
					Disconnect. <INFO> should be 0.
		RNA_GETINFO	Will send a WM_NETCONNET message to the window specified
					in the <INFO> parameter.  This allows an application to
					inquire what the entryname of this instance of RNAApp is.

		Finding instances of RNAApp.  By creating a function as follows:

		BOOL FindRNAAppWindow(HWND hWnd, LPARAM lParam)
		{
			TCHAR  	szClassName[32];

			GetClassName (hWnd, szClassName,
				sizeof(szClassName)/sizeof(TCHAR));

			if (!_tcscmp (szClassName, TEXT("Dialog")) &&
				(RNAAPP_MAGIC_NUM == GetWindowLong (hWnd, DWL_USER))) {
				*((HWND *)lParam) = hWnd;
				return FALSE;
				}
			return TRUE;
		}

		And then performing the following command:
		
		EnumWindows(FindRNAAppWindow, (LPARAM)&hOldRNAWnd);

		You can determine the window handle of the currently running instance
		of RNAApp.exe (if any).
		

******************************************************************************/


#include <windows.h>
#include <tchar.h>
#include "string.h"
#include "memory.h"
#include "commctrl.h"
#include "raserror.h"
#include "notify.h"
#include "ras.h"
#include "tapihelp.h"
#include "resource.h"
#include "windev.h"     // for IsAPIReady()
#ifdef USE_SIP
#	include <sipapi.h>
	typedef BOOL (WINAPI* LPFNSIP)(SIPINFO*);
	typedef DWORD (WINAPI* LPFNSIPSTATUS)();
#endif

// direct macro for Shell_NotifyIcon, so that we have it even if the COREDLL thunk is absent
#define Shell_NotifyIcon PRIV_IMPLICIT_DECL(BOOL, SH_SHELL, 6, (DWORD,PNOTIFYICONDATA))

#define SIP_UP      0
#define SIP_DOWN    1

// Globals

const TCHAR     szAppName[] = TEXT("rnaapp");
TCHAR       	EntryName[RAS_MaxEntryName + 1];
RASENTRY    	v_RasEntry;   // The Entry.
HWND        	v_hMainWnd;
HWND        	v_hStatusWnd;
HWND        	v_hCancelBtn;
HWND        	v_hDismissBtn;
HINSTANCE   	v_hInstance;
HRASCONN    	v_hRasConn;
RASDIALPARAMS   v_RasDialParams;
BOOL        	v_fMinOnConnect;
BOOL			v_fNoPassword;
BOOL			v_fNoMsgBox;
BOOL			v_fInErrorBox;
BOOL			v_fReturnFalse;
DWORD			v_RefCount;
DWORD			v_RetryCnt;
BOOL            v_fPassword;
HICON           v_hDirectIcon;
HICON           v_hDialupIcon;
HICON			v_hVPNIcon;
HICON			v_hNotifyIcon;
DWORD			v_dwDeviceID;
RNAAPP_INFO		v_RNAAppInfo;
#ifdef USE_SIP
LPFNSIP         v_lpfnSipGetInfo;
LPFNSIP         v_lpfnSipSetInfo;
LPFNSIPSTATUS	v_lpfnSipStatus;
HINSTANCE 		v_hSipLib;
#endif

// Locals

static	BOOL	NetDisconnect;
static	BOOL	CredentialPrompt;
static  HWND    hCredentials;

#define IDM_TASKBAR_NOTIFY  (WM_USER + 200)
#define IDM_START_RASDIAL   (WM_USER + 201)
	

// Debug Zones 

#ifdef DEBUG
DBGPARAM dpCurSettings = 
{
    TEXT("rnaapp"), 
	{
		TEXT("Unused"),TEXT("Unused"),TEXT("Unused"),TEXT("Unused"),
		TEXT("Unused"),TEXT("Unused"),TEXT("Unused"),TEXT("Unused"),
		TEXT("Unused"),TEXT("Ras"),TEXT("Tapi"),TEXT("Misc"),
		TEXT("Alloc"),TEXT("Function"),TEXT("Warning"),TEXT("Error") 
	},
    0x00000000
}; 

#define ZONE_RAS        DEBUGZONE(9)	    // 0x0200
#define ZONE_TAPI       DEBUGZONE(10)       // 0x0400
#define ZONE_MISC       DEBUGZONE(11)       // 0x0800
#define ZONE_ALLOC      DEBUGZONE(12)       // 0x1000
#define ZONE_FUNCTION   DEBUGZONE(13)       // 0x2000
#define ZONE_WARN       DEBUGZONE(14)       // 0x4000
#define ZONE_ERROR      DEBUGZONE(15)       // 0x8000

#endif  // DEBUG


// What size should we be?

#define SCREEN_WIDTH    240
#define SCREEN_HEIGTH   80


void CALLBACK 
lineCallbackFunc(DWORD dwDevice, DWORD dwMsg, DWORD dwCallbackInstance, 
				 DWORD dwParam1, DWORD dwParam2, DWORD dwParam3)
{
   return;
}
void
ShowError (DWORD Message, DWORD Title)
{
    TCHAR           szTemp[218];
    TCHAR           szTemp2[128];
	
	if( v_fNoMsgBox ) {
		return;
	}
	LoadString (v_hInstance, Message, szTemp, sizeof(szTemp)/sizeof(TCHAR));
	LoadString (v_hInstance, Title, szTemp2, sizeof(szTemp2)/sizeof(TCHAR));
	MessageBox( v_hMainWnd, szTemp, szTemp2, MB_SETFOREGROUND | MB_APPLMODAL | MB_OK |
				MB_ICONEXCLAMATION);
}
void
SetTapiDisplayStuff(HWND hDlg)
{
    TCHAR	szPhone[256];
    LPTSTR  pLocName;

	szPhone[0] = TEXT('\0');

	RasGetDispPhoneNum (NULL, EntryName, szPhone, sizeof(szPhone));
	SetWindowText(GetDlgItem(hDlg, IDC_PHONE), szPhone);
        
    if (pLocName = TapiGetDefaultLocationName()) {
        SetWindowText(GetDlgItem(hDlg, IDC_DIALFROM), pLocName);
        LocalFree(pLocName);
    }    
    return;
}

LRESULT CALLBACK
DlgProcCredentials( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam )
{
    BOOL RemovePassword;
    static BOOL fReEnable = FALSE;

    switch( message ) {
    case WM_INITDIALOG:
		// Since main window is hidden, I need to bring myself to the
		// foreground
		//
		SetForegroundWindow ( hDlg );
		if (!_tcscmp (v_RasEntry.szDeviceType, RASDT_Modem)) {
            SendMessage(GetDlgItem(hDlg, IDC_STATUSICON), STM_SETIMAGE,
                        IMAGE_ICON, (LPARAM) v_hDialupIcon);
		} else if (!_tcscmp (v_RasEntry.szDeviceType, RASDT_Direct)) {
            SendMessage(GetDlgItem(hDlg, IDC_STATUSICON), STM_SETIMAGE,
                        IMAGE_ICON, (LPARAM) v_hDirectIcon);
		} else if (!_tcscmp (v_RasEntry.szDeviceType, RASDT_Vpn)) {
            SendMessage(GetDlgItem(hDlg, IDC_STATUSICON), STM_SETIMAGE,
                        IMAGE_ICON, (LPARAM) v_hVPNIcon);
		} else {
			RETAILMSG (1, (TEXT("Error, unhandled Device Type\r\n")));
			ASSERT (0);
		}


		hCredentials = hDlg;				// save handle for cancel

        SetWindowText(GetDlgItem(hDlg, IDC_ENTRYNAME), EntryName);

        SetTapiDisplayStuff(hDlg);
        
        SetDlgItemText( hDlg, IDC_USERNAME, v_RasDialParams.szUserName );
        SetDlgItemText( hDlg, IDC_PASSWORD, v_RasDialParams.szPassword );
        SetDlgItemText( hDlg, IDC_DOMAIN, v_RasDialParams.szDomain );

        if (v_fPassword) {
            // A password has been saved so check the box
            SendMessage(GetDlgItem(hDlg, IDC_SAVEPASSWORD), BM_SETCHECK,1,0);
        }
		
        if (v_RasDialParams.szUserName[0] == TEXT('\0')) {
            SetFocus(GetDlgItem(hDlg, IDC_USERNAME));
		} else {
            SetFocus(GetDlgItem(hDlg, IDC_PASSWORD));
		}
        return FALSE;

    case WM_COMMAND:
        
        switch( LOWORD( wParam ) ) {
        case IDCANCEL: 				// Ignore values

            EndDialog( hDlg, FALSE );
            break;

        case IDCONNECT: 					// Get the dialog values

            GetDlgItemText( hDlg, IDC_USERNAME,
							v_RasDialParams.szUserName, UNLEN + 1);

            GetDlgItemText( hDlg, IDC_PASSWORD,
							v_RasDialParams.szPassword, PWLEN + 1);

            GetDlgItemText( hDlg, IDC_DOMAIN,
							v_RasDialParams.szDomain, DNLEN + 1);

            if (SendMessage(GetDlgItem(hDlg, IDC_SAVEPASSWORD),
							BM_GETCHECK, 0, 0)) {
                RemovePassword = FALSE;
            } else {
                RemovePassword = TRUE;
            }
            RasSetEntryDialParams( NULL, &v_RasDialParams, RemovePassword );
            EndDialog( hDlg, TRUE );
            break;

        case IDDIALPROPERTIES:
            lineTranslateDialog(v_hLineApp, v_dwDeviceID, v_dwVersion,
								hDlg, NULL);
            SetTapiDisplayStuff(hDlg);
            break;
            
#ifdef USE_SIP
        case IDC_PASSWORD:
            if (v_lpfnSipGetInfo && v_lpfnSipSetInfo &&
                ((HIWORD(wParam) == EN_SETFOCUS) || 
                ((HIWORD(wParam) == EN_KILLFOCUS) &&
                  fReEnable)))
            {
                SIPINFO si;

                memset(&si, 0, sizeof(SIPINFO));
                si.cbSize = sizeof(SIPINFO);
                if ((*v_lpfnSipGetInfo)(&si))
                {
                    // Toggle password completion, but keep track of 
                    // whether or not this flag was set a priori when setting focus
                    if (HIWORD(wParam) == EN_SETFOCUS)
                    {
                        fReEnable = !(si.fdwFlags & SIPF_DISABLECOMPLETION);
                        si.fdwFlags |= SIPF_DISABLECOMPLETION;
                    }
                    else
                    {
                        si.fdwFlags &= ~SIPF_DISABLECOMPLETION;
                    }
		            (*v_lpfnSipSetInfo)(&si);
                }
            }
            break;
#endif

        default:

            break;
        }

    case WM_DESTROY:

        return( FALSE );

    default:
		
        return( FALSE );
        break;
    }
}

VOID
SetStatusWnd (DWORD StringID)
{
	TCHAR			szTemp[128];
	
	LoadString (v_hInstance, StringID, szTemp,
				sizeof(szTemp)/sizeof(TCHAR));
	SetWindowText(GetDlgItem(v_hMainWnd, IDC_STATUSMSG),
				  szTemp);
}

DWORD
RasErrorToStringID (DWORD RasError, DWORD Default, BOOL fOnRasDial)
{
	DWORD StringID;
	
	switch (RasError) {
	case ERROR_PORT_DISCONNECTED :
		StringID = IDS_CARRIER_DROPPED;
		break;
	case ERROR_NO_CARRIER :
		if (!_tcscmp (v_RasEntry.szDeviceType, RASDT_Modem)) {
			// Dialup
			StringID = IDS_NO_CARRIER;
		} else {
			// Direct
			StringID = IDS_DCC_NO_CARRIER;
		}
		break;
	case ERROR_NO_DIALTONE :
		StringID = IDS_NO_DIALTONE;
		break;
	case ERROR_DEVICE_NOT_READY :
		StringID = IDS_DEVICE_NOT_READY;
		break;
	case ERROR_LINE_BUSY :
		StringID = IDS_LINE_BUSY;
		break;
	case ERROR_NO_ANSWER :
		StringID = IDS_NO_ANSWER;
		break;
	case ERROR_PORT_NOT_AVAILABLE :
		StringID = IDS_PORT_NOT_AVAIL;
		break;
	default :
		StringID = Default;
		break;
	}
	return StringID;
}

BOOL
RasDialEvent( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    RASCONNSTATE    RasState = (RASCONNSTATE)wParam;
	TCHAR			szTemp[128];
	TCHAR			szFmtStr[128];
	LPTSTR			pPhoneStr;
	DWORD			StringID;
    DWORD           Flag;

	DEBUGMSG (ZONE_RAS, (TEXT("RasDialEvent: wParam: 0x%X\r\n"), wParam));

    switch(RasState) {
    case RASCS_OpenPort:
		SetStatusWnd (IDS_OPENING_PORT);
        break;

    case RASCS_PortOpened:
		if (!_tcscmp (v_RasEntry.szDeviceType, RASDT_Modem)) {
			pPhoneStr = NULL;
			
	        Flag = 0;
	        if (v_RasEntry.dwfOptions & RASEO_UseCountryAndAreaCodes){
		        Flag |= LINETRANSLATEOPTION_FORCELD;
	        }
				        
	        if (v_RasEntry.dwfOptions & RASEO_DialAsLocalCall) {
		        Flag |= LINETRANSLATEOPTION_FORCELOCAL;
	        }
	
			// Get the displayable phone and dialing from location from TAPI
			pPhoneStr = TapiLineTranslateAddress(v_dwDeviceID,
				v_RasEntry.dwCountryCode,
				v_RasEntry.szAreaCode, v_RasEntry.szLocalPhoneNumber, FALSE, Flag);

			if (pPhoneStr) {
				LoadString (v_hInstance, IDS_DIAL_WITH_NUMBER, szFmtStr,
							sizeof(szFmtStr)/sizeof(TCHAR));
				wsprintf (szTemp, szFmtStr, pPhoneStr);
				LocalFree (pPhoneStr);
				SetWindowText (GetDlgItem(v_hMainWnd, IDC_STATUSMSG), szTemp);
			} else	{
				SetStatusWnd (IDS_DIAL_NO_NUMBER);
			}
		} else {
			if (v_RetryCnt++) {
				LoadString (v_hInstance, IDS_CONNECTING_TO_HOST_RETRY,
							szFmtStr, sizeof(szFmtStr)/sizeof(TCHAR));
				wsprintf (szTemp, szFmtStr, v_RetryCnt);
				SetWindowText(GetDlgItem(v_hMainWnd, IDC_STATUSMSG), szTemp);
			} else {
				SetStatusWnd (IDS_CONNECTING_TO_HOST);
			}
		}
        break;

    case RASCS_ConnectDevice:
		SetStatusWnd (IDS_CONNECTING_DEVICE);
        break;

    case RASCS_DeviceConnected:
		SetStatusWnd (IDS_DEVICE_CONNECTED);
        break;

    case RASCS_AllDevicesConnected:
		SetStatusWnd (IDS_PHY_EST);
        break;

    case RASCS_Authenticate:
		SetStatusWnd (IDS_AUTH_USER);
        break;

    case RASCS_AuthNotify:
		SetStatusWnd (IDS_AUTH_EVENT);

        // Process the lParam 

        switch( lParam ) {
        case ERROR_RESTRICTED_LOGON_HOURS:
			StringID = IDS_AUTH_ERR_REST_HOUR;
			break;
		case ERROR_ACCT_DISABLED:
			StringID = IDS_AUTH_ERR_ACCT_DISABLED;
			break;
        case ERROR_PASSWD_EXPIRED:
			StringID = IDS_AUTH_ERR_PWD_EXP;
			break;
		case ERROR_NO_DIALIN_PERMISSION:
			StringID = IDS_AUTH_ERR_NO_DIALIN;
			break;
        case ERROR_CHANGING_PASSWORD:
			StringID = IDS_AUTH_ERR_CHG_PWD;
			break;
		default :
			StringID = IDS_AUTH_ERR_UNKNOWN;
			break;

		}
		// Clear the saved authentication params
		v_RasDialParams.szUserName[0] = TEXT('\0');
		v_RasDialParams.szPassword[0] = TEXT('\0');
		v_RasDialParams.szDomain[0] = TEXT('\0');
		RasSetEntryDialParams( NULL, &v_RasDialParams, FALSE );

		// Save the error code.
		if (!v_RNAAppInfo.ErrorCode) {
			v_RNAAppInfo.ErrorCode = lParam;
		}
		
		if( !v_fNoMsgBox ) {
			// Let everyone know we're in a message box
			v_fInErrorBox = TRUE;

			// Get the Message box title for error
			ShowError (StringID, IDS_AUTH_ERROR_TITLE);

			if (v_fReturnFalse) {
				return FALSE;
			}
				
		}
        break;

    case RASCS_AuthRetry:
    case RASCS_AuthCallback:
    case RASCS_AuthChangePassword:
        break;

    case RASCS_AuthProject:
		SetStatusWnd (IDS_AUTH_PROJ);
        break;

    case RASCS_AuthLinkSpeed:
        break;

    case RASCS_AuthAck:
		SetStatusWnd (IDS_AUTH_RESP);
        break;

    case RASCS_ReAuthenticate:
        break;

    case RASCS_Authenticated:
		SetStatusWnd (IDS_USER_AUTH);
        break;

    case RASCS_PrepareForCallback:
    case RASCS_WaitForModemReset:
    case RASCS_WaitForCallback:
    case RASCS_Projected:
    case RASCS_Interactive:
    case RASCS_RetryAuthentication:
    case RASCS_CallbackSetByCaller:
    case RASCS_PasswordExpired:

        break;

    case RASCS_Connected:

        // We are CONNECTED - Update Status window
		SetStatusWnd (IDS_CONNECTED);

		// Change button label
		LoadString (v_hInstance, IDS_DISCONNECT, szTemp,
					sizeof(szTemp)/sizeof(TCHAR));
        SetWindowText(GetDlgItem(v_hMainWnd, IDCANCEL), szTemp);
		SetWindowText(GetDlgItem(v_hMainWnd, IDC_CANCEL_TEXT), TEXT(""));

		// Set our window title
		LoadString (v_hInstance, IDS_CONNECTED_TO, szFmtStr,
					sizeof(szFmtStr)/sizeof(TCHAR));
        wsprintf( szTemp, szFmtStr, EntryName );
        SetWindowText( hWnd, szTemp );
		
        if (v_fMinOnConnect) {
	        if(IsAPIReady(SH_SHELL))
   				ShowWindow(v_hMainWnd, SW_HIDE);
   			else
				SetWindowPos(v_hMainWnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        }
        // Send the BROADCAST CONNECTED message

        RETAILMSG( 1, (TEXT("Posting WM_NETCONNECT(TRUE) message\r\n")));
		v_RNAAppInfo.dwSize = sizeof(v_RNAAppInfo);
		v_RNAAppInfo.hWndRNAApp = (DWORD)hWnd;
		v_RNAAppInfo.ErrorCode = 0;
		_tcscpy (v_RNAAppInfo.RasEntryName, EntryName);
        SendNotifyMessage( HWND_BROADCAST, WM_NETCONNECT, (WPARAM)TRUE,
							(LPARAM)&v_RNAAppInfo );
		sndPlaySound(TEXT("RNBegin"), SND_ALIAS|SND_ASYNC|SND_NODEFAULT);
        break;

    case RASCS_Disconnected:
		RETAILMSG (1, (TEXT("RASCS_Disconnected: Ecode=%d\r\n"),
					   lParam));

        // We are DISCONNECTED
		if( CredentialPrompt ) {
			// If the credential window is active cancel it.

			NetDisconnect = TRUE;
			SendMessage( hCredentials, WM_COMMAND, IDCANCEL, 0 );
			NetDisconnect = FALSE;
		}

        // Update Status window
		SetStatusWnd (IDS_DISCONNECTED);

		sndPlaySound(TEXT("RNIntr"), SND_ALIAS|SND_ASYNC|SND_NODEFAULT);
		
		if (!v_RNAAppInfo.ErrorCode) {
			v_RNAAppInfo.ErrorCode = lParam;
		}
		
		// Hangup the RAS connection
        if( v_hRasConn ) {
            RasHangup( v_hRasConn );
            v_hRasConn = 0;
		}

		if( !v_fNoMsgBox && !v_fInErrorBox) {
			
			StringID = RasErrorToStringID (lParam, IDS_DISCONNECTED, FALSE);

			ShowError (StringID, IDS_REM_NET);
		}

		if (v_fInErrorBox) {
		  v_fReturnFalse = TRUE;
		  return TRUE;
		} else {
		  return( FALSE );
		}
        break;
    }
    return TRUE;
}

void PositionSIP(int nSipState)
{
#ifdef USE_SIP
    // Do we have the sip function?
    if (v_lpfnSipGetInfo && v_lpfnSipSetInfo)
    {
	    SIPINFO si;

        // See whether the SIP is up or down
        memset(&si, 0, sizeof(SIPINFO));
        si.cbSize = sizeof(SIPINFO);
        if ((*v_lpfnSipGetInfo)(&si))
        {
            // Has the SIP state changed?
            if ((!(si.fdwFlags & SIPF_ON) && SIP_UP == nSipState) ||
                (si.fdwFlags & SIPF_ON && !(SIP_UP == nSipState)))
            {
                si.fdwFlags ^= SIPF_ON;
		        (*v_lpfnSipSetInfo)(&si);
            }
        }
	}
#endif
}

void LoadSIP(void)
{
#ifdef USE_SIP        
    if( (v_hSipLib = LoadLibrary( TEXT("coredll.dll") )) &&
        (v_lpfnSipStatus = (LPFNSIPSTATUS)GetProcAddress( v_hSipLib, TEXT("SipStatus"))) &&
	    (v_lpfnSipGetInfo = (LPFNSIP)GetProcAddress( v_hSipLib, TEXT("SipGetInfo"))) &&
	    (v_lpfnSipSetInfo = (LPFNSIP)GetProcAddress( v_hSipLib, TEXT("SipSetInfo"))) &&
	    (SIP_STATUS_AVAILABLE == v_lpfnSipStatus()) )
	{
		DEBUGMSG(1, (L"RNAAPP: Sip Loaded & ready\r\n"));
    }
    else
    {
		FreeLibrary(v_hSipLib);
		v_hSipLib = NULL;
        v_lpfnSipStatus = NULL;
	    v_lpfnSipGetInfo = v_lpfnSipSetInfo = NULL;
	}
#endif
}


LRESULT CALLBACK
WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    TCHAR           szTemp[128];
	TCHAR			szTemp2[128];
	DWORD			RetVal;
    NOTIFYICONDATA  tnid;
	DWORD			StringID;
    int				cxScreen;

    switch( message ) {
    case WM_INITDIALOG:
		// Do the icon
		if (!_tcscmp (v_RasEntry.szDeviceType, RASDT_Modem)) {
            SendMessage(GetDlgItem(hWnd, IDC_STATUSICON), STM_SETIMAGE,
                        IMAGE_ICON, (LPARAM) v_hDialupIcon);
		} else if (!_tcscmp (v_RasEntry.szDeviceType, RASDT_Direct)) {
            SendMessage(GetDlgItem(hWnd, IDC_STATUSICON), STM_SETIMAGE,
                        IMAGE_ICON, (LPARAM) v_hDirectIcon);
		} else if (!_tcscmp (v_RasEntry.szDeviceType, RASDT_Vpn)) {
            SendMessage(GetDlgItem(hWnd, IDC_STATUSICON), STM_SETIMAGE,
                        IMAGE_ICON, (LPARAM) v_hVPNIcon);
		} else {
			RETAILMSG (1, (TEXT("Error, unhandled Device Type\r\n")));
			ASSERT (0);
		}

		
		LoadString (v_hInstance, IDS_CONNECTING_TO, szTemp,
					sizeof(szTemp)/sizeof(TCHAR));
        wsprintf( szTemp2, szTemp, EntryName );
        SetWindowText( hWnd, szTemp2 );

		// Set notification icon
        tnid.cbSize = sizeof(NOTIFYICONDATA); 
        tnid.hWnd = hWnd; 
        tnid.uID = 13; 
        tnid.uFlags = NIF_MESSAGE | NIF_ICON; 
        tnid.uCallbackMessage = IDM_TASKBAR_NOTIFY;
		
		if (!_tcscmp (v_RasEntry.szDeviceType, RASDT_Modem)) {
			v_hNotifyIcon = (HICON )LoadImage( v_hInstance,
											   MAKEINTRESOURCE(IDI_DIALUP),
											   IMAGE_ICON, 16, 16,
											   LR_DEFAULTCOLOR);
		} else if (!_tcscmp (v_RasEntry.szDeviceType, RASDT_Direct)) {
			v_hNotifyIcon = (HICON )LoadImage( v_hInstance,
											   MAKEINTRESOURCE(IDI_DIRECTCC),
											   IMAGE_ICON, 16, 16,
											   LR_DEFAULTCOLOR);
		} else if (!_tcscmp (v_RasEntry.szDeviceType, RASDT_Vpn)) {
			v_hNotifyIcon = (HICON )LoadImage( v_hInstance,
											   MAKEINTRESOURCE(IDI_VPN),
											   IMAGE_ICON, 16, 16,
											   LR_DEFAULTCOLOR);
		} else {
			RETAILMSG (1, (TEXT("Error, unhandled Device Type\r\n")));
			ASSERT (0);
		}
		tnid.hIcon = v_hNotifyIcon;
			
        if(IsAPIReady(SH_SHELL))
	        Shell_NotifyIcon( NIM_ADD, &tnid );
        // We didn't set the focus so return TRUE....
        return TRUE;
        
    case WM_CREATE:
        break;

    case IDM_START_RASDIAL:
		cxScreen = GetSystemMetrics(SM_CXSCREEN);

        // Start a RAS Session 

        memset( (char *)&v_RasDialParams, 0, sizeof( RASDIALPARAMS ) );

        v_RasDialParams.dwSize = sizeof(RASDIALPARAMS);
        _tcscpy( v_RasDialParams.szEntryName, EntryName );
        RasGetEntryDialParams( NULL, &v_RasDialParams, &v_fPassword );

		// If this is a dial-up entry, show them the user params 
		// (Username/Password, etc) to modify before continuing.

		// Is this a dial-up entry? (and secret no-password not specified)

        ShowWindow( hWnd, SW_SHOW );
		
		if(!v_fNoPassword && !_tcscmp (v_RasEntry.szDeviceType, RASDT_Modem)) {

			PositionSIP(SIP_UP);

			if (!DialogBox(v_hInstance,
						   MAKEINTRESOURCE((cxScreen < 480) ? IDD_USERPWD2 :
										   IDD_USERPWD),
						   hWnd, (DLGPROC)DlgProcCredentials)) {
				// Exit.

				v_RNAAppInfo.ErrorCode = ERROR_USER_DISCONNECTION;
				DestroyWindow (hWnd);
                PositionSIP(SIP_DOWN);
				break;
			}

            PositionSIP(SIP_DOWN);
			SendMessage( hWnd, WM_PAINT, 0, 0 );
		}

		SetForegroundWindow ( hWnd );
        
		// Begin the actual connection process

		if( RetVal = RasDial( NULL, NULL, 
					 &v_RasDialParams, 
					 0xffffffff, 
					 hWnd, 
					 &v_hRasConn ) ) {
			RETAILMSG (1, (TEXT("Error from RasDial %d %d\r\n"),
						   RetVal, GetLastError()));
			
			if( !v_fNoMsgBox ) {
				StringID = RasErrorToStringID (RetVal, IDS_RASDIAL_ERR, TRUE);

				ShowError (StringID, IDS_REM_NET);
			}

			if( v_hRasConn ) {
				RasHangup( v_hRasConn );
				v_hRasConn = 0;
			}

			v_RNAAppInfo.ErrorCode = RetVal;

			// This makes us go away....
			DestroyWindow (hWnd);
		}
		break;
	case RNA_RASCMD :
		switch (wParam) {
		case RNA_ADDREF :
			v_RefCount++;
			DEBUGMSG (ZONE_WARN, (TEXT("AddRef %d\r\n"), v_RefCount));
			break;
		case RNA_DELREF :
			if (v_RefCount) {
				v_RefCount--;
			}
			DEBUGMSG (ZONE_WARN, (TEXT("DelRef %d\r\n"), v_RefCount));
			if (!v_RefCount) {
				SendMessage (hWnd, WM_COMMAND, IDCANCEL, 0);
				DEBUGMSG (ZONE_WARN, (TEXT("Sending myself a cancel\r\n")));
			}
			break;
		case RNA_GETINFO :
			if (IsWindow((HWND)lParam)) {
				DEBUGMSG (ZONE_WARN,
						  (TEXT("GetInfo: Sending WM_NETCONNECT to 0x%X\r\n"),
						   lParam));
				SendNotifyMessage ((HWND)lParam, WM_NETCONNECT, (WPARAM)TRUE,
							 (LPARAM)&v_RNAAppInfo);
			} else {
				DEBUGMSG (ZONE_ERROR,
						  (TEXT("GetInfo: lParam 0x%X is not a window\r\n"),
						   lParam));
			}
			break;
		default :
			DEBUGMSG (ZONE_WARN, (TEXT("Unknown RNA_RASCMD %d\r\n"), wParam));
			break;
		}
		break;

	case WM_TIMER :
		break;
		
    case WM_COMMAND:

        switch( LOWORD( wParam ) ) {
        case IDDISMISS:
			{
		        if(IsAPIReady(SH_SHELL))
    	   			ShowWindow(hWnd, SW_HIDE);
   				else
					SetWindowPos(hWnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
			}
            break;

        case IDCANCEL:
        	DEBUGMSG( ZONE_MISC, (TEXT("IDCANCEL:v_hRasConn: 0x%8x\r\n"),
								  v_hRasConn ));

            if( v_hRasConn ) {
        		DEBUGMSG( ZONE_MISC, ( TEXT( "-> rasHangup\r\n" )));
                RasHangup( v_hRasConn );
        		DEBUGMSG( ZONE_MISC, ( TEXT( "<- rasHangup\r\n" )));
                v_hRasConn = 0;
            }
			v_RNAAppInfo.ErrorCode = ERROR_USER_DISCONNECTION;
			sndPlaySound(TEXT("RNEnd"), SND_ALIAS|SND_ASYNC|SND_NODEFAULT);
            DestroyWindow( hWnd );
            break;
        }
        break;

    case WM_RASDIALEVENT:

        if( FALSE == RasDialEvent( hWnd, message, wParam, lParam ) ) {
            // Fatal Error.

			if (!v_RNAAppInfo.ErrorCode) {
				v_RNAAppInfo.ErrorCode = ERROR_DISCONNECTION;
			}

            DestroyWindow( hWnd );
        }
        break;

    case IDM_TASKBAR_NOTIFY:

        if( lParam == WM_LBUTTONDBLCLK) {
            DEBUGMSG( ZONE_MISC,
					  (TEXT("Got IDM_TASKBAR_NOTIFY: LBUTTONDOWN\r\n")));
			SetForegroundWindow(hWnd);
            ShowWindow( hWnd, SW_SHOWNORMAL );
        }
        break;

    case WM_DESTROY:
        DEBUGMSG(1, (TEXT("DESTROY\n")));
        
        tnid.cbSize = sizeof(NOTIFYICONDATA); 
        tnid.hWnd = hWnd; 
        tnid.uID = 13; 
         
        if(IsAPIReady(SH_SHELL))
        	Shell_NotifyIcon(NIM_DELETE, &tnid); 
        PostQuitMessage(0);
        break;

    default:
		break;
    }
    return (0);
}

BOOL
InitApplication(HINSTANCE hInstance)
{
    return TRUE;
}

BOOL
InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    DWORD   ScreenX = GetSystemMetrics(SM_CXSCREEN);
    DWORD   ScreenY = GetSystemMetrics(SM_CYSCREEN);

    v_hDirectIcon = LoadIcon (hInstance, MAKEINTRESOURCE(IDI_DIRECTCC));
    v_hDialupIcon = LoadIcon (hInstance, MAKEINTRESOURCE(IDI_DIALUP));
    v_hVPNIcon = LoadIcon (hInstance, MAKEINTRESOURCE(IDI_VPN));
    
    v_hMainWnd = CreateDialog(hInstance,
                              MAKEINTRESOURCE( GetSystemMetrics(SM_CXSCREEN) <
											   480 ? IDD_DCCSTATUS2 :
											   IDD_DCCSTATUS),
							  NULL, WndProc);

    if (v_hMainWnd) {
		SetWindowLong (v_hMainWnd, DWL_USER, RNAAPP_MAGIC_NUM);
		if(IsAPIReady(SH_SHELL))
			ShowWindow(v_hMainWnd, SW_HIDE);
		else
			SetWindowPos(v_hMainWnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        UpdateWindow(v_hMainWnd);
    }

    return (NULL != v_hMainWnd);
}
BOOL FindRNAAppWindow(HWND hWnd, LPARAM lParam)
{
    TCHAR  	szClassName[32];

	GetClassName (hWnd, szClassName, sizeof(szClassName)/sizeof(TCHAR));

	if (!_tcscmp (szClassName, TEXT("Dialog")) &&
		(RNAAPP_MAGIC_NUM == GetWindowLong (hWnd, DWL_USER))) {
		*((HWND *)lParam) = hWnd;
		return FALSE;
	}
	return TRUE;
}

int WINAPI
#ifdef UNDER_CE
WinMain(HINSTANCE hInstance, HINSTANCE hInstPrev, LPWSTR pCmdLine,
     int nCmdShow)
#else
WinMain(HINSTANCE hInstance, HINSTANCE hInstPrev, LPSTR pCmdLine,
     int nCmdShow)
#endif
{
    MSG         msg;
    BOOL        fParmError = FALSE;
    BOOL        fHaveQuote;
    DWORD       cb;
    DWORD       Index;
    long        lReturn;
	DWORD		dwDebug;
	BOOL		TapiInitialFlg = FALSE;

	DEBUGREGISTER(0);

    v_hInstance = hInstance;
	v_hMainWnd = NULL;
	memset(&v_RNAAppInfo, 0, sizeof(v_RNAAppInfo));

#ifdef USE_TTCONTROLS
	if (!InitCommCtrlTable()) {
		DEBUGMSG (ZONE_ERROR, (TEXT("RNAAPP: No commctrl.dll\r\n")));
	}
	else {
		INITCOMMONCONTROLSEX icce;
		icce.dwSize = sizeof(INITCOMMONCONTROLSEX);
		icce.dwICC  = ICC_TOOLTIP_CLASSES|ICC_CAPEDIT_CLASS;
		CallCommCtrlFunc(InitCommonControlsEx)(&icce); 
	}
#endif

	// see if the SIP is enabled
	LoadSIP();

    // Parse the command line.
    while (*pCmdLine != TEXT('\0') && !fParmError) {
        // Skip leading whitespace
        while (*pCmdLine == TEXT(' ')) {
            pCmdLine++;
        }
        if ((*pCmdLine == TEXT('-')) || (*pCmdLine == TEXT('/'))) {
            pCmdLine++;
            switch (*pCmdLine) {
			case TEXT('n') :
			case TEXT('N') :
				v_fNoMsgBox = TRUE;
				*pCmdLine++;
				break;

			case TEXT('p') :
			case TEXT('P') :
				v_fNoPassword = TRUE;
				*pCmdLine++;
				break;

			case TEXT('m') :
            case TEXT('M') :
                v_fMinOnConnect = TRUE;
                *pCmdLine++;
                break;

			case TEXT('c') :
			case TEXT('C') :
				pCmdLine++;
				while ( ( (*pCmdLine >= TEXT('0') ) &&
						(*pCmdLine <= TEXT('9'))) ||
						((*pCmdLine >= TEXT('A')) &&
						 (*pCmdLine <= TEXT('F')))) {
					v_RNAAppInfo.Context <<= 4;
					if ((*pCmdLine >= TEXT('0')) && (*pCmdLine <= TEXT('9'))) {
						v_RNAAppInfo.Context += *pCmdLine - TEXT('0');
					} else {
						v_RNAAppInfo.Context += *pCmdLine - TEXT('A') + 10;
					}
					pCmdLine++;
				}
				DEBUGMSG (ZONE_WARN, (TEXT("Context=0x%X\r\n"),
							  v_RNAAppInfo.Context));
				break;

			case TEXT('e') :
            case TEXT('E') :
                // Already processed?
                if (EntryName[0] != TEXT('\0')) {
                    DEBUGMSG (ZONE_ERROR,
							  (TEXT("EntryName already specified ('%s')\r\n"),
							   pCmdLine));
                    fParmError = TRUE;
                    break;
                }
                *pCmdLine++;
                if (*pCmdLine == TEXT('\"')) {
                    *pCmdLine++;
                    fHaveQuote = TRUE;
                } else {
                    fHaveQuote = FALSE;
                }
           
                // Copy until the next space.
                for (Index = 0; pCmdLine[Index] != TEXT('\0'); Index++) {
                    if (fHaveQuote && (pCmdLine[Index] == TEXT('\"'))) {
                        pCmdLine++;
                        break;
                    }
                    if (!fHaveQuote && (pCmdLine[Index] == TEXT(' '))) {
                        break;
                    }
					if (Index < RAS_MaxEntryName) {
						EntryName[Index] = pCmdLine[Index];
					}
                }
                // Skip past any processed characters.
                pCmdLine += (Index);
                EntryName[Index] = TEXT('\0');
                break;
            default :
                // Invalid parameter.
                DEBUGMSG (ZONE_ERROR, (TEXT("Invalid option %s\r\n"),
									   pCmdLine));
                fParmError = TRUE;
                break;
            }
        } else {
			DEBUGMSG (ZONE_ERROR, (TEXT("Invalid parameter '%s'\r\n"),
								   pCmdLine));
			fParmError = TRUE;
		}
    }
    if (fParmError || (EntryName[0] == TEXT('\0'))) {
        DEBUGMSG (ZONE_ERROR, (TEXT("Parameter Error\r\n")));
		v_RNAAppInfo.ErrorCode = ERROR_INVALID_PARAMETER;
		ShowError (IDS_INVALID_PARAMETER, IDS_REM_NET);
        goto ExitApp;
    }

    // This will create the default entries if the key does not exist.
    RasEnumEntries (NULL, NULL, NULL, &cb, NULL);

    v_RasEntry.dwSize = sizeof(RASENTRY);
    cb = sizeof(RASENTRY);
    if (RasGetEntryProperties (NULL, EntryName, &(v_RasEntry),
                               &cb, NULL, NULL)) {
        DEBUGMSG (ZONE_ERROR, (TEXT("Invalid Ras Entry '%s'\r\n"), EntryName));
		v_RNAAppInfo.ErrorCode = ERROR_CANNOT_FIND_PHONEBOOK_ENTRY;
		ShowError (IDS_INVALID_PARAMETER, IDS_REM_NET);
        goto ExitApp;
    }


	// Initialize the application
    DEBUGMSG( ZONE_ERROR, ( TEXT(" About to InitApplication\r\n" )));

    if( InitApplication( hInstance ) == FALSE ) {
		v_RNAAppInfo.ErrorCode = ERROR_EVENT_INVALID;
		ShowError (IDS_INVALID_PARAMETER, IDS_REM_NET);
        goto ExitApp;
    }

    DEBUGMSG (ZONE_ERROR, (TEXT( "About to InitInstance\r\n" )));

    if( InitInstance( hInstance, nCmdShow ) == FALSE ) {
		v_RNAAppInfo.ErrorCode = ERROR_INVALID_PARAMETER;
        goto ExitApp;
    }

    PostMessage( v_hMainWnd, IDM_START_RASDIAL, 0, 0 );

   //
   // Initialize TAPI
   //
	dwDebug = 0;
#ifdef DEBUG
	dwDebug |= (ZONE_ERROR) ? TAPI_ZONE_ERROR : 0;
	dwDebug |= (ZONE_WARN) ? TAPI_ZONE_WARN : 0;
	dwDebug |= (ZONE_FUNCTION) ? TAPI_ZONE_FUNCTION : 0;
	dwDebug |= (ZONE_ALLOC) ? TAPI_ZONE_ALLOC : 0;
	dwDebug |= (ZONE_TAPI) ? TAPI_ZONE_INFO : 0;
#endif
	lReturn = TapiInitialize(hInstance, lineCallbackFunc, 
							 szAppName,dwDebug);
	TapiInitialFlg = TRUE;
   
	v_dwDeviceID = TapiFindDeviceID(v_RasEntry.szDeviceName,
									RAS_MaxDeviceName);

	DEBUGMSG(ZONE_TAPI, (TEXT("The device id is %d\n"), v_dwDeviceID));
   
    DEBUGMSG( ZONE_MISC, ( TEXT( "About to enter GetMessage Loop\r\n" )));
    while( GetMessage( &msg, NULL, 0, 0) != FALSE ) {
        if (v_hMainWnd == 0 || !IsDialogMessage(v_hMainWnd, &msg)) {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }
    }

ExitApp:
	if (v_hLineApp && TapiInitialFlg) {
		lineShutdown (v_hLineApp);
	}

    RETAILMSG( 1, (TEXT("Posting WM_NETCONNECT(FALSE) message, ErrorCode = %d(0x%X)\r\n" ),
				   v_RNAAppInfo.ErrorCode, v_RNAAppInfo.ErrorCode));
	v_RNAAppInfo.dwSize = sizeof(v_RNAAppInfo);
	v_RNAAppInfo.hWndRNAApp = (DWORD)NULL;
	_tcscpy (v_RNAAppInfo.RasEntryName, EntryName);
    SendNotifyMessage( HWND_BROADCAST, WM_NETCONNECT, (WPARAM)FALSE,
				 		(LPARAM)&v_RNAAppInfo );

    DEBUGMSG( ZONE_MISC, (TEXT( "Exiting WinMain\r\n" )));
    return( msg.wParam );
}
