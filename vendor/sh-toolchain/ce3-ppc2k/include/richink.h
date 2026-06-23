//***************************************************************************
//
//  Copyright (c) 1997-1999  Microsoft Corporation.  All Rights Reserved.
//
//  File:
//          richink.h
//
//  Description:
//
//          Include file for WinCE RichInk Edit Control.
//
//***************************************************************************

#ifndef _RICHINK_INCLUDED_
#define _RICHINK_INCLUDED_

#include <windows.h>
#include <commctrl.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


// Constants

#define WC_RICHINKA   "RichInk"
#define WC_RICHINKW  L"RichInk"
#ifdef UNICODE
#define WC_RICHINK    WC_RICHINKW
#else
#define WC_RICHINK    WC_RICHINKA
#endif

// Application calls this function to load and initialize the control properly.
// The control can be inserted as a dialog custom control with class setting "RichInk"
//  (see constant WC_RICHINK above).
void InitRichInkDLL(void);


// Defines and typedefs needed for the Rich Ink stream messages.

//---------------------------------------------------------
// EditStreamCallback
//
//  The EditStreamCallback function is an application-defined
//  callback function used with the EM_STREAMIN and EM_STREAMOUT
//  messages. It is used to transfer a stream of data into or
//  out of a rich edit control. The EDITSTREAMCALLBACK type defines
//  a pointer to this callback function. EditStreamCallback is a
//  placeholder for the application-defined function name. 
//
// SEE Microsoft Developer Studio Help or MSDN for more details.
//---------------------------------------------------------
typedef DWORD (CALLBACK *EDITSTREAMCALLBACK)
(DWORD dwCookie, LPBYTE pbBuff, LONG cb, LONG *pcb);

DWORD CALLBACK EditStreamCallback(
  DWORD dwCookie, // application-defined value
  LPBYTE pbBuff,  // pointer to a buffer
  LONG cb,        // number of bytes to read or write
  LONG *pcb       // pointer to number of bytes transferred
);

typedef struct _editstream
{
    DWORD dwCookie;     /* user value passed to callback as first parameter */
    DWORD dwError;      /* last error */
    EDITSTREAMCALLBACK pfnCallback;
} EDITSTREAM;

typedef struct tagCOOKIE
{
    HANDLE hFile;
    LPBYTE pbStart;
    LPBYTE pbCur;
    LONG   bCount;
    DWORD  dwError;
} COOKIE, * PCOOKIE;


// Stream formats:
#define SF_TEXT         0x0001
#define SF_RTF          0x0002

#define SF_UNICODE      0x0010                          // Unicode data of some kind
#define SF_UTEXT        (SF_TEXT | SF_UNICODE)            // Unicode text file

#define SFF_PWI         0x0800      
#define SF_PWI          ( SF_RTF | SFF_PWI | 0x010000 ) // Pocket Word Ink (PWI) format


// Supported EM_ and WM_ Messages:

