/*++ 

Copyright (c) 1990-2000 Microsoft Corporation.  All rights reserved.

Module Name:

    wdm.h

Abstract:

    This module defines the WDM types, constants, and functions that are
    exposed to device drivers.

Revision History:

--*/

#ifndef _WDMDDK_
#define _WDMDDK_
#define _NTDDK_

#define NT_INCLUDED
#define _CTYPE_DISABLE_MACROS

#ifdef __cplusplus
extern "C" {
#endif

//#include <ntstatus.h>
#include <memory.h>
#include <ceddk.h>


typedef UCHAR KIRQL;
typedef CONST char *PCSZ;

typedef char CCHAR, *PCCHAR;
typedef short CSHORT, *PCSHORT;
typedef ULONG CLONG, *PCLONG;
typedef KIRQL *PKIRQL;

#ifndef FASTCALL
#define FASTCALL
#endif
#ifndef PAGED_CODE
#define PAGED_CODE()
#endif

#ifndef MAXULONG
#define MAXULONG 0xffffffff  // From ntdef.h
#endif

//
// MessageId: STATUS_BUFFER_OVERFLOW
//
// MessageText:
//
//  {Buffer Overflow}
//  The data was too large to fit into the specified buffer.
//
#define STATUS_BUFFER_OVERFLOW           ((NTSTATUS)0x80000005L)

//
// MessageId: STATUS_NOT_IMPLEMENTED
//
// MessageText:
//
//  {Not Implemented}
//  The requested operation is not implemented.
//
#define STATUS_NOT_IMPLEMENTED           ((NTSTATUS)0xC0000002L)

//
// MessageId: STATUS_NO_SUCH_DEVICE
//
// MessageText:
//
//  A device which does not exist was specified.
//
#define STATUS_NO_SUCH_DEVICE            ((NTSTATUS)0xC000000EL)

//
// MessageId: STATUS_NO_SUCH_FILE
//
// MessageText:
//
//  {File Not Found}
//  The file %s does not exist.
//
#define STATUS_NO_SUCH_FILE              ((NTSTATUS)0xC000000FL)

//
// MessageId: STATUS_INVALID_DEVICE_REQUEST
//
// MessageText:
//
//  The specified request is not a valid operation for the target device.
//
#define STATUS_INVALID_DEVICE_REQUEST    ((NTSTATUS)0xC0000010L)

//
// MessageId: STATUS_NO_MEDIA_IN_DEVICE
//
// MessageText:
//
//  {No Disk}
//  There is no disk in the drive.
//  Please insert a disk into drive %s.
//
#define STATUS_NO_MEDIA_IN_DEVICE        ((NTSTATUS)0xC0000013L)

//
// MessageId: STATUS_MORE_PROCESSING_REQUIRED
//
// MessageText:
//
//  {Still Busy}
//  The specified I/O request packet (IRP) cannot be disposed of because the
//  I/O operation is not complete.
//
#define STATUS_MORE_PROCESSING_REQUIRED  ((NTSTATUS)0xC0000016L)

//
// MessageId: STATUS_INSUFFICIENT_RESOURCES
//
// MessageText:
//
//  Insufficient system resources exist to complete the API.
//
#define STATUS_INSUFFICIENT_RESOURCES    ((NTSTATUS)0xC000009AL)     // ntsubauth

//
// MessageId: STATUS_MEDIA_CHANGED
//
// MessageText:
//
//  {Media Changed}
//  The media may have changed.
//
#define STATUS_MEDIA_CHANGED             ((NTSTATUS)0x8000001CL)

//
// MessageId: STATUS_BUFFER_TOO_SMALL
//
// MessageText:
//
//  {Buffer Too Small}
//  The buffer is too small to contain the entry.  No information has been
//  written to the buffer.
//
#define STATUS_BUFFER_TOO_SMALL          ((NTSTATUS)0xC0000023L)

//
// MessageId: STATUS_DEVICE_NOT_READY
//
// MessageText:
//
//  {Drive Not Ready}
//  The drive is not ready for use; its door may be open.
//  Please check drive %s and make sure that a disk is inserted
//  and that the drive door is closed.
//
#define STATUS_DEVICE_NOT_READY          ((NTSTATUS)0xC00000A3L)

//
// MessageId: STATUS_ILLEGAL_FUNCTION
//
// MessageText:
//
//  The specified handle is not open to the server end of the named pipe.
//
#define STATUS_ILLEGAL_FUNCTION          ((NTSTATUS)0xC00000AFL)

//
// MessageId: STATUS_DEVICE_DOES_NOT_EXIST
//
// MessageText:
//
//  This device does not exist.
//
#define STATUS_DEVICE_DOES_NOT_EXIST     ((NTSTATUS)0xC00000C0L)

//
// MessageId: STATUS_MORE_ENTRIES
//
// MessageText:
//
//  Returned by enumeration APIs to indicate more information is available
//  to successive calls.
//
#define STATUS_MORE_ENTRIES              ((NTSTATUS)0x00000105L)

//
// MessageId: STATUS_DEVICE_CONFIGURATION_ERROR
//
// MessageText:
//
//  The I/O device is configured incorrectly or the configuration parameters to the
//  driver are incorrect.
//
#define STATUS_DEVICE_CONFIGURATION_ERROR ((NTSTATUS)0xC0000182L)

//
// MessageId: STATUS_IO_DEVICE_ERROR
//
// MessageText:
//
//  The I/O device reported an I/O error.
//
#define STATUS_IO_DEVICE_ERROR           ((NTSTATUS)0xC0000185L)


//
// Determine if an argument is present by testing the value of the pointer
// to the argument value.
//
#ifndef ARGUMENT_PRESENT
#define ARGUMENT_PRESENT(ArgumentPointer)    (\
    (CHAR *)(ArgumentPointer) != (CHAR *)(NULL) )
#endif

//
// Processor modes.
//

typedef CCHAR KPROCESSOR_MODE;

typedef enum _MODE {
    KernelMode,
    UserMode,
    MaximumMode
} MODE;

typedef enum _LOCK_OPERATION {
    IoReadAccess,
    IoWriteAccess,
    IoModifyAccess
} LOCK_OPERATION;

typedef struct _DEVICE_DESCRIPTION  DEVICE_DESCRIPTION, *PDEVICE_DESCRIPTION;
typedef struct _DEVICE_OBJECT       DEVICE_OBJECT,      *PDEVICE_OBJECT;
typedef struct _DMA_ADAPTER         DMA_ADAPTER,        *PDMA_ADAPTER;
typedef struct _IO_TIMER                                *PIO_TIMER;
typedef struct _IRP                 IRP,                *PIRP;

typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    LPWSTR Buffer;
} UNICODE_STRING, *PUNICODE_STRING;


//
// I/O system definitions.
//
// Define a Memory Descriptor List (MDL)
//
// An MDL describes pages in a virtual buffer in terms of physical pages.  The
// pages associated with the buffer are described in an array that is allocated
// just after the MDL header structure itself.  In a future compiler this will
// be placed at:
//
//      ULONG Pages[];
//
// Until this declaration is permitted, however, one simply calculates the
// base of the array by adding one to the base MDL pointer:
//
//      Pages = (PULONG) (Mdl + 1);
//
// Notice that while in the context of the subject thread, the base virtual
// address of a buffer mapped by an MDL may be referenced using the following:
//
//      Mdl->StartVa | Mdl->ByteOffset
//
// NOTE: If MDL_64_BIT_VA flag is set, the StartVa field contains
//       the Virtual Page Number (VirtualAddress / PAGE_SIZE).
//       HENCE, the reference to the base virtual address becomes:
//
//        (PVOID64)(((LONG LONG)Mdl->StartVa << PAGE_SHIFT) | Mdl->ByteOffset)
//

typedef struct _MDL {
    struct _MDL *Next;
//    CSHORT Size;
    SHORT MdlFlags;
//    struct _EPROCESS *Process;
    PVOID MappedSystemVa;
    PVOID StartVa;
    ULONG ByteCount;
    ULONG ByteOffset;
} MDL, *PMDL;

#define MDL_MAPPED_TO_SYSTEM_VA     0x0001
#define MDL_PAGES_LOCKED            0x0002
#define MDL_SOURCE_IS_NONPAGED_POOL 0x0004
#define MDL_ALLOCATED_FIXED_SIZE    0x0008
#define MDL_PARTIAL                 0x0010
#define MDL_PARTIAL_HAS_BEEN_MAPPED 0x0020
#define MDL_IO_PAGE_READ            0x0040
#define MDL_WRITE_OPERATION         0x0080
#define MDL_PARENT_MAPPED_SYSTEM_VA 0x0100
#define MDL_LOCK_HELD               0x0200
#define MDL_SCATTER_GATHER_VA       0x0400
#define MDL_IO_SPACE                0x0800
#define MDL_NETWORK_HEADER          0x1000
#define MDL_MAPPING_CAN_FAIL        0x2000
#define MDL_ALLOCATED_MUST_SUCCEED  0x4000
#define MDL_64_BIT_VA               0x8000


#define MDL_MAPPING_FLAGS (MDL_MAPPED_TO_SYSTEM_VA     | \
                           MDL_PAGES_LOCKED            | \
                           MDL_SOURCE_IS_NONPAGED_POOL | \
                           MDL_PARTIAL_HAS_BEEN_MAPPED | \
                           MDL_PARENT_MAPPED_SYSTEM_VA | \
                           MDL_LOCK_HELD               | \
                           MDL_SYSTEM_VA               | \
                           MDL_IO_SPACE )

//
// Define the various device type values.  Note that values used by Microsoft
// Corporation are in the range 0-32767, and 32768-65535 are reserved for use
// by customers.
//

#define DEVICE_TYPE ULONG

#define FILE_DEVICE_BEEP                0x00000001
#define FILE_DEVICE_CD_ROM              0x00000002
#define FILE_DEVICE_CD_ROM_FILE_SYSTEM  0x00000003
#define FILE_DEVICE_CONTROLLER          0x00000004
#define FILE_DEVICE_DATALINK            0x00000005
#define FILE_DEVICE_DFS                 0x00000006
#define FILE_DEVICE_DISK                0x00000007
#define FILE_DEVICE_DISK_FILE_SYSTEM    0x00000008
#define FILE_DEVICE_FILE_SYSTEM         0x00000009
#define FILE_DEVICE_INPORT_PORT         0x0000000a
#define FILE_DEVICE_KEYBOARD            0x0000000b
#define FILE_DEVICE_MAILSLOT            0x0000000c
#define FILE_DEVICE_MIDI_IN             0x0000000d
#define FILE_DEVICE_MIDI_OUT            0x0000000e
#define FILE_DEVICE_MOUSE               0x0000000f
#define FILE_DEVICE_MULTI_UNC_PROVIDER  0x00000010
#define FILE_DEVICE_NAMED_PIPE          0x00000011
#define FILE_DEVICE_NETWORK             0x00000012
#define FILE_DEVICE_NETWORK_BROWSER     0x00000013
#define FILE_DEVICE_NETWORK_FILE_SYSTEM 0x00000014
#define FILE_DEVICE_NULL                0x00000015
#define FILE_DEVICE_PARALLEL_PORT       0x00000016
#define FILE_DEVICE_PHYSICAL_NETCARD    0x00000017
#define FILE_DEVICE_PRINTER             0x00000018
#define FILE_DEVICE_SCANNER             0x00000019
#define FILE_DEVICE_SERIAL_MOUSE_PORT   0x0000001a
#define FILE_DEVICE_SERIAL_PORT         0x0000001b
#define FILE_DEVICE_SCREEN              0x0000001c
#define FILE_DEVICE_SOUND               0x0000001d
#define FILE_DEVICE_STREAMS             0x0000001e
#define FILE_DEVICE_TAPE                0x0000001f
#define FILE_DEVICE_TAPE_FILE_SYSTEM    0x00000020
#define FILE_DEVICE_TRANSPORT           0x00000021
#define FILE_DEVICE_UNKNOWN             0x00000022
#define FILE_DEVICE_VIDEO               0x00000023
#define FILE_DEVICE_VIRTUAL_DISK        0x00000024
#define FILE_DEVICE_WAVE_IN             0x00000025
#define FILE_DEVICE_WAVE_OUT            0x00000026
#define FILE_DEVICE_8042_PORT           0x00000027
#define FILE_DEVICE_NETWORK_REDIRECTOR  0x00000028
#define FILE_DEVICE_BATTERY             0x00000029
#define FILE_DEVICE_BUS_EXTENDER        0x0000002a
#define FILE_DEVICE_MODEM               0x0000002b
#define FILE_DEVICE_VDM                 0x0000002c
#define FILE_DEVICE_MASS_STORAGE        0x0000002d
#define FILE_DEVICE_SMB                 0x0000002e
#define FILE_DEVICE_KS                  0x0000002f
#define FILE_DEVICE_CHANGER             0x00000030
#define FILE_DEVICE_SMARTCARD           0x00000031

#define FILE_AUTOGENERATED_DEVICE_NAME  0x00000100

//
// define IO_NO_INCREMENT
//

#define IO_NO_INCREMENT                 0

//
// Define File Object (FO) flags
//

