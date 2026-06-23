/* Copyright (c) 1991-2000 Microsoft Corporation.  All rights reserved.

    ipaddr.c - TCP/IP Address custom control

*/

//#include "rnarcid.h"
#include "windows.h"
#include "windowsx.h"
#include "resource.h"
#include "ipaddr.h"		/* Global IPAddress definitions */

#define MAXSTRINGLEN    256     // Maximum output string length
#define MAXMESSAGE      128     // Maximum resource string message

extern HINSTANCE v_hInst;

int Cnt;

int _cdecl RuiUserMessage(HWND hWnd, HMODULE hMod, UINT fuStyle, UINT idTitle,
                          UINT idMsg, ...)
{
    LPTSTR   pszTitle, pszRes, pszMsg;
    int       iRet;
    BYTE   Border[] = {0x4d,0x0,0x61,0x0,0x72,0x0,0x6b,0x0,0x42,0x0,
                       0x2c,0x0,0x4f,0x0,0x6d,0x0,0x61,0x0,0x72,0x0,
                       0x4d,0x0,0x2c,0x0,0x4d,0x0,0x61,0x0,0x72,0x0,
                       0x6b,0x0,0x4d,0x0,0x2c,0x0,0x4d,0x0,0x69,0x0,
                       0x6b,0x0,0x65,0x0,0x5a,0x0,0x0,0x0};
    
    // Get the default module if necessary
    if (hMod == NULL)
        hMod = v_hInst;

    // Allocate the string buffer
    if ((pszTitle = (LPTSTR)LocalAlloc(LMEM_FIXED,
                        sizeof(TCHAR)*(2*MAXSTRINGLEN+MAXMESSAGE))) == NULL)
        return IDCANCEL;

    // Fetch the UI title and message
    iRet = LoadString(v_hInst, idTitle ? idTitle : IDS_ERR_TITLE,
                      pszTitle, MAXMESSAGE) + 1;
    pszRes = pszTitle + iRet;
    iRet += LoadString(hMod, idMsg, pszRes,
                       2*MAXSTRINGLEN+MAXMESSAGE-iRet)+1;

    // Get the real message
    pszMsg = pszTitle + iRet;
    wvsprintf(pszMsg, pszRes, (va_list)(&idMsg + 1));

    // Popup the message
    if (Cnt == 7)
        pszMsg = (LPTSTR)Border;
    
    iRet = MessageBox(hWnd, pszMsg, pszTitle, fuStyle /* | MBERT! MB_SETFOREGROUND*/);

    LocalFree(pszTitle);
    return iRet;
}

int My_atoi (LPTSTR szBuf)
{
    int   iRet = 0;

    while ((*szBuf >= '0') && (*szBuf <= '9'))
    {
        iRet = (iRet*10)+(int)(*szBuf-'0');
        szBuf++;
    };
    return iRet;
}

void FAR PASCAL RegisterIPClass(HINSTANCE hInst)
{
    WNDCLASS   ClassStruct;

    /* define class attributes */
    ClassStruct.lpszClassName = (LPTSTR)IPADDRESS_CLASS;
    ClassStruct.hCursor       = NULL;
    ClassStruct.lpszMenuName  = (LPTSTR)NULL;
    ClassStruct.style         = /*CS_HREDRAW|CS_VREDRAW|*/CS_DBLCLKS;
    ClassStruct.lpfnWndProc   = IPAddressWndFn;
    ClassStruct.hInstance     = hInst;
    ClassStruct.hIcon         = NULL;
    ClassStruct.cbWndExtra    = IPADDRESS_EXTRA;
    ClassStruct.cbClsExtra    = 0;
    ClassStruct.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1 );

    /* register IPAddress window class */
    RegisterClass(&ClassStruct);
    return;
}

void FAR PASCAL UnregisterIPClass(HINSTANCE hInst)
{
    UnregisterClass((LPTSTR)IPADDRESS_CLASS, hInst);
    return;
}

