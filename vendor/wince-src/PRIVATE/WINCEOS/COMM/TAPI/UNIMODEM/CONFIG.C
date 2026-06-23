// ---------------------------------------------------------------------------
//
// Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.
//
// ---------------------------------------------------------------------------

#include <windows.h>
#include <memory.h>

#include <tapi.h>
#include "mcx.h"
#include "tspi.h"
#include "tspip.h"
#include "config.h"
#include "netui.h"

//
// Read the COMMPROPs to get the settings the device allows
//
void
GetSettableFields(
    PTLINEDEV   pLineDev,
    DWORD * pdwSettableBaud,
    WORD * pwSettableData,
    WORD * pwSettableStopParity
    )
{
    HANDLE hComDev;
    COMMPROP commP;

    *pdwSettableBaud = BAUD_110 | BAUD_300 | BAUD_600 | BAUD_1200 |
                               BAUD_2400 | BAUD_4800 | BAUD_9600 | BAUD_14400 |
                               BAUD_19200 | BAUD_38400 | BAUD_57600 | BAUD_115200;
    *pwSettableData = DATABITS_5 | DATABITS_6 | DATABITS_7 | DATABITS_8;
    *pwSettableStopParity = STOPBITS_10 | STOPBITS_15 | STOPBITS_20 |
                                     PARITY_NONE | PARITY_ODD | PARITY_EVEN |
                                     PARITY_SPACE | PARITY_MARK;

    if (pLineDev->hDevice_r0 != (HANDLE)INVALID_DEVICE) {
        if (!GetCommProperties(pLineDev->hDevice_r0, &commP)) {
            DEBUGMSG(ZONE_ERROR, (TEXT("UNIMODEM:GetSettableFields - GetCommProperties failed %d\n"), GetLastError()));
            return;
        }
    } else {
        if ((hComDev = CreateFile(
                            pLineDev->szDeviceName,
                            GENERIC_READ,
                            0,
                            NULL,
                            OPEN_EXISTING,
                            0,
                            0
                            ))
            != INVALID_HANDLE_VALUE) {
            if (!GetCommProperties(hComDev, &commP)) {
                DEBUGMSG(ZONE_ERROR, (TEXT("UNIMODEM:GetSettableFields - GetCommProperties failed %d\n"), GetLastError()));
                CloseHandle(hComDev);
                return;
            }
            CloseHandle(hComDev);
        } else {
            DEBUGMSG(ZONE_ERROR,
                (TEXT("UNIMODEM:GetSettableFields - CreateFile(%s) failed %d\n"), pLineDev->szDeviceName, GetLastError()));
            return;
        }
    }

    *pdwSettableBaud        = commP.dwSettableBaud;
    *pwSettableData         = commP.wSettableData;
    *pwSettableStopParity   = commP.wSettableStopParity;

}   // GetSettableFields

 
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
BOOL
TSPI_EditMiniConfig(
   HWND        hWndOwner,
   PTLINEDEV   pLineDev,
   PDEVMINICFG lpSettingsIn,
   PDEVMINICFG lpSettingsOut
   )
{
	LINECONFIGDATA	LineConfigData;
	BOOL			fRetVal;

	DEBUGMSG (ZONE_MISC, (TEXT("+TSPI_EditMiniConfig(0x%X, 0x%X, 0x%X)\r\n"),
						  hWndOwner, lpSettingsIn, lpSettingsOut));

	// Just copy the in to the out as a default
	memcpy ((char *)lpSettingsOut, (char *)lpSettingsIn,
			sizeof(DEVMINICFG));
	
	memset ((char *)&LineConfigData, 0, sizeof(LINECONFIGDATA));
	
	// Move the data over.
	LineConfigData.dwVersion = NETUI_LCD_STRUCTVER;
	LineConfigData.dwBaudRate = lpSettingsIn->dwBaudRate;

    GetSettableFields(
        pLineDev,
    	&(LineConfigData.dwSettableBaud),
    	&(LineConfigData.wSettableData),
        &(LineConfigData.wSettableStopParity)
        );

	LineConfigData.bByteSize = lpSettingsIn->ByteSize;
	LineConfigData.bParity = lpSettingsIn->Parity;
	LineConfigData.bStopBits = lpSettingsIn->StopBits;
	LineConfigData.wWaitBong = lpSettingsIn->wWaitBong;
	LineConfigData.dwCallSetupFailTimer = lpSettingsIn->dwCallSetupFailTimer;
	
	if (lpSettingsIn->dwModemOptions & MDM_BLIND_DIAL) {
		LineConfigData.dwModemOptions |= NETUI_LCD_MDMOPT_BLIND_DIAL;
	}
	if (lpSettingsIn->dwModemOptions & MDM_FLOWCONTROL_SOFT) {
		LineConfigData.dwModemOptions |= NETUI_LCD_MDMOPT_SOFT_FLOW;
	}
	if (lpSettingsIn->dwModemOptions & MDM_FLOWCONTROL_HARD) {
		LineConfigData.dwModemOptions |= NETUI_LCD_MDMOPT_HARD_FLOW;
	}
	if (lpSettingsIn->dwModemOptions & MDM_SPEED_ADJUST) {  // DCC automatic baud rate detection
		LineConfigData.dwModemOptions |= NETUI_LCD_MDMOPT_AUTO_BAUD;
	}
	if (lpSettingsIn->fwOptions & TERMINAL_PRE) {
		LineConfigData.dwTermOptions |= NETUI_LCD_TRMOPT_PRE_DIAL;
	}
	if (lpSettingsIn->fwOptions & TERMINAL_POST) {
		LineConfigData.dwTermOptions |= NETUI_LCD_TRMOPT_POST_DIAL;
	}
	if (lpSettingsIn->fwOptions & MANUAL_DIAL) {
		LineConfigData.dwTermOptions |= NETUI_LCD_TRMOPT_MANUAL_DIAL;
	}

	// Set the maximum length
	LineConfigData.dwModMaxLen = DIAL_MODIFIER_LEN;
	wcscpy (LineConfigData.szDialModifier, lpSettingsIn->szDialModifier);

	
	if (fRetVal = CallLineConfigEdit(hWndOwner, &LineConfigData)) {
		// Move the data back.
		lpSettingsOut->wWaitBong = LineConfigData.wWaitBong;
		lpSettingsOut->dwCallSetupFailTimer =
			LineConfigData.dwCallSetupFailTimer;

        lpSettingsOut->dwModemOptions &= ~(MDM_FLOWCONTROL_SOFT|MDM_FLOWCONTROL_HARD|MDM_SPEED_ADJUST|MDM_BLIND_DIAL);
		if (LineConfigData.dwModemOptions & NETUI_LCD_MDMOPT_SOFT_FLOW) {
			lpSettingsOut->dwModemOptions |= MDM_FLOWCONTROL_SOFT;
		}
		if (LineConfigData.dwModemOptions & NETUI_LCD_MDMOPT_HARD_FLOW) {
			lpSettingsOut->dwModemOptions |= MDM_FLOWCONTROL_HARD;
		}
		if (LineConfigData.dwModemOptions & NETUI_LCD_MDMOPT_AUTO_BAUD) {
			lpSettingsOut->dwModemOptions |= MDM_SPEED_ADJUST;
		}
		if (LineConfigData.dwModemOptions & NETUI_LCD_MDMOPT_BLIND_DIAL) {
			lpSettingsOut->dwModemOptions |= MDM_BLIND_DIAL;
		}
		
		lpSettingsOut->wWaitBong = LineConfigData.wWaitBong;
		lpSettingsOut->dwBaudRate = LineConfigData.dwBaudRate;

        lpSettingsOut->fwOptions &= ~(MANUAL_DIAL|TERMINAL_PRE|TERMINAL_POST);
		if (LineConfigData.dwTermOptions & NETUI_LCD_TRMOPT_MANUAL_DIAL) {
			lpSettingsOut->fwOptions |= MANUAL_DIAL;
		}
		if (LineConfigData.dwTermOptions & NETUI_LCD_TRMOPT_PRE_DIAL) {
			lpSettingsOut->fwOptions |= TERMINAL_PRE;
		}
		if (LineConfigData.dwTermOptions & NETUI_LCD_TRMOPT_POST_DIAL) {
			lpSettingsOut->fwOptions |= TERMINAL_POST;
		}
		
		lpSettingsOut->ByteSize = LineConfigData.bByteSize;
		lpSettingsOut->StopBits = LineConfigData.bStopBits;
		lpSettingsOut->Parity = LineConfigData.bParity;
		wcscpy (lpSettingsOut->szDialModifier, LineConfigData.szDialModifier);
	} 

	DEBUGMSG (ZONE_MISC, (TEXT("-TSPI_EditMiniConfig: %d\r\n"), fRetVal));
	return fRetVal;
}
 

