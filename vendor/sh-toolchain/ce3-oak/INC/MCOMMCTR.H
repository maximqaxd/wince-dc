/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

Module Name:

mcommctr.h

Abstract:

Notes:


--*/

#ifndef __MCOMMCTR_H__
#define __MCOMMCTR_H__

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

#ifndef WINCEMACRO
#error WINCEMACRO not defined __FILE__
#endif

#ifdef USER_CALL

#define ImageList_Create USER_CALL (HIMAGELIST, ImageListCreate, ( \
    int cx, int cy, UINT flags, int cInitial, int cGrow))

#define ImageList_Destroy USER_CALL (BOOL, ImageListDestroy, ( \
    HIMAGELIST himl))

#define ImageList_GetImageCount USER_CALL (int, ImageListGetImageCount, ( \
    HIMAGELIST himl))

#define ImageList_Add USER_CALL (int, ImageListAdd, ( \
    HIMAGELIST himl, HBITMAP hbmImage, HBITMAP hbmMask))

#define ImageList_ReplaceIcon USER_CALL (int, ImageListReplaceIcon, ( \
    HIMAGELIST himl, int i, HICON hicon))

#define ImageList_SetBkColor USER_CALL (COLORREF, ImageListSetBkColor, ( \
    HIMAGELIST himl, COLORREF clrBk))

#define ImageList_GetBkColor USER_CALL (COLORREF, ImageListGetBkColor, ( \
    HIMAGELIST himl))

#define ImageList_SetOverlayImage USER_CALL (BOOL, ImageListSetOverlayImage, ( \
    HIMAGELIST himl, int iImage, int iOverlay))

#define ImageList_Draw USER_CALL (BOOL, ImageListDraw, ( \
    HIMAGELIST himl, int i, HDC hdcDst, int x, int y, UINT fStyle))

#define ImageList_Replace USER_CALL (BOOL, ImageListReplace, ( \
    HIMAGELIST himl, int i, HBITMAP hbmImage, HBITMAP hbmMask))

#define ImageList_AddMasked USER_CALL (int, ImageListAddMasked, ( \
    HIMAGELIST himl, HBITMAP hbmImage, COLORREF crMask))

#define ImageList_DrawEx USER_CALL (BOOL, ImageListDrawEx, ( \
    HIMAGELIST himl, int i, HDC hdcDst, int x, int y, int dx, int dy, \
    COLORREF rgbBk, COLORREF rgbFg, UINT fStyle))

#define ImageList_Remove USER_CALL (BOOL, ImageListRemove, ( \
    HIMAGELIST himl, int i))

#define ImageList_GetIcon USER_CALL (HICON, ImageListGetIcon, ( \
    HIMAGELIST himl, int i, UINT flags))

#define ImageList_LoadImage USER_CALL(HIMAGELIST, ImageListLoadImage, ( \
    HINSTANCE hinst, LPCWSTR lpbmp, int cx, int cGrow, COLORREF crMask, \
    UINT uType, UINT uFlags))

#define ImageList_BeginDrag USER_CALL (BOOL, ImageListBeginDrag, ( \
    HIMAGELIST himlTrack, int iTrack, int dxHotspot, int dyHotSpot))

#define ImageList_EndDrag USER_CALL (void, ImageListEndDrag, ( \
    void ))

#define ImageList_DragEnter USER_CALL (BOOL, ImageListDragEnter, ( \
    HWND hwndLock, int x, int y))

#define ImageList_DragLeave USER_CALL (BOOL, ImageListDragLeave, ( \
    HWND hwndLock))

#define ImageList_DragMove USER_CALL (BOOL, ImageListDragMove, ( \
    int x, int y))

#define ImageList_SetDragCursorImage USER_CALL (BOOL, ImageListSetDragCursorImage, ( \
    HIMAGELIST himl, int iDrag, int dxHotspot, int dyHotspot))

#define ImageList_DragShowNolock USER_CALL (BOOL, ImageListDragShowNolock, ( \
    BOOL fShow))

