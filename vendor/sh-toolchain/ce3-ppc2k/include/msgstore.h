/*---------------------------------------------------------------------------*\
 *
 * (c) Copyright Microsoft Corp. 1996 All Rights Reserved
 *
 *  module: msgstore.h
 *  date:   960105
 *
 *  purpose: Definitions for Mail Message Store
 *
\*---------------------------------------------------------------------------*/

#ifndef MSGSTORE_H
#define MSGSTORE_H

// Only include resource defs for the .RC file...
#ifndef __MSGSTORE_RC__

// Load all the filesystem/registry/database definitions
#ifdef UNDER_NT

// Is this a REMOTE version (versus local using the SDK)?
#ifdef MSGSTORE_RPC

#ifndef UNDER_REPL
#include <rapi.h>
#endif

#else //MSGSTORE_RPC Under the SDK...

// The Emulator has a conflict with _DBGPARAM definition...
#ifdef DEBUG
#define DEBUG_WAS_ON
#undef DBGCHK
#undef DEBUG
#undef DEBUGMSG
#undef DEBUGCHK
#undef DEBUGREGISTER
#endif //DEBUG


#ifdef DEBUG_WAS_ON
#undef DEBUG_WAS_ON
#define DEBUG
#endif //DEBUG_WAS_ON
#endif //MSGSTORE_RPC

#else //UNDER_NT We really are building for the device

#include <windbase.h>

#endif //UNDER_NT

/*------------------------------------------------------------------------*/
/* Useful constants and typedefs.                                         */
/*------------------------------------------------------------------------*/

#define MAIL_VERSION    0x0200

// Mail folders
#define MAIL_FOLDER_ALL         0x000000FF  // Source (current) folder
#define MAIL_FOLDER_FIRST       0x00000000  // Start of user defined folders
#define MAIL_FOLDER_LAST        0x00000014  // End of user defined folders
                                            // could go to 0xfb, but limit to 20
#define MAIL_FOLDER_SENT        0x000000FC  // copy of sent mail
#define MAIL_FOLDER_OUTBOX      0x000000FD  // local OUTBOX (last local folder)
#define MAIL_FOLDER_INBOX       0x000000FE  // univeral INBOX
#define MAIL_FOLDER_ANY         0x000000FF  // Search for any folder
#define MAIL_FOLDER_NAMELEN     16          // Max folder name length

#define MAIL_FOLDER_TARGET      0x0000FF00  // Target folder for MAIL_COPY
#define MAIL_TARGET_SHIFT       8           // number of bits to shift


// Status flags
#define MAIL_STATUS_ALL         0xC3FF0000  // Status of this message
#define MAIL_STATUS_FREE        0x00010000  // This is a free message
#define MAIL_STATUS_DELETE      0x00020000  // Delete message
#define MAIL_STATUS_COPY        0x00040000  // Copy message to local target
#define MAIL_STATUS_MOVE        0x00080000  // Marked for copy and delete
#define MAIL_STATUS_READ        0x00100000  // Marked as read (during a MailGet)
#define MAIL_STATUS_SERVICE     0x00200000  // Inbox Message exists on a service
#define MAIL_STATUS_COMPOSED    0x00400000  // Composed locally (To: address)
#define MAIL_STATUS_REMOTE      0x00800000  // Inbox body ONLY exists on service
#define MAIL_STATUS_GC          0x01000000  // Garbage collection

#define MAIL_STATUS_NOT         0x02000000  // Look for invert of status bits (GET)
#define MAIL_STATUS_ANY         (MAIL_STATUS_NOT|MAIL_STATUS_FREE)
#define MAIL_STATUS_ALIVE       (MAIL_STATUS_ANY|MAIL_STATUS_DELETE)
#define MAIL_STATUS_LOCAL       (MAIL_STATUS_ALIVE|MAIL_STATUS_SERVICE)
#define MAIL_STATUS_BODY        (MAIL_STATUS_ALIVE|MAIL_STATUS_REMOTE)
#define MAIL_STATUS_UNREAD      (MAIL_STATUS_ALIVE|MAIL_STATUS_READ)
#define MAIL_GET_BODY           0x08000000  // Get body (during a MailGet)

