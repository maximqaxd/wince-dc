/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

Module Name:  

    ethdbg.h

Abstract:  

    
    Defs for debug/boot messages over ethernet.  This file is shared by
    the ethernet bootloader (eboot), kernel ethernet debug support
    routines (debug messages, kernel debugger, and text shell), and the
    corresponding desktop applications.

    Messages are transferred in UDP packets, and have a simple protocol which
    provides lost/duplicate packet detection.
    
--*/
#ifndef _ETHDBG_H
#define _ETHDBG_H

// Default UDP ports used for Ethernet download and EDBG messages.  May be overriden
// by device in BOOTME message.
#define  EDBG_DOWNLOAD_PORT 980   // For downloading images to bootloader via TFTP
#define  EDBG_SVC_PORT      981   // Other types of transfers

// Image downloads are achieved by TFTP to the download port using the
// following filename.
#define EDBG_DOWNLOAD_FILENAME "boot.bin"

// Number of milliseconds to wait before retransmitting packets. Subsequent retransmissions
// will occur at 2,4,8, etc up to INTERVAL_MAX.
#define EDBG_RETRANSMIT_INTERVAL_MS  1000
#define EDBG_RETRANSMIT_INTERVAL_MAX 8000

// Number of milliseconds before timing out waiting for an ack
#define EDBG_TIMEOUT_INTERVAL_MS     10000

// Number of seconds to wait for a response to a gratuitous ARP
#define ARP_RESPONSE_WAIT 2

// Some macros to manipulate sequence numbers
#define EDBG_MAX_SEQ  255
#define SEQ_BETWEEN(start,s,end)  ((((start)<=(s)) && ((s)<(end))) || (((end)<(start)) && ((start)<=(s))) || (((s)<(end)) && ((end)<(start))))
#define SEQ_DELTA(start,end) (((start) <= (end)) ? ((end) - (start)) : ((EDBG_MAX_SEQ+1) - (start) + (end)))
#define SEQ_ADD(s,amount)  (((s) + (amount)) % (EDBG_MAX_SEQ+1))
#define SEQ_SUB1(s)        (((s)+EDBG_MAX_SEQ)%(EDBG_MAX_SEQ+1))
#define SEQ_INC(s)         ((s) = SEQ_ADD((s),1))

#include <pshpack1.h>

// Format of UDP packet data -- all EDBG messages have this header, except for
// downloads, which use TFTP.
typedef struct _ETH_DBG_HDR
{
    DWORD Id;        // Protocol identifier ("EDBG" on the wire)
    UCHAR Service;   // Service identifier
    UCHAR Flags;     // Flags (see defs below)
    UCHAR SeqNum;    // For detection of dropped packets
    UCHAR Cmd;       // For administrative messages
    UCHAR Data[1];   // Cmd specific data starts here (format is determined by 
                     // Cmd, len is determined by UDP packet size)
} ETH_DBG_HDR, *PETH_DBG_HDR;

// Byte string for Id field (note - must not conflict with valid TFTP
// opcodes (0-5), as we share the download port with TFTP)
#define EDBG_ID  0x47424445 // "GBDE"

// Defs for reserved values of the Service field
#define EDBG_SVC_DBGMSG  0   // Debug messages
#define EDBG_SVC_PPSH    1   // Text shell and PPFS file system
#define EDBG_SVC_KDBG    2   // Kernel debugger
#define EDBG_SVC_ADMIN 0xFF  // Administrative messages 
// Service Ids from 3-FE are used for user apps
#define NUM_DFLT_EDBG_SERVICES  3  

// For calculating offset of a structure element
#ifndef OFFSETOF
#define OFFSETOF(s,m)           ((DWORD)&(((s*)0)->m))
#endif

#define EDBG_DATA_OFFSET (OFFSETOF(ETH_DBG_HDR,Data))

// Size of send and receive windows (except for stop and wait mode)
#define EDBG_WINDOW_SIZE 8

// The window size can be negotiated up to this amount if a client provides
// enough memory.
#define EDBG_MAX_WINDOW_SIZE  16

