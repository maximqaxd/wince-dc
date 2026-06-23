//***************************************************************************
//
//  Copyright (c) 1997-1999  Microsoft Corporation.  All Rights Reserved.
//
//  File:
//          inkx.h
//
//  Description:
//
//          Include file for WinCE Ink Control.
//
//***************************************************************************

#ifndef _INKX_INCLUDED_
#define _INKX_INCLUDED_

#include <windows.h>
#include <commctrl.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


// Constants
#define WC_INKXA   "InkX"
#define WC_INKXW  L"InkX"
#ifdef UNICODE
#define WC_INKX    WC_INKXW
#else
#define WC_INKX    WC_INKXA
#endif

// By default the InkX control has a VoiceBar that appears at the top of the control.
// This can be changed by using the following window styles.  Do not OR these
// styles together.
//
#define IS_NOVOICEBAR            0x0200    // Don't create a VoiceBar
#define IS_BOTTOMVOICEBAR        0x0400    // VoiceBar appears at bottom of the control

// Application calls this function to load and initialize the control properly.
// The control can be inserted as a dialog custom control with class setting "InkX"
//  (see constant WC_INKX above).
void InitInkX(void);

//---------------------------------------------------------
// IM_SHOWCMDBAR
//
// NOTE:  THIS MESSAGE IS NO LONGER SUPPORTED.
//
// Use APIs in RichInk.h to implement a command bar UI.
//---------------------------------------------------------


//---------------------------------------------------------
// IM_GETDATALEN
//
// Used to return the length of the data.
//
// wParam   = 0;    // not used, must be zero
// lParam   = 0;    // not used, must be zero 
//
// Return Values:
//  This message returns the length in bytes of the ink data
//  in the main compose window. Application uses this value
//  to allocate buffer to store the ink data.
//---------------------------------------------------------
#define IM_GETDATALEN   (WM_USER + 514)

//---------------------------------------------------------
// IM_GETDATA
//
// Used to retrieve the data
//
// wParam   = (INT)cbBuffer;    // Length of the buffer in bytes.
// lParam   = (BYTE *)lpBuffer; // Pointer to the buffer 
//
// Return Value:
//  This message returns the size of ink data stored in bytes.
//---------------------------------------------------------
#define IM_GETDATA      (WM_USER + 515)

//---------------------------------------------------------
// IM_SETDATA
//
// Used to set the data.  Stores the Ink data from a previous
// IM_GETDATA call which will be inserted into the current
// compose window.
//
// wParam   = (INT)cbInkData;       // Length of the ink data buffer
// lParam   = (BYTE *)lpInkData;    // Pointer to ink data buffer 
//
// Return Value:
//  This message does not return a value. 
//---------------------------------------------------------
#define IM_SETDATA      (WM_USER + 516)

//---------------------------------------------------------
// IM_CLEARALL
//
// Used to erase all contents from the current compose window.
//
// wParam   = 0;    // not used, must be zero
// lParam   = 0;    // not used, must be zero 
//
// Return Value:
//  This message does not return a value. 
//---------------------------------------------------------
#define IM_CLEARALL     (WM_USER + 519)

//---------------------------------------------------------
// IM_REINIT
//
// Now the same as IM_CLEARALL -- constant is provided here
//  for compatibility with previous header files.
//---------------------------------------------------------
#define IM_REINIT       (WM_USER + 521)

//---------------------------------------------------------
// IM_GETRICHINK
//
// Used to get the window handle to the RichInk control
//
// wParam   = 0;    // reserved, must be zero
// lParam   = 0;    // reserved, must be zero
//
// Return Value:
//  This message returns the window handle to the 
//  RichInk control.
//---------------------------------------------------------
#define IM_GETRICHINK   (WM_USER + 532)

//---------------------------------------------------------
//
// NOTE:  Many of the standard EM_* messages are supported
//        by passing them through to the RichInk control.
//        See the RichInk.h header file for a specific list
//        of messages that are supported.
//
//---------------------------------------------------------



//_____________________________________________________________________________
//
// VoiceBar messages                (also see the szHotRecording message)
//_____________________________________________________________________________


//_____________________________________________________________________________
//
// IM_VOICEBAR
//
// Used to show or hide the VoiceBar.
//
// wParam   = TRUE or FALSE;    // TRUE shows the VoiceBar,
//                              // FALSE hides the VoiceBar
// lParam   = 0;                // reserved, must be zero
//
// Return Value:
//  This message does not return a value. 
//
#define IM_VOICEBAR     (WM_USER + 530)

