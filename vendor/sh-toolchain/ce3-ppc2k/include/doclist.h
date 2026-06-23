//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
//
//				Copyright (c) 1998  Microsoft Corporation
//
//	Module:	DocList.h
//
//	Description:
//
//		Header for DocList exported functions
//
//	Author:  Kevin Paulson
//
// History:
//
//	1/28/98  a-kevpau	Created
//
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

#ifndef __DOCLIST_H__
#define __DOCLIST_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tagDLC 
{
    DWORD       dwStructSize;   // size of DOCLISTCREATE structure
    HWND        hwndParent;     // handle to parent
    LPCTSTR     pszFolder;      // initial folder to display
    LPCTSTR     pstrFilter;     // pointer to filter string
    WORD        wFilterIndex;   // index into filter string for initial filter
    WORD        wId;            // id of DocList control
    DWORD       dwFlags;        // creation flags
} DOCLISTCREATE, * PDOCLISTCREATE;

//___________________________________________________________________________
//
// DocList_Create
//
// Creates the DocList window
//
// RETURNS:
//
//		HWND
//
HWND DocList_Create(PDOCLISTCREATE pDLC);

//___________________________________________________________________________
//
// DocList_GetNextSelectedWaveFile
//
// Gets the next selected wave file
//
// RETURNS:
//
//		BOOL
//
BOOL DocList_GetNextSelectedWaveFile(HWND hwnd, int* pIndex, LPTSTR szPath, const size_t cchPath);

//___________________________________________________________________________
//
// DocList_GetFirstSelectedWaveFile
//
// Gets the first selected wave file
//
// RETURNS:
//
//		BOOL
//
BOOL DocList_GetFirstSelectedWaveFile(HWND hwnd, int* pIndex, LPTSTR szPath, const size_t cchPath);

// notification structure used by DocList
//
typedef struct tagDLNHDR
{
	NMHDR	nmhdr;
    int     iItem;
	LPCTSTR	pszPath;
} DLNHDR, *PDLNHDR;

//___________________________________________________________________________
//
// Messages sent from application to DocList
//

// Sent when application want the listview to be refreshed
// wParam: not used
// lParam: not used
// Returns: HRESULT
#define DLM_REFRESH         (WM_USER + 600)

// Sent when the file filter is changed
// (e.g. user changes settings in an Options dialog)
// wParam: index of filter
// lParam: not used
// Returns: HRESULT
#define DLM_SETFILTERINDEX  (WM_USER + 601)

// Sent when application want the listview to be refreshed
// but doesn't want selection to be restored
// wParam: not used
// lParam: not used
// Returns: HRESULT
#define DLM_UPDATE          (WM_USER + 602)

// Sent when application doesn't want the listview to update
// wParam: not used
// lParam: not used
// Returns: nothing
#define DLM_DISABLEUPDATES  (WM_USER + 603)

// Sent when application wants the listview to update
// wParam: not used
// lParam: not used
// Returns: nothing
#define DLM_ENABLEUPDATES   (WM_USER + 604)

// Sent when application wants the listview to be resorted
// wParam: not used
// lParam: not used
// Returns: TRUE sorting successful
#define DLM_SETSORTORDER    (WM_USER + 605)

// Sent when application wants the pathname associated 
// with the selected item. 
// wParam: size of buffer
// lParam: points to buffer, wParam is size of buffer
// Returns: TRUE if item exists
#define DLM_GETSELPATHNAME  (WM_USER + 606)

// Sent when application wants to have the item
// associated with the pathname selected
// wParam: is a bool determining if the item should be visible
// lParam: points to buffer,
// Returns: HRESULT 
#define DLM_SELECTITEM      (WM_USER + 607)

// Sent when application wants to select an item by index
// wParam: index to select
// lParam: not used
// Returns: nothing 
#define DLM_SETSELECT       (WM_USER + 608)

// Sent when application wants to create an item
// containing the PA structure passed in
// wParam: is the index where item should be inserted
// lParam: points to the PA structure
// Returns: nothing 
#define DLM_SETONEITEM      (WM_USER + 609)

// Sent when application wants to select the next wave file
// wParam: not used
// lParam: pointer to integer set to the  current item or -1
//          to get the first wave file
// Returns: TRUE if there is a next wave file
#define DLM_GETNEXTWAVE     (WM_USER + 610)

// Sent when application wants to select the previous wave file
// wParam: not used
// lParam: pointer to integer set to the  current item or -1
//          to get the first wave file
// Returns: TRUE if there is a previous wave file
#define DLM_GETPREVWAVE     (WM_USER + 611)

// Sent when application wants to set a folder
// wParam: not used
// lParam: pointer to name of folder to set doclist to
// Returns: nothing
#define DLM_SETFOLDER       (WM_USER + 612)

// Sent when application wants to get the number of items selected
// wParam: not used
// lParam: not used
// Returns: number of items selected
#define DLM_GETSELCOUNT     (WM_USER + 613)

