/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

Module Name:  

ntcompat.h

Abstract:

Compatiblity defines for CE NDIS drivers

Notes: 


--*/
#ifndef _NTCOMPAT_H_
#define _NTCOMPAT_H_

#include <windows.h>
#include <ceddk.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef CONST char *PCSZ;

typedef char CCHAR, *PCCHAR;
typedef short CSHORT, *PCSHORT;
typedef ULONG CLONG, *PCLONG;

// wdm.h also includes these defs
#ifndef _WDMDDK_
typedef struct _STRING {
          USHORT        Length;
          USHORT        MaximumLength;
#ifdef MIDL_PASS
          [size_is(MaximumLength), length_is(Length)]
#endif
          PCHAR         Buffer;
        } STRING, *PSTRING;

typedef struct _CSTRING {
          USHORT        Length;
          USHORT        MaximumLength;
          CONST char    *Buffer;
        } CSTRING, *PCSTRING;

typedef struct _UNICODE_STRING {
          USHORT        Length;
          USHORT        MaximumLength;
#ifdef MIDL_PASS
          [size_is(MaximumLength / 2), length_is((Length) / 2)] USHORT *Buffer;
#else
          PWSTR         Buffer;
#endif
        } UNICODE_STRING, *PUNICODE_STRING;

typedef STRING ANSI_STRING, *PANSI_STRING;
typedef STRING OEM_STRING, *POEM_STRING;
typedef STRING CANSI_STRING;
typedef PSTRING PCANSI_STRING;

typedef LARGE_INTEGER PHYSICAL_ADDRESS, *PPHYSICAL_ADDRESS;

/* @@@ until we have it for real */
typedef VOID MDL, *PMDL;


/* DMA transfer widths */
typedef enum _DMA_WIDTH {
          Width8Bits,
          Width16Bits,
          Width32Bits,
          MaximumDmaWidth
        } DMA_WIDTH, *PDMA_WIDTH;

/* DMA transfer speeds */
typedef enum _DMA_SPEED {
          Compatible,
          TypeA,
          TypeB,
          TypeC,
          MaximumDmaSpeed
        } DMA_SPEED, *PDMA_SPEED;

#endif // _WDMDDK_

/* resources */
/* bit masks for Flags when type is CmResourceTypeInterrupt */
#define CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE   0
#define CM_RESOURCE_INTERRUPT_LATCHED           1

/* bit masks for Flags when type is CmResourceTypeMemory */
#define CM_RESOURCE_MEMORY_READ_WRITE           0x0000
#define CM_RESOURCE_MEMORY_READ_ONLY            0x0001
#define CM_RESOURCE_MEMORY_WRITE_ONLY           0x0002
#define CM_RESOURCE_MEMORY_PREFETCHABLE         0x0004
#define CM_RESOURCE_MEMORY_COMBINEDWRITE        0x0008
#define CM_RESOURCE_MEMORY_24                   0x0010

/* bit masks for Flags when type is CmResourceTypePort */
#define CM_RESOURCE_PORT_MEMORY                 0
#define CM_RESOURCE_PORT_IO                     1

/* bit masks for Flags when type is CmResourceTypeDma */
#define CM_RESOURCE_DMA_8                       0x0000
#define CM_RESOURCE_DMA_16                      0x0001
#define CM_RESOURCE_DMA_32                      0x0002

/* resource options */
#define IO_RESOURCE_PREFERRED                   0x01
#define IO_RESOURCE_DEFAULT                     0x02
#define IO_RESOURCE_ALTERNATIVE                 0x08

typedef enum _CM_RESOURCE_TYPE {
          CmResourceTypeNull = 0,
          CmResourceTypePort,
          CmResourceTypeInterrupt,
          CmResourceTypeMemory,
          CmResourceTypeDma,
          CmResourceTypeDeviceSpecific,
          CmResourceTypeMaximum
        } CM_RESOURCE_TYPE;

typedef enum _CM_SHARE_DISPOSITION {
          CmResourceShareUndetermined = 0,
          CmResourceShareDeviceExclusive,
          CmResourceShareDriverExclusive,
          CmResourceShareShared
        } CM_SHARE_DISPOSITION;

