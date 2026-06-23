/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1997-2000 Microsoft Corporation.  All rights reserved.

File:

	startui.h

Abstract:  

This header file defines the "startui" interface.  The startui component 
implements the user interface of the startup dialog, i.e., the dialog that 
appears when the user wakes up the device.  This may be a password 
request, owner information, etc.  It should not be confused with the 
Welcome application which hand and palm PC's display when cold booted.  

The startui component is driven by the "startup" component.  The startup 
component is not replaceable but it is removeable if you do not want any 
startup dialog at all.  

The startui component implements the following:

	Startup_Initialize
	Startup_DlgProc
	Startup_WantStartupScreen
	Startup_PowerOnNotification
	Startup_PowerOffWhileStartupActiveNotification

The startup component implements the following:

	Startup_DestroyDialogCallback


--*/



#ifdef __cplusplus
extern "C" {
#endif

/*++

	IDD_STARTUP_DIALOG:

The resource identifier for the startup dialog.


--*/
#define IDD_STARTUP_DIALOG	0x200


/*++

	Startup_Initialize:

Called only once.  It gives the startui component a chance to do any 
initialization it needs to do.  It also tells the startui component what 
HINSTANCE it can use to load resources from GWE.  


--*/
void
Startup_Initialize(
	HINSTANCE hinst
	);


/*++

Startup_DlgProc

The dialog procedure for the startup dialog.


--*/
BOOL
Startup_DlgProc(
	HWND	hDlg,
	UINT	message,
	WPARAM	wParam,
	LPARAM	lParam
	);




/*++

	Startup_WantStartupScreen:


Called to find out if the startup screen should be displayed.  Returns 
TRUE if the startup dialog should appear and FALSE otherwise.  This 
function usually reads registry settings controlled by the user to decide 
whether a startup dialog should appear.  

Note that this function may actually be called when powering off so that the 
system can display the startup screen faster at power on.  


--*/
BOOL
Startup_WantStartupScreen(
	void
	);


/*++

	Startup_PowerOnNotification:

Called when the device is powered on.  This is simply a notification and 
this function is not required to do anything.  

This is running in a normal thread context so posting messages, etc., is 
OK.  


--*/
void
Startup_PowerOnNotification(
	HWND hwndDlg
	);


/*++

	Startup_PowerOffWhileStartupActiveNotification:

Called when power is being turned off while the startup dialog is active 
(visible).  This notification gives the dialog a chance to hide any 
controls which are displaying data that will be stale when the power comes 
back on.  This function is simply a notification and this function is not 
required to do anything.  


--*/
void
Startup_PowerOffWhileStartupActiveNotification(
	HWND hwndDlg
	);


/*++

	Startup_DestroyDialogCallback:

This function is NOT implemented by the startui component.  Rather, it is 
a function which the startui component must call instead of calling 
DestoyWindow when it is ready to bring down its dialog.  


--*/
void
Startup_DestroyDialogCallback(
	void
	);


#ifdef __cplusplus
}
#endif
