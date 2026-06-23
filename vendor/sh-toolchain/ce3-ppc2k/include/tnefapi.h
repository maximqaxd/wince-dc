/*
 *  TNEF.H
 *
 *  This declares the published interface to TNEFUTIL.DLL, a set of
 *  utility fuctions designed to encode/decode unusual mail properties in
 *  a Transport-Neutral form.
 *
 *  Owner: Dave Whitney
 *
 *  Copyright 1986-1998 Microsoft Corporation, All Rights Reserved
 */

#ifndef     _TNEFAPI_H_
#define     _TNEFAPI_H_

#ifdef      __cplusplus
extern "C" {
#endif      /* __cplusplus */

/**************************************************/
/* Constants **************************************/
/**************************************************/

/*
 *  Type INETENCODE - describes the encoding type for MIME encodings
 */
typedef enum {
    CODE_NONE = 0,
    CODE_UU,                                /* UUEncode                         */
    CODE_B64,                               /* base64                           */
    CODE_QP,                                /* quoted-printable                 */
    CODE_QPH,                               /* quoted-printable, in msg hdr     */
    CODE_QP_SIZE,                           /* measure the size or quoted-printable data */
} INETENCODE;


/*
 *  Codepages supported by TnefConverToUnicode
 */

#define CPG_US_ASCII            1

#define CPG_IBM_852             852
#define CPG_IBM_866             866

#define CPG_THAI                874
#define CPG_JPN_SJ              932
#define CPG_CHN_GB              936
#define CPG_KOR_5601            949
#define CPG_TWN                 950

#define CPG_UCS_2               1200
#define CPG_UCS_2_BE            1201

#define CPG_WINDOWS_1250        1250
#define CPG_WINDOWS_1251        1251
#define CPG_WINDOWS_1252        1252
#define CPG_WINDOWS_1253        1253
#define CPG_WINDOWS_1254        1254
#define CPG_WINDOWS_1255        1255
#define CPG_WINDOWS_1256        1256
#define CPG_WINDOWS_1257        1257
#define CPG_WINDOWS_1258        1258

#define CPG_KOI8_R              20866
#define CPG_KOI8_U              21866

#define CPG_ISO8859_1           28591
#define CPG_ISO8859_2           28592
#define CPG_ISO8859_3           28593
#define CPG_ISO8859_4           28594
#define CPG_ISO8859_5           28595
#define CPG_ISO8859_6           28596
#define CPG_ISO8859_7           28597
#define CPG_ISO8859_8           28598
#define CPG_ISO8859_9           28599
#define CPG_ISO8859_10          28600
#define CPG_ISO8859_14          28604
#define CPG_ISO8859_15          28605

#define CPG_USER_DEFINED        50000

#define CPG_ISO_2022_JP         50220
#define CPG_ISO_2022_JP_ESC     50221
#define CPG_ISO_2022_JP_SIO     50222
#define CPG_ISO_2022_KR         50225
#define CPG_ISO_2022_TW         50226
#define CPG_ISO_2022_CH         50227

#define CPG_JP_AUTO             50932
#define CPG_KR_AUTO             50949

#define CPG_EUC_JP              51932
#define CPG_EUC_CH              51936
#define CPG_EUC_KR              51949
#define CPG_EUC_TW              51950

#define CPG_CHN_HZ              52936

#define CPG_UTF7                65000
#define CPG_UTF8                65001



/**************************************************/
/* Structures *************************************/
/**************************************************/

/*
 *  Type TNEFMIMEINFO
 *
 *  Used by TnefGetInfo, TnefBuildInfo
 */
#include <pshpack1.h>
typedef struct _tnefinfo {
    INETENCODE  wContentEncoding;           /* Content encoding                             */
    BOOL        fAttHdrs;                   /* Tells TnefGetInfo to get att headers only    */
    HANDLE      hHeap;                      /* Heap (from msg) to allocate from             */
    LPWSTR      szTnefCorrelator;           /* TNEF correlator from msg header              */
    WCHAR       szTnefFileName[MAX_PATH];   /* File name of TNEF file, if found             */
} TNEFINFO;
#include <poppack.h>

/**************************************************/
/* Functions **************************************/
/**************************************************/

/*
 *  TnefGetInfo
 *
 *  Purpose:
 *      Pulls info out of an encoded TNEF file
 *
 *  Parameters:
 *      TNEFINFO *          [in]        describes the TNEF file
 *      MailNonIpmInfo *    [out]       filled in
 *      MailAttArray *      [out]       filled in
 *
 *  Returns:
 *      BOOL        TRUE on success
 *
 *  Side Effects:
 *      * The file identified by ptmi->szTnefFileName is deleted once
 *        processing is completed - error or not
 *      * The remaining parms are filled in
 */
BOOL WINAPI TnefGetInfo(const TNEFINFO *, MailNonIpmInfo **, MailAttArray **);
typedef BOOL (WINAPI * LPFNTNEFGETINFO) (const TNEFINFO *, MailNonIpmInfo **, MailAttArray **);

/*
 *  TnefBuildInfo
 *
 *  Purpose:
 *      Puts info into an encoded TNEF file.
 *      Attachments are not supported in this release.
 *
 *  Parameters:
 *      TNEFINFO *          [in]        describes the TNEF file
 *      LPWSTR              [in]        msg subject
 *      MailNonIpmInfo *    [in]        other msg properties
 *      MailAttArray *      [in]        msg attachments
 *
 *  Returns:
 *      BOOL        TRUE indicates success
 *
 *  Side Effects:
 *      szTnefFileName is created on success. Caller is responsible for deleting it.
 *
 */
BOOL WINAPI TnefBuildInfo(const TNEFINFO *, LPWSTR, MailNonIpmInfo *, MailAttArray *);
typedef BOOL (WINAPI * LPFNTNEFBUILDINFO) (const TNEFINFO *, LPWSTR, MailNonIpmInfo *, MailAttArray *);

/*
 *  TnefEncodeFile
 *
 *  Purpose:
 *      Encode an attachment for sending
 *
 *  Parameters:
 *      LPWSTR              [in]        attachment filename
 *      INETENCODE          [in]        encoding method
 *
 *  Returns:
 *      LPWSTR  - filename of encoded file
 *
 *  Side Effects:
 *      file is created
 *
 *  Notes:
 *      Input file is not affected.
 *      Returned file must be deleted by caller.
 *      returned name must be LocalFree'd by caller
 */
LPWSTR WINAPI TnefEncodeFile(LPWSTR, INETENCODE);
typedef LPWSTR (WINAPI * LPFNTNEFENCODEFILE) (LPWSTR, INETENCODE);

/*
 *  TnefDecodeFile
 *
 *  Purpose:
 *      Decodes a file
 *
 *  Parameters:
 *      INETENCODE          [in]        encoding method
 *      LPWSTR              [in]        name of file to decode
 *      LPWSTR              [in]        name of output file
 *
 *  Returns:
 *      BOOL            TRUE on success
 *
 *  Side Effects:
 *      DecodedFileName is created. Caller is responsible for deleting.
 */
BOOL WINAPI TnefDecodeFile(INETENCODE, LPWSTR, LPWSTR);
typedef BOOL (WINAPI * LPFNTNEFDECODEFILE) (INETENCODE, LPWSTR, LPWSTR);

/*
 *  TnefEncodeBuffer
 *
 *  Purpose:
 *      Encode some binary
 *
 *  Parameters:
 *      INETENCODE          [in]        encoding method
 *      UINT                [in]        size of input buffer
 *      BYTE *              [in]        data to encode
 *      UINT *              [in/out]    size of output buffer/encoded data
 *      LPSTR               [in/out]    encoded data
 *
 *  Returns:
 *      BOOL        TRUE on success
 *
 *  Notes:
 *      Output buffer needs to be big enough to contain
 *      all encoded data.
 */
BOOL WINAPI TnefEncodeBuffer(INETENCODE, UINT, BYTE *, UINT *, LPSTR);
typedef BOOL (WINAPI * LPFNTNEFENCODEBUFFER) (INETENCODE, UINT, BYTE *, UINT *, LPSTR);

/*
 *  TnefDecodeBuffer
 *
 *  Purpose:
 *      Decode a buffer of ANSI text
 *
 *  Parameters:
 *      INETENCODE          [in]        encoding method
 *      LPSTR               [in]        buffer to decode
 *      BYTE *              [in]        decoded buffer
 *      UINT *              [in/out]    size of output buffer/decoded data
 *
 *  Returns:
 *      BOOL        TRUE on success
 *
 *  Notes:
 *      Output buffer needs to be big enough to contain
 *      all decoded data.
 */
BOOL WINAPI TnefDecodeBuffer(INETENCODE, LPSTR, BYTE *, UINT *);
typedef BOOL (WINAPI * LPFNTNEFDECODEBUFFER) (INETENCODE, LPSTR, BYTE *, UINT *);

/*
 *  TnefConvertToUnicode
 *
 *  Purpose:
 *      Convert a string encoded in the specified codepage to Unicode
 *
 *  Parameters:
 *      UINT                [in]        codepage to decode from
 *      DWORD               [in]        character-type options
 *      LPCSTR              [in]        input string
 *      int                 [in]        length of the input string
 *      LPWSTR              [in]        output buffer
 *      int                 [in]        size of the output buffer
 *
 *  Returns:
 *      int        if output buffer size is not 0, number of charactes copied to it;
 *                 otherwise, size of the buffer required to contain the output string;
 *                 0 indicates failure
 */
int WINAPI TnefConvertToUnicode(UINT, DWORD, LPCSTR, int, LPWSTR, int);
typedef int (WINAPI * LPFNTNEFCONVERTTOUNICODE) (UINT, DWORD, LPCSTR, int, LPWSTR, int);

/*
 *  TnefConvertToUnicodeEx
 *
 *  Purpose:
 *      Convert a string encoded in the specified codepage to Unicode, autodetecting
 *      the source codepage if necessary
 *
 *  Parameters:
 *      UINT                [in]        codepage to decode from
 *      DWORD               [in]        character-type options
 *      LPCSTR              [in]        input string
 *      int                 [in]        length of the input string
 *      LPWSTR              [in]        output buffer
 *      int                 [in]        size of the output buffer
 *      UINT*               [out]       detected source codepage
 *
 *  Returns:
 *      int        if output buffer size is not 0, number of charactes copied to it;
 *                 otherwise, size of the buffer required to contain the output string;
 *                 0 indicates failure
 */
int WINAPI TnefConvertToUnicodeEx(UINT, DWORD, LPCSTR, int, LPWSTR, int, UINT*);
typedef int (WINAPI * LPFNTNEFCONVERTTOUNICODEEX) (UINT, DWORD, LPCSTR, int, LPWSTR, int, UINT*);

/*
 *  TnefConvertFromUnicode
 *
 *  Purpose:
 *      Convert a string from Unicode to the specified codepage
 *
 *  Parameters:
 *      UINT                [in]        codepage to convert to
 *      DWORD               [in]        character-type options
 *      LPCWSTR             [in]        input string
 *      int                 [in]        length of the input string
 *      LPSTR               [in]        output buffer
 *      int                 [in]        size of the output buffer
 *
 *  Returns:
 *      int        if output buffer size is not 0, number of charactes copied to it;
 *                 otherwise, size of the buffer required to contain the output string;
 *                 0 indicates failure
 */
int WINAPI TnefConvertFromUnicode(UINT, DWORD, LPCWSTR, int, LPSTR, int);
typedef int (WINAPI * LPFNTNEFCONVERTFROMUNICODE) (UINT, DWORD, LPCWSTR, int, LPSTR, int);

/*
 *  TnefGetFirstCodePage
 *
 *  Purpose:
 *      Create a codepage enumerator and start enumerating
 *
 *  Parameters:
 *      HANDLE*             [out]       handle to the enumerator
 *      UINT*               [out]       first codepage
 *      LPWSTR              [out]       buffer for the description of the first codepage
 *      UINT                [in]        length of the description buffer
 *
 *  Returns:
 *      BOOL        TRUE on success
 */
BOOL WINAPI TnefGetFirstCodePage(HANDLE*, UINT*, LPWSTR, UINT);
typedef BOOL (WINAPI * LPFNTNEFGETFIRSTCODEPAGE) (HANDLE*, UINT*, LPWSTR, UINT);

/*
 *  TnefGetNextCodePage
 *
 *  Purpose:
 *      Get the next codepage using the codepage enumerator
 *
 *  Parameters:
 *      HANDLE              [in]        handle to the enumerator
 *      UINT*               [out]       codepage
 *      LPWSTR              [out]       buffer for the description of the codepage
 *      UINT                [in]        length of the description buffer
 *
 *  Returns:
 *      BOOL        TRUE on success
 *
 *  Notes:
 *      Codepage descriptions are no longer than 64 characters
 */
BOOL WINAPI TnefGetNextCodePage(HANDLE, UINT*, LPWSTR, UINT);
typedef BOOL (WINAPI * LPFNTNEFGETNEXTCODEPAGE) (HANDLE, UINT*, LPWSTR, UINT);

/*
 *  TnefFreeCodePageEnum
 *
 *  Purpose:
 *      Free the codepage enumerator
 *
 *  Parameters:
 *      HANDLE              [in]        handle to the enumerator to be freed
 *
 *  Returns:
 *      void
 */
void WINAPI TnefFreeCodePageEnum(HANDLE);
typedef void (WINAPI * LPFNTNEFFREECODEPAGEENUM) (HANDLE);

/*
 *  TnefDetectTextCodePage
 *
 *  Purpose:
 *      Determine a codepage that can be used to encode the specified Unicode text
 *
 *  Parameters:
 *      LPWSTR              [in]        Unicode text
 *      UINT*               [out]       codepage
 *      BOOL*               [out]       TRUE, if it was impossible to find a
 *                                      single codepage capable of encoding all of the text
 *
 *  Returns:
 *      BOOL        TRUE on success
 *
 *  Notes:
 *      This routine should not be relied upon to produce precise results.
 *      The codepage returned to the caller may not be capable of encoding
 *      all of the specified text.
 */
BOOL WINAPI TnefDetectTextCodePage(LPWSTR, UINT*, BOOL*);
typedef BOOL (WINAPI * LPFNTNEFDETECTTEXTCODEPAGE) (LPWSTR, UINT*, BOOL*);

/*
 *  TnefCharsetToCodePage
 *
 *  Purpose:
 *      Map an IANA/ISO charset to a corresponding codepage
 *
 *  Parameters:
 *      LPWSTR              [in]        IANA/ISO charset name to be mapped
 *      UINT*               [out]       codepage
 *
 *  Returns:
 *      BOOL        TRUE on success
 */
BOOL WINAPI TnefCharsetToCodePage(LPWSTR, UINT*);
typedef BOOL (WINAPI * LPFNTNEFCHARSETTOCODEPAGE) (LPWSTR, UINT*);

/*
 *  TnefCodePageToCharset
 *
 *  Purpose:
 *      Map a codepage to a corresponding IANA/ISO charset
 *
 *  Parameters:
 *      UINT                [in]        codepage to be mapped
 *      LPWSTR              [out]       buffer to contain IANA/ISO charset name
 *      UINT                [in]        length of the charset buffer
 *
 *  Returns:
 *      BOOL        TRUE on success
 *
 *  Notes:
 *      Charset names are no longer than 50 characters
 */
BOOL WINAPI TnefCodePageToCharset(UINT, LPWSTR, UINT);
typedef BOOL (WINAPI * LPFNTNEFCODEPAGETOCHARSET) (UINT, LPWSTR, UINT);

/*
 *  ModifiedUTF7ToUnicode
 *
 *  Purpose:
 *      Converts the IMAP "modified" UTF7 string into straight Unicode.
 *      Includes quoted chars:  \\   \"   &-
 *
 *  Parameters:
 *      LPCSTR              [in]        address of string to convert
 *      int                 [in]        number of bytes in string
 *      LPWSTR              [out]       address of wide-character buffer
 *      int                 [in]        size of output buffer
 *
 *  Returns:
 *      int             character count of Unicode string, including NULL
 *                      terminator.
 */
int WINAPI ModifiedUTF7ToUnicode(LPCSTR lpMultiByteStr, int cchMultiByte, LPWSTR lpWideCharStr, int cchWideChar);
typedef int (WINAPI * LPFNMODIFIEDUTF7TOUNICODE) (LPCSTR, int, LPWSTR, int);

/*
 *  UnicodeToModifiedUTF7
 *
 *  Purpose:
 *      Converts the straight Unicode string into IMAP "modified" UTF7.
 *      Quotes chars:  \   "   &
 *
 *  Parameters:
 *      LPCWSTR             [in]        address of string to convert
 *      int                 [in]        number of bytes in string
 *      LPSTR               [out]       address of wide-character buffer
 *      int                 [in]        size of output buffer
 *
 *  Returns:
 *      int             character count of Unicode string, including NULL
 *                      terminator.
 */
int WINAPI UnicodeToModifiedUTF7(LPCWSTR wszIn, int cchIn, LPSTR pchOut, int cchOut);
typedef int (WINAPI * LPFNUNICODETOMODIFIEDUTF7) (LPCWSTR, int, LPSTR, int);

#ifdef      __cplusplus
}
#endif      /* __cplusplus */

#endif      /* _TNEFAPI_H_ */
