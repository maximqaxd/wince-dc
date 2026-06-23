/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

Module Name:  

pnotify.h

Abstract:

Private portion of notify.h - Header for User Notifications

Notes: 


--*/
#ifndef _PRIV_NOTIFY_H_
#define _PRIV_NOTIFY_H_

#ifdef __cplusplus
extern "C"  {
#endif

BOOL CeEventHasOccurred (LONG lWhichEvent, TCHAR *pwszEndOfCommandLine);

#ifdef __cplusplus
}
#endif /*__cplusplus*/

#endif /* _PRIV_NOTIFY_H_ */