// Max size for an EDBG frame.  Based on ethernet MTU - protocol overhead.
// Limited to one MTU because we don't do IP fragmentation on device.
#define EDBG_MAX_DATA_SIZE 1446

// Defs for Flags field.
#define EDBG_FL_FROM_DEV 0x01   // Set if message is from the device
#define EDBG_FL_NACK     0x02   // Set if frame is a nack
#define EDBG_FL_ACK      0x04   // Set if frame is an ack
#define EDBG_FL_SYNC     0x08   // Can be used to reset sequence # to 0
#define EDBG_FL_ADMIN_RESP 0x10   // For admin messages, indicate whether this is a response

// Definitions for Cmd field (used for administrative messages)
// Msgs from device
#define EDBG_CMD_BOOTME      0   // Initial bootup message from device

// Msgs from PC
#define EDBG_CMD_SETDEBUG    1 // Used to set debug zones on device (TBD)
#define EDBG_CMD_JUMPIMG     2 // Command to tell bootloader to jump to existing
                               // flash or RAM image. Data is same as CMD_OS_CONFIG.
#define EDBG_CMD_OS_CONFIG   3 // Configure OS for debug ethernet services
#define EDBG_CMD_QUERYINFO   4 // "Ping" device, and return information (same fmt as bootme)
#define EDBG_CMD_RESET       5 // Command to have platform perform SW reset (e.g. so it
                               // can be reprogrammed).  Support for this command is
                               // processor dependant, and may not be implemented
                               // on all platforms (requires HW mods for Odo).

// Msgs from device or PC 
#define EDBG_CMD_SVC_CONFIG  6
#define EDBG_CMD_SVC_DATA    7

#define EDBG_CMD_DEBUGBREAK  8 // Break into debugger

// Structures for Data portion of EDBG packets

#define EDBG_MAX_DEV_NAMELEN 16

// BOOTME message - Devices broadcast this message when booted to request configuration
#define	EDBG_CURRENT_BOOTME_VERSION	2

typedef struct _EDBG_BOOTME_DATA
{
    UCHAR  VersionMajor;    // Bootloader version
    UCHAR  VersionMinor;    // Bootloader version
    USHORT MACAddr[3];      // Ether address of device (net byte order)
    DWORD  IPAddr;          // IP address of device (net byte order)
    UCHAR  PlatformId[EDBG_MAX_DEV_NAMELEN+1];  // Platform Id string (NULL terminated)
    UCHAR  DeviceName[EDBG_MAX_DEV_NAMELEN+1];  // Device name string (NULL terminated). Should include
                            // platform and number based on Ether address (e.g. Odo42, CEPCLS2346, etc)
    UCHAR  CPUId;           // CPU identifier (upper nibble = type)
// The following fields were added in CE 3.0 Platform Builder release
    UCHAR  uBootmeVer;      // BOOTME Version.  Must be in the range 2 -> EDBG_CURRENT_BOOTME_VERSION, or
                            // remaining fields will be ignored by Eshell and defaults will be used.
#define	EDBG_BOOTFLAG_FORCE_DOWNLOAD	0x0001	// always download image
    DWORD  dwBootFlags;     // Boot Flags
    USHORT wDownloadPort;   // Download Port (net byte order) (0 -> EDBG_DOWNLOAD_PORT) 
    USHORT wSvcPort;        // Service Port (net byte order) (0 -> EDBG_SVC_PORT)

} EDBG_BOOTME_DATA, *PEDBG_BOOTME_DATA;

// Defs for CPUId
#define EDBG_CPU_TYPE_SHX  0x10
#define EDBG_CPU_TYPE_MIPS 0x20
#define EDBG_CPU_TYPE_X86  0x30
#define EDBG_CPU_TYPE_ARM  0x40
#define EDBG_CPU_TYPE_PPC  0x50
    