//---------------------------------------------------------
//
//  EM_STREAMIN
//  
//  The EM_STREAMIN message replaces the contents of a rich ink
//  control with a stream of data provided by an application-defined
//  EditStreamCallback callback function.
//
//
//  wParam = (WPARAM) (UINT) uFormat; 
//  lParam = (LPARAM) (EDITSTREAM FAR *) lpStream; 
// 
//  Parameters
//
//      uFormat 
//
//      A set of bit flags that indicate the data format and replacement options.
//      This value must specify one of the following data formats.
//
//      SF_TEXT     Ascii Text 
//      SF_RTF      Rich Text Format (RTF) 
//      SF_UTEXT    Unicode text
//      SF_PWI      Pocket Word Ink (PWI)
//
//
//      lpStream 
//
//      Pointer to an EDITSTREAM structure. On input, the pfnCallback member of
//      this structure must point to an application-defined EditStreamCallback
//      function. On output, the dwError member can contain a nonzero error code
//      if an error occurred.
//
//      When this DLL is used on the desktop, it is assumed that the EDITSTREAM
//      structure contains a dwCookie of the following type:
//
//          typedef struct
//          {
//              IStream        *pstm;
//              BOOL            bValue;
//              void           *pData;
//              BOOL            bLoss;
//          }  COOKIE;
//
//      The rich ink control may then modify the value bLoss to indicate
//      whether any data loss occurs during the input conversion.
//
//      Return value
//
//      Returns zero if no errors.
//
//      Remarks
//
//      When you send an EM_STREAMIN message, the rich ink control makes
//      repeated calls to the EditStreamCallback function specified by the
//      pfnCallback member of the EDITSTREAM structure. 
//      Each time the callback function is called, it fills a buffer with
//      data to read into the control. This continues until the callback
//      function indicates that the stream-in operation has been completed
//      or an error occurs.
//
//      NOTE:  The EditStreamCallback callback function returns the number
//             of bytes processed.  If this is not the same as the number
//             of bytes requested then the caller assumes an end of file
//             condition has occurred.
//
//---------------------------------------------------------
#define EM_STREAMIN             (WM_USER + 73)
 

//---------------------------------------------------------
//
//  EM_STREAMOUT
//  
//  The EM_STREAMOUT message causes a rich ink control to pass
//  its contents to an application-defined EditStreamCallback callback
//  function. The callback function can then write the stream of data
//  to a file or any other location that it chooses. 
//
//  wParam = (WPARAM) (UINT) uFormat; 
//  lParam = (LPARAM) (EDITSTREAM FAR *) lpStream; 
// 
//  Parameters
//
//      uFormat 
//
//      A set of bit flags that indicate the data format and replacement options.
//      This value must specify one of the following data formats.
//
//      SF_TEXT     Ascii Text 
//      SF_RTF      Rich Text Format (RTF) 
//      SF_UTEXT    Unicode text
//      SF_PWI      Pocket Word Ink (PWI)
//
//
//      lpStream 
//
//      Pointer to an EDITSTREAM structure. On input, the pfnCallback member
//      of this structure must point to an application-defined EditStreamCallback
//      function. On output, the dwError member can contain a nonzero error code
//      if an error occurred. 
//
//      When this DLL is used on the desktop, it is assumed that the EDITSTREAM
//      structure contains a dwCookie of the following type:
//
//          typedef struct
//          {
//              IStream        *pstm;
//              BOOL            bValue;
//              void           *pData;
//              BOOL            bLoss;
//          }  COOKIE;
//
//      The rich ink control may then modify the value bLoss to indicate
//      whether any data loss occurs during the output conversion.
//
//
//      Return value
//
//      Returns zero if no errors.
//
//      Remarks
//
//      When you send an EM_STREAMOUT message, the rich ink control makes
//      repeated calls to the EditStreamCallback function specified by the
//      pfnCallback member of the EDITSTREAM structure. Each time it calls
//      the callback function, the control passes a buffer containing a
//      portion of the contents of the control. This process continues until
//      the control has passed all its contents to the callback function,
//      or until an error occurs.
//
//      NOTE:  The EditStreamCallback callback function returns the number
//             of bytes processed.  If this is not the same as the number
//             of bytes requested then the caller assumes an end of file
//             condition has occurred.
//
//      NOTE:  When streaming out data as Unicode text a NULL character
//             always terminates the output.  This in effect adds 2 bytes
//             to the file size when streaming in Unicode text and then
//             streaming it back out.  This is NOT the case when streaming
//             normal text.  With normal text, no NULL character is added.
//
//      NOTE:  When streaming out data as SF_PWI if there are no text formating
//             or ink objects contained within the document then the contents may
//             be streamed out as pure multi-byte characters rather than binary 
//             data in order to have smaller file sizes.
//
//---------------------------------------------------------
#define EM_STREAMOUT            (WM_USER + 74)


