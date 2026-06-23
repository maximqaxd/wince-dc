/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

Module Name:

tchddi.h

Abstract:

    This module contains all the definitions for the DDI (Device Driver
    Interface) for the touch panel device.

Notes:


--*/
/*++

@doc EX_TOUCH_DDI EXTERNAL DRIVERS DDI TOUCH_PANEL

    This module contains all the definitions for the DDI (Device Driver
    Interface) for the touch panel device.

--*/

#ifndef __TCHDDI_H__
#define __TCHDDI_H__

#ifdef __cplusplus
extern "C" {
#endif

//
// Definitions
//




/*      @TYPE   TOUCH_PANEL_SAMPLE_FLAGS |

These flags specify information about a given touch sample sent to the input system.

See Also:<nl>
<tab><tab><l enumTouchPanelSampleFlags.enumTouchPanelSampleFlags><nl>
<tab><tab><l TouchDriverEnableCallback.PFN_TOUCH_PANEL_CALLBACK>
*/
typedef UINT32 TOUCH_PANEL_SAMPLE_FLAGS, *P_TOUCH_PANEL_SAMPLE_FLAGS;

/*		@ENUM	enumTouchPanelSampleFlags | The specific values of the touch sample flags.

		@XREF
				<tab><f TouchPanelEnable>

		@COMM

The TouchSampleValidFlag is set to indicate a valid reading.  The driver
is allowed to report back an invalid reading to the input system (which
will be ignored) but these should be kept to an absolute minimum.

The TouchSampleDownFlag shows the state of finger or stylus.  When the
finger or stylus is pressed to the touch screen, the driver will be
reporting back readings with both the valid and down flags set.  When the
stylus or finger is removed from the touch screen, the driver is REQUIRED
to send back at least one reading with the valid flag set but the down
flag cleared to indicate that the stylus/finger is up.

The TouchSampleIsCalibratedFlag is set by the driver to notify the input
system that it is not necessary to further calibrate the XY coordinates of
the sample.

The TouchSamplePreviousDownFlag shows the state from the previous valid
sample.  It is not necessary for the driver to report this to the input
system but it is useful in the driver itself.

*/
enum enumTouchPanelSampleFlags
{
	TouchSampleValidFlag			= 0x01,		//@EMEM	The sample is valid.
	TouchSampleDownFlag				= 0x02,		//@EMEM	The finger/stylus is down.
	TouchSampleIsCalibratedFlag		= 0x04,		//@EMEM	The XY data has already been calibrated.
	TouchSamplePreviousDownFlag		= 0x08,		//@EMEM	The state of the previous valid sample.
    TouchSampleIgnore				= 0x10,     //@EMEM Ignore this sample.

	TouchSampleMouse				= 0x40000000	//	reserved

};

#define TouchSampleDown(Flags)          (Flags&TouchSampleDownFlag)
#define TouchSamplePreviousDown(Flags)  (Flags&TouchSamplePreviousDownFlag)
#define TouchSampleIsMouse(Flags)		(Flags&TouchSampleMouse)

//
// TouchPanelGetDeviceCaps query capability definitions
//

// @const ULONG | TPDC_SAMPLERATE_ID |
// Sampling rate device capability.
//
#define TPDC_SAMPLE_RATE_ID				0
struct TPDC_SAMPLE_RATE
	{
	INT		SamplesPerSecondLow;
	INT		SamplesPerSecondHigh;
	INT		CurrentSampleRateSetting;	//	0 => Low, 1 => High
	};

//
// @type DDI_TOUCH_PANEL_CALIBRATION_FLAGS |
// Calibration flags
//
// @comm
// The definition is:<nl>
// <tab>typedef UINT32 DDI_TOUCH_PANEL_CALIBRATION_FLAGS;

    typedef UINT32 DDI_TOUCH_PANEL_CALIBRATION_FLAGS;

//
// @const ULONG | TPDC_CALIBRATION_POINT_COUNT_ID |
// Calibration points device capability
//
#define TPDC_CALIBRATION_POINT_COUNT_ID	1
struct TPDC_CALIBRATION_POINT_COUNT
{
	DDI_TOUCH_PANEL_CALIBRATION_FLAGS	flags;
	INT									cCalibrationPoints;
};

//
// @const ULONG | TPDC_CALIBRATION_POINT_ID |
// Calibration mapping data device capability
//
#define TPDC_CALIBRATION_POINT_ID			2
struct TPDC_CALIBRATION_POINT
{
	INT		PointNumber;
	INT		cDisplayWidth;
	INT		cDisplayHeight;
	INT		CalibrationX;
	INT		CalibrationY;
};

//
// TouchPanelSetMode set capability defintions
//
// @const ULONG | TPSM_SAMPLERATE_HIGH_ID |
// Device mode for the high sampling rate for points.
//
// @const ULONG | TPSM_SAMPLERATE_LOW_ID |
// Device mode for the low sampling rate for points.
//
//

#define TPSM_SAMPLERATE_HIGH_ID			0x00
#define TPSM_SAMPLERATE_LOW_ID			0x01
#define TPSM_PRIORITY_HIGH_ID			0x02
#define TPSM_PRIORITY_NORMAL_ID			0x04


// Function Definitions


BOOL
TouchPanelGetDeviceCaps(
	INT		iIndex,
    LPVOID  lpOutput
    );

typedef BOOL (*PFN_TOUCH_PANEL_GET_DEVICE_CAPS)(
	INT		iIndex,
    LPVOID  lpOutput
    );
			


BOOL
TouchPanelSetMode(
    INT		iIndex,
    LPVOID	lpInput
    );

typedef BOOL (*PFN_TOUCH_PANEL_SET_MODE)(
    INT		iIndex,
    LPVOID	lpInput
    );
			


//
// Function Prototype definition for the point callback function
//
// @type PFN_TOUCH_PANEL_CALLBACK |
// The function signature for the calibration and point callback functions.
// A pointer to this function is passed to <f TouchPanelEnable>.  The
// touch driver subsequently calls this function whenever a new touch
// sample is generated.
// @comm
// The signature function is:<nl>
// <tab>typedef VOID (*PFN_TOUCH_PANEL_CALLBACK)(<nl>
//  <tab><tab>DDI_TOUCH_PANEL_SAMPLE_FLAGS,<nl>
//    <tab><tab>INT,<nl>
//    <tab><tab>INT<nl>
//    <tab>);
//

typedef BOOL (*PFN_TOUCH_PANEL_CALLBACK)(
    TOUCH_PANEL_SAMPLE_FLAGS	Flags,
    INT	X,
    INT	Y
    );

typedef VOID (*PFN_DISP_DRIVER_MOVE_CURSOR)(
    INT32	X,
    INT32	Y
    );

BOOL
TouchPanelInitializeCursor(
	PFN_DISP_DRIVER_MOVE_CURSOR
	);
typedef BOOL  (*PFN_TOUCH_PANEL_INITIALIZE_CURSOR)(
				PFN_DISP_DRIVER_MOVE_CURSOR
				);


BOOL
TouchPanelEnable(
	PFN_TOUCH_PANEL_CALLBACK
    );
typedef BOOL  (*PFN_TOUCH_PANEL_ENABLE)(PFN_TOUCH_PANEL_CALLBACK);

VOID
TouchPanelDisable(
    VOID
    );
typedef VOID (*PFN_TOUCH_PANEL_DISABLE)(VOID);


BOOL
TouchPanelReadCalibrationPoint(
	INT	*pUncalibratedX,
	INT	*pUncalibratedY
    );
typedef BOOL (*PFN_TOUCH_PANEL_READ_CALIBRATION_POINT)(
						INT	*pUncalibratedX,
						INT	*pUncalibratedY
						);

VOID
TouchPanelReadCalibrationAbort(
	VOID
	);
typedef VOID (*PFN_TOUCH_PANEL_READ_CALIBRATION_ABORT)(
					VOID
					);


BOOL
TouchPanelSetCalibration(
	INT32	cCalibrationPoints,		//The number of calibration points
	INT32	*pScreenXBuffer,		//List of screen X coords displayed
	INT32	*pScreenYBuffer,		//List of screen Y coords displayed
	INT32	*pUncalXBuffer,			//List of X coords collected
	INT32	*pUncalYBuffer			//List of Y coords collected
	);
typedef BOOL (*PFN_TOUCH_PANEL_SET_CALIBRATION)(
					INT32	cCalibrationPoints,
					INT32	*pScreenXBuffer,
					INT32	*pScreenYBuffer,
					INT32	*pUncalXBuffer,
					INT32	*pUncalYBuffer
                    );


void
TouchPanelCalibrateAPoint(
    INT32   UncalX,
    INT32   UncalY,
    INT32   *pCalX,
    INT32   *pCalY
    );
typedef void  (*PFN_TOUCH_PANEL_CALIBRATE_A_POINT)(
					INT32	UncalX,
					INT32	UncalY,
					INT32	*pCalX,
					INT32	*pCalY
					);


void
TouchPanelPowerHandler(
	BOOL	bOff
	);
typedef void (*PFN_TOUCH_PANEL_POWER_HANDLER)(
				BOOL	bOff
				);


#ifdef __cplusplus
}
#endif

	
#endif
