/*

Copyright (c) 1994-2000 Microsoft Corporation.  All rights reserved.

*/

#define DIALOGCLASSNAME TEXT("Dialog")

/* Dialog structure (dlg) */
//IMPORTANT: sizeof this structure must be == DLGWINDOWEXTRA.
typedef struct
{
    LPARAM  result;          // contains result code for a processed message
    DLGPROC lpfnDlg;         // dialog procedure
    LONG    lUser;           // user space
    HANDLE  hProc;           // Process owning dlgproc
    HWND    hwndFocus;       // control with current focus
    LONG	lresult;		 // final return value for a modal dialog
    WORD    wDefID;          // Original DefID for the dialog
    WORD    fEnd       : 1;
    HFONT   hUserFont;
} DLG1, *PDLG1;

#define DLGEX_SIGNATURE 0xFFFF
typedef struct tagDLGNEWTEMPLATE
{
    WORD    wDlgVer;
    WORD    wSignature;
    DWORD   dwHelpID;
    DWORD   dwExStyle;
    DWORD   dwStyle;
    BYTE    cDlgItems; // NOTE: if 32, this comes in as a WORD but is converted by cvtres32.asm in win32\win32c
    short   x;
    short   y;
    short   cx;
    short   cy;
}   DLGNEWTEMPLATE, FAR *LPDLGNEWTEMPLATE;
    /* MenuName
    ** ClassName
    ** Title
    ** if DS_SETFONT:  pointsize, weight, bItalic, bCharSet, FontName
    */

typedef struct tagNEWDLGITEMTEMPLATE
{
    DWORD   dwExStyle;
    DWORD   dwStyle;
    short   x;
    short   y;
    short   cx;
    short   cy;
    DWORD   wID;
} NEWDLGITEMTEMPLATE, FAR *LPNEWDLGITEMTEMPLATE;
    /* ClassName
    ** Title
    ** length word
    */

/*
 * Special case string token codes.  These must be the same as in the resource
 * compiler's RC.H file.
 */
/*
 * NOTE: Order is assumed and much be this way for applications to be
 * compatable with windows 2.0
 */
#define CODEBIT         0x80
#define BUTTONCODE      0x80
#define EDITCODE        0x81
#define STATICCODE      0x82
#define LISTBOXCODE     0x83
#define SCROLLBARCODE   0x84
#define COMBOBOXCODE    0x85

void    CheckDefPushButton(HWND, HWND, HWND);
HWND    GetParentDialog(HWND hwndDlg);
HWND    NextControl(HWND hwndParent, HWND hwnd, UINT uSkip);
HWND    PrevControl(HWND hwndParent, HWND hwnd, UINT uSkip);
HWND    FindDlgItem(HWND hwndParent, int id);
void    DlgSetFocus(HWND hwnd);
HWND 	GotoNextMnem(HWND hwndDlg, HWND hwnd, WCHAR ch);

#define IsDefPushButton(hwnd)   \
    (SendMessage(hwnd,WM_GETDLGCODE,0,0L)&DLGC_DEFPUSHBUTTON)
#define IsUndefPushButton(hwnd)      \
    (SendMessage(hwnd,WM_GETDLGCODE,0,0L)&DLGC_UNDEFPUSHBUTTON)
#define IsPushButton(hwnd)      \
    (SendMessage(hwnd,WM_GETDLGCODE,0,0L)&(DLGC_UNDEFPUSHBUTTON|DLGC_DEFPUSHBUTTON))