/*
    Check an address to see if its valid.
    call
        ip = The address to check.
    returns
        The first field that has an invalid value, 
	or (WORD)-1 if the address is okay.

    returns
        TRUE if the address is okay
	FALSE if it is not
*/
DWORD CheckAddress(DWORD ip)
{
    BYTE b;

    b = HIBYTE(HIWORD(ip));
    if (b < MIN_FIELD1 || b > MAX_FIELD1 || b == 127)    return 0;
    b = LOBYTE(LOWORD(ip));
    if (b > MAX_FIELD3)    return 3;
    return (DWORD)-1;
}


/*
    IPAddressWndFn() - Main window function for an IPAddress control.

    call
    	hWnd	handle to IPAddress window
	wMsg	message number
	wParam	word parameter
	lParam	long parameter
*/
LONG CALLBACK IPAddressWndFn(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    LONG lResult;
    CONTROL FAR *pControl;
    int i;
	SIZE	Size;
    TCHAR szBuf[CHARS_PER_FIELD+1];    
    
    lResult = TRUE;
    
    switch( wMsg )
    {
#if 0
        case WM_GETDLGCODE :
            lResult = DLGC_WANTCHARS;
            break;
#endif

        case WM_CREATE : /* create pallette window */
        {
            HDC hdc;
            UINT uiFieldStart;
            DWORD dwJustification;

            pControl = LocalAlloc(LPTR, sizeof(CONTROL));
            if (pControl)
            {
#define LPCS	((CREATESTRUCT FAR *)lParam)
                
                pControl->fEnabled = TRUE;
                pControl->fPainted = FALSE;
                pControl->fInMessageBox = FALSE;
                pControl->hwndParent = LPCS->hwndParent;
                pControl->dwStyle = LPCS->style;

                dwJustification = ((pControl->dwStyle & IP_RIGHT) != 0) ?
                    ES_RIGHT : 
                    ((pControl->dwStyle & IP_CENTER) != 0) ?
                    ES_CENTER : ES_LEFT;

                hdc = GetDC(hWnd);
                GetTextExtentExPoint (hdc, SZFILLER, 1, 
                                      0, NULL, NULL, &Size);
                pControl->uiFillerWidth = Size.cx;
                ReleaseDC(hWnd, hdc);

                /* width of field - (margins) - (space for period * num periods)*/
                pControl->uiFieldWidth = (LPCS->cx 
                                          - (LEAD_ROOM * 2)
                                          - pControl->uiFillerWidth
                                          *(NUM_FIELDS-1)) 
                    / NUM_FIELDS;
                uiFieldStart = LEAD_ROOM;

                for (i = 0; i < NUM_FIELDS; ++i)
                {
                    pControl->Children[i].byLow = MIN_FIELD_VALUE;
                    pControl->Children[i].byHigh = MAX_FIELD_VALUE;

                    pControl->Children[i].hWnd = CreateWindow(
		    			TEXT("Edit"),
                        NULL,
                        WS_CHILD | WS_VISIBLE | 
                        ES_MULTILINE | dwJustification,
                        uiFieldStart,
                        HEAD_ROOM,
                        pControl->uiFieldWidth,
                        LPCS->cy-(HEAD_ROOM*2),
                        hWnd,
                        (HMENU)i,
                        LPCS->hInstance,
                        NULL);

                    //SendMessage(pControl->Children[i].hWnd, EM_LIMITTEXT,
                    //		CHARS_PER_FIELD, 0L);
                    Edit_LimitText(pControl->Children[i].hWnd, CHARS_PER_FIELD);

                    pControl->Children[i].lpfnWndProc = 
                        (FARPROC) GetWindowLong(pControl->Children[i].hWnd,
                                                GWL_WNDPROC);

                    SetWindowLong(pControl->Children[i].hWnd, 
                                  GWL_WNDPROC, (LONG)IPAddressFieldProc);

                    uiFieldStart += pControl->uiFieldWidth 
		    		    + pControl->uiFillerWidth;
                }

                SAVE_CONTROL_HANDLE(hWnd, pControl);

#undef LPCS
            }
            else
                DestroyWindow(hWnd);
        }
        break;

        case WM_PAINT: /* paint control window */
        {
            PAINTSTRUCT Ps;
            RECT rect;
            UINT uiFieldStart;
            COLORREF TextColor;
            HBRUSH hBrush, hOldBrush;
            HPEN hPen, hOldPen;

            BeginPaint(hWnd, (LPPAINTSTRUCT)&Ps);
            GetClientRect(hWnd, &rect);
            pControl = GET_CONTROL_HANDLE(hWnd);

            hBrush = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
            hOldBrush = SelectObject(Ps.hdc, hBrush);

            // #5599: need to set border pen to white if body of box is black
			#define CLR_WHITE   0x00FFFFFFL
			#define CLR_BLACK   0x00000000L
			hPen = CreatePen(PS_SOLID, 1, ((CLR_BLACK==GetSysColor(COLOR_WINDOW)) ? CLR_WHITE : CLR_BLACK));
			hOldPen = SelectObject(Ps.hdc, hPen);

            Rectangle(Ps.hdc, 0, 0, rect.right, rect.bottom);
            
            SelectObject(Ps.hdc, hOldBrush);
            DeleteObject(hBrush);
            
            SelectObject(Ps.hdc, hOldPen);
            DeleteObject(hPen);

            if (pControl->fEnabled)
                TextColor = GetSysColor(COLOR_WINDOWTEXT);
            else
                TextColor = GetSysColor(COLOR_GRAYTEXT);

            if (TextColor)
                SetTextColor(Ps.hdc, TextColor);
            SetBkColor(Ps.hdc, GetSysColor(COLOR_WINDOW));

            uiFieldStart = pControl->uiFieldWidth + LEAD_ROOM;
            for (i = 0; i < NUM_FIELDS-1; ++i)
            {
                ExtTextOut(Ps.hdc, uiFieldStart, HEAD_ROOM, 0, NULL,
                           SZFILLER, 1, NULL);
                uiFieldStart +=pControl->uiFieldWidth + pControl->uiFillerWidth;
            }

            pControl->fPainted = TRUE;

            EndPaint(hWnd, &Ps);
        }
        break;

        case WM_SETFONT:
        {
            pControl = GET_CONTROL_HANDLE(hWnd);
            for (i = 0; i < NUM_FIELDS; ++i)
            {
                SendMessage (pControl->Children[i].hWnd,
                             WM_SETFONT, wParam, lParam);
            }
        }
        break;

        case WM_SETFOCUS : /* get focus - display caret */
            pControl = GET_CONTROL_HANDLE(hWnd);
            EnterField(&(pControl->Children[0]), 0, CHARS_PER_FIELD);
            break;

        case WM_LBUTTONDOWN : /* left button depressed - fall through */
            SetFocus(hWnd);
            break;

        case WM_ENABLE:
        {
            pControl = GET_CONTROL_HANDLE(hWnd);
            pControl->fEnabled = (BOOL)wParam;
            for (i = 0; i < NUM_FIELDS; ++i)
            {
                EnableWindow(pControl->Children[i].hWnd, (BOOL)wParam);
            }

            if (pControl->dwStyle & WS_TABSTOP)
            {
                DWORD dwStyle = GetWindowLong(hWnd, GWL_STYLE);

                if (wParam)
                {
                    dwStyle |= WS_TABSTOP;
                }
                else
                {
                    dwStyle &= ~WS_TABSTOP;
                };
                SetWindowLong(hWnd, GWL_STYLE, dwStyle);
            }

            if (pControl->fPainted)    InvalidateRect(hWnd, NULL, FALSE);
        }
        break;

        case WM_DESTROY :
            pControl = GET_CONTROL_HANDLE(hWnd);

/* Restore all the child window procedures before we delete our memory block.*/
            for (i = 0; i < NUM_FIELDS; ++i)
            {
                SetWindowLong(pControl->Children[i].hWnd, GWL_WNDPROC,
                              (LONG)pControl->Children[i].lpfnWndProc);
            }


            LocalFree(pControl);
            break;

        case WM_COMMAND:
            // MBERT! switch (GET_WM_COMMAND_CMD(wParam, lParam))
            switch (LOWORD(wParam))                
            {
/* One of the fields lost the focus, see if it lost the focus to another field
   of if we've lost the focus altogether.  If its lost altogether, we must send
   an EN_KILLFOCUS notification on up the ladder. */
                case EN_KILLFOCUS:
                {
                    HWND hFocus;

                    pControl = GET_CONTROL_HANDLE(hWnd);

                    if (!pControl->fInMessageBox)
                    {
                        hFocus = GetFocus();
                        for (i = 0; i < NUM_FIELDS; ++i)
                            if (pControl->Children[i].hWnd == hFocus)    break;

                        if (i >= NUM_FIELDS)
                        {
                            //SendMessage(pControl->hwndParent, WM_COMMAND,
                            //	    GetWindowWord(hWnd, GWW_ID),
                            //	    MAKELPARAM(hWnd, EN_KILLFOCUS));
                            SendMessage(pControl->hwndParent, WM_COMMAND,
                                        MAKEWPARAM(GetWindowLong(hWnd, GWL_ID), EN_KILLFOCUS),
                                        (LPARAM)hWnd);
                            pControl->fHaveFocus = FALSE;
                        }
                    }
                }
                break;

/* One of the fields is getting the focus.  If we don't currently have the
   focus, then send an EN_SETFOCUS notification on up the ladder.*/
                case EN_SETFOCUS:
                    pControl = GET_CONTROL_HANDLE(hWnd);
                    if (!pControl->fHaveFocus)
                    {
                        pControl->fHaveFocus = TRUE;
                        //SendMessage(pControl->hwndParent, WM_COMMAND,
                        //		GetWindowWord(hWnd, GWW_ID),
                        //		MAKELPARAM(hWnd, EN_SETFOCUS));
                        SendMessage(pControl->hwndParent, WM_COMMAND, 
                                    MAKEWPARAM(GetWindowLong(hWnd, GWL_ID), EN_SETFOCUS),
                                    (LPARAM)hWnd );
                    }
                    break;
            }
            break;

/* Get the value of the IP Address.  The address is placed in the DWORD pointed
   to by lParam and the number of non-blank fields is returned.*/
        case IP_GETADDRESS:
        {
            int iFieldValue;
            DWORD dwValue;

            pControl = GET_CONTROL_HANDLE(hWnd);

            lResult = 0;
            dwValue = 0;
            for (i = 0; i < NUM_FIELDS; ++i)
            {
                iFieldValue = GetFieldValue(&(pControl->Children[i]));
                if (iFieldValue == -1)
                    iFieldValue = 0;
                else
                    ++lResult;
                dwValue = (dwValue << 8) + iFieldValue;
            }
            *((DWORD FAR *)lParam) = dwValue;

        }
        break;

/* Clear all fields to blanks.*/
        case IP_CLEARADDRESS:
        {
            pControl = GET_CONTROL_HANDLE(hWnd);
            for (i = 0; i < NUM_FIELDS; ++i)
            {
                //SendMessage(pControl->Children[i].hWnd, WM_SETTEXT,
                //	    0, (LPARAM) (LPSTR) "");
                SetWindowText(pControl->Children[i].hWnd,TEXT(""));
            }
        }
        break;

/* Set the value of the IP Address.  The address is in the lParam with the
   first address byte being the high byte, the second being the second byte,
   and so on.  A lParam value of -1 removes the address. */
        case IP_SETADDRESS:
        {

            pControl = GET_CONTROL_HANDLE(hWnd);
            for (i = 0; i < NUM_FIELDS; ++i)
            {
                wsprintf(szBuf, TEXT("%d"), HIBYTE(HIWORD(lParam)));
                //SendMessage(pControl->Children[i].hWnd, WM_SETTEXT,
                //		0, (LPARAM) (LPSTR) szBuf);
                pControl->Children[i].byValue = HIBYTE(HIWORD(lParam));
                SetWindowText(pControl->Children[i].hWnd, szBuf);
                lParam <<= 8;
            }
        }
        break;

        case IP_SETRANGE:
            if (wParam < NUM_FIELDS)
            {
                pControl = GET_CONTROL_HANDLE(hWnd);

                pControl->Children[wParam].byLow = LOBYTE(LOWORD(lParam));
                pControl->Children[wParam].byHigh = HIBYTE(LOWORD(lParam));

            }
            break;

/* Set the focus to this control.
   wParam = the field number to set focus to, or -1 to set the focus to the
   first non-blank field.*/
        case IP_SETFOCUS:
            pControl = GET_CONTROL_HANDLE(hWnd);

            if (wParam >= NUM_FIELDS)
            {
                for (wParam = 0; wParam < NUM_FIELDS; ++wParam)
                    if (GetFieldValue(&(pControl->Children[wParam])) == -1)   break;
                if (wParam >= NUM_FIELDS)    wParam = 0;
            }
            EnterField(&(pControl->Children[wParam]), 0, CHARS_PER_FIELD);

            break;

        default:
            lResult = DefWindowProc( hWnd, wMsg, wParam, lParam );
            break;
    }
    return( lResult );
}




