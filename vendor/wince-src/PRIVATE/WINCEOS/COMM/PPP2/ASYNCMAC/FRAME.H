/*****************************************************************************
* 
*  Copyright (c) 1999-2000 Microsoft Corporation.  All rights reserved.
*
*   @doc
*   @module frame.h | AsyncMac Framing constants
*
*   @comm
*/



// PPP Constants
#define PPP_FLAG_BYTE       0x7e
#define PPP_ESC_BYTE        0x7d

// SLIP special character codes
#define SLIP_END             0xC0    /* indicates end of packet */
#define SLIP_ESC             0xDB    /* indicates byte stuffing */
#define SLIP_ESC_END         0xDC    /* ESC ESC_END means END data byte */
#define SLIP_ESC_ESC         0xDD    /* ESC ESC_ESC means ESC data byte */



// ----------------------------------------------------------------
//
// Extern declarations
//
// ----------------------------------------------------------------

// pppframe.c
VOID AssemblePPPFrame(PASYNCMAC_OPEN_LINE pOpenLine, PNDIS_WAN_PACKET WanPacket);

// slipframe.c
VOID AssembleSLIPFrame(PASYNCMAC_OPEN_LINE pOpenLine, PNDIS_WAN_PACKET WanPacket);

// crcppp.c
USHORT CalcCRCPPP(PUCHAR cp, UINT len);