//---------------------------------------------------------
// IMPORTANT COMPATIBILITY NOTE:
//
//  Previous versions of the richink.dll required the EM_STREAMIN
//  and EM_STREAMOUT messages to pass a pointer to the parameter 
//  which specifies the format type rather than passing the format
//  type directly.  Here is an example:
//
//  DWORD   wpSF = SF_PWI;
//
//  SendMessage (hwndInk, EM_STREAMIN, (WPARAM)&wpSF, (LPARAM)&es);
//---------------------------------------------------------


//---------------------------------------------------------
//
//  EM_CLEARALL
//  
//  The EM_CLEARALL message causes a rich ink control to clear
//  all of the current documents data.
//
//  wParam = (WPARAM) (BOOL) fDirty; 
//  lParam = (LPARAM) 0; 
// 
//  Parameters
//
//      fDirty -- true to dirty window for repaint after clear. 
//
//  Return value - returns nothing.
//
//---------------------------------------------------------
#define EM_CLEARALL             (WM_USER + 331)


//---------------------------------------------------------
//
//  EM_SETVIEWATTRIBUTES
//  
//  The EM_SETVIEWATTRIBUTES message sets the view attributes
//  of the rich ink control.
//
//  wParam = (WPARAM) 0; 
//  lParam = (LPARAM) (VIEWATTRIBUTES) pViewAttributes; 
// 
//  Parameters
//
//      pViewAttributes - points to a VIEWATTRIBUTES struct.
//
//  Return value - returns nothing.
//
//---------------------------------------------------------
#define EM_SETVIEWATTRIBUTES    (WM_USER + 332)

//____________________________________________________
//
// mask values for EM_SETVIEWATTRIBUTES
//
#define VIEWATTRIBUTE_ZOOM          0x01
#define VIEWATTRIBUTE_PAGESTYLE     0x02
#define VIEWATTRIBUTE_INKLAYER      0x04
#define VIEWATTRIBUTE_VIEW          0x08

typedef struct tagVIEWATTRIBUTES
{
    UINT    mask;
    UINT    uView;          // The view to change to
    UINT    uZoom;          // The zoom percent or level to change to
    UINT    uPageStyle;     // Set the line style shown in view
    UINT    uInkLayer;      // Set the ink layer in this view
    BOOL    fReserved;      // Reserved for future use set to zero.
} VIEWATTRIBUTES, *PVIEWATTRIBUTES;

//____________________________________________________
//  WRAP modes
//
// Define the richink wrap modes.
//
#define RI_WRAPTOPAGE           0
#define RI_WRAPTOWINDOW         1

//____________________________________________________
//  VIEW modes
//
// View types for EM_SETVIEW and EM_SETVIEWATTRIBUTES messages.
//
#define VT_TYPINGVIEW       0
#define VT_WRITINGVIEW      2
#define VT_DRAWINGVIEW      3

//____________________________________________________
//  INK layer modes
//
// Define the ink layers.
//
#define VL_SMARTINK             0
#define VL_WRITINGINK           1
#define VL_DRAWINGINK           2
#define VL_LINKS                3
#define VL_SMARTLINKS           4   // VL_SMARTLINKS differs from VL_LINKS 
                                    // in that focus is set to control if 
                                    // a link isn't clicked.

//____________________________________________________
// PAGE Styles
//
//  Only a single page style can be set at one time.
//
//  The PS_DOTTEDLINES and PS_YELLOWBACKGROUND
//   attributes can be OR'ed with a single page style
//   to produce dotted lines or yellow background.
//
#define PS_LEFTMARGIN           0x0000
#define PS_TOPMARGIN            0x0001
#define PS_RULEDLINES           0x0002
#define PS_GRIDLINES            0x0004
#define PS_TOPLEFTMARGIN        0x0008
#define PS_NONE                 0x0010
#define PS_DOTTEDLINES          0x0020
#define PS_YELLOWBACKGROUND     0x0040

//____________________________________________________
// Pen mode defines
//
#define MODE_PEN                    0
#define MODE_SELECT                 1
#define MODE_SPACE                  2

