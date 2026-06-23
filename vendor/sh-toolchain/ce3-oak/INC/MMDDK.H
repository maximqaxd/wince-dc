// -----------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//  Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.
//  
// -----------------------------------------------------------------------------
//
//  MMDDK.H - Include file for Multimedia Device Development Kit
//
//  Note: You must include the WINDOWS.H header file before including this file.
//  
// -----------------------------------------------------------------------------
#ifndef _INC_MMDDK
#define _INC_MMDDK

#include "pshpack1.h"   // Assume byte packing throughout

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*    If defined, the following flags inhibit inclusion
 *    of the indicated items:
 *
 *        MMNOWAVEDEV         - Waveform support
 */
#ifdef MMNOWAVE
  #define MMNOWAVEDEV
#endif


#ifndef MMNOWAVEDEV
/****************************************************************************

		       Waveform device driver support

****************************************************************************/

// maximum number of wave device drivers loaded
#define MAXWAVEDRIVERS 10


//------------------------------------------------------------------------------
//
//  @doc    WDEV_EXT
//
//  @struct WAVEOPENDESC | 
//          The WAVEOPENDESC structure contains information needed by 
//          waveform input and output drivers. The 
//          structure is created by waveapi.dll and passed to the 
//          driver with a <m WODM_OPEN> or <m WIDM_OPEN> message. 
//  
//  @field  HWAVE | hWave |
//          Specifies the client’s handle to the device, as assigned 
//          by waveapi.dll.
//  
//  @field  LPWAVEFORMATEX | lpFormat |
//          Points to a WAVEFORMATEX structure, indicating the 
//          waveform data format requested by the client. (The 
//          WAVEFORMATEX structure is described in the Win32 SDK.) 
//  
//  @field  DWORD | dwCallback |
//          Specifies the pointer to the callback function inside waveapi.dll.
//  
//  @field  DWORD | dwInstance |
//          Contains the dwCallbackInstance argument that the client 
//          specified when calling the <f waveInOpen> or <f waveOutOpen>
//          function. 
//  
//  @field  UINT | uMappedDeviceID | 
//          For wave mapper, contains device identifier of mapped device. 
//
//------------------------------------------------------------------------------
typedef struct waveopendesc_tag {
    HWAVE          hWave;             // handle
    LPWAVEFORMATEX lpFormat;          // format of wave data
    DWORD          dwCallback;        // callback
    DWORD          dwInstance;        // app's private instance information
    UINT           uMappedDeviceID;   // device to map to if WAVE_MAPPED set
} WAVEOPENDESC;
typedef WAVEOPENDESC FAR *LPWAVEOPENDESC;

// messages sent to wodMessage() entry-point function
#define WODM_GETNUMDEVS       3
#define WODM_GETDEVCAPS       4
#define WODM_OPEN             5
#define WODM_CLOSE            6
#define WODM_PREPARE          7
#define WODM_UNPREPARE        8
#define WODM_WRITE            9
#define WODM_PAUSE            10
#define WODM_RESTART          11
#define WODM_RESET            12
#define WODM_GETPOS           13
#define WODM_GETPITCH         14
#define WODM_SETPITCH         15
#define WODM_GETVOLUME        16
#define WODM_SETVOLUME        17
#define WODM_GETPLAYBACKRATE  18
#define WODM_SETPLAYBACKRATE  19
#define WODM_BREAKLOOP        20
#define WODM_MAPPER_STATUS              (DRVM_MAPPER_STATUS + 0)
#define WAVEOUT_MAPPER_STATUS_DEVICE    0
#define WAVEOUT_MAPPER_STATUS_MAPPED    1
#define WAVEOUT_MAPPER_STATUS_FORMAT    2
#define WODM_BUSY             21

// messages sent to widMessage() entry-point function
#define WIDM_GETNUMDEVS  50
#define WIDM_GETDEVCAPS  51
#define WIDM_OPEN        52
#define WIDM_CLOSE       53
#define WIDM_PREPARE     54
#define WIDM_UNPREPARE   55
#define WIDM_ADDBUFFER   56
#define WIDM_START       57
#define WIDM_STOP        58
#define WIDM_RESET       59
#define WIDM_GETPOS      60
#define WIDM_MAPPER_STATUS              (DRVM_MAPPER_STATUS + 0)
#define WAVEIN_MAPPER_STATUS_DEVICE     0
#define WAVEIN_MAPPER_STATUS_MAPPED     1
#define WAVEIN_MAPPER_STATUS_FORMAT     2

#endif // ifndef MMNOWAVEDEV



#define MMDDKINC

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#include "poppack.h"        /* Revert to default packing */

#endif /* _INC_MMDDK */
