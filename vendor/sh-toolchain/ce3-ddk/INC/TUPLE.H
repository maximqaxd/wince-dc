/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

Module Name:  

    tuple.h

Abstract:  

    This file implements the Windows CE PCMCIA metaformat card information
    tuple structure definitions.  This is provided as a sample to platform
    writers and is expected to be able to be used without modification on most
    (if not all) hardware platforms.

Functions:


Notes:

    Since the tuples are compressed and must be processed sequentially,
    the definitions provided here are in the form of a union of all
    the elements in the tuple.  This allows a byte pointer to the union
    to be incremented as it references each of the elements of the tuple.

    Also described are the parsed structures returned by the function
    CardGetParsedTuple().

--*/

// @doc DRIVERS

#ifndef TUPLE_INC
#define TUPLE_INC

#pragma pack(1)

// Tuple Codes

#define CISTPL_NULL         0x00
#define CISTPL_DEVICE       0x01
#define CISTPL_LONGLINK_CB  0x02        // CardBus
#define CISTPL_CONFIG_CB    0x04        // CardBus
#define CISTPL_CFTABLE_ENTRY_CB 0x05    // CardBus
#define CISTPL_LONGLINK_MFC 0x06        // Multiple Function
#define CISTPL_LONG_LINK_MFC 0x06       // Multiple Function
#define CISTPL_CHECKSUM     0x10
#define CISTPL_LONGLINK_A   0x11
#define CISTPL_LONGLINK_C   0x12
#define CISTPL_LINKTARGET   0x13
#define CISTPL_NO_LINK      0x14
#define CISTPL_VERS_1       0x15
#define CISTPL_ALTSTR       0x16
#define CISTPL_DEVICE_A     0x17
#define CISTPL_JEDEC_C      0x18
#define CISTPL_JEDEC_A      0x19
#define CISTPL_CONFIG       0x1A
#define CISTPL_CFTABLE_ENTRY 0x1B
#define CISTPL_DEVICE_OC    0x1C
#define CISTPL_DEVICE_OA    0x1D
#define CISTPL_GEODEVICE    0x1E
#define CISTPL_GEODEVICE_A  0x1F
#define CISTPL_MANFID       0x20
#define CISTPL_FUNCID       0x21
#define CISTPL_FUNCE        0x22
#define CISTPL_VERS_2       0x40
#define CISTPL_FORMAT       0x41
#define CISTPL_GEOMETRY     0x42
#define CISTPL_BYTEORDER    0x43
#define CISTPL_DATE         0x44
#define CISTPL_BATTERY      0x45
#define CISTPL_ORG          0x46
#define CISTPL_END          0xFF


// Device Information Tuple Definitions

typedef union {
    BYTE dt_DevID;
    BYTE dt_ExtDevSpeed;
    BYTE dt_DevSize;
} DEVICE_TPL;

// Device ID Definitions

#define DEVID_DSPEED 0x07    // Device speed mask
#define DEVID_WPS    0x08    // Device write protect switch bit
#define DEVID_DTYPE  0xF0    // Device type code mask

// Device Speed Code Definitions

#define DSPEED_NULL  0x00
#define DSPEED_250NS 0x01
#define DSPEED_200NS 0x02
#define DSPEED_150NS 0x03
#define DSPEED_100NS 0x04
#define DSPEED_EXT   0x07

// Device Type Code Definitions

#define DTYPE_NULL     0x00
#define DTYPE_ROM      0x01
#define DTYPE_OTPROM   0x02
#define DTYPE_EPROM    0x03
#define DTYPE_EEPROM   0x04
#define DTYPE_FLASH    0x05
#define DTYPE_SRAM     0x06
#define DTYPE_DRAM     0x07
#define DTYPE_FUNCSPEC 0x0D
#define DTYPE_EXTEND   0x0E

// Extended Device Speed Definitions

#define ESPEED_EXP  0x07    // Extended device speed exponent mask
#define ESPEED_MANT 0x71    // Extended device speed mantissa mask
#define ESPEED_EXT  0x80    // Additional ext speed byte follows

// Extended Device Speed Code Exponent Definitions

