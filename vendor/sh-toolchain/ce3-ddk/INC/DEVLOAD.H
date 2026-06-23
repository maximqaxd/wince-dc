/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

Module Name:  

devload.h

Abstract:

Device loader structures and defines

Notes: 


--*/

#ifndef __DEVLOAD_H_
#define __DEVLOAD_H_

#ifdef __cplusplus
extern "C" {
#endif

// @doc DRIVERS

//
// These keys are under HKEY_LOCAL_MACHINE
//
#define DEVLOAD_BUILT_IN_KEY TEXT("Drivers\\BuiltIn")
#define DEVLOAD_PCMCIA_KEY   TEXT("Drivers\\PCMCIA")
#define DEVLOAD_ACTIVE_KEY   TEXT("Drivers\\Active")
#define DEVLOAD_DETECT_KEY   DEVLOAD_PCMCIA_KEY TEXT("\\Detect")

//
// These are the required and optional values under a device key.
// - If the entrypoint name is present, then it will be called and the device
// DLL is responsible to call RegisterDevice.
// When the entrypoint name is not present, DEVLOAD will call RegisterDevice on
// behalf of the device and will increment the device index appropriately.  In
// this case, the device prefix must be present.
// - If the context value is present, then it will be passed as the dwInfo parameter
// to RegisterDevice.  If it is not present, then a handle to the device's registry
// will be passed in dwInfo.
// - If the init code value is present it indicates the I/O control code to use in a
// call to the device's DeviceIoControl function to indicate that the device has been
// initialized.  This provides a context for the device driver to load another driver
// which uses its device.
//
#define DEVLOAD_DLLNAME_VALNAME      TEXT("Dll")     // DLL name (required)
#define DEVLOAD_DLLNAME_VALTYPE      REG_SZ
#define DEVLOAD_LOADORDER_VALNAME    TEXT("Order")   // LoadOrder (required)
#define DEVLOAD_LOADORDER_VALTYPE    REG_DWORD
#define DEVLOAD_ENTRYPOINT_VALNAME   TEXT("Entry")   // Entrypoint name (optional)
#define DEVLOAD_ENTRYPOINT_VALTYPE   REG_SZ
#define DEVLOAD_PREFIX_VALNAME       TEXT("Prefix")  // Device prefix (optional)
#define DEVLOAD_PREFIX_VALTYPE       REG_SZ
#define DEVLOAD_INDEX_VALNAME        TEXT("Index")   // Device index (optional)
#define DEVLOAD_INDEX_VALTYPE        REG_DWORD
#define DEVLOAD_CONTEXT_VALNAME      TEXT("Context") // Device context (optional)
#define DEVLOAD_CONTEXT_VALTYPE      REG_DWORD
#define DEVLOAD_INITCODE_VALNAME     TEXT("Ioctl")   // Device IO control code to indicate context (optional)
#define DEVLOAD_INITCODE_VALTYPE     REG_DWORD

//
// The presence of the value "Keep" will cause device.exe to skip the call to
// FreeLibrary after calling the specified entrypoint.  This only affects
// builtin drivers that specify an entrypoint.
//
#define DEVLOAD_KEEPLIB_VALNAME      TEXT("Keep")
#define DEVLOAD_KEEPLIB_VALTYPE      REG_DWORD

//
// Structure passed in the input buffer of DeviceIoControl() for the 
// post initialization ioctl
//
typedef struct _POST_INIT_BUF {
    HANDLE p_hDevice;        // device handle from RegisterDevice
    HKEY   p_hDeviceKey;     // open registry handle to the driver's device key
} POST_INIT_BUF, *PPOST_INIT_BUF;

//
// These values reside under a device's active key
//
#define DEVLOAD_CLIENTINFO_VALNAME  TEXT("ClientInfo") // ClientInfo DWORD from ActivateDriver
#define DEVLOAD_CLIENTINFO_VALTYPE  REG_DWORD
#define DEVLOAD_HANDLE_VALNAME      TEXT("Hnd")     // Device handle (from RegisterDevice)
#define DEVLOAD_HANDLE_VALTYPE      REG_DWORD
#define DEVLOAD_DEVNAME_VALNAME     TEXT("Name")    // Device name (i.e "COM1:")
#define DEVLOAD_DEVNAME_VALTYPE     REG_SZ
#define DEVLOAD_DEVKEY_VALNAME      TEXT("Key")     // Device key in \Drivers\(Built-In or PCMCIA)
#define DEVLOAD_DEVKEY_VALTYPE      REG_SZ
#define DEVLOAD_PNPID_VALNAME       TEXT("PnpId")   // Plug and Play Id (PCMCIA, optional)
#define DEVLOAD_PNPID_VALTYPE       REG_SZ
#define DEVLOAD_SOCKET_VALNAME      TEXT("Sckt")    // PCMCIA socket (optional)
#define DEVLOAD_SOCKET_VALTYPE      REG_DWORD       // Actually a CARD_SOCKET_HANDLE   

//
// TAPI Pnp support
//
#define DEVLOAD_TSPDLL_VALNAME      TEXT("Tsp")     // TAPI Service Provider DLL
#define DEVLOAD_TSPDLL_VALTYPE      REG_SZ
#define DEVLOAD_TSPDEV_VALNAME      TEXT("THnd")    // TAPI device index
#define DEVLOAD_TSPDEV_VALTYPE      REG_DWORD

//
// Prototype for the optional device driver entrypoint
//
typedef DWORD (*PFN_DEV_ENTRY)(LPTSTR);     // Parameter is registry path of device's key

//
// Prototype for the detection function.
//
// Return is NULL for undetected or pointer to the device key under HLM\Drivers\PCMCIA
// of the device driver to load for a detected card.
//
typedef LPTSTR (*PFN_DETECT_ENTRY)(
                    CARD_SOCKET_HANDLE, // Socket containing the card to detect
                    UCHAR,              // Device type from CISTPL_FUNCID or 0xff for unknown
                    LPTSTR,             // Buffer to put name of device key
                    DWORD);             // Number of characters in buffer parameter

#define DEVNAME_LEN     16  // Max length of device name
#define DEVDLL_LEN      64  // Max length of device driver DLL name
#define DEVENTRY_LEN    64  // Max length of device driver entrypoint name
#define DEVPREFIX_LEN    8  // Max length of device prefix

//
// Device APIs: EnumPnpIds, EnumDevices, GetDeviceKeys and OpenDeviceKey
// (in coredll.dll)
//
DWORD EnumPnpIds(LPTSTR PnpList, LPDWORD lpBuflen);
DWORD EnumDevices(LPTSTR DevList, LPDWORD lpBuflen);
DWORD GetDeviceKeys(LPCTSTR DevName, LPTSTR ActiveKey, LPDWORD lpActiveLen,
                                     LPTSTR DriverKey, LPDWORD lpDriverLen);
HKEY OpenDeviceKey(LPCTSTR ActiveKey);

#ifdef __cplusplus
}
#endif

#endif //__DEVLOAD_H__

