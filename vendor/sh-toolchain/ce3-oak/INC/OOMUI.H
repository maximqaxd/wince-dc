/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1997-2000 Microsoft Corporation.  All rights reserved.

Component Name:

oomui.h

Abstract:  

This header file defines the interface to the GWE component "oomui".
This component implements the user interface of the out-of-memory dialog, i.e.,
the dialog that appears when the device is nearly out of memory.

The "oomui" component works in conjunction with the "oom" component.  The
"oom" component is not replaceable, but it is removeable if you do not want 
any OOM dialog at all.

The OOM UI is based on two immortal windows: the OOM window and the Not
Responding window.  The OOM window is the window that gives the user of choice
of how to free up memory, for example, a list of windows that can be closed. The
Not Responding window is the window that comes up when a window cannot be closed
either because it is disabled or because it did not respond to the WM_CLOSE
message.  The Not Responding window is like a message box that can return one of
three values (via PostQuitMessage): REPLY_ENDTASK (forcibly terminate the task),
REPLY_CHOOSE (let the user choose another way to free memory, i.e., return to
the OOM Window), or REPLY_WAIT (continue to wait for the window to close).

Because these windows are immortal, the oomui component should never destroy
them. It should also never hide them, because the oom component is responsible
for this. When the user is done with one of these windows (e.g. when he presses
OK), the window procedure should simply call PostQuitMessage.

The OOM window is given information about the windows which are candidates to be
closed.  This information is passed in as an array of WINDOWINFO structures to
OomUI_SetWindowsInfo.  The first field is the hwnd.  The second field is the
name of the window.  (The OOM UI component cannot call GetWindowText because
this would send a window message to the window, and all other windows are frozen
when the OOM window is up.  The third field is fToBeClosed.  The oomui component
should set this field to TRUE if the user wants this window to be (politely)
closed. In this case, the oomui component should also call OomUI_CloseWindow on
the window. The fourth field is fToBeTerminated, which the oomui component
should set to TRUE if the window's process should be forcibly terminated.

--*/

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

// Possible replies from the Not Responding window (via PostQuitMessage)
typedef enum 
{ 
	REPLY_ENDTASK, 
	REPLY_CHOOSE, 
	REPLY_WAIT 
} NOTRESPONDINGREPLY;


typedef struct
{
	HWND   hwnd;				// handle to the window, not to be modified
	LPCTSTR szWindowName;   	// the window's title, not to be modified
	UINT32 fToBeClosed	  : 1; 	// Should this window be closed?
	UINT32 fToBeTerminated: 1; 	// Should this window be forcibly terminated?
} WINDOWINFO;

/* OomUI_CreateOomWindow() creates and returns the OOM window.  The OOM window
is immortal.  The oomui component should not destroy or hide this window. 
*/
HWND OomUI_CreateOomWindow (void);

/* OomUI_CreateNotRespondingWindow creates and returns the Not responding
window.   The oomui component should not destroy or hide this window. 
*/
HWND OomUI_CreateNotRespondingWindow (void);

/* OomUI_WndProc is the window procedure for the OOM window */
LRESULT CALLBACK OomUI_OomWndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

/* OomUI_NotRespondingWndProc is the window procedure for the Not Responding
window. */
LRESULT CALLBACK OomUI_NotRespondingWndProc (HWND hwnd, UINT message, 
											 WPARAM wParam, LPARAM lParam);

/* OomUI_SetWindowsInfo provides the oomui component with information about the
windows that are candidates to be closed.  cWindows is the number of entries
in the rgwi array.
*/
void OomUI_SetWindowsInfo (int cWindows, WINDOWINFO *rgwi);

/* OomUI_Initialize is called only once.  It gives the oomui component a chance
to do whatever initialization it needs to do.  It also informs the oomui 
component of the HINSTANCE it can use to load resources.
*/
void OomUI_Initialize (HINSTANCE hinst);

/* OomUI_OnShow is called just before the OOM window is about to be shown.
This function is not required to do anything, but it may use this opportunity
to set up the information the OOM window will display. */
void OomUI_OnShow (void);

/* OomUI_FShowOomWindow is called when the system determines the OOM window
should be shown.  This function gives the oomui component a chance to prevent 
the OOM window from coming up (by returning FALSE).  This is discouraged; 
however, it may be necessary if there are no options open to the user for 
freeing memory.
*/
BOOL OomUI_FShowOomWindow (void);

/* OomUICallback_NonClientPaint can be called by the oomui component to paint 
its nonclient area in the "active" colors. 
*/
void OomUICallback_NonClientPaint (HWND hwnd);

/* OomUICallback_CloseWindow is called by the oomui component when it wants the
system to attempt to close a window (via WM_CLOSE).  If this function returns
FALSE, the window could not be sent a WM_CLOSE.  If it returns TRUE, it was
sent a WM_CLOSE.  Note that this does not indicate whether the window actually
closed or not.  The oomui component does not need to worry about that.
*/
BOOL OomUICallback_CloseWindow (WINDOWINFO *pwi);

/* OomUICallback_IsCritical can be called by the oomui component to determine
if memory is critically low, as opposed to simply low.
*/
BOOL OomUICallback_IsCritical (void);


#ifdef __cplusplus
}
#endif
