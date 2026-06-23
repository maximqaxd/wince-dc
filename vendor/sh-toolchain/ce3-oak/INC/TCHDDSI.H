/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

Module Name:  

tchddsi.h

Abstract:  

Notes: 


--*/
/*++

@doc EX_TOUCH_DDSI EXTERNAL DRIVERS DDSI TOUCH_PANEL

    This module contains all the definitions for the DDSI (Device Driver
    Service Provider Interface) for the touch panel device.

--*/

#ifndef __TCHDDSI_H__
#define __TCHDDSI_H__

#include <tchddi.h>


#ifdef __cplusplus
extern "C" {
#endif


//
// The PDD defines the touch Interrupt IDs.
//
extern const DWORD gIntrTouch;
extern const DWORD gIntrTouchChanged;

// Definitions
BOOL
DdsiTouchPanelGetDeviceCaps(
	INT		iIndex,
	LPVOID	lpOutput
    );

typedef BOOL (*PFN_DDSI_TOUCH_PANEL_GET_DEVICE_CAPS)(
	INT		iIndex,
	LPVOID	lpOutput
    );

BOOL
DdsiTouchPanelSetMode(
	INT		iIndex,
	LPVOID	lpInput
    );

typedef BOOL (*PFN_DDSI_TOUCH_PANEL_SET_MODE)(
	INT		iIndex,
	LPVOID	lpInput
    );


BOOL
DdsiTouchPanelEnable(
    void
    );

typedef BOOL (*PFN_DDSI_TOUCH_PANEL_ENABLE)(
    void
    );


void
DdsiTouchPanelDisable(
    void
    );

typedef BOOL (*PFN_DDSI_TOUCH_PANEL_DISABLE)(
    void
    );


LONG
DdsiTouchPanelAttach(
    void
    );

typedef LONG (*PFN_DDSI_TOUCH_PANEL_ATTACH)(
    void
    );


LONG
DdsiTouchPanelDetach(
    void
    );

typedef LONG (*PFN_DDSI_TOUCH_PANEL_DETACH)(
    void
    );

void
DdsiTouchPanelGetPoint(
    P_TOUCH_PANEL_SAMPLE_FLAGS	pTipState,
    INT							*pUncalX,
    INT							*pUncalY
    );

typedef void (*PFN_DDSI_TOUCH_PANEL_GET_POINT)(
    P_TOUCH_PANEL_SAMPLE_FLAGS	pTipState,
    INT							*pUncalX,
    INT							*pUncalY
    );

BOOL
DdsiTouchPanelSetCalibration(
	INT32	cCalibrationPoints,	
	INT32	*pScreenXBuffer,	
	INT32	*pScreenYBuffer,	
	INT32	*pUncalXBuffer,		
	INT32	*pUncalYBuffer		
	);

typedef BOOL (*PFN_DDSI_TOUCH_PANEL_SET_CALIBRATION)(
	INT32	cCalibrationPoints,
	INT32	*pScreenXBuffer,
	INT32	*pScreenYBuffer,
	INT32	*pUncalXBuffer,
	INT32	*pUncalYBuffer
	);

void
DdsiTouchPanelCalibrateAPoint(
    INT32   UncalX,
    INT32   UncalY,
    INT32   *pCalX,
    INT32   *pCalY
    );

void
DdsiTouchPanelPowerHandler(
	BOOL	bOff
	);

typedef void (*PFN_DDSI_TOUCH_PANEL_CALIBRATE_A_POINT)(
    INT32   UncalX,
    INT32   UncalY,
    INT32   *pCalX,
    INT32   *pCalY
    );


#ifdef DEBUG
# ifdef BUILD_AS_LIB
// If we link to gwe, we ned to share his zones, or come
// up with our own variant of debug zones.  For now, just
// don't do anything.
#define DEBUGMSG
# else
// If we build as out own DLL, we can use the debug api
// Note that some zones (like delays and timing) rely
// on special builds of other components (cause detector)
// in order to display valid data.
#define ZONE_SAMPLES	DEBUGZONE(0)
#define ZONE_CALIBRATE	DEBUGZONE(1)
#define ZONE_STATS	    DEBUGZONE(2)
#define ZONE_THREAD	    DEBUGZONE(3)
#define ZONE_TIPSTATE	DEBUGZONE(4)
#define ZONE_INIT	    DEBUGZONE(5)
#define ZONE_6	        DEBUGZONE(6)
#define ZONE_7	        DEBUGZONE(7)
#define ZONE_8	        DEBUGZONE(8)
#define ZONE_MISC		DEBUGZONE(9)
#define ZONE_DELAYS		DEBUGZONE(10)
#define ZONE_TIMING		DEBUGZONE(11)
#define ZONE_ALLOC		DEBUGZONE(12)
#define ZONE_FUNCTION	DEBUGZONE(13)
#define ZONE_WARN		DEBUGZONE(14)
#define ZONE_ERROR		DEBUGZONE(15)

// unofficial zones - The upper 16 zones don't show up with nice
// names in cesh, etc. because we only store mnemonics for the first
// 16 zones in DBGPARAM.
#define ZONE_RXDATA		DEBUGZONE(16)
#define ZONE_TXDATA		DEBUGZONE(17)
#define ZONE_CELOG		DEBUGZONE(18)

# endif
#endif


#ifdef __cplusplus
}
#endif


#endif
