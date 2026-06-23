/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

Module Name:  

led_drvr.h

--*/


#ifdef __cplusplus
extern "C" {
#endif


EXTERN_C
VOID
WINAPI
NLedDriverPowerDown(
	BOOL power_down
	);
typedef VOID (WINAPI *PFN_NLED_DRIVER_POWER_DOWN)(BOOL);




EXTERN_C
BOOL
WINAPI
NLedDriverGetDeviceInfo(
	INT		nId,
	PVOID	pOutput
	);
typedef BOOL (WINAPI *PFN_NLED_DRIVER_GET_DEVICE_INFO)(INT, PVOID);



EXTERN_C
BOOL
WINAPI
NLedDriverSetDevice(
	INT		nId,
	PVOID	pInput
	);
typedef BOOL (WINAPI *PFN_NLED_DRIVER_SET_DEVICE)(INT, PVOID);



EXTERN_C
BOOL
WINAPI
NLedDriverInitialize(
	void
	);
typedef BOOL (WINAPI *PFN_NLED_DRIVER_INITIALIZE)(void);

#ifdef __cplusplus
}
#endif