// Sent when application wants to get the current file filter selection
// such as when setting the current file filter in a combo box
// file filter index is returned from SendMessage
// wParam: not used
// lParam: not used
// Returns: filter index
#define DLM_GETFILTERINDEX  (WM_USER + 614)

// Sent when application wants the item associated with 
// the pathname selected during the next refresh
// wParam: not used
// lParam: points to pathname buffer
// Returns: nothing
#define DLM_SETSELPATHNAME  (WM_USER + 615)

// Sent when application wants to bring up the Rename/Move
// dialog for the selected items.
// wParam: not used
// lParam: not used
// Returns: TRUE if successful, 
//          FALSE if cancel out of dialog
#define DLM_RENAMEMOVE      (WM_USER + 616)

// Sent when application wants to send IR
// wParam: not used
// lParam: points to pathname buffer
// Returns: nothing
#define DLM_SENDIR          (WM_USER + 617)

// Sent when application wants to send e-mail
// wParam: not used
// lParam: points to pathname buffer
// Returns: nothing
#define DLM_SENDEMAIL       (WM_USER + 618)

// Sent when application wants to delete selected items
// wParam: not used
// lParam: not used
// Returns: HRESULT
#define DLM_DELETESEL       (WM_USER + 619)

// Sent when application wants to receive IR
// wParam: not used
// lParam: points to pathname buffer
// Returns: nothing
#define DLM_RECEIVEIR       (WM_USER + 620)

// Sent when application wants to get the count of items
// in the doclist
// wParam: not used
// lParam: not used
// Returns: number of items in doclist
#define DLM_GETITEMCOUNT    (WM_USER + 621)

// Sent when application wants to select items in the doc list
// in the doclist
// wParam: not used
// lParam: not used
// Returns: Select items in the doc list
#define DLM_SELECTALL       (WM_USER + 622)

// Sent when application wants doc list to check if current
// folder is valid
// wParam: not used
// lParam: not used
// Returns: 
//      TRUE if folder is valid, 
//      FALSE if folder isn't valid and update will occur
#define DLM_VALIDATEFOLDER  (WM_USER + 623)

// Sent when application wants to get the next item in the doclist
// wParam: (WPARAM) (int) iStart 
// lParam: MAKELPARAM((UINT) flags, 0)
// Returns: the index of the next item if successful, or -1 otherwise.
#define DLM_GETNEXTITEM     LVM_GETNEXTITEM

// Sent when application wants change the state of an item in the doclist
// wParam: (WPARAM) (int) i -- index of doclist item 
// lParam: (LPARAM) (LPLVITEM) pitem -- pointer to an LVITEM structure
// Returns: TRUE if successful
#define DLM_SETITEMSTATE    LVM_SETITEMSTATE

//___________________________________________________________________________
//
// Notifications sent from DocList to parent
//
#define DLN_START				11000

// Item has been clicked.  Pathname sent with notification.  
// Application should determine what to do with activation. 
// (e.g. play wave file, open document)
#define DLN_ITEMACTIVATED       (DLN_START)

// There aren't any files in list view so application should 
// update UI (e.g. disable property button)
#define DLN_NOITEMS             (DLN_START+1)	

// Escape key was pressed in list view.  Application may want 
// to take action (e.g. stop playback)
#define DLN_ESCAPE              (DLN_START+2)	

// VK_UP was pressed in list view.  Application may want to take 
// action (e.g. set selection on a previous wave file).  
// If the application wants to override the default behavior 
// it should return TRUE in response to this notification.
#define DLN_UP                  (DLN_START+3)	

// VK_DOWN was pressed in list view.  Application may want to take 
// action (e.g. set selection on next wave file).
// If the application wants to override the default behavior 
// it should return TRUE in response to this notification.
#define DLN_DOWN                (DLN_START+4)	

// Sent just before deleting one or more selections.  
// Allows application to abort deletion operation.
#define DLN_PREDELETESEL        (DLN_START+5)	

// Sent to application just before deleting a selection.  
// Pathname is sent with message header.  
// Allows application to properly close file if it is open.
#define DLN_DELETINGSEL         (DLN_START+6)	

// Item is changing in list view so application may want to take action 
// (e.g. stop playback of a wave file)
#define DLN_ITEMCHANGING        (DLN_START+7)	

// Item has changed in list view so application may want to take action 
// (e.g. playback the newly selected wave file)
#define DLN_ITEMCHANGED         (DLN_START+8)

// Sent when "Print..." is selected from the context menu.  The 
// pathname of the file to print is passed in.
#define DLN_PRINT               (DLN_START+9)	

// Sent when "Create Copy" is selected from the context menu.  The 
// pathname of the file is passed in.
#define DLN_COPY                (DLN_START+10)	

// Sent when "Properties" is selected from the context menu.  The 
// pathname of the file is passed in.
#define DLN_PROP                (DLN_START+11)	

// Sent when a new folder is selected.
// pathname of the file is passed in.
#define DLN_FOLDER              (DLN_START+12)	

// Sent when LVN_ITEMCHANGED is received and no item was selected
//
#define DLN_NOHIT               (DLN_START+13)  

// Sent just before the DocList is updated
// To abort the updating return TRUE
//
#define DLN_PREUPDATE           (DLN_START+14)  

