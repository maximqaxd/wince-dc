/*****************************************************************************\
*                                                                             *
* shellapi.h -  SHELL.DLL functions, types, and definitions                   *
*                                                                             *
* Copyright (c) 1992-1998, Microsoft Corp.  All rights reserved               *
*                                                                             *
\*****************************************************************************/

#ifndef _INC_SHELLAPI
#define _INC_SHELLAPI


//
// Define API decoration for direct importing of DLL references.
//
#ifndef WINSHELLAPI
#if !defined(_SHELL32_)
#define WINSHELLAPI DECLSPEC_IMPORT
#else
#define WINSHELLAPI
#endif
#endif // WINSHELLAPI


#include <pshpack1.h>

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */


// @CESYSGEN IF GWES_ICONCMN

#define EIRESID(x) (-1 * (int)(x))

#ifndef ExtractIconExA
HICON WINAPI ExtractIconExA(LPCSTR lpszFile, int nIconIndex, HICON FAR *phiconLarge, HICON FAR *phiconSmall, UINT nIcons);
#endif

#ifndef ExtractIconExW
HICON WINAPI ExtractIconExW(LPCWSTR lpszFile, int nIconIndex, HICON FAR *phiconLarge, HICON FAR *phiconSmall, UINT nIcons);
#endif

#ifdef UNICODE
#define ExtractIconEx  ExtractIconExW
#else
#define ExtractIconEx  ExtractIconExA
#endif // !UNICODE

// @CESYSGEN ENDIF


////
//// Shell File Operations
////

#ifndef FO_MOVE //these need to be kept in sync with the ones in shlobj.h

#define FO_MOVE           0x0001
#define FO_COPY           0x0002
#define FO_DELETE         0x0003
#define FO_RENAME         0x0004

#define FOF_MULTIDESTFILES         0x0001
#define FOF_CONFIRMMOUSE           0x0002
#define FOF_SILENT                 0x0004  // don't create progress/report
#define FOF_RENAMEONCOLLISION      0x0008
#define FOF_NOCONFIRMATION         0x0010  // Don't prompt the user.
#define FOF_WANTMAPPINGHANDLE      0x0020  // Fill in SHFILEOPSTRUCT.hNameMappings
                                      // Must be freed using SHFreeNameMappings
#define FOF_ALLOWUNDO              0x0040
#define FOF_FILESONLY              0x0080  // on *.*, do only files
#define FOF_SIMPLEPROGRESS         0x0100  // means don't show names of files
#define FOF_NOCONFIRMMKDIR         0x0200  // don't confirm making any needed dirs

typedef WORD FILEOP_FLAGS;

#define PO_DELETE       0x0013  // printer is being deleted
#define PO_RENAME       0x0014  // printer is being renamed
#define PO_PORTCHANGE   0x0020  // port this printer connected to is being changed
                                // if this id is set, the strings received by
                                // the copyhook are a doubly-null terminated
                                // list of strings.  The first is the printer
                                // name and the second is the printer port.
#define PO_REN_PORT     0x0034  // PO_RENAME and PO_PORTCHANGE at same time.

// no POF_ flags currently defined

typedef WORD PRINTEROP_FLAGS;

#endif // FO_MOVE

// implicit parameters are:
//      if pFrom or pTo are unqualified names the current directories are
//      taken from the global current drive/directory settings managed
//      by Get/SetCurrentDrive/Directory
//
//      the global confirmation settings

#ifndef UNICODE_ONLY
typedef struct _SHFILEOPSTRUCTA
{
        HWND            hwnd;
        UINT            wFunc;
        LPCSTR          pFrom;
        LPCSTR          pTo;
        FILEOP_FLAGS    fFlags;
        BOOL            fAnyOperationsAborted;
        LPVOID          hNameMappings;
        LPCSTR           lpszProgressTitle; // only used if FOF_SIMPLEPROGRESS
} SHFILEOPSTRUCTA, FAR *LPSHFILEOPSTRUCTA;
#endif //!UNICODE_ONLY
#ifndef ANSI_ONLY
typedef struct _SHFILEOPSTRUCTW
{
        HWND            hwnd;
        UINT            wFunc;
        LPCWSTR         pFrom;
        LPCWSTR         pTo;
        FILEOP_FLAGS    fFlags;
        BOOL            fAnyOperationsAborted;
        LPVOID          hNameMappings;
        LPCWSTR          lpszProgressTitle; // only used if FOF_SIMPLEPROGRESS
} SHFILEOPSTRUCTW, FAR *LPSHFILEOPSTRUCTW;
#endif //!ANSI_ONLY
#ifdef UNICODE
typedef SHFILEOPSTRUCTW SHFILEOPSTRUCT;
typedef LPSHFILEOPSTRUCTW LPSHFILEOPSTRUCT;
#else
typedef SHFILEOPSTRUCTA SHFILEOPSTRUCT;
typedef LPSHFILEOPSTRUCTA LPSHFILEOPSTRUCT;
#endif // UNICODE

