/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

Module Name:  
    ohcdddsi.h
    
Abstract:  
    This file provides the definitions for the USB OHCD MDD->PDD 
    interface.
Notes: 

    @doc DRIVERS           
    
    @topic   Creating an OHCI driver for Windows CE    |
    
             The Windows CE Open Host Controller Driver (OHCD) module consists of
             two portions - a platform independant portion (MDD) which is provided with
             the OS in public\\common\\oak\\drivers\\usb\\hcd\\ohcd, and a platform 
             dependant portion (PDD), which must be written by the OEM.  
             
             The bulk of the processing occurs in the MDD, with the PDD providing 
             the necessary glue to interface the hardware to a particular platform.  
             Since the OHCI spec is very detailed as to how the hardware should behave, 
             there is very little that needs to be done in the PDD, primarily locating 
             the card at init, and providing the MDD with pointers to the hardware and 
             shared memory area.
             
             To configure the OHCI driver to be loaded by device.exe during system
             startup, the standard driver registry settings should be added to the
             platform.reg file, for example
             

             ; USB - OHCI driver <nl>
             [HKEY_LOCAL_MACHINE\\Drivers\\BuiltIn\\OHCI] <nl>
                "Prefix"="HCD"   <nl>
                "Dll"="ohci.dll" <nl>
                "Index"=dword:1  <nl>
                "Order"=dword:1  <nl>
             
             This will cause device.exe to load ohci.dll, and call the initialization
             routine.
--*/
#ifndef   __OHCDDDSI_H__
#define   __OHCDDDSI_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Debug defs */
#ifdef DEBUG
#define ZONE_INIT               DEBUGZONE(0)
#define ZONE_ATTACH             DEBUGZONE(1)
#define ZONE_DESCRIPTORS        DEBUGZONE(2)
#define ZONE_HUB                DEBUGZONE(3)
#define ZONE_PCI                DEBUGZONE(4)
#define ZONE_INTR               DEBUGZONE(5)
#define ZONE_TD                 DEBUGZONE(6)
#define ZONE_ED                 DEBUGZONE(7)
#define ZONE_CRITICAL           DEBUGZONE(8)
#define ZONE_UNUSED1            DEBUGZONE(9)
#define ZONE_UNUSED2            DEBUGZONE(10)
#define ZONE_UNUSED3            DEBUGZONE(11)
#define ZONE_ALLOC              DEBUGZONE(12)
#define ZONE_FUNCTION           DEBUGZONE(13)
#define ZONE_WARNING            DEBUGZONE(14)
#define ZONE_ERROR              DEBUGZONE(15)

#define ZONE_VERBOSE            DEBUGZONE(16) // Adds extra verbosity to some zones
#endif

/* @topic  OHCD MDD functions |
 *         The following functions are exported by the OHCD MDD code.  They must be 
 *         called at various points in the PDD code to allow the MDD to update state.
 */

/* @func  LPVOID | OhcdMdd_CreateMemoryObject | Create the OHCI shared memory area.
 * 
 * @comm  This function is called by the OHCD PDD to initialize the shared memory area used
 *        by the OHCI driver.  This area is used for the Host Controller Communication
 *        Area (HCCA) to communicate with the OHCI hardware, and is also used to allocate 
 *        DMA buffers for client driver transfers.  There are two options available - if a 
 *        platform has an area of memory set aside that can be used by OHCI (e.g. specified 
 *        in the config.bib file), it may pass the virtual and physical address pointers in to this function.
 *        This is the recommended method.  Alternatively, the PDD may pass NULL in for the
 *        physical and virtual address pointers, in which case the <f OhcdMdd_CreateMemoryObject>
 *        function will attempt to allocate a suitable memory buffer from system RAM. 
 *
 *        The OHCI shared memory area is broken into two separate areas - a high priority pool
 *        of buffers for the HCCA area and isochronous client buffers, and a lower priority
 *        pool of buffers for other client transfers.  Refer to the section on client driver
 *        transfers for a description of how client driver buffer management works.
 *
 *        The size of the shared memory area is dependant on the perceived usage of USB within the
 *        platform.  The OHCI controller uses 4K page boundaries for internal data structures,
 *        so the size should be a multiple of 4K, and must be at least 8K, for the control
 *        structures and memory needed for device configuration.  Additionally, client drivers
 *        may need to utilize this area for their transfers, so it is recommended that additional
 *        memory be provided.  The maximum DMA memory used for a client transfer is 8K, so OEMs should
 *        try to determine what a reasonable number of outstanding client transfers may be.
 *
 *        A recommended value for general purpose USB operation is 38K, with 20K reserved 
 *        for high priority transfers.  Platforms which anticipate less demand on the USB subsystem
 *        can safely reduce this to 28K (20K high priority).  Reducing the size of the high priority
 *        buffer may cause isoch transfers to fail if memory cannot be allocated, whereas reducing
 *        the size of the non high priority buffer will reduce performance on bulk transfers (but
 *        won't cause failures, since bulk transfers can block to wait for memory to become available).
 *
 *        If possible, OEMs should consider placing the OHCD shared memory area in high 
 *        speed memory, particularly to support streaming (isochronous) data transfers.
 *
 * @rdesc Returns pointer to OHCD memory object, or NULL if failure occurs.
 */
