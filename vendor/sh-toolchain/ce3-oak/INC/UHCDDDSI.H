/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

Module Name:  
    uhcdddsi.h
    
Abstract:  
    This file provides the definitions for the USB UHCD MDD->PDD 
    interface.
Notes: 

    @doc DRIVERS           
    
    @topic   Creating an UHCI driver for Windows CE    |
    
             The Windows CE Universal Host Controller Driver (UHCD) module consists of
             two portions - a platform independant portion (MDD) which is provided with
             the OS in public\\common\\oak\\drivers\\usb\\hcd\\uhcd, and a platform 
             dependant portion (PDD), which must be written by the OEM.  
             
             The bulk of the processing occurs in the MDD, with the PDD providing 
             the necessary glue to interface the hardware to a particular platform.  
             Since the UHCI spec is very detailed as to how the hardware should behave, 
             there is very little that needs to be done in the PDD, primarily locating 
             the card at init, and providing the MDD with pointers to the hardware and 
             shared memory area.
             
             To configure the UHCI driver to be loaded by device.exe during system
             startup, the standard driver registry settings should be added to the
             platform.reg file, for example
             

             ; USB - UHCI driver <nl>
             [HKEY_LOCAL_MACHINE\\Drivers\\BuiltIn\\UHCI] <nl>
                "Prefix"="HCD"   <nl>
                "Dll"="uhci.dll" <nl>
                "Index"=dword:1  <nl>
                "Order"=dword:1  <nl>
             
             This will cause device.exe to load uhci.dll, and call the initialization
             routine.
--*/
#ifndef   __UHCDDDSI_H__
#define   __UHCDDDSI_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Debug defs */
#ifdef DEBUG
#define ZONE_UHCD               DEBUGZONE(0)
#define ZONE_INIT               DEBUGZONE(1)
#define ZONE_REGISTERS          DEBUGZONE(2)
#define ZONE_HUB                DEBUGZONE(3)
#define ZONE_ATTACH             DEBUGZONE(4)
#define ZONE_DESCRIPTORS        DEBUGZONE(5)
#define ZONE_FUNCTION           DEBUGZONE(6)
#define ZONE_PIPE               DEBUGZONE(7)
#define ZONE_TRANSFER           DEBUGZONE(8)
#define ZONE_QH                 DEBUGZONE(9)
#define ZONE_TD                 DEBUGZONE(10)
#define ZONE_CPHYSMEM           DEBUGZONE(11)
#define ZONE_VERBOSE            DEBUGZONE(12)
#define ZONE_WARNING            DEBUGZONE(13)
#define ZONE_ERROR              DEBUGZONE(14)
#define ZONE_UNUSED             DEBUGZONE(15)

#define ZONE_DEVICE             (ZONE_HUB || ZONE_FUNCTION)
#endif

/* @topic  UHCD MDD functions |
 *         The following functions are exported by the UHCD MDD code.  They must be 
 *         called at various points in the PDD code to allow the MDD to update state.
 */

/* @func  LPVOID | UhcdMdd_CreateMemoryObject | Create the UHCI shared memory area.
 * 
 * @comm  This function is called by the UHCD PDD to initialize the shared memory area used
 *        by the UHCI driver.  This area is used to allocate DMA buffers for client driver
 *        transfers.
 *
 *        There are two options available - if a platform has an area of memory set aside that
 *        can be used by UHCI (e.g. specified in the config.bib file), it may pass the virtual
 *        and physical address pointers in to this function. This is the recommended method.
 *        Alternatively, the PDD may pass NULL in for the physical and virtual address pointers,
 *        in which case the <f UhcdMdd_CreateMemoryObject> function will attempt to allocate a
 *        suitable memory buffer from system RAM. 
 *
 *        The UHCI shared memory area is broken into two separate areas - a high priority pool
 *        for isochronous client buffers, and a lower priority pool for other transfers.
 *        Refer to the section on client driver transfers for a description of how client
 *        driver buffer management works.
 *
 *        The size of the shared memory area is dependant on the perceived usage of USB within the
 *        platform.  The UHCI controller uses 4K page boundaries for internal data structures,
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
 *        If possible, OEMs should consider placing the UHCD shared memory area in high 
 *        speed memory, particularly to support streaming (isochronous) data transfers.
 *
 * @rdesc Returns pointer to UHCD memory object, or NULL if failure occurs.
 */