#define FO_FILE_OPEN                    0x00000001
#define FO_SYNCHRONOUS_IO               0x00000002
//#define FO_ALERTABLE_IO                 0x00000004
//#define FO_NO_INTERMEDIATE_BUFFERING    0x00000008
//#define FO_WRITE_THROUGH                0x00000010
//#define FO_SEQUENTIAL_ONLY              0x00000020
//#define FO_CACHE_SUPPORTED              0x00000040
//#define FO_NAMED_PIPE                   0x00000080
//#define FO_STREAM_FILE                  0x00000100
//#define FO_MAILSLOT                     0x00000200
//#define FO_GENERATE_AUDIT_ON_CLOSE      0x00000400
//#define FO_DIRECT_DEVICE_OPEN           0x00000800
//#define FO_FILE_MODIFIED                0x00001000
//#define FO_FILE_SIZE_CHANGED            0x00002000
#define FO_CLEANUP_COMPLETE             0x00004000
//#define FO_TEMPORARY_FILE               0x00008000
//#define FO_DELETE_ON_CLOSE              0x00010000
//#define FO_OPENED_CASE_SENSITIVE        0x00020000
#define FO_HANDLE_CREATED               0x00040000
#define FO_FILE_FAST_IO_READ            0x00080000

typedef struct _FILE_OBJECT {
//    CSHORT Type;
//    CSHORT Size;
    PDEVICE_OBJECT DeviceObject;
//    PVOID DoNotUse1;
    PVOID FsContext;
    PVOID FsContext2;
//    PSECTION_OBJECT_POINTERS SectionObjectPointer;
//    PVOID PrivateCacheMap;
    NTSTATUS FinalStatus;
    struct _FILE_OBJECT *RelatedFileObject;
//    BOOLEAN LockOperation;
//    BOOLEAN DeletePending;
    BOOLEAN ReadAccess;
    BOOLEAN WriteAccess;
//    BOOLEAN DeleteAccess;
    BOOLEAN SharedRead;
    BOOLEAN SharedWrite;
//    BOOLEAN SharedDelete;
    ULONG Flags;
    HANDLE hCloseEvent;
//    UNICODE_STRING FileName;
    LARGE_INTEGER CurrentByteOffset;
//    ULONG Waiters;
    ULONG Busy;
//    PVOID LastLock;
    ULONG Lock;
    HANDLE Event;
    HANDLE hevtSynchronous;
//    PIO_COMPLETION_CONTEXT CompletionContext;
} FILE_OBJECT, *PFILE_OBJECT;

//
// Define the base asynchronous I/O argument types
//