typedef struct _CM_PARTIAL_RESOURCE_DESCRIPTOR {
          UCHAR Type;
          UCHAR ShareDisposition;
          USHORT Flags;
          union {
            struct {
              PHYSICAL_ADDRESS  Start;
              ULONG             Length;
            } Port;

            struct {
              ULONG             Level;
              ULONG             Vector;
              ULONG             Affinity;
            } Interrupt;

            struct {
              PHYSICAL_ADDRESS  Start;
              ULONG             Length;
            } Memory;

            struct {
              ULONG             Channel;
              ULONG             Port;
              ULONG             Reserved1;
            } Dma;

            struct {
              ULONG             DataSize;
              ULONG             Reserved1;
              ULONG             Reserved2;
            } DeviceSpecificData;
          } u;
        } CM_PARTIAL_RESOURCE_DESCRIPTOR, *PCM_PARTIAL_RESOURCE_DESCRIPTOR;

typedef struct _CM_PARTIAL_RESOURCE_LIST {
          USHORT                Version;
          USHORT                Revision;
          ULONG                 Count;
          CM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptors[1];
        } CM_PARTIAL_RESOURCE_LIST, *PCM_PARTIAL_RESOURCE_LIST;

typedef struct _CM_FULL_RESOURCE_DESCRIPTOR {
          INTERFACE_TYPE        InterfaceType;
          ULONG                 BusNumber;
          CM_PARTIAL_RESOURCE_LIST PartialResourceList;
        } CM_FULL_RESOURCE_DESCRIPTOR, *PCM_FULL_RESOURCE_DESCRIPTOR;

typedef struct _CM_RESOURCE_LIST {
          ULONG                 Count;
          CM_FULL_RESOURCE_DESCRIPTOR List[1];
        } CM_RESOURCE_LIST, *PCM_RESOURCE_LIST;

typedef struct _IO_RESOURCE_DESCRIPTOR {
          UCHAR                 Option;
          UCHAR                 Type;
          UCHAR                 ShareDisposition;
          UCHAR                 Spare1;
          USHORT                Flags;
          USHORT                Spare2;

          union {
            struct {
              ULONG             Length;
              ULONG             Alignment;
              PHYSICAL_ADDRESS  MinimumAddress;
              PHYSICAL_ADDRESS  MaximumAddress;
            } Port;

            struct {
              ULONG             Length;
              ULONG             Alignment;
              PHYSICAL_ADDRESS  MinimumAddress;
              PHYSICAL_ADDRESS  MaximumAddress;
            } Memory;

            struct {
              ULONG             MinimumVector;
              ULONG             MaximumVector;
            } Interrupt;

            struct {
              ULONG             MinimumChannel;
              ULONG             MaximumChannel;
            } Dma;

        } u;
      } IO_RESOURCE_DESCRIPTOR, *PIO_RESOURCE_DESCRIPTOR;

typedef struct _IO_RESOURCE_LIST {
          USHORT                Version;
          USHORT                Revision;
          ULONG                 Count;
          IO_RESOURCE_DESCRIPTOR Descriptors[1];
        } IO_RESOURCE_LIST, *PIO_RESOURCE_LIST;

typedef struct _IO_RESOURCE_REQUIREMENTS_LIST {
          ULONG                 ListSize;
          INTERFACE_TYPE        InterfaceType;
          ULONG                 BusNumber;
          ULONG                 SlotNumber;
          ULONG                 Reserved[3];
          ULONG                 AlternativeLists;
          IO_RESOURCE_LIST      List[1];
        } IO_RESOURCE_REQUIREMENTS_LIST, *PIO_RESOURCE_REQUIREMENTS_LIST;

#ifndef _WDMDDK_
typedef struct _DRIVER_OBJECT {
          HANDLE                ObjectHandle;
          UNICODE_STRING        DriverName;
          UNICODE_STRING        RegistryPath;
          UNICODE_STRING        ImagePath;
        } DRIVER_OBJECT, *PDRIVER_OBJECT;

typedef struct _DEVICE_OBJECT {
          CSHORT                Type;
          USHORT                Size;
          LONG                  ReferenceCount;
          PDRIVER_OBJECT        DriverObject;
          struct _DEVICE_OBJECT *NextDevice;
          struct _DEVICE_OBJECT *AttachedDevice;
        } DEVICE_OBJECT, *PDEVICE_OBJECT;
#endif

typedef
  VOID
  (*PWORKER_THREAD_ROUTINE)(
    IN PVOID Parameter
  );

