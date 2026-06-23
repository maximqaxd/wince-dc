//===========================================================================
//
// Copyright (c) Microsoft Corporation 1991-1995
//
// File: wceshell.h
//
//===========================================================================
#ifndef __WCESHELL_H_
#define __WCESHELL_H_

#include <shlobj.h>	

#ifdef __cplusplus
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////
	
//
// EXPLORERINFO -- Information about current filesystem browser
//
typedef struct _EXPLORERINFO
{
 	DWORD uFlags;
	LPSHELLFOLDER lpsf;
	LPITEMIDLIST pidl;
	HWND hwndExplorer;
	HWND hwndListView;
	HWND hwndToolbar;
	HMENU hmenu;
	LONG idCmdFirst;
	UINT idView;
	UINT nSortOrder;
	BOOL fRecycle;
	CEOID oidRoot; // This is nonzero when browsing removable storage
	BOOL fInLabelEdit;
	BOOL fShowBanner;
	DWORD dwData;
} EXPLORERINFO, * LPEXPLORERINFO;
typedef const EXPLORERINFO * LPCEXPLORERINFO;


//-------------------------------------------------------------------------
//
// IShellListView interface  /* WinCE Only Interface ! */
//
//
// [Member functions]
//
// IShellListView::GetAttributes(prgfInOut)
//   This function returns the state of the command bar for the main window
//   it is called to determine which buttons to put on the explorer command
//   bar.
//
// IShellListView::RegisterFilter(lpfnFilter, dwID)
//   This function is called to register a Shell Filter callback. The
//   dwID will get passed back to the callback along with the PIDL, being
//   refered to. This function will be called to determine if the PIDL
//   should be visible when this vew is visible.
//
// IShellListView::UIActivate(lpED, hmenu, cmdFirst, cmdLast)
//   This function is called when the UI for this view will become visible,
//   your view is alloed to add context senstive verbs to the hmenu, at
//   the begining of the menu and can have commanf id's within the range of
//   cmdFirst and cmdLast.
//
// IShellListView::UIDeactivate(lpED, hmenu, cmdFirst)
//   This function is called when the UI for this view will become invisible,
//   or go away. Your view needs to remove its commands from the menu, and
//   also all items and columns from the ListView hwnd. Anything that that
//   this view has attached the hwnd (ie OLE DropTarget) must also be removed,
//   since the life of the hwnd ListView is not guarenteed when this function
//   is returned.
//
// IShellListView::UpdateContents(lpED)
//   This function is called to refresh the contents of the listview window.
//   THis function is where you can add or remove items from the listview.
//
// IShellListView::ContextMenu(lpED)
//   This function is called to cause you to display the context menu for
//   which should be displayed at this point. This context menu will only
//   be for items 
//
// IShellListView::Notify(lpED, msg, wp, lp)
//   This function is called in response to a WM_NOTIFY, WM_COMMAND,
//   or WM_INITMENUPOPUP for the "File" context menu.
//   
//-------------------------------------------------------------------------

#undef 	INTERFACE
#define	INTERFACE 	IShellListView

#define SLVGA_CANDELETE          SFGAO_CANDELETE
#define SLVGA_HASPROPSHEET       SFGAO_HASPROPSHEET
#define SLVGA_HASLARGEICONS      0x00000200L
#define SLVGA_HASSMALLICONS      0x00000400L
#define SLVGA_HASDETAILS         0x00000800L
#define SLVGA_CANGOUPONE         0x00001000L
#define SLVGA_HASNEWFOLDER       0x00002000L
#define SLVGA_DEFAULTBROWSER     0x00008000L
#define SLVGA_DESKTOPWINDOW      0x10000000L


// The browser object should be able to use the following callback
// to determine whether or not to display a specific PIDL. Browser objects
// do not need to support this, but if the do they must return NOERROR
// to IShellListView::RegisterFilter() and call this routine during any
// repainting or updates.

typedef LRESULT (CALLBACK* LPFNSHELLFILTER)(UINT, LPITEMIDLIST);


