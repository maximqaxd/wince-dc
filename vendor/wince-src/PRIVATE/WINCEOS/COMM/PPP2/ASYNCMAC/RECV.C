/*++

Copyright (c) 1999-2000 Microsoft Corporation.  All rights reserved.

Module Name:

	recv.c

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

#include "cclib.h"

//
//	The receive packet data buffer is allocated so as to contain:
//
//	1. 128 bytes of header space reserved for PPP Van Jacobsen TCP/IP header decompression, plus
//	2. 2 bytes for PPP protocol field, plus
//	3. MaxFrameSize (typically 1500) bytes of PPP frame data, plus
//  4. 2 bytes for a 16-bit CRC
//
//	Note that the recv packet buffer does not need to deal with escaped bytes or flags.
//	Those bytes are processed by the receive state machine and so are never stored in the receive packet buffer.
//	See RFC 1661 Section 6.1 description of Maximum-Receive-Unit
//
#define HEADER_BYTES_RESERVED_FOR_VAN_JACOBSEN_DECOMPRESSION	128
#define MAX_PROTOCOL_FIELD_LENGTH								2

//
//	Activesync 3.1 seems to send frames that are larger than the default MTU.
//	So, we add some extra bytes factor to allow it to work with this asyncmac driver.
//
#define ACTIVESYNC31_EXTRA_BYTES	50

#define TRAILER_BYTES_FOR_CRC									2
#define RECEIVE_PACKET_SIZE(pAdapter)	(HEADER_BYTES_RESERVED_FOR_VAN_JACOBSEN_DECOMPRESSION + \
										 MAX_PROTOCOL_FIELD_LENGTH + \
										(pAdapter)->Info.MaxFrameSize + \
										ACTIVESYNC31_EXTRA_BYTES + \
										TRAILER_BYTES_FOR_CRC)

void
ProcessEndOfPacket(
	PASYNCMAC_OPEN_LINE	pOpenLine,
	PBYTE				pRecvPacket,
	DWORD				cbPacketData)
//
//	Process packet data when the closing flag is received.
//
//	Check the CRC and indicate the the packet to the protocol if it is good.
//
{
	USHORT				CRCData;
	NDIS_STATUS			Status;
	PASYNCMAC_ADAPTER	pAdapter = pOpenLine->pAdapter;
	BOOLEAN				bGoodCRC = TRUE;

	DEBUGMSG (ZONE_FUNCTION | ZONE_RECV, (TEXT("AsyncMac: +ProcessEndOfPacket: length=%d\n"), cbPacketData));

	if (pOpenLine->WanLinkInfo.RecvFramingBits & PPP_FRAMING)
	{
		// Packet must contain at least 1 byte of data + 2 CRC bytes to be valid
		if (cbPacketData < (1 + TRAILER_BYTES_FOR_CRC))
		{
			DEBUGMSG (ZONE_RECV, (TEXT("MaxRxThread: Discarding short packet of length %d\n"), cbPacketData));
			return;
		}

		// Extract the 16 bit CRC from the last two bytes of the packet data
		// Note that the LSB of the CRC is transmitted first

		CRCData = (pRecvPacket[cbPacketData-1] << 8) | pRecvPacket[cbPacketData-2];
		CRCData ^= 0xFFFF;
		cbPacketData -= TRAILER_BYTES_FOR_CRC;

		// Validate the CRC

		bGoodCRC = CRCData == CalcCRCPPP (pRecvPacket, cbPacketData);
	}

	if (bGoodCRC)
	{
		DEBUGMSG (ZONE_RECV, (TEXT("AsyncMac: MaxRxThread: Have good packet\n")));

		NdisMWanIndicateReceive(&Status, pAdapter->hMiniportAdapter, pOpenLine->hNdisLinkContext, pRecvPacket, cbPacketData);
		NdisMWanIndicateReceiveComplete (pAdapter->hMiniportAdapter, pOpenLine->hNdisLinkContext);

		if (NDIS_STATUS_SUCCESS != Status)
		{
			DEBUGMSG (ZONE_ERROR, (TEXT("MacRxThread: ERROR: NdisMWanIndicateReceive Status=0x%X\n"), Status));
		}
	}
	else
	{
		DEBUGMSG (ZONE_RECV, (TEXT("AsyncMac: CRC Error 0x%X != 0x%X\n"),
					 CRCData, CalcCRCPPP (pRecvPacket, cbPacketData)));
	}
}

DWORD WINAPI
MacRxThread (LPVOID pVArg)
{
	PASYNCMAC_OPEN_LINE	pOpenLine;
	PBYTE				pRecvPacketStart,
						pRecvPacket,
						pRecvPacketCurrent,
						pRecvPacketEnd,
						pRecvBuffer;
	BOOL				bByteIsEscaped,
						bFrameTooLong,
						bPppFraming;
	COMMTIMEOUTS		CommTimeouts;
	DWORD				Error,
						cbRecvBuffer;

	// Establish a reference to openline
	pOpenLine = GetOpenLinePtr(pVArg);
	if (pOpenLine == NULL)
	{
		DEBUGMSG (ZONE_ERROR | ZONE_RECV, (TEXT("\n-MacRxThread: Unable to get reference to pOpenLine\n")));
		return 0;
	}

	ASSERT (CHK_AOL(pOpenLine));
	ASSERT (CHK_AA(pOpenLine->pAdapter));
	ASSERT (pOpenLine->hPort != NULL);
	
	bPppFraming = (pOpenLine->WanLinkInfo.RecvFramingBits & PPP_FRAMING) != 0;

	// We are interested in the following events on the comm port:
	//		EV_RXCHAR - data has been received
	//		EV_RLSD	  - carrier detect line changed state
	//		EV_POWER  - WinCE power event (port powered down)

	SetCommMask(pOpenLine->hPort, EV_RXCHAR | EV_RLSD | EV_POWER );	

	// Configure the port timeouts to:
	//		1. Return immediately on a ReadFile (that is, do not block waiting for bytes to arrive)
	//		2. Allow 500 ms + 2ms/byte before timing out a WriteFile

    GetCommTimeouts( pOpenLine->hPort, &CommTimeouts );
    CommTimeouts.ReadIntervalTimeout = MAXDWORD;
    CommTimeouts.ReadTotalTimeoutMultiplier = 0;
    CommTimeouts.ReadTotalTimeoutConstant = 0;
    CommTimeouts.WriteTotalTimeoutMultiplier = 2;
    CommTimeouts.WriteTotalTimeoutConstant = 500;
    SetCommTimeouts( pOpenLine->hPort, &CommTimeouts );

	//
	//	Allocate the receive packet that we will fill in and indicate to PPP.
	//
	pRecvPacketStart = AsyncMacAllocateMemory(RECEIVE_PACKET_SIZE(pOpenLine->pAdapter));
	if (pRecvPacketStart == NULL)
	{
		DEBUGMSG (ZONE_RECV, (TEXT("MacRxThread: Unable to allocate RecvPacket %d bytes\n"), RECEIVE_PACKET_SIZE(pOpenLine->pAdapter)));
		CloseHandle( pOpenLine->hRxThrd);
		pOpenLine->hRxThrd = NULL;
		ReleaseOpenLinePtr(pOpenLine);
		return 0;
	}

	//
	//	Allocate the receive buffer passed to ReadFile to retrieve bytes from serial port.
	//
	cbRecvBuffer = pOpenLine->pAdapter->dwRecvBufSize;
	pRecvBuffer = AsyncMacAllocateMemory(cbRecvBuffer);
	if (pRecvBuffer == NULL)
	{
		DEBUGMSG (ZONE_RECV, (TEXT("MacRxThread: Unable to allocate RecvBuffer %d bytes\n"), cbRecvBuffer));
		CloseHandle( pOpenLine->hRxThrd);
		pOpenLine->hRxThrd = NULL;
		AsyncMacFreeMemory (pRecvPacketStart, RECEIVE_PACKET_SIZE(pOpenLine->pAdapter));
		ReleaseOpenLinePtr(pOpenLine);
		return 0;
	}

	// Get a pointer to the portion of the receive packet that gets filled in by AsyncMac
	pRecvPacket = pRecvPacketStart + HEADER_BYTES_RESERVED_FOR_VAN_JACOBSEN_DECOMPRESSION;

	pRecvPacketEnd = pRecvPacketStart + RECEIVE_PACKET_SIZE(pOpenLine->pAdapter);
	bByteIsEscaped = FALSE;
	pRecvPacketCurrent = pRecvPacket;
	bFrameTooLong = TRUE;				// Setting this TRUE forces us to see a flag byte (0x7E) before first packet

	while (pOpenLine->dwFlags & AOL_FLAGS_SENT_LINE_UP)
	{
		DWORD				Mask;
		DWORD				ModemStatus;

		DEBUGMSG (ZONE_RECV, (TEXT("AsyncMac: WaitCommEvent...\n"), Mask));

		if ( WaitCommEvent( pOpenLine->hPort, &Mask, NULL ) == TRUE )
		{
			ASSERT(AsyncMacGuardRegionOk(pOpenLine, sizeof(ASYNCMAC_OPEN_LINE)));
			ASSERT(AsyncMacGuardRegionOk(pOpenLine->pAdapter, sizeof(ASYNCMAC_ADAPTER)));
			ASSERT(AsyncMacGuardRegionOk(pRecvPacketStart, RECEIVE_PACKET_SIZE(pOpenLine->pAdapter)));

			DEBUGMSG (ZONE_RECV, (TEXT("AsyncMac: ...WaitCommEvent returned: Mask=0x%X\n"), Mask));

			if (Mask & (EV_POWER | EV_RLSD))
			{
				// Power loss/DCD change of state Detection

				ModemStatus = 0;
				(void)GetCommModemStatus(pOpenLine->hPort, &ModemStatus);

				if ((Mask & EV_POWER) || !(ModemStatus & MS_RLSD_ON))
				{
					DEBUGMSG( ZONE_RECV | ZONE_ERROR,( TEXT( "AsyncMac: Power=%hs CD=%hs\n"),
						Mask & EV_POWER ? "OFF" : "ON", ModemStatus & MS_RLSD_ON ? "ON" : "OFF"));

					// Indicate Mac down due to loss of power or carrier detect (CD)

					SendLineDown(pOpenLine);
					break;
				}
			}

			if (Mask & EV_RXCHAR)
			{
				// Receive data event

				BYTE	byte;
				DWORD	cbReceiveData, dwBytesRead;
				PBYTE	pbReceiveData;


				DEBUGMSG(ZONE_RECV, (TEXT( "AsyncMac:RX avail\n" )));

                // Read & process data till no more is available.
                do {
				pbReceiveData = pRecvBuffer;
				if (ReadFile(pOpenLine->hPort, pbReceiveData, cbRecvBuffer, &cbReceiveData, 0 ) == FALSE)
				{
					// Serial functions will return an error if a PCMCIA card has
					// been removed. If the error is INVALID_HANDLE or GEN_FAILURE
					// the PCMCIA card was removed. In this case the MAC layer is
					// down.

					Error = GetLastError();
					DEBUGMSG( ZONE_RECV | ZONE_ERROR, (TEXT( "AsyncMac:ReadFile failed %d\n"), Error));

					if( ((ERROR_INVALID_HANDLE == Error)  || Error == ERROR_GEN_FAILURE)
					&& 	!(pOpenLine->dwFlags & AOL_FLAGS_ERROR_INDICATED) )
					{
						pOpenLine->dwFlags |= AOL_FLAGS_ERROR_INDICATED;
						SendLineDown (pOpenLine);
					}
					goto SerialError;
				}

                dwBytesRead = cbReceiveData;
                
				DEBUGMSG(ZONE_RECV, (TEXT("->mac:RX %d bytes, spaceLeft=%d pCurr=%x\n" ), cbReceiveData, pRecvPacketEnd - pRecvPacketCurrent, pRecvPacketCurrent));
#ifdef DEBUG
				if (ZONE_RECV)
				{
					DEBUGMSG(1, (TEXT("\nAsyncMac ReadFile:\n")));
					DumpMem(pbReceiveData, cbReceiveData);
					DEBUGMSG(1, (TEXT("\n\n")));
				}
#endif

				// Run the data through the RX packet state machine
				//
				// Note that the state machine does not distinguish between opening and closing
				// flags.  Any flag both terminates a current packet (if any) and begins a new
				// packet.  This allows a shared opening/closing flag between packets.

				while (cbReceiveData--)
				{
					byte = *pbReceiveData++;

					//
					//	Note the sequence ESC FLAG is illegal, so we don't care whether
					//	we are in the escaped state or not when checking for a flag.
					//
					if ( bPppFraming && byte == PPP_FLAG_BYTE
					|| (!bPppFraming && byte == SLIP_END))
					{
						if (pRecvPacketCurrent > pRecvPacket && !bFrameTooLong)
						{
							// Indicate the packet to the protocol as appropriate
							ASSERT(AsyncMacGuardRegionOk(pRecvPacketStart, RECEIVE_PACKET_SIZE(pOpenLine->pAdapter)));
							ProcessEndOfPacket(pOpenLine, pRecvPacket, pRecvPacketCurrent - pRecvPacket);
							ASSERT(AsyncMacGuardRegionOk(pRecvPacketStart, RECEIVE_PACKET_SIZE(pOpenLine->pAdapter)));
						}

						// Reinitialize state for the next packet
						pRecvPacketCurrent = pRecvPacket;
						bFrameTooLong = FALSE;
						bByteIsEscaped = FALSE;
					}
					else if ( bPppFraming && byte == PPP_ESC_BYTE
						 || (!bPppFraming && byte == SLIP_ESC))
					{
						bByteIsEscaped = TRUE;
					}
					else // Just a regular byte
					{
						// Check that there is space left to store the byte

						if (pRecvPacketCurrent < pRecvPacketEnd)
						{
							if (bByteIsEscaped)
							{
								if (bPppFraming)
								{
									byte ^= 0x20;
								}
								else // SLIP
								{
									if (byte == SLIP_ESC_ESC)
										byte = SLIP_ESC;
									else if (byte == SLIP_ESC_END)
										byte = SLIP_END;
								}
								bByteIsEscaped = FALSE;
							}
							*pRecvPacketCurrent++ = byte;
						}
						else // no space left -- frame too long
						{
							DEBUGMSG (!bFrameTooLong && ZONE_ERROR,
								(TEXT("AsyncMac: MacRxThread-ERROR: Packet too long (pCurr=%x byte=%02x, offset=%d), tossing\n"),
								pRecvPacketCurrent, byte, pbReceiveData - pRecvBuffer));
							bFrameTooLong = TRUE;
						}
					}
				} // while (cbReceiveData--)
				DEBUGMSG( ZONE_RECV, (TEXT( "Mac: RecvPacket size now %d bytes, pCurr=%x\n"), pRecvPacketCurrent - pRecvPacket, pRecvPacketCurrent));

				}while( dwBytesRead );
				
			}
		}
		else // WaitCommEvent did not return TRUE
		{
			Error = GetLastError();

			DEBUGMSG( ZONE_RECV | ZONE_ERROR, (TEXT( "Mac:WaitCommEvent failed %d\n"), Error ));

			// Serial functions will return an error if a PCMCIA card has
			// been removed. If the error is INVALID_HANDLE or GEN_FAILURE
			// the PCMCIA card was removed. In this case the MAC layer is
			// down.

			if( (Error == ERROR_INVALID_HANDLE
			||	 Error == ERROR_GEN_FAILURE)
			&& 	!(pOpenLine->dwFlags & AOL_FLAGS_ERROR_INDICATED) )
			{
				pOpenLine->dwFlags |= AOL_FLAGS_ERROR_INDICATED;
				//            MacNotifyDown( pOpenLine, ERROR_DEVICE_NOT_READY );
				SendLineDown (pOpenLine);
			}
			goto SerialError;
		}
	}

SerialError:
	CloseHandle( pOpenLine->hRxThrd);
	pOpenLine->hRxThrd = NULL;
	
	AsyncMacFreeMemory (pRecvBuffer, cbRecvBuffer);
	AsyncMacFreeMemory (pRecvPacketStart, RECEIVE_PACKET_SIZE(pOpenLine->pAdapter));
	ReleaseOpenLinePtr(pOpenLine);

	DEBUGMSG (ZONE_RECV, (TEXT("\nAsyncMac: -MacRxThread: Exiting\n")));
	return 0;
}
