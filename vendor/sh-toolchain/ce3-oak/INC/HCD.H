// Copyright (c) 1999-2000 Microsoft Corporation.  All rights reserved.
#ifndef     __HCD_H__
#define     __HCD_H__

#ifndef     __USBTYPES_H__
#include    "usbtypes.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct  _HCD_FUNCS;
typedef struct _HCD_FUNCS HCD_FUNCS, * PHCD_FUNCS, * LPHCD_FUNCS;
typedef struct _HCD_FUNCS const * PCHCD_FUNCS;
typedef struct _HCD_FUNCS const * LPCHCD_FUNCS;

/******************************************************************************/
// Entry Points for USBD

BOOL HcdAttach(LPVOID lpvHcd, LPCHCD_FUNCS lpHcdFuncs, LPLPVOID lppvContext);
BOOL HcdDetach(LPVOID lpvContext);

BOOL HcdDeviceAttached(LPVOID lpvContext, UINT iDevice,
                       LPCUSB_DEVICE lpDeviceInfo, LPLPVOID lppvDeviceDetach);
BOOL HcdDeviceDetached(LPVOID lpvDeviceDetach);



/******************************************************************************/
// HCD Entry Points
// NOTE: the USBD does not statically call these.  It calls them through
// a table of pointers that gets passed to it from the HCD (either OHIC or UHCI)


//USB Subsystem Commands
typedef BOOL (* LPHCD_GET_FRAME_NUMBER)(LPVOID, LPDWORD);
BOOL HcdGetFrameNumber(LPVOID lpvHcd, LPDWORD lpdwFrameNumber);
typedef BOOL (* LPHCD_GET_FRAME_LENGTH)(LPVOID, LPUSHORT);
BOOL HcdGetFrameLength(LPVOID lpvHcd, LPUSHORT lpuFrameLength);
typedef BOOL (* LPHCD_ADJUST_FRAME_LENGTH)(LPVOID, INT);
BOOL HcdAdjustFrameLength(LPVOID lpvHcd, INT nFrameAdjustment); // only (1, -1)


//Controls pipe handling
typedef BOOL (* LPHCD_OPEN_PIPE)(LPVOID, UINT, DWORD,
                                 LPCUSB_ENDPOINT_DESCRIPTOR, LPUINT);
BOOL HcdOpenPipe(LPVOID lpvHcd, UINT iDevice, DWORD dwFlags,
                 LPCUSB_ENDPOINT_DESCRIPTOR pEndpointDescriptor,
                 LPUINT lpiEndpointIndex);
typedef BOOL (* LPHCD_CLOSE_PIPE)(LPVOID, UINT, UINT);
BOOL HcdClosePipe(LPVOID lpvHcd, UINT iDevice, UINT iEndpointIndex);


//Transfer commands
typedef BOOL (* LPHCD_ISSUE_TRANSFER)(LPVOID, UINT, UINT,
                                      LPTRANSFER_NOTIFY_ROUTINE, LPVOID, DWORD,
                                      LPCUSB_DEVICE_REQUEST, DWORD, UINT,
                                      LPCDWORD, DWORD, LPVOID, ULONG, LPCVOID,
                                      LPDWORD, LPDWORD, LPBOOL, LPDWORD,
                                      LPDWORD);
BOOL HcdIssueTransfer(LPVOID lpvHcd, UINT iDevice, UINT iEndpointIndex,
                      LPTRANSFER_NOTIFY_ROUTINE lpStartAddress,
                      LPVOID lpvNotifyParameter, DWORD dwFlags,
                      LPCUSB_DEVICE_REQUEST lpControlHeader,
                      DWORD dwStartingFrame, UINT nOffsets, LPCDWORD aOffsets,
                      DWORD dwBufferSize, LPVOID lpvBuffer, ULONG paBuffer,
                      LPCVOID lpvCancelId, LPDWORD adwIsochErrors,
                      LPDWORD adwIsochLengths, LPBOOL lpfComplete,
                      LPDWORD lpdwBytesTransfered, LPDWORD lpdwError);

typedef BOOL (* LPHCD_ABORT_TRANSFER)(LPVOID, UINT, UINT,
                                      LPTRANSFER_NOTIFY_ROUTINE, LPVOID,
                                      LPCVOID);
BOOL HcdAbortTransfer(LPVOID lpvHcd, UINT iDevice, UINT iEndpointIndex,
                      LPTRANSFER_NOTIFY_ROUTINE lpStartAddress,
                      LPVOID lpvNotifyParameter, LPCVOID lpvCancelId);

struct _HCD_FUNCS {
    DWORD                       dwCount;

    LPHCD_GET_FRAME_NUMBER      lpGetFrameNumber;
    LPHCD_GET_FRAME_LENGTH      lpGetFrameLength;
    LPHCD_ADJUST_FRAME_LENGTH   lpAdjustFrameLength;
    LPHCD_OPEN_PIPE             lpOpenPipe;
    LPHCD_CLOSE_PIPE            lpClosePipe;
    LPHCD_ISSUE_TRANSFER        lpIssueTransfer;
    LPHCD_ABORT_TRANSFER        lpAbortTransfer;
};

#ifdef __cplusplus
}
#endif

#endif