#define SPDEXP_1NS   0x00
#define SPDEXP_10NS  0x01
#define SPDEXP_100NS 0x02
#define SPDEXP_1US   0x03
#define SPDEXP_10US  0x04
#define SPDEXP_100US 0x05
#define SPDEXP_1MS   0x06
#define SPDEXP_10MS  0x07

// Extended Device Speed Code Mantissa Definitions

#define SPDMANT_1_0 0x01
#define SPDMANT_1_2 0x02
#define SPDMANT_1_3 0x03
#define SPDMANT_1_5 0x04
#define SPDMANT_2_0 0x05
#define SPDMANT_2_5 0x06
#define SPDMANT_3_0 0x07
#define SPDMANT_3_5 0x08
#define SPDMANT_4_0 0x09
#define SPDMANT_4_5 0x0A
#define SPDMANT_5_0 0x0B
#define SPDMANT_5_5 0x0C
#define SPDMANT_6_0 0x0D
#define SPDMANT_7_0 0x0E
#define SPDMANT_8_0 0x0F

// Device Size Byte Definitions

#define DSIZE_CODE  0x07
#define DSIZE_UNITS 0xF1

// Device Size Code Definitions

#define SCODE_512  0x00
#define SCODE_2K   0x01
#define SCODE_8K   0x02
#define SCODE_32K  0x03
#define SCODE_128K 0x04
#define SCODE_512K 0x05
#define SCODE_2M   0x06


// Level 1 Version/Product Information Tuple Definitions

typedef struct {
    BYTE v1_Major;
    BYTE v1_Minor;
    BYTE v1_Info;
} L1VERS_TPL;

// Device Information String Terminator Definition

#define TPLL1V_END 0xFF


// Configuration Tuple Definitions

typedef union {
    struct {
    BYTE cc_Sz;
    BYTE cc_Last;
    };
    BYTE cc_RAdr;
    BYTE cc_RMsk;
} CONFIG_TPL;

// Configuration Size Field Byte Definitions

#define TPCC_RASZ 0x03
#define TPCC_RMSZ 0x3C
#define TPCC_RFSZ_MASK  0xC0
#define TPCCRFSZ(X)     ((X & TPCC_RFSZ_MASK) >> 6)
#define TPCC_RMSZ_MASK  0x3C
#define TPCCRMSZ(X)     ((X & TPCC_RMSZ_MASK) >> 2)
#define TPCC_RASZ_MASK  0x03
#define TPCCRASZ(X)     (X & TPCC_RASZ_MASK)

// Configuration Last Entry Index Byte Definitions

#define TPCC_LASTINDX 0x3F


// Configuration Table Entry Tuple Definitions

typedef union {
    BYTE ce_Indx;
    BYTE ce_IF;
    BYTE ce_FS;
    union {
    BYTE ce_PD_Selection;
    BYTE ce_PD_NomV;
    BYTE ce_PD_MinV;
    BYTE ce_PD_MaxV;
    BYTE ce_PD_StaticI;
    BYTE ce_PD_AvgI;
    BYTE ce_PD_PeakI;
    BYTE ce_PD_PDwnI;
    };
    BYTE ce_TD;
    union {
    BYTE ce_IO_Parm;
    BYTE ce_IO_RangeDesc;
    BYTE ce_IO_Address;
    BYTE ce_IO_Length;
    };
    union {
    BYTE ce_IR_Parm;
    WORD ce_IR_MASK;
    };
    BYTE ce_MS;
    BYTE ce_MI;
} CFENTRY_TPL;

// Configuration Table Index Byte Definitions

#define TPCE_ENTRYNUM 0x3F
#define TPCE_DEFAULT  0x40
#define TPCE_INTFACE  0x80

// Configuration Table Entry Interface Description Field Definitions

#define TPCE_IFTYPE       0x0F
#define TPCE_BVDSACTIVE   0x10
#define TPCE_WPACTIVE     0x20
#define TPCE_RDYBSYACTIVE 0x40
#define TPCE_MWAITREQD    0x80

// Configuration Table Entry Feature Selection Byte Definitions