#define ImageList_GetDragImage USER_CALL (HIMAGELIST, ImageListGetDragImage, ( \
    POINT FAR *ppt, POINT FAR *pptHotspot))

#define ImageList_GetIconSize USER_CALL (BOOL, ImageListGetIconSize, ( \
    HIMAGELIST himl, int FAR *cx, int FAR *cy))

#define ImageList_SetIconSize USER_CALL (BOOL, ImageListSetIconSize, ( \
    HIMAGELIST himl, int cx, int cy))

#define ImageList_GetImageInfo USER_CALL (BOOL, ImageListGetImageInfo, ( \
    HIMAGELIST himl, int i, IMAGEINFO FAR *pImageInfo))

#define ImageList_Merge USER_CALL (HIMAGELIST, ImageListMerge, ( \
    HIMAGELIST himl1, int i1, HIMAGELIST himl2, int i2, int dx, int dy))

#define ImageList_CopyDitherImage USER_CALL (void, ImageListCopyDitherImage, ( \
    HIMAGELIST himlDest, WORD iDst, int xDst, int yDst, HIMAGELIST himlSrc, \
    int iSrc, UINT fStyle))

#define ImageList_DrawIndirect USER_CALL (BOOL, ImageListDrawIndirect, ( \
    IMAGELISTDRAWPARAMS *pimldp))

#define ImageList_Copy USER_CALL (BOOL, ImageListCopy, (	\
	HIMAGELIST himlDst,		\
	int iDst,				\
	HIMAGELIST himlSrc,		\
	int iSrc,				\
	UINT uFlags ))

#define ImageList_Duplicate USER_CALL (HIMAGELIST, ImageListDuplicate, (	\
	HIMAGELIST himl ))

#define ImageList_SetImageCount USER_CALL (BOOL, ImageListSetImageCount, (	\
	HIMAGELIST himl,	\
	UINT uNewCount ))

#elif defined(COREDLL)

HIMAGELIST xxx_ImageList_Create (int cx, int cy, UINT flags, int cInitial, int cGrow);
#define ImageList_Create xxx_ImageList_Create

BOOL xxx_ImageList_Destroy (HIMAGELIST himl);
#define ImageList_Destroy xxx_ImageList_Destroy

int xxx_ImageList_GetImageCount (HIMAGELIST himl);
#define ImageList_GetImageCount xxx_ImageList_GetImageCount

int xxx_ImageList_Add (HIMAGELIST himl, HBITMAP hbmImage, HBITMAP hbmMask);
#define ImageList_Add xxx_ImageList_Add

int xxx_ImageList_ReplaceIcon (HIMAGELIST himl, int i, HICON hicon);
#define ImageList_ReplaceIcon xxx_ImageList_ReplaceIcon

COLORREF xxx_ImageList_SetBkColor (HIMAGELIST himl, COLORREF clrBk);
#define ImageList_SetBkColor xxx_ImageList_SetBkColor

COLORREF xxx_ImageList_GetBkColor (HIMAGELIST himl);
#define ImageList_GetBkColor xxx_ImageList_GetBkColor

BOOL xxx_ImageList_SetOverlayImage (HIMAGELIST himl, int iImage, int iOverlay);
#define ImageList_SetOverlayImage xxx_ImageList_SetOverlayImage

BOOL xxx_ImageLis_tDraw (HIMAGELIST himl, int i, HDC hdcDst, int x, int y, UINT fStyle);
#define ImageList_Draw xxx_ImageList_Draw

BOOL xxx_ImageList_Replace (HIMAGELIST himl, int i, HBITMAP hbmImage, HBITMAP hbmMask);
#define ImageList_Replace xxx_ImageList_Replace

int xxx_ImageListAdd_Masked (HIMAGELIST himl, HBITMAP hbmImage, COLORREF crMask);
#define ImageListAdd_Masked xxx_ImageList_AddMasked

