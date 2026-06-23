/*---------------------------------------------------------------------------*\
 *
 * (c) Copyright Microsoft Corp. 1994 All Rights Reserved
 *
 *  module: print.h
 *  date:
 *  author: tonykit
 *
 *  purpose: 
 *
\*---------------------------------------------------------------------------*/
#ifndef __PRINT_H__
#define __PRINT_H__

/////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif

#define PRINT_IR_PORT                   6
#define PRINT_SERIAL_PORT               8
#define PRINT_ERROR                     ((DWORD)-1)

BOOL   WINAPI CEChoosePrinter(HWND hwndParent, DWORD *lpdwPort);
HANDLE WINAPI CEPrinterOpen(DWORD dwPort);
BOOL   WINAPI CEPrinterSend(HANDLE hPrint, LPCVOID lpBuffer, DWORD dwBytes);
VOID   WINAPI CEPrinterClose(HANDLE hPrint);


#ifdef __cplusplus
}
#endif
/////////////////////////////////////////////////////////////////////////////
	
#endif /* __PRINT_H__ */