// Mail message store flags:
#define MAIL_FLAGS_ALL          0xFF000000  // All defined store flags
#define MAIL_GET_FLAGS          0x10000000  // ONLY get flags field (during a get)
#define MAIL_FULL               0x20000000  // Get full body when copying
#define MAIL_STATUS_ATTACHMENTS 0x40000000  // This message has attachments
#define MAIL_STATUS_ATT_REQ     0x80000000  // Attachment marked for download

#define MAIL_UNUSED             0x00000000  // Currently unused bits

#define MAXSTRPROPLEN   (16*4092)           // Database max string length
#define MAXSTRPROPCHARS (8*4092)            // Since we use WCHARS

typedef struct MailMsg_s {
    DWORD       dwMsgId;            // ID of message for retrieval (ignore on store)
    DWORD       dwFlags;            // MAIL_* flags
    DWORD       dwMsgLen;           // Length of complete body (on service)
    WORD        wBodyLen;           // Local body length (maintained by msgstore)
    FILETIME    ftDate;             // time message was received
    LPWSTR      szSvcId;            // Service defined ID
    LPWSTR      szSvcNam;           // Service name
    WCHAR*      pwcHeaders;         // NULL separated miscellaneous header fields
    LPWSTR      szBody;             // Message body
    CEOID       oid;                // Database object ID (leave alone!)
    HANDLE      hHeap;              // Heap for all message storage
} MailMsg;

typedef struct MailAtt_s {
    UINT        uiAttachmentNumber; // which attachment is specified
    DWORD       dwFlags;            // ATT_* flags
    ULONG       ulCharacterPosition;// not used
    ULONG       ulSize;             // the size of the attachment, in bytes
    LPWSTR      szOriginalName;     // the original name of the file
    LPWSTR      szLocalName;        // the local name of the file
} MailAtt, *lpMailAtt;

#pragma warning( disable:4200 ) // don't warn about 0-size array
typedef struct MailAttArray_s {
    DWORD       dwFlags;
    UINT        uiAttCount;
    lpMailAtt   AttArray[];
} MailAttArray, *lpMailAttArray;

typedef struct MailNonIpmInfo_s {
    LPWSTR      szMessageClass;
    UINT        uiPropCount;
    CEPROPVAL   propArray[];
} MailNonIpmInfo, *lpMailNonIpmInfo;
#pragma warning( default:4200 )


// Flags used in the MailAtt structure

#define ATT_ALL_FLAGS_MASK          0xFF000000
#define ATT_THIS_FLAGS_MASK         0x00FF0000
#define ATT_NUMBER_MASK             0x0000FFFF
#define ATT_DISP_RETRIEVE           0x00010000
#define ATT_DOWNLOADABLE            0x00020000

// Header strings used in the PROPID_ATT_STRINGS property

#define ATT_HEADER_NAME             L"A"
#define ATT_HEADER_SIZE             L"S"
#define ATT_HEADER_NAME_PREFIX      L"L"
#define ATT_HEADER_DISPOSITION      L"D"

// Per-attachment codes used in the ATT_HEADER_DISPOSITION field.

#define ATT_DISPOSITION_NONE        L' '
#define ATT_DISPOSITION_RETRIEVE    L'R'
#define ATT_DISPOSITION_DELETE      L'X'

#define MAX_ATTACHMENTS 64


// Mail error codes
#define MAIL_ERR_NONE           0
#define MAIL_ERR_BADPARAM       1
#define MAIL_ERR_BADHANDLE      2
#define MAIL_ERR_CANTALLOC      3

#define MAIL_ERR_NOSUCHDB       4
#define MAIL_ERR_CANTCREDB      5
#define MAIL_ERR_BADVER         6
#define MAIL_ERR_REOPEN         7

