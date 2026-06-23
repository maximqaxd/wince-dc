/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

Module Name:  

keybdpdd.h

Abstract:  

Interface to the PDD layer of the keyboard interface.

Notes: 

The keyboard model device driver (MDD) uses this standard PDD interface to 
provide keyboard functionality.

--*/

#ifdef __cplusplus
extern "C" {
#endif

extern const DWORD gIntrKeyboard;

//      @DOC    EXTERNAL DRIVERS KEYBD_DRIVER
/*      @FUNC   BOOL |
				KeybdPdd_DllEntry |
				Called by the MDD when the keyboard driver is loaded.

		@PARM	HANDLE	| hinstDLL
		@PARM	DWORD	| Op
		@PARM	LPVOID	| lpvReserved

		@RDESC	Returns 0 to fail the driver load; non-zero for success.

*/
BOOL
WINAPI
KeybdPdd_DllEntry(
	HANDLE  hinstDLL,
	DWORD   Op,
	LPVOID  lpvReserved
	);





//      @DOC    EXTERNAL DRIVERS KEYBD_DRIVER
/*      @FUNC   INT	|
				KeybdPdd_GetEvent |
				Called by the MDD to get key events from the PDD layer.

        @PARM   UINT32			| VirtualKeys[16]
        @PARM   KEY_STATE_FLAGS	| KeyEvents[16]

		@RDESC	Returns the number of key events.

		@COMM

The PDD can return up to 16 key events.

		@COMM

If the keyboard needs to be scanned periodically, the MDD should be 
awakened periodically by signalling the keyboard interrupt.  This 
mechanism should not be allowed to run freely however since the driver 
thread will use all available processor time.  

		@COMM

0 is a valid return value if no new key events are detected.

*/
INT
WINAPI
KeybdPdd_GetEvent(
	UINT32			VirtualKeys[16],
	KEY_STATE_FLAGS	KeyEvents[16]
	);

INT
WINAPI
KeybdPdd_GetEventEx(
	UINT32			VirtualKeys[16],
	UINT32			ScanCodes[16],
	KEY_STATE_FLAGS	KeyEvents[16]
	);



//      @DOC    EXTERNAL DRIVERS KEYBD_DRIVER
/*      @FUNC   void |
				KeybdPdd_PowerHandler |
				Called by the MDD when the device is powering up or down

		@PARM	BOOL	bOff | non-zero if the device is powering off, 0 if powering on.

		@COMM

This function is called in a kernel context and may not make any system 
call or cause any kind of rescheduling.  It is pretty much limited to 
accessing its own memory.  

*/
void
WINAPI
KeybdPdd_PowerHandler(
	BOOL	bOff
	);



BOOL
WINAPI
KeybdPdd_InitializeDriver(
	PFN_KEYBD_EVENT_CALLBACK	pfnKeybdEventCallback
	);

BOOL
WINAPI
KeybdPdd_InitializeDriverEx(
	PFN_KEYBD_EVENT_CALLBACK_EX	pfnKeybdEventCallbackEx
	);

void
WINAPI
KeybdPdd_ToggleKeyNotification(
	KEY_STATE_FLAGS	KeyStateFlags
	);

KEY_STATE_FLAGS
WINAPI
KeybdPdd_KeyStateToggled(
	void
	);


#ifdef __cplusplus
}
#endif