typedef struct _IO_STATUS_BLOCK {
    NTSTATUS Status;
    ULONG Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

typedef
VOID
(*PIO_APC_ROUTINE) (
    IN PVOID ApcContext,
    IN PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG Reserved
    );

//
// DPC routine
//

struct _KDPC;

typedef
VOID
(*PKDEFERRED_ROUTINE) (
    IN struct _KDPC *Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

//
// Define driver initialization routine type.
//

typedef
NTSTATUS
(*PDRIVER_INITIALIZE) (
    IN struct _DRIVER_OBJECT *DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

//
// Define driver cancel routine type.
//

typedef
VOID
(*PDRIVER_CANCEL) (
    IN struct _DEVICE_OBJECT *DeviceObject,
    IN struct _IRP *Irp
    );

//
// Define driver dispatch routine type.
//

typedef
NTSTATUS
(*PDRIVER_DISPATCH) (
    IN struct _DEVICE_OBJECT *DeviceObject,
    IN struct _IRP *Irp
    );

//
// Define driver start I/O routine type.
//

typedef
VOID
(*PDRIVER_STARTIO) (
    IN struct _DEVICE_OBJECT *DeviceObject,
    IN struct _IRP *Irp
    );

//
// Define driver unload routine type.
//

typedef
VOID
(*PDRIVER_UNLOAD) (
    IN struct _DRIVER_OBJECT *DriverObject
    );

//
// Define driver AddDevice routine type.
//

typedef
NTSTATUS
(*PDRIVER_ADD_DEVICE) (
    IN struct _DRIVER_OBJECT *DriverObject,
    IN struct _DEVICE_OBJECT *PhysicalDeviceObject
    );

//
// Define fast I/O procedure prototypes.
//
// Fast I/O read and write procedures.
//

typedef
BOOLEAN
(*PFAST_IO_CHECK_IF_POSSIBLE) (
    IN struct _FILE_OBJECT *FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN BOOLEAN CheckForReadOperation,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN struct _DEVICE_OBJECT *DeviceObject
    );

typedef
BOOLEAN
(*PFAST_IO_READ) (
    IN struct _FILE_OBJECT *FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    OUT PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN struct _DEVICE_OBJECT *DeviceObject
    );

typedef
BOOLEAN
(*PFAST_IO_WRITE) (
    IN struct _FILE_OBJECT *FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN struct _DEVICE_OBJECT *DeviceObject
    );

//
// Fast I/O device control procedure.
//

typedef
BOOLEAN
(*PFAST_IO_DEVICE_CONTROL) (
    IN struct _FILE_OBJECT *FileObject,
    IN BOOLEAN Wait,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN ULONG IoControlCode,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN struct _DEVICE_OBJECT *DeviceObject
    );

//
// Define callback for drivers that have device objects attached to lower-
// level drivers' device objects.  This callback is made when the lower-level
// driver is deleting its device object.
//

typedef
VOID
(*PFAST_IO_DETACH_DEVICE) (
    IN struct _DEVICE_OBJECT *SourceDevice,
    IN struct _DEVICE_OBJECT *TargetDevice
    );

//
//  Define Mdl-based routines for the server to call
//

typedef
BOOLEAN
(*PFAST_IO_MDL_READ) (
    IN struct _FILE_OBJECT *FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG LockKey,
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN struct _DEVICE_OBJECT *DeviceObject
    );

typedef
BOOLEAN
(*PFAST_IO_MDL_READ_COMPLETE) (
    IN struct _FILE_OBJECT *FileObject,
    IN PMDL MdlChain,
    IN struct _DEVICE_OBJECT *DeviceObject
    );

typedef
BOOLEAN
(*PFAST_IO_PREPARE_MDL_WRITE) (
    IN struct _FILE_OBJECT *FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG LockKey,
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN struct _DEVICE_OBJECT *DeviceObject
    );

typedef
BOOLEAN
(*PFAST_IO_MDL_WRITE_COMPLETE) (
    IN struct _FILE_OBJECT *FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PMDL MdlChain,
    IN struct _DEVICE_OBJECT *DeviceObject
    );

typedef
VOID
(*PPNP_CALLBACK) (
    IN PVOID Context
    );


//
// Define the structure to describe the Fast I/O dispatch routines.  Any
// additions made to this structure MUST be added monotonically to the end
// of the structure, and fields CANNOT be removed from the middle.
//

typedef struct _FAST_IO_DISPATCH {
    ULONG SizeOfFastIoDispatch;
    PFAST_IO_CHECK_IF_POSSIBLE FastIoCheckIfPossible;
    PFAST_IO_READ FastIoRead;
    PFAST_IO_WRITE FastIoWrite;
//    PFAST_IO_QUERY_BASIC_INFO FastIoQueryBasicInfo;
//    PFAST_IO_QUERY_STANDARD_INFO FastIoQueryStandardInfo;
//    PFAST_IO_LOCK FastIoLock;
//    PFAST_IO_UNLOCK_SINGLE FastIoUnlockSingle;
//    PFAST_IO_UNLOCK_ALL FastIoUnlockAll;
//    PFAST_IO_UNLOCK_ALL_BY_KEY FastIoUnlockAllByKey;
    PFAST_IO_DEVICE_CONTROL FastIoDeviceControl;
//    PFAST_IO_ACQUIRE_FILE AcquireFileForNtCreateSection;
//    PFAST_IO_RELEASE_FILE ReleaseFileForNtCreateSection;
    PFAST_IO_DETACH_DEVICE FastIoDetachDevice;
//    PFAST_IO_QUERY_NETWORK_OPEN_INFO FastIoQueryNetworkOpenInfo;
//    PFAST_IO_ACQUIRE_FOR_MOD_WRITE AcquireForModWrite;
    PFAST_IO_MDL_READ MdlRead;
    PFAST_IO_MDL_READ_COMPLETE MdlReadComplete;
    PFAST_IO_PREPARE_MDL_WRITE PrepareMdlWrite;
    PFAST_IO_MDL_WRITE_COMPLETE MdlWriteComplete;
//    PFAST_IO_READ_COMPRESSED FastIoReadCompressed;
//    PFAST_IO_WRITE_COMPRESSED FastIoWriteCompressed;
//    PFAST_IO_MDL_READ_COMPLETE_COMPRESSED MdlReadCompleteCompressed;
//    PFAST_IO_MDL_WRITE_COMPLETE_COMPRESSED MdlWriteCompleteCompressed;
//    PFAST_IO_QUERY_OPEN FastIoQueryOpen;
//    PFAST_IO_RELEASE_FOR_MOD_WRITE ReleaseForModWrite;
//    PFAST_IO_ACQUIRE_FOR_CCFLUSH AcquireForCcFlush;
//    PFAST_IO_RELEASE_FOR_CCFLUSH ReleaseForCcFlush;
} FAST_IO_DISPATCH, *PFAST_IO_DISPATCH;

//
// Define DPC importance.
//
// LowImportance - Queue DPC at end of target DPC queue.
// MediumImportance - Queue DPC at front of target DPC queue.
// HighImportance - Queue DPC at front of target DPC DPC queue and interrupt
//     the target processor if the DPC is targeted and the system is an MP
//     system.
//
// N.B. If the target processor is the same as the processor on which the DPC
//      is queued on, then the processor is always interrupted if the DPC queue
//      was previously empty.
//

typedef enum _KDPC_IMPORTANCE {
    LowImportance,
    MediumImportance,
    HighImportance
} KDPC_IMPORTANCE;

//
// Deferred Procedure Call (DPC) object
//

typedef struct _KDPC {
//    CSHORT Type;
    UCHAR Number;
    UCHAR Importance;
    LIST_ENTRY DpcListEntry;
    PKDEFERRED_ROUTINE DeferredRoutine;
    PVOID DeferredContext;
    PVOID SystemArgument1;
    PVOID SystemArgument2;
    PULONG Lock;
} KDPC, *PKDPC;


// 
// string definition
//

typedef struct _STRING {
          USHORT        Length;
          USHORT        MaximumLength;
          PCHAR         Buffer;
        } STRING, *PSTRING;

typedef STRING ANSI_STRING, *PANSI_STRING;


//
// Device Queue object and entry
//

typedef struct _KDEVICE_QUEUE {
//    CSHORT Type;
//    CSHORT Size;
    LIST_ENTRY DeviceListHead;
    CRITICAL_SECTION Lock;
    BOOLEAN Busy;
} KDEVICE_QUEUE, *PKDEVICE_QUEUE;

typedef struct _KDEVICE_QUEUE_ENTRY {
    LIST_ENTRY DeviceListEntry;
    ULONG SortKey;
    BOOLEAN Inserted;
} KDEVICE_QUEUE_ENTRY, *PKDEVICE_QUEUE_ENTRY;


//
// Define the actions that a driver execution routine may request of the
// adapter/controller allocation routines upon return.
//

typedef enum _IO_ALLOCATION_ACTION {
    KeepObject = 1,
    DeallocateObject,
    DeallocateObjectKeepRegisters
} IO_ALLOCATION_ACTION, *PIO_ALLOCATION_ACTION;

//
// Define device driver adapter/controller execution routine.
//

typedef
IO_ALLOCATION_ACTION
(*PDRIVER_CONTROL) (
    IN struct _DEVICE_OBJECT *DeviceObject,
    IN struct _IRP *Irp,
    IN PVOID MapRegisterBase,
    IN PVOID Context
    );

//
// Define completion routine types for use in stack locations in an IRP
//

typedef
NTSTATUS
(*PIO_COMPLETION_ROUTINE) (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

#ifndef _PO_DDK_
#define _PO_DDK_

typedef enum _SYSTEM_POWER_STATE {
    PowerSystemUnspecified = 0,
    PowerSystemWorking,
    PowerSystemSleeping1,
    PowerSystemSleeping2,
    PowerSystemSleeping3,
    PowerSystemHibernate,
    PowerSystemShutdown,
    PowerSystemMaximum
} SYSTEM_POWER_STATE, *PSYSTEM_POWER_STATE;

typedef enum _DEVICE_POWER_STATE {
    PowerDeviceUnspecified = 0,
    PowerDeviceD0,
    PowerDeviceD1,
    PowerDeviceD2,
    PowerDeviceD3,
    PowerDeviceMaximum
} DEVICE_POWER_STATE, *PDEVICE_POWER_STATE;

typedef union _POWER_STATE {
    SYSTEM_POWER_STATE SystemState;
    DEVICE_POWER_STATE DeviceState;
} POWER_STATE, *PPOWER_STATE;

typedef enum _POWER_STATE_TYPE {
    SystemPowerState,
    DevicePowerState
} POWER_STATE_TYPE, *PPOWER_STATE_TYPE;
#endif

//
// Define I/O Request Packet (IRP) stack locations
//

typedef struct _IO_STACK_LOCATION {
    UCHAR MajorFunction;
    UCHAR MinorFunction;
    UCHAR Flags;
    UCHAR Control;

    //
    // The following user parameters are based on the service that is being
    // invoked.  Drivers and file systems can determine which set to use based
    // on the above major and minor function codes.
    //

    union {

        //
        // System service parameters for:  NtCreateFile
        //

        struct {
            ULONG Options;
            USHORT FileAttributes;
            USHORT ShareAccess;
        } Create;


        //
        // System service parameters for:  NtReadFile
        //

        struct {
            ULONG Length;
            ULONG Key;
            LARGE_INTEGER ByteOffset;
        } Read;

        //
        // System service parameters for:  NtWriteFile
        //

        struct {
            ULONG Length;
            ULONG Key;
            LARGE_INTEGER ByteOffset;
        } Write;


        //
        // System service parameters for:  NtFlushBuffersFile
        //
        // No extra user-supplied parameters.
        //


        //
        // System service parameters for:  NtDeviceIoControlFile
        //
        // Note that the user's output buffer is stored in the UserBuffer field
        // and the user's input buffer is stored in the SystemBuffer field.
        //

        struct {
            ULONG OutputBufferLength;
            ULONG InputBufferLength;
            ULONG IoControlCode;
            PVOID Type3InputBuffer;
        } DeviceIoControl;

#if 0
        //
        // Non-system service parameters.
        //
        // Parameters for IRP_MN_QUERY_DEVICE_RELATIONS
        //

        struct {
            DEVICE_RELATION_TYPE Type;
        } QueryDeviceRelations;

        //
        // Parameters for IRP_MN_QUERY_INTERFACE
        //

        struct {
            GUID *InterfaceType;
            USHORT Size;
            USHORT Version;
            PINTERFACE Interface;
        } QueryInterface;

        //
        // Parameters for IRP_MN_QUERY_CAPABILITIES
        //

        struct {
            PDEVICE_CAPABILITIES Capabilities;
        } DeviceCapabilities;

        //
        // Parameters for IRP_MN_QUERY_RESOURCE_REQUIREMENTS
        //

        struct {
            ULONG NoArguments;
        } QueryResourceRequirements;

        //
        // Parameters for IRP_MN_SET_RESOURCE_REQUIREMENTS
        //

        struct {
            PIO_RESOURCE_REQUIREMENTS_LIST IoResourceRequirementList;
        } SetResourceRequirements;

        //
        // Parameters for IRP_MN_READ_CONFIG and IRP_MN_WRITE_CONFIG
        //

        struct {
            ULONG WhichSpace;
            PVOID Buffer;
            ULONG Offset;
            ULONG Length;
        } ReadWriteConfig;

        //
        // Parameters for IRP_MN_EJECT
        //

        struct {
            ULONG NoArguments;
        } Eject;

        //
        // Parameters for IRP_MN_SET_LOCK
        //

        struct {
            BOOLEAN Lock;
        } SetLock;

        //
        // Parameters for IRP_MN_QUERY_ID
        //

        struct {
            BUS_QUERY_ID_TYPE IdType;
        } QueryId;

        //
        // Parameters for IRP_MN_PNP_DEVICE_STATE
        //

        struct {
            BOOLEAN Query;
            UCHAR Reserved[3];
            PNP_DEVICE_STATE PnpDeviceState;
        } PnpDeviceState;

        //
        // Parameters for IRP_MN_WAIT_WAKE
        //

        struct {
            SYSTEM_POWER_STATE PowerState;
        } WaitWake;

        //
        // Parameter for IRP_MN_POWER_SEQUENCE
        //

        struct {
            PPOWER_SEQUENCE PowerSequence;
        } PowerSequence;
#endif

        //
        // Parameters for IRP_MN_SET_POWER and IRP_MN_QUERY_POWER
        //

        struct {
            ULONG SystemContext;
            POWER_STATE_TYPE Type;
            POWER_STATE State;
        } Power;

#if 0

        //
        // Parameters for device removal irp
        //

        struct {
            PDEVICE_OBJECT DeviceToRemove;
        } RemoveDevice;

        //
        // Parameters for StartDevice
        //

        struct {
            PCM_RESOURCE_LIST AllocatedResources;
        } StartDevice;
#endif

        //
        // Parameters for Cleanup
        //
        // No extra parameters supplied
        //

        //
        // Others - driver-specific
        //

        struct {
            PVOID Argument1;
            PVOID Argument2;
            PVOID Argument3;
            PVOID Argument4;
        } Others;

    } Parameters;

    //
    // Save a pointer to this device driver's device object for this request
    // so it can be passed to the completion routine if needed.
    //

    PDEVICE_OBJECT DeviceObject;

    //
    // The following location contains a pointer to the file object for this
    //

    PFILE_OBJECT FileObject;

    //
    // The following routine is invoked depending on the flags in the above
    // flags field.
    //

    PIO_COMPLETION_ROUTINE CompletionRoutine;

    //
    // The following is used to store the address of the context parameter
    // that should be passed to the CompletionRoutine.
    //

    PVOID Context;

} IO_STACK_LOCATION, *PIO_STACK_LOCATION;

//
// Define the major function codes for IRPs.  The lower 128 codes, from 0x00 to
// 0x7f are reserved to Microsoft.  The upper 128 codes, from 0x80 to 0xff, are
// reserved to customers of Microsoft.
//
// WinCE Note: IRPs have been renumbered to save space
//
#define IRP_MJ_CREATE                   0x00
//#define IRP_MJ_CREATE_NAMED_PIPE        0x01
#define IRP_MJ_CLOSE                    0x01
#define IRP_MJ_READ                     0x02
#define IRP_MJ_WRITE                    0x03
//#define IRP_MJ_QUERY_INFORMATION        0x05
//#define IRP_MJ_SET_INFORMATION          0x06
//#define IRP_MJ_QUERY_EA                 0x07
//#define IRP_MJ_SET_EA                   0x08
#define IRP_MJ_FLUSH_BUFFERS            0x04
//#define IRP_MJ_QUERY_VOLUME_INFORMATION 0x0a
//#define IRP_MJ_SET_VOLUME_INFORMATION   0x0b
//#define IRP_MJ_DIRECTORY_CONTROL        0x0c
//#define IRP_MJ_FILE_SYSTEM_CONTROL      0x0d
#define IRP_MJ_DEVICE_CONTROL           0x05
#define IRP_MJ_INTERNAL_DEVICE_CONTROL  0x06
#define IRP_MJ_SHUTDOWN                 0x07
//#define IRP_MJ_LOCK_CONTROL             0x11
#define IRP_MJ_CLEANUP                  0x08
//#define IRP_MJ_CREATE_MAILSLOT          0x13
//#define IRP_MJ_QUERY_SECURITY           0x14
//#define IRP_MJ_SET_SECURITY             0x15
#define IRP_MJ_POWER                    0x09
//#define IRP_MJ_NOT_DEFINED              0x17    // available
#define IRP_MJ_DEVICE_CHANGE            0x0a
//#define IRP_MJ_QUERY_QUOTA              0x19
//#define IRP_MJ_SET_QUOTA                0x1a
#define IRP_MJ_PNP                      0x0b
#define IRP_MJ_PNP_POWER                IRP_MJ_PNP      // Obsolete
#define IRP_MJ_MAXIMUM_FUNCTION         0x0c

//
// Define the minor function codes for IRPs.  The lower 128 codes, from 0x00 to
// 0x7f are reserved to Microsoft.  The upper 128 codes, from 0x80 to 0xff, are
// reserved to customers of Microsoft.
//

//
// PNP/Power minor function codes.
//

#define IRP_MN_START_DEVICE                 0x00
#define IRP_MN_QUERY_REMOVE_DEVICE          0x01
#define IRP_MN_REMOVE_DEVICE                0x02
#define IRP_MN_CANCEL_REMOVE_DEVICE         0x03
#define IRP_MN_STOP_DEVICE                  0x04
#define IRP_MN_QUERY_STOP_DEVICE            0x05
#define IRP_MN_CANCEL_STOP_DEVICE           0x06

#define IRP_MN_QUERY_DEVICE_RELATIONS       0x07
#define IRP_MN_QUERY_INTERFACE              0x08
#define IRP_MN_QUERY_CAPABILITIES           0x09
#define IRP_MN_QUERY_RESOURCES              0x0A
#define IRP_MN_QUERY_RESOURCE_REQUIREMENTS  0x0B
#define IRP_MN_SET_RESOURCE_REQUIREMENTS    0x0C
#define IRP_MN_ADJUST_RESOURCES             0x0D
#define IRP_MN_SET_DEVICE_RESOURCES         0x0E

#define IRP_MN_READ_CONFIG                  0x0F
#define IRP_MN_WRITE_CONFIG                 0x10
#define IRP_MN_EJECT                        0x11
#define IRP_MN_SET_LOCK                     0x12
#define IRP_MN_QUERY_ID                     0x13
#define IRP_MN_PNP_DEVICE_STATE             0x14

#define IRP_MN_WAIT_WAKE                    0x15
#define IRP_MN_POWER_SEQUENCE               0x16
#define IRP_MN_SET_POWER                    0x17
#define IRP_MN_QUERY_POWER                  0x18

//
// Define I/O Request Packet (IRP) flags
//

#define IRP_NOCACHE                     0x00000001
#define IRP_PAGING_IO                   0x00000002
#define IRP_MOUNT_COMPLETION            0x00000002
#define IRP_SYNCHRONOUS_API             0x00000004
#define IRP_ASSOCIATED_IRP              0x00000008
#define IRP_BUFFERED_IO                 0x00000010
#define IRP_DEALLOCATE_BUFFER           0x00000020
#define IRP_INPUT_OPERATION             0x00000040
#define IRP_SYNCHRONOUS_PAGING_IO       0x00000040
#define IRP_CREATE_OPERATION            0x00000080
#define IRP_READ_OPERATION              0x00000100
#define IRP_WRITE_OPERATION             0x00000200
#define IRP_CLOSE_OPERATION             0x00000400
#define IRP_DEFER_IO_COMPLETION         0x00000800
#define IRP_OB_QUERY_NAME               0x00001000
#define IRP_HOLD_DEVICE_QUEUE           0x00002000

//
// Define I/O request packet (IRP) alternate flags for allocation control.
//

#define IRP_QUOTA_CHARGED               0x01
#define IRP_ALLOCATED_MUST_SUCCEED      0x02
#define IRP_ALLOCATED_FIXED_SIZE        0x04

//
// I/O Request Packet (IRP) definition
//

typedef struct _IRP {
//    CSHORT Type;
//    USHORT Size;

    //
    // Define the common fields used to control the IRP.
    //

    //
    // Define a pointer to the Memory Descriptor List (MDL) for this I/O
    // request.  This field is only used if the I/O is "direct I/O".
    //

    PMDL MdlAddress;

    //
    // Flags word - used to remember various flags.
    //

    ULONG Flags;

    //
    // The following union is used for one of three purposes:
    //
    //    1. This IRP is an associated IRP.  The field is a pointer to a master
    //       IRP.
    //
    //    2. This is the master IRP.  The field is the count of the number of
    //       IRPs which must complete (associated IRPs) before the master can
    //       complete.
    //
    //    3. This operation is being buffered and the field is the address of
    //       the system space buffer.
    //

    union {
        struct _IRP *MasterIrp;
        LONG IrpCount;
        PVOID SystemBuffer;
    } AssociatedIrp;

    //
    // Thread list entry - allows queueing the IRP to the thread pending I/O
    // request packet list.
    //

    LIST_ENTRY ThreadListEntry;

    //
    // I/O status - final status of operation.
    //

    IO_STATUS_BLOCK IoStatus;

    //
    // Requestor mode - mode of the original requestor of this operation.
    //

//    KPROCESSOR_MODE RequestorMode;

    //
    // Pending returned - TRUE if pending was initially returned as the
    // status for this packet.
    //

    BOOLEAN PendingReturned;

    //
    // Stack state information.
    //

    CHAR StackCount;
    CHAR CurrentLocation;

    //
    // Cancel - packet has been canceled.
    //

    BOOLEAN Cancel;

    //
    // Cancel Irql - Irql at which the cancel spinlock was acquired.
    //

//    KIRQL CancelIrql;

    //
    // ApcEnvironment - Used to save the APC environment at the time that the
    // packet was initialized.
    //

//    CCHAR ApcEnvironment;

    //
    // Allocation control flags.
    //

    UCHAR AllocationFlags;

    //
    // User parameters.
    //

    PIO_STATUS_BLOCK UserIosb;
    HANDLE UserEvent;
    union {
        struct {
            PIO_APC_ROUTINE UserApcRoutine;
            PVOID UserApcContext;
        } AsynchronousParameters;
        LARGE_INTEGER AllocationSize;
    } Overlay;

    //
    // CancelRoutine - Used to contain the address of a cancel routine supplied
    // by a device driver when the IRP is in a cancelable state.
    //

    PDRIVER_CANCEL CancelRoutine;

    //
    // Note that the UserBuffer parameter is outside of the stack so that I/O
    // completion can copy data back into the user's address space without
    // having to know exactly which service was being invoked.  The length
    // of the copy is stored in the second half of the I/O status block. If
    // the UserBuffer field is NULL, then no copy is performed.
    //

    PVOID UserBuffer;

    //
    // Kernel structures
    //
    // The following section contains kernel structures which the IRP needs
    // in order to place various work information in kernel controller system
    // queues.  Because the size and alignment cannot be controlled, they are
    // placed here at the end so they just hang off and do not affect the
    // alignment of other fields in the IRP.
    //

    union {

        struct {

            union {

                //
                // DeviceQueueEntry - The device queue entry field is used to
                // queue the IRP to the device driver device queue.
                //

                KDEVICE_QUEUE_ENTRY DeviceQueueEntry;

                struct {

                    //
                    // The following are available to the driver to use in
                    // whatever manner is desired, while the driver owns the
                    // packet.
                    //

                    PVOID DriverContext[4];

                } ;

            } ;

            //
            // Thread - pointer to caller's Thread Control Block.
            //

//            PETHREAD Thread;
            HANDLE CallerProcess;

            //
            // Auxiliary buffer - pointer to any auxiliary buffer that is
            // required to pass information to a driver that is not contained
            // in a normal buffer.
            //

            PCHAR AuxiliaryBuffer;

            //
            // List entry - used to queue the packet to completion queue, among
            // others.
            //

            LIST_ENTRY ListEntry;

            //
            // Current stack location - contains a pointer to the current
            // IO_STACK_LOCATION structure in the IRP stack.  This field
            // should never be directly accessed by drivers.  They should
            // use the standard functions.
            //

            struct _IO_STACK_LOCATION *CurrentStackLocation;

            //
            // Original file object - pointer to the original file object
            // that was used to open the file.  This field is owned by the
            // I/O system and should not be used by any other drivers.
            //

            PFILE_OBJECT OriginalFileObject;

        } Overlay;

        //
        // APC - This APC control block is used for the special kernel APC as
        // well as for the caller's APC, if one was specified in the original
        // argument list.  If so, then the APC is reused for the normal APC for
        // whatever mode the caller was in and the "special" routine that is
        // invoked before the APC gets control simply deallocates the IRP.
        //

//        KAPC Apc;

        //
        // CompletionKey - This is the key that is used to distinguish
        // individual I/O operations initiated on a single file handle.
        //

        ULONG CompletionKey;

    } Tail;

} IRP, *PIRP;

//
// Define stack location control flags
//

#define SL_PENDING_RETURNED             0x01
#define SL_INVOKE_ON_CANCEL             0x20
#define SL_INVOKE_ON_SUCCESS            0x40
#define SL_INVOKE_ON_ERROR              0x80

//
// Define the Device Object Extension Flags
//

#define DOE_UNLOAD_PENDING              0x00000001
#define DOE_DELETE_PENDING              0x00000002
#define DOE_REMOVE_PENDING              0x00000004
#define DOE_REMOVE_PROCESSED            0x00000008
#define DOE_START_PENDING               0x00000010

typedef struct _DEVOBJ_EXTENSION {

//    CSHORT          Type;
//    USHORT          Size;

    //
    // Public part of the DeviceObjectExtension structure
    //

    PDEVICE_OBJECT  DeviceObject;               // owning device object

    //
    // Shared Power Management fields
    //

    ULONG           IdleCount;                  // SHARED field via PoSetDeviceBusy()
//    POWER_STATE     CurrentPowerState;
    ULONG           CurrentDevicePowerState;
    BOOLEAN         StartIoQueueHolding;        // PO & IO
    BOOLEAN         UseAsyncPowerUp;
    BOOLEAN         PowerControlNeeded;
    BOOLEAN         PowerControlPagable;

	
    //
    // Device object extension flags.  Protected by the IopDatabaseLock.
    //

    ULONG ExtensionFlags;
    
    //
    // AttachedTo is a pointer to the device object that this device
    // object is attached to.  The attachment chain is now doubly
    // linked: this pointer and DeviceObject->AttachedDevice provide the
    // linkage.
    //

    PDEVICE_OBJECT  AttachedTo;

    //
    // Note: any new shared fields get added here.
    //



} DEVOBJ_EXTENSION, *PDEVOBJ_EXTENSION;

//
// Define Driver Object (DRVO) flags
//

#define DRVO_UNLOAD_INVOKED             0x00000001
#define DRVO_LEGACY_DRIVER              0x00000002

typedef struct _DRIVER_EXTENSION {

    //
    // Back pointer to Driver Object
    //

    struct _DRIVER_OBJECT *DriverObject;

    //
    // The AddDevice entry point is called by the Plug & Play manager
    // to inform the driver when a new device instance arrives that this
    // driver must control.
    //

    PDRIVER_ADD_DEVICE AddDevice;

    //
    // The count field is used to count the number of times the driver has
    // had its registered reinitialization routine invoked.
    //

    ULONG Count;

    //
    // The service name field is used by the pnp manager to determine
    // where the driver related info is stored in the registry.
    //

    UNICODE_STRING ServiceKeyName;

    //
    // Note: any new shared fields get added here.
    //


} DRIVER_EXTENSION, *PDRIVER_EXTENSION;

typedef struct _DRIVER_OBJECT {
//    CSHORT Type;
//    CSHORT Size;

    //
    // The following links all of the devices created by a single driver
    // together on a list, and the Flags word provides an extensible flag
    // location for driver objects.
    //

    PDEVICE_OBJECT DeviceObject;
    ULONG Flags;

    //
    // The following section describes where the driver is loaded.  The count
    // field is used to count the number of times the driver has had its
    // registered reinitialization routine invoked.
    //

//    PVOID DriverStart;
//    ULONG DriverSize;
//    PVOID DriverSection;
    PDRIVER_EXTENSION DriverExtension;

    //
    // The driver name field is used by the error log thread
    // determine the name of the driver that an I/O request is/was bound.
    //

//    UNICODE_STRING DriverName;

    //
    // The following section is for registry support.  This is a pointer
    // to the path to the hardware information in the registry
    //

//    PUNICODE_STRING HardwareDatabase;

    //
    // The following section contains the optional pointer to an array of
    // alternate entry points to a driver for "fast I/O" support.  Fast I/O
    // is performed by invoking the driver routine directly with separate
    // parameters, rather than using the standard IRP call mechanism.  Note
    // that these functions may only be used for synchronous I/O, and when
    // the file is cached.
    //

    PFAST_IO_DISPATCH FastIoDispatch;

    //
    // The following section describes the entry points to this particular
    // driver.  Note that the major function dispatch table must be the last
    // field in the object so that it remains extensible.
    //

    PDRIVER_INITIALIZE DriverInit;
    PDRIVER_STARTIO DriverStartIo;
    PDRIVER_UNLOAD DriverUnload;
    PDRIVER_DISPATCH MajorFunction[IRP_MJ_MAXIMUM_FUNCTION + 1];

} DRIVER_OBJECT;
typedef struct _DRIVER_OBJECT *PDRIVER_OBJECT; // ntndis



//
// Define Device Status for IoReportDeviceStatus
// 0 - 0x7fffffff are for private use and 0x80000000 -
// 0xffffffff are reserved for system.
//

#define DEVICE_STATUS_OK            0x80000000
#define DEVICE_STATUS_MALFUNCTIONED 0x80000001
#define DEVICE_STATUS_REMOVED       0x80000002
#define DEVICE_STATUS_DISABLED      0x80000003

//
// Define Wait Context Block (WCB)
//

typedef struct _WAIT_CONTEXT_BLOCK {
    KDEVICE_QUEUE_ENTRY WaitQueueEntry;
    PDRIVER_CONTROL DeviceRoutine;
    PVOID DeviceContext;
    ULONG NumberOfMapRegisters;
    PVOID DeviceObject;
    PVOID CurrentIrp;
    PKDPC BufferChainingDpc;
} WAIT_CONTEXT_BLOCK, *PWAIT_CONTEXT_BLOCK;


//
// Define Device Object (DO) flags
//

#define DO_VERIFY_VOLUME                0x00000002
#define DO_BUFFERED_IO                  0x00000004
#define DO_EXCLUSIVE                    0x00000008
#define DO_DIRECT_IO                    0x00000010
#define DO_MAP_IO_BUFFER                0x00000020
#define DO_DEVICE_HAS_NAME              0x00000040
#define DO_DEVICE_INITIALIZING          0x00000080
#define DO_SYSTEM_BOOT_PARTITION        0x00000100
#define DO_LONG_TERM_REQUESTS           0x00000200
#define DO_NEVER_LAST_DEVICE            0x00000400
#define DO_SHUTDOWN_REGISTERED          0x00000800
#define DO_BUS_ENUMERATED_DEVICE        0x00001000

//
// Device Object structure definition
//

typedef struct _DEVICE_OBJECT {
//    CSHORT Type;
//    USHORT Size;
    LONG ReferenceCount;
    struct _DRIVER_OBJECT *DriverObject;
    struct _DEVICE_OBJECT *NextDevice;
    struct _DEVICE_OBJECT *AttachedDevice;
    struct _IRP *CurrentIrp;
//    PIO_TIMER Timer;
    ULONG Flags;                                // See above:  DO_...
    ULONG Characteristics;                      // See ntioapi:  FILE_...
//    PVOID DoNotUse1;
    PVOID DeviceExtension;
    DEVICE_TYPE DeviceType;
    CCHAR StackSize;
    union {
//        LIST_ENTRY ListEntry;
        WAIT_CONTEXT_BLOCK Wcb;
    } Queue;
    ULONG AlignmentRequirement;
    KDEVICE_QUEUE DeviceQueue;
    KDPC Dpc;

    //
    //  The following field is for exclusive use by the filesystem to keep
    //  track of the number of Fsp threads currently using the device
    //

    ULONG ActiveThreadCount;
//    PSECURITY_DESCRIPTOR SecurityDescriptor;
//    KEVENT DeviceLock;

//    USHORT SectorSize;
    USHORT Spare1;

    struct _DEVOBJ_EXTENSION  *DeviceObjectExtension;
//    PVOID  Reserved;
} DEVICE_OBJECT, *PDEVICE_OBJECT;

#if x86

int __cdecl _inp(unsigned short);
unsigned short __cdecl _inpw(unsigned short);
unsigned long __cdecl _inpd(unsigned short);

int __cdecl _outp(unsigned short, int);
unsigned short __cdecl _outpw(unsigned short, unsigned short);
unsigned long __cdecl _outpd(unsigned short, unsigned long);

ULONG __inline READ_PORT_ULONG(PULONG port)
{
    return _inpd((USHORT)port);
}

VOID __inline WRITE_PORT_ULONG(PULONG port, ULONG value)
{
    _outpd((USHORT)port, (value));
}

USHORT __inline READ_PORT_USHORT(PUSHORT port)
{
    return _inpw((USHORT)port);
}

VOID __inline WRITE_PORT_USHORT(PUSHORT port, USHORT value)
{
    _outpw((USHORT)port, (value));
}

UCHAR __inline READ_PORT_UCHAR(PUCHAR port)
{
    return _inp((USHORT)port);
}

VOID __inline WRITE_PORT_UCHAR(PUCHAR port, UCHAR value)
{
    _outp((USHORT)port, (value));
}

#define READ_REGISTER_ULONG(reg) \
    (*(volatile unsigned long * const)(reg))

#define WRITE_REGISTER_ULONG(reg, val) \
    (*(volatile unsigned long * const)(reg)) = (val)

#define READ_REGISTER_USHORT(reg) \
    (*(volatile unsigned short * const)(reg))

#define WRITE_REGISTER_USHORT(reg, val) \
    (*(volatile unsigned short * const)(reg)) = (val)

#define READ_REGISTER_UCHAR(reg) \
    (*(volatile unsigned char * const)(reg))

#define WRITE_REGISTER_UCHAR(reg, val) \
    (*(volatile unsigned char * const)(reg)) = (val)

// end of x86

#elif defined(MIPS) || defined(PPC) || defined(ARM)

ULONG __inline READ_PORT_ULONG(PULONG port)
{
    return *(volatile unsigned long * const)port;
}

VOID __inline WRITE_PORT_ULONG(PULONG port, ULONG value)
{
    *(volatile unsigned long * const)port = value;
}

USHORT __inline READ_PORT_USHORT(PUSHORT port)
{
    return *(volatile unsigned short * const)port;
}

VOID __inline WRITE_PORT_USHORT(PUSHORT port, USHORT value)
{
    *(volatile unsigned short * const)port = value;
}

UCHAR __inline READ_PORT_UCHAR(PUCHAR port)
{
    return *(volatile unsigned char * const)port;
}

VOID __inline WRITE_PORT_UCHAR(PUCHAR port, UCHAR value)
{
    *(volatile unsigned char * const)port = value;
}

#define READ_REGISTER_ULONG(reg) \
    (*(volatile unsigned long * const)(reg))

#define WRITE_REGISTER_ULONG(reg, val) \
    (*(volatile unsigned long * const)(reg)) = (val)

#define READ_REGISTER_USHORT(reg) \
    (*(volatile unsigned short * const)(reg))

#define WRITE_REGISTER_USHORT(reg, val) \
    (*(volatile unsigned short * const)(reg)) = (val)

#define READ_REGISTER_UCHAR(reg) \
    (*(volatile unsigned char * const)(reg))

#define WRITE_REGISTER_UCHAR(reg, val) \
    (*(volatile unsigned char * const)(reg)) = (val)

#elif SHx

ULONG READ_PORT_ULONG(PULONG port);

VOID WRITE_PORT_ULONG(PULONG port, ULONG value);

USHORT READ_PORT_USHORT(PUSHORT port);

VOID WRITE_PORT_USHORT(PUSHORT port, USHORT value);

UCHAR READ_PORT_UCHAR(PUCHAR port);

VOID WRITE_PORT_UCHAR(PUCHAR port, UCHAR value);

#define READ_REGISTER_ULONG(reg) \
    (*(volatile unsigned long * const)(reg))

#define WRITE_REGISTER_ULONG(reg, val) \
    (*(volatile unsigned long * const)(reg)) = (val)

#define READ_REGISTER_USHORT(reg) \
    (*(volatile unsigned short * const)(reg))

#define WRITE_REGISTER_USHORT(reg, val) \
    (*(volatile unsigned short * const)(reg)) = (val)

#define READ_REGISTER_UCHAR(reg) \
    (*(volatile unsigned char * const)(reg))

#define WRITE_REGISTER_UCHAR(reg, val) \
    (*(volatile unsigned char * const)(reg)) = (val)

#endif // MIPS


//
// DMA related 
//
typedef struct _SCATTER_GATHER_ELEMENT {
    PHYSICAL_ADDRESS Address;
    ULONG Length;
    ULONG Reserved;
} SCATTER_GATHER_ELEMENT, *PSCATTER_GATHER_ELEMENT;

typedef struct _SCATTER_GATHER_LIST {
    ULONG NumberOfElements;
    ULONG Reserved;        
    // Elements is not only 1 element long.  It can be of any size.
    SCATTER_GATHER_ELEMENT Elements[1];
} SCATTER_GATHER_LIST, *PSCATTER_GATHER_LIST;


//
// Define the DMA transfer widths.
//
typedef enum _DMA_WIDTH {
    Width8Bits,
    Width16Bits,
    Width32Bits,
    MaximumDmaWidth
}DMA_WIDTH, *PDMA_WIDTH;


//
// Define DMA transfer speeds.
//
typedef enum _DMA_SPEED {
    Compatible,
    TypeA,
    TypeB,
    TypeC,
    TypeF,
    MaximumDmaSpeed
}DMA_SPEED, *PDMA_SPEED;


//
// Define the device description structure.
//
typedef struct _DEVICE_DESCRIPTION {
    ULONG Version;
    BOOLEAN Master;
    BOOLEAN ScatterGather;
    BOOLEAN DemandMode;
    BOOLEAN AutoInitialize;
    BOOLEAN Dma32BitAddresses;
    BOOLEAN IgnoreCount;
    BOOLEAN Reserved1;          // must be false
    BOOLEAN Dma64BitAddresses;
    ULONG DoNotUse2;
    ULONG DmaChannel;
    INTERFACE_TYPE  InterfaceType;
    DMA_WIDTH DmaWidth;
    DMA_SPEED DmaSpeed;
    ULONG MaximumLength;
    ULONG DmaPort;
} DEVICE_DESCRIPTION, *PDEVICE_DESCRIPTION;

//
// Define the supported version numbers for the device description structure.
//
#define DEVICE_DESCRIPTION_VERSION  0
#define DEVICE_DESCRIPTION_VERSION1 1


typedef struct _DMA_OPERATIONS *PDMA_OPERATIONS;

typedef struct _DMA_ADAPTER {
//    USHORT Version;
    USHORT Size;
    PDMA_OPERATIONS DmaOperations;
    // Private Bus Device Driver data follows,
} DMA_ADAPTER, *PDMA_ADAPTER;

typedef VOID (*PPUT_DMA_ADAPTER)(
    PDMA_ADAPTER DmaAdapter
    );

typedef PVOID (*PALLOCATE_COMMON_BUFFER)(
    IN PDMA_ADAPTER DmaAdapter,
    IN ULONG Length,
    OUT PPHYSICAL_ADDRESS LogicalAddress,
    IN BOOLEAN CacheEnabled
    );

typedef VOID (*PFREE_COMMON_BUFFER)(
    IN PDMA_ADAPTER DmaAdapter,
    IN ULONG Length,
    IN PHYSICAL_ADDRESS LogicalAddress,
    IN PVOID VirtualAddress,
    IN BOOLEAN CacheEnabled
    );

typedef NTSTATUS (*PALLOCATE_ADAPTER_CHANNEL)(
    IN PDMA_ADAPTER DmaAdapter,
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG NumberOfMapRegisters,
    IN PDRIVER_CONTROL ExecutionRoutine,
    IN PVOID Context
    );

typedef BOOLEAN (*PFLUSH_ADAPTER_BUFFERS)(
    IN PDMA_ADAPTER DmaAdapter,
    IN PMDL Mdl,
    IN PVOID MapRegisterBase,
    IN PVOID CurrentVa,
    IN ULONG Length,
    IN BOOLEAN WriteToDevice
    );

typedef VOID (*PFREE_ADAPTER_CHANNEL)(
    IN PDMA_ADAPTER DmaAdapter
    );

typedef VOID (*PFREE_MAP_REGISTERS)(
    IN PDMA_ADAPTER DmaAdapter,
    PVOID MapRegisterBase,
    ULONG NumberOfMapRegisters
    );

typedef PHYSICAL_ADDRESS (*PMAP_TRANSFER)(
    IN PDMA_ADAPTER DmaAdapter,
    IN PMDL Mdl,
    IN PVOID MapRegisterBase,
    IN PVOID CurrentVa,
    IN OUT PULONG Length,
    IN BOOLEAN WriteToDevice
    );

typedef ULONG (*PGET_DMA_ALIGNMENT)(
    IN PDMA_ADAPTER DmaAdapter
    );

typedef ULONG (*PREAD_DMA_COUNTER)(
    IN PDMA_ADAPTER DmaAdapter
    );

typedef VOID
(*PDRIVER_LIST_CONTROL)(
    IN struct _DEVICE_OBJECT *DeviceObject,
    IN struct _IRP *Irp,
    IN PSCATTER_GATHER_LIST ScatterGather,
    IN PVOID Context
    );

typedef NTSTATUS
(*PGET_SCATTER_GATHER_LIST)(
    IN PDMA_ADAPTER DmaAdapter,
    IN PDEVICE_OBJECT DeviceObject,
    IN PMDL Mdl,
    IN PVOID CurrentVa,
    IN ULONG Length,
    IN PDRIVER_LIST_CONTROL ExecutionRoutine,
    IN PVOID Context,
    IN BOOLEAN WriteToDevice
    );

typedef VOID
(*PPUT_SCATTER_GATHER_LIST)(
    IN PDMA_ADAPTER DmaAdapter,
    IN PSCATTER_GATHER_LIST ScatterGather,
    IN BOOLEAN WriteToDevice
    );

typedef struct _DMA_OPERATIONS {
    ULONG Size;
    PPUT_DMA_ADAPTER PutDmaAdapter;
    PALLOCATE_COMMON_BUFFER AllocateCommonBuffer;
    PFREE_COMMON_BUFFER FreeCommonBuffer;
    PALLOCATE_ADAPTER_CHANNEL AllocateAdapterChannel;
    PFLUSH_ADAPTER_BUFFERS FlushAdapterBuffers;
    PFREE_ADAPTER_CHANNEL FreeAdapterChannel;
    PFREE_MAP_REGISTERS FreeMapRegisters;
    PMAP_TRANSFER MapTransfer;
    PGET_DMA_ALIGNMENT GetDmaAlignment;
    PREAD_DMA_COUNTER ReadDmaCounter;
    PGET_SCATTER_GATHER_LIST GetScatterGatherList;
    PPUT_SCATTER_GATHER_LIST PutScatterGatherList;
} DMA_OPERATIONS;


__inline
PVOID
HalAllocateCommonBuffer(
    IN PDMA_ADAPTER DmaAdapter,
    IN ULONG Length,
    OUT PPHYSICAL_ADDRESS LogicalAddress,
    IN BOOLEAN CacheEnabled
    ){

    PALLOCATE_COMMON_BUFFER allocateCommonBuffer;
    PVOID commonBuffer;

    allocateCommonBuffer = *(DmaAdapter)->DmaOperations->AllocateCommonBuffer;
    ASSERT( allocateCommonBuffer != NULL );

    commonBuffer = allocateCommonBuffer( DmaAdapter,
                                         Length,
                                         LogicalAddress,
                                         CacheEnabled );

    return commonBuffer;
}


__inline
VOID
HalFreeCommonBuffer(
    IN PDMA_ADAPTER DmaAdapter,
    IN ULONG Length,
    IN PHYSICAL_ADDRESS LogicalAddress,
    IN PVOID VirtualAddress,
    IN BOOLEAN CacheEnabled
    ){

    PFREE_COMMON_BUFFER freeCommonBuffer;

    freeCommonBuffer = *(DmaAdapter)->DmaOperations->FreeCommonBuffer;
    ASSERT( freeCommonBuffer != NULL );

    freeCommonBuffer( DmaAdapter,
                      Length,
                      LogicalAddress,
                      VirtualAddress,
                      CacheEnabled );
}


__inline
NTSTATUS
IoAllocateAdapterChannel(
    IN PDMA_ADAPTER DmaAdapter,
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG NumberOfMapRegisters,
    IN PDRIVER_CONTROL ExecutionRoutine,
    IN PVOID Context
    ){

    PALLOCATE_ADAPTER_CHANNEL allocateAdapterChannel;
    NTSTATUS status;

    allocateAdapterChannel =
        *(DmaAdapter)->DmaOperations->AllocateAdapterChannel;

    ASSERT( allocateAdapterChannel != NULL );

    status = allocateAdapterChannel( DmaAdapter,
                                     DeviceObject,
                                     NumberOfMapRegisters,
                                     ExecutionRoutine,
                                     Context );

    return status;
}


__inline
BOOLEAN
IoFlushAdapterBuffers(
    IN PDMA_ADAPTER DmaAdapter,
    IN PMDL Mdl,
    IN PVOID MapRegisterBase,
    IN PVOID CurrentVa,
    IN ULONG Length,
    IN BOOLEAN WriteToDevice
    ){

    PFLUSH_ADAPTER_BUFFERS flushAdapterBuffers;
    BOOLEAN result;

    flushAdapterBuffers = *(DmaAdapter)->DmaOperations->FlushAdapterBuffers;
    ASSERT( flushAdapterBuffers != NULL );

    result = flushAdapterBuffers( DmaAdapter,
                                  Mdl,
                                  MapRegisterBase,
                                  CurrentVa,
                                  Length,
                                  WriteToDevice );
    return result;
}


__inline
VOID
IoFreeAdapterChannel(
    IN PDMA_ADAPTER DmaAdapter
    ){

    PFREE_ADAPTER_CHANNEL freeAdapterChannel;

    freeAdapterChannel = *(DmaAdapter)->DmaOperations->FreeAdapterChannel;
    ASSERT( freeAdapterChannel != NULL );

    freeAdapterChannel( DmaAdapter );
}


__inline
VOID
IoFreeMapRegisters(
    IN PDMA_ADAPTER DmaAdapter,
    IN PVOID MapRegisterBase,
    IN ULONG NumberOfMapRegisters
    ){

    PFREE_MAP_REGISTERS freeMapRegisters;

    freeMapRegisters = *(DmaAdapter)->DmaOperations->FreeMapRegisters;
    ASSERT( freeMapRegisters != NULL );

    freeMapRegisters( DmaAdapter,
                      MapRegisterBase,
                      NumberOfMapRegisters );
}


__inline
PHYSICAL_ADDRESS
IoMapTransfer(
    IN PDMA_ADAPTER DmaAdapter,
    IN PMDL Mdl,
    IN PVOID MapRegisterBase,
    IN PVOID CurrentVa,
    IN OUT PULONG Length,
    IN BOOLEAN WriteToDevice
    ){

    PHYSICAL_ADDRESS physicalAddress;
    PMAP_TRANSFER mapTransfer;

    mapTransfer = *(DmaAdapter)->DmaOperations->MapTransfer;
    ASSERT( mapTransfer != NULL );

    physicalAddress = mapTransfer( DmaAdapter,
                                   Mdl,
                                   MapRegisterBase,
                                   CurrentVa,
                                   Length,
                                   WriteToDevice );

    return physicalAddress;
}


__inline
ULONG
HalGetDmaAlignment(
    IN PDMA_ADAPTER DmaAdapter
    )
{
    PGET_DMA_ALIGNMENT getDmaAlignment;
    ULONG alignment;

    getDmaAlignment = *(DmaAdapter)->DmaOperations->GetDmaAlignment;
    ASSERT( getDmaAlignment != NULL );

    alignment = getDmaAlignment( DmaAdapter );
    return alignment;
}


__inline
ULONG
HalReadDmaCounter(
    IN PDMA_ADAPTER DmaAdapter
    )
{
    PREAD_DMA_COUNTER readDmaCounter;
    ULONG counter;

    readDmaCounter = *(DmaAdapter)->DmaOperations->ReadDmaCounter;
    ASSERT( readDmaCounter != NULL );

    counter = readDmaCounter( DmaAdapter );
    return counter;
}

typedef struct _DMA_ADAPTER *(*PGET_DMA_ADAPTER)(
    IN PVOID Context,
    IN struct _DEVICE_DESCRIPTION *DeviceDescriptor,
    OUT PULONG NumberOfMapRegisters
    );


LPVOID 
HalAllocatePhysicalMemory(
    IN LPVOID StartAddress,
    IN ULONG Size, 
    OUT PPHYSICAL_ADDRESS PhysicalBuffer,
    IN BOOLEAN CacheEnabled
    );

ULONG 
HalFreePhysicalMemory(
    IN PVOID VirtualAddress,
    IN ULONG Length 
    );

BOOL
HalMapInterrupt(
    IN ULONG InterruptNumber,
    IN ULONG InterruptVector,
    OUT DWORD *InterruptId
    );

PIRP
IoAllocateIrp(
    IN CCHAR StackSize,
    IN BOOLEAN ChargeQuota
    );

PMDL
IoAllocateMdl(
    IN PVOID VirtualAddress,
    IN ULONG Length,
    IN BOOLEAN SecondaryBuffer,
    IN BOOLEAN ChargeQuota,
    IN OUT PIRP Irp OPTIONAL
    );

/*
NTKERNELAPI
NTSTATUS
IoAttachDevice(
    IN PDEVICE_OBJECT SourceDevice,
    IN PUNICODE_STRING TargetDevice,
    OUT PDEVICE_OBJECT *AttachedDevice
    );
*/

NTKERNELAPI
PDEVICE_OBJECT
IoAttachDeviceToDeviceStack(
    IN PDEVICE_OBJECT SourceDevice,
    IN PDEVICE_OBJECT TargetDevice
    );

VOID
IoBuildPartialMdl(
    IN PMDL SourceMdl,
    IN OUT PMDL TargetMdl,
    IN PVOID VirtualAddress,
    IN ULONG Length
    );

PIRP
IoBuildAsynchronousFsdRequest(
    IN ULONG MajorFunction,
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PVOID Buffer OPTIONAL,
    IN ULONG Length OPTIONAL,
    IN PLARGE_INTEGER StartingOffset OPTIONAL,
    IN PIO_STATUS_BLOCK IoStatusBlock OPTIONAL
    );

PIRP
IoBuildDeviceIoControlRequest(
    IN ULONG IoControlCode,
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN BOOLEAN InternalDeviceIoControl,
    IN HANDLE Event,
    OUT PIO_STATUS_BLOCK IoStatusBlock
    );


PIRP
IoBuildSynchronousFsdRequest(
    IN ULONG MajorFunction,
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PVOID Buffer OPTIONAL,
    IN ULONG Length OPTIONAL,
    IN PLARGE_INTEGER StartingOffset OPTIONAL,
    IN HANDLE Event,
    OUT PIO_STATUS_BLOCK IoStatusBlock
    );

NTSTATUS
IoCallDriver(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

VOID
IoCompleteRequest(
    IN PIRP Irp,
    IN CCHAR PriorityBoost
    );

NTSTATUS
IoCreateDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN ULONG DeviceExtensionSize,
    IN PUNICODE_STRING DeviceName OPTIONAL,
    IN DEVICE_TYPE DeviceType,
    IN ULONG DeviceCharacteristics,
    IN BOOLEAN Exclusive,
    OUT PDEVICE_OBJECT *DeviceObject
    );

VOID
IoDeleteDevice(
    IN PDEVICE_OBJECT DeviceObject
    );

NTKERNELAPI
VOID
IoDetachDevice(
    IN OUT PDEVICE_OBJECT TargetDevice
    );

VOID
IoFreeIrp(
    IN PIRP Irp
    );

VOID
IoFreeMdl(
    IN PMDL Mdl
    );

NTSTATUS
IoSendWinCePnPMessage(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

NTSTATUS
IoSendWinCePnPCallback(
    IN PPNP_CALLBACK pCallback,
    IN PVOID pContext
    );

//++
//
// PIO_STACK_LOCATION
// IoGetCurrentIrpStackLocation(
//     IN PIRP Irp
//     )
//
// Routine Description:
//
//     This routine is invoked to return a pointer to the current stack location
//     in an I/O Request Packet (IRP).
//
// Arguments:
//
//     Irp - Pointer to the I/O Request Packet.
//
// Return Value:
//
//     The function value is a pointer to the current stack location in the
//     packet.
//
//--

#define IoGetCurrentIrpStackLocation( Irp ) ( (Irp)->Tail.Overlay.CurrentStackLocation )


//++
//
// PIO_STACK_LOCATION
// IoGetNextIrpStackLocation(
//     IN PIRP Irp
//     )
//
// Routine Description:
//
//     This routine is invoked to return a pointer to the next stack location
//     in an I/O Request Packet (IRP).
//
// Arguments:
//
//     Irp - Pointer to the I/O Request Packet.
//
// Return Value:
//
//     The function value is a pointer to the next stack location in the packet.
//
//--

#define IoGetNextIrpStackLocation( Irp ) (\
    (Irp)->Tail.Overlay.CurrentStackLocation - 1 )

NTSTATUS
IoGetDeviceObjectPointer(
    IN PUNICODE_STRING ObjectName,
    IN ACCESS_MASK DesiredAccess,
    OUT PFILE_OBJECT *FileObject,
    OUT PDEVICE_OBJECT *DeviceObject
    );

NTKERNELAPI
struct _DMA_ADAPTER *
IoGetDmaAdapter(
    IN PDEVICE_OBJECT PhysicalDeviceObject,           
    IN struct _DEVICE_DESCRIPTION *DeviceDescription,
    IN OUT PULONG NumberOfMapRegisters
    );

PDEVICE_OBJECT
IoGetRelatedDeviceObject (
    IN PFILE_OBJECT FileObject
    );

VOID
IoInitializeIrp(
    IN OUT PIRP Irp,
    IN USHORT PacketSize,
    IN CCHAR StackSize
    );

//++
//
// VOID
// IoSetCompletionRoutine(
//     IN PIRP Irp,
//     IN PIO_COMPLETION_ROUTINE CompletionRoutine,
//     IN PVOID Context,
//     IN BOOLEAN InvokeOnSuccess,
//     IN BOOLEAN InvokeOnError,
//     IN BOOLEAN InvokeOnCancel
//     )
//
// Routine Description:
//
//     This routine is invoked to set the address of a completion routine which
//     is to be invoked when an I/O packet has been completed by a lower-level
//     driver.
//
// Arguments:
//
//     Irp - Pointer to the I/O Request Packet itself.
//
//     CompletionRoutine - Address of the completion routine that is to be
//         invoked once the next level driver completes the packet.
//
//     Context - Specifies a context parameter to be passed to the completion
//         routine.
//
//     InvokeOnSuccess - Specifies that the completion routine is invoked when the
//         operation is successfully completed.
//
//     InvokeOnError - Specifies that the completion routine is invoked when the
//         operation completes with an error status.
//
//     InvokeOnCancel - Specifies that the completion routine is invoked when the
//         operation is being canceled.
//
// Return Value:
//
//     None.
//
//--

#define IoSetCompletionRoutine( Irp, Routine, CompletionContext, Success, Error, Cancel ) { \
    PIO_STACK_LOCATION irpSp;                                               \
    ASSERT( (Success) | (Error) | (Cancel) ? (Routine) != NULL : TRUE );    \
    irpSp = IoGetNextIrpStackLocation( (Irp) );                             \
    irpSp->CompletionRoutine = (Routine);                                   \
    irpSp->Context = (CompletionContext);                                   \
    irpSp->Control = 0;                                                     \
    if ((Success)) { irpSp->Control = SL_INVOKE_ON_SUCCESS; }               \
    if ((Error)) { irpSp->Control |= SL_INVOKE_ON_ERROR; }                  \
    if ((Cancel)) { irpSp->Control |= SL_INVOKE_ON_CANCEL; } }

//++
//
// VOID
// IoSetNextIrpStackLocation (
//     IN OUT PIRP Irp
//     )
//
// Routine Description:
//
//     This routine is invoked to set the current IRP stack location to
//     the next stack location, i.e. it "pushes" the stack.
//
// Arguments:
//
//     Irp - Pointer to the I/O Request Packet (IRP).
//
// Return Value:
//
//     None.
//
//--

#define IoSetNextIrpStackLocation( Irp ) {      \
    (Irp)->CurrentLocation--;                   \
    (Irp)->Tail.Overlay.CurrentStackLocation--; }

//++
//
// VOID
// IoCopyCurrentIrpStackLocationToNext(
//     IN PIRP Irp
//     )
//
// Routine Description:
//
//     This routine is invoked to copy the IRP stack arguments and file
//     pointer from the current IrpStackLocation to the next
//     in an I/O Request Packet (IRP).
//
//     If the caller wants to call IoCallDriver with a completion routine
//     but does not wish to change the arguments otherwise,
//     the caller first calls IoCopyCurrentIrpStackLocationToNext,
//     then IoSetCompletionRoutine, then IoCallDriver.
//
// Arguments:
//
//     Irp - Pointer to the I/O Request Packet.
//
// Return Value:
//
//     None.
//
//--

#define IoCopyCurrentIrpStackLocationToNext( Irp ) { \
    PIO_STACK_LOCATION irpSp; \
    PIO_STACK_LOCATION nextIrpSp; \
    irpSp = IoGetCurrentIrpStackLocation( (Irp) ); \
    nextIrpSp = IoGetNextIrpStackLocation( (Irp) ); \
    RtlCopyMemory( nextIrpSp, irpSp, FIELD_OFFSET(IO_STACK_LOCATION, CompletionRoutine)); \
    nextIrpSp->Control = 0; }

//++
//
// VOID
// IoSkipCurrentIrpStackLocation (
//     IN PIRP Irp
//     )
//
// Routine Description:
//
//     This routine is invoked to increment the current stack location of
//     a given IRP.
//
//     If the caller wishes to call the next driver in a stack, and does not
//     wish to change the arguments, nor do they wish to set a completion
//     routine, then the caller first calls IoSkipCurrentIrpStackLocation
//     and the calls IoCallDriver.
//
// Arguments:
//
//     Irp - Pointer to the I/O Request Packet.
//
// Return Value:
//
//     None
//
//--

#define IoSkipCurrentIrpStackLocation( Irp ) \
    (Irp)->CurrentLocation++; \
    (Irp)->Tail.Overlay.CurrentStackLocation++;

//++
//
// USHORT
// IoSizeOfIrp(
//     IN CCHAR StackSize
//     )
//
// Routine Description:
//
//     Determines the size of an IRP given the number of stack locations
//     the IRP will have.
//
// Arguments:
//
//     StackSize - Number of stack locations for the IRP.
//
// Return Value:
//
//     Size in bytes of the IRP.
//
//--

#define IoSizeOfIrp( StackSize ) \
    ((USHORT) (sizeof( IRP ) + ((StackSize) * (sizeof( IO_STACK_LOCATION )))))

//++
//
// VOID
// IoMarkIrpPending(
//     IN OUT PIRP Irp
//     )
//
// Routine Description:
//
//     This routine marks the specified I/O Request Packet (IRP) to indicate
//     that an initial status of STATUS_PENDING was returned to the caller.
//     This is used so that I/O completion can determine whether or not to
//     fully complete the I/O operation requested by the packet.
//
// Arguments:
//
//     Irp - Pointer to the I/O Request Packet to be marked pending.
//
// Return Value:
//
//     None.
//
//--

#define IoMarkIrpPending( Irp ) ( \
    IoGetCurrentIrpStackLocation( (Irp) )->Control |= SL_PENDING_RETURNED )

