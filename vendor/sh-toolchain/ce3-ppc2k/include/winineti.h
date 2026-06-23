
#if !defined(_WININETEX_)
#define _WININETEX_

#if defined(__cplusplus)
extern "C" {
#endif

#define MAX_CACHE_ENTRY_INFO_SIZE       4096
#define INTERNET_FLAG_UNUSED_1          0x00020000
#define INTERNET_FLAG_UNUSED_2          0x00000020
#define INTERNET_FLAG_UNUSED_3          0x00000008
#define INTERNET_FLAG_UNUSED_4          0x00000004

//
// INTERNET_PREFETCH_STATUS -
//

typedef struct {

    //
    // dwStatus - status of download. See INTERNET_PREFETCH_ flags
    //

    DWORD dwStatus;

    //
    // dwSize - size of file downloaded so far
    //

    DWORD dwSize;
} INTERNET_PREFETCH_STATUS, * LPINTERNET_PREFETCH_STATUS;

//
// INTERNET_PREFETCH_STATUS - dwStatus values
//

#define INTERNET_PREFETCH_PROGRESS  0
#define INTERNET_PREFETCH_COMPLETE  1
#define INTERNET_PREFETCH_ABORTED   2


#define INTERNET_ONLINE_OFFLINE_INFO    INTERNET_CONNECTED_INFO
#define LPINTERNET_ONLINE_OFFLINE_INFO  LPINTERNET_CONNECTED_INFO
#define dwOfflineState                  dwConnectedState


#define ISO_FORCE_OFFLINE       ISO_FORCE_DISCONNECTED


#ifdef __WINCRYPT_H__
#ifdef ALGIDDEF
//
// INTERNET_SECURITY_INFO - contains information about certificate
// and encryption settings for a connection.
//

#define INTERNET_SECURITY_INFO_DEFINED

typedef struct {

    //
    // dwSize - Size of INTERNET_SECURITY_INFO structure.
    //

    DWORD dwSize;


    //
    // pCertificate - Cert context pointing to leaf of certificate chain.
    //

    PCCERT_CONTEXT pCertificate;

    //
    // Start SecPkgContext_ConnectionInfo
    // The following members must match those
    // of the SecPkgContext_ConnectionInfo
    // sspi structure (schnlsp.h)
    //


    //
    // dwProtocol - Protocol that this connection was made with
    //  (PCT, SSL2, SSL3, etc)
    //

    DWORD dwProtocol;

    //
    // aiCipher - Cipher that this connection as made with
    //

    ALG_ID aiCipher;

    //
    // dwCipherStrength - Strength (in bits) that this connection
    //  was made with;
    //

    DWORD dwCipherStrength;

    //
    // aiHash - Hash that this connection as made with
    //

    ALG_ID aiHash;

    //
    // dwHashStrength - Strength (in bits) that this connection
    //  was made with;
    //

    DWORD dwHashStrength;

    //
    // aiExch - Key Exchange type that this connection as made with
    //

    ALG_ID aiExch;

    //
    // dwExchStrength - Strength (in bits) that this connection
    //  was made with;
    //

    DWORD dwExchStrength;


} INTERNET_SECURITY_INFO, * LPINTERNET_SECURITY_INFO;
#endif // ALGIDDEF
#endif // __WINCRYPT_H__

#ifdef INTERNET_SECURITY_INFO_DEFINED

INTERNETAPI
DWORD
WINAPI
ShowSecurityInfo(
    IN HWND                            hWndParent,
    IN LPINTERNET_SECURITY_INFO        pSecurityInfo
    );
#endif // INTERNET_SECURITY_INFO_DEFINED



INTERNETAPI
DWORD
WINAPI
ShowX509EncodedCertificate(
    IN HWND    hWndParent,
    IN LPBYTE  lpCert,
    IN DWORD   cbCert
    );

INTERNETAPI
DWORD
WINAPI
ShowClientAuthCerts(
    IN HWND hWndParent
    );

INTERNETAPI
DWORD
WINAPI
ParseX509EncodedCertificateForListBoxEntry(
    IN LPBYTE  lpCert,
    IN DWORD   cbCert,
    OUT LPSTR  lpszListBoxEntry,
    IN LPDWORD lpdwListBoxEntry
    );

//
// This is a private API for Trident.  It displays
// security info based on a URL
//

INTERNETAPI
BOOL
WINAPI
InternetShowSecurityInfoByURL(
    IN       LPSTR    lpszURL,
    IN       HWND     hwndParent
    );


BOOLAPI
InternetDebugGetLocalTime(
    OUT SYSTEMTIME * pstLocalTime,
    OUT DWORD      * pdwReserved
    );


INTERNETAPI
BOOL
WINAPI
InternetWriteFileExA(
    IN HINTERNET hFile,
    IN LPINTERNET_BUFFERSA lpBuffersIn,
    IN DWORD dwFlags,
    IN DWORD dwContext
    );
INTERNETAPI
BOOL
WINAPI
InternetWriteFileExW(
    IN HINTERNET hFile,
    IN LPINTERNET_BUFFERSW lpBuffersIn,
    IN DWORD dwFlags,
    IN DWORD dwContext
    );
#ifdef UNICODE
#define InternetWriteFileEx  InternetWriteFileExW
#else
#define InternetWriteFileEx  InternetWriteFileExA
#endif // !UNICODE

#define INTERNET_OPTION_CONTEXT_VALUE_OLD       10
#define INTERNET_OPTION_NET_SPEED               61

#define INTERNET_OPTION_OFFLINE_TIMEOUT INTERNET_OPTION_DISCONNECTED_TIMEOUT
#define INTERNET_OPTION_LINE_STATE      INTERNET_OPTION_CONNECTED_STATE


#define INTERNET_STATE_ONLINE       INTERNET_STATE_CONNECTED
#define INTERNET_STATE_OFFLINE      INTERNET_STATE_DISCONNECTED
#define INTERNET_STATE_OFFLINE_USER INTERNET_STATE_DISCONNECTED_BY_USER
#define INTERNET_LINE_STATE_MASK    (INTERNET_STATE_ONLINE | INTERNET_STATE_OFFLINE)
#define INTERNET_BUSY_STATE_MASK    (INTERNET_STATE_IDLE | INTERNET_STATE_BUSY)


//
// the following are used with InternetSetOption(..., INTERNET_OPTION_CALLBACK_FILTER, ...)
// to filter out unrequired callbacks. INTERNET_STATUS_REQUEST_COMPLETE cannot
// be filtered out
//

#define INTERNET_STATUS_FILTER_RESOLVING        0x00000001
#define INTERNET_STATUS_FILTER_RESOLVED         0x00000002
#define INTERNET_STATUS_FILTER_CONNECTING       0x00000004
#define INTERNET_STATUS_FILTER_CONNECTED        0x00000008
#define INTERNET_STATUS_FILTER_SENDING          0x00000010
#define INTERNET_STATUS_FILTER_SENT             0x00000020
#define INTERNET_STATUS_FILTER_RECEIVING        0x00000040
#define INTERNET_STATUS_FILTER_RECEIVED         0x00000080
#define INTERNET_STATUS_FILTER_CLOSING          0x00000100
#define INTERNET_STATUS_FILTER_CLOSED           0x00000200
#define INTERNET_STATUS_FILTER_HANDLE_CREATED   0x00000400
#define INTERNET_STATUS_FILTER_HANDLE_CLOSING   0x00000800
#define INTERNET_STATUS_FILTER_PREFETCH         0x00001000
#define INTERNET_STATUS_FILTER_REDIRECT         0x00002000
#define INTERNET_STATUS_FILTER_STATE_CHANGE     0x00004000


BOOLAPI
FtpCommandA(
    IN HINTERNET hConnect,
    IN BOOL fExpectResponse,
    IN DWORD dwFlags,
    IN LPCSTR lpszCommand,
    IN DWORD dwContext
    );
BOOLAPI
FtpCommandW(
    IN HINTERNET hConnect,
    IN BOOL fExpectResponse,
    IN DWORD dwFlags,
    IN LPCWSTR lpszCommand,
    IN DWORD dwContext
    );
#ifdef UNICODE
#define FtpCommand  FtpCommandW
#else
#define FtpCommand  FtpCommandA
#endif // !UNICODE

INTERNETAPI
DWORD
WINAPI
FtpGetFileSize(
    IN HINTERNET hFile,
    OUT LPDWORD lpdwFileSizeHigh OPTIONAL
    );

typedef struct _INTERNET_COOKIE {
    DWORD cbSize;
    LPSTR pszName;
    LPSTR pszData;
    LPSTR pszDomain;
    LPSTR pszPath;
    FILETIME *pftExpires;
    DWORD dwFlags;
} INTERNET_COOKIE, *PINTERNET_COOKIE;

#define INTERNET_COOKIE_IS_SECURE   0x01


//
// server-push API
//

#define SRVPSH_ACTION_INVALID                0
#define SRVPSH_ACTION_NEED_NEW_INPUTBUFFER   1
#define SRVPSH_ACTION_HEADERS_AVAILABLE      2
#define SRVPSH_ACTION_DATA_AVAILABLE         3
#define SRVPSH_ACTION_DATA_DONE              4

INTERNETAPI
HINTERNET
WINAPI
InternetOpenServerPushParse(
    IN OPTIONAL HINTERNET hInternet,
    IN LPSTR lpszMimeType,
    IN DWORD dwReserved
    );

BOOLAPI
InternetServerPushParse(
    IN HINTERNET hSrvPushHandle,
    OUT LPDWORD lpdwAction,
    IN DWORD dwFlags,
    IN LPVOID lpvBuffer,
    IN DWORD  dwBufferSize,
    OUT LPVOID * lplpvDataStream,
    OUT LPDWORD lpdwDataStreamSize
    );


#define ERROR_INTERNET_NO_NEW_CONTAINERS        (INTERNET_ERROR_BASE + 51)


#define ERROR_INTERNET_OFFLINE  ERROR_INTERNET_DISCONNECTED

//
// internal error codes that are used to communicate specific information inside
// of Wininet but which are meaningless at the interface
//

#define INTERNET_INTERNAL_ERROR_BASE            (INTERNET_ERROR_BASE + 900)

#define ERROR_INTERNET_INTERNAL_SOCKET_ERROR    (INTERNET_INTERNAL_ERROR_BASE + 1)
#define ERROR_INTERNET_CONNECTION_AVAILABLE     (INTERNET_INTERNAL_ERROR_BASE + 2)
#define ERROR_INTERNET_NO_KNOWN_SERVERS         (INTERNET_INTERNAL_ERROR_BASE + 3)
#define ERROR_INTERNET_PING_FAILED              (INTERNET_INTERNAL_ERROR_BASE + 4)
#define ERROR_INTERNET_NO_PING_SUPPORT          (INTERNET_INTERNAL_ERROR_BASE + 5)
#define ERROR_INTERNET_CACHE_SUCCESS            (INTERNET_INTERNAL_ERROR_BASE + 6)

#define HTTP_1_1_CACHE_ENTRY            0x00000040
#define PENDING_DELETE_CACHE_ENTRY      0x00400000
#define POST_RESPONSE_CACHE_ENTRY       0x04000000
#define INSTALLED_CACHE_ENTRY           0x10000000

//
// INTERNET_CACHE_CONFIG_PATH_ENTRY
//

typedef struct _INTERNET_CACHE_CONFIG_PATH_ENTRYA {
    CHAR   CachePath[MAX_PATH];
    DWORD dwCacheSize;  // in KBytes
} INTERNET_CACHE_CONFIG_PATH_ENTRYA, * LPINTERNET_CACHE_CONFIG_PATH_ENTRYA;
typedef struct _INTERNET_CACHE_CONFIG_PATH_ENTRYW {
    WCHAR  CachePath[MAX_PATH];
    DWORD dwCacheSize;  // in KBytes
} INTERNET_CACHE_CONFIG_PATH_ENTRYW, * LPINTERNET_CACHE_CONFIG_PATH_ENTRYW;
#ifdef UNICODE
typedef INTERNET_CACHE_CONFIG_PATH_ENTRYW INTERNET_CACHE_CONFIG_PATH_ENTRY;
typedef LPINTERNET_CACHE_CONFIG_PATH_ENTRYW LPINTERNET_CACHE_CONFIG_PATH_ENTRY;
#else
typedef INTERNET_CACHE_CONFIG_PATH_ENTRYA INTERNET_CACHE_CONFIG_PATH_ENTRY;
typedef LPINTERNET_CACHE_CONFIG_PATH_ENTRYA LPINTERNET_CACHE_CONFIG_PATH_ENTRY;
#endif // UNICODE

//
// INTERNET_CACHE_CONFIG_INFO
//

typedef struct _INTERNET_CACHE_CONFIG_INFOA {
    DWORD dwReserved1;
    DWORD dwReserved2;
    DWORD dwReserved3;
    DWORD dwReserved4;
    BOOL  fPerUser;
    DWORD dwSyncMode;
    DWORD dwNumCachePaths;
    INTERNET_CACHE_CONFIG_PATH_ENTRYA CachePaths[ANYSIZE_ARRAY];
} INTERNET_CACHE_CONFIG_INFOA, * LPINTERNET_CACHE_CONFIG_INFOA;
typedef struct _INTERNET_CACHE_CONFIG_INFOW {
    DWORD dwReserved1;
    DWORD dwReserved2;
    DWORD dwReserved3;
    DWORD dwReserved4;
    BOOL  fPerUser;
    DWORD dwSyncMode;
    DWORD dwNumCachePaths;
    INTERNET_CACHE_CONFIG_PATH_ENTRYW CachePaths[ANYSIZE_ARRAY];
} INTERNET_CACHE_CONFIG_INFOW, * LPINTERNET_CACHE_CONFIG_INFOW;
#ifdef UNICODE
typedef INTERNET_CACHE_CONFIG_INFOW INTERNET_CACHE_CONFIG_INFO;
typedef LPINTERNET_CACHE_CONFIG_INFOW LPINTERNET_CACHE_CONFIG_INFO;
#else
typedef INTERNET_CACHE_CONFIG_INFOA INTERNET_CACHE_CONFIG_INFO;
typedef LPINTERNET_CACHE_CONFIG_INFOA LPINTERNET_CACHE_CONFIG_INFO;
#endif // UNICODE

#define INTERNET_CACHE_FLAG_ALLOW_COLLISIONS     0x00000001
#define INTERNET_CACHE_FLAG_INSTALLED_ENTRY      0x00000002
#define INTERNET_CACHE_FLAG_ENTRY_OR_MAPPING     0x00000004
#define CACHE_ENTRY_MODIFY_DATA_FC  0x80000000

// Flags for CreateContainer

#define INTERNET_CACHE_CONTAINER_NOSUBDIRS (0x1)
#define INTERNET_CACHE_CONTAINER_AUTODELETE (0x2)
#define INTERNET_CACHE_CONTAINER_RESERVED1 (0x4)
#define INTERNET_CACHE_CONTAINER_NODESKTOPINIT (0x8)
#define INTERNET_CACHE_CONTAINER_MAP_ENABLED (0x10)

BOOLAPI
CreateUrlCacheContainerA(
     IN LPCSTR Name,
     IN LPCSTR lpCachePrefix,
     LPCSTR lpszCachePath,
     IN DWORD KBCacheLimit,
     IN DWORD dwContainerType,
     IN DWORD dwOptions,
     IN OUT LPVOID pvBuffer,
     IN OUT LPDWORD cbBuffer
     );
BOOLAPI
CreateUrlCacheContainerW(
     IN LPCSTR Name,
     IN LPCSTR lpCachePrefix,
     LPCWSTR lpszCachePath,
     IN DWORD KBCacheLimit,
     IN DWORD dwContainerType,
     IN DWORD dwOptions,
     IN OUT LPVOID pvBuffer,
     IN OUT LPDWORD cbBuffer
     );
#ifdef UNICODE
#define CreateUrlCacheContainer  CreateUrlCacheContainerW
#else
#define CreateUrlCacheContainer  CreateUrlCacheContainerA
#endif // !UNICODE

BOOLAPI
DeleteUrlCacheContainerA(
     IN LPCSTR Name,
     IN DWORD dwOptions
     );
BOOLAPI
DeleteUrlCacheContainerW(
     IN LPCSTR Name,
     IN DWORD dwOptions
     );
#ifdef UNICODE
#define DeleteUrlCacheContainer  DeleteUrlCacheContainerW
#else
#define DeleteUrlCacheContainer  DeleteUrlCacheContainerA
#endif // !UNICODE

//
// INTERNET_CACHE_ENTRY_INFO -
//


typedef struct _INTERNET_CACHE_CONTAINER_INFOA {
    DWORD dwCacheVersion;       // version of software
    LPSTR lpszName;             // embedded pointer to the container name string.
    LPSTR lpszCachePrefix;      // embedded pointer to the container URL prefix
    LPSTR lpszVolumeLabel;      // embedded pointer to the container volume label if any.
    LPSTR lpszVolumeTitle;      // embedded pointer to the container volume title if any.
} INTERNET_CACHE_CONTAINER_INFOA, * LPINTERNET_CACHE_CONTAINER_INFOA;
typedef struct _INTERNET_CACHE_CONTAINER_INFOW {
    DWORD dwCacheVersion;       // version of software
    LPSTR lpszName;             // embedded pointer to the container name string.
    LPSTR lpszCachePrefix;      // embedded pointer to the container URL prefix
    LPSTR lpszVolumeLabel;      // embedded pointer to the container volume label if any.
    LPSTR lpszVolumeTitle;      // embedded pointer to the container volume title if any.
} INTERNET_CACHE_CONTAINER_INFOW, * LPINTERNET_CACHE_CONTAINER_INFOW;
#ifdef UNICODE
typedef INTERNET_CACHE_CONTAINER_INFOW INTERNET_CACHE_CONTAINER_INFO;
typedef LPINTERNET_CACHE_CONTAINER_INFOW LPINTERNET_CACHE_CONTAINER_INFO;
#else
typedef INTERNET_CACHE_CONTAINER_INFOA INTERNET_CACHE_CONTAINER_INFO;
typedef LPINTERNET_CACHE_CONTAINER_INFOA LPINTERNET_CACHE_CONTAINER_INFO;
#endif // UNICODE

//  FindFirstContainer options
#define CACHE_FIND_CONTAINER_RETURN_NOCHANGE (0x1)

INTERNETAPI
HANDLE
WINAPI
FindFirstUrlCacheContainerA(
    IN OUT LPDWORD pdwModified,
    OUT LPINTERNET_CACHE_CONTAINER_INFOA lpContainerInfo,
    IN OUT LPDWORD lpdwContainerInfoBufferSize,
    IN DWORD dwOptions
    );
INTERNETAPI
HANDLE
WINAPI
FindFirstUrlCacheContainerW(
    IN OUT LPDWORD pdwModified,
    OUT LPINTERNET_CACHE_CONTAINER_INFOW lpContainerInfo,
    IN OUT LPDWORD lpdwContainerInfoBufferSize,
    IN DWORD dwOptions
    );
#ifdef UNICODE
#define FindFirstUrlCacheContainer  FindFirstUrlCacheContainerW
#else
#define FindFirstUrlCacheContainer  FindFirstUrlCacheContainerA
#endif // !UNICODE

BOOLAPI
FindNextUrlCacheContainerA(
    IN HANDLE hEnumHandle,
    OUT LPINTERNET_CACHE_CONTAINER_INFOA lpContainerInfo,
    IN OUT LPDWORD lpdwContainerInfoBufferSize
    );
BOOLAPI
FindNextUrlCacheContainerW(
    IN HANDLE hEnumHandle,
    OUT LPINTERNET_CACHE_CONTAINER_INFOW lpContainerInfo,
    IN OUT LPDWORD lpdwContainerInfoBufferSize
    );
#ifdef UNICODE
#define FindNextUrlCacheContainer  FindNextUrlCacheContainerW
#else
#define FindNextUrlCacheContainer  FindNextUrlCacheContainerA
#endif // !UNICODE



typedef enum {
    WININET_SYNC_MODE_NEVER=0,
    WININET_SYNC_MODE_ON_EXPIRY,
    WININET_SYNC_MODE_ONCE_PER_SESSION,
    WININET_SYNC_MODE_ALWAYS
} WININET_SYNC_MODE;


BOOLAPI
FreeUrlCacheSpaceA(
    IN LPCSTR lpszCachePath,
    IN DWORD dwSize,
    IN DWORD dwFilter
    );
BOOLAPI
FreeUrlCacheSpaceW(
    IN LPCWSTR lpszCachePath,
    IN DWORD dwSize,
    IN DWORD dwFilter
    );
#ifdef UNICODE
#define FreeUrlCacheSpace  FreeUrlCacheSpaceW
#else
#define FreeUrlCacheSpace  FreeUrlCacheSpaceA
#endif // !UNICODE

//
// config APIs.
//

#define CACHE_CONFIG_FORCE_CLEANUP_FC           0x00000020
#define CACHE_CONFIG_DISK_CACHE_PATHS_FC        0x00000040
#define CACHE_CONFIG_SYNC_MODE_FC               0x00000080
#define CACHE_CONFIG_CONTENT_PATHS_FC           0x00000100
#define CACHE_CONFIG_COOKIES_PATHS_FC           0x00000200
#define CACHE_CONFIG_HISTORY_PATHS_FC           0x00000400

BOOLAPI
GetUrlCacheConfigInfoA(
    OUT LPINTERNET_CACHE_CONFIG_INFOA lpCacheConfigInfo,
    IN OUT LPDWORD lpdwCacheConfigInfoBufferSize,
    IN DWORD dwFieldControl
    );
BOOLAPI
GetUrlCacheConfigInfoW(
    OUT LPINTERNET_CACHE_CONFIG_INFOW lpCacheConfigInfo,
    IN OUT LPDWORD lpdwCacheConfigInfoBufferSize,
    IN DWORD dwFieldControl
    );
#ifdef UNICODE
#define GetUrlCacheConfigInfo  GetUrlCacheConfigInfoW
#else
#define GetUrlCacheConfigInfo  GetUrlCacheConfigInfoA
#endif // !UNICODE

BOOLAPI
SetUrlCacheConfigInfoA(
    IN LPINTERNET_CACHE_CONFIG_INFOA lpCacheConfigInfo,
    IN DWORD dwFieldControl
    );
BOOLAPI
SetUrlCacheConfigInfoW(
    IN LPINTERNET_CACHE_CONFIG_INFOW lpCacheConfigInfo,
    IN DWORD dwFieldControl
    );
#ifdef UNICODE
#define SetUrlCacheConfigInfo  SetUrlCacheConfigInfoW
#else
#define SetUrlCacheConfigInfo  SetUrlCacheConfigInfoA
#endif // !UNICODE


INTERNETAPI
DWORD
WINAPI
RunOnceUrlCache(
        HWND    hwnd,
        HINSTANCE hinst,
        LPSTR   lpszCmd,
        int     nCmdShow);

INTERNETAPI
DWORD
WINAPI
DeleteIE3Cache(
        HWND    hwnd,
        HINSTANCE hinst,
        LPSTR   lpszCmd,
        int     nCmdShow);

BOOLAPI
UpdateUrlCacheContentPath(LPSTR szNewPath);

// Cache header data defines.

#define CACHE_HEADER_DATA_CURRENT_SETTINGS_VERSION   0
#define CACHE_HEADER_DATA_CONLIST_CHANGE_COUNT       1
#define CACHE_HEADER_DATA_COOKIE_CHANGE_COUNT        2
#define CACHE_HEADER_DATA_NOTIFICATION_HWND          3
#define CACHE_HEADER_DATA_NOTIFICATION_MESG          4


BOOL
GetUrlCacheHeaderData(IN DWORD nIdx, OUT LPDWORD lpdwData);

BOOL
SetUrlCacheHeaderData(IN DWORD nIdx, IN  DWORD  dwData);

BOOL
IncrementUrlCacheHeaderData(IN DWORD nIdx, OUT LPDWORD lpdwData);

BOOL
LoadUrlCacheContent();

BOOL
GetUrlCacheContainerInfo(
    IN LPSTR lpszUrlName,
    OUT LPINTERNET_CACHE_CONTAINER_INFO lpContainerInfo,
    IN OUT LPDWORD lpdwContainerInfoBufferSize,
    IN DWORD dwOptions
    );


//
// Autodial APIs
//

INTERNETAPI
DWORD
WINAPI
InternetDial(
    IN HWND     hwndParent,
    IN LPTSTR   lpszConnectoid,
    IN DWORD    dwFlags,
    OUT LPDWORD lpdwConnection,
    IN DWORD    dwReserved);

// Flags for InternetDial - must not conflict with InternetAutodial flags
//                          as they are valid here also.
#define INTERNET_DIAL_UNATTENDED       0x8000

INTERNETAPI
DWORD
WINAPI
InternetHangUp(
    IN DWORD    dwConnection,
    IN DWORD    dwReserved);

#define INTERENT_GOONLINE_REFRESH 0x00000001
#define INTERENT_GOONLINE_MASK 0x00000001
INTERNETAPI
BOOL
WINAPI
InternetGoOnline(
    IN LPTSTR   lpszURL,
    IN HWND     hwndParent,
    IN DWORD    dwFlags);

INTERNETAPI
BOOL
WINAPI
InternetAutodial(
    IN DWORD    dwFlags,
    IN DWORD    dwReserved);

// Flags for InternetAutodial
#define INTERNET_AUTODIAL_FORCE_ONLINE          1
#define INTERNET_AUTODIAL_FORCE_UNATTENDED      2
#define INTERNET_AUTODIAL_FAILIFSECURITYCHECK   4

#define INTERNET_AUTODIAL_FLAGS_MASK (INTERNET_AUTODIAL_FORCE_ONLINE | INTERNET_AUTODIAL_FORCE_UNATTENDED | INTERNET_AUTODIAL_FAILIFSECURITYCHECK)
INTERNETAPI
BOOL
WINAPI
InternetAutodialHangup(
    IN DWORD    dwReserved);

INTERNETAPI
BOOL
WINAPI
InternetGetConnectedState(
    OUT LPDWORD  lpdwFlags,
    IN DWORD    dwReserved);

// Flags for InternetGetConnectedState
#define INTERNET_CONNECTION_MODEM           1
#define INTERNET_CONNECTION_LAN             2
#define INTERNET_CONNECTION_PROXY           4
#define INTERNET_CONNECTION_MODEM_BUSY      8

//
// Custom dial handler functions
//

// Custom dial handler prototype
typedef DWORD (CALLBACK * PFN_DIAL_HANDLER) (HWND, LPCSTR, DWORD, LPDWORD);

// Flags for custom dial handler
#define INTERNET_CUSTOMDIAL_CONNECT         0
#define INTERNET_CUSTOMDIAL_UNATTENDED      1
#define INTERNET_CUSTOMDIAL_DISCONNECT      2
#define INTERNET_CUSTOMDIAL_SHOWOFFLINE     4

// Custom dial handler supported functionality flags
#define INTERNET_CUSTOMDIAL_SAFE_FOR_UNATTENDED 1
#define INTERNET_CUSTOMDIAL_WILL_SUPPLY_STATE   2
#define INTERNET_CUSTOMDIAL_CAN_HANGUP          4

INTERNETAPI
BOOL
WINAPI
InternetSetDialState(
    IN LPCTSTR  lpszConnectoid,
    IN DWORD    dwState,
    IN DWORD    dwReserved);

// States for InternetSetDialState
#define INTERNET_DIALSTATE_DISCONNECTED     1

// Registry entries used by the dialing code
// All of these entries are in:
// HKCU\software\microsoft\windows\current version\internet settings

#define REGSTR_DIAL_CUSTOM_AVI      "DialAVI"
#define REGSTR_DIAL_CUSTOM_BMP      "DialBMP"
#define REGSTR_DIAL_REDIAL_WAIT     "RedialWait"
#define REGSTR_DIAL_REDIAL_ATTEMPTS "RedialAttempts"
#define REGSTR_DIAL_AUTOCONNECT     "AutoConnect"


// Used by security manager.

INTERNETAPI
BOOL
WINAPI
IsHostInProxyBypassList(
    IN INTERNET_SCHEME tScheme,
    IN LPCSTR   lpszHost,
    IN DWORD    cchHost);

// Used by Shell to determine if anyone has loaded wininet yet
// Shell code calls OpenMutex with this name and if no mutex is
// obtained, we know that no copy of wininet has been loaded yet

#define WININET_STARTUP_MUTEX "WininetStartupMutex"


#if defined(__cplusplus)
}
#endif

#endif // !define(_WININETEX_)
