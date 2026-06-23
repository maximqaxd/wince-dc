/******************************************************************************
Copyright (c) 1996-2000 Microsoft Corporation.  All rights reserved.

DemoDisp.h
******************************************************************************/

#ifndef _DEMODISP_H_
#define _DEMODISP_H_

#ifdef __cplusplus
extern "C" {
#endif


#define DEMO_DISP_REG_KEY			TEXT("HARDWARE\\DEVICEMAP\\DISPLAY")
#define DEMO_DISP_REG_VALUE			TEXT("DriverName")


#define DEMO_DISP_INIT_FUNCTION		TEXT("DirtyRectInit")
#define	DEMO_DISP_UPDATE_FUNCTION	TEXT("DirtyRectUpdate")

typedef BOOL (*PFN_DIRTY_RECT_INIT)(
	int cxScreen,
	int cyScreen,
	int cdwStride,
	LPVOID pDibBuffer);

typedef BOOL (*PFN_DIRTY_RECT_UPDATE)(
	LPCRECT prc);

extern PFN_DIRTY_RECT_UPDATE v_pfnDirtyRectUpdate;

#ifdef __cplusplus
}
#endif

#endif // _DEMODISP_H_
