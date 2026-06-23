/*++

Copyright (c) 1993-2000 Microsoft Corporation.  All rights reserved.

Module Name:

	hiddi.hpp	

Abstract:

	Hid Driver Interface

--*/


#ifndef _HIDDI_HPP_
#define _HIDDI_HPP_


#define FILE_DEVICE_HID         FILE_DEVICE_UNKNOWN
#define USB_IOCTL_INTERNAL_INDEX       0x0000
#define USB_IOCTL_INDEX                0x00ff


/*++

	IOCTL_HID_ENUM_DEVICES

	Enumerates the devices currently connected to HID.
	
	Input Buffer is a SHiddiEnumDeviceHeader and space for dwCount
	SHiddiEnumDevice structures.

	If there are more than dwCount devices of type dwDevType attached, this
	will set dwCount to the number of devices and not fill out any of the
	SHiddiEnumDevice structures.

	Note the dwId in the Header is only to be used in a special case.  If it
	is non-zero then the dwCount must be 1.  In that case, this will return
	the information for that specific device.
	
	Output buffer should be the same buffer as Input buffer, so that changes
	made get copied into it.

--*/

#define IOCTL_HID_ENUM_DEVICES \
			CTL_CODE(FILE_DEVICE_HID,  \
					USB_IOCTL_INDEX, \
					METHOD_BUFFERED,  \
					FILE_ANY_ACCESS)

struct SHiddiEnumDeviceHeader
{
	DWORD	dwDevType;
	DWORD	dwCount;
	DWORD	dwId;
};

struct SHiddiEnumDevice
{
	DWORD	dwDevType;
	DWORD	dwId;
	WORD	wUsagePage;
	WORD	wUsage;
};


#define IOCTL_HID_ATTACH \
			CTL_CODE(FILE_DEVICE_HID,  \
					USB_IOCTL_INDEX + 1, \
					METHOD_BUFFERED,  \
					FILE_ANY_ACCESS)

struct SHiddiAttach
{
	DWORD	dwDevType;
	DWORD	dwId;
	BOOL	bAttaching;
};

#define IOCTL_HID_GET_ATTACH_STATE \
			CTL_CODE(FILE_DEVICE_HID,  \
			USB_IOCTL_INDEX + 2, \
			METHOD_BUFFERED,  \
			FILE_ANY_ACCESS)

struct SHiddiAttachState
{
	DWORD	dwId;
	BOOL	bAttached;
};


#define IOCTL_HID_ACQUIRE \
			CTL_CODE(FILE_DEVICE_HID,  \
			USB_IOCTL_INDEX + 3, \
			METHOD_BUFFERED,  \
			FILE_ANY_ACCESS)

struct SHiddiAcquire
{
	DWORD	dwId;
	BOOL	bAcquiring;
};


#define IOCTL_HID_ENUM_OBJECTS \
			CTL_CODE(FILE_DEVICE_HID,  \
					USB_IOCTL_INDEX + 4, \
					METHOD_BUFFERED,  \
					FILE_ANY_ACCESS)

struct SHiddiEnumObjectHeader
{
	DWORD	dwId;
	DWORD	dwCount;
	DWORD	dwAxes;
	DWORD	dwButtons;
	DWORD	dwPovs;
};

const DWORD gcHiddiObjectTypeUndefined = 0;
const DWORD gcHiddiObjectTypeXAxis = 1;
const DWORD gcHiddiObjectTypeYAxis = 2;
const DWORD gcHiddiObjectTypeZAxis = 3;
const DWORD gcHiddiObjectTypeRxAxis = 4;
const DWORD gcHiddiObjectTypeRyAxis = 5;
const DWORD gcHiddiObjectTypeRzAxis = 6;
const DWORD gcHiddiObjectTypeSlider = 7;
const DWORD gcHiddiObjectTypePov = 8;
const DWORD gcHiddiObjectTypeButton = 9;
const DWORD gcHiddiObjectTypeUnknown = 0xFFFFFFFF;


struct SHiddiEnumObject
{
	WORD	obId;
	WORD	obType;
	WORD	wUsagePage;
	WORD	wUsage;
	DWORD	dwMin;
	DWORD	dwMax;
};


// Set Format has two functions, set the instantaneous data format and 
// set the buffer size for buffered format.
// dwBufCount is the buffer size and dwCount refers to the number of 
// SHiddiSetFormat structures follow the header.  A dwCount of zero is 
// perfectly valid.  In that case, HID will only set the buffer count 
// and not affect the data format.

#define IOCTL_HID_SET_FORMAT \
			CTL_CODE(FILE_DEVICE_HID,  \
					USB_IOCTL_INDEX + 5, \
					METHOD_BUFFERED,  \
					FILE_ANY_ACCESS)

struct SHiddiSetFormatHeader
{
	DWORD	dwId;
	DWORD	dwCount;
	DWORD	dwBufCount;
};


struct SHiddiSetFormat
{
	BYTE	obId;
	BYTE	offset;
};


#define IOCTL_HID_TRANSFER_EVENT \
			CTL_CODE(FILE_DEVICE_HID,  \
					USB_IOCTL_INDEX + 6, \
					METHOD_BUFFERED,  \
					FILE_ANY_ACCESS)

struct SHiddiTransferEvent
{
	DWORD dwId;
	HANDLE hEvent;
};


#define IOCTL_HID_GET_BUF_DATA \
			CTL_CODE(FILE_DEVICE_HID,  \
					USB_IOCTL_INDEX + 7, \
					METHOD_BUFFERED,  \
					FILE_ANY_ACCESS)

struct SHiddiGetBufData
{
	DWORD dwId;
	DWORD dwFlags;
	DWORD dwDodSize;
	DWORD dwDodCount;
	PBYTE pDodArray;
};



// Input buffer is just a DWORD holding the dwId.
// Output buffer is the buffer into which to write the data.
#define IOCTL_HID_GET_INST_DATA \
			CTL_CODE(FILE_DEVICE_HID,  \
					USB_IOCTL_INDEX + 8, \
					METHOD_BUFFERED,  \
					FILE_ANY_ACCESS)


// Input buffer is just a DWORD holding the dwId.
// Output buffer is ignored.
#define IOCTL_HID_POLL \
			CTL_CODE(FILE_DEVICE_HID,  \
					USB_IOCTL_INDEX + 9, \
					METHOD_BUFFERED,  \
					FILE_ANY_ACCESS)




#endif _HIDDI_HPP_