#define EDBG_CPU_SH3    (EDBG_CPU_TYPE_SHX  | 0)
#define EDBG_CPU_SH4    (EDBG_CPU_TYPE_SHX  | 1)
#define EDBG_CPU_R3000  (EDBG_CPU_TYPE_MIPS | 0)
#define EDBG_CPU_R4101  (EDBG_CPU_TYPE_MIPS | 1)
#define EDBG_CPU_R4102  (EDBG_CPU_TYPE_MIPS | 2)
#define EDBG_CPU_R4111  (EDBG_CPU_TYPE_MIPS | 3)
#define EDBG_CPU_R4200  (EDBG_CPU_TYPE_MIPS | 4)
#define EDBG_CPU_R4300  (EDBG_CPU_TYPE_MIPS | 5)
#define EDBG_CPU_R5230  (EDBG_CPU_TYPE_MIPS | 6)
#define EDBG_CPU_i486   (EDBG_CPU_TYPE_X86  | 0)
#define EDBG_CPU_SA1100 (EDBG_CPU_TYPE_ARM | 0)
#define EDBG_CPU_ARM720 (EDBG_CPU_TYPE_ARM | 1)
#define EDBG_CPU_PPC821 (EDBG_CPU_TYPE_PPC | 0)
#define EDBG_CPU_PPC403 (EDBG_CPU_TYPE_PPC | 1)

// Configure message - Sent to device to configure OS components for ethernet
// support.  Sent as data field of JUMPIMG command, as well as OS_CONFIG.
typedef struct _EDBG_OS_CONFIG_DATA
{
    UCHAR Flags;           // Flags that will be used to determine what features are
                           // enabled over ethernet (saved in driver globals by bootloader)
#define EDBG_FL_DBGMSG    0x01  // Debug messages
#define EDBG_FL_PPSH      0x02  // Text shell
#define EDBG_FL_KDBG      0x04  // Kernel debugger
#define EDBG_FL_CLEANBOOT 0x08  // Force a clean boot
    
    UCHAR Pad0;            // Pad to USHORT

    // The following specify addressing info, only valid if the corresponding
    // flag is set in the Flags field.
    DWORD  DbgMsgIPAddr;
    USHORT DbgMsgPort;
    DWORD  PpshIPAddr;
    USHORT PpshPort;
    DWORD  KdbgIPAddr;
    USHORT KdbgPort;
    
} EDBG_OS_CONFIG_DATA, *PEDBG_OS_CONFIG_DATA;

#define MAX_SVC_NAMELEN  19
#define EDBG_SVCNAME_DBGMSG  "DBGMSG"
#define EDBG_SVCNAME_PPSH    "PPSH"
#define EDBG_SVCNAME_KDBG    "KDBG"

// Format of service config message from device
typedef struct _EDBG_DEV_SVC_CONFIG_DATA
{
    UCHAR   ProtocolVersion;  // Version of EDBG protocol
    char    ServiceName[MAX_SVC_NAMELEN+1];  // NULL terminated service name
    UCHAR   ServiceId;       // Identifier that goes in the Service field 

    // Some protocol configuration specified by the device
    UCHAR   WindowSize;  // Protocol window size
    UCHAR   Flags;
#define EDBG_CFGFL_STOP_AND_WAIT  0x01 // Stop after each transmission and wait for ack
#define EDBG_CFGFL_NOACKS         0x02 // Don't ack data messages
#define EDBG_CFGFL_MULTIINST      0x04 // Allow multiple instances of this service on device

// Only flags within this mask are sent in the SVC_CONFIG message. (other flags are
// used by RegisterClient for local state info)
#define CFGFL_MASK 0x0F

} EDBG_DEV_SVC_CONFIG_DATA;


// Format of service config message from desktop
typedef struct _EDBG_DESKTOP_SVC_CONFIG_DATA
{
    UCHAR   ProtocolVersion; // Version of EDBG protocol
    char    ServiceName[MAX_SVC_NAMELEN+1];  // NULL terminated service name
    DWORD   IPAddr;          // IP address (net byte order)
    USHORT  Port;            // UDP port the app is using (net byte order)    
} EDBG_DESKTOP_SVC_CONFIG_DATA;