VOID
IoStartNextPacket(
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN Cancelable
    );

VOID
IoStartNextPacketByKey(
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN Cancelable,
    IN ULONG Key
    );

VOID
IoStartPacket(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PULONG Key OPTIONAL,
    IN PDRIVER_CANCEL CancelFunction OPTIONAL
    );

#if defined(DBG) || defined(DEBUG)

#define KdPrint(_x_)   DbgPrint _x_
#define KdBreakPoint() DebugBreak()

#else

#define KdPrint(_x_)
#define KdBreakPoint()

#endif

ULONG
DbgPrint(
  IN PCHAR DebugMessage,
  ...
);


VOID
KeFlushIoBuffers (
    IN PMDL Mdl,
    IN BOOLEAN ReadOperation,
    IN BOOLEAN DmaOperation
    );

VOID
KeInitializeDeviceQueue (
    IN PKDEVICE_QUEUE DeviceQueue
    );

BOOLEAN
KeInsertDeviceQueue (
    IN PKDEVICE_QUEUE DeviceQueue,
    IN PKDEVICE_QUEUE_ENTRY DeviceQueueEntry
    );

BOOLEAN
KeInsertByKeyDeviceQueue (
    IN PKDEVICE_QUEUE DeviceQueue,
    IN PKDEVICE_QUEUE_ENTRY DeviceQueueEntry,
    IN ULONG SortKey
    );

