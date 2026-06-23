/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

Module Name:  

    atapi.h

Abstract:

    This module contains the function prototypes and constant, type and
    structure definitions for the WINCE Implementation of the AT
    Attachment API for disk devices.

Notes: 


--*/
#ifndef _ATAPI_H_
#define _ATAPI_H_

#ifdef __cplusplus
extern "C" {
#endif

//
// ATA register definitions, as offsets from a base
//
#define ATA_IO_REG_BASE     0x170
#define ATA_IO_REG_BASE_SECONDARY     0x1F0
#define ATA_MEM_REG_BASE    0    
#define ATA_REG_DATA        0
#define ATA_REG_FEATURE     1   // write
#define ATA_REG_ERROR       1   // read
#define ATA_REG_SECT_CNT    2
#define ATA_REG_SECT_NUM    3
#define ATA_REG_CYL_LOW     4
#define ATA_REG_CYL_HIGH    5
#define ATA_REG_DRV_HEAD    6
#define ATA_REG_COMMAND     7   // write
#define ATA_REG_STATUS      7   // read     (reading this acknowledges the interrupt)
#define ATA_REG_LENGTH      ATA_REG_STATUS+1

#define ATA_ALT_IO_REG_BASE    0x376
#define ATA_ALT_IO_REG_BASE_SECONDARY    0x3F6
#define ATA_ALT_MEM_REG_BASE   (ATA_MEM_REG_BASE + 0xE)
#define ATA_ALT_REG_LENGTH  1
    
#define ATA_REG_ALT_STATUS  0   // read     (reading this does not ack the interrupt)
#define ATA_REG_DRV_CTRL    0   // write

//
// ATA error register bit masks.
//
#define ATA_ERROR_GENERAL          0x01
#define ATA_ERROR_ABORTED          0x04
#define ATA_ERROR_BAD_SECT_NUM     0x10
#define ATA_ERROR_UNCORRECTABLE    0x40
#define ATA_ERROR_BAD_BLOCK        0x80

//
// ATA commands for the command register.
//
#define ATA_CMD_RECALIBRATE         0x10 // move drive heads to track 0
#define ATA_CMD_READ                0x20 // retries enabled
#define ATA_CMD_WRITE               0x30 // retries enabled
#define ATA_CMD_SEEK                0x70
#define ATA_CMD_SET_DRIVE_PARMS     0x91 // set drive parameters
#define ATA_CMD_IDLE                0x97 // identify drive parameters
#define ATA_CMD_IDENTIFY            0xEC // identify drive parameters

//
// ATA status register bit masks.
//                                  
#define ATA_STATUS_ERROR            0x01 // error bit in status register
#define ATA_STATUS_CORRECTED_ERROR  0x04 // corrected error in status register
#define ATA_STATUS_DATA_REQ         0x08 // data request bit in status register
#define ATA_STATUS_SEEK_DONE        0x10 // DSC - Drive Seek Complete
#define ATA_STATUS_WRITE_FAULT      0x20 // DWF - Drive Write Fault
#define ATA_STATUS_READY            0x40
#define ATA_STATUS_BUSY             0x80

//
// ATA drive select/head register bit masks.
//
#define ATA_HEAD_MUST_BE_ON         0xA0 // These 2 bits are always on
#define ATA_HEAD_DRIVE_1            (0x00 | ATA_HEAD_MUST_BE_ON)
#define ATA_HEAD_DRIVE_2            (0x10 | ATA_HEAD_MUST_BE_ON)
#define ATA_HEAD_LBA_MODE           (0x40 | ATA_HEAD_MUST_BE_ON)

//
// ATA device control register bit masks.
//
#define ATA_CTRL_ENABLE_INTR        0x00
#define ATA_CTRL_DISABLE_INTR       0x02
#define ATA_CTRL_RESET              0x04

#pragma pack(1)
//
// Data returned by the ATA_CMD_IDENTIFY command
//
typedef struct _IDENTIFY_DATA {
    USHORT GeneralConfiguration;            // 00
    USHORT NumberOfCylinders;               // 01
    USHORT Reserved1;                       // 02
    USHORT NumberOfHeads;                   // 03
    USHORT UnformattedBytesPerTrack;        // 04
    USHORT UnformattedBytesPerSector;       // 05
    USHORT SectorsPerTrack;                 // 06
    USHORT VendorUnique1[3];                // 07
    USHORT SerialNumber[10];                // 10
    USHORT BufferType;                      // 20
    USHORT BufferSectorSize;                // 21
    USHORT NumberOfEccBytes;                // 22
    USHORT FirmwareRevision[4];             // 23
    USHORT ModelNumber[20];                 // 27
    UCHAR  MaximumBlockTransfer;            // 47 low byte
    UCHAR  VendorUnique2;                   // 47 high byte
    USHORT DoubleWordIo;                    // 48
    USHORT Capabilities;                    // 49
    USHORT Reserved2;                       // 50
    UCHAR  VendorUnique3;                   // 51 low byte
    UCHAR  PioCycleTimingMode;              // 51 high byte
    UCHAR  VendorUnique4;                   // 52 low byte
    UCHAR  DmaCycleTimingMode;              // 52 high byte
    USHORT TranslationFieldsValid;          // 53 (low bit)
    USHORT NumberOfCurrentCylinders;        // 54
    USHORT NumberOfCurrentHeads;            // 55
    USHORT CurrentSectorsPerTrack;          // 56
    ULONG  CurrentSectorCapacity;           // 57 & 58
    UCHAR  MultiSectorCount;                // 59 low
    UCHAR  MultiSectorSettingValid;         // 59 high (low bit)
    ULONG  TotalUserAddressableSectors;     // 60 & 61
    UCHAR  SingleDmaModesSupported;         // 62 low byte
    UCHAR  SingleDmaTransferActive;         // 62 high byte
    UCHAR  MultiDmaModesSupported;          // 63 low byte
    UCHAR  MultiDmaTransferActive;          // 63 high byte
    USHORT Reserved[192];                   // 64
} IDENTIFY_DATA, *PIDENTIFY_DATA;

#pragma pack()

#ifdef __cplusplus
}
#endif

#endif // _ATAPI_H_

