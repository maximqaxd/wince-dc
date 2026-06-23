/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

Module Name:  

windev.h

Abstract:  

Notes: 


--*/
#ifndef __WINDEV_H__
#define __WINDEV_H__ 1

#ifdef __cplusplus
extern "C" {
#endif

// @doc OSDEVICE

//      @func PVOID | ttt_Init | Device initialization routine
//  @parm DWORD | dwInfo | info passed to RegisterDevice
//  @rdesc      Returns a DWORD which will be passed to Open & Deinit or NULL if
//                      unable to initialize the device.
//      @comm   Routine exported by a device driver.  "ttt" is the string passed in as
//                      lpszType in RegisterDevice

DWORD ttt_Init(DWORD dwInfo);

//      @func PVOID | ttt_Deinit | Device deinitialization routine
//  @parm DWORD | dwData | value returned from ttt_Init call
//  @rdesc      Returns TRUE for success, FALSE for failure.
//      @comm   Routine exported by a device driver.  "ttt" is the string passed in as
//                      lpszType in RegisterDevice

BOOL ttt_Deinit(DWORD dwData);

//      @func PVOID | ttt_Open | Device open routine
//  @parm DWORD | dwData | value returned from ttt_Init call
//  @parm DWORD | dwAccess | requested access (combination of GENERIC_READ and GENERIC_WRITE)
//  @parm DWORD | dwShareMode | requested share mode (combination of FILE_SHARE_READ and FILE_SHARE_WRITE)
//  @rdesc      Returns a DWORD which will be passed to Read, Write, etc or NULL if
//                      unable to open device.
//      @comm   Routine exported by a device driver.  "ttt" is the string passed in as
//                      lpszType in RegisterDevice

DWORD ttt_Open(DWORD dwData, DWORD dwAccess, DWORD dwShareMode);

//      @func BOOL | ttt_Close | Device close routine
//  @parm DWORD | dwOpenData | value returned from ttt_Open call
//  @rdesc      Returns TRUE for success, FALSE for failure
//      @comm   Routine exported by a device driver.  "ttt" is the string passed in as
//                      lpszType in RegisterDevice

BOOL ttt_Close(DWORD dwOpenData);

//      @func DWORD | ttt_Read | Device read routine
//  @parm DWORD | dwOpenData | value returned from ttt_Open call
//  @parm LPVOID | pBuf | buffer to receive data
//  @parm DWORD | len | maximum length to read
//  @rdesc      Returns 0 for end of file, -1 for error, otherwise the number of
//                      bytes read.  The length returned is guaranteed to be the length
//                      requested unless end of file or an error condition occurs.
//      @comm   Routine exported by a device driver.  "ttt" is the string passed in as
//                      lpszType in RegisterDevice

DWORD ttt_Read(DWORD dwOpenData, LPVOID pBuf, DWORD len);

//      @func DWORD | ttt_Write | Device write routine
//  @parm DWORD | dwOpenData | value returned from ttt_Open call
//  @parm LPCVOID | pBuf | buffer containing data
//  @parm DWORD | len | maximum length to write
//  @rdesc      Returns -1 for error, otherwise the number of bytes written.  The
//                      length returned is guaranteed to be the length requested unless an
//                      error condition occurs.
//      @comm   Routine exported by a device driver.  "ttt" is the string passed in as
//                      lpszType in RegisterDevice

DWORD ttt_Write(DWORD dwOpenData, LPCVOID pBuf, DWORD len);

//      @func DWORD | ttt_Seek | Device seek routine
//  @parm DWORD | dwOpenData | value returned from ttt_Open call
//  @parm long | pos | position to seek to (relative to type)
//  @parm DWORD | type | FILE_BEGIN, FILE_CURRENT, or FILE_END
//  @rdesc      Returns current position relative to start of file, or -1 on error
//      @comm   Routine exported by a device driver.  "ttt" is the string passed in as
//                      lpszType in RegisterDevice

DWORD ttt_Seek(DWORD dwOpenData, long pos, DWORD type);

//      @func BOOL | ttt_IOControl | Device IO control routine
//  @parm DWORD | dwOpenData | value returned from ttt_Open call
//  @parm DWORD | dwCode | io control code to be performed
//  @parm PBYTE | pBufIn | input data to the device
//  @parm DWORD | dwLenIn | number of bytes being passed in
//  @parm PBYTE | pBufOut | output data from the device
//  @parm DWORD | dwLenOut |maximum number of bytes to receive from device
//  @parm PDWORD | dwActualOut | actual number of bytes received from device
//  @rdesc      Returns TRUE for success, FALSE for failure
//      @comm   Routine exported by a device driver.  "ttt" is the string passed in as
//                      lpszType in RegisterDevice

BOOL ttt_IOControl(DWORD dwOpenData, DWORD dwCode, PBYTE pBufIn, DWORD dwLenIn, PBYTE pBufOut, DWORD dwLenOut, PDWORD dwActualOut);

//      @func void | ttt_PowerDown | Device powerdown routine
//  @parm DWORD | dwData | value returned from ttt_Init call
//      @comm   Called to suspend device.  You cannot call any routines aside from
//                      those in your dll in this call.

void ttt_PowerDown(DWORD dwData);

//      @func void | ttt_PowerUp | Device powerup routine
//  @parm DWORD | dwData | value returned from ttt_Init call
//      @comm   Called to restore device from suspend mode.  You cannot call any
//                      routines aside from those in your dll in this call.

void ttt_PowerUp(DWORD dwData);

//
// Macro definition for defining IOCTL and FSCTL function control codes.  Note
// that function codes 0-2047 are reserved for Microsoft Corporation, and
// 2048-4095 are reserved for customers.
//

#define CTL_CODE( DeviceType, Function, Method, Access ) (                 \
    ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) \
)