BOOL xxx_ImageList_DrawEx (HIMAGELIST himl, int i, HDC hdcDst, int x, int y, int dx, int dy, COLORREF rgbBk, COLORREF rgbFg, UINT fStyle);
#define ImageList_DrawEx xxx_ImageList_DrawEx

BOOL xxx_ImageList_Remove (HIMAGELIST himl, int i);
#define ImageList_Remove xxx_ImageList_Remove

HICON xxx_ImageList_GetIcon (HIMAGELIST himl, int i, UINT flags);
#define ImageList_GetIcon xxx_ImageList_GetIcon

HIMAGELIST xxx_ImageList_LoadImage (HINSTANCE hinst, LPCWSTR lpbmp, int cx, int cGrow, COLORREF crMask, UINT uType, UINT uFlags);
#define ImageList_LoadImage xxx_ImageList_LoadImage

BOOL xxx_ImageList_BeginDrag (HIMAGELIST himlTrack, int iTrack, int dxHotspot, int dyHotSpot);
#define ImageList_BeginDrag xxx_ImageList_BeginDrag

void xxx_ImageList_EndDrag (void );
#define ImageList_EndDrag xxx_ImageList_EndDrag

BOOL xxx_ImageList_DragEnter (HWND hwndLock, int x, int y);
#define ImageList_DragEnter xxx_ImageList_DragEnter

BOOL xxx_ImageList_DragLeave (HWND hwndLock);
#define ImageList_DragLeave xxx_ImageList_DragLeave

BOOL xxx_ImageList_DragMove (int x, int y);
#define ImageList_DragMove xxx_ImageList_DragMove

BOOL xxx_ImageList_SetDragCursorImage (HIMAGELIST himl, int iDrag, int dxHotspot, int dyHotspot);
#define ImageList_SetDragCursorImage xxx_ImageList_SetDragCursorImage

BOOL xxx_ImageList_DragShowNolock (BOOL fShow);
#define ImageList_DragShowNolock xxx_ImageList_DragShowNolock

HIMAGELIST xxx_ImageList_GetDragImage (POINT FAR *ppt, POINT FAR *pptHotspot);
#define ImageList_GetDragImage xxx_ImageList_GetDragImage

BOOL xxx_ImageList_GetIconSize (HIMAGELIST himl, int FAR *cx, int FAR *cy);
#define ImageList_GetIconSize xxx_ImageList_GetIconSize

BOOL xxx_ImageList_SetIconSize (HIMAGELIST himl, int cx, int cy);
#define ImageList_SetIconSize xxx_ImageList_SetIconSize

BOOL xxx_ImageList_GetImageInfo (HIMAGELIST himl, int i, IMAGEINFO FAR *pImageInfo);
#define ImageList_GetImageInfo xxx_ImageList_GetImageInfo

HIMAGELIST xxx_ImageList_Merge (HIMAGELIST himl1, int i1, HIMAGELIST himl2, int i2, int dx, int dy);
#define ImageList_Merge xxx_ImageList_Merge

void xxx_ImageList_CopyDitherImage (HIMAGELIST himlDest, WORD iDst, int xDst, int yDst, HIMAGELIST himlSrc, int iSrc, UINT fStyle);
#define ImageList_CopyDitherImage xxx_ImageList_CopyDitherImage

BOOL xxx_ImageList_DrawIndirect (IMAGELISTDRAWPARAMS *pimldp);
#define ImageList_DrawIndirect xxx_ImageList_DrawIndirect

BOOL xxx_ImageList_Copy (HIMAGELIST himlDst, int iDst, HIMAGELIST himlSrc, int iSrc, UINT uFlags );
#define ImageList_Copy xxx_ImageList_Copy

HIMAGELIST xxx_ImageList_Duplicate (HIMAGELIST himl );
#define ImageList_Duplicate xxx_ImageList_Duplicate

BOOL xxx_ImageList_SetImageCount (HIMAGELIST himl, UINT uNewCount );
#define ImageList_SetImageCount xxx_ImageList_SetImageCount

#endif

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif // ! __MCOMMCTR_H__