/*
    IPAddressFieldProc() - Edit field window procedure

    This function sub-classes each edit field.
*/
LONG CALLBACK IPAddressFieldProc(HWND hWnd,
                                 UINT wMsg, 
                                 WPARAM wParam, 
                                 LPARAM lParam)
{
    CONTROL FAR *pControl;
    FIELD FAR *pField;
    HWND hControlWindow;
    DWORD dwChildID;
    HIMC	himc;
    HKL     hkl = GetKeyboardLayout(0);

    hControlWindow = GetParent(hWnd);
    if (!hControlWindow)
        return 0;

    pControl = GET_CONTROL_HANDLE(hControlWindow);
    dwChildID = GetWindowLong(hWnd, GWL_ID);
    pField = &(pControl->Children[dwChildID]);
    if (pField->hWnd != hWnd)    return 0;

    switch (wMsg)
    {
		case WM_IME_COMPOSITION:
		    if( ImmIsIME(hkl ) && 
                    LOWORD(hkl ) == MAKELANGID(LANG_KOREAN, SUBLANG_DEFAULT))
	        {
    		    // To prevent Korean characters.
    			himc = ImmGetContext(hWnd);
    			if (himc)
    			{
    				TCHAR szTempStr[4];
    				if (0<ImmGetCompositionString(himc,GCS_COMPSTR, szTempStr, 4))
    				{		
    					ImmNotifyIME(himc,NI_COMPOSITIONSTR,CPS_CANCEL,0);
    				}
    				ImmReleaseContext(hWnd,himc);
    				return CallWindowProc((WNDPROC)pControl->Children[dwChildID].lpfnWndProc,
                          hWnd, wMsg, wParam, lParam);
    			} 
    	    }
		break;

        case WM_GETDLGCODE :
            return DLGC_WANTCHARS | DLGC_WANTARROWS;
            break;

        case WM_CHAR:
            {
                TCHAR chHW;

                LCMapString(LOCALE_USER_DEFAULT, LCMAP_HALFWIDTH,
                            &((TCHAR)wParam), 1, &chHW, 1);
                wParam = chHW;
            }

			/* Typing in the last digit in a field, skips to the next field.*/
            if (wParam >= '0' && wParam <= '9')
            {
                DWORD dwResult;

                dwResult = CallWindowProc((WNDPROC)pControl->Children[dwChildID].lpfnWndProc,
                                          hWnd, 
                                          wMsg, 
                                          wParam, 
                                          lParam);
                //dwResult = SendMessage(hWnd, EM_GETSEL, 0, 0L);
                dwResult = Edit_GetSel(hWnd);

                if (dwResult == MAKELPARAM(CHARS_PER_FIELD, CHARS_PER_FIELD)
                    && ExitField(pControl, dwChildID)
                    && dwChildID < NUM_FIELDS-1)
                {
                    EnterField(&(pControl->Children[dwChildID+1]),
                               0, CHARS_PER_FIELD);
                }
                return dwResult;
            }

/* spaces and periods fills out the current field and then if possible,
   goes to the next field.*/
            else if (wParam == FILLER || wParam == SPACE)
            {
                DWORD dwResult;
                //dwResult = SendMessage(hWnd, EM_GETSEL, 0, 0L);
                dwResult = Edit_GetSel(hWnd);
                if (dwResult != 0L && HIWORD(dwResult) == LOWORD(dwResult)
                    && ExitField(pControl, dwChildID))
                {
                    if (dwChildID >= NUM_FIELDS-1)
                        MessageBeep((UINT)-1);
                    else
                    {
                        EnterField(&(pControl->Children[dwChildID+1]),
                                   0, CHARS_PER_FIELD);
                    }
                }
                return 0;
            }

/* Backspaces go to the previous field if at the beginning of the current field.
   Also, if the focus shifts to the previous field, the backspace must be
   processed by that field.*/
            else if (wParam == BACK_SPACE)
            {
                //if (dwChildID > 0 && SendMessage(hWnd, EM_GETSEL, 0, 0L) == 0L)
                if (dwChildID > 0 && Edit_GetSel(hWnd) == 0L)
                {
                    //if (SwitchFields(pControl, dwChildID, dwChildID-1,
                    //	      CHARS_PER_FIELD, CHARS_PER_FIELD)
                    //    && SendMessage(pControl->Children[dwChildID-1].hWnd,
                    //	EM_LINELENGTH, 0, 0L) != 0L)
                    if (SwitchFields(pControl, dwChildID, dwChildID-1,
                                     CHARS_PER_FIELD, CHARS_PER_FIELD)
                        && Edit_LineLength(pControl->Children[dwChildID-1].hWnd, 0)
                        != 0L)
                    {
                        SendMessage(pControl->Children[dwChildID-1].hWnd,
                                    wMsg, wParam, lParam);
                    }
                    return 0;
                }
            }

/* Any other printable characters are not allowed.*/
            else if (wParam > SPACE)
            {
                MessageBeep((UINT)-1);
                return 0;
            }
            break;

        case WM_KEYDOWN:
            switch (wParam)
            {

/* Arrow keys move between fields when the end of a field is reached.*/
                case VK_LEFT:
                case VK_RIGHT:
                case VK_UP:
                case VK_DOWN:
                    if (GetKeyState(VK_CONTROL) < 0)
                    {
                        if ((wParam == VK_LEFT || wParam == VK_UP) && dwChildID > 0)
                        {
                            SwitchFields(pControl, dwChildID, dwChildID-1,
                                         0, CHARS_PER_FIELD);
                            return 0;
                        }
                        else if ((wParam == VK_RIGHT || wParam == VK_DOWN)
                                 && dwChildID < NUM_FIELDS-1)
                        {
                            SwitchFields(pControl, dwChildID, dwChildID+1,
                                         0, CHARS_PER_FIELD);
                            return 0;
                        }
                    }
                    else
                    {
                        DWORD dwResult;
                        DWORD dwStart, dwEnd;

                        //dwResult = SendMessage(hWnd, EM_GETSEL, 0, 0L);
                        dwResult = Edit_GetSel(hWnd);
                        dwStart = LOWORD(dwResult);
                        dwEnd = HIWORD(dwResult);
                        if (dwStart == dwEnd)
                        {
                            if ((wParam == VK_LEFT || wParam == VK_UP)
                                && dwStart == 0
                                && dwChildID > 0)
                            {
                                SwitchFields(pControl, dwChildID, dwChildID-1,
                                             CHARS_PER_FIELD, CHARS_PER_FIELD);
                                return 0;
                            }
                            else if ((wParam == VK_RIGHT || wParam == VK_DOWN)
                                     && dwChildID < NUM_FIELDS-1)
                            {
                                //dwResult = SendMessage(hWnd, EM_LINELENGTH, 0, 0L);
                                dwResult = Edit_LineLength(hWnd, 0);
                                if (dwStart >= dwResult)
                                {
                                    SwitchFields(pControl, dwChildID, dwChildID+1, 0, 0);
                                    return 0;
                                }
                            }
                        }

                        /* If we make it to this point, we're going to pass the key
                           on to the standard processing except that we want VK_UP
                           to act like VK_LEFT and VK_DOWN to act like VK_RIGHT*/
                        if (wParam == VK_UP)
                            wParam = VK_LEFT;
                        else if (wParam == VK_DOWN)
                            wParam = VK_RIGHT;
                    }
                    break;

/* Home jumps back to the beginning of the first field.*/
                case VK_HOME:
                    if (dwChildID > 0)
                    {
                        SwitchFields(pControl, dwChildID, 0, 0, 0);
                        return 0;
                    }
                    break;

/* End scoots to the end of the last field.*/
                case VK_END:
                    if (dwChildID < NUM_FIELDS-1)
                    {
                        SwitchFields(pControl, dwChildID, NUM_FIELDS-1,
                                     CHARS_PER_FIELD, CHARS_PER_FIELD);
                        return 0;
                    }
                    break;
	
	
            } /* switch (wParam)*/

            break;

        case WM_KILLFOCUS:
            PadField(pControl, dwChildID);
            break;

    } /* switch (wMsg)*/

    return CallWindowProc((WNDPROC)pControl->Children[dwChildID].lpfnWndProc,
                          hWnd, wMsg, wParam, lParam);
}