LPVOID  UhcdMdd_CreateMemoryObject(
                                   DWORD cbSize,            // @parm [IN] - Total size of UHCD shared memory area.
                                   DWORD cbHighPrioritySize,// @parm [IN] - Bytes reserved for high priority (isoch/interrupt) transfers
                                   PUCHAR pVirtAddr,        // @parm [IN] - Virtual address of memory area (may be NULL).
                                   PUCHAR pPhysAddr);       // @parm [IN] - Physical address of memory area (may be NULL).

/* @func  BOOL | UhcdMdd_DestroyMemoryObject | Clean up an UHCI memory object.
 *
 * @comm  This function should be called when an UHCI controller is removed, or if an error
 *        occurs during initialization, to allow the MDD to clean up internal data structures.
 *
 * @rdesc Returns TRUE if successful, FALSE if error occurs.
 */
BOOL    UhcdMdd_DestroyMemoryObject(
                                    LPVOID lpvMemoryObject); // @parm [IN] - Pointer to UHCD memory object.

/* @func  LPVOID | UhcdMdd_CreateUhcdObject | Create and initialize UHCI driver object.
 * 
 * @comm  This function is called by the PDD during initialization to initialize an UHCI
 *        driver object. As part of this processing, the USB Driver (USBD.DLL) is loaded,
 *        and the UHCI interrupt is hooked into the system.  Then, the UHCI hardware is
 *        placed into OPERATIONAL state, and list processing begins (device attach events
 *        are serviced, transfers initiated, etc).
 *        
 *        There can only be one UHCI host controller active in the system.
 *
 * @rdesc Returns pointer to UHCI driver object, or NULL if failure occurs.
 */
LPVOID  UhcdMdd_CreateUhcdObject(
                                 LPVOID lpvUhcdPddObject, // @parm [IN] - Pointer to PDD specific data for this instance.
                                 LPVOID lpvMemoryObject,  // @parm [IN] - Memory object created by <f UhcdMdd_CreateMemoryObject>.
                                 LPCWSTR szRegKey,        // @parm [IN] - Registry key where UHCI configuration settings are stored.
                                 PUCHAR ioPortBase,       // @parm [IN] - Pointer to UHCI controller registers.
                                 DWORD dwSysIntr);        // @parm [IN] - Logical value for UHCI interrupt (SYSINTR_xx).

/* @func  BOOL | UhcdMdd_DestroyUhcdObject | Clean up an UHCI driver object.
 *
 * @comm  This function should be called when an UHCI controller is removed, or if an error
 *        occurs during initialization, to allow the MDD to clean up internal data structures.
 * @rdesc Returns TRUE if successful, FALSE if error occurs.
 */
BOOL    UhcdMdd_DestroyUhcdObject(
                                  LPVOID lpvUhcdObject); // @parm [IN] - Pointer to UHCI driver object.

/* @func  BOOL | UhcdMdd_PowerUp | UHCD MDD power up handler.
 *
 * @comm  This function should be called by the PDD as part of the processing during
 *        <f UhcdPdd_PowerUp>.  It will cause the MDD to restore state and reinitialize
 *        the UHCI hardware.  All client drivers that were loaded at the time of power
 *        down will be unloaded.  Once this is completed, the UHCI hardware will be placed
 *        into the OPERATIONAL state, and drivers for devices which are still attached to
 *        the USB will be loaded.
 * @rdesc Return TRUE if successful, FALSE if error occurs.
 */
BOOL    UhcdMdd_PowerUp(
                        LPVOID lpvUhcdObject);   // @parm [IN] - Pointer to UHCI driver object.

