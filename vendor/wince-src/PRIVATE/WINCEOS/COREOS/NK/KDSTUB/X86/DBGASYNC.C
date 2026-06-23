/* Copyright (c) 1999-2000 Microsoft Corporation.  All rights reserved. */
#ifdef DEBUG
#ifdef DEBUG_DEBUGGER

#define COM2_BASE           0x02F8

#define comTxBuffer         0x00
#define comRxBuffer         0x00
#define comDivisorLow       0x00
#define comDivisorHigh      0x01
#define comIntEnable        0x01
#define comIntId            0x02
#define comFIFOControl      0x02
#define comLineControl      0x03
#define comModemControl     0x04
#define comLineStatus       0x05
#define comModemStatus      0x06

#define LS_TSR_EMPTY        0x40
#define LS_THR_EMPTY        0x20
#define LS_RX_BREAK         0x10
#define LS_RX_FRAMING_ERR   0x08
#define LS_RX_PARITY_ERR    0x04
#define LS_RX_OVERRUN       0x02
#define LS_RX_DATA_READY    0x01

#define LS_RX_ERRORS        ( LS_RX_FRAMING_ERR | LS_RX_PARITY_ERR | LS_RX_OVERRUN )

//   14400 = 8
//   16457 = 7 +/-
//   19200 = 6
//   23040 = 5
//   28800 = 4
//   38400 = 3
//   57600 = 2
//  115200 = 1

int __cdecl _inp(unsigned short);
int __cdecl _outp(unsigned short, int);

#pragma intrinsic(_inp, _outp)

void ComPortPutByte(unsigned char ucChar)
{
    static  int     bInitialized = 0;
    unsigned char   ucStatus;
    

    if (!bInitialized)
    {
        _outp(COM2_BASE+comLineControl, 0x80);   // Access Baud Divisor
        _outp(COM2_BASE+comDivisorLow, 0x02);    // 57600
        _outp(COM2_BASE+comDivisorHigh, 0x00);
        _outp(COM2_BASE+comFIFOControl, 0x01);   // Enable FIFO if present
        _outp(COM2_BASE+comLineControl, 0x03);   // 8 bit, no parity

        _outp(COM2_BASE+comIntEnable, 0x00);     // No interrupts, polled

        _outp(COM2_BASE+comModemControl, 0x03);  // Assert DTR, RTS
        
        bInitialized = 1;
    }
    
    do
    {
        ucStatus = _inp(COM2_BASE+comLineStatus);
    }
    while (!(ucStatus & LS_THR_EMPTY));

    _outp(COM2_BASE+comTxBuffer, ucChar);
}
#endif
#endif