//
// The following functions are used to verify setting properties of a DEVMINICONFIG via
// calls to lineDevSpecific.
//

BOOL
IsValidBaudRate(
    DWORD dwBaudRate,
    DWORD dwSettableBaud
    )
{
    //
    // Baud rates and settable bits from winbase.h
    //
    switch (dwBaudRate) {
    case CBR_110:   if (dwSettableBaud & BAUD_110)  return TRUE; break;
    case CBR_300:   if (dwSettableBaud & BAUD_300)  return TRUE; break;
    case CBR_600:   if (dwSettableBaud & BAUD_600)  return TRUE; break;
    case CBR_1200:  if (dwSettableBaud & BAUD_1200) return TRUE; break;
    case CBR_2400:  if (dwSettableBaud & BAUD_2400) return TRUE; break;
    case CBR_4800:  if (dwSettableBaud & BAUD_4800) return TRUE; break;
    case CBR_9600:  if (dwSettableBaud & BAUD_9600) return TRUE; break;
    case CBR_14400: if (dwSettableBaud & BAUD_14400)return TRUE; break;
    case CBR_19200: if (dwSettableBaud & BAUD_19200)return TRUE; break;
    case CBR_38400: if (dwSettableBaud & BAUD_38400)return TRUE; break;
    case CBR_56000: if (dwSettableBaud & BAUD_56K)  return TRUE; break;
    case CBR_57600: if (dwSettableBaud & BAUD_57600)return TRUE; break;
    case CBR_115200:if (dwSettableBaud & BAUD_115200) return TRUE; break;
    case CBR_128000:if (dwSettableBaud & BAUD_128K)   return TRUE; break;
    
    case CBR_256000:
    default: if (dwSettableBaud & BAUD_USER) return TRUE; break;
    }
    return FALSE;
}   // IsValidBaudRate


