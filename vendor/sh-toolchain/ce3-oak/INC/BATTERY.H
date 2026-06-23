/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

Module Name:  

battery.h

Abstract:  

Interface to the Battery Driver library

Notes: 


--*/
/* 
    @doc DRIVERS

    @module Battery Device Driver | Interface to the Battery Driver library
   
    @topic Battery Driver | The Battery Driver is an object file (battery.obj) 
    that contains the functions and variables declared in Battery.h, 
    which constitute the interface by which GWE gets information about the 
    main and backup batteries and AC power.
*/   
   
#ifndef _BATTERY_H_
#define _BATTERY_H_

#ifdef __cplusplus
extern "C" {
#endif


/* 
    @func BOOL | BatteryDrvrGetStatus | Return the status of the main and backup
	batteries
    
    @parm PSYSTEM_POWER_STATUS_EX2 | pSystemPowerStatus | Buffer to be filled 
    in with the battery status information.  See the Win32 Documentation for a
	description of the SYSTEM_POWER_STATUS structure.  

*/

BOOL WINAPI BatteryDrvrGetStatus (PSYSTEM_POWER_STATUS_EX2 pPowerStatus,
                                  PBOOL pfBatteriesChangedSinceLastCall);


/* 
    @func LONG | BatteryDrvrGetLevels | Return the number of levels the battery
    driver is capable of returning in the BatteryFlag and BackupBatteryFlag
	fields of the BATTERY_STATUS structure.
    
	@rdesc The LOWORD of the return value is the number of main battery levels
	which BatteryDrvrGetStatus is capable of returning in the BatteryFlag
	field of the BATTERY_STATUS structure.  For example, if only 
	BATTERY_FLAG_HIGH and BATTERY_FLAG_LOW can be returned, the LOWORD of the 
	return value would be 2.  If BATTERY_FLAG_CRITICAL  can also be returned,
	the LOWORD of the return value would be 3.  The values BATTERY_FLAG_CHARGING, 
	BATTERY_FLAG_NO_BATTERY, or BATTERY_FLAG_UNKNOWN are to be ignored for the 
	purposes of this count.

	The HIWORD of the return value is the number of backup battery levels which
 	BatteryDrvrGetStatus is capable of returning.  The same rules apply as to
	the main battery levels.
*/    
LONG WINAPI BatteryDrvrGetLevels (void);



/* 
    @func BOOL | BatteryDrvrSupportsChangeNotification | Returns whether the 
	hardware can detect when the batteries have been changed.  If FALSE,
	BATTERY_STATUS.fBatteriesChangedSinceLastCall will always be FALSE.
*/
BOOL WINAPI BatteryDrvrSupportsChangeNotification (void);

#ifdef __cplusplus
}
#endif

#endif
