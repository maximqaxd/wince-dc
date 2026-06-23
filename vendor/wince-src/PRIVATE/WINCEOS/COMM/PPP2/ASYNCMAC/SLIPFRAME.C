/*++

Copyright (c) 1993-2000 Microsoft Corporation.  All rights reserved.

Module Name:

    pppframe.c

Abstract:

Environment:

--*/

#include "windows.h"
#include "tapi.h"
#include "ndis.h"
#include "ndiswan.h"
#include "ndistapi.h"
#include "asyncmac.h"
#include "frame.h"


VOID
AssembleSLIPFrame(
	IN		PASYNCMAC_OPEN_LINE pOpenLine,
	IN	OUT	PNDIS_WAN_PACKET	WanPacket)
//
//	Encode an IP packet into a SLIP frame. 
//
//             +---------------------+
//             | IP packet           |
//             +---------------------+
//            /                       \
//           /                         \
//       +--+---------------------------+--+
//       |C0| Escaped IP packet         |C0|
//       +--+---------------------------+--+
//
//	The transformation is done within the WanPacket buffer.
//	Recall that in the function MpInit we set the HeaderPadding in the WAN_INFO structure
//	to a sufficient size to allow for the addition of the leading flag and the expansion
//  of the PPP frame data.  Also, the TailPadding was set to allow for the addition of
//  the escaped CRC and trailing flag.
//
{
	PUCHAR		pOldFrame;
	PUCHAR		pNewFrame;
	int			dataSize;
	UCHAR		c;

	ASSERT(WanPacket->StartBuffer   <= WanPacket->EndBuffer);
	ASSERT(WanPacket->StartBuffer   <= WanPacket->CurrentBuffer);
	ASSERT(WanPacket->CurrentBuffer + WanPacket->CurrentLength <= WanPacket->EndBuffer);

	//
	// Initialize locals
	//

    pOldFrame = WanPacket->CurrentBuffer;

    pNewFrame = WanPacket->StartBuffer;

	//
	// for quicker access, get a copy of data length field
	//
	dataSize = WanPacket->CurrentLength;

	*pNewFrame++ = SLIP_END; //  mark beginning of frame

	//
	// Copy oldFrame to newFrame, escaping all ESC and END chars
	//
	while (dataSize--)
	{
		c = *pOldFrame++;


		//
		// Make sure we aren't overwriting the next byte of the old frame to be processed
		//
		ASSERT(dataSize == 0 || pNewFrame != pOldFrame);

		//
		// Check if we have to escape out this byte or not
		//
		if (c == SLIP_ESC)
		{
			*pNewFrame++ = SLIP_ESC;
			c = SLIP_ESC_ESC;
		}
		else if (c == SLIP_END)
		{
			*pNewFrame++ = SLIP_ESC;
			c = SLIP_ESC_END;
		}

		ASSERT(dataSize == 0 || pNewFrame != pOldFrame);

		*pNewFrame++ = c;
	}

	//
	// Mark end of frame
	//
	*pNewFrame++ = SLIP_END;

	//
	// Calc how many bytes we expanded to including start and end bytes
	//
	WanPacket->CurrentLength = (ULONG)(pNewFrame - WanPacket->StartBuffer);

	//
	// Put in the adjusted length -- actual num of bytes to send
	//

	WanPacket->CurrentBuffer = WanPacket->StartBuffer;

	DEBUGMSG(ZONE_SEND | ZONE_FUNCTION, 
		(TEXT("PPP: -AssembleSLIPFrame: Frame: start=%x end=%x length=%x\n"), WanPacket->CurrentBuffer, pNewFrame, WanPacket->CurrentLength));

	ASSERT(WanPacket->CurrentBuffer + WanPacket->CurrentLength <= WanPacket->EndBuffer);
}
