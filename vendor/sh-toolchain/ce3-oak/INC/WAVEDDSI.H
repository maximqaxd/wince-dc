//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//  Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.
//  
// -----------------------------------------------------------------------------
//
// @doc     WPDD_EXT
//
// @topic   Designing a Waveform Audio PDD |
//
//          The DDSI defines the interface between the machine 
//          independent component (MDD) and the machine dependent 
//          component (PDD).  The DDSI abstracts the physical operation of 
//          the audio device to a virtual model independent of the physical 
//          device operation.  This abstraction insulates the machine 
//          dependent component from the machine independent component, 
//          meeting the design goal of encapsulation.  Additionally, the 
//          DDSI has minimal operating system dependencies,  simplifying 
//          the machine dependent component and allowing it to be 
//          hardware-centric not operating system-centric.  The physical 
//          implementation of the audio device is left to the platform 
//          developer and need not follow the virtual model, as long as the 
//          platform developer supplies the software necessary for converting 
//          the physical implementation to the abstracted model.
//
//          The DDSI provides support for enabling and disabling the device,
//          device configuration, and transferring wave data to the device.
//
//          Implementation of the DDSI functionality resides in the PDD.
//
//          The PDD (Platform Dependent Driver) is responsible for 
//          communicating with the audio circuit to start and stop playback
//          and/or recording and initialize and deinitialize the circuits.
//
//          If audio device circuitry uses more that one hardware interrupt
//          (for example, one for input and one for output), they should
//          be multiplexed in the Harware Abstraction Layer's (HAL)
//          Interrupt Service Routines (ISR) to both return SYSINTR_AUDIO.
//          When <f PDD_AudioGetInterruptType> is called to determine the
//          cause of the interrupt, the function can demupltiplex and report
//          the playback or record status. If this mechanism is not suitable 
//          for the audio device architecture, the supplied MDD will need 
//          to be modified to handle multiple virtual interrupts and multiple
//          interrupt handling threads.
//
// @xref                                          <nl>
//          <t Designing a Waveform Audio Driver> <nl>
//          <f PDD_AudioGetInterruptType>,        <nl>
//          <f PDD_AudioInitialize>,              <nl>
//          <f PDD_AudioDeinitialize>,            <nl>
//          <f PDD_AudioPowerHandler>,            <nl>
//          <f PDD_AudioMessage>,                 <nl>
//          <f PDD_WaveProc>                      <nl>
//
// -----------------------------------------------------------------------------
#ifndef __AUDIDDSI_H
#define __AUDIDDSI_H