LPVOID  OhcdMdd_CreateMemoryObject(
                                   DWORD cbSize,            // @parm [IN] - Total size of OHCD shared memory area.
                                   DWORD cbHighPrioritySize,// @parm [IN] - Bytes reserved for high priority (isoch/interrupt) transfers
                                                            //              and OHCI HCCA area.
                                   PUCHAR pVirtAddr,        // @parm [IN] - Virtual address of memory area (may be NULL).
                                   PUCHAR pPhysAddr);       // @parm [IN] - Physical address of memory area (may be NULL).

/* @func  BOOL | OhcdMdd_DestroyMemoryObject | Clean up an OHCI memory object.
 *
 * @comm  This function should be called when an OHCI controller is removed, or if an error
 *        occurs during initialization, to allow the MDD to clean up internal data structures.
 *
 * @rdesc Returns TRUE if successful, FALSE if error occurs.
 */
BOOL    OhcdMdd_DestroyMemoryObject(
                                    LPVOID lpvMemoryObject); // @parm [IN] - Pointer to OHCD memory object.

/* @func  LPVOID | OhcdMdd_CreateOhcdObject | Create and initialize OHCI driver object.
 * 
 * @comm  This function is called by the PDD during initialization to initialize an OHCI
 *        driver object. As part of this processing, the USB Driver (USBD.DLL) is loaded,
 *        and the OHCI interrupt is hooked into the system.  Then, the OHCI hardware is
 *        placed into OPERATIONAL state, and list processing begins (device attach events
 *        are serviced, transfers initiated, etc).
 *        
 *        The Windows CE USB subsystem can support multiple host controllers, the PDD should
 *        create an OHCI driver object for each physical host controller in the system.
 *
 * @rdesc Returns pointer to OHCI driver object, or NULL if failure occurs.
 */
LPVOID  OhcdMdd_CreateOhcdObject(
                                 LPVOID lpvOhcdPddObject, // @parm [IN] - Pointer to PDD specific data for this instance.
                                 LPVOID lpvMemoryObject,  // @parm [IN] - Memory object created by <f OhcdMdd_CreateMemoryObject>.
                                 LPCWSTR szRegKey,        // @parm [IN] - Registry key where OHCI configuration settings are stored.
                                 PUCHAR ioPortBase,       // @parm [IN] - Pointer to OHCI controller registers.
                                 DWORD dwSysIntr);        // @parm [IN] - Logical value for OHCI interrupt (SYSINTR_xx).

/* @func  BOOL | OhcdMdd_DestroyOhcdObject | Clean up an OHCI driver object.
 *
 * @comm  This function should be called when an OHCI controller is removed, or if an error
 *        occurs during initialization, to allow the MDD to clean up internal data structures.
 * @rdesc Returns TRUE if successful, FALSE if error occurs.
 */
BOOL    OhcdMdd_DestroyOhcdObject(
                                  LPVOID lpvOhcdObject); // @parm [IN] - Pointer to OHCI driver object.