// Sent just after the DocList is updated
//
#define DLN_POSTUPDATE          (DLN_START+15)  

// Sent when DocList receives WM_FILECHANGEINFO
// To abort processing the message return TRUE
//
#define DLN_FILECHANGEINFO      (DLN_START+16)  

// Flags for DocList
//
#define DLF_SHOWEXTENSION   0x0001

// Macro functions
//
#define DocList_Refresh(hwnd) \
    (HRESULT)SendMessage((hwnd), DLM_REFRESH, 0, 0L)

#define DocList_Update(hwnd) \
    (HRESULT)SendMessage((hwnd), DLM_UPDATE, 0, 0L)

#define DocList_DisableUpdate(hwnd) \
    (void)SendMessage((hwnd), DLM_DISABLEUPDATES, 0, 0L)

#define DocList_EnableUpdate(hwnd) \
    (void)SendMessage((hwnd), DLM_ENABLEUPDATES, 0, 0L)

#define DocList_SetSortOrder(hwnd) \
    (BOOL)SendMessage((hwnd), DLM_SETSORTORDER, 0, 0L)

#define DocList_SetFilterIndex(hwnd, index) \
    (HRESULT)SendMessage((hwnd), DLM_SETFILTERINDEX, (WPARAM)index, 0L)

#define DocList_GetFilterIndex(hwnd) \
    (int)SendMessage((hwnd), DLM_GETFILTERINDEX, 0, 0L)

#define DocList_GetSelectedPathname(hwnd, szPath, size) \
    (BOOL)SendMessage((hwnd), DLM_GETSELPATHNAME, (WPARAM)(size), (LPARAM)(szPath))

#define DocList_SetSelectedPathname(hwnd, szPath) \
    (void)SendMessage((hwnd), DLM_SETSELPATHNAME, 0, (LPARAM)(szPath))

#define DocList_SelectItem(hwnd, szPath, fVisible) \
    (HRESULT)SendMessage((hwnd), DLM_SELECTITEM, (WPARAM)(fVisible), (LPARAM)(szPath))

#define DocList_SetSelection(hwnd, item) \
    (void)SendMessage((hwnd), DLM_SETSELECT, (WPARAM)(item), 0L)

#define DocList_DeleteSelection(hwnd) \
    (HRESULT)SendMessage((hwnd), DLM_DELETESEL, 0, 0L)

#define DocList_SetOneItem(hwnd, item, pPA) \
    (HRESULT)SendMessage((hwnd), DLM_SETONEITEM, (WPARAM)(item), (LPARAM)(pPA))

#define DocList_SetFolder(hwnd, szFolder) \
    (void)SendMessage((hwnd), DLM_SETFOLDER, 0, (LPARAM)(szFolder))

#define DocList_GetSelectCount(hwnd) \
    (int)SendMessage((hwnd), DLM_GETSELCOUNT, 0, 0L)

#define DocList_GetNextWaveFile(hwnd, pitem) \
    (BOOL)SendMessage((hwnd), DLM_GETNEXTWAVE, 0, (LPARAM)(pitem))

#define DocList_GetPrevWaveFile(hwnd, pitem) \
    (BOOL)SendMessage((hwnd), DLM_GETPREVWAVE, 0, (LPARAM)(pitem))

#define DocList_RenameMoveSelectedItems(hwnd) \
    (BOOL)SendMessage((hwnd), DLM_RENAMEMOVE, 0, 0L)

#define DocList_SendIR(hwnd, szPath) \
    (void)SendMessage((hwnd), DLM_SENDIR, 0, (LPARAM)(szPath))

#define DocList_ReceiveIR(hwnd, szPath) \
    (void)SendMessage((hwnd), DLM_RECEIVEIR, 0, (LPARAM)(szPath))

#define DocList_SendEMail(hwnd, szPath) \
    (void)SendMessage((hwnd), DLM_SENDEMAIL, 0, (LPARAM)(szPath))

#define DocList_GetItemCount(hwnd) \
    (int)SendMessage((hwnd), DLM_GETITEMCOUNT, 0, 0L)

#define DocList_SelectAll(hwnd) \
    (void)SendMessage((hwnd), DLM_SELECTALL, 0, 0L)

#define DocList_ValidateFolder(hwnd) \
    (BOOL)SendMessage((hwnd), DLM_VALIDATEFOLDER, 0, 0L)

#define DocList_GetNextItem(hwnd, i, flags) \
    (int)SendMessage((hwnd), DLM_GETNEXTITEM, (WPARAM)(int)(i), MAKELPARAM((flags), 0))

#define DocList_SetItemState(hwnd, i, data, mask) \
{ LV_ITEM _ms_lvi;\
  _ms_lvi.stateMask = mask;\
  _ms_lvi.state = data;\
  SendMessage((hwnd), DLM_SETITEMSTATE, (WPARAM)i, (LPARAM)(LV_ITEM FAR *)&_ms_lvi);\
}

#ifdef __cplusplus
}
#endif

#endif // __DOCLIST_H__