/* @func  BOOL | UhcdMdd_PowerDown | UHCD MDD power down handler.
 *
 * @comm  This function should be called by the PDD as part of the processing during
 *        <f UhcdPdd_PowerDown>.  It will cause the MDD to save state and prepare the
 *        UHCI controller for power down.  The UHCI controller will be placed into
 *        reset (SOF generation disabled).
 * @rdesc Return TRUE if successful, FALSE if error occurs.
 */
BOOL    UhcdMdd_PowerDown(
                          LPVOID lpvUhcdObject);   // @parm [IN] - Pointer to UHCI driver object.

/* @topic  UHCD PDD functions |
 *         The following functions must be provided by the UHCD PDD code.
 *
 */


/* @func  BOOL | UhcdPdd_DllMain | DLL Entry point for PDD portion of UHCI driver.
 *
 * @comm  This function is provided to allow the PDD to perform any processing at
 *        DLL entry time.  Generally, this function doesn't do much and most initialization
 *        should be performed in the <f UhcdPdd_Init> routine.
 *
 * @rdesc Return TRUE if successful, FALSE if error occurs.
 */
BOOL    UhcdPdd_DllMain(
                        HANDLE hinstDLL,    // @parm [IN] - Handle to DLL instance.
                        DWORD  dwReason,    // @parm [IN] - Reason code. 
                        LPVOID lpvReserved);// @parm [IN] - System parameter.

/* @func  DWORD | UhcdPdd_Init | Initialize UHCI platform specific HW and structures.
 *
 * @comm  PDD Entry point - called at system init to detect and configure UHCI card.
 *        As part of the processing for this function, the <f UhcdMdd_CreateUhcdObject>
 *        function should be called to initialize the MDD and UHCI hardware.
 *
 * @rdesc Return pointer to PDD specific data structure, or NULL if error.
 */
DWORD   UhcdPdd_Init(
                     DWORD dwContext);  // @parm [IN] - Pointer to a string indicating our active registry key.

/* @func  BOOL | UhcdPdd_CheckConfigPower | Check power for requested configuration.
 *
 * @comm  When a device is attached to a root hub port, UHCI selects a configuration based
 *        on the power requirements of the system, by calling this function for each
 *        configuration and then selecting the configuration with the highest power
 *        requirements. If no configurations can be supported, the device is not configured.
 *
 *        This allows the platform to perform additional processing if necessary (e.g.
 *        checking whether on AC or batteries, possibly sampling current system power
 *        status, etc). 
 *
 * @rdesc Return TRUE if configuration can be supported, FALSE if not.
 */
BOOL    UhcdPdd_CheckConfigPower(
                                 UCHAR bPort,         // @parm [IN] - Root hub port # (1-N).
                                 DWORD dwConfigPower, // @parm [IN] - Milliamps required by configuration.
                                 DWORD dwTotalPower); // @parm [IN] - Power drawn by other devices on this port (ma).

/* @func  void | UhcdPdd_PowerUp | UHCD power up handler.
 *
 * @comm  This function is called during the system power on sequence.  
 *        In this function, the <f UhcdMdd_PowerUp> function should be called to allow
 *        the MDD to restore state information.  The UHCI memory area (as passed to  
 *        <f UhcdMdd_CreateUhcdObject>) must be preserved across a suspend/resume cycle.  
 *        Note that no system calls may be made from within this function.
 */
void    UhcdPdd_PowerUp(
                        DWORD dwDeviceContext); // @parm [IN] - Pointer to device context returned from <f UhcdPdd_Init>

/* @func  void | UhcdPdd_PowerDown | UHCD power off handler.
 *
 * @comm  This function is called during the system power off sequence.
 *        In this function, the <f UhcdMdd_PowerDown> function should be called to allow
 *        the MDD to save state information.  Then, any processing specific to the platform
 *        (e.g. removing power from the host controller hardware) can be done.  Note that
 *        no system calls may be made from within this function.
 */
