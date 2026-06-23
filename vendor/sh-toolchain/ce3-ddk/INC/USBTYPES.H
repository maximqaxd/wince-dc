/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

Module Name:  
    usbtypes.h

Abstract:  
    Definitions for Universal Serial Bus driver.
    
Notes: 

--*/
#ifndef   __USBTYPES_H__
#define   __USBTYPES_H__

#include    <windef.h>

#ifndef   __USB100_H__
#include    "usb100.h"
#endif


#ifdef __cplusplus
extern "C" {
#endif

// defines the maximum size of strings related to driver loading
#define     USB_MAX_LOAD_STRING     (MAX_PATH + 1)

// Flags for transfer functions
#define     USB_IN_TRANSFER         0x00000080
#define     USB_OUT_TRANSFER        0x00000000
#define     USB_NO_WAIT             0x00000100 //transfers without events get completed immediately
#define     USB_SHORT_TRANSFER_OK   0x00000200 //allows the transfer to be shorter than the buffer
#define     USB_START_ISOCH_ASAP    0x00000400
#define     USB_COMPRESS_ISOCH      0x00000800
#define     USB_SEND_TO_DEVICE      0x00001000
#define     USB_SEND_TO_INTERFACE   0x00002000
#define     USB_SEND_TO_ENDPOINT    0x00004000
#define     USB_DONT_BLOCK_FOR_MEM  0x00008000 // Don't block waiting for memory allocations

// USB_DEVICE_REQUEST.bmRequestType bits for control Pipes
#define     USB_REQUEST_DEVICE_TO_HOST      0x80
#define     USB_REQUEST_HOST_TO_DEVICE      0x00
#define     USB_REQUEST_STANDARD            0x00
#define     USB_REQUEST_CLASS               0x20
#define     USB_REQUEST_VENDOR              0x40
#define     USB_REQUEST_RESERVED            0x60
#define     USB_REQUEST_FOR_DEVICE          0x00
#define     USB_REQUEST_FOR_INTERFACE       0x01
#define     USB_REQUEST_FOR_ENDPOINT        0x02
#define     USB_REQUEST_FOR_OTHER           0x03

// USB Errors
#define     USB_NO_ERROR                        0x00000000
#define     USB_CRC_ERROR                       0x00000001
#define     USB_BIT_STUFFING_ERROR              0x00000002
#define     USB_DATA_TOGGLE_MISMATCH_ERROR      0x00000003
#define     USB_STALL_ERROR                     0x00000004
#define     USB_DEVICE_NOT_RESPONDING_ERROR     0x00000005
#define     USB_PID_CHECK_FAILURE_ERROR         0x00000006
#define     USB_UNEXPECTED_PID_ERROR            0x00000007
#define     USB_DATA_OVERRUN_ERROR              0x00000008
#define     USB_DATA_UNDERRUN_ERROR             0x00000009
#define     USB_BUFFER_OVERRUN_ERROR            0x0000000C
#define     USB_BUFFER_UNDERRUN_ERROR           0x0000000D
#define     USB_NOT_ACCESSED_ERROR              0x0000000E

#define     USB_ISOCH_ERROR                     0x00000100
#define     USB_CANCELED_ERROR                  0x00000101
#define     USB_NOT_COMPLETE_ERROR              0x00000103
#define     USB_CLIENT_BUFFER_ERROR             0x00000104

typedef USB_ENDPOINT_DESCRIPTOR * LPUSB_ENDPOINT_DESCRIPTOR;
typedef USB_INTERFACE_DESCRIPTOR * LPUSB_INTERFACE_DESCRIPTOR;
typedef USB_CONFIGURATION_DESCRIPTOR * LPUSB_CONFIGURATION_DESCRIPTOR;
typedef USB_DEVICE_DESCRIPTOR * LPUSB_DEVICE_DESCRIPTOR;
typedef USB_STRING_DESCRIPTOR * LPUSB_STRING_DESCRIPTOR;

typedef USB_ENDPOINT_DESCRIPTOR const * PCUSB_ENDPOINT_DESCRIPTOR;
typedef USB_ENDPOINT_DESCRIPTOR const * LPCUSB_ENDPOINT_DESCRIPTOR;

typedef USB_INTERFACE_DESCRIPTOR const * PCUSB_INTERFACE_DESCRIPTOR;
typedef USB_INTERFACE_DESCRIPTOR const * LPCUSB_INTERFACE_DESCRIPTOR;

typedef USB_CONFIGURATION_DESCRIPTOR const * PCUSB_CONFIGURATION_DESCRIPTOR;
typedef USB_CONFIGURATION_DESCRIPTOR const * LPCUSB_CONFIGURATION_DESCRIPTOR;

typedef USB_DEVICE_DESCRIPTOR const * PCUSB_DEVICE_DESCRIPTOR;
typedef USB_DEVICE_DESCRIPTOR const * LPCUSB_DEVICE_DESCRIPTOR;

typedef USB_STRING_DESCRIPTOR const * PCUSB_STRING_DESCRIPTOR;
typedef USB_STRING_DESCRIPTOR const * LPCUSB_STRING_DESCRIPTOR;


typedef LPVOID USB_HANDLE;
typedef LPVOID USB_PIPE;
typedef LPVOID USB_TRANSFER;

typedef UINT * LPUINT;
typedef DWORD const * LPCDWORD;
typedef USHORT * LPUSHORT;
typedef LPVOID * LPLPVOID;
typedef ULONG FAR * LPULONG;

typedef DWORD (WINAPI *LPTRANSFER_NOTIFY_ROUTINE)(LPVOID lpvNotifyParameter);

// Device info returned by GetDeviceInfo
typedef struct _USB_ENDPOINT {
    const DWORD                             dwCount;

    const USB_ENDPOINT_DESCRIPTOR           Descriptor;
    const LPCVOID                           lpvExtended;
} USB_ENDPOINT, * PUSB_ENDPOINT, * LPUSB_ENDPOINT;
typedef USB_ENDPOINT const * PCUSB_ENDPOINT;
typedef USB_ENDPOINT const * LPCUSB_ENDPOINT;

typedef struct _USB_INTERFACE {
    const DWORD                             dwCount;

    const USB_INTERFACE_DESCRIPTOR          Descriptor;
    const LPCVOID                           lpvExtended;
    const LPCUSB_ENDPOINT                   lpEndpoints;
} USB_INTERFACE, * PUSB_INTERFACE, * LPUSB_INTERFACE;
typedef USB_INTERFACE const * PCUSB_INTERFACE;
typedef USB_INTERFACE const * LPCUSB_INTERFACE;

typedef struct _USB_CONFIGURATION {
    const DWORD                             dwCount;

    const USB_CONFIGURATION_DESCRIPTOR      Descriptor;
    const LPCVOID                           lpvExtended;
    const DWORD                             dwNumInterfaces;
    const LPCUSB_INTERFACE                  lpInterfaces;
} USB_CONFIGURATION, * PUSB_CONFIGURATION, * LPUSB_CONFIGURATION;
typedef USB_CONFIGURATION const * PCUSB_CONFIGURATION;
typedef USB_CONFIGURATION const * LPCUSB_CONFIGURATION;

typedef struct _USB_DEVICE {
    const DWORD                             dwCount;

    const USB_DEVICE_DESCRIPTOR             Descriptor;
    const LPCUSB_CONFIGURATION              lpConfigs;
    const LPCUSB_CONFIGURATION              lpActiveConfig;
} USB_DEVICE, * PUSB_DEVICE, * LPUSB_DEVICE;
typedef USB_DEVICE const * PCUSB_DEVICE;
typedef USB_DEVICE const * LPCUSB_DEVICE;

typedef struct _USB_DEVICE_REQUEST
{
    UCHAR   bmRequestType;
    UCHAR   bRequest;
    USHORT  wValue;
    USHORT  wIndex;
    USHORT  wLength;
} USB_DEVICE_REQUEST, * PUSB_DEVICE_REQUEST, * LPUSB_DEVICE_REQUEST;
typedef USB_DEVICE_REQUEST const * PCUSB_DEVICE_REQUEST;
typedef USB_DEVICE_REQUEST const * LPCUSB_DEVICE_REQUEST;


#ifdef __cplusplus
}
#endif

#endif

