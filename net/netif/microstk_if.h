//
// microstk_if.h - the exact microstk <-> link-driver ABI, extracted from mppp.dll +
// its PDB in Ghidra. A link DLL named "mppp.dll" must export InterfaceInitialize;
// microstk LoadLibraryW("mppp.dll") + GetProcAddress("InterfaceInitialize") and
// drives the link through the ifnet vtable below.
//
// Contract (from mppp's InterfaceInitialize @ 1000aa64, Transmit @ 1000b300,
// Receive @ 1000ab5c, Ioctl @ 1000b3b4):
//   InterfaceInitialize(name, &pIfnet):
//     pIf = LocalAlloc(LPTR, sizeof(ifnet)+ctx); wcscpy(pIf->ifn_szName, name);
//     pIf->ifn_Ioctl = OurIoctl; pIf->ifn_IPOutput = OurTransmit;
//     pIf->ifn_dwFlags |= <link flags>; pIf->ifn_dwFlags &= ~1;   // start DOWN
//     *pIfnet = pIf; return 1;
//   microstk then back-fills ifn_IPInput + the 5 packet-pool callbacks +
//   ifn_IPInterfaceConfigure BEFORE any TX/RX.
//   TX  = ifn_IPOutput(pIf, NDIS_PACKET*, nextHopIP); MUST ifn_FreePacket(pkt); ret 0.
//   RX  = AllocatePacket(); AllocateBufferWithMemory(len,&mem); memcpy(mem,ip,len);
//         chain buf onto pkt (Head/Tail); ifn_IPInput(pIf, pkt).
//   UP  = ifn_dwFlags |= 1 (after IP config); Ioctl may just return 0.
//   IP  = ifn_IPInterfaceConfigure(pIf, ip, mask, gw, mtu) after DHCP.
//
#ifndef MICROSTK_IF_H
#define MICROSTK_IF_H

#include <windows.h>
#include <types.h>          // ulong/uint/ushort/uchar (SDK BSD-style types)

#pragma pack(push, 4)

// 12-byte NDIS buffer: a {next, data, len} cell.
typedef struct _NDIS_BUFFER {
    struct _NDIS_BUFFER *Next;           // 0
    void                *VirtualAddress; // 4
    ulong                BufferLength;    // 8
} NDIS_BUFFER;

// 52-byte NDIS packet; only Private.{TotalLength,Head,Tail} matter to us.
typedef struct _NDIS_PACKET_PRIVATE {
    uint         PhysicalCount;  // 0
    uint         TotalLength;    // 4
    NDIS_BUFFER *Head;           // 8
    NDIS_BUFFER *Tail;           // 12
    void        *Pool;           // 16
    uint         Count;          // 20
    ulong        Flags;          // 24
    uchar        ValidCounts;    // 28
    uchar        NdisPacketFlags;// 29
    ushort       OobOffset;      // 30
} NDIS_PACKET_PRIVATE;

typedef struct _NDIS_PACKET {
    NDIS_PACKET_PRIVATE Private; // 0..31
    uchar               u32[16]; // 32
    uchar               ProtocolReserved[4]; // 48
} NDIS_PACKET;

struct ifnet;
typedef int   (*PFN_Ioctl)(struct ifnet *, ulong cmd, void *arg);
typedef ulong (*PFN_IPOutput)(struct ifnet *, NDIS_PACKET *, ulong nextHopIP);
typedef void  (*PFN_IPInput)(struct ifnet *, NDIS_PACKET *);
typedef NDIS_BUFFER *(*PFN_AllocBufMem)(ulong len, uchar **ppMem);
typedef NDIS_PACKET *(*PFN_AllocPacket)(void);
typedef void  (*PFN_FreePacket)(NDIS_PACKET *);
typedef void  (*PFN_QueueTimeout)(void *fn, void *arg, ulong ms);
typedef void  (*PFN_FreeBuffer)(NDIS_BUFFER *);
typedef int   (*PFN_IPConfigure)(struct ifnet *, ulong ip, ulong mask, ulong a, ulong b);

// 112-byte ifnet (offsets verified against mppp.pdb).
typedef struct ifnet {
    ushort          ifn_szName[16];     // 0   - we set (wcscpy)
    ulong           ifn_ipAddr;         // 32
    ulong           ifn_ipPeerAddr;     // 36
    ulong           ifn_netMask;        // 40
    ulong           ifn_subnetMask;     // 44
    ulong           ifn_broadAddr;      // 48
    ulong           ifn_subnetAddr;     // 52
    ulong           ifn_netbroadAddr;   // 56
    ulong           ifn_netAddr;        // 60
    ulong           ifn_dwFlags;        // 64  - we set (bit0 = UP)
    ulong           ifn_dwMTU;          // 68  - we set (1500 for ethernet)
    ulong           ifn_dwIPMTU;        // 72
    PFN_Ioctl       ifn_Ioctl;          // 76  - we set
    PFN_IPOutput    ifn_IPOutput;       // 80  - we set (TX)
    PFN_IPInput     ifn_IPInput;        // 84  - microstk sets (RX deliver)
    PFN_AllocBufMem ifn_AllocateBufferWithMemory; // 88  - microstk sets
    PFN_AllocPacket ifn_AllocatePacket;           // 92  - microstk sets
    PFN_FreePacket  ifn_FreePacket;               // 96  - microstk sets
    PFN_QueueTimeout ifn_QueueTimeout;            // 100 - microstk sets
    PFN_FreeBuffer  ifn_FreeBuffer;               // 104 - microstk sets
    PFN_IPConfigure ifn_IPInterfaceConfigure;     // 108 - microstk sets
} ifnet;

#pragma pack(pop)

// ifn_dwFlags bits (microstk): bit0 = interface UP. mppp sets 0x88 for PPP
// (point-to-point); for ethernet we want broadcast routing, NOT point-to-point.
#define IFF_UP          0x0001
#define IFF_BROADCAST   0x0002          // ethernet (route by netmask, not peer)

#endif // MICROSTK_IF_H