/*------------- SAMPLE CODE ------------------

        VIEWATTRIBUTES  viewAttr;
    
        // Clear the struct
        memset(&viewAttr, 0, sizeof(viewAttr));

        // Set the window attributes
        // what view should this be called?
        viewAttr.mask           |= VIEWATTRIBUTE_VIEW;
        viewAttr.uView          = VT_TYPINGVIEW;
       
        // this view should have typing in it
        viewAttr.mask           |= VIEWATTRIBUTE_TYPINGONLY;
        viewAttr.fTypingOnly    = FALSE;

        // Set zoom level 100%
        viewAttr.mask           |= VIEWATTRIBUTE_ZOOM;
        viewAttr.uZoom          = 100;

        // Set the page style
        viewAttr.mask |= VIEWATTRIBUTE_PAGESTYLE | VIEWATTRIBUTE_INKLAYER;
        
        // no lines
        viewAttr.uPageStyle     = PS_NONE;
        viewAttr.uInkLayer      = VL_SMARTINK;
        
        // Set the attributes
        SendMessage(hwndGlobalNotes, EM_SETVIEWATTRIBUTES, 0, (LPARAM)&viewAttr);

------------- END SAMPLE CODE ------------------*/


//---------------------------------------------------------
//
// Message to get and set individual aspects of the view mode.
//
//---------------------------------------------------------

//---------------------------------------------------------
//
//  EM_SETPAGESTYLE
//  
//  The EM_SETPAGESTYLE message sets the page style
//  (ruled lines, etc.) of the rich ink control.
//
//  wParam = (WPARAM) (UINT) pageStyle; 
//  lParam = (LPARAM) 0; 
// 
//  Parameters
//
//      pageStyle - Page Style to set (PS_* constants)
//
//  Return value - returns nothing.
//
//---------------------------------------------------------
#define EM_SETPAGESTYLE         (WM_USER + 287)


//---------------------------------------------------------
//
//  EM_GETPAGESTYLE
//  
//  The EM_GETPAGESTYLE message gets the page style
//  (ruled lines, etc.) of the rich ink control.
//
//  wParam = (WPARAM) 0; 
//  lParam = (LPARAM) 0; 
// 
//  Returned value:
//
//      pageStyle - current Page Style (PS_* constants)
//                    (No error conditions returned)
//
//---------------------------------------------------------
#define EM_GETPAGESTYLE         (WM_USER + 323)


//---------------------------------------------------------
//
//  EM_SETWRAPMODE
//  
//  The EM_SETWRAPMODE message sets the wrap mode
//  (window or page) of the rich ink control.
//
//  wParam = (WPARAM) wrapMode:
//                  RI_WRAPTOWINDOW = wrap to window
//                  RI_WRAPTOPAGE = wrap to page
//  lParam = (LPARAM) 0; 
// 
//  Return value - returns nothing.
//
//---------------------------------------------------------
#define EM_SETWRAPMODE          (WM_USER + 319)


//---------------------------------------------------------
//
//  EM_GETWRAPMODE
//  
//  The EM_GETWRAPMODE message gets the wrap mode
//  (window or page) of the rich ink control.
//
//  wParam = (WPARAM) 0; 
//  lParam = (LPARAM) 0; 
// 
//  Returned value:
//
//      wrapMode:
//              RI_WRAPTOWINDOW = wrap to window
//              RI_WRAPTOPAGE = wrap to page
//
//---------------------------------------------------------
#define EM_GETWRAPMODE          (WM_USER + 227)


//---------------------------------------------------------
//
//  EM_SETVIEW
//  
//  The EM_SETVIEW message sets the view style
//  (typing, writing or drawing) of the rich ink control.
//
//  wParam = (WPARAM) view to set:
//                  VT_TYPINGVIEW - typing view
//                  VT_WRITINGVIEW - writing view
//                  VT_DRAWINGVIEW - drawing view
//
//  lParam = (LPARAM) 0; 
// 
//  Return value - returns nothing.
//
//---------------------------------------------------------
#define EM_SETVIEW              (WM_USER + 284)