//
// Define the method codes for how buffers are passed for I/O and FS controls
//

#define METHOD_BUFFERED                 0
#define METHOD_IN_DIRECT                1
#define METHOD_OUT_DIRECT               2
#define METHOD_NEITHER                  3

//
// Define the access check value for any access
//
//
// The FILE_READ_ACCESS and FILE_WRITE_ACCESS constants are also defined in
// ntioapi.h as FILE_READ_DATA and FILE_WRITE_DATA. The values for these
// constants *MUST* always be in sync.
//

#define FILE_ANY_ACCESS                 0
#define FILE_READ_ACCESS          ( 0x0001 )    // file & pipe
#define FILE_WRITE_ACCESS         ( 0x0002 )    // file & pipe


// begin_ntddk begin_nthal begin_ntifs
//
// Define the various device type values.  Note that values used by Microsoft
// Corporation are in the range 0-32767, and 32768-65535 are reserved for use
// by customers.
//

#define DEVICE_TYPE ULONG

#define FILE_DEVICE_BEEP                0x00000001
#define FILE_DEVICE_CD_ROM              0x00000002
#define FILE_DEVICE_CD_ROM_FILE_SYSTEM  0x00000003
#define FILE_DEVICE_CONTROLLER          0x00000004
#define FILE_DEVICE_DATALINK            0x00000005
#define FILE_DEVICE_DFS                 0x00000006
#define FILE_DEVICE_DISK                0x00000007
#define FILE_DEVICE_DISK_FILE_SYSTEM    0x00000008
#define FILE_DEVICE_FILE_SYSTEM         0x00000009
#define FILE_DEVICE_INPORT_PORT         0x0000000a
#define FILE_DEVICE_KEYBOARD            0x0000000b
#define FILE_DEVICE_MAILSLOT            0x0000000c
#define FILE_DEVICE_MIDI_IN             0x0000000d
#define FILE_DEVICE_MIDI_OUT            0x0000000e
#define FILE_DEVICE_MOUSE               0x0000000f
#define FILE_DEVICE_MULTI_UNC_PROVIDER  0x00000010
#define FILE_DEVICE_NAMED_PIPE          0x00000011
#define FILE_DEVICE_NETWORK             0x00000012
#define FILE_DEVICE_NETWORK_BROWSER     0x00000013
#define FILE_DEVICE_NETWORK_FILE_SYSTEM 0x00000014
#define FILE_DEVICE_NULL                0x00000015
#define FILE_DEVICE_PARALLEL_PORT       0x00000016
#define FILE_DEVICE_PHYSICAL_NETCARD    0x00000017
#define FILE_DEVICE_PRINTER             0x00000018
#define FILE_DEVICE_SCANNER             0x00000019
#define FILE_DEVICE_SERIAL_MOUSE_PORT   0x0000001a
#define FILE_DEVICE_SERIAL_PORT         0x0000001b
#define FILE_DEVICE_SCREEN              0x0000001c
#define FILE_DEVICE_SOUND               0x0000001d
#define FILE_DEVICE_STREAMS             0x0000001e
#define FILE_DEVICE_TAPE                0x0000001f
#define FILE_DEVICE_TAPE_FILE_SYSTEM    0x00000020
#define FILE_DEVICE_TRANSPORT           0x00000021
#define FILE_DEVICE_UNKNOWN             0x00000022
#define FILE_DEVICE_VIDEO               0x00000023
#define FILE_DEVICE_VIRTUAL_DISK        0x00000024
#define FILE_DEVICE_WAVE_IN             0x00000025
#define FILE_DEVICE_WAVE_OUT            0x00000026
#define FILE_DEVICE_8042_PORT           0x00000027
#define FILE_DEVICE_NETWORK_REDIRECTOR  0x00000028



// EnumDevices enumerates the active devices in the system.
// The return buffer is filled with a double-NULL terminated list of NULL
// terminated strings of the device names, for example: COM1:\0COM2:\0PGR1:\0\0.
DWORD EnumDevices(LPTSTR DevList, LPDWORD lpBuflen);

// GetDeviceKeys returns the registry paths for the device driver key and the
// active device key for the specified device.
DWORD GetDeviceKeys(LPCTSTR DevName, LPTSTR ActiveKey, LPDWORD lpActiveLen,
                    LPTSTR DriverKey, LPDWORD lpDriverLen);

// IsAPIReady tells whether the specified API set has been registered
BOOL IsAPIReady(DWORD hAPI);

#ifdef __cplusplus
}
#endif

#endif	// __WINDEV_H__