void    UhcdPdd_PowerDown(
                          DWORD dwDeviceContext);  // @parm [IN] - Pointer to device context returned from <f UhcdPdd_Init>

/* @func  BOOL | UhcdPdd_Deinit | UHCD deinitialization routine.
 * 
 * @comm  This function is called if the UHCI driver is unloaded.  As part of the processing
 *        for this function, <f UhcdMdd_DestroyUhcdObject> should be called to allow the MDD
 *        to clean up internal data structures.
 * @rdesc Return TRUE if deinit was successful, FALSE if error occurred.
 */
BOOL    UhcdPdd_Deinit(
                       DWORD dwDeviceContext); // @parm [IN] - Pointer to device context returned from <f UhcdPdd_Init>

/* @func  DWORD | UhcdPdd_Open | UHCD open routine.
 *
 * @comm  This is the standard stream interface driver open routine.  It is currently unused.
 * @rdesc For future compatibility, drivers should return 1.
 */
DWORD   UhcdPdd_Open(
                     DWORD dwDeviceContext,  // @parm [IN] - Pointer to device context returned from <f UhcdPdd_Init>
                     DWORD dwAccessCode,     // @parm - Unused
                     DWORD dwShareMode);     // @parm - Unused

/* @func  BOOL | UhcdPdd_Close | UHCD close routine.
 *
 * @comm  This is the standard stream interface driver close routine.  It is currently unused.
 * @rdesc For future compatibility, drivers should return TRUE.
 */
BOOL    UhcdPdd_Close(
                      DWORD dwOpenContext);  // @parm [IN] - Pointer to device context returned from <f UhcdPdd_Init>

/* @func  DWORD | UhcdPdd_Read | UHCD read routine.
 *
 * @comm  This is the standard stream interface driver read routine.  It is currently unused.
 * @rdesc For future compatibility, drivers should return -1.
 */
DWORD   UhcdPdd_Read(
                     DWORD dwOpenContext,    // @parm [IN] - Pointer to device context returned from <f UhcdPdd_Init>
                     LPVOID lpvBuffer, 
                     DWORD dwCount);

/* @func  DWORD | UhcdPdd_Write | UHCD write routine.
 *
 * @comm  This is the standard stream interface driver write routine.  It is currently unused.
 * @rdesc For future compatibility, drivers should return -1.
 */
DWORD   UhcdPdd_Write(
                      DWORD dwOpenContext,    // @parm [IN] - Pointer to device context returned from <f UhcdPdd_Init>
                      LPCVOID lpvSourceBytes,
                      DWORD dwNumberOfBytes);

/* @func  DWORD | UhcdPdd_Seek | UHCD seek routine.
 *
 * @comm  This is the standard stream interface driver seek routine.  It is currently unused.
 * @rdesc For future compatibility, drivers should return -1.
 */
DWORD   UhcdPdd_Seek(
                     DWORD dwOpenContext,     // @parm [IN] - Pointer to device context returned from <f UhcdPdd_Init>
                     LONG lAmount, 
                     DWORD dwType);

/* @func  BOOL | UhcdPdd_IOControl | UHCD ioctl routine.
 *
 * @comm  This is the standard stream interface driver ioctl routine.  It is currently unused.
 * @rdesc For future compatibility, drivers should return FALSE.
 */
BOOL    UhcdPdd_IOControl(
                          DWORD dwOpenContext, // @parm [IN] - Pointer to device context returned from <f UhcdPdd_Init>
                          DWORD dwCode, 
                          PBYTE pbBufIn,
                          DWORD dwLenIn, 
                          PBYTE pbBufOut, 
                          DWORD dwLenOut,
                          PDWORD pdwActualOut);


/* @func  void | UhcdPdd_InitiatePowerUp.
 *
 * @comm  This function will reinitialize the UHCD after a Resume event. It should be called from the USB IST after a
 *        resume has been detected.
 * 
 */
void    UhcdPdd_InitiatePowerUp(void);



#ifdef __cplusplus
}
#endif

#endif