#ifdef __cplusplus
extern "C" {
#endif

// -----------------------------------------------------------------------------
//
//  @doc    WPDD_EXT
//
//  @const   gIntrAudio | Audio Interrupt ID (i.e. SYSINTR_AUDIO). Declared and initialized by PDD. 
// -----------------------------------------------------------------------------
extern DWORD gIntrAudio;

// -----------------------------------------------------------------------------
//
//  @doc    WPDD_EXT
//
//  @type   AUDIO_STATE | State of the audio device. The value can represent
//          a state change of either the input or output circuit or both. The 
//          lower nibble represents the input status, and the upper nibble 
//          represents the output status, so if the current interrupt is for 
//          both input and output, the values can be OR'ed together. However,
//          if the interrupt only represents one circuit, the other nibble
//          should remain zero, otherwise, the MDD will interpret this as
//          a change or update in the circuit's status.
//
//  @flag   AUDIO_STATE_OUT_PLAYING | Sound is currently playing.
//
//  @flag   AUDIO_STATE_OUT_UNDERFLOW | Sound was playing but not 
//          not enough data has been supplied to continue.
//
//  @flag   AUDIO_STATE_OUT_STOPPED | Playback has been stopped.
//
//  @flag   AUDIO_STATE_IN_RECORDING | Sound is currently recording.
//
//  @flag   AUDIO_STATE_IN_OVERFLOW | Sound was recording but the reserve
//          buffer was not processed in time before the DMA needed to write to 
//          it again.
//
//  @flag   AUDIO_STATE_IN_STOPPED | Recording has been stopped.
//
//  @flag   AUDIO_STATE_IGNORE | The interrupt does not represent a state
//          change or update and should be ignored by the MDD. This state can 
//          be used to report a PDD private interrupt (i.e. Initializing a 
//          CODEC may generate an interrupt that needn't be reported).
//
//  @flag   AUDIO_STATE_OUT_MASK | Bitmask for the output state (0xF0). 
//
//  @flag   AUDIO_STATE_IN_MASK | Bitmask for the input state (0x0F). 
//
// -----------------------------------------------------------------------------
typedef BYTE AUDIO_STATE;

#define AUDIO_STATE_IGNORE          0

#define AUDIO_STATE_IN_RECORDING    0x01
#define AUDIO_STATE_IN_OVERFLOW     0x02
#define AUDIO_STATE_IN_STOPPED      0x03
#define AUDIO_STATE_IN_MASK         0x0F

#define AUDIO_STATE_OUT_PLAYING     0x10
#define AUDIO_STATE_OUT_UNDERFLOW   0x20
#define AUDIO_STATE_OUT_STOPPED     0x30
#define AUDIO_STATE_OUT_MASK        0xF0


// -----------------------------------------------------------------------------
//
//  @doc    WPDD_EXT
//
//  @enum   WAPI_INOUT | Direction of the Wave API function. Designates 
//          if the function is for input or output.
//
//  @flag   WAPI_IN |  Function or data is for input.
//
//  @flag   WAPI_OUT | Function or data is for output.
//
// -----------------------------------------------------------------------------
typedef enum {
    WAPI_IN = 0, 
    WAPI_OUT
} WAPI_INOUT, *PWAPI_INOUT;




// ---------------------------------------------------------------------------
//
// @doc     WPDD_EXT
//
// @func    AUDIO_STATE | PDD_AudioGetInterruptType |
//          Determines the cause of the SYSINTR_AUDIO interrupt. The PDD
//          should report the status of the playback or record circuitry.
//          The interrupt should occur as a result of the audio DMA passing
//          a buffer boundary.  See the <t AUDIO_STATE> return codes for the
//          possible states that an interrupt can represent.
//
// @rdesc   Returns one of the <t AUDIO_STATE> values.
//
// ---------------------------------------------------------------------------
AUDIO_STATE 
PDD_AudioGetInterruptType (
    VOID
    );


//------------------------------------------------------------------------------
//
//  @doc    WPDD_EXT
//
//  @func   DWORD | PDD_AudioMessage | This function sends messages to the 
//          Audio PDD. Custom functions can be accessed via this routine.
//          This function is a pass through of both the <f waveOutMessage> and
//          <f waveInMessage> functions.
//
//  @parm   UINT | uMsg  | The message to send.
//
//  @parm   DWORD | dwParam1 | Parameter 1.
//
//  @parm   DWORD | dwParam2 | Parameter 2.
//
//  @rdesc  Dependent on uMsg.
//
//  @xref   <f waveOutMessage>, <f waveInMessage>
//
//------------------------------------------------------------------------------
DWORD
PDD_AudioMessage(
    UINT uMsg,
    DWORD dwParam1,
    DWORD dwParam2
    );


// ---------------------------------------------------------------------------
//
// @doc     WPDD_EXT
//
// @func    BOOL | PDD_AudioInitialize | Initializes 
//          the audio device for operation.  PDD_AudioInitialize performs 
//          any one time initialization. This may include allocating memory 
//          for device registers and any DMA buffer pages, etc. 
//
// @rdesc   TRUE if successful; otherwise, FALSE.
//
// ---------------------------------------------------------------------------
BOOL 
PDD_AudioInitialize (
    DWORD dwIndex
    );


// ---------------------------------------------------------------------------
//
// @doc     WPDD_EXT
//
// @func    VOID | PDD_AudioDeinitialize |
//          Powers down and disconnects the audio device. This function is
//          called if the audio driver gets unloaded from the OS or if the 
//          driver loading process fails.
//
// @rdesc   None.
//
// ---------------------------------------------------------------------------
VOID 
PDD_AudioDeinitialize (
    VOID
    );


// ---------------------------------------------------------------------------
//
// @doc     WPDD_EXT
//
// @func    VOID | PDD_AudioPowerHandler | 
//          Powers up or down the PDD.
//
// @parm    BOOL | power_down | If the value of power_down is TRUE, the audio 
//          device is shut down. If FALSE, then this is a power_up request.
//
// @rdesc   None.
//
// ---------------------------------------------------------------------------
VOID 
PDD_AudioPowerHandler (
    BOOL power_down
    );


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
#define WPDM_CLOSE          1
#define WPDM_CONTINUE       2
#define WPDM_ENDOFDATA      3
#define WPDM_GETDEVCAPS     4
#define WPDM_GETVOLUME      5
#define WPDM_OPEN           6
#define WPDM_PAUSE          7
#define WPDM_RESTART        8
#define WPDM_SETVOLUME      9
#define WPDM_STANDBY        10
#define WPDM_START          11
#define WPDM_STOP           12

// ---------------------------------------------------------------------------
//
//  @doc    WPDD_EXT
//
//  @func   MMRESULT | PDD_WaveProc | This function sends messages to the 
//          Audio PDD.  It is used to implement all of the waveOut and waveIn
//          functions at the PDD level.
//
//  @parm   WAPI_INOUT | apidir  | The direction of the API requested. 
//          See the <t WAPI_INOUT> definition.
//
//  @parm   DWORD | dwCode  | The function to execute.
//
//  @parm   DWORD | dwParam1 | Parameter 1 of the function.
//
//  @parm   DWORD | dwParam2 | Parameter 2 of the function.
//
//  @rdesc  MMRESULT dependent on dwCode. If dwCode and apidir indicate a 
//          function that is not supported, the PDD_WaveProc should return
//          MMSYSERR_NOTSUPPORTED.
//
//  @xref   <m WPDM_CLOSE>,
//          <m WPDM_CONTINUE>,
//          <m WPDM_ENDOFDATA>,
//          <m WPDM_GETDEVCAPS>,
//          <m WPDM_GETVOLUME>, 
//          <m WPDM_OPEN>,
//          <m WPDM_PAUSE>,
//          <m WPDM_RESTART>,
//          <m WPDM_SETVOLUME>,
//          <m WPDM_STANDBY>,
//          <m WPDM_START>,
//          <m WPDM_STOP>
//
// ---------------------------------------------------------------------------
MMRESULT
PDD_WaveProc(
    WAPI_INOUT apidir,
    DWORD      dwCode,
    DWORD      dwParam1,
    DWORD      dwParam2
    );



// ---------------------------------------------------------------------------
//
// @doc     WPDD_EXT
//
// @msg     WPDM_SETVOLUME | Modifies the current volume setting of 
//          the audio device.
//
// @parm    WAPI_INOUT | apidir | Output only
//
// @flag    WAPI_IN | Unused.
// @flag    WAPI_OUT | See <f waveOutSetVolume>
//
// @parm    DWORD | dwParam1 | Specifies a volume to set the audio device.  For 
//          a description of volume format, see the <f waveOutSetVolume> function 
//          in the Win32 Programmers Reference.
//
// @parm    DWORD | dwParam2 | Unused.
//
// @rdesc   If the physical audio device is capable of setting the volume, it
//          does so, and returns MMSYSERR_NOERROR, otherwise MMSYSERR_NOTSUPPORTED.
//
//  @xref   <f PDD_WaveProc>
//
// ---------------------------------------------------------------------------


// ---------------------------------------------------------------------------
//
// @doc     WPDD_EXT
//
// @msg     WPDM_GETVOLUME | Queries the current volume setting of the 
//          audio device.
//
// @parm    WAPI_INOUT | apidir | Output only
//
// @flag    WAPI_IN | Unused.
// @flag    WAPI_OUT | See <f waveOutGetVolume>
//
// @parm    DWORD | dwParam1 | Pointer to a DWORD location to receive 
//          the volume setting. For a description of volume format, see 
//          the <f waveOutSetVolume> function in the Win32 Programmers Reference.
//
// @parm    DWORD | dwParam2 | Unused.
//
// @rdesc   If the physical audio device is capable of getting the volume, it
//          does so, and returns MMSYSERR_NOERROR, otherwise MMSYSERR_NOTSUPPORTED.
//
//  @xref   <f PDD_WaveProc>
//
// ---------------------------------------------------------------------------


// ---------------------------------------------------------------------------
//
// @doc     WPDD_EXT
//
// @msg     WPDM_START | Start with the data pointed to by the wave header. 
//
// @parm    WAPI_INOUT | apidir | Input and Output
//
// @flag    WAPI_IN | Start recording into the buffers given.
// @flag    WAPI_OUT | Start playback of the buffers given.
//
// @parm    DWORD | dwParam1 | Pointer to a WAVEHDR. Points to the head of 
//          the list of wave headers.
//
// @parm    DWORD | dwParam2 | Unused.
//
// @rdesc   Returns MMSYSERR_NOERROR if supported, otherwise MMSYSERR_NOTSUPPORTED.
//
//  @xref   <f PDD_WaveProc>
//
// ---------------------------------------------------------------------------


// ---------------------------------------------------------------------------
//
// @doc     WPDD_EXT
//
// @msg     WPDM_ENDOFDATA |
//          Indicates to the PDD that no more output data is available, 
//          and the audio circuit should finish playing the data blocks
//          already received. After this function is called, the hardware
//          is expected to generate one more interrupt of type AUDIO_STATE_OUT_STOPPED 
//          to indicate that the function asynchronously completed okay.
//
// @parm    WAPI_INOUT | apidir | Output only
//
// @flag    WAPI_IN | Unused.
// @flag    WAPI_OUT | Support required for output.
//
// @parm    DWORD | dwParam1 | Unused.
//
// @parm    DWORD | dwParam2 | Unused.
//
// @rdesc   Returns MMSYSERR_NOERROR if supported, otherwise MMSYSERR_NOTSUPPORTED.
//
//  @xref   <f PDD_WaveProc>
//
// ---------------------------------------------------------------------------


// ---------------------------------------------------------------------------
//
// @doc     WPDD_EXT
//
// @msg     WPDM_CONTINUE | Continue with the data pointed to by the wave header. 
//
// @parm    WAPI_INOUT | apidir | Input and Output
//
// @flag    WAPI_IN | Continue recording.
// @flag    WAPI_OUT | Continue playback.
//
// @parm    DWORD | dwParam1 | Pointer to a WAVEHDR. Points to the head of 
//          the list of wave headers.
//
// @parm    DWORD | dwParam2 | Unused.
//
// @rdesc   Returns MMSYSERR_NOERROR if supported, otherwise MMSYSERR_NOTSUPPORTED.
//
//  @xref   <f PDD_WaveProc>
//
// ---------------------------------------------------------------------------


// ---------------------------------------------------------------------------
//
// @doc     WPDD_EXT
//
// @msg     WPDM_RESTART | Continue playing the paused data.
//
// @parm    WAPI_INOUT | apidir | Output only
//
// @flag    WAPI_IN | Unused.
// @flag    WAPI_OUT | Support required for output.
//
// @parm    DWORD | dwParam1 | Pointer to a WAVEHDR. Points to the head of 
//          the list of wave headers.
//
// @parm    DWORD | dwParam2 | Unused.
//
// @rdesc   Returns MMSYSERR_NOERROR if supported, otherwise MMSYSERR_NOTSUPPORTED.
//
//  @xref   <f PDD_WaveProc>
//
// ---------------------------------------------------------------------------


// ---------------------------------------------------------------------------
//
// @doc     WPDD_EXT
//
// @msg     WPDM_PAUSE | Pause playback.
//
// @parm    WAPI_INOUT | apidir | Output only
//
// @flag    WAPI_IN | Unused.
// @flag    WAPI_OUT | Support required for output.
//
// @parm    DWORD | dwParam1 | Unused.
//
// @parm    DWORD | dwParam2 | Unused.
//
// @rdesc   Returns MMSYSERR_NOERROR if supported, otherwise MMSYSERR_NOTSUPPORTED.
//
//  @xref   <f PDD_WaveProc>
//
// ---------------------------------------------------------------------------


// ---------------------------------------------------------------------------
//
// @doc     WPDD_EXT
//
// @msg     WPDM_STANDBY | Put the audio circuitry into standby.
//          This should power down as much of the audio circuit as possible.
//          When <m WPDM_START> is called again, the audio circuitry 
//          should be able to power back up as quickly as possibly.
//          This function is called between playback of sounds to minimize
//          audio circuit power drain.
//
// @parm    WAPI_INOUT | apidir | Input and Output
//
// @flag    WAPI_IN |  Puts input circuitry in standby.
// @flag    WAPI_OUT | Puts output circuitry in standby.
//
// @parm    DWORD | dwParam1 | Unused.
//
// @parm    DWORD | dwParam2 | Unused.
//
// @rdesc   Returns MMSYSERR_NOERROR if supported, otherwise MMSYSERR_NOTSUPPORTED.
//
//  @xref   <f PDD_WaveProc>
//
// ---------------------------------------------------------------------------


// ---------------------------------------------------------------------------
//
// @doc     WPDD_EXT
//
// @msg     WPDM_OPEN | Indicates that the waveform
//          device will be sent data of the given format. The audio 
//          circuit or software variables can be initialized at this
//          time. 
//
// @parm    WAPI_INOUT | apidir | Input and Output
//
// @flag    WAPI_IN |  Opens input path.
// @flag    WAPI_OUT | Opens output path.
//
// @parm    DWORD | dwParam1 | Pointer to a WAVEFORMATEX structure.
//          Points to the format of the data to to be played.
//
// @parm    DWORD | dwParam2 | BOOL.  If TRUE, then only check if the given 
//          format is supported. If not supported, returns WAVEERR_BADFORMAT.
//          If FALSE, then it is a real open request.
//
// @rdesc   If the device cannot be opened, return MMSYSERR_ALLOCATED.
//          If the format is not supported, return WAVEERR_BADFORMAT. If 
//          successful, return MMSYSERR_NOERROR. If not supported, return
//          MMSYSERR_NOTSUPPORTED.
//
//  @xref   <f PDD_WaveProc>
//
// ---------------------------------------------------------------------------


// ---------------------------------------------------------------------------
//
// @doc     WPDD_EXT
//
// @msg     WPDM_CLOSE | Notifies the PDD that waveXXXClose function 
//          has been called. Allows the PDD to undo any setup that took place
//          in <m WPDM_OPEN> if necessary. This function isn't required by the
//          MDD to do anything.
//
// @parm    WAPI_INOUT | apidir | Input and Output
//
// @flag    WAPI_IN |  Closes input path.
// @flag    WAPI_OUT | Closes output path.
//
// @parm    DWORD | dwParam1 | Unused.
//
// @parm    DWORD | dwParam2 | Unused.
//
// @rdesc   Returns MMSYSERR_NOERROR if supported, otherwise MMSYSERR_NOTSUPPORTED.
//
//  @xref   <f PDD_WaveProc>
//
// ---------------------------------------------------------------------------


// ---------------------------------------------------------------------------
//
// @doc     WPDD_EXT
//
// @msg     WPDM_GETDEVCAPS | This function queries
//          the hardware to determine its capabilities.
//
// @parm    WAPI_INOUT | apidir | Input and Output
//
// @flag    WAPI_IN |  Obtain input capabilities.
// @flag    WAPI_OUT | Obtain output capabilities.
//
// @parm    DWORD | dwParam1 | Points to a <t WAVEOUTCAPS> struct
//          that will hold the capabilities information. If this parameter
//          is NULL, the PDD should return immediately indicating whether the
//          driver supports the given API direction. For example, if APIDIR is 
//          WAPI_IN, and input is not supported, the function should return
//          MMSYSERR_NOTSUPPORTED.
//
// @parm    DWORD | dwParam2 | Size of the <t WAVEOUTCAPS> structure.
//
// @rdesc   Returns MMSYSERR_NOERROR if supported, otherwise MMSYSERR_NOTSUPPORTED.
//
//  @xref   <f PDD_WaveProc>
//
// ---------------------------------------------------------------------------


// ---------------------------------------------------------------------------
//
// @doc     WPDD_EXT
//
// @msg     WPDM_STOP | Stop playback/record immediately.
//
// @parm    WAPI_INOUT | apidir | Input and Output
//
// @flag    WAPI_IN | Stop recording. No follow-up interrupt required or expected.
// @flag    WAPI_OUT | Stop playback. No follow-up interrupt required or expected.
//
// @parm    DWORD | dwParam1 | Pointer to a WAVEHDR. Points to the head of 
//          the list of wave headers. (input only, unused on output)
//
// @parm    DWORD | dwParam2 | Unused.
//
// @rdesc   Returns MMSYSERR_NOERROR if supported, otherwise MMSYSERR_NOTSUPPORTED.
//
//  @xref   <f PDD_WaveProc>
//
// ---------------------------------------------------------------------------


#ifdef __cplusplus
}
#endif


#endif
