/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

Module Name:  

dispdrvr.h

Abstract:  

Interface to the Display Driver library

Notes: 


--*/
/* 
    @doc EXTERNAL DISPDRVR

    @module Display Device Driver | Interface to the Display Driver library
   
    @topic Display Driver | The Display Driver is an object file (dispdrvr.obj) 
    that contains the functions and variables declared in DispDrvr.h, 
    which constitute the interface by which GDI talks to the physical 
    display device.
*/   
   
#ifndef _DISPDRVR_H_
#define _DISPDRVR_H_

#ifdef __cplusplus
extern "C" {
#endif

/* @globalv If the physical LCD memory is in the DIB format GDI uses internally,
   then DispDrvrPhysicalFrameBuffer is the physical address of the LCD 
   memory (and GDI will write directly into it).
   If the physical LCD memory is not in DIB format (for instance, if it has 
   multiple planes) then DispDrvrPhysicalFrameBuffer must be NULL.
*/
extern void * DispDrvrPhysicalFrameBuffer;

/* @globalv Number of pixels in one row of the screen
*/
extern int DispDrvr_cxScreen;

/* @globalv Number of pixels in one column of the screen
*/
extern int DispDrvr_cyScreen;

/* @globalv DispDrvr_cdwStride is the number of DWORDs between vertically 
    adjacent pixels in the DIB buffer.

   More specifically, 
   Let A be the byte address of pixel (x, y).
   Let B be the byte address of pixel (x, y + 1).
   B - A must be divisible by 4.
   DispDrvr_cdwStride = (B - A) / 4
   
   If DispDrvrPhysicalFrameBuffer is NULL, DispDrvr_cdwStride must be 
   DispDrvr_cxScreen / pixels per DWORD.
*/ 
extern int DispDrvr_cdwStride;

/* @globalv	Width, in millimeters, of the physical screen
*/
extern int DispDrvr_HORZSIZE;

/* @globalv	Height, in millimeters, of the physical screen
*/
extern int DispDrvr_VERTSIZE;

/* @globalv	Number of pixels per logical inch along the screen width
*/
extern int DispDrvr_LOGPIXELSX;

/* @globalv	Number of pixels per logical inch along the screen height
*/
extern int DispDrvr_LOGPIXELSY;




/* @func void | DispDrvrInitialize | Initialize the display driver
    
   @comm DispDrvrInitialize is called exactly once before any other display
   driver function is called or any other display driver variable is read.  The
   driver can do whatever initialization is necessary. For instance, it can
   initialize the screen hardware, or set DispDrvr_cxScreen and
   DispDrvr_cyScreen.  The display driver is not _required_ to do anything.
*/
void DispDrvrInitialize (void);


/* @func void | DispDrvrSetDibBuffer | Gives the Display Driver a pointer to the
    composition buffer
    
   @parm void * | pvDibBuffer | Pointer to the composition buffer
    
   @comm DispDrvrSetDibBuffer is called exactly once.  It gives the Display
   Driver a pointer to a bitmap in GDI's DIB format.  This bitmap is the
   composition buffer on which GDI draws.  The display driver copies pixels
   from this composition buffer when <f DispDrvrDirtyRectDump> is called.
*/   
void DispDrvrSetDibBuffer  (void * pvDibBuffer);


/* 
    @func void | DispDrvrDirtyRectDump | Dump a dirty rectangle to the screen
    
    @parm LPCRECT | prc | The dirty rectangle

    @comm When DispDrvrDirtyRectDump is called, the Display Driver should copy
    the pixels corresponding to <p prc> from the DIB buffer (previously set by
    <f DispDrvrSetDibBuffer>) to the physical display.
*/    
void DispDrvrDirtyRectDump (LPCRECT prc);



/* 
    @func void | DispDrvrPowerHandler | Power state notification

	@comm	Called just before power off with bOff==TRUE and just after power is 
	turned back on with bOff==FALSE.  The display driver should turn off/on 
	the power to the display hardware.  

	@comm	This function must not call any external API's.  It is called on a kernel
	stack with all scheduling disabled.

*/    

void DispDrvrPowerHandler(BOOL	bOff);



/* 
    @func void | DispDrvrMoveCursor | Move the cursor
    
    @parm INT32 | xLocation | The new x location of the cursor left side
    @parm INT32 | yLocation | The new y location of the cursor top

    @comm When this function is called, the cursor is erased and then redrawn
	in the new location.  The location parameters are in screen coordinates.
*/    
void DispDrvrMoveCursor (INT32 xLocation, INT32 yLocation);



#define CONTRAST_CMD_GET		0
#define CONTRAST_CMD_SET		1
#define CONTRAST_CMD_INCREASE	2
#define CONTRAST_CMD_DECREASE	3
#define CONTRAST_CMD_DEFAULT	4

/* 
    @func BOOL | DispDrvrContrastControl | Control screen contrast

	@parm	int		| Cmd		| Contrast command
	@parm	DWORD*	| pValue	| Device dependent contrast value

	@comm	Cmd is one of:
				<tab>CONTRAST_CMD_GET - gets the current contrast setting;<nl>
				<tab>CONTRAST_CMD_SET - sets the new contrast setting;<nl>
				<tab>CONTRAST_CMD_INCREASE - increases the current contrast setting;<nl>
				<tab>CONTRAST_CMD_DECREASE - increases the current contrast setting.<nl>

	@comm	If Cmd == CONTRAST_CMD_SET, pValue points to the new contrast setting.
			For all other cases, it points to the location to store the new contrast
			setting.  This parameter may not be NULL.

*/

BOOL
DispDrvrContrastControl(
	int		Cmd,
	DWORD	*pValue
	);



#ifdef __cplusplus
}
#endif

#endif