//---------------------------------------------------------
//
//  EM_GETVIEW
//  
//  The EM_GETVIEW message gets the view style
//  (typing, writing or drawing) of the rich ink control.
//
//  wParam = (WPARAM) 0; 
//  lParam = (LPARAM) 0; 
// 
//  Returned value:   current view:
//
//                  VT_TYPINGVIEW - typing view
//                  VT_WRITINGVIEW - writing view
//                  VT_DRAWINGVIEW - drawing view
//
//---------------------------------------------------------
#define EM_GETVIEW              (WM_USER + 254)


//---------------------------------------------------------
//
//  EM_SETINKLAYER
//  
//  The EM_SETINKLAYER message sets the page style
//  (ruled lines, etc.) of the rich ink control.
//
//  wParam = (WPARAM) (UINT) inkLayer; 
//  lParam = (LPARAM) 0; 
// 
//  Parameters
//
//      inkLayer - Ink Layer to set (VL_* constants)
//
//  Return value - returns previous Ink Layer.
//
//---------------------------------------------------------
#define EM_SETINKLAYER          (WM_USER + 288)


//---------------------------------------------------------
//
//  EM_SETZOOMPERCENT
//  
//  The EM_SETZOOMPERCENT message sets the zoom percentage 
//  of the rich ink control.
//
//  wParam = (LPARAM) 0; 
//  lParam = (WPARAM) (UINT) zoomPercent; 
// 
//  Parameters
//
//      zoomPercent - zoom percent to set.
//
//  NOTE - there is a minimum of 75% and maximum of
//         300%. values outside this range will b
//         normalized to be within this range.
//
//  Return value - returns nothing.
//
//---------------------------------------------------------
#define EM_SETZOOMPERCENT       (WM_USER + 290)


//---------------------------------------------------------
//
//  EM_GETZOOMPERCENT
//  
//  The EM_GETZOOMPERCENT message gets the zoom percentage 
//  of the rich ink control.
//
//  wParam = (WPARAM) 0; 
//  lParam = (LPARAM) 0; 
// 
//  Returned value:
//
//      zoomPercent - Contains the current zoom percentage.
//                    (No error conditions returned)
//
//---------------------------------------------------------
#define EM_GETZOOMPERCENT       (WM_USER + 289)


//---------------------------------------------------------
//
//  EM_SETPENMODE
//  
//  The EM_SETPENMODE message sets the new pen mode 
//  (pen, select, space) of the rich ink control.
//
//  wParam = (WPARAM) (UINT) penMode; 
//  lParam = (LPARAM) 0; 
// 
//  Parameters
//
//      penMode - Contains Pen Mode (MODE_PEN,
//                MODE_SELECT, or MODE_SPACE
//
//  Return value - returns nothing.
//
//---------------------------------------------------------
#define EM_SETPENMODE           (WM_USER + 329)


//---------------------------------------------------------
//
//  EM_GETPENMODE
//  
//  The EM_SETPENMODE message sets the new pen mode 
//  (pen, select, space) of the rich ink control.
//
//  wParam = (WPARAM) 0; 
//  lParam = (LPARAM) 0; 
// 
//  Returned value:
//
//      penMode - Contains the Pen Mode (MODE_PEN,
//                MODE_SELECT, or MODE_SPACE)
//                (No error conditions returned)
//
//---------------------------------------------------------
#define EM_GETPENMODE           (WM_USER + 328)


//---------------------------------------------------------
//
//  EM_UNDOEVENT / EM_REDOEVENT
//  
//  The EM_UNDOEVENT and EM_REDOEVENT messages undo or redo
//  the last undoable or redoable actions.
//
//  wParam = (WPARAM) 0; 
//  lParam = (LPARAM) 0; 
// 
//  Return value - returns nothing.
//
//---------------------------------------------------------
#define EM_UNDOEVENT            (WM_USER + 234)
#define EM_REDOEVENT            (WM_USER + 235)