#ifndef UNICODE_ONLY
WINSHELLAPI int WINAPI SHFileOperationA(LPSHFILEOPSTRUCTA lpFileOp);
#endif //!UNICODE_ONLY
#ifndef ANSI_ONLY
WINSHELLAPI int WINAPI SHFileOperationW(LPSHFILEOPSTRUCTW lpFileOp);
#endif //!ANSI_ONLY
#ifdef UNICODE
#define SHFileOperation  SHFileOperationW
#else
#define SHFileOperation  SHFileOperationA
#endif // !UNICODE


////
////  Begin ShellExecuteEx and family
////
// @CESYSGEN IF COREDLL_SHEXEC

/* regular WinExec() codes */
#define SE_ERR_FNF              2       // file not found
#define SE_ERR_PNF              3       // path not found
#define SE_ERR_ACCESSDENIED     5       // access denied
#define SE_ERR_OOM              8       // out of memory
#define SE_ERR_DLLNOTFOUND              32

/* error values for ShellExecute() beyond the regular WinExec() codes */
#define SE_ERR_SHARE                    26
#define SE_ERR_ASSOCINCOMPLETE          27
#define SE_ERR_DDETIMEOUT               28
#define SE_ERR_DDEFAIL                  29
#define SE_ERR_DDEBUSY                  30
#define SE_ERR_NOASSOC                  31

// values for the fMask field
#define SEE_MASK_CLASSNAME        0x00000001
#define SEE_MASK_NOCLOSEPROCESS 0x00000040
#define SEE_MASK_FLAG_NO_UI     0x00000400

typedef struct _SHELLEXECUTEINFO
{
        DWORD cbSize;
        ULONG fMask;
        HWND hwnd;
        LPCTSTR lpVerb;
        LPCTSTR lpFile;
        LPCTSTR lpParameters;
        LPCTSTR lpDirectory;
        int nShow;
        HINSTANCE hInstApp;
        // Optional fields
        LPVOID lpIDList;
        LPCTSTR lpClass;
        HKEY hkeyClass;
        DWORD dwHotKey;
        HANDLE hIcon;
        HANDLE hProcess;
} SHELLEXECUTEINFO, FAR *LPSHELLEXECUTEINFO;

WINSHELLAPI BOOL WINAPI ShellExecuteEx(LPSHELLEXECUTEINFO lpExecInfo);

// @CESYSGEN ENDIF
////
////  End ShellExecuteEx and family
////


// These APIs are sysgened out if the system is to run Shell-less
// @CESYSGEN IF COREDLL_SHELLAPIS
//
//// Tray notification definitions
typedef struct _NOTIFYICONDATAA {
        DWORD cbSize;
        HWND hWnd;
        UINT uID;
        UINT uFlags;
        UINT uCallbackMessage;
        HICON hIcon;
        CHAR  szTip[64];
} NOTIFYICONDATAA, *PNOTIFYICONDATAA;

typedef struct _NOTIFYICONDATAW {
        DWORD cbSize;
        HWND hWnd;
        UINT uID;
        UINT uFlags;
        UINT uCallbackMessage;
        HICON hIcon;
        WCHAR  szTip[64];
} NOTIFYICONDATAW, *PNOTIFYICONDATAW;
#ifdef UNICODE
typedef NOTIFYICONDATAW NOTIFYICONDATA;
typedef PNOTIFYICONDATAW PNOTIFYICONDATA;
#else
typedef NOTIFYICONDATAA NOTIFYICONDATA;
typedef PNOTIFYICONDATAA PNOTIFYICONDATA;
#endif // UNICODE

#define NIM_ADD         0x00000000
#define NIM_MODIFY      0x00000001
#define NIM_DELETE      0x00000002

#define NIF_MESSAGE     0x00000001
#define NIF_ICON        0x00000002
#define NIF_TIP         0x00000004

BOOL WINAPI Shell_NotifyIcon(DWORD dwMessage, PNOTIFYICONDATA lpData);
////
//// End Tray Notification Icons
////
// @CESYSGEN ENDIF


