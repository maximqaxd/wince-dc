/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

Module Name:  

pegdser.h

Abstract:  

Notes: 


--*/
//	@doc	EXTERNAL SERIALDEVICE

#ifndef __PEGDSER_H__
#define __PEGDSER_H__

#ifdef __cplusplus
extern "C" {
#endif

// We'll need some defines
#include "windev.h"

//
// DeviceIOControl dwIoControlCode values for Serial ports
//
#define IOCTL_SERIAL_SET_BREAK_ON       CTL_CODE(FILE_DEVICE_SERIAL_PORT, 1,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_SET_BREAK_OFF      CTL_CODE(FILE_DEVICE_SERIAL_PORT, 2,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_SET_DTR            CTL_CODE(FILE_DEVICE_SERIAL_PORT, 3,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_CLR_DTR            CTL_CODE(FILE_DEVICE_SERIAL_PORT, 4,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_SET_RTS            CTL_CODE(FILE_DEVICE_SERIAL_PORT, 5,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_CLR_RTS            CTL_CODE(FILE_DEVICE_SERIAL_PORT, 6,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_SET_XOFF           CTL_CODE(FILE_DEVICE_SERIAL_PORT, 7,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_SET_XON            CTL_CODE(FILE_DEVICE_SERIAL_PORT, 8,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_GET_WAIT_MASK      CTL_CODE(FILE_DEVICE_SERIAL_PORT, 9,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_SET_WAIT_MASK      CTL_CODE(FILE_DEVICE_SERIAL_PORT,10,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_WAIT_ON_MASK       CTL_CODE(FILE_DEVICE_SERIAL_PORT,11,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_GET_COMMSTATUS     CTL_CODE(FILE_DEVICE_SERIAL_PORT,12,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_GET_MODEMSTATUS    CTL_CODE(FILE_DEVICE_SERIAL_PORT,13,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_GET_PROPERTIES     CTL_CODE(FILE_DEVICE_SERIAL_PORT,14,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_SET_TIMEOUTS       CTL_CODE(FILE_DEVICE_SERIAL_PORT,15,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_GET_TIMEOUTS       CTL_CODE(FILE_DEVICE_SERIAL_PORT,16,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_PURGE              CTL_CODE(FILE_DEVICE_SERIAL_PORT,17,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_SET_QUEUE_SIZE     CTL_CODE(FILE_DEVICE_SERIAL_PORT,18,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_IMMEDIATE_CHAR     CTL_CODE(FILE_DEVICE_SERIAL_PORT,19,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_GET_DCB		CTL_CODE(FILE_DEVICE_SERIAL_PORT,20,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_SET_DCB		CTL_CODE(FILE_DEVICE_SERIAL_PORT,21,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_ENABLE_IR		CTL_CODE(FILE_DEVICE_SERIAL_PORT,22,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_DISABLE_IR		CTL_CODE(FILE_DEVICE_SERIAL_PORT,23,METHOD_BUFFERED,FILE_ANY_ACCESS)


//	@struct SERIAL_QUEUE_SIZES | Structure used to specify queue sizes
//	@xref <f ttt_IOControl> <f IOCTL_SERIAL_SET_QUEUE_SIZE>
typedef struct _SERIAL_QUEUE_SIZES {
	DWORD	cbInQueue;		//@field The number of bytes requested for
							//		 the input queue
	DWORD	cbOutQueue;		//@field The number of bytes requested for
							//		 the output queue
} SERIAL_QUEUE_SIZES, *PSERIAL_QUEUE_SIZES;

//  @struct SERIAL_DEV_STATUS | Structure used to get communication
//		status.
//	@xref <f ttt_IOControl> <f IOCTL_SERIAL_GET_COMMSTATUS>
typedef struct _SERIAL_DEV_STATUS {
	DWORD	Errors;			//@field mask of error type
		//		@tab2	Value |	Meaning
		//		@tab2	CE_BREAK | The hardware detected a break
		//			condition.
		//		@tab2	CE_FRAME | The hardware detected a framing
		//			error.
		//		@tab2	CE_IOE | An I/O error occurred during
		//			communications with the device.
		//		@tab2	CE_MODE | The requested
		//			mode is not supported, or the hCommDev parameter is
		//			invalid. If this value is specified, it is the only valid
		//			error.
		//		@tab2	CE_OVERRUN | A character-buffer overrun has
		//			occurred. The next character is lost.
		//		@tab2	CE_RXOVER | An input buffer overflow has occurred.
		//			There is either no room in the input buffer, or a
		//			character was received after the end-of-file (EOF)
		//			character.
		//		@tab2	CE_RXPARITY | The hardware detected a parity error.
		//		@tab2	CE_TXFULL | The application tried to transmit a
		//			character, but the output buffer was full.
		//		@tab2	CE_DNS | The parallel device is not selected.
		//		@tab2	CE_PTO | A time-out occurred on a parallel device.
		//		@tab2	CE_OOP | The parallel device signaled that it is out
		//			of paper.
	COMSTAT	ComStat;		//@field ComStat structure.
} SERIAL_DEV_STATUS, *PSERIAL_DEV_STATUS;

#ifdef __cplusplus
}
#endif

#endif // __PEGDSER_H_
