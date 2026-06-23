/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

Module Name:  

	halether.h

Abstract:  

    
    Defines for the interface to the kernel ethernet services.  Specifies the
    required services by the HAL to allow the debug ethernet (EDBG) services
    (debug messages, kernel debugger, PPSH shell) to operate.
	
    @doc    EDBG                                       
    
    @topic  Interfacing a platform to the Windows CE Ethernet debug (EDBG) services |
    
            In order to use the Ethernet transport for Windows CE debug services
            (dbgmsg, CESH, windbg), the OEM must provide a set of routines to 
            control the debug Ethernet hardware specific to the platform (OEMEthxxx
            functions described below), and link ethdbg.lib into the kernel.  Sample
            drivers for NE2000 (ne2kdbg.lib) and SMC9000 (smc9000.lib) compatible 
            Ethernet adapters are provided, or an OEM may write a driver for the 
            hardware on their particular platform.
            
            Note that a dedicated debug Ethernet controller must be used, it 
            cannot be shared with NDIS drivers in the OS.
            
            The Ethernet routines may be interrupt driven or may run in polling
            mode.  It is highly recommended to use interrupts for debug Ethernet,
            since polling can result in much greater system delays in the event
            that packets are dropped on the network.  The debug Ethernet ISR is
            much like other driver ISRs - it should be very small and typically
            will just mask the interrupt and return the corresponding SYSINTR
            value to the OS, which will handle the interrupt event internally.
            This means the Interrupt Service Thread (IST) does not need to be
            provided by the OEM - a call will be made to the OEMEthISR() function
            to perform the interrupt handling.
            
            Even though the model is interrupt driven, there are times when interrupts
            cannot be processed by the system (e.g. very early in boot, or within
            a device driver's power ON/OFF handler).  So, the OEM routines must be
            able to work in polled mode as well. In these situations, the EDBG subsystem
            will repeatedly call OEMEthGetFrame() when waiting for receive data, and
            use OEMEthGetSecs() to implement timeouts.
            
            All of the debug Ethernet routines (OEMEthxxx) are called in a restricted
            context, and may not make any system calls.  Delays must be accomplished
            by hardware timers, blocking or sleeping is not allowed.
            
--*/
#ifndef _HALETHER_H_
#define _HALETHER_H_

#include <pkfuncs.h>
#include <ethdbg.h>

#ifdef __cplusplus
extern "C" {
#endif

// These macros are machine dependent.  They convert the little endian representation used on
//  this CPU to the big endian format that is transmitted on the network.
#define htons( value ) ((UINT16)((((UINT16)value) << 8) | (((UINT16)((UINT16)value)) >> 8)))
#define ntohs( value ) htons( value )
#define htonl( value ) ((((ULONG)value) << 24) | ((0x0000FF00UL & ((ULONG)value)) << 8) | ((0x00FF0000UL & ((ULONG)value)) >> 8) | (((ULONG)value) >> 24))
#define ntohl( value ) htonl( value )

// Ethernet Maximum Transfer Unit, the max # of bytes in a single Ethernet frame
#define ETHER_MTU 1500

//
//  These pointers are set in to point to the corresponding functions
//  in ethdbg.lib when EdbgInit() is called.
//
typedef BOOL (*PFN_EdbgRegisterClient)(UCHAR *pId, char *ServiceName, UCHAR Flags, UCHAR WindowSize, UCHAR *pBufferPool);
typedef BOOL (*PFN_EdbgDeregisterClient)(UCHAR Id);
typedef BOOL (*PFN_EdbgSend)(UCHAR Id, UCHAR *pUserData, DWORD dwUserDataLen);
typedef BOOL (*PFN_EdbgRecv)(UCHAR Id, UCHAR *pRecvBuf, DWORD *pdwLen, DWORD Timeout);
typedef BOOL (*PFN_EdbgInitializeInterrupt)();
typedef BOOL (*PFN_EdbgRegisterDfltClient)(UCHAR Service, UCHAR Flags, UCHAR **ppTxBuffer, UCHAR **ppRxBuffer);

extern PFN_EdbgRegisterClient pEdbgRegisterClient;
extern PFN_EdbgDeregisterClient pEdbgDeregisterClient;
extern PFN_EdbgSend pEdbgSend;
extern PFN_EdbgRecv pEdbgRecv;
extern PFN_EdbgInitializeInterrupt pEdbgInitializeInterrupt;
extern PFN_EdbgRegisterDfltClient pEdbgRegisterDfltClient;

//
// The ethernet debug (EDBG) functions may be called via KernelIoControl if these IOCTLS are
// supported in OEMIoControl. Provide some functions to ease writing apps on top of EDBG.
//
_inline BOOL
CallEdbgRegisterClient(UCHAR *pId, char *ServiceName, UCHAR Flags, UCHAR WindowSize, UCHAR *pBufferPool)
{
    return KernelIoControl(IOCTL_EDBG_REGISTER_CLIENT, pId, Flags, ServiceName, WindowSize, (LPDWORD)pBufferPool);
}
_inline BOOL
CallEdbgDeregisterClient(UCHAR Id)
{
    return KernelIoControl(IOCTL_EDBG_DEREGISTER_CLIENT, NULL,Id,NULL,0,NULL);
}
_inline BOOL
CallEdbgRegisterDfltClient(UCHAR Service, UCHAR Flags, UCHAR **ppTxBuffer, UCHAR **ppRxBuffer)
{
    return KernelIoControl(IOCTL_EDBG_REGISTER_DFLT_CLIENT, (UCHAR *)ppTxBuffer, Service, (UCHAR *)ppRxBuffer, Flags, NULL);
}
_inline BOOL
CallEdbgSend(UCHAR Id, UCHAR *pUserData, DWORD dwUserDataLen)
{
    return KernelIoControl(IOCTL_EDBG_SEND, pUserData, Id, NULL, dwUserDataLen, NULL);
}
_inline BOOL
CallEdbgRecv(UCHAR Id, UCHAR *pRecvBuf, DWORD *pdwLen, DWORD Timeout)
{
    return KernelIoControl(IOCTL_EDBG_RECV, pRecvBuf, Id, pdwLen, Timeout, NULL);
}
_inline BOOL
CallEdbgSetDebug(DWORD ZoneMask)
{
    return KernelIoControl(IOCTL_EDBG_SET_DEBUG, NULL, ZoneMask, NULL, 0, NULL);
}

/*
 * @struct EDBG_ADDR | Addressing info for the debug Ethernet subsystem
 * 
 * For speed, all values are stored in net byte order (big endian).  Use the
 * htonl/ntohl/htons/ntohs macros to convert to/from local byte order.
 */
typedef struct _EDBG_ADDR {
	DWORD  dwIP;         // @field IP address (net byte order)
	USHORT wMAC[3];      // @field Ethernet address (net byte order)
	USHORT wPort;        // @field UDP port # (net byte order) - only used if appropriate
    
} EDBG_ADDR;

/*
 *  @struct EDBG_ADAPTER | Adapter info struct
 *
 *  Contains information pertaining to the debug Ethernet adapter.
 */
typedef struct _EDBG_ADAPTER
{
    EDBG_ADDR Addr;       // @field Ethernet and IP Address of device
    DWORD     SysIntrVal; // @field Logical interrupt for Ethernet controller. If set to EDBG_SYSINTR_NOINTR,
                          //        no interrupt will be registered, and the adapter will only be accessed
                          //        in polled mode.  This may be used for systems with limited IRQ lines, but
                          //        is not recommended due to delays in the system which may occur when packets
                          //        are dropped on the network.
    DWORD DHCPLeaseTime;  // @field Duration of DHCP IP lease in seconds
    DWORD EdbgFlags;      // @field Information about ethernet system
} EDBG_ADAPTER;

// Values for EDBG_ADAPTER.EdbgFlags
#define EDBG_FLAGS_STATIC_IP    0x00000001

#define DEFAULT_DHCP_LEASE  259200  // Number of seconds in 3 days.

#define EDBG_SYSINTR_NOINTR  0xFFFFFFFF

// The following defs can be used for platforms which support multiple adapter types.
// Identifiers are provided here for drivers which are built in common\oak\drivers\ethdbg.
// Any platform specific adapter types can be identified based off of EDBG_ADAPTER_OEM.
#define EDBG_ADAPTER_SMC9000  0
#define EDBG_ADAPTER_NE2000   1

#define EDBG_ADAPTER_OEM      16

// Prototypes for HAL functions to control ethernet adapter.  The EDBG library calls the OEMEthxxx
// functions within a KCall, so no system calls may be used in any of these functions.


/* @func BOOL | OEMEthInit | Initialize debug Ethernet hardware 
 * 
 * @comm  Called by the debug Ethernet subsystem as part of EdbgInit() processing. Should
 *        initialize the debug Ethernet adapter and fill in the address and interrupt information.
 *        If this function returns FALSE, no kernel services will be enabled over Ethernet.
 *        
 *        
 * @rdesc Return TRUE if successful, or FALSE if no adapter is present or an error occurs.
 */
BOOL OEMEthInit(
                EDBG_ADAPTER *pAdapter);  // @parm [OUT] -- Adapter info struct, must be filled out
                                          //                with platform specific info.

/* @func void | OEMEthEnableInts | Enable interrupts on the debug Ethernet adapter
 *
 * @comm The debug Ethernet adapter should be initially configured with interrupts disabled.
 *       When the system is initially coming up, the EDBG subsystem operates in polling mode.
 *       At a certain point in the boot process (if a valid SYSINTR value has been specified
 *       in OEMEthInit), the EDBG subsystem will switch over to interrupt mode, and call this
 *       function to enable the adapter to start processing interrupts.
 */
void OEMEthEnableInts();

/* @func void | OEMEthDisableInts | Disable interrupts on the debug Ethernet adapter
 *
 * @comm This routine should mask interrupts from the debug Ethernet hardware.
 */
void OEMEthDisableInts();

/* @func DWORD | OEMEthISR | Debug Ethernet interrupt handler
 * 
 * @comm This function is called by the EDBG subsystem to process an interrupt event.  It
 *       should perform any hardware specific processing required, and return a mask of the
 *       interrupt events that are present.  Currently, the only event that the EDBG subsystem
 *       will process is a receive interrupt event (INTR_TYPE_RX).  If this value is returned,
 *       the OEMEthGetFrame() function will be called to read the received data. 
 *
 * @rdesc Return a mask of interrupt events, or 0 for no pending ints.
 */
DWORD OEMEthISR();
#define INTR_TYPE_RX   0x0001   // EDBG lib only cares about RX intrs, others handled internally

/* @func BOOL | OEMEthGetFrame | Receive data from the debug Ethernet adapter
 *
 * @comm  This function is called by the EDBG subsystem to check to see if a frame has
 *        been received, and if so, copy it to the specified buffer.   NOTE: To optimize
 *        performance, it is recommended that broadcast frames be discarded as early as 
 *        possible in this routine.  The only broadcast frames required by the EDBG 
 *        subsystem are ARP messages, so processing can be reduced by filtering out all
 *        other broadcast frames.
 * @rdesc Return TRUE if receive data is present, FALSE if not.
 */
BOOL OEMEthGetFrame(
                    BYTE *pData,        // @parm [OUT] - Buffer to receive frame data
                    UINT16 *pwLength);  // @parm [IN]  - Set to length of buffer
                                        //       [OUT] - Set to number of bytes received

/* @func  BOOL | OEMEthSendFrame | Send data over debug Ethernet adapter
 * @rdesc Return TRUE if successful, FALSE if an error occurred and the data could not be sent.
 * @comm  This function is called by the EDBG subsystem to send an Ethernet frame.  The frame
 *        is completely formatted (including Ethernet header) before this function is called.
*/
BOOL OEMEthSendFrame(
                     BYTE *pData,     // @parm [IN] - Buffer containing frame data
                     DWORD dwLength); // @parm [IN] - Number of bytes in frame

/* @func  BOOL | OEMEthQueryClientInfo | Get platform specific information for EDBG services
 *
 * @comm  This function is called by the EDBG subsystem when the default Ethernet services
 *        (debug messages, CESH shell, kernel debugger) are configured.  The platform needs
 *        to return address information and a buffer pool for the specified client.  The buffers 
 *        are used for formatting and receiving EDBG packets (single buffer pool specified, 
 *        divided in two for Rx and Tx buffers).  By specifying a smaller window size, less 
 *        memory can be used, but the protocol will be less efficient.  The standard window
 *        size is EDBG_WINDOWS_SIZE.  The largest allowed window size is EDBG_MAX_WINDOW_SIZE.  
 *        The amount of memory required per client is (2*WindowSize*1500) bytes.
 * @rdesc Return TRUE if client for service can be configured for Ethernet, FALSE if not. 
 */
BOOL OEMEthQueryClientInfo(
                           UCHAR Id,             // @parm [IN] - Service ID (one of EDBG_SVC defs from ethdbg.h).              
                           EDBG_ADDR *pPeerAddr, // @parm [OUT]- Filled in with the peer Ether/IP address and UDP port number. 
                           PUCHAR pWindowSize,   // @parm [OUT]- Filled in with the client window size.                        
                           PUCHAR *ppBufferPool);// @parm [OUT]- Filled in with the packet buffer pool address.                

/* @func  DWORD | OEMEthGetSecs | Get second count from RTC
 * 
 * @comm  This function is used for handling timeouts while in polling mode.  Must return the number of 
 *        seconds from some fixed time (doesn't matter what, as long as it increments).  Note the  
 *        restriction on calling system calls - for example you can't call FileTimeToSystemTime from    
 *        within this function.                                                                         
 * @rdesc Return second count.
 */
DWORD OEMEthGetSecs();

/*
 * @topic   Debug Ethernet routines provided in ethdbg.lib |
 * 
 *          If an OEM links ethdbg.lib in to the kernel, the following functions
 *          are exported.  Some of these (i.e. <f EdbgInit>) are required to set up
 *          the EDBG subsystem, while others may be exported via KernelIOControl
 *          to applications which may want to use the debug Ethernet services.  Refer
 *          to the sample platform code (oemioctl.c) for suggested usage of the IOCTL
 *          codes to access these functions.
 *
 *          In order to set up the default debug Ethernet services (DBGMSG, CESH,
 *          KDBG), use <f SetKernelCommDev>(KERNEL_SVC_xxx, KERNEL_COMM_ETHER).  This
 *          will internally call the <f EdbgRegisterDfltClient> function to connect through 
 *          the debug Ethernet interface to the peer application running on the desktop.  
 *          Once this connection is established, further communication for the specified 
 *          service will be done over the debug Ethernet interface.
 */

/* @func BOOL | EdbgInit | Initialize debug Ethernet subsystem
 *
 * @comm This function should be called by the HAL to initialize the EDBG subsystem.
 *       As part of the <f EdbgInit> processing, the kernel will call the platform
 *       <f OEMEthInit> routine to perform hardware initialization.  This function
 *       must be called before any debug services are redirected to Ethernet via
 *       <f SetKernelCommDev>. It may be called any time after the <f KernelStart>
 *       routine has been called, typically as part of <f OEMInitDebugSerial> or
 *       <f OEMInit> processing. <f EdbgInit> should also be called in the resume from
 *       suspend processing, in <f OEMPowerOff>.
 *      
 * @rdesc Return TRUE if init successful, FALSE if not. 
 */
BOOL EdbgInit();

/* @func BOOL | EdbgSendUDP | Format and send a UDP packet using the debug Ethernet interface.
 *
 * @comm This function can be used to send a UDP datagram using the debug Ethernet adapter.  All
 *       protocol formatting is done inside this function, so the frame buffer must be large enough
 *       to hold all protocol headers (42 bytes) plus the user data.  The debug Ethernet
 *       routines will not do IP fragmentation, so the total of user data + protocol headers must
 *       be less than the Ethernet MTU (1500 bytes). Also, the debug Ethernet subsystem will not ARP
 *       for unknown addresses, so the pDestAddr struct must be filled out completely, including
 *       Ethernet address.  Typically, an OEM will provide KernelIOControls to get interesting
 *       Ethernet addresses (e.g. the address of the host running Eshell).
 *
 *       Note that for Windows CE 2.1, there is no way to receive IP or UDP packets on the debug 
 *       Ethernet interface.
 *
 * @rdesc Return TRUE if packet was successfully sent, FALSE if an error occurred.
 */
BOOL EdbgSendUDP(
                 BYTE *pFrameBuffer,    // @parm [IN] - Formatting buffer (must be at least 42 + cwLength bytes)
                 EDBG_ADDR *pDestAddr,  // @parm [IN] - Address of destination
                 USHORT SrcPort,        // @parm [IN] - Source UDP port number
                 BYTE *pData,           // @parm [IN] - User data buffer
                 UINT16 cwLength );     // @parm [IN] - # of bytes of data in user buffer

/* @func void | EdbgOutputDebugString |  Print messages to the debug serial port
 *
 * @comm  This function is provided to allow debugging of the debug Ethernet services.  It will
 *        format and print a message to the debug serial port, even if debug messages (i.e. NKDbgPrintfW) 
 *        are routed over Ethernet.  If the debugger (windbg) is running over the serial port, the message will
 *        be formatted such that it shows up in the debugger output window.  The formatting capabilities
 *        of this function are limited to the following format characters:
 *
 *               Format string : type 
 *
 *               u : unsigned <nl>
 *               d : int      <nl>
 *               c : char     <nl>
 *               s : string   <nl>
 *               x : 4-bit hex number  <nl>
 *               B : 8-bit hex number  <nl>
 *               H : 16-bit hex number <nl>
 *               X : 32-bit hex number <nl>
 *
 *        <f EdbgOutputDebugString> will eventually call the <f OEMWriteDebugByte> function to write
 *        characters to the debug serial port.
 */
void EdbgOutputDebugString(
                           const char *fmt,     // @parm [IN] - Format string
                           ...);                // @parmvar [IN] - Variable argument list
void EdbgDumpHexBuf(PUCHAR pBuf, DWORD Count);
char *inet_ntoa(DWORD dwIP);
DWORD inet_addr( char *pszDottedD );

BOOL EdbgRegisterDfltClient(UCHAR Service, UCHAR Flags, UCHAR **ppTxBuffer, UCHAR **ppRxBuffer);
// Called by kernel to install EDBG ISR
BOOL EdbgInitializeInterrupt();


// Prototypes for Ethernet controller library functions
typedef BOOL   (*PFN_EDBG_INIT)(BYTE *pbBaseAddress, DWORD dwMultiplier, USHORT MacAddr[3]);
typedef void   (*PFN_EDBG_ENABLE_INTS)(void);
typedef void   (*PFN_EDBG_DISABLE_INTS)(void);
typedef DWORD  (*PFN_EDBG_GET_PENDING_INTS)(void);
typedef UINT16 (*PFN_EDBG_GET_FRAME)(BYTE *pbData, UINT16 *pwLength );
typedef UINT16 (*PFN_EDBG_SEND_FRAME)(BYTE *pbData, DWORD dwLength );
typedef BOOL   (*PFN_EDBG_READ_EEPROM)(UINT16 EEPROMAddress, UINT16 *pwVal);
typedef BOOL   (*PFN_EDBG_WRITE_EEPROM)(UINT16 EEPROMAddress, UINT16 Data );
typedef DWORD  (*PFN_EDBG_SET_OPTIONS)(DWORD dwOptions);

// Prototypes for SMC9000.LIB functions
BOOL   SMCInit( BYTE *pbBaseAddress, DWORD dwMultiplier, USHORT MacAddr[3]);
void   SMCEnableInts(void);
void   SMCDisableInts(void);
DWORD  SMCGetPendingInterrupts(void);
UINT16 SMCGetFrame( BYTE *pbData, UINT16 *pwLength );
UINT16 SMCSendFrame( BYTE *pbData, DWORD dwLength );
BOOL   SMCReadEEPROM( UINT16 EEPROMAddress, UINT16 *pwVal);
BOOL   SMCWriteEEPROM( UINT16 EEPROMAddress, UINT16 Data );
UINT16 SMCReadSwitches(void);
DWORD  SMCSetOptions(DWORD dwOptions);
#define  OPT_BROADCAST_FILTERING  0x0001   // Filter out broadcast packets (except ARP packets)

// Prototypes for NE2000.LIB functions
BOOL   NE2000Init( BYTE *pbBaseAddress, DWORD dwMultiplier, USHORT MacAddr[3]);
void   NE2000EnableInts(void);
void   NE2000DisableInts(void);
DWORD  NE2000GetPendingInts(void);
UINT16 NE2000GetFrame( BYTE *pbData, UINT16 *pwLength );
UINT16 NE2000SendFrame( BYTE *pbData, DWORD dwLength );
BOOL   NE2000ReadEEPROM( UINT16 EEPROMAddress, UINT16 *pwVal);
BOOL   NE2000WriteEEPROM( UINT16 EEPROMAddress, UINT16 Data );
DWORD  NE2000SetOptions(DWORD dwOptions);

// Prototypes for the EBOOT.LIB functions
UINT16 EbootInitDHCP( EDBG_ADDR *pMyAddr );
BOOL   EbootSendUDP(BYTE *pFrameBuffer, EDBG_ADDR *pDestAddr, EDBG_ADDR *pSrcAddr, BYTE *pData, UINT16 cwLength);
UINT16 EbootProcessARP( EDBG_ADDR *pMyAddr, BYTE *pFrameBuffer );
// Return codes from EbootProcessARP
#define PROCESS_ARP_IGNORE       0
#define PROCESS_ARP_REQUEST      1
#define PROCESS_ARP_REQUEST_ERR  2 // EbootProcessARP failed to send response
#define PROCESS_ARP_RESPONSE     3 // Some other station has our IP address

UINT16 EbootGratuitousARP( EDBG_ADDR *pMyAddr, BYTE *pFrameBuffer );

UINT16 EbootProcessDHCP( EDBG_ADDR *pMyAddr, DWORD *pSubnetMask, BYTE *pbData, WORD cwLength, DWORD *pLease, BOOL *pfwDHCPComplete );
UINT16 EbootDHCPRetransmit( EDBG_ADDR *pMyAddr, EDBG_ADDR *pSrcAddr, UCHAR *pFrame );
UINT16 EbootCheckUDP(EDBG_ADDR *pMyAddr, BYTE *pFrameBuffer, UINT16 *wDestPort, UINT16 *wSrcPort, UINT16 **pwData, UINT16 *cwLength );
BOOL   EbootProcessEDBG(EDBG_ADDR *pMyAddr, EDBG_ADDR *pEshellHost, BYTE *pFrameBuffer,
                        UINT16 *pwUDPData, UINT16 cwUDPDataLength,BOOL *pfJump, EDBG_OS_CONFIG_DATA **ppCfgData);
void   EbootSendBootme(EDBG_ADDR *pMyAddr, UCHAR VersionMajor, UCHAR VersionMinor, char *szPlatformString, char *szDeviceName, UCHAR CPUId, DWORD dwBootFlags);
BOOL   EbootReadSerialIP( EDBG_ADDR *pMyAddr, DWORD *pSubnetMask);
void EbootInitTFtp( UINT16 wOdoWKSP, UINT16 wHostWKSP );
void   EbootInitTFtpd( void );
// These are the various operations that may be passed to the registered callback function by the
//  server process.  Their descriptions are:
typedef enum {
	TFTPD_OPEN,		// A new link has just been requested 
	TFTPD_READ,		// The link is requesting more data
	TFTPD_WRITE,	// The link is delivering data
	TFTPD_DESTROY	// An error occurred which as forced termination of the link
} TFtpdCallBackOps;
typedef UINT16 (*TFtpdCallBackFunc)( char *pszFileName, TFtpdCallBackOps Operation, BYTE *pbData, UINT16 *cwLength, char **ppszErrorMsg );
UINT16 EbootTFtpdServerRegister( char *pszFileName, TFtpdCallBackFunc pfCallBack );
WORD EbootTFtpReceiver( EDBG_ADDR *pMyAddr, BYTE *pFrameBuffer, UINT16 wDestPort, UINT16 wSrcPort, UINT16 *pwUDPData, UINT16 cwUDPDataLength );



// KernelIoControls() for HAL Ethernet services.
//
// IOCTL_HAL_SEND_UDP
//    Sends a UDP Packet via the kernel ethernet services.
//
//			UDP_PACKET UDPPacket;
//			UDPPacket.dwDestIP = <IPAddr>;
//			UDPPacket.DestMAC = <MAC Addr>;
//			UDPPacket.wDestPort = <Dest Port>;
//			UDPPacket.wSrcPort = <Src Port>;
//			UDPPacket.wPacketLen = <Length of Packet>;
//			memcpy (UDPPacket.Packet, <DATA>m UDPPacket.wPacketLen);
//			if (!KernelIoControl (IOCTL_HAL_SEND_UDP, &UDPPacket, 
//							 sizeof(UDPPacket), NULL, 0, NULL)) {
//				RETAILMSG (1, (TEXT("Error sending Packet %d\r\n"),
//					GetLastError()));
//			}
//

// KernelIoControl Codes are defined in pkfuncs.h


// structure for KernelIoControl(IOCTL_HAL_SEND_UDP).  This struct both indicates
// address and port information, and serves as a frame buffer.  The first 42 bytes
// of this struct will be overwritten with the Ether/IP/UDP headers.
#define MAX_PACKET	1400

typedef struct _UDP_PACKET
{
	UINT32		dwDestIP;			// The IP Address
	BYTE		DestMAC[6];			// The MAC address.
	UINT16		wSrcPort;
	UINT16		wDestPort;
	UINT16		wPacketLen;
    BYTE        Reserved[26];
	BYTE		Packet[MAX_PACKET]; // This must fall at offset 42
} UDP_PACKET, *PUDP_PACKET;

// Indexes we use to get IP and Mac Addresses (via the KernelIOControl service)
#define IPINFO_ODO				0
#define IPINFO_DOWNLOAD			1
#define IPINFO_DEBUGMSG			2
#define IPINFO_KDEBUG			3
#define IPINFO_ESHELL			4

// Structure returned from IOCTL_HAL_GET_IP_ADDR
typedef struct _IP_INFO {
	UINT32		dwIP;				// The IP Address
	BYTE		MAC[6];				// The MAC address.
} IP_INFO, *PIP_INFO;	



// Define some standard ports we use.
// NOTE: The following port is reserved thru the IANA for use
// by the remote replication agent.  They only use it over TCP
// Use only for UDP, 5679 is also available for UDP...
#define ETHER_STAT_PORT			5678


#ifdef __cplusplus
}
#endif


#endif // _HALETHER_H_