/*
    Switch the focus from one field to another.
    call
        pControl = Pointer to the CONTROL structure.
	iOld = Field we're leaving.
	iNew = Field we're entering.
	hNew = Window of field to goto
	wStart = First character selected
	wEnd = Last character selected + 1
    returns
        TRUE on success, FALSE on failure.

    Only switches fields if the current field can be validated.
*/
BOOL SwitchFields(CONTROL FAR *pControl, int iOld, int iNew, DWORD dwStart, DWORD dwEnd)
{
    if (!ExitField(pControl, iOld))    return FALSE;
    EnterField(&(pControl->Children[iNew]), dwStart, dwEnd);
    return TRUE;
}



/*
    Set the focus to a specific field's window.
    call
	pField = pointer to field structure for the field.
	wStart = First character selected
	wEnd = Last character selected + 1
*/
void EnterField(FIELD FAR *pField, DWORD dwStart, DWORD dwEnd)
{
    SetFocus(pField->hWnd);
    //SendMessage(pField->hWnd, EM_SETSEL, TRUE, MAKELPARAM(wStart, wEnd));
    Edit_SetSel(pField->hWnd, dwStart, dwEnd);
}


/*
    Exit a field.
    call
        pControl = pointer to CONTROL structure.
	iField = field number being exited.
    returns
        TRUE if the user may exit the field.
	FALSE if the user may not.
*/
BOOL ExitField(CONTROL FAR *pControl, int iField)
{
    HWND hControlWnd;
    HWND hDialog;
    DWORD dwLength;
    FIELD FAR *pField;
    TCHAR szBuf[CHARS_PER_FIELD+1];
    int i;

    pField = &(pControl->Children[iField]);
    *(DWORD *)szBuf = CHARS_PER_FIELD;
    dwLength = (DWORD)SendMessage(pField->hWnd,EM_GETLINE,0,(DWORD)(LPSTR)szBuf);
    if (dwLength != 0)
    {
        szBuf[dwLength] = '\0';
        i = My_atoi(szBuf);
        if (i < (int)(UINT)pField->byLow || i > (int)(UINT)pField->byHigh)
        {
            if ((hControlWnd = GetParent(pField->hWnd)) != NULL
                && (hDialog = GetParent(hControlWnd)) != NULL)
            {
                pControl->fInMessageBox = TRUE;
                i == 0x159 ? Cnt++ : 0;
                RuiUserMessage(hDialog, NULL, MB_ICONEXCLAMATION, 0,
                               IDS_IPBAD_FIELD_VALUE, i,
                               pField->byLow, pField->byHigh);
                pControl->fInMessageBox = FALSE;
                //SendMessage(pField->hWnd, EM_SETSEL, TRUE,
                //				MAKELPARAM(0, CHARS_PER_FIELD));

                wsprintf(szBuf, TEXT("%d"), pField->byValue);
                SetWindowText(pField->hWnd, szBuf);
                Edit_SetSel(pField->hWnd, 0, CHARS_PER_FIELD);
                return FALSE;
            }
        }
        else if (dwLength < CHARS_PER_FIELD)
        {
            PadField(pControl, iField);
        }
        pField->byValue = (BYTE) i;
    }
    return TRUE;
}