/* @func  BOOL | OhcdMdd_PowerUp | OHCD MDD power up handler.
 *
 * @comm  This function should be called by the PDD as part of the processing during
 *        <f OhcdPdd_PowerUp>.  It will cause the MDD to restore state and reinitialize
 *        the OHCI hardware.  All client drivers that were loaded at the time of power
 *        down will be unloaded.  Once this is completed, the OHCI hardware will be placed
 *        into the OPERATIONAL state, and drivers for devices which are still attached to
 *        the USB will be loaded.
 * @rdesc Return TRUE if successful, FALSE if error occurs.
 */
BOOL    OhcdMdd_PowerUp(
                        LPVOID lpvOhcdObject);   // @parm [IN] - Pointer to OHCI driver object.

/* @func  BOOL | OhcdMdd_PowerDown | OHCD MDD power down handler.
 *
 * @comm  This function should be called by the PDD as part of the processing during
 *        <f OhcdPdd_PowerDown>.  It will cause the MDD to save state and prepare the
 *        OHCI controller for power down.  The OHCI controller will be placed into
 *        reset (SOF generation disabled).
 * @rdesc Return TRUE if successful, FALSE if error occurs.
 */
BOOL    OhcdMdd_PowerDown(
                          LPVOID lpvOhcdObject);   // @parm [IN] - Pointer to OHCI driver object.

/* @topic  OHCD PDD functions |
 *         The following functions must be provided by the OHCD PDD code.
 *
 */


/* @func  BOOL | OhcdPdd_DllMain | DLL Entry point for PDD portion of OHCI driver.
 *
 * @comm  This function is provided to allow the PDD to perform any processing at
 *        DLL entry time.  Generally, this function doesn't do much and most initialization
 *        should be performed in the <f OhcdPdd_Init> routine.
 *
 * @rdesc Return TRUE if successful, FALSE if error occurs.
 */
BOOL    OhcdPdd_DllMain(
                        HANDLE hinstDLL,    // @parm [IN] - Handle to DLL instance.
                        DWORD  dwReason,    // @parm [IN] - Reason code. 
                        LPVOID lpvReserved);// @parm [IN] - System parameter.

/* @func  DWORD | OhcdPdd_Init | Initialize OHCI platform specific HW and structures.
 *
 * @comm  PDD Entry point - called at system init to detect and configure OHCI card.
 *        As part of the processing for this function, the <f OhcdMdd_CreateOhcdObject>
 *        function should be called to initialize the MDD and OHCI hardware.
 *
 * @rdesc Return pointer to PDD specific data structure, or NULL if error.
 */
DWORD   OhcdPdd_Init(
                     DWORD dwContext);  // @parm [IN] - Pointer to a string indicating our active registry key.

/* @func  BOOL | OhcdPdd_CheckConfigPower | Check power for requested configuration.
 *
 * @comm  When a device is initially attached to the bus, OHCI selects a configuration based
 *        on the power requirements of the system, by calling this function for each
 *        configuration until one that can be supported is found.  If no configurations 
 *        in a device can be supported, the device is not configured.
 *
 *        This allows the platform to perform additional processing if necessary (e.g.
 *        checking whether on AC or batteries, possibly sampling current system power
 *        status, etc). 
 *
 * @rdesc Return TRUE if configuration can be supported, FALSE if not.
 */
BOOL    OhcdPdd_CheckConfigPower(
                                 UCHAR bPort,         // @parm [IN] - Root hub port # (1-N).
                                 DWORD dwConfigPower, // @parm [IN] - Milliamps required by configuration.
                                 DWORD dwTotalPower); // @parm [IN] - Power drawn by other devices on this port (ma).

/* @func  void | OhcdPdd_PowerUp | OHCD power up handler.
 *
 * @comm  This function is called during the system power on sequence.  
 *        In this function, the <f OhcdMdd_PowerUp> function should be called to allow
 *        the MDD to restore state information.  The OHCI memory area (as passed to  
 *        <f OhcdMdd_CreateOhcdObject>) must be preserved across a suspend/resume cycle.  
 *        Note that no system calls may be made from within this function.
 */
void    OhcdPdd_PowerUp(
                        DWORD dwDeviceContext); // @parm [IN] - Pointer to device context returned from <f OhcdPdd_Init>

