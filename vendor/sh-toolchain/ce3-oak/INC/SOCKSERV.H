/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

Module Name:  

    sockserv.h

Abstract:  

    This file implements the Windows CE PCMCIA Socket Services structures and
    definitions.  This is provided as a sample to platform writers and is
    expected to be able to be used without modification on most (if not
    all) hardware platforms.

Functions:


Notes:


--*/

#ifndef __SOCKSERV_H__
#define __SOCKSERV_H__ 1

#pragma pack(1)

#ifdef __cplusplus
extern "C" {
#endif

//
// @doc DRIVERS
//

// PDD is responsible for defining constants for State & Level interrupts
extern DWORD gIntrPcmciaState;
extern DWORD gIntrPcmciaLevel;

//**********************************************************************
// Adapter information and power entries
//**********************************************************************

//
// PDCARD_ADAPTER_INFO.fAdapterCaps
//
#define ADAPT_CAPS_STATUS_SHARED 0x01
#define ADAPT_CAPS_POWER_SHARED  0x02
#define ADAPT_CAPS_WINDOW_SAME   0x04

//
// @struct PDCARD_ADAPTER_INFO | Structure used by <f PDCardInquireAdapter> to report
//                  the characteristics and capabilities of the socket controller.
//
typedef struct _PDCARD_ADAPTER_INFO {
    UINT32 uMemGranularity;     // @field Should be 1 for most implementations.
                                // (it is 4 for the perp)
    UINT8  fAdapterCaps;        // @field Bit encoded capabilities
    UINT8  uCacheLine;          // @field Number of 32 bit words in a cache line
    UINT16 uPowerEntries;       // @field Number of PDCARD_POWER_ENTRY structures
                                //          immediately following this structure.
} PDCARD_ADAPTER_INFO, *PPDCARD_ADAPTER_INFO;

//
// PDCARD_POWER_ENTRY.fSupply
//
#define PWR_SUPPLY_VPP2  0x20    // power entry describes Vpp2
#define PWR_SUPPLY_VPP1  0x40    // power entry describes Vpp1
#define PWR_SUPPLY_VCC   0x80    // power entry describes Vcc

//
// @struct PDCARD_POWER_ENTRY | Structure used to indicate the voltage levels that
//                              a socket controller can produce.
//
typedef struct _PDCARD_POWER_ENTRY {
    UINT8 uPowerLevel;          // @field Voltage in 0.1 volt increments
    UINT8 fSupply;              // @field Bit field indicating the voltage source (Vcc, Vpp1 or Vpp2)
} PDCARD_POWER_ENTRY, *PPDCARD_POWER_ENTRY;

//
// PDCARD_ADAPTER_STATE
//
#define ADP_STATE_POWERDOWN 1   // Socket controller is in power save mode.
                                // In power save mode, a card insertion or removal can be detected.
#define ADP_STATE_MAINTAIN  2   // Adapter is capable of saving and restoring its state during power save modes.
#define ADP_STATE_POWEROFF  4   // Socket controller is powered off.

// Indicate to socket services that PDCardSetAdapter or PDCardGetAdapter has
// been called from kernel mode.
#define ADP_STATE_KERNEL_MODE  0x80000000

typedef UINT32 PDCARD_ADAPTER_STATE, *PPDCARD_ADAPTER_STATE;


//**********************************************************************
// memory and I/O window information
//**********************************************************************

//
// PDCARD_WINDOW_INFO.fWindowCaps
//
#define WIN_CAP_COMMON    0x01 // common memory can be mapped into host space
#define WIN_CAP_ATTRIBUTE 0x02 // attribute memory can be mapped
#define WIN_CAP_IO        0x04 // I/O ports can be mapped into host I/O space
#define WIN_CAP_WAIT      0x80 // WAIT signal can extend access cycle

//
// PDCARD_WINDOW_INFO.fMemoryCaps
//
#define MEM_CAP_PRG_BASE   0x0001    // programmable base address
#define MEM_CAP_PRG_SIZE   0x0002    // programmable base window size
#define MEM_CAP_ENABLE_WIN 0x0004    // Enable/disable window
#define MEM_CAP_8BIT       0x0008    // 8 bit data bus supported
#define MEM_CAP_16BIT      0x0010    // 16 bit data bus supported
#define MEM_CAP_BASE_ALIGN 0x0020    // base must align with multiple of size
#define MEM_CAP_OFF_ALIGN  0x0080    // card offset must align on size boundary
#define MEM_CAP_PAGING_HW  0x0100    // Paging hardware available
#define MEM_CAP_PAGING_SH  0x0200    // Paging hardware shared
#define MEM_CAP_PAGING_EN  0x0400    // Enable/disable paging
#define MEM_CAP_WRITE_PROT 0x0800    // window write protect supported

//
// PDCARD_WINDOW_INFO.fIOCaps
//
#define IO_CAP_PRG_BASE   MEM_CAP_PRG_BASE
#define IO_CAP_PRG_SIZE   MEM_CAP_PRG_SIZE
#define IO_CAP_ENABLE_WIN MEM_CAP_ENABLE_WIN
#define IO_CAP_8BIT       MEM_CAP_8BIT
#define IO_CAP_16BIT      MEM_CAP_16BIT
#define IO_CAP_BASE_ALIGN MEM_CAP_BASE_ALIGN
#define IO_CAP_INPACK     0x0080    // INPACK# supported

//
// @struct PDCARD_WINDOW_INFO | Structure used by <f PDCardInquireWindow> to report 
//          a memory or I/O window's characteristics and capabilities.
//
typedef struct _PDCARD_WINDOW_INFO {
    UINT16 fSockets;        // @field bit 0 -> socket 0, bit 1 -> socket 1, ...
    UINT16 fWindowCaps;     // @field Bit encoded window capabilities
    UINT16 fMemoryCaps;     // @field Bit encoded memory window capabilities
    UINT16 fIOCaps;         // @field Bit encoded I/O window capabilities     
    UINT32 uMemFirstByte;   // @field Physical address of first addressable byte
    UINT32 uMemLastByte;    // @field Physical address of last addressable byte
    UINT32 uMemMinSize;     // @field Minimum size of window
    UINT32 uMemMaxSize;     // @field Maximum size of window
    UINT32 uMemGranularity; // @field Required window granularity
    UINT32 uMemBase;        // @field Required window base address alignment 
    UINT32 uMemOffset;      // @field Required card offset alignment
    UINT32 uIOFirstByte;    // @field Physical address of first addressable byte
    UINT32 uIOLastByte;     // @field Physical address of last addressable byte
    UINT32 uIOMinSize;      // @field Minimum size of window
    UINT32 uIOMaxSize;      // @field Maximum size of window
    UINT32 uIOGranularity;  // @field Required window granularity
    UINT8  uAddressLines;   // @field Number of I/O lines decoded
    UINT8  fSlowest;        // @field Bit encoded slowest access speed
    UINT8  fFastest;        // @field Bit encoded fastest access speed
} PDCARD_WINDOW_INFO, *PPDCARD_WINDOW_INFO;


//
// PDCARD_WINDOW_STATE.fState
//
#define WIN_STATE_MAPS_IO    0x01    // 1 = I/O, 0 = memory
#define WIN_STATE_ENABLED    0x02
#define WIN_STATE_16BIT      0x04
#define WIN_STATE_ATTRIBUTE  0x80    // memory window accesses attribute space

//
// @struct PDCARD_WINDOW_STATE | Structure used by <f PDCardGetWindow> and
//      <f PDCardSetWindow> to report or change a memory or I/O window's state.
//
typedef struct _PDCARD_WINDOW_STATE {
    UINT16 uSocket;     // @field Socket number to which this window is mappped
    UINT8  fState;      // @field Bit encoded window state
    UINT8  fSpeed;      // @field Bit encoded access speed
    UINT32 uSize;       // @field Size of window
    UINT32 uBase;       // @field Physical address of beginning of window
    UINT32 uOffset;     // @field Offset in PC card memory
} PDCARD_WINDOW_STATE, *PPDCARD_WINDOW_STATE;

//**********************************************************************
// Socket information
//**********************************************************************

//
// PDCARD_SOCKET_STATE.fSocketCaps
//
#define SOCK_CAP_MEM_ONLY   0x01
#define SOCK_CAP_IO         0x02
#define SOCK_CAP_LOW_VOLT   0x10
#define SOCK_CAP_33_VOLT    0x20
#define SOCK_CAP_KEEP_POWERED  0x80 // Keep PCMCIA bus powered while the system
                                    // is in the suspended state.

//
// PDCARD_SOCKET_STATE.fControlCaps
//
#define SOCK_IND_WRITE_PROTECT 0x01
#define SOCK_IND_CARD_LOCK     0x02
#define SOCK_IND_BATTERY       0x20
#define SOCK_IND_INUSE         0x40
#define SOCK_IND_XIP           0x80
#define SOCK_CTRL_EJECT        0x04
#define SOCK_CTRL_INSERT       0x08
#define SOCK_CTRL_LOCK         0x10
#define SOCK_ENABLE_SPEAKER    0x20  // Enable SPKR from PC card

//
// PDCARD_SOCKET_STATE.fIREQRouting
//
#define SOCK_IREQ_ENABLE    0x80  // Enable PCMCIA IREQ ints on the system
#define SOCK_IREQ_WAKEUP    0x40  // Allow PCMCIA ints to wakeup the system.

//
// PDCARD_SOCKET_STATE.fVcc
//
#define SOCK_VCC_LEVEL_MASK    0x0F    // Mask for power entry index
#define SOCK_VCC_CARD_OK       0x10

//
// @struct PDCARD_SOCKET_STATE | Structure used by <f PDCardGetSocket> and
//      <f PDCardSetSocket> to retrieve or change the specified socket's characteristics.
//
typedef struct _PDCARD_SOCKET_STATE {
    UINT8 fSocketCaps;          // @field Socket capabilities
    UINT8 fInterruptEvents;     // @field Status change interrupt mask.  The initial state of this field
                                //        indicates which events can cause a status change interrupt.
    UINT8 fNotifyEvents;        // @field Latched socket state
    UINT8 fControlCaps;         // @field Control capabilities
    UINT8 fInterfaceType;       // @field Memory-only or memory and I/O
    UINT8 fIREQRouting;         // @field Enable/disable IREQ
    UINT8 fVcc;                 // @field Vcc power entry index and status
    UINT8 uVpp1;                // @field Vpp1 power entry index
    UINT8 uVpp2;                // @field Vpp2 power entry index
} PDCARD_SOCKET_STATE, *PPDCARD_SOCKET_STATE;


#pragma pack()   // return packing to normal


//
// Socket Services Function Prototypes
//
STATUS PDCardInitServices(void);
STATUS PDCardInquireAdapter(PPDCARD_ADAPTER_INFO pAdapterInfo);
STATUS PDCardGetAdapter(UINT32 uSocket, PPDCARD_ADAPTER_STATE pAdapterState);
STATUS PDCardSetAdapter(UINT32 uSocket, PPDCARD_ADAPTER_STATE pAdapterState);
STATUS PDCardInquireWindow(UINT32 uWindow, PPDCARD_WINDOW_INFO pWindowInfo);
STATUS PDCardGetWindow(UINT32 uWindow, PPDCARD_WINDOW_STATE pWindowState);
STATUS PDCardSetWindow(UINT32 uWindow, PPDCARD_WINDOW_STATE pWindowState);
STATUS PDCardGetSocket(UINT32 uSocket, PPDCARD_SOCKET_STATE pSocketState);
STATUS PDCardSetSocket(UINT32 uSocket, PPDCARD_SOCKET_STATE pSocketState);
STATUS PDCardResetSocket(UINT32 uSocket);

//
// PCMCIA memory access functions
//
// Standard PCMCIA controllers provide several memory windows to access the
// the different areas of card memory.  Individual windows can access common
// or attribute memory and the controller asserts the correct signals when a
// particular window is used.
// On some platforms, software must toggle all the PCMCIA signals, hence access
// must be controlled via an API which can properly serialize conflicting
// accesses.
//
UINT8 PDCardReadAttrByte (PVOID pCardMem, UINT32 uOffset);
UINT8 PDCardReadCmnByte  (PVOID pCardMem, UINT32 uOffset);
UINT8 PDCardReadIOByte   (PVOID pCardMem, UINT32 uOffset);
VOID  PDCardWriteAttrByte(PVOID pCardMem, UINT32 uOffset, UINT8 uByte);
VOID  PDCardWriteCmnByte (PVOID pCardMem, UINT32 uOffset, UINT8 uByte);
VOID  PDCardWriteIOByte  (PVOID pCardMem, UINT32 uOffset, UINT8 uByte);

#ifdef __cplusplus
}
#endif

#endif

