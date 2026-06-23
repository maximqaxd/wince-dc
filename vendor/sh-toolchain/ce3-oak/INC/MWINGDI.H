/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

Module Name:  

mwingdi.h

Abstract:  

Notes: 


--*/

#ifdef __cplusplus
extern "C" {
#endif

//
// THIS API LIST MUST COME BEFORE THE IFDEF ON _WINGDIP_H_!
//
#ifndef LAST_GDI_API_INDEX

#define ADDFONTRESOURCEW		2
#define BITBLT					3
#define COMBINERGN				4
#define CREATECOMPATIBLEDC		5
#define CREATEDIBPATTERNBRUSHPT	6
#define CREATEDIBSECTION		7
#define CREATEFONTINDIRECT		8
#define CREATERECTRGNINDIRECT	9
#define CREATEPENINDIRECT		10
#define CREATESOLIDBRUSH		11
#define DELETEDC				12
#define DELETEOBJECT			13
#define DRAWEDGE				14
#define DRAWFOCUSRECT			15
#define DRAWTEXTW				16
#define ELLIPSE					17
#define ENUMFONTFAMILIESW		18
#define ENUMFONTSW				19
#define EXCLUDECLIPRECT			20
#define EXTTEXTOUTW				21
#define FILLRECT				22
#define GETBKCOLOR				24
#define GETBKMODE				25
#define GETCLIPRGN				26
#define GETCURRENTOBJECT		27
#define GETDEVICECAPS			28
#define GETNEARESTCOLOR			29
#define GETOBJECT				30
#define GETOBJECTTYPE			31
#define GETPIXEL				32
#define GETREGIONDATA			33
#define GETRGNBOX				34
#define GETSTOCKOBJECT			35
#define PATBLT					36
#define GETTEXTCOLOR			37
#define GETTEXTEXTENTEXPOINTW	38
#define GETTEXTFACEW			39
#define GETTEXTMETRICS			40
#define MASKBLT					41
#define OFFSETRGN				42
#define POLYGON					43
#define POLYLINE				44
#define PTINREGION				45
#define RECTANGLE				46
#define RECTINREGION			47
#define REMOVEFONTRESOURCEW		48
#define RESTOREDC				49
#define ROUNDRECT				50
#define SAVEDC					51
#define SELECTCLIPRGN			52
#define SELECTOBJECT			53
#define SETBKCOLOR				54
#define SETBKMODE				55
#define SETBRUSHORGEX			56
#define SETPIXEL				57
#define SETTEXTCOLOR			58
#define STRETCHBLT				59
#define CREATEBITMAP            60
#define CREATECOMPATIBLEBITMAP  61
#define GETSYSCOLORBRUSH		62
#define INTERSECTCLIPRECT   	63
#define GETCLIPBOX				64
#define CEREMOVEFONTRESOURCE    65
#define ENABLEEUDC              66
#define CLOSEENHMETAFILE        67
#define CREATEENHMETAFILEW      68
#define DELETEENHMETAFILE       69
#define PLAYENHMETAFILE         70
#define CREATEPALETTE           71
#define SELECTPALETTE           72
#define REALIZEPALETTE          73
#define GETPALETTEENTRIES       74
#define SETPALETTEENTRIES       75
#define GETSYSTEMPALETTEENTRIES 76
#define GETNEARESTPALETTEINDEX  77
#define CREATEPEN               78
#define STARTDOC                79
#define ENDDOC                  80
#define STARTPAGE               81
#define ENDPAGE                 82
#define ABORTDOC                83
#define SETABORTPROC            84
#define CREATEDC                85
#define CREATERECTRGN           86
#define FILLRGN                 87
#define SETROP2                 88
#define SETRECTRGN              89
#define RECTVISIBLE             90
#define CREATEPATTERNBRUSH      91
#define CREATEBITMAPFROMPOINTER 92
#define SETVIEWPORTORGEX        93
#define TRANSPARENTIMAGE        94
#define SETOBJECTOWNER          95
#define TRANSLATECHARSETINFO    96
#define EXTESCAPE               97

//	These are used.  The macros are in mwinuser.h
//#define MID_SetWindowsHookExW		98
//#define MID_UnhookWindowsHookEx	99
//#define MID_GetForegroundInfo		100
//#define MID_SetWindowRgn		109
//#define MID_GetDDrawCallbacks 110
//#define MID_GetWindowRgn		111
//#define MID_GetDesktopWindow	113
//#define MID_InSendMessage		114
//#define MID_GetQueueStatus	115
//#define MID_AllKeys			116


#endif  // !LAST_GDI_API_INDEX


//
// Now we can do the conditional on _WINGDIP_H_
//
#ifndef _WINGDIP_H_
#define _WINGDIP_H_

#ifndef WINCEMACRO
#error WINCEMACRO not defined __FILE__
#endif



// @CESYSGEN IF GWES_PGDI || GWES_MGBASE
#define CreateCompatibleDC											\
			IMPLICIT_DECL(HDC, SH_GDI, CREATECOMPATIBLEDC,			\
				(HDC hdc))
#define DeleteDC													\
			IMPLICIT_DECL(BOOL, SH_GDI, DELETEDC,					\
				(HDC hdc))
#define DeleteObject												\
			IMPLICIT_DECL(BOOL, SH_GDI, DELETEOBJECT,				\
				(HGDIOBJ hObject))
#define GetBkColor													\
			IMPLICIT_DECL(COLORREF, SH_GDI, GETBKCOLOR,				\
				(HDC hdc))
#define GetBkMode													\
			IMPLICIT_DECL(int, SH_GDI, GETBKMODE,					\
			(HDC hdc))
#define GetCurrentObject											\
			IMPLICIT_DECL(HGDIOBJ, SH_GDI, GETCURRENTOBJECT,		\
				(HDC hdc,											\
				UINT uObjectType))
#define GetDeviceCaps												\
			IMPLICIT_DECL(int, SH_GDI, GETDEVICECAPS,				\
				(HDC hdc,											\
				int nIndex))
#define GetObjectW													\
			IMPLICIT_DECL(int, SH_GDI, GETOBJECT,					\
				(HGDIOBJ hgdiobj,									\
				int cbBuffer,										\
				LPVOID lpvObject))
#define GetObjectType												\
			IMPLICIT_DECL(DWORD, SH_GDI, GETOBJECTTYPE,				\
				(HGDIOBJ hgdiobj))
#define GetStockObject												\
			IMPLICIT_DECL(HGDIOBJ, SH_GDI, GETSTOCKOBJECT,			\
				(int fnObject))
#define GetTextColor												\
			IMPLICIT_DECL(COLORREF, SH_GDI, GETTEXTCOLOR,			\
				(HDC hdc))
#define SelectObject												\
			IMPLICIT_DECL(HGDIOBJ, SH_GDI, SELECTOBJECT,			\
				(HDC hdc,											\
				HGDIOBJ hgdiobj))
#define SetBkColor													\
			IMPLICIT_DECL(COLORREF, SH_GDI, SETBKCOLOR,				\
				(HDC hdc,											\
				COLORREF crColor))
#define SetBkMode													\
			IMPLICIT_DECL(int, SH_GDI, SETBKMODE,					\
				(HDC hdc,											\
				int iBkMode))
#define SetTextColor												\
			IMPLICIT_DECL(COLORREF, SH_GDI, SETTEXTCOLOR,			\
				(HDC hdc,											\
				COLORREF crColor))
// @CESYSGEN ENDIF


// @CESYSGEN IF GWES_MGDRAW
#define CreatePatternBrush										    \
			IMPLICIT_DECL(HBRUSH, SH_GDI, CREATEPATTERNBRUSH,	    \
				(HBITMAP  hbmp))        							
#define CreatePen       											\
			IMPLICIT_DECL(HPEN, SH_GDI, CREATEPEN,      			\
            (int  fnPenStyle,                                       \
             int  nWidth,                                           \
             COLORREF  crColor))
#define SetROP2     												\
			IMPLICIT_DECL(int, SH_GDI, SETROP2,         			\
				(HDC hdc,											\
				int  fnDrawMode))
// @CESYSGEN ENDIF


// @CESYSGEN IF GWES_PGDI || GWES_MGDRAW
#define CreateDIBPatternBrushPt										\
			IMPLICIT_DECL(HBRUSH, SH_GDI, CREATEDIBPATTERNBRUSHPT,	\
				(CONST VOID *lpPackedDIB,							\
				UINT iUsage))
#define CreatePenIndirect											\
			IMPLICIT_DECL(HPEN, SH_GDI, CREATEPENINDIRECT,			\
				(CONST LOGPEN *lplgpn))
#define CreateSolidBrush											\
			IMPLICIT_DECL(HBRUSH, SH_GDI, CREATESOLIDBRUSH,			\
				(COLORREF crColor))
#define DrawEdge													\
			IMPLICIT_DECL(BOOL, SH_GDI, DRAWEDGE,					\
				(HDC hdc,											\
				LPRECT lprc,										\
				UINT uEdgeType,										\
				UINT uFlags))
#define DrawFocusRect												\
			IMPLICIT_DECL(BOOL, SH_GDI, DRAWFOCUSRECT,				\
				(HDC hdc,											\
				CONST RECT *lprc))
#define Ellipse														\
			IMPLICIT_DECL(BOOL, SH_GDI, ELLIPSE,					\
				(HDC hdc,											\
				int nLeftRect,										\
				int nTopRect,										\
				int nRightRect,										\
				int nBottomRect))
#define FillRect													\
			IMPLICIT_DECL(int, SH_GDI, FILLRECT,					\
				(HDC hdc,											\
				CONST RECT *lprc,									\
				HBRUSH hbr))
#define FillRgn  													\
			IMPLICIT_DECL(BOOL, SH_GDI, FILLRGN,					\
				(HDC   hdc,											\
				HRGN   hrgn,									    \
				HBRUSH hbr))
#define GetPixel													\
			IMPLICIT_DECL(COLORREF, SH_GDI, GETPIXEL,				\
				(HDC hdc,											\
				int nXPos,											\
				int nYPos))
#define GetSysColorBrush											\
			IMPLICIT_DECL(HBRUSH, SH_GDI, GETSYSCOLORBRUSH,			\
				(int nIndex))
#define Polygon														\
			IMPLICIT_DECL(BOOL, SH_GDI, POLYGON,					\
				(HDC hdc,											\
				CONST POINT *lpPoints,								\
				int nCount))
#define Polyline													\
			IMPLICIT_DECL(BOOL, SH_GDI, POLYLINE,					\
				(HDC hdc,											\
				CONST POINT *lppt,									\
				int cPoints))
#define Rectangle													\
			IMPLICIT_DECL(BOOL, SH_GDI, RECTANGLE,					\
				(HDC hdc,											\
				int nLeftRect,										\
				int nTopRect,										\
				int nRightRect,										\
				int nBottomRect))
#define RoundRect													\
			IMPLICIT_DECL(BOOL, SH_GDI, ROUNDRECT,					\
				(HDC hdc,											\
				int nLeftRect,										\
				int nTopRect,										\
				int nRightRect,										\
				int nBottomRect,									\
				int nWidth,											\
				int nHeight))
#define SetBrushOrgEx												\
			IMPLICIT_DECL(BOOL, SH_GDI, SETBRUSHORGEX,				\
				(HDC hdc,											\
				int nXOrg,											\
				int nYOrg,											\
				LPPOINT lppt))
#define SetPixel													\
			IMPLICIT_DECL(COLORREF, SH_GDI, SETPIXEL,				\
				(HDC hdc,											\
				int X,												\
				int Y,												\
				COLORREF crColor))
// @CESYSGEN ENDIF


// @CESYSGEN IF GWES_PGDI || GWES_MGBITMAP
#define CreateBitmap                                                \
            IMPLICIT_DECL(HBITMAP, SH_GDI, CREATEBITMAP,            \
                (int nWidth,                                        \
                int nHeight,                                        \
                UINT cPlanes,                                       \
                UINT cBitsPerPel,                                   \
                CONST VOID *lpvBits))
#define CreateCompatibleBitmap                                      \
            IMPLICIT_DECL(HBITMAP, SH_GDI, CREATECOMPATIBLEBITMAP,  \
                (HDC hdc,                                           \
                int nWidth,                                         \
                int nHeight))
// @CESYSGEN ENDIF


// @CESYSGEN IF GWES_PGDI || GWES_MGRGN
#define CombineRgn													\
			IMPLICIT_DECL(int, SH_GDI, COMBINERGN,					\
				(HRGN hrgnDest,										\
				HRGN hrgnSrc1,										\
				HRGN hrgnSrc2,										\
				int fnCombineMode))
#define CreateRectRgnIndirect										\
			IMPLICIT_DECL(HRGN, SH_GDI, CREATERECTRGNINDIRECT,		\
				(CONST RECT *lprc))
#define ExcludeClipRect												\
			IMPLICIT_DECL(int, SH_GDI,EXCLUDECLIPRECT,				\
				(HDC hdc,											\
				int nLeftRect,										\
				int nTopRect,										\
				int nRightRect,										\
				int nBottomRect))
#define GetClipBox													\
			IMPLICIT_DECL(int, SH_GDI, GETCLIPBOX,					\
				(HDC hdc,											\
				LPRECT lprc))
#define GetClipRgn													\
			IMPLICIT_DECL(int, SH_GDI, GETCLIPRGN,					\
				(HDC hdc,											\
				HRGN hrgn))
#define GetRegionData												\
			IMPLICIT_DECL(DWORD, SH_GDI, GETREGIONDATA,				\
				(HRGN hRgn,											\
				DWORD dwCount,										\
				LPRGNDATA lpRgnData))
#define GetRgnBox													\
			IMPLICIT_DECL(int, SH_GDI, GETRGNBOX,					\
				(HRGN hrgn,											\
				RECT *lprc))
#define IntersectClipRect							  				\
			IMPLICIT_DECL(int, SH_GDI, INTERSECTCLIPRECT,			\
				(HDC hdc,											\
				int nLeftRect,										\
				int nTopRect,										\
				int nRightRect,										\
				int nBottomRect))
#define OffsetRgn													\
			IMPLICIT_DECL(int, SH_GDI, OFFSETRGN,					\
				(HRGN hrgn,											\
				int nXOffset,										\
				int nYOffset))
#define PtInRegion													\
			IMPLICIT_DECL(BOOL, SH_GDI, PTINREGION,					\
				(HRGN hrgn,											\
				int X,												\
				int Y))
#define RectInRegion												\
			IMPLICIT_DECL(BOOL, SH_GDI, RECTINREGION,				\
				(HRGN hrgn,											\
				CONST RECT *lprc))
#define SelectClipRgn												\
			IMPLICIT_DECL(int, SH_GDI, SELECTCLIPRGN,				\
				(HDC hdc,											\
				HRGN hrgn))
// @CESYSGEN ENDIF


// @CESYSGEN IF GWES_MGRGN
#define CreateRectRgn										        \
			IMPLICIT_DECL(HRGN, SH_GDI, CREATERECTRGN,		        \
				(int  nLeftRect,                                    \
                 int  nTopRect,                                     \
                 int  nRightRect,                                   \
                 int  nBottomRect))
#define RectVisible                                                 \
			IMPLICIT_DECL(BOOL, SH_GDI, RECTVISIBLE,				\
				(HDC hdc,											\
				CONST RECT *lprc))
#define SetRectRgn  												\
			IMPLICIT_DECL(BOOL, SH_GDI, SETRECTRGN, 				\
				(HRGN hrgn,											\
				int  nLeftRect,                                     \
                int  nTopRect,                                      \
                int  nRightRect,                                    \
                int  nBottomRect))
// @CESYSGEN ENDIF


// @CESYSGEN IF GWES_MGBASE
#define SetViewportOrgEx											\
			IMPLICIT_DECL(BOOL, SH_GDI, SETVIEWPORTORGEX,		    \
				(HDC hdc,											\
				int nXOrg,											\
				int nYOrg,											\
				LPPOINT lppt))
// @CESYSGEN ENDIF


// @CESYSGEN IF GWES_PGDI || GWES_MGBLT
#define BitBlt														\
			IMPLICIT_DECL(BOOL, SH_GDI, BITBLT,						\
				(HDC hdcDest,										\
				int nXDest,											\
				int nYDest,											\
				int nWidth,											\
				int nHeight,										\
	   			HDC hdcSrc,											\
	   			int nXSrc,											\
	   			int nYSrc,											\
	   			DWORD dwRop))
// @CESYSGEN ENDIF


// @CESYSGEN IF GWES_PGDI || GWES_MGBLT2
#define MaskBlt														\
			IMPLICIT_DECL(BOOL, SH_GDI, MASKBLT,					\
				(HDC hdcDest,										\
				int nXDest,											\
				int nYDest,											\
				int nWidth,											\
				int nHeight,										\
				HDC hdcSrc,											\
				int nXSrc,											\
				int nYSrc,											\
				HBITMAP hbmMask,									\
				int xMask,											\
				int yMask,											\
				DWORD dwRop))
#define PatBlt														\
			IMPLICIT_DECL(BOOL, SH_GDI, PATBLT,						\
				(HDC hdcDest,										\
				int nXLeft,											\
				int nYLeft,											\
				int nWidth,											\
				int nHeight,										\
	   			DWORD dwRop))
#define StretchBlt													\
			IMPLICIT_DECL(BOOL, SH_GDI, STRETCHBLT,					\
				(HDC hdcDest,										\
				int nXOriginDest,									\
				int nYOriginDest,									\
				int nWidthDest,										\
				int nHeightDest,									\
				HDC hdcSrc,											\
				int nXOriginSrc,									\
				int nYOriginSrc,									\
				int nWidthSrc,										\
				int nHeightSrc,										\
				DWORD dwRop))
// @CESYSGEN ENDIF


// @CESYSGEN IF GWES_MGBLT2
#define TransparentImage											\
			IMPLICIT_DECL(BOOL, SH_GDI, TRANSPARENTIMAGE,   		\
				(HDC hdcDest,										\
				int nXOriginDest,									\
				int nYOriginDest,									\
				int nWidthDest,										\
				int nHeightDest,									\
				HANDLE hSrc,										\
				int nXOriginSrc,									\
				int nYOriginSrc,									\
				int nWidthSrc,										\
				int nHeightSrc,										\
				COLORREF TransparentColor))
// @CESYSGEN ENDIF


// @CESYSGEN IF GWES_PGDI || GWES_MGTT || GWES_MGRAST
#define AddFontResourceW											\
			IMPLICIT_DECL(int, SH_GDI, ADDFONTRESOURCEW,			\
				(LPCWSTR lpszFilename))
#define CreateFontIndirectW											\
			IMPLICIT_DECL(HFONT, SH_GDI, CREATEFONTINDIRECT,		\
				(CONST LOGFONT *lplf))
#define ExtTextOutW													\
			IMPLICIT_DECL(BOOL, SH_GDI, EXTTEXTOUTW,				\
				(HDC hdc,											\
				int X,												\
				int Y,												\
				UINT fuOptions,										\
				CONST RECT *lprc,									\
				LPCWSTR lpszString,									\
				UINT cbCount,										\
				CONST INT *lpDx))
#define GetTextExtentExPointW										\
			IMPLICIT_DECL(BOOL, SH_GDI, GETTEXTEXTENTEXPOINTW,		\
				(HDC hdc,											\
				LPCWSTR lpszStr,									\
				int cchString,										\
				int nMaxExtent,										\
				LPINT lpnFit,										\
				LPINT alpDx,										\
				LPSIZE lpSize))
#define GetTextMetricsW												\
			IMPLICIT_DECL(BOOL, SH_GDI, GETTEXTMETRICS,				\
				(HDC hdc,											\
				LPTEXTMETRIC lptm))
#define RemoveFontResourceW											\
			IMPLICIT_DECL(BOOL, SH_GDI, REMOVEFONTRESOURCEW,		\
				(LPCWSTR lpFileName))
// @CESYSGEN ENDIF


// @CESYSGEN IF GWES_PGDI || GWES_MGDRWTXT
#define DrawTextW													\
			IMPLICIT_DECL(int, SH_GDI, DRAWTEXTW,					\
				(HDC hdc,											\
				LPCWSTR lpszStr,									\
				int cchStr,											\
				RECT *lprc,											\
				UINT wFormat))
// @CESYSGEN ENDIF


// @CESYSGEN IF GWES_PGDI || GWES_MGTT || GWES_MGRAST2
#define EnumFontFamiliesW                                           \
            IMPLICIT_DECL(int, SH_GDI, ENUMFONTFAMILIESW,           \
                (HDC hdc,                                           \
                LPCWSTR lpszFamily,                                 \
                FONTENUMPROC lpEnumFontFamProc,                     \
                LPARAM lParam))
#define EnumFontsW                                                  \
            IMPLICIT_DECL(int, SH_GDI, ENUMFONTFAMILIESW,           \
                (HDC hdc,                                           \
                LPCWSTR lpszFaceName,                               \
                FONTENUMPROC lpEnumFontProc,                        \
                LPARAM lParam))
#define GetTextFaceW												\
			IMPLICIT_DECL(int, SH_GDI, GETTEXTFACEW,				\
				(HDC hdc,											\
				int nCount,											\
				LPWSTR lpFaceName))
// @CESYSGEN ENDIF


// @CESYSGEN IF GWES_MGTCI
#define TranslateCharsetInfo                						\
			IMPLICIT_DECL(BOOL, SH_GDI, TRANSLATECHARSETINFO,		\
				(DWORD *lpSrc,										\
				LPCHARSETINFO lpCs,									\
				DWORD dwFlags))
// @CESYSGEN ENDIF


// @CESYSGEN IF GWES_PGDI || GWES_MGDC
#define CreateDCW											        \
			IMPLICIT_DECL(HDC, SH_GDI, CREATEDC,			        \
				(LPCWSTR        lpszDriver,                         \
                LPCWSTR         lpszDevice,                         \
                LPCWSTR         lpszOutput,                         \
                CONST DEVMODEW *lpInitData))
#define ExtEscape													\
			IMPLICIT_DECL(int, SH_GDI, EXTESCAPE,					\
				(HDC    hdc,                                        \
                int    iEscape,                                     \
                int    cjInput,                                     \
                LPCSTR lpInData,                                    \
                int    cjOutput,                                    \
                LPSTR  lpOutData                                    \
                ))
#define RestoreDC													\
			IMPLICIT_DECL(BOOL, SH_GDI, RESTOREDC,					\
				(HDC hdc,											\
				int nSavedDC))
#define SaveDC														\
			IMPLICIT_DECL(int, SH_GDI, SAVEDC,						\
				(HDC hdc))
// @CESYSGEN ENDIF


// @CESYSGEN IF GWES_MGPAL
#define CreatePalette   									        \
			IMPLICIT_DECL(HPALETTE, SH_GDI, CREATEPALETTE,          \
				(CONST LOGPALETTE *pLogPal))
#define GetNearestPaletteIndex									    \
			IMPLICIT_DECL(UINT, SH_GDI, GETNEARESTPALETTEINDEX,		\
				(HPALETTE hpal,										\
				COLORREF crColor))
#define GetPaletteEntries   									    \
			IMPLICIT_DECL(UINT, SH_GDI, GETPALETTEENTRIES,          \
				(HPALETTE hpal,									    \
                UINT iStart,                                        \
                UINT nEntries,                                      \
				LPPALETTEENTRY pPe))
#define GetSystemPaletteEntries									    \
			IMPLICIT_DECL(UINT, SH_GDI, GETSYSTEMPALETTEENTRIES,    \
				(HDC hc,										    \
                UINT iStart,                                        \
                UINT nEntries,                                      \
				LPPALETTEENTRY pPe))
#define RealizePalette   									        \
			IMPLICIT_DECL(UINT, SH_GDI, REALIZEPALETTE,             \
				(HDC hdc))
#define SelectPalette   									        \
			IMPLICIT_DECL(HPALETTE, SH_GDI, SELECTPALETTE,          \
				(HDC hdc,   									    \
                HPALETTE hPal,                                      \
				BOOL bForceBackground))
#define SetPaletteEntries   									    \
			IMPLICIT_DECL(UINT, SH_GDI, SETPALETTEENTRIES,          \
				(HPALETTE hpal,									    \
                UINT iStart,                                        \
                UINT nEntries,                                      \
				CONST LPPALETTEENTRY pPe))
// @CESYSGEN ENDIF


// @CESYSGEN IF GWES_PGDI || GWES_MGPAL
#define GetNearestColor												\
			IMPLICIT_DECL(COLORREF, SH_GDI, GETNEARESTCOLOR,		\
				(HDC hdc,											\
				COLORREF crColor))
// @CESYSGEN ENDIF


// @CESYSGEN IF GWES_MGPRINT
#ifndef NOMETAFILE
#define CloseEnhMetaFile                                            \
            IMPLICIT_DECL(HENHMETAFILE, SH_GDI, CLOSEENHMETAFILE,   \
                (HDC hdc))
#define CreateEnhMetaFileW                                          \
            IMPLICIT_DECL(HDC, SH_GDI, CREATEENHMETAFILEW,          \
                (HDC hdc,                                           \
                LPCWSTR lpFilename,                                 \
                CONST RECT *lpRect,                                 \
                LPCWSTR lpDescription))
#define DeleteEnhMetaFile                                           \
            IMPLICIT_DECL(BOOL, SH_GDI, DELETEENHMETAFILE,          \
                (HENHMETAFILE hMetaFile))
#define PlayEnhMetaFile                                             \
            IMPLICIT_DECL(BOOL, SH_GDI, PLAYENHMETAFILE,            \
                (HDC hdc,                                           \
                HENHMETAFILE hMetaFile,                             \
                CONST RECT *lpRect))
#endif  /* NOMETAFILE */

#define AbortDoc											        \
			IMPLICIT_DECL(int, SH_GDI, ABORTDOC,			        \
				(HDC hdc))
#define EndDoc                                                      \
            IMPLICIT_DECL(int, SH_GDI, ENDDOC,                      \
                (HDC hdc))
#define EndPage                                                      \
            IMPLICIT_DECL(int, SH_GDI, ENDPAGE,                      \
                (HDC hdc))
#define SetAbortProc												\
			IMPLICIT_DECL(int, SH_GDI, SETABORTPROC,				\
				(HDC hdc,											\
				ABORTPROC abortproc))
#define StartDocW												    \
			IMPLICIT_DECL(int, SH_GDI, STARTDOC,			        \
				(HDC hdc,											\
				CONST DOCINFOW *pdocinfo))
#define StartPage												    \
			IMPLICIT_DECL(int, SH_GDI, STARTPAGE,			        \
				(HDC hdc))
// @CESYSGEN ENDIF

// @CESYSGEN IF GWES_MGDX
#define CreateBitmapFromPointer                                     \
        	IMPLICIT_DECL(HBITMAP, SH_GDI, CREATEBITMAPFROMPOINTER, \
                (CONST BITMAPINFO *pbmi,                            \
                INT32		      iStride,                          \
                PVOID             pvBits))
// @CESYSGEN ENDIF

// OAK defines
#define CeRemoveFontResource \
	IMPLICIT_DECL(BOOL, SH_GDI, CEREMOVEFONTRESOURCE,(CEOID oid))

#define EnableEUDC \
	IMPLICIT_DECL(BOOL, SH_GDI, ENABLEEUDC,(BOOL bEnable))


// @CESYSGEN IF GWES_MGWINMGR
#define SetObjectOwner \
	IMPLICIT_DECL(BOOL, SH_GDI, SETOBJECTOWNER,                     \
                (HGDIOBJ hgdiobj, HPROCESS hprocess))
// @CESYSGEN ENDIF


#endif /* _WINGDIP_H_ */

#ifdef __cplusplus
}
#endif