DECLARE_INTERFACE_(IShellListView, IUnknown)
{						
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

	// *** IShellListView methods ***
	STDMETHOD(GetAttributes) (THIS_ ULONG *rgfInOut) PURE;
	STDMETHOD(RegisterFilter) (THIS_ LPFNSHELLFILTER lpfnFilter,
							   UINT dwID) PURE;
	STDMETHOD(UIActivate) (THIS_ LPCEXPLORERINFO lped, HMENU hmenu, int cmdFirst,
						   int cmdLast) PURE;
	STDMETHOD(UIDeactivate) (THIS_ LPCEXPLORERINFO lped, HMENU hmenu, int cmdFirst) PURE;
	STDMETHOD(UpdateContents) (THIS_ LPCEXPLORERINFO lped) PURE;
	STDMETHOD(ContextMenu) (THIS_ LPCEXPLORERINFO lped) PURE;
	STDMETHOD(Notify) (THIS_ LPCEXPLORERINFO lped, UINT msg, WPARAM wp, LPARAM lp) PURE;
};

typedef IShellListView * LPSHELLLISTVIEW;

//
// Borwser Object DLL's must implement this function instead of the OLE
// function DllGetClassObject. The key differance between these two API's
// is that this API returns an object instance instead of an instance of
// the object's class factory.
//
typedef HRESULT (STDAPICALLTYPE * PFNDLLCREATEBROWSEROBJECT) (REFCLSID, REFIID, LPVOID *);
typedef PFNDLLCREATEBROWSEROBJECT LPFNDLLCREATEBROWSEROBJECT;

STDAPI DllCreateBrowserObject(REFCLSID rclsid, REFIID riid, LPVOID *ppvObj);

//
//  Helper function which loads a Browser object DLL and creates
//  an instance of a Browser object, this object needs to support the
//  interfaces IShellListView and IShellFolder
//

WINSHELLAPI
HRESULT
WINAPI
SHCreateBrowserObject(REFCLSID rclsid,
					  REFIID riid,
					  LPVOID *ppv);

//-------------------------------------------------------------------------
//
// IShellCommandUI interface  /* WinCE Only Interface ! */
//
// -- This is used to replace the fact that we don't have
//    proper OLE menu merging support on Windows CE.
//
//-------------------------------------------------------------------------
#if defined(_OLEAUTO_H_)

#undef 	INTERFACE
#define	INTERFACE 	IShellCommandUI

#define SCUIGA_STOP           0x00000100L
#define SCUIGA_REFRESH        0x00000200L
#define SCUIGA_STARTPAGE      0x00000400L
#define SCUIGA_SEARCHPAGE     0x00000800L

// tell the browser to ask the user if [s]he wants to add a link to the
// Follow-Up Links folder
// (LPCTSTR)WPARAM = pointer to the URL string
// (LPCTSTR)LPARAM = pointer to the post data string
// if return value is 0, msg not handled - control should put up its own message
// if return value is 1, user added this URL to follow-up links
// if return value is -1, user did not add this URL to follow-up links
#define WM_FOLLOWUPLINK       WM_APP+1000

// tell a window that a combo box (that is its child) would like some
// suggestions to complete text entered in the edit control (i.e. besides
// completions based on prefix-matching in the combo box's list box)
// (HWND)WPARAM = window handle of the edit control of the combo box
// (HWND)LPARAM = window handle of the list box control in which to add strings
#define WM_COMBOCOMPLETE      WM_APP+1001

// queries whether the owning window wants to restrict the command (wparam)
// the wparams are defined as the FCIDM_ command set.  return 0 for no, return 1 for restricted
#define WM_RESTRICTEDCOMMAND    (WM_APP + 1002)

DECLARE_INTERFACE_(IShellCommandUI, IUnknown)
{						
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

	// *** IShellCommandUI methods ***
	STDMETHOD(GetAttributes) (THIS_ ULONG *rgfInOut) PURE;
	STDMETHOD(CommandUIActivate) (THIS_ HWND hwndToolbar, HMENU hmenu, int cmdFirst,
								  int cmdLast) PURE;
	STDMETHOD(CommandUIDeactivate) (THIS_ HWND hwndToolbar, HMENU hmenu) PURE;
    STDMETHOD(Navigate)(THIS_ BSTR URL, VARIANT FAR* Flags, VARIANT FAR* TargetFrameName, VARIANT FAR* PostData, VARIANT FAR* Headers) PURE;
    STDMETHOD(Refresh)(THIS) PURE;
    STDMETHOD(Stop)(THIS) PURE;
	STDMETHOD(HandleCommand) (THIS_ UINT msg, WPARAM wp, LPARAM lp, BOOL *rgfHandled) PURE;
};

#define WM_HANDLETAB		(WM_USER+71)

typedef IShellCommandUI * LPSHELLCOMMANDUI;

#endif //defined(_OLEAUT32_)

/////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif

#endif /* __WCESHELL_H__ */