BOOL
IsValidByteSize(
    DWORD dwByteSize,
    DWORD dwSettableData
    )
{
    //
    // Settable bits from winbase.h
    //
    switch (dwByteSize) {
    case 5: if (dwSettableData & DATABITS_5) return TRUE; break;
    case 6: if (dwSettableData & DATABITS_6) return TRUE; break;
    case 7: if (dwSettableData & DATABITS_7) return TRUE; break;
    case 8: if (dwSettableData & DATABITS_8) return TRUE; break;
    case 16:if (dwSettableData & DATABITS_16) return TRUE; break;
    }
    return FALSE;
}   // IsValidByteSize


BOOL
IsValidParity(
    DWORD dwParity,
    DWORD dwSettableParity
    )
{
    //
    // Parity options and settable bits from winbase.h
    //
    switch (dwParity) {
    case NOPARITY:      if (dwSettableParity & PARITY_NONE) return TRUE; break;
    case ODDPARITY:     if (dwSettableParity & PARITY_ODD)  return TRUE; break;
    case EVENPARITY:    if (dwSettableParity & PARITY_EVEN) return TRUE; break;
    case MARKPARITY:    if (dwSettableParity & PARITY_MARK) return TRUE; break;
    case SPACEPARITY:   if (dwSettableParity & PARITY_SPACE)return TRUE; break;
    }
    return FALSE;
}   // IsValidParity


BOOL
IsValidStopBits(
    DWORD dwStopBits,
    DWORD dwSettableStopBits
    )
{
    //
    // Stop bit values and settable bits from winbase.h
    //
    switch (dwStopBits) {
    case ONESTOPBIT:    if (dwSettableStopBits & STOPBITS_10) return TRUE; break;
    case ONE5STOPBITS:  if (dwSettableStopBits & STOPBITS_15) return TRUE; break;
    case TWOSTOPBITS:   if (dwSettableStopBits & STOPBITS_20) return TRUE; break;
    }
    return FALSE;
}   // IsValidStopBits


BOOL
IsBadStringPtr(
    LPCWSTR lpStr,
    DWORD dwMaxLen
    )
{
    DWORD len;
    BOOL bRet;

    bRet = FALSE;

    try {
        len = wcslen(lpStr);
    } except (EXCEPTION_EXECUTE_HANDLER) {
        DEBUGMSG(ZONE_ERROR, (TEXT("UNIMODEM:IsBadStringPtr: Exception!\n")));        
        bRet = TRUE;
    }
    if (bRet) {
        if (len > dwMaxLen) {
            DEBUGMSG(ZONE_ERROR, (TEXT("UNIMODEM:IsBadStringPtr: String too long %d > %d\n"), len, dwMaxLen));
            bRet = TRUE;
        }
    }
    return bRet;
}   // IsBadStringPtr