//---------------------------------------------------------
//
//  EM_CANUNDO / EM_CANREDO
//  
//  The EM_CANUNDO and EM_CANREDO messages determine if there
//  undoable or redoable operations.
//
//  wParam = (WPARAM) 0; 
//  lParam = (LPARAM) 0; 
// 
//  Returned value:
//      TRUE - non-zero if there is a transaction
//      FALSE - zero if no transactions.
//
//---------------------------------------------------------
// Note:  EM_CANUNDO is defined in winuser.h
#define EM_CANREDO              (WM_USER + 246)


//---------------------------------------------------------
//
//  EM_INSERTLINKS
//  
//  The EM_INSERTLINKS message inserts the passed string 
//  as a link string in the rich ink control.
//
//  wParam = (WPARAM) (UINT) strLength; 
//  lParam = (LPARAM) (LPWCHAR) lpString; 
// 
//  Parameters
//
//      strLenght - number of Unicode characters in string.
//      lpString - pointer to Unicode string.
//
//  Return value - returns nothing.
//
// Sample strings:  "Tap <file:notes{Go to Notes}> to go to Notes\r"
//                  "Tap <file:SHFind{Find}> to find things."
//
// Brief syntax explanations:
//
//   <file:’ indicates a start of a link ending with ‘>’.
//
//   "Notes" indicates shell execute command.  If there are no backslashes depicting a path
//        (e.g., \\mydocs\foo.pwi) the first parameter is executed and the subsequent 
//        items are sent as parameters.
//
//    {Go to Notes} indicates text shown as link.
//
//   "Tap " and " to go to Notes" are normal text.
//
//   ‘\r’ indicates new line.
//
// NOTE -- To set up a Link Control in a Dialog use the ES_EX_CONTROLPARENT
//         WS_EX_STYLE bit to set up the proper font and modes.
//
//    CONTROL    "",IDC_GLOBALOPTIONS_LINK,RICHINK_CLASS, ES_MULTILINE | WS_TABSTOP, 
//               4,139,100,10, ES_EX_CONTROLPARENT
//
//  THEN use SendMessage to specify the text for the link:
//
//   HWND hwndGlobalNotes = GetDlgItem(hDlg,  IDC_GLOBALOPTIONS_LINK);
//
//   SendMessage (hwndGlobalNotes, EM_INSERTLINKS, (WPARAM)lstrlen(szTitle), (LPARAM)szTitle);        
//
//---------------------------------------------------------
#define EM_INSERTLINKS          (WM_USER + 333)

// Extended Window style supported by the RichInk control.
// Used with EM_INSERTLINKS to specify standard link view styles.
#define ES_EX_CONTROLPARENT		0x00100000

//---------------------------------------------------------
//
// Other useful Rich Edit control and Windows messages 
//  that are supported by the Rich Ink control:
//
//      EM_GETSEL
//      EM_SETSEL
//      EM_REPLACESEL
//      EM_GETMODIFY
//      EM_SETMODIFY
//
//      WM_GETTEXTLENGTH
//      WM_GETTEXT
//      WM_SETTEXT 
//
//      WM_CUT
//      WM_COPY
//      WM_PASTE
//      WM_CLEAR
//      EM_CANPASTE
//
// See Microsoft Developer Studio Help or MSDN for more details.
//
// NOTE:    The EM_SETSEL, EM_GETSEL and EM_REPLACESEL messages are
//          intended for use with text in the VT_TYPINGVIEW and 
//          VT_WRITINGVIEW view modes.  These operations with the view 
//          mode set to VT_DRAWINGVIEW will have inconsistent results.
//
//---------------------------------------------------------

#ifndef EM_CANPASTE
#define EM_CANPASTE				(WM_USER + 50)
#endif


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _RICHINK_INCLUDED_