/* @func  void | OhcdPdd_PowerDown | OHCD power off handler.
 *
 * @comm  This function is called during the system power off sequence.
 *        In this function, the <f OhcdMdd_PowerDown> function should be called to allow
 *        the MDD to save state information.  Then, any processing specific to the platform
 *        (e.g. removing power from the host controller hardware) can be done.  Note that
 *        no system calls may be made from within this function.
 */
void    OhcdPdd_PowerDown(
                          DWORD dwDeviceContext);  // @parm [IN] - Pointer to device context returned from <f OhcdPdd_Init>

/* @func  BOOL | OhcdPdd_Deinit | OHCD deinitialization routine.
 * 
 * @comm  This function is called if the OHCI driver is unloaded.  As part of the processing
 *        for this function, <f OhcdMdd_DestroyOhcdObject> should be called to allow the MDD
 *        to clean up internal data structures.
 * @rdesc Return TRUE if deinit was successful, FALSE if error occurred.
 */
BOOL    OhcdPdd_Deinit(
                       DWORD dwDeviceContext); // @parm [IN] - Pointer to device context returned from <f OhcdPdd_Init>

/* @func  DWORD | OhcdPdd_Open | OHCD open routine.
 *
 * @comm  This is the standard stream interface driver open routine.  It is currently unused.
 * @rdesc For future compatibility, drivers should return 1.
 */
DWORD   OhcdPdd_Open(
                     DWORD dwDeviceContext,  // @parm [IN] - Pointer to device context returned from <f OhcdPdd_Init>
                     DWORD dwAccessCode,     // @parm - Unused
                     DWORD dwShareMode);     // @parm - Unused

/* @func  BOOL | OhcdPdd_Close | OHCD close routine.
 *
 * @comm  This is the standard stream interface driver close routine.  It is currently unused.
 * @rdesc For future compatibility, drivers should return TRUE.
 */
BOOL    OhcdPdd_Close(
                      DWORD dwOpenContext);  // @parm [IN] - Pointer to device context returned from <f OhcdPdd_Init>

/* @func  DWORD | OhcdPdd_Read | OHCD read routine.
 *
 * @comm  This is the standard stream interface driver read routine.  It is currently unused.
 * @rdesc For future compatibility, drivers should return -1.
 */
DWORD   OhcdPdd_Read(
                     DWORD dwOpenContext,    // @parm [IN] - Pointer to device context returned from <f OhcdPdd_Init>
                     LPVOID lpvBuffer, 
                     DWORD dwCount);

/* @func  DWORD | OhcdPdd_Write | OHCD write routine.
 *
 * @comm  This is the standard stream interface driver write routine.  It is currently unused.
 * @rdesc For future compatibility, drivers should return -1.
 */
DWORD   OhcdPdd_Write(
                      DWORD dwOpenContext,    // @parm [IN] - Pointer to device context returned from <f OhcdPdd_Init>
                      LPCVOID lpvSourceBytes,
                      DWORD dwNumberOfBytes);

/* @func  DWORD | OhcdPdd_Seek | OHCD seek routine.
 *
 * @comm  This is the standard stream interface driver seek routine.  It is currently unused.
 * @rdesc For future compatibility, drivers should return -1.
 */
DWORD   OhcdPdd_Seek(
                     DWORD dwOpenContext,     // @parm [IN] - Pointer to device context returned from <f OhcdPdd_Init>
                     LONG lAmount, 
                     DWORD dwType);

/* @func  BOOL | OhcdPdd_IOControl | OHCD ioctl routine.
 *
 * @comm  This is the standard stream interface driver ioctl routine.  It is currently unused.
 * @rdesc For future compatibility, drivers should return FALSE.
 */
BOOL    OhcdPdd_IOControl(
                          DWORD dwOpenContext, // @parm [IN] - Pointer to device context returned from <f OhcdPdd_Init>
                          DWORD dwCode, 
                          PBYTE pbBufIn,
                          DWORD dwLenIn, 
                          PBYTE pbBufOut, 
                          DWORD dwLenOut,
                          PDWORD pdwActualOut);


#ifdef __cplusplus
}
#endif

#endif
