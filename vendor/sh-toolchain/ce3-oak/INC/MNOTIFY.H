/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

Module Name:  

mnotify.h

Abstract:  

Private portion of notify.h - Header for User Notifications

Notes: 


--*/
#ifndef _MACRO_NOTIFY_H_
#define _MACRO_NOTIFY_H_

#ifdef __cplusplus
extern "C" {
#endif

    
// SDK functions
#define CeGetUserNotificationPreferences IMPLICIT_DECL(BOOL,   SH_GDI, 101, (HWND hWndParent, PCE_USER_NOTIFICATION lpNotification))
#define CeSetUserNotificationEx			 IMPLICIT_DECL(HANDLE, SH_GDI, 102, (HANDLE hNotification, CE_NOTIFICATION_TRIGGER *pcent, CE_USER_NOTIFICATION *pceun))
#define CeClearUserNotification			 IMPLICIT_DECL(BOOL,   SH_GDI, 103, (HANDLE hNotification))
#define CeRunAppAtEvent					 IMPLICIT_DECL(BOOL,   SH_GDI, 104, (TCHAR *pwszAppName, LONG lWhichEvent))
#define CeHandleAppNotifications		 IMPLICIT_DECL(BOOL,   SH_GDI, 105, (TCHAR *pwszAppName))
#define CeGetUserNotificationHandles	 IMPLICIT_DECL(BOOL,   SH_GDI, 106, (HANDLE *rghNotifications, DWORD cHandles, DWORD *pcHandlesNeeded))
#define CeGetUserNotification			 IMPLICIT_DECL(BOOL,   SH_GDI, 107, (HANDLE hNotif, DWORD cByes, DWORD *pcBytesNeeded, BYTE *pBuffer))
#define CeRunAppAtTime                   IMPLICIT_DECL(BOOL,   SH_GDI, 112, (TCHAR *pwszAppName, SYSTEMTIME *pstTime))

 // OAK functions
#define CeEventHasOccurred				 IMPLICIT_DECL(BOOL,   SH_GDI, 108, (LONG lWhichEvent, TCHAR *pwszEndOfCommandLine))
      
#ifdef __cplusplus
}
#endif

#endif /* _MACRO_NOTIFY_H_ */