PKDEVICE_QUEUE_ENTRY
KeRemoveDeviceQueue (
    IN PKDEVICE_QUEUE DeviceQueue
    );

PKDEVICE_QUEUE_ENTRY
KeRemoveByKeyDeviceQueue (
    IN PKDEVICE_QUEUE DeviceQueue,
    IN ULONG SortKey
    );

BOOLEAN
KeRemoveEntryDeviceQueue (
    IN PKDEVICE_QUEUE DeviceQueue,
    IN PKDEVICE_QUEUE_ENTRY DeviceQueueEntry
    );


typedef CRITICAL_SECTION KSPIN_LOCK, *PKSPIN_LOCK;

#define KeAcquireSpinLockAtDpcLevel(lock)   EnterCriticalSection(lock)
// Add IRQL to avoid unreferenced param errors
#define KeAcquireSpinLock(lock,oldirql)     EnterCriticalSection(lock); oldirql
#define KeInitializeSpinLock(lock)          InitializeCriticalSection(lock)
#define KeReleaseSpinLock(lock,newirql)     LeaveCriticalSection(lock); newirql
#define KeReleaseSpinLockFromDpcLevel(lock) LeaveCriticalSection(lock)
#define KeFreeSpinLock(lock)                DeleteCriticalSection(lock)