#define TPCE_POWER    0x03
#define TPCE_TIMING   0x04
#define TPCE_IOSPACE  0x08
#define TPCE_IRQ      0x10
#define TPCE_MEMSPACE 0x60
#define TPCE_MISC     0x80

// Configuration Table Entry Power Desc Parameter Selection Byte Definitions

#define TPCE_PD_NOMV    0x01
#define TPCE_PD_MINV    0x02
#define TPCE_PD_MAXV    0x04
#define TPCE_PD_STATICI 0x08
#define TPCE_PD_AVGI    0x10
#define TPCE_PD_PEAKI   0x20
#define TPCE_PD_PDWNI   0x40

// Configuration Table Entry Power Description Structure Parameter Definitions

#define TPCE_PD_EXP   0x07
#define TPCE_PD_MANT  0x71
#define TPCE_PD_EXT   0x80

#define TPCE_PD_EXTEN 0x7F

// Configuration Table Entry Power Description Voltage Exponent Definitions

#define PDEXP_10UV    0x00
#define PDEXP_100UV   0x01
#define PDEXP_1MV     0x02
#define PDEXP_10MV    0x03
#define PDEXP_100MV   0x04
#define PDEXP_1V      0x05
#define PDEXP_10V     0x06
#define PDEXP_100V    0x07

// Configuration Table Entry Power Description Current Exponent Definitions

#define PDEXP_100NA   0x00
#define PDEXP_1UA     0x01
#define PDEXP_10UA    0x02
#define PDEXP_100UA   0x03
#define PDEXP_1MA     0x04
#define PDEXP_10MA    0x05
#define PDEXP_100MA   0x06
#define PDEXP_1A      0x07

// Configuration Table Entry Power Description Mantissa Definitions

#define PDMANT_1_0    0x00
#define PDMANT_1_2    0x01
#define PDMANT_1_3    0x02
#define PDMANT_1_5    0x03
#define PDMANT_2_0    0x04
#define PDMANT_2_5    0x05
#define PDMANT_3_0    0x06
#define PDMANT_3_5    0x07
#define PDMANT_4_0    0x08
#define PDMANT_4_5    0x09
#define PDMANT_5_0    0x0A
#define PDMANT_5_5    0x0B
#define PDMANT_6_0    0x0C
#define PDMANT_7_0    0x0D
#define PDMANT_8_0    0x0E
#define PDMANT_9_0    0x0F

// Configuration Table Entry Timing Information Definitions

#define TPCE_TD_WAIT   0x03
#define TPCE_TD_RDYBSY 0x1C

// Configuration Table Entry I/O Space Address Parameter Definitions

#define TPCE_IO_ADDRLINES 0x1F
#define TPCE_IO_BUS8      0x20
#define TPCE_IO_BUS16     0x40
#define TPCE_IO_RANGE     0x80

// Configuration Table Entry I/O Space Range Descriptor Definitions

#define TPCE_IO_NUMRANGES  0x0F
#define TPCE_IO_ADDRSIZE   0x30
#define TPCE_IO_LENGTHSIZE 0xC0

// Configuration Table Entry IRQ Interrupt Request Descriptor Definitions

#define TPCE_IR_IRQN  0x0F
#define TPCE_IR_NMI   0x01
#define TPCE_IR_LOCK  0x02
#define TPCE_IR_BERR  0x04
#define TPCE_IR_VEND  0x08
#define TPCE_IR_MASK  0x10
#define TPCE_IR_LEVEL 0x20
#define TPCE_IR_PULSE 0x40
#define TPCE_IR_SHARE 0x80



//******************************************************

typedef struct tTPCE_IF
{
    BYTE InterfaceType;
    BYTE BVDsActive;
    BYTE WPActive;
    BYTE RdyBsyActive;
    BYTE MWaitActive;
} tTPCE_IF;

typedef struct tTPCE_FS
{
    BYTE Power;
    BYTE Timing;
    BYTE IOSpace;
    BYTE IRQ;
    BYTE MemSpace;
    BYTE Misc;
} tTPCE_FS;

