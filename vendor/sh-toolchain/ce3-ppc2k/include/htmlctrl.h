/*---------------------------------------------------------------------------*\
 *
 * (c) Copyright Microsoft Corp. 1996-1999 All Rights Reserved
 *
 *  module: htmlctrl.h
 *  date:
 *  author: scottsh jaym
 *
\*---------------------------------------------------------------------------*/

#ifndef __HTMLCTRL_H__
#define __HTMLCTRL_H__

////////////////////////////////////////////////////////////////////////////////


#define DTM_ADDTEXT             (WM_USER + 101)
#define DTM_ADDTEXTW            (WM_USER + 102)
#define DTM_SETIMAGE            (WM_USER + 103)
#define DTM_ENDOFSOURCE         (WM_USER + 104)
#define DTM_ANCHOR              (WM_USER + 105)
#define DTM_ANCHORW             (WM_USER + 106)
#define DTM_ENABLESHRINK        (WM_USER + 107)
#define DTM_FITTOWINDOW         (WM_USER + 107)
#define DTM_SCROLLINTOVIEW      (WM_USER + 108)
#define DTM_IMAGEFAIL           (WM_USER + 109)
#define DTM_REALLYDONE          (WM_USER + 110)
#define DTM_SELECTALL           (WM_USER + 111)
#define DTM_ISSELECTION         (WM_USER + 112)
#define DTM_CLEAR               (WM_USER + 113)
#define DTM_ENABLECLEARTYPE     (WM_USER + 114)
#define DTM_ENABLESCRIPTING     (WM_USER + 115)
#define DTM_ZOOMLEVEL           (WM_USER + 116)
#define DTM_LAYOUTWIDTH         (WM_USER + 117)
#define DTM_LAYOUTHEIGHT        (WM_USER + 118)

#define NM_HOTSPOT              (WM_USER + 101)
#define NM_INLINE_IMAGE         (WM_USER + 102)
#define NM_INLINE_SOUND         (WM_USER + 103)
#define NM_TITLE                (WM_USER + 104)
#define NM_META                 (WM_USER + 105)
#define NM_BASE                 (WM_USER + 106)
#define NM_CONTEXTMENU          (WM_USER + 107)
#define NM_INLINE_XML           (WM_USER + 108)

#define DISPLAYCLASS    TEXT("DISPLAYCLASS")

EXTERN_C BOOL InitHTMLControl(HINSTANCE hinst);

typedef struct tagNM_HTMLVIEWA
{
    NMHDR   hdr; 
    LPCSTR  szTarget;
    LPCSTR  szData;
    DWORD   dwCookie;
} NM_HTMLVIEWA; 

typedef struct tagNM_HTMLVIEWW
{
    NMHDR   hdr; 
    LPCWSTR szTarget;
    LPCWSTR szData;
    DWORD   dwCookie;
} NM_HTMLVIEWW;

#ifdef UNICODE
#define NM_HTMLVIEW NM_HTMLVIEWW
#else
#define NM_HTMLVIEW NM_HTMLVIEWA
#endif  // UNICODE

typedef struct tagINLINEIMAGEINFO
{
    DWORD       dwCookie;
    int         iOrigHeight;
    int         iOrigWidth;
    HBITMAP     hbm;
    BOOL        bOwnBitmap;
} INLINEIMAGEINFO, *LPINLINEIMAGEINFO;

////////////////////////////////////////////////////////////////////////////////

#endif //__HTMLCTRL_H__