#define CURRENT_EDBG_VERSION 1

typedef struct _EDBG_RESET_DATA
{
    UCHAR   Flags;
} EDBG_RESET_DATA;

#define EDBG_RESET_CLEAN  0x1   // Perform clean boot

// For EDBG_DBGMSGs, Data field contains debug string text. Others TBD.

// Query info response contains the same info as BOOTME
typedef EDBG_BOOTME_DATA EDBG_QUERYINFO_DATA;

// Format of debug message data
typedef struct _EDBG_DBGMSG_INFO
{
    DWORD dwLen;        // Length of this header (not including dbgmsg text)
    DWORD dwThreadId;   // Thread Id
    DWORD dwProcId;     // Process Id
    DWORD dwTimeStamp;  // Time stamp (ms)

} EDBG_DBGMSG_INFO;

#include <poppack.h>

//
// Client defs
//

#ifndef ZONE_INIT
// Debug zone definitions
#define  ZONE_WARNING    0x0001   // Warnings
#define  ZONE_INIT       0x0002   // Init messages, including register
#define  ZONE_FRAMEDUMP  0x0004   // Dump of each Tx/Rx frame, including time stamp
#define  ZONE_TIMER      0x0008   // Timer insertions/removals
#define  ZONE_SEND       0x0010   // Tx state info
#define  ZONE_RECV       0x0020   // Rx state info
#define  ZONE_RETRANSMIT 0x0040   // Retransmissions

// These zones only apply on device
#ifdef UNDER_CE
#define  ZONE_INTR      0x0100  // Interrupts
#define  ZONE_ADAPTER   0x0200  // Must be combined with INTR,SEND,or RECV
#define  ZONE_LEDS      0x0400  // For writing to alpha LEDs (useful for measuring timings)
#define  ZONE_DHCP      0x0800
#endif
#endif

// Function prototypes - on the device, these are implemented as KerneIoControls (see halether.h)
#ifdef UNDER_CE
BOOL EdbgRegisterClient(UCHAR *pId, char *ServiceName, UCHAR Flags, UCHAR WindowSize, UCHAR *pBufferPool);
#define EDBG_DFLT_BUFFER_POOL_SIZE  (EDBG_WINDOW_SIZE*2*1500)
void EdbgSetDebug(DWORD DebugZoneMask);
#else
BOOL _cdecl EdbgRegisterClient(UCHAR *pId,char *ServiceName,UCHAR Flags,char *EshellServer,char *DeviceName);
BOOL _cdecl EdbgGetDeviceInfo(char *DeviceName, UCHAR Id, EDBG_BOOTME_DATA *pBootmeData);
BOOL _cdecl EdbgResetDevice(char *DeviceName, UCHAR Id, BOOL CleanBoot);
typedef void (*EDBG_BOOT_CALLBACK_FUNC)(PVOID pParam, PEDBG_BOOTME_DATA pBootmeData);
BOOL EdbgRegisterBootCallback(UCHAR Id, EDBG_BOOT_CALLBACK_FUNC pCallBack, PVOID pParam);
BOOL EdbgDeregisterBootCallback(UCHAR Id, EDBG_BOOT_CALLBACK_FUNC pCallBack, PVOID pParam);
// If this value is passed in, callback is registered for all devices
#define ID_ALL_DEVICES 0xFF

typedef void (* PFN_OutputDebugString) (char *fmt, ...);
void _cdecl EdbgSetDebug(DWORD DebugZoneMask, PFN_OutputDebugString pDebugStringFunc);
void _cdecl EdbgSetTestParams(BOOL fLazyReads,DWORD dwTossPercent);
#endif

BOOL _cdecl EdbgDeregisterClient(UCHAR Id);
BOOL _cdecl EdbgSend(UCHAR Id, UCHAR *pUserData, DWORD dwUserDataLen);
BOOL _cdecl EdbgRecv(UCHAR Id, UCHAR *pRecvBuf, DWORD *pdwLen, DWORD Timeout);


#endif // _ETHDBG_H