typedef struct tTPCE_PD
{
    BYTE   NomVPresent;
    BYTE   MinVPresent;
    BYTE   MaxVPresent;
    BYTE   StaticIPresent;
    BYTE   AvgIPresent;
    BYTE   PeakIPresent;
    BYTE   PDwnIPresent;
    BYTE   RFUPresent;
    double NomV;
    double MinV;
    double MaxV;
    double StaticI;
    double AvgI;
    double PeakI;
    double PDwnI;
    double RFU;
} tTPCE_PD;

typedef struct tTPCE_TD
{
    BYTE   WaitScalePresent;
    BYTE   ReadyBusyScalePresent;
    BYTE   ReservedScale7Present;
    double WaitScale;
    double ReadyBusyScale;
    double ReservedScale7;
} tTPCE_TD;

typedef struct tTPCE_IO
{
    BYTE IOAddrLines;
    BYTE Bus8;
    BYTE Bus16;
    BYTE Range;
    BYTE NumberIOAddrRanges;
    BYTE AddressSize;
    BYTE LengthSize;
} tTPCE_IO;

typedef struct tTPCE_IR
{
    BYTE Irqn;
    BYTE Vend;
    BYTE Berr;
    BYTE Iock;
    BYTE Nmi;
    BYTE Mask0;
    BYTE Level;
    BYTE Pulse;
    BYTE Share;
    BYTE Irq0;
    BYTE Irq1;
    BYTE Irq2;
    BYTE Irq3;
    BYTE Irq4;
    BYTE Irq5;
    BYTE Irq6;
    BYTE Irq7;
    BYTE Irq8;
    BYTE Irq9;
    BYTE Irq10;
    BYTE Irq11;
    BYTE Irq12;
    BYTE Irq13;
    BYTE Irq14;
    BYTE Irq15;
} tTPCE_IR;

typedef struct tTPCE_MS
{
    BYTE NumWindows;
    BYTE LengthSize;
    BYTE CardAddrSze;
    BYTE HostAddr;
} tTPCE_MS;

typedef struct tTPCE_MI
{
    BYTE MaxTwinCards;
    BYTE Audio;
    BYTE ReadOnly;
    BYTE PwrDown;
    BYTE Reserved;
    BYTE Ext;
} tTPCE_MI;

typedef struct tCardEntryTuple
{
    BYTE     ConfigEntryNumber;
    BYTE     DefaultBit;
    BYTE     InterfaceBytePresent;
    tTPCE_IF IF;
    tTPCE_FS FS;
    tTPCE_PD PDVcc;
    tTPCE_PD PDVpp1;
    tTPCE_PD PDVpp2;
    tTPCE_TD TD;
    tTPCE_IO IO;
    tTPCE_IR IR;
    tTPCE_MS MS;
    tTPCE_MI MI;
} tCardEntryTuple;

typedef struct tDGTPL
{
    BYTE DGTPL_BUS;
    BYTE DGTPL_EBS;
    BYTE DGTPL_RBS;
    BYTE DGTPL_WBS;
    BYTE DGTPL_PART;
    BYTE DGTPL_HWIL;
} tDGTPL;

//
// CISTPL_FUNCID function codes
//
#define PCCARD_TYPE_VENDOR_SPECIFIC 0
#define PCCARD_TYPE_MEMORY        1
#define PCCARD_TYPE_SERIAL        2
#define PCCARD_TYPE_PARALLEL      3
#define PCCARD_TYPE_FIXED_DISK    4
#define PCCARD_TYPE_VIDEO         5
#define PCCARD_TYPE_NETWORK       6
#define PCCARD_TYPE_AIMS          7
#define PCCARD_TYPE_UNKNOWN       0xff


//
// Parsed structures returned by CardGetParsedTuple
//

//
// @struct PARSED_CONFIG | Structure returned by CardGetParsedTuple for a parsed
//                         CISTPL_CONFIG tuple
//
// Note:
//  1) only one byte of the register presence mask is used,
//  2) custom interface subtuples are not parsed
//
typedef struct _PARSED_CONFIG {
    UINT32  ConfigBase;         // @field Base address of the card configuration registers.
    UINT8   RegMask;            // @field Config register presence mask.
    UINT8   LastConfigIndex;    // @field Index of last entry in the card configuration table.
} PARSED_CONFIG, * PPARSED_CONFIG;