#define KeRaiseIrql(newirql,oldirql)        newirql; oldirql
#define KeLowerIrql(newirql)                newirql

__inline VOID KeQueryTickCount(PLARGE_INTEGER val) {
    val->QuadPart = GetTickCount();
}

// Returns # of 100ns units that elapse with each tick.  CE tick count is in ms
__inline ULONG KeQueryTimeIncrement(VOID) {
    return (10000);
}


// NT's new InterlockedExchangePointer() takes care of 32 bit/64 bit pointers.
// whereas the old InterlockedExchange() only work on 32 bit pointers.
// 
__inline PVOID InterlockedExchangePointer( PVOID *Target, PVOID Value ) {
	return (PVOID) InterlockedExchange( (LPLONG)Target, (LONG)Value );
}

VOID
MmBuildMdlForNonPagedPool (
    IN OUT PMDL MemoryDescriptorList
    );

PMDL
MmCreateMdl(
    IN PMDL MemoryDescriptorList OPTIONAL,
    IN PVOID Base,
    IN ULONG Length
    );

//++
//
// PVOID
// MmGetMdlVirtualAddress (
//     IN PMDL Mdl
//     )
//
// Routine Description:
//
//     The MmGetMdlVirtualAddress returns the virual address of the buffer
//     described by the Mdl.
//
// Arguments:
//
//     Mdl - Pointer to an MDL.
//
// Return Value:
//
//     Returns the virtual address of the buffer described by the Mdl
//
//--

