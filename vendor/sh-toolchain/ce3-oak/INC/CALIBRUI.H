/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1997-2000 Microsoft Corporation.  All rights reserved.

File:

	CalibrUi.h


Abstract:  

This header file defines the "calibrui" interface.  The calibrui component 
implements the user interface for the calibration screen.  The system code 
which controls calibration steps through a number of states and calls 
functions in the calibrui component to perform most of the drawing 
operations.  This allows customizing the visuals for a particular class of 
devices.  

The calibrui component implements the following:

	TouchCalibrateUI_Initialize
	TouchCalibrateUI_DrawMainScreen
	TouchCalibrateUI_DrawConfirmationScreen
	TouchCalibrateUI_HandleUserInputMessage
	TouchCalibrateUI_WaitForConfirmation


The system implements the following:

	TouchCalibrateUI_Done:


--*/

#ifdef __cplusplus
extern "C" {
#endif

typedef enum 
{
	TCS_UNSTARTED, 
	TCS_STARTED, 
	TCS_CONFIRMING, 
	TCS_ACCEPTED,
	TCS_REJECTED
} TCS, TOUCH_CALIBRATE_STATE;


/*++

	TouchCalibrateUI_Initialize:

Called exactly once.  It gives the calibrui component a chance to do 
whatever initialization is necessary.  It also informs the component of 
the address of the TouchCalibrate_State variable.  This variable can be 
inspected to determine what state the calibration process is currently in.  
It also informs the component of the current HINSTANCE, which it can use 
to load resources.  


--*/
void
TouchCalibrateUI_Initialize(
	TOUCH_CALIBRATE_STATE	*ptcs,
	HINSTANCE				hinst
	);


/*++

	TouchCalibrateUI_DrawMainScreen:

Called to draw the calibration main screen.  This routine should draw on 
the given hdc whatever UI is desired for the calibration screen, such as 
strings telling the user what to do.  


--*/
void
TouchCalibrateUI_DrawMainScreen(
	HDC	hdc
	);


/*++

	TouchCalibrateUI_DrawConfirmationScreen:

Called to draw the calibration confirmation screen This routine should 
draw on the given hdc whatever UI is desired for the confirmation screen.  
The confirmation screen appears after the user is done pressing on the 
calibration targets but before the new setting have been saved.  


--*/
void
TouchCalibrateUI_DrawConfirmationScreen(
	HDC	hdc
	);


/*++

	TouchCalibrateUI_HandleUserInputMessage:

Called whenever the user presses a key or touches the screen during 
calibration.  Usually this function ignores all input until 
TouchCalibrate_State == TCS_CONFIRMING.  This function is responsible for 
calling TouchCalibrateUI_Done() when the user does whatever action is 
required to accept or reject the calibration.  


--*/
void
TouchCalibrateUI_HandleUserInputMessage(
	UINT	Message,
	UINT	wParam,
	UINT	lParam
	);




/*++

	TouchCalibrateUI_WaitForConfirmation:

Called while the confirmation screen is up and the system is waiting for 
the user to accept or reject the calibration.  The simplest correct 
implementation of this function is: WaitForSingleObject(hevt, INFINITE); 
but more complex implementations, for example, ones involving timeouts, 
are possible.  The hdc passed in is the hdc of the confirmation screen.

Returning without setting the touch calibrate state to TCS_ACCEPTED is
treated as a rejection.


--*/
void
TouchCalibrateUI_WaitForConfirmation(
	HANDLE	hevt,
	HDC hdc
	);


/*++

	TouchCalibrateUI_Done:

This function is NOT implemented by the calibrui component.  The calibrui 
component must call this function when the user has done whatever action 
is required to either accept or reject a completed calibration.  The value 
of TouchCalibrationState should be either TCS_REJECTED or TCS_ACCEPTED.  


--*/
void
TouchCalibrateUI_Done(
	TOUCH_CALIBRATE_STATE	tcs
	);


#ifdef __cplusplus
}
#endif