#define MAIL_ERR_NOHEADER       8
#define MAIL_ERR_BADHEADER      9
#define MAIL_ERR_WRITEHEADER    10

#define MAIL_ERR_WRITEFOLDER    11
#define MAIL_ERR_NOSUCHFOLDER   12
#define MAIL_ERR_BADFOLDER      13

#define MAIL_ERR_WRITEMSG       14
#define MAIL_ERR_WRITEBODY      15
#define MAIL_ERR_NOMATCH        16
#define MAIL_ERR_BADMSGID       17
#define MAIL_ERR_BADMSGREC      18
#define MAIL_ERR_BADMSGBODY     19
#define MAIL_ERR_BADOID         20
#define MAIL_ERR_UPDATEMSG      21
#define MAIL_ERR_CANTDEL        22
#define MAIL_ERR_BADHEADREC     23
#define MAIL_ERR_BADMSGHEAD     24
#define MAIL_ERR_BADPIECE       25
#define MAIL_ERR_NOHEAP         26
#define MAIL_ERR_CANTDELATT     27
#define MAIL_ERR_FOLDEREXISTS   28
#define MAIL_ERR_BADFOLDERNAME  29


/********************************************************************\
 *                                                                  *
 *  Mail Database Layout:                                           *
 *                                                                  *
 *  HEAD:       Id:UI4:0x00000000,Ver:UI2:LastID:UI4                *
 *  Msg:        Id:UI4:0xxxxxxxxx,Flags:UI4,MsgLen:UI4,BodyLen:UI2, *
 *                  Date:FT,From:WSTR,PreSubject:WSTR,                              *
 *                  Subject:WSTR,Headers:Blob,Body:LPWSTR,          *
 *                  SvcOid:UI4
 *                                                                  *
 *  Sort Fields:        From Subj Date BodyLen                      *
 *                                                                  *
 *                                                                  *
 *  Helper Database Layout:                                         *
 *                                                                  *
 *  Folder:     FldId:UI2:0x00xx,FldNam:WSTR                        *
 *  SvcId:      SvcNam:WSTR,SvcId:WSTR,MsgId:UI4,MsgOid:UI4         *
 *                                                                  *
 *  Sort Fields:        FldId FldNam SvcId MsgId                                 *
 *                                                                  *
\********************************************************************/

#define MAIL_ID_HEAD        0x00000000

#define MAKEPROP(n,t)   ((n<<16)|CEVT_##t)
#define PROPNUM(name)       (PROPID_##name >> 16)

//
// Message database fields.
//
#define PROPID_ID           MAKEPROP(0,UI4)
#define PROPID_FLAGS        MAKEPROP(1,UI4)
#define PROPID_MSGLEN       MAKEPROP(2,UI4)
#define PROPID_BODYLEN      MAKEPROP(3,UI2)
#define PROPID_DATE         MAKEPROP(4,FILETIME)
#define PROPID_FROM         MAKEPROP(5,LPWSTR)
#define PROPID_PRESUBJECT   MAKEPROP(6,LPWSTR)
#define PROPID_SUBJECT      MAKEPROP(7,LPWSTR)
#define PROPID_HEADERS      MAKEPROP(8,BLOB)
#define PROPID_SVCOID       MAKEPROP(9,UI4)
#define PROPID_BODY         MAKEPROP(10,LPWSTR)
#define PROPID_VERSION      MAKEPROP(11,UI2)
#define PROPID_LASTID       MAKEPROP(12,UI4)

//
// Helper database fields.
//
#define PROPID_FLDID        MAKEPROP(0,UI2)
#define PROPID_FLDNAM       MAKEPROP(1,LPWSTR)
#define PROPID_SVCNAM       MAKEPROP(2,LPWSTR)
#define PROPID_SVCID        MAKEPROP(3,LPWSTR)
#define PROPID_MSGID        MAKEPROP(4,UI4)
#define PROPID_MSGOID       MAKEPROP(5,UI4)