//
// Parsed CISTPL_CFTABLE_ENTRY tuples.
//
// All CISTPL_CFTABLE_ENTRY tuples for a given socket/function are parsed
// and the results are returned using the following three structures.
//
// Note: the custom interface subtuples are not parsed
//

//
//  If the POWER_DESCR structure is changed, then PARSED_CFTABLE must be realigned
//
// @struct POWER_DESCR | Structure used for the power description fields
// VccDescr, Vpp1Descr and Vpp2Descr in a PARSED_CFTABLE structure.
//
// Note:
//  1) The voltages are in 0.1 volt units,
//  2) The currents are in milliAmps,
//
typedef struct _POWER_DESCR {
    UINT16   ValidMask;      // @field Describes which of the following fields are valid
                            // (bit 0 set implies NominalV is valid,
                            // bit 1 set implies MinV is valid)
                            // etc
    UINT16   NominalV;       // @field Nominal operating supply voltage
    UINT16   MinV;           // @field Minimum operating supply voltage
    UINT16   MaxV;           // @field Maximum operating supply voltage
    UINT16   StaticI;        // @field Continuous supply current required
    UINT16   AvgI;           // @field Max current required averaged over 1 second
    UINT16   PeakI;          // @field Max current required averaged over 10 milliseconds
    UINT16   PowerDownI;     // @field Power-down supply current required
} POWER_DESCR, * PPOWER_DESCR;

//
// POWER_DESCR.ValidMask
// 
#define PWR_DESCR_NOMINALV  0x01
#define PWR_DESCR_MINV      0x02
#define PWR_DESCR_MAXV      0x04
#define PWR_DESCR_STATICI   0x08
#define PWR_DESCR_AVGI      0x10
#define PWR_DESCR_PEAKI     0x20
#define PWR_DESCR_POWERDWNI 0x40

#define MAX_IO_RANGES   4   // limit this to 4, it can be as high as 16

//
// @struct PARSED_CFTABLE | Structure used by CardGetParsedTuple for parsed
// CISTPL_CFTABLE_ENTRY tuples.
//
typedef struct _PARSED_CFTABLE {
    POWER_DESCR VccDescr;   // @field Power description of Vcc
    POWER_DESCR Vpp1Descr;  // @field Power description of Vpp1
    POWER_DESCR Vpp2Descr;  // @field Power description of Vpp2

    UINT32  IOLength[MAX_IO_RANGES];    // @field IOLength[i] is the number of bytes in 
    UINT32  IOBase[MAX_IO_RANGES];      // @field the I/O address range starting at IOBase[i]
                                        // There are NumIOEntries in each array.
    UINT8   NumIOEntries;   // @field Number of I/O address range entries in the two arrays:
                            // IOLength[] and IOBase[]

    UINT8   ConfigIndex;    // @field Value to be written to the configuration option
                            // register to enable this CFTABLE_ENTRY.
    UINT8   ContainsDefaults;   // @field True if this entry contains defaults to be used by subsequent entries.

    UINT8   IFacePresent;   // @field True if the following 5 fields are valid:
    UINT8   IFaceType;      // @field 0 = memory, 1 = I/O, 2,3,8-15 = rsvd, 4-7 = custom
    UINT8   BVDActive;      // @field True if BVD1 and BVD2 are active in the Pin Replacement Register (PRR)
    UINT8   WPActive;       // @field True if the card write protect is active in the PRR
    UINT8   ReadyActive;    // @field True if READY is active in the PRR
    UINT8   WaitRequired;   // @field True if the WAIT signal is required for memory cycles

    UINT8   IOAccess;       // @field 1 = 8 bit only, 2 = 8 bit access to 16 bit registers is not supported,
                            // 3 = 8 bit access to 16 bit registers is supported

    UINT8   NumIOAddrLines; // @field 0 = socket controller provides address decode
                            // 1-26 = number of address lines decoded
    UINT8   Pad0;
} PARSED_CFTABLE, * PPARSED_CFTABLE;

#pragma pack()

#endif
