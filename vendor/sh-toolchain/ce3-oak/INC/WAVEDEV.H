//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//  Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.
//  
// -----------------------------------------------------------------------------
//
// @doc     WDEV_EXT
//
// @module  wavedev.h | Provides the device.exe DeviceIoControl control codes
//          for communicating with both waveapi.dll and wavedev.dll.
//
// -----------------------------------------------------------------------------
#ifndef __WAVEDEV_H__
#define __WAVEDEV_H__

#include "windev.h"

#ifdef __cplusplus
extern "C" {
#endif


//------------------------------------------------------------------------------
//
//  @doc    WDEV_EXT
//
//  @topic  Wave Audio API Manager Device Interface |
//          The COREDLL.DLL Dynamic Link Library gives user applications 
//          access to the Waveform Audio functions.
//
//          The Waveform Audio API Manager exports the waveXXX functions via the 
//          device manager's IOCTL calls. IOCTL_XXX constants are defined in
//          wavedev.h.
//
//  @ex     Software Layers |
//     
// *       ===============    ===============
// *       | Application |    | Application |  Module(s)
// *       ===============    ===============
// *      
// *         --------- MMSYSTEM.H ---------    Interface
// *      
// *       ==================================
// *       |          COREDLL.DLL           |  Module  (coredll.dll)
// *       ==================================
// *      
// *         ---------- WAVEDEV.H ---------    Interface
// *      
// *         ------ DeviceIoControl() -----    PSL API
// *      
// *       ==================================
// *       |     WAPI (Wave API Manager)    |  Module (waveapi.dll)
// *       ==================================
// *      
// *         ---------- WAVEDEV.H ---------    Interface
// *      
// *         ------ DeviceIoControl() -----    PSL API
// *      
// *       ==================================
// *       |            WAVEMDD             |  Module (wavedev.dll)
// *       ==================================
// *      
// *         --------- WAVEDDSI.H ---------    Interface
// *      
// *       ==================================
// *       |            WAVEPDD             |  Module (wavedev.dll)
// *       ==================================
//
//------------------------------------------------------------------------------



//
// DeviceIOControl dwIoControlCode values for Wave In devices
//
#define IOCTL_WAVE_IN_ADD_BUFFER         CTL_CODE(FILE_DEVICE_WAVE_IN, 1, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WAVE_IN_CLOSE              CTL_CODE(FILE_DEVICE_WAVE_IN, 2, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WAVE_IN_GET_DEV_CAPS       CTL_CODE(FILE_DEVICE_WAVE_IN, 3, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WAVE_IN_GET_ERROR_TEXT     CTL_CODE(FILE_DEVICE_WAVE_IN, 4, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WAVE_IN_GET_ID             CTL_CODE(FILE_DEVICE_WAVE_IN, 5, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WAVE_IN_GET_NUM_DEVS       CTL_CODE(FILE_DEVICE_WAVE_IN, 6, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WAVE_IN_GET_POSITION       CTL_CODE(FILE_DEVICE_WAVE_IN, 7, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WAVE_IN_MESSAGE            CTL_CODE(FILE_DEVICE_WAVE_IN, 8, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WAVE_IN_OPEN               CTL_CODE(FILE_DEVICE_WAVE_IN, 9, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WAVE_IN_PREPARE_HEADER     CTL_CODE(FILE_DEVICE_WAVE_IN,10, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WAVE_IN_RESET              CTL_CODE(FILE_DEVICE_WAVE_IN,11, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WAVE_IN_START              CTL_CODE(FILE_DEVICE_WAVE_IN,12, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WAVE_IN_STOP               CTL_CODE(FILE_DEVICE_WAVE_IN,13, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WAVE_IN_UNPREPARE_HEADER   CTL_CODE(FILE_DEVICE_WAVE_IN,14, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_WAVE_OUT_BREAK_LOOP        CTL_CODE(FILE_DEVICE_WAVE_OUT, 1, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WAVE_OUT_CLOSE             CTL_CODE(FILE_DEVICE_WAVE_OUT, 2, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WAVE_OUT_GET_DEV_CAPS      CTL_CODE(FILE_DEVICE_WAVE_OUT, 3, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WAVE_OUT_GET_ERROR_TEXT    CTL_CODE(FILE_DEVICE_WAVE_OUT, 4, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WAVE_OUT_GET_ID            CTL_CODE(FILE_DEVICE_WAVE_OUT, 5, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WAVE_OUT_GET_NUM_DEVS      CTL_CODE(FILE_DEVICE_WAVE_OUT, 6, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WAVE_OUT_GET_PITCH         CTL_CODE(FILE_DEVICE_WAVE_OUT, 7, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WAVE_OUT_GET_PLAYBACK_RATE CTL_CODE(FILE_DEVICE_WAVE_OUT, 8, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WAVE_OUT_GET_POSITION      CTL_CODE(FILE_DEVICE_WAVE_OUT, 9, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WAVE_OUT_GET_VOLUME        CTL_CODE(FILE_DEVICE_WAVE_OUT,10, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WAVE_OUT_MESSAGE           CTL_CODE(FILE_DEVICE_WAVE_OUT,11, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WAVE_OUT_OPEN              CTL_CODE(FILE_DEVICE_WAVE_OUT,12, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WAVE_OUT_PAUSE             CTL_CODE(FILE_DEVICE_WAVE_OUT,13, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WAVE_OUT_PREPARE_HEADER    CTL_CODE(FILE_DEVICE_WAVE_OUT,14, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WAVE_OUT_RESET             CTL_CODE(FILE_DEVICE_WAVE_OUT,15, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WAVE_OUT_RESTART           CTL_CODE(FILE_DEVICE_WAVE_OUT,16, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WAVE_OUT_SET_PITCH         CTL_CODE(FILE_DEVICE_WAVE_OUT,17, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WAVE_OUT_SET_PLAYBACK_RATE CTL_CODE(FILE_DEVICE_WAVE_OUT,18, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WAVE_OUT_SET_VOLUME        CTL_CODE(FILE_DEVICE_WAVE_OUT,19, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WAVE_OUT_UNPREPARE_HEADER  CTL_CODE(FILE_DEVICE_WAVE_OUT,20, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WAVE_OUT_WRITE             CTL_CODE(FILE_DEVICE_WAVE_OUT,21, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_SND_PLAY_SOUND             CTL_CODE(FILE_DEVICE_SOUND,    1, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_PLAY_SOUND                 CTL_CODE(FILE_DEVICE_SOUND,    2, METHOD_BUFFERED, FILE_ANY_ACCESS)

// For sending messages to the "WAV" device audio driver (wavedev.dll)
#define IOCTL_WAV_MESSAGE                CTL_CODE(FILE_DEVICE_SOUND,    3, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// For sending messages to an installed "ACM" device codec
//
#define IOCTL_ACM_MESSAGE                   CTL_CODE(FILE_DEVICE_SOUND,  100, METHOD_BUFFERED, FILE_ANY_ACCESS)
//
// IOCTL values for Win32 ACM Functions sent to waveapi.dll
//
#define IOCTL_ACM_DRIVER_ADD                CTL_CODE(FILE_DEVICE_SOUND,  110, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_ACM_DRIVER_CLOSE              CTL_CODE(FILE_DEVICE_SOUND,  111, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_ACM_DRIVER_DETAILS            CTL_CODE(FILE_DEVICE_SOUND,  112, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_ACM_DRIVER_ENUM               CTL_CODE(FILE_DEVICE_SOUND,  113, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_ACM_DRIVER_ID                 CTL_CODE(FILE_DEVICE_SOUND,  114, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_ACM_DRIVER_MESSAGE            CTL_CODE(FILE_DEVICE_SOUND,  115, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_ACM_DRIVER_OPEN               CTL_CODE(FILE_DEVICE_SOUND,  116, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_ACM_DRIVER_PRIORITY           CTL_CODE(FILE_DEVICE_SOUND,  117, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_ACM_DRIVER_REMOVE             CTL_CODE(FILE_DEVICE_SOUND,  118, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_ACM_FILTER_CHOOSE             CTL_CODE(FILE_DEVICE_SOUND,  120, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_ACM_FILTER_DETAILS            CTL_CODE(FILE_DEVICE_SOUND,  121, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_ACM_FILTER_ENUM               CTL_CODE(FILE_DEVICE_SOUND,  122, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_ACM_FILTER_TAG_DETAILS        CTL_CODE(FILE_DEVICE_SOUND,  123, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_ACM_FILTER_TAG_ENUM           CTL_CODE(FILE_DEVICE_SOUND,  124, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_ACM_FORMAT_CHOOSE             CTL_CODE(FILE_DEVICE_SOUND,  130, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_ACM_FORMAT_DETAILS            CTL_CODE(FILE_DEVICE_SOUND,  131, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_ACM_FORMAT_ENUM               CTL_CODE(FILE_DEVICE_SOUND,  132, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_ACM_FORMAT_SUGGEST            CTL_CODE(FILE_DEVICE_SOUND,  133, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_ACM_FORMAT_TAG_DETAILS        CTL_CODE(FILE_DEVICE_SOUND,  134, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_ACM_FORMAT_TAG_ENUM           CTL_CODE(FILE_DEVICE_SOUND,  135, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_ACM_STREAM_CLOSE              CTL_CODE(FILE_DEVICE_SOUND,  140, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_ACM_STREAM_CONVERT            CTL_CODE(FILE_DEVICE_SOUND,  141, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_ACM_STREAM_MESSAGE            CTL_CODE(FILE_DEVICE_SOUND,  142, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_ACM_STREAM_OPEN               CTL_CODE(FILE_DEVICE_SOUND,  143, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_ACM_STREAM_PREPARE_HEADER     CTL_CODE(FILE_DEVICE_SOUND,  144, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_ACM_STREAM_RESET              CTL_CODE(FILE_DEVICE_SOUND,  145, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_ACM_STREAM_SIZE               CTL_CODE(FILE_DEVICE_SOUND,  146, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_ACM_STREAM_UNPREPARE_HEADER   CTL_CODE(FILE_DEVICE_SOUND,  147, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_ACM_GET_VERSION               CTL_CODE(FILE_DEVICE_SOUND,  150, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_ACM_METRICS                   CTL_CODE(FILE_DEVICE_SOUND,  151, METHOD_BUFFERED, FILE_ANY_ACCESS)




//------------------------------------------------------------------------------
//
//  @doc    WDEV_EXT
//
//  @struct MMDRV_MESSAGE_PARAMS | Structure for parameters passed to 
//          the <f WAV_IOControl>() function.
//
//  @field  UINT | uDeviceId |
//          Device identifier (0, 1, 2, and so on) for the target device.
//
//  @field  UINT | uMsg | Specifies one of the <t Wave Input Driver Messages> 
//          or <t Wave Output Driver Messages>.
//
//  @field  DWORD | dwUser | Specifies a device instance identifier. For
//          the <m WODM_OPEN> and <m WIDM_OPEN> messages, this is an <p output> 
//          parameter. The device creates an instance identifier and 
//          returns it in the address specified as the argument. For all 
//          other messages, this is an <p input> parameter. The argument 
//          is the instance identifier.
//
//  @field  DWORD | dwParam1 | Specifies the first message parameter. 
//          Dependent on message type.
//
//  @field  DWORD | dwParam2 | Specifies the second message parameter. 
//          Dependent on message type.
//
//  @xref   <t Wave Input Driver Messages> (WIDM_XXX) <nl>
//          <t Wave Output Driver Messages> (WODM_XXX)
//
//------------------------------------------------------------------------------
typedef struct {
    UINT uDeviceId;
    UINT uMsg;
    DWORD dwUser;
    DWORD dwParam1;
    DWORD dwParam2;
} MMDRV_MESSAGE_PARAMS, *PMMDRV_MESSAGE_PARAMS;


//------------------------------------------------------------------------------
//
//  @doc    WACM_EXT
//
//  @struct ACMDRV_MESSAGE_PARAMS | Structure for parameters passed to 
//          the <f ACM_IOControl>() function.
//
//  @field  DWORD | dwDriverID |
//          Instance identifier. This value is message-dependent. 
//
//  @field  HANDLE | hDriver | Driver handle.
//
//  @field  UINT | uMsg | Specifies one of the <t ACM Driver Messages>.
//
//  @field  LPARAM | lParam1 | Specifies the first message parameter. 
//          Dependent on message type.
//
//  @field  LPARAM | lParam2 | Specifies the second message parameter. 
//          Dependent on message type.
//
//  @xref   <t ACM Driver Messages> (ACMDM_XXX)
//
//------------------------------------------------------------------------------
typedef struct {
    DWORD  dwDeviceId;
    HANDLE hDriver;
    UINT   uMsg;
    LPARAM lParam1;
    LPARAM lParam2;
} ACMDRV_MESSAGE_PARAMS, *PACMDRV_MESSAGE_PARAMS;


//------------------------------------------------------------------------------
//
// Parameters for the WC_PlayClick function.  This function allows an
// OEM to provide a function for playing Touch and Key Clicks on a device
// other than the standard wave device.
//
//------------------------------------------------------------------------------
typedef BOOL  (*PFN_WAVE_PLAY_CLICK)(
    WORD wClickType,     // Is this a touch or a key click
    WORD wClickVolume,   // Is it soft, loud, or muted
    DWORD dwResID        // What resource ID was read from the registry
    );  

// Possible values for wClickType
#define WC_CLICKTYPE_TOUCH 0
#define WC_CLICKTYPE_KEYBD 1

// Possible values for wClickVolume.  Muted must be zero in order for
// default case to work correctly in UpdateFromRegistry
#define WC_CLICKVOL_MUTED 0
#define WC_CLICKVOL_QUIET 1
#define WC_CLICKVOL_LOUD  2


//------------------------------------------------------------------------------
//
// Parameters for the PlaySoundHook functions.  These functions allow a
// project to hook the (snd)PlaySound function before and after the actual
// playing of the sound.
//
//------------------------------------------------------------------------------
typedef BOOL (*PFN_PLAY_SOUND_HOOK_START)(
    LPCWSTR szSoundName,
    DWORD dwSndType
    );  

#define SND_APPS            0x01000000
#define SND_EVENTS          0x02000000
#define SND_NOTIFICATIONS   0x04000000
#define SND_TCHCLICK        0x80000000
#define SND_KEYCLICK        0x40000000
#define SND_CLICK           0xC0000000

typedef void (*PFN_PLAY_SOUND_HOOK_STOP)(void);  
typedef void (*PFN_PLAY_SOUND_HOOK_UPDATE)(void);

#ifdef __cplusplus
}
#endif


#endif // __WAVEDEV_H__