#define MmGetMdlVirtualAddress(Mdl)                                     \
    ((PVOID) ((PCHAR) (((Mdl)->MdlFlags & MDL_64_BIT_VA) ?              \
    (PVOID) ((ULONG) (Mdl)->StartVa << PAGE_SHIFT) : (Mdl)->StartVa) +  \
    (Mdl)->ByteOffset))

//++
//
// ULONG
// MmGetMdlByteCount (
//     IN PMDL Mdl
//     )
//
// Routine Description:
//
//     The MmGetMdlByteCount returns the length in bytes of the buffer
//     described by the Mdl.
//
// Arguments:
//
//     Mdl - Pointer to an MDL.
//
// Return Value:
//
//     Returns the byte count of the buffer described by the Mdl
//
//--

#define MmGetMdlByteCount(Mdl)  ((Mdl)->ByteCount)

//++
//
// ULONG
// MmGetMdlByteOffset (
//     IN PMDL Mdl
//     )
//
// Routine Description:
//
//     The MmGetMdlByteOffset returns the byte offset within the page
//     of the buffer described by the Mdl.
//
// Arguments:
//
//     Mdl - Pointer to an MDL.
//
// Return Value:
//
//     Returns the byte offset within the page of the buffer described by the Mdl
//
//--

#define MmGetMdlByteOffset(Mdl)  ((Mdl)->ByteOffset)

//++
//
// PVOID
// MmGetMdlStartVa (
//     IN PMDL Mdl
//     )
//
// Routine Description:
//
//     The MmGetMdlBaseVa returns the virtual address of the buffer
//     described by the Mdl rounded down to the nearest page.
//
// Arguments:
//
//     Mdl - Pointer to an MDL.
//
// Return Value:
//
//     Returns the returns the starting virtual address of the MDL.
//
//
//--

#define MmGetMdlBaseVa(Mdl)  (((Mdl)->MdlFlags & MDL_64_BIT_VA) ?       \
    (PVOID) (((ULONG) (Mdl)->StartVa) << PAGE_SHIFT) : (Mdl)->StartVa)

//++
//
// VOID
// MmInitializeMdl (
//     IN PMDL MemoryDescriptorList,
//     IN PVOID BaseVa,
//     IN ULONG Length
//     )
//
// Routine Description:
//
//     This routine initializes the header of a Memory Descriptor List (MDL).
//
// Arguments:
//
//     MemoryDescriptorList - Pointer to the MDL to initialize.
//
//     BaseVa - Base virtual address mapped by the MDL.
//
//     Length - Length, in bytes, of the buffer mapped by the MDL.
//
// Return Value:
//
//     None.
//
//--

#define MmInitializeMdl(MemoryDescriptorList, BaseVa, Length) { \
    (MemoryDescriptorList)->Next = (PMDL) NULL; \
    (MemoryDescriptorList)->MdlFlags = 0; \
    (MemoryDescriptorList)->StartVa = (PVOID) PAGE_ALIGN((BaseVa)); \
    (MemoryDescriptorList)->ByteOffset = BYTE_OFFSET((BaseVa)); \
    (MemoryDescriptorList)->ByteCount = (Length); \
    }

//#define MmInitializeMdl(MemoryDescriptorList, BaseVa, Length) { \
//    (MemoryDescriptorList)->Next = (PMDL) NULL; \
//    (MemoryDescriptorList)->Size = (CSHORT)(sizeof(MDL) +  \
//            (sizeof(ULONG) * ADDRESS_AND_SIZE_TO_SPAN_PAGES((BaseVa), (Length)))); \
//    (MemoryDescriptorList)->MdlFlags = 0; \
//    (MemoryDescriptorList)->StartVa = (PVOID) PAGE_ALIGN((BaseVa)); \
//    (MemoryDescriptorList)->ByteOffset = BYTE_OFFSET((BaseVa)); \
//    (MemoryDescriptorList)->ByteCount = (Length); \
//    }


//++
//
// PVOID
// MmGetSystemAddressForMdl (
//     IN PMDL MDL
//     )
//
// Routine Description:
//
//     This routine returns the mapped address of an MDL, if the
//     Mdl is not already mapped or a system address, it is mapped.
//
// Arguments:
//
//     MemoryDescriptorList - Pointer to the MDL to map.
//
// Return Value:
//
//     Returns the base address where the pages are mapped.  The base address
//     has the same offset as the virtual address in the MDL.
//
//--

//#define MmGetSystemAddressForMdl(MDL)
//     (((MDL)->MdlFlags & (MDL_MAPPED_TO_SYSTEM_VA)) ?
//                             ((MDL)->MappedSystemVa) :
//                ((((MDL)->MdlFlags & (MDL_SOURCE_IS_NONPAGED_POOL)) ?
//                      ((PVOID)((ULONG)(MDL)->StartVa | (MDL)->ByteOffset)) :
//                            (MmMapLockedPages((MDL),KernelMode)))))

#define MmGetSystemAddressForMdl(MDL)                                  \
     (((MDL)->MdlFlags & (MDL_MAPPED_TO_SYSTEM_VA |                    \
                        MDL_SOURCE_IS_NONPAGED_POOL)) ?                \
                             ((MDL)->MappedSystemVa) :                 \
                             (MmMapLockedPages((MDL),KernelMode)))

PVOID
MmMapLockedPages (
    IN PMDL MemoryDescriptorList,
    IN KPROCESSOR_MODE AccessMode
    );

//++
//
// VOID
// MmPrepareMdlForReuse (
//     IN PMDL MDL
//     )
//
// Routine Description:
//
//     This routine will take all of the steps necessary to allow an MDL to be
//     re-used.
//
// Arguments:
//
//     MemoryDescriptorList - Pointer to the MDL that will be re-used.
//
// Return Value:
//
//     None.
//
//--

#define MmPrepareMdlForReuse(MDL)                                       \
    if (((MDL)->MdlFlags & MDL_PARTIAL_HAS_BEEN_MAPPED) != 0) {         \
        ASSERT(((MDL)->MdlFlags & MDL_PARTIAL) != 0);                   \
        MmUnmapLockedPages( (MDL)->MappedSystemVa, (MDL) );             \
    } else if (((MDL)->MdlFlags & MDL_PARTIAL) == 0) {                  \
        ASSERT(((MDL)->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA) == 0);       \
    }

VOID
MmProbeAndLockPages (
    IN OUT PMDL MemoryDescriptorList,
    IN KPROCESSOR_MODE AccessMode,
    IN LOCK_OPERATION Operation
    );

VOID
MmUnlockPages (
    IN PMDL MemoryDescriptorList
    );

VOID
MmUnmapLockedPages (
    IN PVOID BaseAddress,
    IN PMDL MemoryDescriptorList
    );

NTSTATUS
ObDereferenceObject(
    IN PVOID Object
    );


//
//  Doubly-linked list manipulation routines.  Implemented as macros
//  but logically these are procedures.
//

//
//  VOID
//  InitializeListHead(
//      PLIST_ENTRY ListHead
//      );
//

#define InitializeListHead(ListHead) (\
    (ListHead)->Flink = (ListHead)->Blink = (ListHead))

//
//  BOOLEAN
//  IsListEmpty(
//      PLIST_ENTRY ListHead
//      );
//

#define IsListEmpty(ListHead) \
    ((ListHead)->Flink == (ListHead))

//
//  PLIST_ENTRY
//  RemoveHeadList(
//      PLIST_ENTRY ListHead
//      );
//

#define RemoveHeadList(ListHead) \
    (ListHead)->Flink;\
    {RemoveEntryList((ListHead)->Flink)}

//
//  PLIST_ENTRY
//  RemoveTailList(
//      PLIST_ENTRY ListHead
//      );
//

#define RemoveTailList(ListHead) \
    (ListHead)->Blink;\
    {RemoveEntryList((ListHead)->Blink)}

//
//  VOID
//  RemoveEntryList(
//      PLIST_ENTRY Entry
//      );
//

#define RemoveEntryList(Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_Flink;\
    _EX_Flink = (Entry)->Flink;\
    _EX_Blink = (Entry)->Blink;\
    _EX_Blink->Flink = _EX_Flink;\
    _EX_Flink->Blink = _EX_Blink;\
    }

//
//  VOID
//  InsertTailList(
//      PLIST_ENTRY ListHead,
//      PLIST_ENTRY Entry
//      );
//

#define InsertTailList(ListHead,Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_ListHead;\
    _EX_ListHead = (ListHead);\
    _EX_Blink = _EX_ListHead->Blink;\
    (Entry)->Flink = _EX_ListHead;\
    (Entry)->Blink = _EX_Blink;\
    _EX_Blink->Flink = (Entry);\
    _EX_ListHead->Blink = (Entry);\
    }

//
//  VOID
//  InsertHeadList(
//      PLIST_ENTRY ListHead,
//      PLIST_ENTRY Entry
//      );
//

#define InsertHeadList(ListHead,Entry) {\
    PLIST_ENTRY _EX_Flink;\
    PLIST_ENTRY _EX_ListHead;\
    _EX_ListHead = (ListHead);\
    _EX_Flink = _EX_ListHead->Flink;\
    (Entry)->Flink = _EX_Flink;\
    (Entry)->Blink = _EX_ListHead;\
    _EX_Flink->Blink = (Entry);\
    _EX_ListHead->Flink = (Entry);\
    }

//
//
//  PSINGLE_LIST_ENTRY
//  PopEntryList(
//      PSINGLE_LIST_ENTRY ListHead
//      );
//

#define PopEntryList(ListHead) \
    (ListHead)->Next;\
    {\
        PSINGLE_LIST_ENTRY FirstEntry;\
        FirstEntry = (ListHead)->Next;\
        if (FirstEntry != NULL) {     \
            (ListHead)->Next = FirstEntry->Next;\
        }                             \
    }


//
//  VOID
//  PushEntryList(
//      PSINGLE_LIST_ENTRY ListHead,
//      PSINGLE_LIST_ENTRY Entry
//      );
//

#define PushEntryList(ListHead,Entry) \
    (Entry)->Next = (ListHead)->Next; \
    (ListHead)->Next = (Entry)


PVOID
PnPLoadWDMDriver(
    IN LPCWSTR pDriverName,
    IN PDEVICE_OBJECT pPDO,
    IN LPWSTR pszRegPath,   
    OUT PDRIVER_OBJECT *ppDriverObject);

NTSTATUS
PnPUnLoadWDMDriver(
    IN PVOID pContext);


VOID
RtlInitUnicodeString(
    PUNICODE_STRING DestinationString,
    PCWSTR SourceString
    );

NTSTATUS
RtlAppendUnicodeToString(
    IN PUNICODE_STRING Destination,
    IN PWSTR Source OPTIONAL
);

NTSTATUS 
RtlIntegerToUnicodeString(
    IN ULONG  Value,
    IN ULONG  Base OPTIONAL,
    IN OUT PUNICODE_STRING  String
    );

NTSTATUS
RtlAppendUnicodeStringToString(
    IN PUNICODE_STRING Destination,
    IN PUNICODE_STRING Source
    );

VOID
RtlCopyUnicodeString(
    OUT PUNICODE_STRING DestinationString,
    IN PUNICODE_STRING SourceString OPTIONAL
    );

#define RtlMoveMemory(Destination,Source,Length) memmove((Destination),(Source),(Length))
#define RtlCopyMemory(Destination,Source,Length) memcpy((Destination),(Source),(Length))
#define RtlFillMemory(Destination,Length,Fill) memset((Destination),(Fill),(Length))
#define RtlZeroMemory(Destination,Length) memset((Destination),0,(Length))


//  BitMap routines.  The following structure, routines, and macros are
//  for manipulating bitmaps.  The user is responsible for allocating a bitmap
//  structure (which is really a header) and a buffer (which must be longword
//  aligned and multiple longwords in size).
//

typedef struct _RTL_BITMAP {
    ULONG SizeOfBitMap;                     // Number of bits in bit map
    PULONG Buffer;                          // Pointer to the bit map itself
} RTL_BITMAP;
typedef RTL_BITMAP *PRTL_BITMAP;


//
//  The following routine initializes a new bitmap.  It does not alter the
//  data currently in the bitmap.  This routine must be called before
//  any other bitmap routine/macro.
//

VOID
RtlInitializeBitMap (
    PRTL_BITMAP BitMapHeader,
    PULONG BitMapBuffer,
    ULONG SizeOfBitMap
    );

//
//  The following two routines either clear or set all of the bits
//  in a bitmap.
//

VOID
RtlClearAllBits (
    PRTL_BITMAP BitMapHeader
    );

VOID
RtlSetAllBits (
    PRTL_BITMAP BitMapHeader
    );

//
//  The following two routines locate a contiguous region of either
//  clear or set bits within the bitmap.  The region will be at least
//  as large as the number specified, and the search of the bitmap will
//  begin at the specified hint index (which is a bit index within the
//  bitmap, zero based).  The return value is the bit index of the located
//  region (zero based) or -1 (i.e., 0xffffffff) if such a region cannot
//  be located
//

ULONG
RtlFindClearBits (
    PRTL_BITMAP BitMapHeader,
    ULONG NumberToFind,
    ULONG HintIndex
    );

