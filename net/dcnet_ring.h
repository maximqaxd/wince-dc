//
// dcnet_ring.h - shared-memory frame ring between the BBA WDM driver (bba.dll,
// in device.exe) and the TCP/IP stack (dcnet.dll, in its host process). Same
// trick as the DCWin compositor: a named CreateFileMapping section that CE maps
// at the same virtual address in every process, so plain structs are valid
// cross-process. Two single-producer/single-consumer rings (lock-free via
// free-running head/tail indices, masked by DCN_FRAMES-1).
//
//   RX ring: bba produces (frames received from the wire), stack consumes.
//   TX ring: stack produces (frames to send),            bba consumes.
//
#ifndef DCNET_RING_H
#define DCNET_RING_H

#include <windows.h>

#define DCNET_SECTION  L"DCNETRING"
#define DCNET_MAGIC    0x44434E52   // 'DCNR'
#define DCN_FRAMES     32           // per ring (power of two)
#define DCN_FRMAX      1536         // max ethernet frame

typedef struct { DWORD len; BYTE data[DCN_FRMAX]; } DcnFrame;

typedef struct
{
    DWORD    magic;                 // DCNET_MAGIC once bba has initialised it
    DWORD    linkUp;                // 1 if the adapter is present/up
    BYTE     mac[6];                // adapter MAC (bba fills it)
    BYTE     pad[2];

    volatile DWORD rxHead;          // bba writes; stack reads
    volatile DWORD rxTail;          // stack writes
    DcnFrame rx[DCN_FRAMES];

    volatile DWORD txHead;          // stack writes; bba reads
    volatile DWORD txTail;          // bba writes
    DcnFrame tx[DCN_FRAMES];
} DcnRing;

#define DCN_MASK (DCN_FRAMES - 1)

#endif // DCNET_RING_H