typedef struct _WORK_QUEUE_ITEM {
          LIST_ENTRY    List;
          PWORKER_THREAD_ROUTINE WorkerRoutine;
          PVOID         Parameter;
        } WORK_QUEUE_ITEM, *PWORK_QUEUE_ITEM;

#define UNICODE_NULL ((WCHAR)0)

#define STATUS_BUFFER_OVERFLOW          ((NTSTATUS)0x80000005L)
#define STATUS_BUFFER_TOO_SMALL         ((NTSTATUS)0xC0000023L)
#define STATUS_INSUFFICIENT_RESOURCES   ((NTSTATUS)0xC000009AL)
#define STATUS_NOT_SUPPORTED            ((NTSTATUS)0xC00000BBL)
#define STATUS_REQUEST_NOT_ACCEPTED     ((NTSTATUS)0xC00000D0L)

#if defined(BINARY_COMPATIBLE) && !defined(PopEntryList)
/* singly-linked list manipulation routines */
/*++
PSINGLE_LIST_ENTRY
PopEntryList(
  PSINGLE_LIST_ENTRY ListHead
)
--*/
#define PopEntryList(_ListHead) \
          (_ListHead)->Next; \
          { \
            PSINGLE_LIST_ENTRY _FirstEntry = (_ListHead)->Next; \
            if (_FirstEntry != NULL) \
              (_ListHead)->Next = _FirstEntry->Next; \
          }

/*++
VOID
PushEntryList(
  PSINGLE_LIST_ENTRY ListHead,
  PSINGLE_LIST_ENTRY Entry
)
--*/
#define PushEntryList(_ListHead, _Entry) \
        { \
          (_Entry)->Next = (_ListHead)->Next; \
          (_ListHead)->Next = _Entry; \
        }
#endif

#if defined(BINARY_COMPATIBLE) && !defined(InitializeListHead)
/* doubly-linked list manipulation routines */
/*++
VOID
InitializeListHead(
  PLIST_ENTRY ListHead
)
--*/
#define InitializeListHead(_ListHead) \
          ((_ListHead)->Flink = (_ListHead)->Blink = _ListHead)

/*++
BOOLEAN
IsListEmpty(
  PLIST_ENTRY ListHead
)
--*/
#define IsListEmpty(_ListHead)   ((_ListHead)->Flink == _ListHead)

/*++
PLIST_ENTRY
RemoveHeadList(
  PLIST_ENTRY ListHead
)
--*/
#define RemoveHeadList(_ListHead) \
          (_ListHead)->Flink; \
          RemoveEntryList((_ListHead)->Flink)

/*++
PLIST_ENTRY
RemoveTailList(
  PLIST_ENTRY ListHead
)
--*/
#define RemoveTailList(_ListHead) \
          (_ListHead)->Blink; \
          RemoveEntryList((_ListHead)->Blink)

/*++
VOID
RemoveEntryList(
  PLIST_ENTRY Entry
)
--*/
#define RemoveEntryList(_Entry) \
        { \
          PLIST_ENTRY _EX_Flink = (_Entry)->Flink; \
          PLIST_ENTRY _EX_Blink = (_Entry)->Blink; \
          _EX_Blink->Flink = _EX_Flink; \
          _EX_Flink->Blink = _EX_Blink; \
        }

/*++
VOID
InsertTailList(
  PLIST_ENTRY ListHead,
  PLIST_ENTRY Entry
)
--*/
#define InsertTailList(_ListHead, _Entry) \
        { \
          PLIST_ENTRY _EX_ListHead = _ListHead; \
          PLIST_ENTRY _EX_Blink = _EX_ListHead->Blink; \
          (_Entry)->Flink = _EX_ListHead; \
          (_Entry)->Blink = _EX_Blink; \
          _EX_Blink->Flink = _Entry; \
          _EX_ListHead->Blink = _Entry; \
        }

/*++
VOID
InsertHeadList(
  PLIST_ENTRY ListHead,
  PLIST_ENTRY Entry
)
--*/
#define InsertHeadList(_ListHead, _Entry) \
        { \
          PLIST_ENTRY _EX_ListHead = _ListHead; \
          PLIST_ENTRY _EX_Flink = _EX_ListHead->Flink; \
          (_Entry)->Flink = _EX_Flink; \
          (_Entry)->Blink = _EX_ListHead; \
          _EX_Flink->Blink = _Entry; \
          _EX_ListHead->Flink = _Entry; \
        }