//
// Function to edit the specific property of a PDEVMINICONFIG
//
LONG
DevSpecificLineConfigEdit(
    PTLINEDEV   pLineDev,
    PUNIMDM_CHG_DEVCFG pChg
    )
{
    DWORD dwSettableBaud;
    DWORD dwSettableData;
    DWORD dwSettableStopParity;
    DWORD rc;

    PDEVMINICFG  pDevMiniCfg;
    LPVARSTRING  lpDevConfig;
    LPWSTR lpszDeviceClass;
    LPWSTR lpszDialModifier;

    DEBUGMSG(ZONE_FUNC, (TEXT("UNIMODEM:+DevSpecificLineConfigEdit\n")));

    if (IsBadReadPtr(pChg, sizeof(UNIMDM_CHG_DEVCFG))) {
        DEBUGMSG(ZONE_ERROR, (TEXT("UNIMODEM:DevSpecificLineConfigEdit: Invalid PUNIMDM_CHG_DEVCFG\n")));
        rc = LINEERR_INVALPOINTER;
        goto exitPoint;
    }

    lpszDeviceClass = (LPWSTR)MapPtrToProcess((LPVOID)pChg->lpszDeviceClass, GetCallerProcess());

    if (!ValidateDevCfgClass(lpszDeviceClass)) {
        DEBUGMSG(ZONE_ERROR, (TEXT("UNIMODEM:DevSpecificLineConfigEdit: LINEERR_INVALDEVICECLASS\n")));
        rc = LINEERR_INVALDEVICECLASS;
        goto exitPoint;
    }
    
    //
    // Validate the buffer pointers
    //
    if (pChg->lpDevConfig == NULL) {
        DEBUGMSG(ZONE_ERROR, (TEXT("UNIMODEM:DevSpecificLineConfigEdit: LINEERR_INVALPOINTER\n")));
        rc = LINEERR_INVALPOINTER;
        goto exitPoint;
    }

    lpDevConfig = (LPVARSTRING)MapPtrToProcess((LPVOID)pChg->lpDevConfig, GetCallerProcess());
    pDevMiniCfg = (PDEVMINICFG)(((LPBYTE)lpDevConfig) + sizeof(VARSTRING));

    //
    // Validate the buffer size
    //
    if (lpDevConfig->dwTotalSize < sizeof(VARSTRING) + sizeof(DEVMINICFG)) {
        DEBUGMSG(ZONE_ERROR, (TEXT("UNIMODEM:DevSpecificLineConfigEdit: LINEERR_STRUCTURETOOSMALL\n")));
        lpDevConfig->dwNeededSize = sizeof(VARSTRING) + sizeof(DEVMINICFG);
        rc = LINEERR_STRUCTURETOOSMALL;
        goto exitPoint;
    }

    lpDevConfig->dwNeededSize = sizeof(VARSTRING) + sizeof(DEVMINICFG);
    lpDevConfig->dwUsedSize   = sizeof(VARSTRING) + sizeof(DEVMINICFG);
    
    //
    // If they specify version 0, then they want the default devconfig. 
    //
    if (pDevMiniCfg->wVersion == 0) {
        getDefaultDevConfig( pLineDev, pDevMiniCfg );        
    }
    
    rc =  LINEERR_INVALPARAM;

    if (pLineDev->DevMiniCfg.wVersion != (pDevMiniCfg->wVersion)) {
        DEBUGMSG(ZONE_ERROR, (TEXT("UNIMODEM:DevSpecificLineConfigEdit: Invalid DevConfig\n")));
        goto exitPoint;
    }

    switch (pChg->dwOption) {
    case UNIMDM_OPT_BAUDRATE:
    case UNIMDM_OPT_BYTESIZE:
    case UNIMDM_OPT_PARITY:
    case UNIMDM_OPT_STOPBITS:
        dwSettableData = dwSettableStopParity = 0;

        GetSettableFields(
            pLineDev,
            &dwSettableBaud,
            (WORD *)&dwSettableData,
            (WORD *)&dwSettableStopParity
            );
    }

    switch (pChg->dwOption) {
    case UNIMDM_OPT_BAUDRATE:
        if (IsValidBaudRate(pChg->dwValue, dwSettableBaud)) {
            pDevMiniCfg->dwBaudRate = pChg->dwValue;
        } else {
            DEBUGMSG(ZONE_ERROR, (TEXT("UNIMODEM:DevSpecificLineConfigEdit: Unsupported baud rate\n")));
            goto exitPoint;
        }
        break;

    case UNIMDM_OPT_BYTESIZE:
        if (IsValidByteSize(pChg->dwValue, dwSettableData)) {
            pDevMiniCfg->ByteSize = (BYTE)pChg->dwValue;
        } else {
            DEBUGMSG(ZONE_ERROR, (TEXT("UNIMODEM:DevSpecificLineConfigEdit: Unsupported byte size\n")));
            goto exitPoint;
        }
        break;

    case UNIMDM_OPT_PARITY:
        if (IsValidParity(pChg->dwValue, dwSettableStopParity)) {
            pDevMiniCfg->Parity = (BYTE)pChg->dwValue;
        } else {
            DEBUGMSG(ZONE_ERROR, (TEXT("UNIMODEM:DevSpecificLineConfigEdit: Unsupported parity\n")));
            goto exitPoint;
        }
        break;

    case UNIMDM_OPT_STOPBITS:
        if (IsValidStopBits(pChg->dwValue, dwSettableStopParity)) {
            pDevMiniCfg->StopBits = (BYTE)pChg->dwValue;
        } else {
            DEBUGMSG(ZONE_ERROR, (TEXT("UNIMODEM:DevSpecificLineConfigEdit: Unsupported stop bits\n")));
            goto exitPoint;
        }
        break;

    case UNIMDM_OPT_WAITBONG:
        pDevMiniCfg->wWaitBong = (WORD)pChg->dwValue;
        break;

    case UNIMDM_OPT_MDMOPTIONS: // MDM_* values from mcx.h
        //
        // Currently CE unimodem only looks at:
        // MDM_BLIND_DIAL
        // MDM_FLOWCONTROL_HARD
        // MDM_FLOWCONTROL_SOFT
        // MDM_SPEED_ADJUST (enables DCC client automatic baud rate detection)
        //
        if (pChg->dwValue > (MDM_V23_OVERRIDE * 2 - 1)) {
            DEBUGMSG(ZONE_ERROR, (TEXT("UNIMODEM:DevSpecificLineConfigEdit: Invalid modem option\n")));
            goto exitPoint;
        }
        pDevMiniCfg->dwModemOptions = pChg->dwValue;
        break;

    case UNIMDM_OPT_TIMEOUT:
        pDevMiniCfg->dwCallSetupFailTimer = pChg->dwValue;
        break;

    case UNIMDM_OPT_TERMOPTIONS:    // NETUI_LCD_TRMOPT_* values from netui.h
        if (pChg->dwValue == 0) {
            pDevMiniCfg->fwOptions = TERMINAL_NONE;
        } else {
            pDevMiniCfg->fwOptions = 0;
            if (pChg->dwValue & NETUI_LCD_TRMOPT_MANUAL_DIAL) {
                pDevMiniCfg->fwOptions |= MANUAL_DIAL;
            }
            if (pChg->dwValue & NETUI_LCD_TRMOPT_PRE_DIAL) {
                pDevMiniCfg->fwOptions |= TERMINAL_PRE;
            }
            if (pChg->dwValue & NETUI_LCD_TRMOPT_POST_DIAL) {
                pDevMiniCfg->fwOptions |= TERMINAL_POST;
            }
        }
        break;

    case UNIMDM_OPT_DIALMOD:
        lpszDialModifier = (LPWSTR)MapPtrToProcess((LPVOID)pChg->dwValue, GetCallerProcess());
        if (IsBadStringPtr(lpszDialModifier, DIAL_MODIFIER_LEN)) {
            DEBUGMSG(ZONE_ERROR, (TEXT("UNIMODEM:DevSpecificLineConfigEdit: Invalid dial modifier\n")));
            goto exitPoint;
        }
        wcscpy(pDevMiniCfg->szDialModifier, lpszDialModifier);
        break;
        
    default:
        DEBUGMSG(ZONE_ERROR, (TEXT("UNIMODEM:DevSpecificLineConfigEdit: Invalid dwOption\n")));
        goto exitPoint;
    }
    
    rc = 0;

exitPoint:
    DEBUGMSG(ZONE_FUNC, (TEXT("UNIMODEM:-DevSpecificLineConfigEdit: returning 0x%x\n"), rc));
    return rc;
}