#define PROPID_ATT_INFO     MAKEPROP(12,UI4)
#define PROPID_ATT_STRINGS  MAKEPROP(13,BLOB)

/*-----------------------------------------------------------------------*/
/* Public routines.                                                      */
/*-----------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

int    MailError(              // Get last error <0=ERR >0=WRN 0=NONE
    HANDLE          hMail);             // Mail context

typedef int (*PMAILERROR)(HANDLE);

int     MailErrorMsg(           // Get last mail error (printable)
    HANDLE          hMail,              // Mail context
    LPWSTR          szBuf,              // Buffer to fill
    int             iBufLen,            // Length of buffer in chars
    int*            piSrcLine);         // Line where error occured (or NULL)

typedef int (*PMAILERRORMSG)(HANDLE,LPWSTR,int,int*);

BOOL    MailOpen(               // Get mail started
    HANDLE*         phMail,             // Returned handle
    BOOL            fAllowCreate);      // Allow creation of dir and db?

typedef BOOL (*PMAILOPEN)(HANDLE*,BOOL);

BOOL    MailOpenNotify(                 // Get mail started
    HANDLE*         phMail,             // Returned handle
    BOOL            fAllowCreate,       // Allow creation of dir and db?
    HWND            hwndNotify);        // HWND for DB notifications

typedef BOOL (*PMAILOPEN)(HANDLE*,BOOL);

BOOL    MailPutFolder(          // Set new folder name
    HANDLE          hMail,              // Current mail context
    BYTE            bId,                // Folder ID
    LPWSTR          szName);            // Name to set (NULL = DELETE)

typedef BOOL (*PMAILPUTFOLDER)(HANDLE,BYTE,LPWSTR);

BOOL    MailGetFolderName(      // Get folder name
    HANDLE          hMail,              // Current mail context
    int*            piId,               // In=Folder ID, Out=Next used ID or -1
    int*            piLen,              // In=buflen Out=needed chrs with terminator
    LPWSTR          szName);            // Buffer to fill (NULL=get length)

typedef BOOL (*PMAILGETFOLDERNAME)(HANDLE,int*,int*,LPWSTR);

BOOL    MailGetFolderId(        // Get folder ID
    HANDLE          hMail,              // Current mail context
    BYTE*           pbId,               // Folder ID to return
    LPWSTR          szName);            // Name to match

typedef BOOL (*PMAILGETFOLDERID)(HANDLE,BYTE*,LPWSTR);

BOOL    MailPut(                // Create a new mail entry
    HANDLE          hMail,              // Current mail context
    MailMsg*        pmm);               // Item information (dwMsgID ignored)

typedef BOOL (*PMAILPUT)(HANDLE,MailMsg*);

BOOL    MailPutAttachment(              // Create a new mail attachment
    HANDLE          hMail,              // Current mail context
    MailMsg*        pmm,                // message the attachment is on
    MailAtt*        pma);               // Item information

typedef BOOL (*PMAILPUTATTACHMENT)(HANDLE,MailMsg*,MailAtt*);

BOOL    MailDeleteAttachment(           // Delete a mail attachment
    HANDLE          hMail,              // Current mail context
    MailMsg*        pmm,                // message the attachment is on
    MailAtt*        pma,                // Item information
    BOOL            fFullDelete);       // Whether to delete header or not.

typedef BOOL (*PMAILDELETEATTACHMENT)(HANDLE,MailMsg*,MailAtt*,BOOL);

BOOL    MailGet(                // Get a message
    HANDLE          hMail,              // Current mail context
    MailMsg*        pmm);               //

typedef BOOL (*PMAILGET)(HANDLE,MailMsg*);

INT    MailGetAttachment(              // Get an attachment
    HANDLE          hMail,              // Current mail context
    MailMsg*        pmm,                //
    MailAtt*        pma);               //

typedef UINT (*PMAILGETATTACHMENT)(HANDLE,MailMsg*,MailAtt*);

BOOL    MailRequestAttachment(              // Request  attachment download
    HANDLE          hMail,              // Current mail context
    MailMsg*        pmm,                // mail msg attachment is on
    MailAtt*        pma,                // attachment being requested
    BOOL            bRetrieve);         // Request retrieve, or cancel?

typedef BOOL (*PMAILREQUESTATTACHMENT)(HANDLE,MailMsg*,MailAtt*,BOOL);

BOOL MailGetSvcId(              // Get a message OID based on szSvcNam/Id
    HANDLE          hMail,              // Current mail context
    MailMsg*        pmm);               // Returns pmm->oid and flags

typedef BOOL (*PMAILGETSVCID)(HANDLE,MailMsg*);

BOOL    MailFirst(              // Get first mail message
    HANDLE          hMail,              // Current mail context
    MailMsg*        pmm);               // Mail message to return

typedef BOOL (*PMAILFIRST)(HANDLE,MailMsg*);

BOOL    MailNext(               // Get next mail message
    HANDLE          hMail,              // Current mail context
    MailMsg*        pmm);               // Mail message to return (in=prev msg)

typedef BOOL (*PMAILNEXT)(HANDLE,MailMsg*);

BOOL    MailUpdate(             // Update flags from a previous get
    HANDLE          hMail,              // Current mail context
    MailMsg*        pmm,                // Message with updated flags
    BOOL            fDoAll);            // Update all other fields as well?

typedef BOOL (*PMAILUPDATE)(HANDLE,MailMsg*,BOOL);

BOOL    MailReadNonIPMProperties(// API for reading extra properties, such as schedule info, from a msg
    HANDLE          hMail,              // Current mail context
    MailMsg*        pmm,                // Message to read said properties from
    CEPROPVAL**     ppPropVal,          // prop list (allocated - caller must free with LocalFree)
    WORD*           pcPropVal);         // count of props in the list

typedef BOOL (*PMAILREADNONIPMPROPERTIES)(HANDLE,MailMsg*,CEPROPVAL**,WORD*);

BOOL    MailWriteNonIPMProperties(// API for writing extra properties, such as schedule info, to a msg
    HANDLE          hMail,              // Current mail context
    MailMsg*        pmm,                // Message to add said properties to
    CEPROPVAL*      pPropVal,           // prop list
    UINT            cPropVal);          // count of props in the list

typedef BOOL (*PMAILWRITENONIPMPROPERTIES)(HANDLE,MailMsg*,CEPROPVAL*,UINT);

BOOL    MailDelete(             // Remove a mail message
    HANDLE          hMail,              // Current mail context
    MailMsg*        pmm);               // Mail message to delete

typedef BOOL (*PMAILDELETE)(HANDLE,MailMsg*);

BOOL    MailFree(               // Give back storage from a prev Get
    MailMsg*        pmm);               // Mail message to free

typedef BOOL (*PMAILFREE)(MailMsg*);

BOOL    MailClose(              // Delete mail context
    HANDLE          hMail);             // Current mail context

typedef BOOL (*PMAILCLOSE)(HANDLE);

// Support routines

DWORD   MailHeaderLen(          // Compute chars in header string
    MailMsg*        pmm);               // String of headers

typedef DWORD (*PMAILHEADERLEN)(MailMsg*);

DWORD   MailLocalAttachmentLen(          // Compute chars in header string
    HANDLE          hMail,              // Current mail context
    MailMsg*        pmm);               // String of headers

typedef DWORD (*PMAILATTACHMENTLEN)(HANDLE,MailMsg*);

LPWSTR MailGetField(            // Get one header field
    MailMsg*        pmm,                // Message to read from
    LPWSTR          szName,             // Name of field
    BOOL            fGetName);          // Get position of Key instead of val

typedef LPWSTR (*PMAILGETFIELD)(MailMsg*,LPWSTR,BOOL);

BOOL MailSetField(              // Set a field (FALSE=ALLOC error)
    MailMsg*        pmm,                // Message to work with
    LPWSTR          szName,             // Name of field
    LPCWSTR         szVal);             // New value (or NULL to delete)

typedef BOOL (*PMAILSETFIELD)(MailMsg*,LPWSTR,LPCWSTR);

typedef enum {
    MAIL_SORT_FROM,
    MAIL_SORT_SUBJ,
    MAIL_SORT_DATE,
    MAIL_SORT_SIZE
} MAILSORTFIELD;

BOOL
MailSetSort(
    HANDLE hMail,                       // Current mail context
    MAILSORTFIELD iSort,                // new sort order
    BOOL fAscending                     // new sort direction; TRUE if ascending
    );

typedef BOOL (*PMAILSETSORT)(HANDLE, MAILSORTFIELD, BOOL);

typedef struct _MAILSORTINFO {
    MAILSORTFIELD iSort;                // sort order
    BOOL fAscending;                    // sort direction
    int cMsgs;                          // message count
} MAILSORTINFO, *PMAILSORTINFO;

BOOL
MailGetSort(
    HANDLE hMail,                       // Current mail context
    PMAILSORTINFO pInfo                 // information structure
    );

typedef BOOL (*PMAILGETSORT)(HANDLE, MAILSORTFIELD *, BOOL *);

BOOL MailSetAttachmentOptions(              // Set a field (FALSE=ALLOC error)
    HANDLE          hMail,
    MailMsg*        pmm,                // Message to work with
    DWORD           dwFlags);

typedef BOOL (*PMAILSETATTACHMENTOPTIONS)(HANDLE,MailMsg*,DWORD);

//////////////////////// Emulation/RPC Support routines  ///////////////////////

LONG MailRegOpenKeyExW(         // Open a pegasus registry key
    HKEY    hKey,                       // handle of open key
    LPCWSTR lpSubKey,                   // address of name of subkey to open
    DWORD   ulOptions,                  // reserved
    REGSAM  samDesired,                 // security access mask
    PHKEY   phkResult);                 // address of handle of open key

typedef LONG (*PMAILREGOPENKEYEXW)(HKEY,LPCWSTR,DWORD,REGSAM,PHKEY);

LONG MailRegEnumKeyExW(         // Retrieve one key
    HKEY    hKey,                       // handle of key to enumerate
    DWORD   dwIndex,                    // index of subkey to enumerate
    LPWSTR  lpName,                     // address of buffer for subkey name
    LPDWORD lpcbName,                   // address for size of subkey buffer
    LPDWORD lpReserved,                 // reserved
    LPWSTR  lpClass,                    // address of buffer for class string
    LPDWORD lpcbClass,                  // address for size of class buffer
    PFILETIME lpftLastWriteTime);       // address for time key last written to

typedef LONG (*PMAILREGENUMKEYEXW)(HKEY,DWORD,LPWSTR,LPDWORD,LPDWORD,LPWSTR,LPDWORD,PFILETIME);

LONG MailRegCloseKey(           // Close a key
    HKEY    hKey);                      // handle of key to close

typedef LONG (*PMAILREGCLOSEKEY)(HKEY);

#ifdef __cplusplus
};
#endif

//
// TransportEx flags.
//
#define TVEX_DISABLE_PREV    0x00000001
#define TVEX_DISABLE_NEXT    0x00000002

//
// TransportViewEx return codes.
//
#define TVEX_VIEW_CURRENT  0   // read current message
#define TVEX_VIEW_DONE     1   // no further processing
#define TVEX_VIEW_PREV     2   // display previous message
#define TVEX_VIEW_NEXT     3   // display next message
#define TVEX_VIEW_REPLY    4   // reply to current message
#define TVEX_VIEW_REPLYALL 5   // reply-all to current message
#define TVEX_VIEW_FORWARD  6   // forward current message
#define TVEX_VIEW_DELETE   7   // delete current message

#endif // !__MSGSTORE_RC__

#endif // MSGSTORE_H
