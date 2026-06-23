/*++

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

Module Name:

    fwentry.h

Abstract:

    include file defines firmware entries.

Note:


--*/
#ifndef _FWENTRY_H_
#define _FWENTRY_H_    1

#include <fwtblidx.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void            (*PFUNC_V_V)(void);
typedef void            (*PFUNC_V_PC)(unsigned char *);
typedef void            (*PFUNC_V_PC_)(unsigned char *, ...);
typedef void            (*PFUNC_V_I)(int);
typedef int             (*PFUNC_I_V)(void);
typedef int             (*PFUNC_I_PCUI)(unsigned char *, unsigned int);
typedef int             (*PFUNC_I_PPCUI)(unsigned char **, unsigned int);

#define FW_FlushICache()                ((PFUNC_V_V)   (FWTBL_FLUSHI))()
#define FW_FlushDCache()                ((PFUNC_V_V)   (FWTBL_FLUSHD))()
#define FW_OutputString(s)              ((PFUNC_V_PC)  (FWTBL_OUTPUTS))(s)
#define FW_OutputFormatString(s)        ((PFUNC_V_PC_) (FWTBL_OUTPUTFORMATS)) s
#define FW_SerialPortRead(buf, cb)      ((PFUNC_I_PCUI)(FWTBL_SERIALREAD))(buf, cb)
#define FW_SerialPortWrite(buf, cb)     ((PFUNC_I_PCUI)(FWTBL_SERIALWRITE))(buf, cb)
#define FW_ParallelPortRead(buf, cb)    ((PFUNC_I_PCUI)(FWTBL_PARALLELREAD))(buf, cb)
#define FW_ParallelPortWrite(buf, cb)   ((PFUNC_I_PCUI)(FWTBL_PARALLELWRITE))(buf, cb)
#define FW_EnterDebugger()              ((PFUNC_V_V)   (FWTBL_ENTERDEBUGGER))()

#ifdef __cplusplus
}
#endif

#endif  // #ifndef _FWENTRY_H_