////
//// Begin SHGetFileInfo
////
// @CESYSGEN IF COREDLL_FILEINFO
/*
 * The SHGetFileInfo API provides an easy way to get attributes
 * for a file given a pathname.
 *
 *   PARAMETERS
 *
 *     pszPath              file name to get info about
 *     dwFileAttributes     file attribs, only used with SHGFI_USEFILEATTRIBUTES
 *     psfi                 place to return file info
 *     cbFileInfo           size of structure
 *     uFlags               flags
 *
 *   RETURN
 *     TRUE if things worked
 */

#ifndef UNICODE_ONLY
typedef struct _SHFILEINFOA
{
        HICON       hIcon;                      // out: icon
        int         iIcon;                      // out: icon index
        DWORD       dwAttributes;               // out: SFGAO_ flags
        CHAR        szDisplayName[MAX_PATH];    // out: display name (or path)
        CHAR        szTypeName[80];             // out: type name
} SHFILEINFOA;
#endif //!UNICODE_ONLY
#ifndef ANSI_ONLY
typedef struct _SHFILEINFOW
{
        HICON       hIcon;                      // out: icon
        int         iIcon;                      // out: icon index
        DWORD       dwAttributes;               // out: SFGAO_ flags
        WCHAR       szDisplayName[MAX_PATH];    // out: display name (or path)
        WCHAR       szTypeName[80];             // out: type name
} SHFILEINFOW;
#endif //!ANSI_ONLY
#ifdef UNICODE
typedef SHFILEINFOW SHFILEINFO;
#else
typedef SHFILEINFOA SHFILEINFO;
#endif // UNICODE

// supported SHGetFileInfo input flags
#define SHGFI_ICON              0x000000100     // get icon
#define SHGFI_DISPLAYNAME       0x000000200     // get display name
#define SHGFI_TYPENAME          0x000000400     // get type name
#define SHGFI_ATTRIBUTES        0x000000800     // get attributes
#define SHGFI_SYSICONINDEX      0x000004000     // get system icon index
#define SHGFI_LARGEICON         0x000000000     // get large icon
#define SHGFI_SMALLICON         0x000000001     // get small icon
#define SHGFI_USEFILEATTRIBUTES 0x000000010     // use passed dwFileAttribute

// supported SHGetFileInfo output attributes
#define SFGAO_LINK              0x00010000L     // Shortcut (link)
#define SFGAO_READONLY          0x00040000L     // read-only
#define SFGAO_FOLDER            0x20000000L     // It's a folder.

DWORD WINAPI SHGetFileInfo(LPCTSTR pszPath, DWORD dwFileAttributes,
						   SHFILEINFO *psfi, UINT cbFileInfo, UINT uFlags);

// supported SHGetSpecialFolderPath nFolder ids
#define CSIDL_DESKTOP            0x0000
#define CSIDL_PROGRAMS           0x0002
#define CSIDL_PERSONAL           0x0005
#define CSIDL_STARTUP            0x0007
#define CSIDL_RECENT             0x0008
#define CSIDL_FONTS              0x0014
#define CSIDL_FAVORITES          0x0016

BOOL WINAPI SHGetSpecialFolderPath(HWND hwndOwner, LPTSTR lpszPath,    
									  int nFolder, BOOL fCreate );

// @CESYSGEN ENDIF
////
//// End SHGetFileInfo
////


//
// SHAddToRecentDocs
//
// @CESYSGEN IF COREDLL_SHELLAPIS
#define SHARD_PIDL	0x00000001L
#define SHARD_PATH      0x00000002L
void WINAPI SHAddToRecentDocs(UINT uFlags, LPCVOID pv);
// @CESYSGEN ENDIF

// @CESYSGEN IF COREDLL_SHORTCUT
#define SHORTCUT_OVERWRITE				0x00000001
DWORD WINAPI SHCreateShortcut(LPTSTR szShortcut, LPTSTR szTarget);
BOOL WINAPI SHGetShortcutTarget(LPCTSTR szShortcut, LPTSTR szTarget, int cb);
// @CESYSGEN ENDIF

// @CESYSGEN IF COREDLL_SHMISC
int WINAPI SHShowOutOfMemory(HWND hwndOwner, UINT grfFlags);
HBITMAP WINAPI SHLoadDIBitmap(LPCTSTR szFileName);
// @CESYSGEN ENDIF

#ifndef UNDER_NT
#ifdef WINCEOEM
#ifdef WINCEMACRO
#include <mshellap.h>
#endif
#endif
#endif


#ifdef __cplusplus
}
#endif  /* __cplusplus */

#include <poppack.h>

#endif  /* _INC_SHELLAPI */