ULONG
RtlFindSetBits (
    PRTL_BITMAP BitMapHeader,
    ULONG NumberToFind,
    ULONG HintIndex
    );

//
//  The following two routines locate a contiguous region of either
//  clear or set bits within the bitmap and either set or clear the bits
//  within the located region.  The region will be as large as the number
//  specified, and the search for the region will begin at the specified
//  hint index (which is a bit index within the bitmap, zero based).  The
//  return value is the bit index of the located region (zero based) or
//  -1 (i.e., 0xffffffff) if such a region cannot be located.  If a region
//  cannot be located then the setting/clearing of the bitmap is not performed.
//

ULONG
RtlFindClearBitsAndSet (
    PRTL_BITMAP BitMapHeader,
    ULONG NumberToFind,
    ULONG HintIndex
    );

ULONG
RtlFindSetBitsAndClear (
    PRTL_BITMAP BitMapHeader,
    ULONG NumberToFind,
    ULONG HintIndex
    );

//
//  The following two routines clear or set bits within a specified region
//  of the bitmap.  The starting index is zero based.
//

VOID
RtlClearBits (
    PRTL_BITMAP BitMapHeader,
    ULONG StartingIndex,
    ULONG NumberToClear
    );

VOID
RtlSetBits (
    PRTL_BITMAP BitMapHeader,
    ULONG StartingIndex,
    ULONG NumberToSet
    );

//
//  The following routine locates a set of contiguous regions of clear
//  bits within the bitmap.  The caller specifies whether to return the
//  longest runs or just the first found lcoated.  The following structure is
//  used to denote a contiguous run of bits.  The two routines return an array
//  of this structure, one for each run located.
//

typedef struct _RTL_BITMAP_RUN {

    ULONG StartingIndex;
    ULONG NumberOfBits;

} RTL_BITMAP_RUN;
typedef RTL_BITMAP_RUN *PRTL_BITMAP_RUN;

ULONG
RtlFindClearRuns (
    PRTL_BITMAP BitMapHeader,
    PRTL_BITMAP_RUN RunArray,
    ULONG SizeOfRunArray,
    BOOLEAN LocateLongestRuns
    );

//
//  The following routine locates the longest contiguous region of
//  clear bits within the bitmap.  The returned starting index value
//  denotes the first contiguous region located satisfying our requirements
//  The return value is the length (in bits) of the longest region found.
//

ULONG
RtlFindLongestRunClear (
    PRTL_BITMAP BitMapHeader,
    PULONG StartingIndex
    );

//
//  The following routine locates the first contiguous region of
//  clear bits within the bitmap.  The returned starting index value
//  denotes the first contiguous region located satisfying our requirements
//  The return value is the length (in bits) of the region found.
//

ULONG
RtlFindFirstRunClear (
    PRTL_BITMAP BitMapHeader,
    PULONG StartingIndex
    );

//
//  The following macro returns the value of the bit stored within the
//  bitmap at the specified location.  If the bit is set a value of 1 is
//  returned otherwise a value of 0 is returned.
//
//      ULONG
//      RtlCheckBit (
//          PRTL_BITMAP BitMapHeader,
//          ULONG BitPosition
//          );
//
//
//  To implement CheckBit the macro retrieves the longword containing the
//  bit in question, shifts the longword to get the bit in question into the
//  low order bit position and masks out all other bits.
//

#define RtlCheckBit(BMH,BP) ((((BMH)->Buffer[(BP) / 32]) >> ((BP) % 32)) & 0x1)

//
//  The following two procedures return to the caller the total number of
//  clear or set bits within the specified bitmap.
//

ULONG
RtlNumberOfClearBits (
    PRTL_BITMAP BitMapHeader
    );

ULONG
RtlNumberOfSetBits (
    PRTL_BITMAP BitMapHeader
    );

//
//  The following two procedures return to the caller a boolean value
//  indicating if the specified range of bits are all clear or set.
//

BOOLEAN
RtlAreBitsClear (
    PRTL_BITMAP BitMapHeader,
    ULONG StartingIndex,
    ULONG Length
    );

BOOLEAN
RtlAreBitsSet (
    PRTL_BITMAP BitMapHeader,
    ULONG StartingIndex,
    ULONG Length
    );

ULONG
RtlFindNextForwardRunClear (
    IN PRTL_BITMAP BitMapHeader,
    IN ULONG FromIndex,
    IN PULONG StartingRunIndex
    );

ULONG
RtlFindLastBackwardRunClear (
    IN PRTL_BITMAP BitMapHeader,
    IN ULONG FromIndex,
    IN PULONG StartingRunIndex
    );

//
//  The following two procedures return to the caller a value indicating
//  the position within a ULONGLONG of the most or least significant non-zero
//  bit.  A value of zero results in a return value of -1.
//

CCHAR
RtlFindLeastSignificantBit (
    IN ULONGLONG Set
    );

CCHAR
RtlFindMostSignificantBit (
    IN ULONGLONG Set
    );

//  Define the splay links and the associated manipuliation macros and
//  routines.  Note that the splay_links should be an opaque type.
//  Routine are provided to traverse and manipulate the structure.
//
typedef struct _RTL_SPLAY_LINKS {
    struct _RTL_SPLAY_LINKS *Parent;
    struct _RTL_SPLAY_LINKS *LeftChild;
    struct _RTL_SPLAY_LINKS *RightChild;
} RTL_SPLAY_LINKS;
typedef RTL_SPLAY_LINKS *PRTL_SPLAY_LINKS;


//
//  The macro procedure InitializeSplayLinks takes as input a pointer to
//  splay link and initializes its substructure.  All splay link nodes must
//  be initialized before they are used in the different splay routines and
//  macros.
//
//  VOID
//  RtlInitializeSplayLinks (
//      PRTL_SPLAY_LINKS Links
//      );
//

#define RtlInitializeSplayLinks(Links) {    \
    PRTL_SPLAY_LINKS _SplayLinks;            \
    _SplayLinks = (PRTL_SPLAY_LINKS)(Links); \
    _SplayLinks->Parent = _SplayLinks;   \
    _SplayLinks->LeftChild = NULL;       \
    _SplayLinks->RightChild = NULL;      \
    }

//
//  The macro function Parent takes as input a pointer to a splay link in a
//  tree and returns a pointer to the splay link of the parent of the input
//  node.  If the input node is the root of the tree the return value is
//  equal to the input value.
//
//  PRTL_SPLAY_LINKS
//  RtlParent (
//      PRTL_SPLAY_LINKS Links
//      );
//

#define RtlParent(Links) (           \
    (PRTL_SPLAY_LINKS)(Links)->Parent \
    )

//
//  The macro function LeftChild takes as input a pointer to a splay link in
//  a tree and returns a pointer to the splay link of the left child of the
//  input node.  If the left child does not exist, the return value is NULL.
//
//  PRTL_SPLAY_LINKS
//  RtlLeftChild (
//      PRTL_SPLAY_LINKS Links
//      );
//

#define RtlLeftChild(Links) (           \
    (PRTL_SPLAY_LINKS)(Links)->LeftChild \
    )

//
//  The macro function RightChild takes as input a pointer to a splay link
//  in a tree and returns a pointer to the splay link of the right child of
//  the input node.  If the right child does not exist, the return value is
//  NULL.
//
//  PRTL_SPLAY_LINKS
//  RtlRightChild (
//      PRTL_SPLAY_LINKS Links
//      );
//

#define RtlRightChild(Links) (           \
    (PRTL_SPLAY_LINKS)(Links)->RightChild \
    )

//
//  The macro function IsRoot takes as input a pointer to a splay link
//  in a tree and returns TRUE if the input node is the root of the tree,
//  otherwise it returns FALSE.
//
//  BOOLEAN
//  RtlIsRoot (
//      PRTL_SPLAY_LINKS Links
//      );
//

#define RtlIsRoot(Links) (                          \
    (RtlParent(Links) == (PRTL_SPLAY_LINKS)(Links)) \
    )

//
//  The macro function IsLeftChild takes as input a pointer to a splay link
//  in a tree and returns TRUE if the input node is the left child of its
//  parent, otherwise it returns FALSE.
//
//  BOOLEAN
//  RtlIsLeftChild (
//      PRTL_SPLAY_LINKS Links
//      );
//

#define RtlIsLeftChild(Links) (                                   \
    (RtlLeftChild(RtlParent(Links)) == (PRTL_SPLAY_LINKS)(Links)) \
    )

//
//  The macro function IsRightChild takes as input a pointer to a splay link
//  in a tree and returns TRUE if the input node is the right child of its
//  parent, otherwise it returns FALSE.
//
//  BOOLEAN
//  RtlIsRightChild (
//      PRTL_SPLAY_LINKS Links
//      );
//

#define RtlIsRightChild(Links) (                                   \
    (RtlRightChild(RtlParent(Links)) == (PRTL_SPLAY_LINKS)(Links)) \
    )

//
//  The macro procedure InsertAsLeftChild takes as input a pointer to a splay
//  link in a tree and a pointer to a node not in a tree.  It inserts the
//  second node as the left child of the first node.  The first node must not
//  already have a left child, and the second node must not already have a
//  parent.
//
//  VOID
//  RtlInsertAsLeftChild (
//      PRTL_SPLAY_LINKS ParentLinks,
//      PRTL_SPLAY_LINKS ChildLinks
//      );
//

#define RtlInsertAsLeftChild(ParentLinks,ChildLinks) { \
    PRTL_SPLAY_LINKS _SplayParent;                      \
    PRTL_SPLAY_LINKS _SplayChild;                       \
    _SplayParent = (PRTL_SPLAY_LINKS)(ParentLinks);     \
    _SplayChild = (PRTL_SPLAY_LINKS)(ChildLinks);       \
    _SplayParent->LeftChild = _SplayChild;          \
    _SplayChild->Parent = _SplayParent;             \
    }

//
//  The macro procedure InsertAsRightChild takes as input a pointer to a splay
//  link in a tree and a pointer to a node not in a tree.  It inserts the
//  second node as the right child of the first node.  The first node must not
//  already have a right child, and the second node must not already have a
//  parent.
//
//  VOID
//  RtlInsertAsRightChild (
//      PRTL_SPLAY_LINKS ParentLinks,
//      PRTL_SPLAY_LINKS ChildLinks
//      );
//

#define RtlInsertAsRightChild(ParentLinks,ChildLinks) { \
    PRTL_SPLAY_LINKS _SplayParent;                       \
    PRTL_SPLAY_LINKS _SplayChild;                        \
    _SplayParent = (PRTL_SPLAY_LINKS)(ParentLinks);      \
    _SplayChild = (PRTL_SPLAY_LINKS)(ChildLinks);        \
    _SplayParent->RightChild = _SplayChild;          \
    _SplayChild->Parent = _SplayParent;              \
    }

//
//  The Splay function takes as input a pointer to a splay link in a tree
//  and splays the tree.  Its function return value is a pointer to the
//  root of the splayed tree.
//

PRTL_SPLAY_LINKS
RtlSplay (
    PRTL_SPLAY_LINKS Links
    );

//
//  The Delete function takes as input a pointer to a splay link in a tree
//  and deletes that node from the tree.  Its function return value is a
//  pointer to the root of the tree.  If the tree is now empty, the return
//  value is NULL.
//

PRTL_SPLAY_LINKS
RtlDelete (
    PRTL_SPLAY_LINKS Links
    );

//
//  The DeleteNoSplay function takes as input a pointer to a splay link in a tree,
//  the caller's pointer to the root of the tree and deletes that node from the
//  tree.  Upon return the caller's pointer to the root node will correctly point
//  at the root of the tree.
//
//  It operationally differs from RtlDelete only in that it will not splay the tree.
//

VOID
RtlDeleteNoSplay (
    PRTL_SPLAY_LINKS Links,
    PRTL_SPLAY_LINKS *Root
    );

//
//  The SubtreeSuccessor function takes as input a pointer to a splay link
//  in a tree and returns a pointer to the successor of the input node of
//  the substree rooted at the input node.  If there is not a successor, the
//  return value is NULL.
//

PRTL_SPLAY_LINKS
RtlSubtreeSuccessor (
    PRTL_SPLAY_LINKS Links
    );

//
//  The SubtreePredecessor function takes as input a pointer to a splay link
//  in a tree and returns a pointer to the predecessor of the input node of
//  the substree rooted at the input node.  If there is not a predecessor,
//  the return value is NULL.
//

PRTL_SPLAY_LINKS
RtlSubtreePredecessor (
    PRTL_SPLAY_LINKS Links
    );

//
//  The RealSuccessor function takes as input a pointer to a splay link
//  in a tree and returns a pointer to the successor of the input node within
//  the entire tree.  If there is not a successor, the return value is NULL.
//

PRTL_SPLAY_LINKS
RtlRealSuccessor (
    PRTL_SPLAY_LINKS Links
    );

//
//  The RealPredecessor function takes as input a pointer to a splay link
//  in a tree and returns a pointer to the predecessor of the input node
//  within the entire tree.  If there is not a predecessor, the return value
//  is NULL.
//

PRTL_SPLAY_LINKS
RtlRealPredecessor (
    PRTL_SPLAY_LINKS Links
    );

//
// Byte swap routines.  These are used to convert from little-endian to
// big-endian and vice-versa.
//

USHORT
FASTCALL
RtlUshortByteSwap(
    IN USHORT Source
    );

ULONG
FASTCALL
RtlUlongByteSwap(
    IN ULONG Source
    );

ULONGLONG
FASTCALL
RtlUlonglongByteSwap(
    IN ULONGLONG Source
    );

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _WDMDDK_
