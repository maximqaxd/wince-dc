/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

Module Name:  

usbdi.h

Abstract:  

    Definitions for the Universal Serial Bus client driver interface.

Notes: 


--*/
#ifndef   __USBDI_H__
#define   __USBDI_H__

#ifndef   __USBTYPES_H__
#include  "usbtypes.h"
#endif


#ifdef __cplusplus
extern "C" {
#endif

// USB Notification Messages
// These messages are passed to the client drivers's LPDEVICE_NOTIFY_ROUTINE
// function.  The client driver registers it's notification routine with
// the LPREGISTER_NOTIFICATION_ROUTINE function
#define     USB_CLOSE_DEVICE    1



// Pass this constant to RegisterClientSettings to load the client driver by
// default for any of the parameters in the USB_DRIVER_SETTINGS structure.

#define     USB_NO_INFO             0xFFFFFFFF

// @doc  DRIVERS
// @struct USB_DRIVER_SETTINGS | Passed to RegisterClientSettings
// to allow USBD to recognize client driver and when to load it.  Fields are
// broken into 3 groups: vendor specific, device specific, and interface
// specific.  Load order is determined by searching first for the most general
// vendor specific client, next the most general device specific client, and 
// finally (for each interface) the most general interface specific client.
// Any of the fields may be set to USB_NO_INFO, which indicates that the client
// should be loaded by default if all other fields match, and no other client
// driver is ahead in the load order.
typedef struct _USB_DRIVER_SETTINGS
{
    DWORD dwCount;           // @field Set to the size of this structure, in bytes

    //Vendor specific (also Device Specific)
    DWORD dwVendorId;        // @field (Vendor) Matches vendor id in device descriptor
    DWORD dwProductId;       // @field (Vendor) Matches procuct id in device descriptor
    DWORD dwReleaseNumber;   // @field (Vendor) Matches release number in device descriptor

    //Device specific
    DWORD dwDeviceClass;     // @field (Device) Matches device class in device descriptor
    DWORD dwDeviceSubClass;  // @field (Device) Matches device subclass in device descriptor
    DWORD dwDeviceProtocol;  // @field (Device) Matches protocol in device descriptor

    //Interface specific
    DWORD dwInterfaceClass;    // @field (Interface) Matches class in interface descriptor
    DWORD dwInterfaceSubClass; // @field (Interface) Matches subclass in interface descriptor
    DWORD dwInterfaceProtocol; // @field (Interface) Matches protocol in interface descriptor

} USB_DRIVER_SETTINGS, * PUSB_DRIVER_SETTINGS, * LPUSB_DRIVER_SETTINGS;
typedef USB_DRIVER_SETTINGS const * PCUSB_DRIVER_SETTINGS;
typedef USB_DRIVER_SETTINGS const * LPCUSB_DRIVER_SETTINGS;

struct  _USB_FUNCS;
typedef struct _USB_FUNCS USB_FUNCS, * PUSB_FUNCS, * LPUSB_FUNCS;
typedef struct _USB_FUNCS const * PCUSB_FUNCS;
typedef struct _USB_FUNCS const * LPCUSB_FUNCS;

/******************************************************************************/
//These are entry points which all Client Drivers MUST have
BOOL USBDeviceAttach(USB_HANDLE hDevice, LPCUSB_FUNCS lpUsbFuncs,
                     LPCUSB_INTERFACE lpInterface, LPCWSTR szUniqueDriverId,
                     LPBOOL fAcceptControl,
                     LPCUSB_DRIVER_SETTINGS lpDriverSettings, DWORD dwUnused);
BOOL USBInstallDriver(LPCWSTR szDriverLibFile);
BOOL USBUnInstallDriver();

// format for the USB notification procedure (Client drivers register this
// function with the USBD to recieve device notifications.
// IMPORTANT:
// If a client driver processes a message it should return TRUE.
// If a client driver does not process a message it should return FALSE.
// All client drivers are required to process the   USB_CLOSE_DEVICE   message


typedef BOOL (WINAPI *LPDEVICE_NOTIFY_ROUTINE)(LPVOID lpvNotifyParameter,
                                               DWORD dwCode,
                                               LPDWORD * dwInfo1,
                                               LPDWORD * dwInfo2,
                                               LPDWORD * dwInfo3,
                                               LPDWORD * dwInfo4);

/******************************************************************************/
//These are entry points to the USBD (USB Driver)


// Manipulates information in the registry which identifies this device
// these functions are called from anytime from any process
// to set up the device in the registry
// To get access to them call LoadLibrary on USBD.DLL and GetProcAddress on the
// function name

//USBD version information
typedef VOID (* LPGET_USBD_VERSION)(LPDWORD lpdwMajorVersion,
                                    LPDWORD lpdwMinorVersion);
VOID GetUSBDVersion(LPDWORD lpdwMajorVersion, LPDWORD lpdwMinorVersion);

// Registers & uneregisters a client driver's Unique Driver ID.
typedef BOOL (* LPREGISTER_CLIENT_DRIVER_ID)(LPCWSTR);
BOOL RegisterClientDriverID(LPCWSTR szUniqueDriverId);
typedef BOOL (* LPUN_REGISTER_CLIENT_DRIVER_ID)(LPCWSTR);
BOOL UnRegisterClientDriverID(LPCWSTR szUniqueDriverId);

// registers & unregisters the conditions under which a driver will be loaded.
typedef BOOL (* LPREGISTER_CLIENT_SETTINGS)(LPCWSTR, LPCWSTR, LPCWSTR,
                                            LPCUSB_DRIVER_SETTINGS);
BOOL RegisterClientSettings(LPCWSTR lpszDriverLibFile,
                            LPCWSTR lpszUniqueDriverId, LPCWSTR szReserved,
                            LPCUSB_DRIVER_SETTINGS lpDriverSettings);
typedef BOOL (* LPUN_REGISTER_CLIENT_SETTINGS)(LPCWSTR, LPCWSTR,
                                               LPCUSB_DRIVER_SETTINGS);
BOOL UnRegisterClientSettings(LPCWSTR lpszUniqueDriverId, LPCWSTR szReserved,
                              LPCUSB_DRIVER_SETTINGS lpDriverSettings);

// open a client drivers's USB registry key
typedef HKEY (* LPOPEN_CLIENT_REGISTRY_KEY)(LPCWSTR);
HKEY OpenClientRegistryKey(LPCWSTR szUniqueDriverId);




// These functions are only available to Drivers
// They are not exported functions.  Drivers obtain access to them through
// the USBDeviceAttach function which passes a table of functions to the client.

//Notification routine functions (
typedef BOOL (* LPREGISTER_NOTIFICATION_ROUTINE)(USB_HANDLE,
                                                 LPDEVICE_NOTIFY_ROUTINE,
                                                 LPVOID);
typedef BOOL (* LPUN_REGISTER_NOTIFICATION_ROUTINE)(USB_HANDLE,
                                                    LPDEVICE_NOTIFY_ROUTINE,
                                                    LPVOID);


// function to load drivers for separate interfaces
typedef BOOL (* LPLOAD_GENERIC_INTERFACE_DRIVER)(USB_HANDLE, LPCUSB_INTERFACE);

//Helper commands
typedef BOOL (* LPTRANSLATE_STRING_DESCR)(LPCUSB_STRING_DESCRIPTOR, LPWSTR,
                                          DWORD);
typedef LPCUSB_INTERFACE (* LPFIND_INTERFACE)(LPCUSB_DEVICE, UCHAR, UCHAR);


//USB Subsystem Commands
typedef BOOL (* LPGET_FRAME_NUMBER)(USB_HANDLE, LPDWORD);
typedef BOOL (* LPGET_FRAME_LENGTH)(USB_HANDLE, LPUSHORT);


//Enables Device to Adjust the USB SOF period on OHCI or UHCI cards
typedef BOOL (* LPTAKE_FRAME_LENGTH_CONTROL)(USB_HANDLE);
typedef BOOL (* LPSET_FRAME_LENGTH)(USB_HANDLE, HANDLE, USHORT);
typedef BOOL (* LPRELEASE_FRAME_LENGTH_CONTROL)(USB_HANDLE);


// Gets info on a device
typedef LPCUSB_DEVICE (* LPGET_DEVICE_INFO)(USB_HANDLE);

//Device commands
typedef USB_TRANSFER (* LPISSUE_VENDOR_TRANSFER)(USB_HANDLE,
                                                 LPTRANSFER_NOTIFY_ROUTINE,
                                                 LPVOID, DWORD,
                                                 LPCUSB_DEVICE_REQUEST, LPVOID,
                                                 ULONG);
typedef USB_TRANSFER (* LPGET_INTERFACE)(USB_HANDLE, LPTRANSFER_NOTIFY_ROUTINE,
                                         LPVOID, DWORD, UCHAR, PUCHAR);
typedef USB_TRANSFER (* LPSET_INTERFACE)(USB_HANDLE, LPTRANSFER_NOTIFY_ROUTINE,
                                         LPVOID, DWORD, UCHAR, UCHAR);
typedef USB_TRANSFER (* LPGET_DESCRIPTOR)(USB_HANDLE, LPTRANSFER_NOTIFY_ROUTINE,
                                          LPVOID, DWORD, UCHAR, UCHAR, WORD,
                                          WORD, LPVOID);
typedef USB_TRANSFER (* LPSET_DESCRIPTOR)(USB_HANDLE, LPTRANSFER_NOTIFY_ROUTINE,
                                          LPVOID, DWORD, UCHAR, UCHAR, WORD,
                                          WORD, PVOID);
typedef USB_TRANSFER (* LPSET_FEATURE)(USB_HANDLE, LPTRANSFER_NOTIFY_ROUTINE,
                                       LPVOID, DWORD, WORD, UCHAR);
typedef USB_TRANSFER (* LPCLEAR_FEATURE)(USB_HANDLE, LPTRANSFER_NOTIFY_ROUTINE,
                                         LPVOID, DWORD, WORD, UCHAR);
typedef USB_TRANSFER (* LPGET_STATUS)(USB_HANDLE, LPTRANSFER_NOTIFY_ROUTINE,
                                      LPVOID, DWORD, UCHAR, LPWORD);
typedef USB_TRANSFER (* LPSYNC_FRAME)(USB_HANDLE, LPTRANSFER_NOTIFY_ROUTINE,
                                      LPVOID, DWORD, UCHAR, LPWORD);


//pipe handling
typedef USB_PIPE (* LPOPEN_PIPE)(USB_HANDLE, LPCUSB_ENDPOINT_DESCRIPTOR);
typedef BOOL (* LPABORT_PIPE_TRANSFERS)(USB_PIPE, DWORD);
typedef BOOL (* LPRESET_PIPE)(USB_PIPE);
typedef BOOL (* LPCLOSE_PIPE)(USB_PIPE);
typedef BOOL (* LPIS_PIPE_HALTED)(USB_PIPE, LPBOOL);

typedef BOOL (* LPRESET_DEFAULT_PIPE)(USB_HANDLE);
typedef BOOL (* LPIS_DEFAULT_PIPE_HALTED)(USB_HANDLE, LPBOOL);

//Transfer commands
typedef USB_TRANSFER (* LPISSUE_CONTROL_TRANSFER)(USB_PIPE,
                                                  LPTRANSFER_NOTIFY_ROUTINE,
                                                  LPVOID, DWORD, LPCVOID, DWORD,
                                                  LPVOID, ULONG);
typedef USB_TRANSFER (* LPISSUE_BULK_TRANSFER)(USB_PIPE,
                                               LPTRANSFER_NOTIFY_ROUTINE,
                                               LPVOID, DWORD, DWORD, LPVOID,
                                               ULONG);
typedef USB_TRANSFER (* LPISSUE_INTERRUPT_TRANSFER)(USB_PIPE,
                                                    LPTRANSFER_NOTIFY_ROUTINE,
                                                    LPVOID, DWORD, DWORD,
                                                    LPVOID, ULONG);
typedef USB_TRANSFER (* LPISSUE_ISOCH_TRANSFER)(USB_PIPE,
                                                LPTRANSFER_NOTIFY_ROUTINE,
                                                LPVOID, DWORD, DWORD, DWORD,
                                                LPCDWORD, LPVOID, ULONG);

// get info on Transfers
typedef BOOL (* LPIS_TRANSFER_COMPLETE)(USB_TRANSFER);
typedef BOOL (* LPGET_TRANSFER_STATUS)(USB_TRANSFER, LPDWORD, LPDWORD);
typedef BOOL (* LPGET_ISOCH_RESULTS)(USB_TRANSFER, DWORD, LPDWORD, LPDWORD);

// transfer maniuplators
typedef BOOL (* LPABORT_TRANSFER)(USB_TRANSFER, DWORD);
typedef BOOL (* LPCLOSE_TRANSFER)(USB_TRANSFER);


// function table
struct _USB_FUNCS {
    DWORD                               dwCount;

    LPGET_USBD_VERSION                  lpGetUSBDVersion;
    LPOPEN_CLIENT_REGISTRY_KEY          lpOpenClientRegistyKey;
    LPREGISTER_NOTIFICATION_ROUTINE     lpRegisterNotificationRoutine;
    LPUN_REGISTER_NOTIFICATION_ROUTINE  lpUnRegisterNotificationRoutine;
    LPLOAD_GENERIC_INTERFACE_DRIVER     lpLoadGenericInterfaceDriver;
    LPTRANSLATE_STRING_DESCR            lpTranslateStringDesc;
    LPFIND_INTERFACE                    lpFindInterface;
    LPGET_FRAME_NUMBER                  lpGetFrameNumber;
    LPGET_FRAME_LENGTH                  lpGetFrameLength;
    LPTAKE_FRAME_LENGTH_CONTROL         lpTakeFrameLengthControl;
    LPRELEASE_FRAME_LENGTH_CONTROL      lpReleaseFrameLengthControl;
    LPSET_FRAME_LENGTH                  lpSetFrameLength;
    LPGET_DEVICE_INFO                   lpGetDeviceInfo;
    LPISSUE_VENDOR_TRANSFER             lpIssueVendorTransfer;
    LPGET_INTERFACE                     lpGetInterface;
    LPSET_INTERFACE                     lpSetInterface;
    LPGET_DESCRIPTOR                    lpGetDescriptor;
    LPSET_DESCRIPTOR                    lpSetDescriptor;
    LPSET_FEATURE                       lpSetFeature;
    LPCLEAR_FEATURE                     lpClearFeature;
    LPGET_STATUS                        lpGetStatus;
    LPSYNC_FRAME                        lpSyncFrame;
    LPOPEN_PIPE                         lpOpenPipe;
    LPABORT_PIPE_TRANSFERS              lpAbortPipeTransfers;
    LPRESET_PIPE                        lpResetPipe;
    LPCLOSE_PIPE                        lpClosePipe;
    LPIS_PIPE_HALTED                    lpIsPipeHalted;
    LPISSUE_CONTROL_TRANSFER            lpIssueControlTransfer;
    LPISSUE_BULK_TRANSFER               lpIssueBulkTransfer;
    LPISSUE_INTERRUPT_TRANSFER          lpIssueInterruptTransfer;
    LPISSUE_ISOCH_TRANSFER              lpIssueIsochTransfer;
    LPIS_TRANSFER_COMPLETE              lpIsTransferComplete;
    LPGET_TRANSFER_STATUS               lpGetTransferStatus;
    LPGET_ISOCH_RESULTS                 lpGetIsochResults;
    LPABORT_TRANSFER                    lpAbortTransfer;
    LPCLOSE_TRANSFER                    lpCloseTransfer;
    LPRESET_DEFAULT_PIPE                lpResetDefaultPipe;
    LPIS_DEFAULT_PIPE_HALTED            lpIsDefaultPipeHalted;
};

#ifdef __cplusplus
}
#endif

#endif

