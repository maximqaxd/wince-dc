/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

Module Name:  

oemfw.h

Abstract:  

This include files defines all the exports an OEM needs
to supply.

Notes: 


--*/

#ifndef _OEMFW_H_
#define _OEMFW_H_    1

#ifdef __cplusplus
extern "C" {
#endif

#define PAR_READ    1
#define PAR_WRITE   2

extern  const unsigned int  FWStackAddr;
extern  const unsigned int  fFastDownload;
extern  unsigned int * const pFastDownloadCodeArea;

int
PowerOnSelfTest(
    void
    );

int
SerialPortInitialize(
    void
    );

int
SerialPortWrite(
    const unsigned char *   pBytesToSend,
    unsigned int            cb
    );

int
SerialPortRead(
    unsigned char * pByteReceived,
    unsigned int    cb
    );

int
ParallelPortInitialize(
    void
    );

int
ParallelPortWrite(
    const unsigned char *   pBytesToSend,
    unsigned int            cb
    );

int
ParallelPortRead(
    unsigned char * pByteReceived,
    unsigned int    cb
    );

void
ParallelPortReadEnd(
    void
    );

int
ParallelPortDataReady(
    unsigned char * pByteReceived
    );

int
ParallelPortSetDirection(
    int nDirection
    );

unsigned int uTimerInit(void);

unsigned int uTimerRead(void);

void uTimerWait(unsigned int nUSec);

#ifdef __cplusplus
}
#endif


#endif  // #ifndef _OEMFW_H_