#endif

NTSTATUS
HalAssignSlotResources( // not in ceddk.h
  IN PUNICODE_STRING RegistryPath,
  IN PUNICODE_STRING DriverClassName OPTIONAL,
  IN PDRIVER_OBJECT DriverObject,
  IN PDEVICE_OBJECT DeviceObject OPTIONAL,
  IN INTERFACE_TYPE BusType,
  IN ULONG BusNumber,
  IN ULONG SlotNumber,
  IN OUT PCM_RESOURCE_LIST *AllocatedResources
);

/* io.c */
NTSTATUS
IoAssignResources( // not in ceddk.h
  IN PUNICODE_STRING RegistryPath,
  IN PUNICODE_STRING DriverClassName OPTIONAL,
  IN PDRIVER_OBJECT DriverObject,
  IN PDEVICE_OBJECT DeviceObject OPTIONAL,
  IN PIO_RESOURCE_REQUIREMENTS_LIST RequestedResources,
  IN OUT PCM_RESOURCE_LIST *AllocatedResources
);

/* ke.c */
VOID
KeStallExecutionProcessor(
  ULONG MicroSecondsToStall
);

#define KeFlushIoBuffers(_Mdl, _ReadOperation, _DmaOperation) CacheSync(CACHE_SYNC_DISCARD)

/* debug.c */
VOID DbgBreakPoint(VOID);

/* this is a standard NT DDK function
   and should not be defined anywhere else */
#if defined(BINARY_COMPATIBLE) && defined(DbgPrint)
#undef DbgPrint
#endif

#if !defined(DbgPrint)
ULONG
DbgPrint(
  IN PCHAR DebugMessage,
  ...
);
#endif

/* rtl.c */
VOID
RtlInitString(
  OUT PSTRING DestinationString,
  IN PCSZ SourceString OPTIONAL
);

VOID
RtlInitAnsiString(
  OUT PANSI_STRING DestinationString,
  IN PCSZ SourceString OPTIONAL
);

VOID
RtlInitUnicodeString(
  OUT PUNICODE_STRING DestinationString,
  IN PCWSTR SourceString OPTIONAL
);

VOID
RtlFreeUnicodeString(
  IN PUNICODE_STRING UnicodeString
);

NTSTATUS
RtlAnsiStringToUnicodeString(
  OUT PUNICODE_STRING DestinationString,
  IN PANSI_STRING SourceString,
  IN BOOLEAN AllocateDestinationString
);

NTSTATUS
RtlUnicodeStringToAnsiString(
  IN OUT PANSI_STRING DestinationString,
  IN PUNICODE_STRING SourceString,
  IN BOOLEAN AllocateDestinationString
);

NTSTATUS
RtlUpcaseUnicodeString(
  OUT PUNICODE_STRING DestinationString,
  IN PUNICODE_STRING SourceString,
  IN BOOLEAN AllocateDestinationString
);

BOOLEAN
RtlEqualUnicodeString(
  IN PUNICODE_STRING String1,
  IN PUNICODE_STRING String2,
  IN BOOLEAN CaseInSensitive
);

VOID
RtlCopyUnicodeString(
  OUT PUNICODE_STRING DestinationString,
  IN PUNICODE_STRING SourceString OPTIONAL
);

NTSTATUS
RtlAppendUnicodeToString(
  IN PUNICODE_STRING Destination,
  IN PWSTR Source OPTIONAL
);

NTSTATUS
RtlAppendUnicodeStringToString(
  IN PUNICODE_STRING Destination,
  IN PUNICODE_STRING Source
);

#if defined(x86)
#define HalGetDmaAlignmentRequirement() ((ULONG)1)
#endif /* x86 */

#if defined(MIPS)
ULONG
HalGetDmaAlignmentRequirement( // not in ceddk.h
  VOID
);
#endif /* MIPS */


#if defined(PPC) || defined(ARM)
// Note these values are intentioally incorrect
ULONG
HalGetDmaAlignmentRequirement(
  VOID
);
#endif /* PPC */


#if defined(SHx)
ULONG
HalGetDmaAlignmentRequirement(
  VOID
);
#endif /* SHx */

#ifdef __cplusplus
}
#endif

#endif