//_____________________________________________________________________________
//
// IM_VOICE_STOP
//
// Causes the voicebar to halt playback or recording (if either is in progress)
//
// wParam   = 0;                // reserved, must be zero
// lParam   = 0;                // reserved, must be zero
//
// Return Value:
//  This message does not return a value. 
//
#define IM_VOICE_STOP   (WM_USER + 537)

//_____________________________________________________________________________
//
// IM_VOICE_PLAY
//
// If an embedded voice object is currently selected, this will force a replay
// of that selected object.  If playback or recording is currently in progress,
// it is halted, first.  Note that under some conditions (selecting an object
// by tapping it, for example) the act of selection will automatically trigger
// playback, without requiring this function.
//
// wParam   = 0;                // reserved, must be zero
// lParam   = 0;                // reserved, must be zero
//
// Return Value:
//  This message does not return a value. 
//
#define IM_VOICE_PLAY   (WM_USER + 540)

//_____________________________________________________________________________
//
// IM_VOICE_RECORD
//
// Playback and recording must currently be stopped (it's a good idea to send
// IM_VOICE_STOP first).  This will begin recording to an inline embedded
// object.  The recording may be stopped by pressing the appropriate button on
// the voice bar, exiting the ink control, or by sending IM_VOICE_STOP.
//
// wParam   = 0;    // reserved, must be zero
// lParam   = 0;    // reserved, must be zero
//
// Return Value:
//   TRUE           recording began successfully
//   FALSE          error, recording has not been started
//
#define IM_VOICE_RECORD (WM_USER + 541)

//_____________________________________________________________________________
//
// IM_RECORDING
//
// Returns TRUE if we're currently recording
//
// wParam   = 0;    // reserved, must be zero
// lParam   = 0;    // reserved, must be zero
//
// Return Value:
//   TRUE           we're currently recording
//   FALSE          we are not recording
//
#define IM_RECORDING    (WM_USER + 536)

//_____________________________________________________________________________
//
// IM_VOICE_PLAYING
//
// Returns TRUE if we're currently playing voice data
//
// wParam   = 0;    // reserved, must be zero
// lParam   = 0;    // reserved, must be zero
//
// Return Value:
//   TRUE           we're currently playing back sound
//   FALSE          we are not playing
//
#define IM_VOICE_PLAYING (WM_USER + 542)


//_____________________________________________________________________________
//
// Defined windows messages
//_____________________________________________________________________________


//_____________________________________________________________________________
//
// szHotRecording
//
// This message is sent to the top-level window whenever the voice hotkey is
// pressed.  If the top-level window cannot handle voice recordings, it should
// return the normal default value of 0 (in which case, we'll record a file,
// and perhaps switch to the "Notes" application).  If the top-level app IS
// able to handle voice, then the non-zero return value will specify
// information such as where to place the data (in main object memory, on a
// storage card, etc.) and the maximum allowed size for the recording.
//
// Rich Ink controls (inkx.h) may be created with embedded voice capabilities.
// To exploit this and support automatic hot-key recording in a rich ink
// control, the top-level window must pass this message on to the appropriate
// control.  Here is code to accomplish that:
//
// First, this messages needs to be registered, as with:
//
//     UINT ulHotRecording = RegisterWindowMessage(szHotRecording);
//
// Then in the default section of WinProc, the registered message should be
// detected and passed through to the rich ink control:
//
//     if (message == ulHotRecording)
//     {
//         return_value = SendMessage (hWndInkX, message, 0, 0);
//         if (return_value != 0)
//         {
//             // optional code to show the ink control's voice bar:
//             SendMessage(hWndInkX, IM_VOICEBAR, TRUE, 0)
//         }
//     }
//     ...
//     return (return_value);
//
// WinProc must return either zero or the exact return code from InkX.
// Return Value from WinProc:
//
//      0    This window cannot handle voice data.  Record voice to a file,
//           and switch to NOTES if appropriate.
//
//   (other) This window CAN handle voice data.  This return value specifies
//           the maximum allowed data size, where to store data, etc.
//
// This message may be redundantly defined in several include files.
#ifndef  szHotRecording
#define  szHotRecording  TEXT("WM_HOTRECORDING")
#endif //szHotRecording


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _INKX_INCLUDED_