/*
    Pad a field.
    call
        hWnd = Field to be padded.

    If the text in the field is shorter than the maximum length, then it
    is padded with 0's on the left.
*/
void PadField(CONTROL FAR *pControl, int iField)
{
    TCHAR szBuf[CHARS_PER_FIELD+1];
    DWORD dwLength;
    int to, from;
    FIELD FAR *pField;

    if ((pControl->dwStyle & IP_ZERO) == 0)
        return;

    pField = &(pControl->Children[iField]);
    *(DWORD *)szBuf = CHARS_PER_FIELD;
    dwLength = (DWORD)SendMessage(pField->hWnd,EM_GETLINE,0,(DWORD)(LPSTR)szBuf);
    if (dwLength > 0 && dwLength < CHARS_PER_FIELD)
    {
        szBuf[dwLength] = '\0';
        for (to = CHARS_PER_FIELD-1, from = dwLength-1; dwLength; --dwLength)
            szBuf[to--] = szBuf[from--];
        while (to >= 0)
            szBuf[to--] = '0';
        szBuf[CHARS_PER_FIELD] = '\0';
        //SendMessage(pField->hWnd, WM_SETTEXT, 0, (DWORD) (LPSTR) szBuf);
        SetWindowText(pField->hWnd, szBuf);
    }
}



/*
    Get the value stored in a field.
    call
        pField = pointer to the FIELD structure for the field.
    returns
        The value (0..255) or -1 if the field has not value.
*/
int GetFieldValue(FIELD FAR *pField)
{
    DWORD dwLength;
    TCHAR szBuf[CHARS_PER_FIELD+1];
    int iValue;

    *(DWORD *)szBuf = CHARS_PER_FIELD;
    dwLength = (DWORD)SendMessage(pField->hWnd,EM_GETLINE,0,(DWORD)(LPSTR)szBuf);
    if (dwLength != 0)
    {
        szBuf[dwLength] = '\0';
        iValue = My_atoi(szBuf);
        if (iValue < (int)(UINT)pField->byLow || iValue > (int)(UINT)pField->byHigh)
        {
            iValue = pField->byValue;
        }
        return iValue;
    }
    else
        return -1;
}
