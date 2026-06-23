/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

Module Name:

pwingdi.h

Abstract:

Private portion of wingdi.h

Notes:


--*/

#ifndef  _PRIV_INC_WINGDI_H
#define  _PRIV_INC_WINGDI_H

#define FLOODFILLBORDER		0
#define FLOODFILLSURFACE	1
#define RC_FLOODFILL        0x1000      /* supports FloodFill           */
#define DT_EDITCONTROL      0x00002000
#define DT_WORD_ELLIPSIS    0x00040000

#include <windbase.h>

#ifdef __cplusplus
extern "C" {
#endif

// Made non-public to prevent ISVs from using it on 2bpp devices.  If they did
// their apps would not work on a color device.
#define PALETTEINDEX(i)     ((COLORREF)(0x01000000 | (DWORD)(WORD)(i)))

// @CESYSGEN IF GWES_MGTT || GWES_MGRAST || GWES_GDIFONTS || GWES_PGDI
WINGDIAPI  BOOL WINAPI CeRemoveFontResource(CEOID);
// @CESYSGEN ENDIF

// GDI Escapes for ExtEscape()
#define QUERYESCSUPPORT				8

// The following are unique to CE
#define GETVFRAMEPHYSICAL			6144
#define GETVFRAMELEN				6145
#define	DBGDRIVERSTAT				6146
#define SETPOWERMANAGEMENT			6147
#define GETPOWERMANAGEMENT			6148
#define CONTRASTCOMMAND				6149

// These esacpe are called at beginning & end of ExtTextOut() so DirtyRect drivers 
// can optimize by accumulating the bounding rectangle & updating at all at once
// Note: This is only an optimization IF the cost of small updates exceeds the 
// cost of accumulating rects & updating a larger rect
#define DRVESC_UPDATE_PAUSE			6150
#define DRVESC_UPDATE_RESUME		6151

typedef enum _VIDEO_POWER_STATE {
    VideoPowerOn = 1,
    VideoPowerStandBy,
    VideoPowerSuspend,
    VideoPowerOff
} VIDEO_POWER_STATE, *PVIDEO_POWER_STATE;


typedef struct _VIDEO_POWER_MANAGEMENT {
    ULONG Length;
    ULONG DPMSVersion;
    ULONG PowerState;
} VIDEO_POWER_MANAGEMENT, *PVIDEO_POWER_MANAGEMENT;

//
//Length - Length of the structure in bytes. Also used to do verisioning.
//
//DPMSVersion - Version of the DPMS standard supported by the device.
//              Only used in the "GET" IOCTL.
//
//PowerState - One of the power states listed in VIDEO_POWER_STATE.
//



// The CONTRASTCOMMAND DrvEscape is used to get/set the contrast
// on the display controlled by the driver being called. It input parameter
// shoud be a ContrastCmdInputParm structure (defined below). Its output
// parameter should be an int. If the command completes successfully
// the DrvEscape should return TRUE, otherwise it should return FALSE.
#define CONTRAST_CMD_GET		0	// Parm=Ignored,	Out=Current setting
#define CONTRAST_CMD_SET		1	// Parm=NewSet,		Out=Resulting setting
#define CONTRAST_CMD_INCREASE	2	// Parm=Amount,		Out=Resulting setting
#define CONTRAST_CMD_DECREASE	3	// Parm=Amount,		Out=Resulting setting
#define CONTRAST_CMD_DEFAULT	4	// Parm=Ignored,	Out=Resulting setting
#define CONTRAST_CMD_MAX		5	// Parm=Ignored,	Out=Max value


//
// A ContrastCmdInputParm is the input parameter to a DrvEscape call
// with CONTRASTCOMMAND set.
//
// ContrastCmdInputParm.command : This is mean to be one of the commands
// defined above.
//
// ContrastCmdInputParm.parm : The definition of this parameter depends
// on the value of command. It will typically be either the contrast
// value to set, or the size to increment the contrast by.
//
struct ContrastCmdInputParm {
	int		command;				
	int 	parm;
};

#ifdef __cplusplus
}
#endif

#endif //_PRIV_INC_WINGDI_H
