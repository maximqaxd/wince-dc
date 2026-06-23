/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

Module Name:

 print.h

Abstract:


Functions:




Notes:


--*/

#ifndef __PPAPI_H__
#define __PPAPI_H__

/////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif


HANDLE WINAPI PrinterOpen(PWSTR sPort);
BOOL   WINAPI PrinterSend(HANDLE hPrint, LPCVOID lpBuffer, DWORD dwBytes);
VOID   WINAPI PrinterClose(HANDLE hPrint);


DWORD WINAPI GetPrinterInfo(HANDLE hPrint, int Opt, PVOID pBuf, PINT pBufLen);

#define PRINTER_ID  1
#define PRINTER_PEN 2

#define PEN_BLACK   0x01
#define PEN_COLOR   0x02


BOOL WINAPI ReportPrinterStatus(DWORD dwStatus, LPCTSTR lpPortName, LPCTSTR lpPrinterName, LPARAM lparam);

#define PRINTER_ERROR_MASK	0x80000000
#define PRINTER_WARN_MASK	0x40000000
#define PRINTER_NOTIFY_MASK	0x20000000
#define PRINTER_VENDOR_MASK	0x10000000
 
#define PRINTER_E_OFFLINE		0x80000001
#define PRINTER_E_NOPAPER		0x80000002
#define PRINTER_E_DEVICE_FAULT	0x80000003
#define PRINTER_E_TIMEOUT		0x80000004
#define PRINTER_E_PAPERJAM		0x80000005

#define PRINTER_N_JOB_COMPLETE	0x20000001
#define PRINTER_N_PRINTING		0x20000002


#ifdef __cplusplus
}
#endif
/////////////////////////////////////////////////////////////////////////////
	
#endif /* __PPAPI_H__ */
