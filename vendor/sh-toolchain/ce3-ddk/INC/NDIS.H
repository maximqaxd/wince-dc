/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

Module Name:  

ndis.h

Abstract:

  This file defines the structures, macros, and functions available to NDIS
  4.0 and above miniport drivers under CE.

Notes: 


--*/

#ifndef _NDIS_H_
#define _NDIS_H_

#ifdef SHx
#pragma warning(disable:4710)
#endif

#if DEBUG
#define DBG     1
#else
#undef DBG
#endif

#ifndef NO_STRICT
#ifndef STRICT
#define STRICT  1
#endif
#endif

#include <windows.h>

#include <memory.h>
#include <ntcompat.h>
#include <ntddndis.h>
#include <netevent.h>

#if !defined(BINARY_COMPATIBLE)
#if defined(NDIS_MINIPORT_DRIVER)
#define BINARY_COMPATIBLE       1
#else
#define BINARY_COMPATIBLE       0
#endif
#endif

#define NDISAPI WINAPI

#define EXPORT

#define NDIS_INIT_FUNCTION(_F) alloc_text(INIT,_F)
#define NDIS_PAGABLE_FUNCTION(_F) alloc_text(PAGE,_F)
#define NDIS_PAGEABLE_FUNCTION(_F) alloc_text(PAGE,_F)

typedef struct _NDIS_MAC_BLOCK  NDIS_MAC_BLOCK, *PNDIS_MAC_BLOCK;
typedef struct _NDIS_PACKET NDIS_PACKET, *PNDIS_PACKET, **PPNDIS_PACKET;
typedef struct _NDIS_WAN_PACKET NDIS_WAN_PACKET, *PNDIS_WAN_PACKET;
typedef struct _NDIS_MINIPORT_BLOCK NDIS_MINIPORT_BLOCK, *PNDIS_MINIPORT_BLOCK;
typedef struct _NDIS_M_DRIVER_BLOCK NDIS_M_DRIVER_BLOCK, *PNDIS_M_DRIVER_BLOCK;
typedef struct _NDIS_OPEN_BLOCK NDIS_OPEN_BLOCK, *PNDIS_OPEN_BLOCK;
typedef struct _NDIS_M_OPEN_BLOCK NDIS_M_OPEN_BLOCK, *PNDIS_M_OPEN_BLOCK;
typedef struct _NDIS_MINIPORT_INTERRUPT
          NDIS_MINIPORT_INTERRUPT, *PNDIS_MINIPORT_INTERRUPT;

/* KSPIN_LOCK is already typedef-ed in winnt.h */
#define KSPIN_LOCK  CRITICAL_SECTION
typedef KSPIN_LOCK *PKSPIN_LOCK;

typedef
  BOOLEAN
  (NDISAPI *PFN_SYNCHRONIZE_ROUTINE)(
    IN PVOID SynchronizeContext
  );

typedef
  VOID
  (NDISAPI *PFN_NDIS_DEFERRED_ROUTINE)(
    IN PVOID DeferredContext
  );

#if (!defined(MIDL_PASS) || defined(__midl)) && (!defined(_M_IX86) || (defined(_INTEGRAL_MAX_BITS) && _INTEGRAL_MAX_BITS >= 64))
typedef __int64 LONGLONG;
typedef unsigned __int64 ULONGLONG;
#define MAXLONGLONG             (0x7fffffffffffffff)
#else
typedef double LONGLONG;
typedef double ULONGLONG;
#endif

typedef LONGLONG *PLONGLONG;
typedef ULONGLONG *PULONGLONG;

typedef ANSI_STRING NDIS_ANSI_STRING, *PNDIS_ANSI_STRING;
typedef UNICODE_STRING NDIS_STRING, *PNDIS_STRING;
typedef PVOID NDIS_HANDLE, *PNDIS_HANDLE;
typedef NTSTATUS NDIS_STATUS, *PNDIS_STATUS;

typedef enum _KINTERRUPT_MODE {
          LevelSensitive,
          Latched
        } KINTERRUPT_MODE;
#define NdisInterruptLatched            Latched
#define NdisInterruptLevelSensitive     LevelSensitive
typedef KINTERRUPT_MODE NDIS_INTERRUPT_MODE, *PNDIS_INTERRUPT_MODE;

typedef struct _NDIS_BUFFER {
          struct _NDIS_BUFFER *Next;
          PVOID   VirtualAddress;
          ULONG   BufferLength;
        } NDIS_BUFFER, *PNDIS_BUFFER;
typedef struct _NDIS_DPC {
          SHORT                 Type;
          UCHAR                 Number;
          UCHAR                 Importance;
          LIST_ENTRY            DpcListEntry;
          PFN_NDIS_DEFERRED_ROUTINE DeferredRoutine;
          PVOID                 DeferredContext;
          PVOID                 SystemArgument1;
          PVOID                 SystemArgument2;
          PULONG                Lock;
        } NDIS_DPC, *PNDIS_DPC;

typedef struct _NDIS_SPIN_LOCK {
          KSPIN_LOCK    SpinLock;
        } NDIS_SPIN_LOCK, *PNDIS_SPIN_LOCK;

typedef struct _REFERENCE {
          NDIS_SPIN_LOCK        SpinLock;
          USHORT                ReferenceCount;
          BOOLEAN               Closing;
        } REFERENCE, *PREFERENCE;

typedef struct _MAP_REGISTER_ENTRY {
          PVOID         MapRegister;
          BOOLEAN       WriteToDevice;
        } MAP_REGISTER_ENTRY, *PMAP_REGISTER_ENTRY;

typedef PHYSICAL_ADDRESS NDIS_PHYSICAL_ADDRESS, *PNDIS_PHYSICAL_ADDRESS;
typedef struct _NDIS_PHYSICAL_ADDRESS_UNIT {
          NDIS_PHYSICAL_ADDRESS PhysicalAddress;
          UINT                  Length;
        } NDIS_PHYSICAL_ADDRESS_UNIT, *PNDIS_PHYSICAL_ADDRESS_UNIT;

typedef struct _NDIS_PACKET_POOL {
          NDIS_SPIN_LOCK        SpinLock;
          PNDIS_PACKET          FreeList;
          UINT                  PacketLength;
          UCHAR                 Buffer[1];
        } NDIS_PACKET_POOL, *PNDIS_PACKET_POOL;

typedef struct _NDIS_PACKET_PRIVATE {
          UINT                  PhysicalCount;  /* number of physical pages */
          UINT                  TotalLength;    /* amount of data */
          PNDIS_BUFFER          Head;           /* first buffer in the chain */
          PNDIS_BUFFER          Tail;           /* last buffer in the chain */
                                                /* if Head is NULL the chain
                                                 * is empty; Tail doesn't have
                                                 * to be NULL also */
          PNDIS_PACKET_POOL     Pool;
          UINT                  Count;
          ULONG                 Flags;          // See fPACKET_xxx bits below
          BOOLEAN               ValidCounts;
          UCHAR                 NdisPacketFlags;
          USHORT                NdisPacketOobOffset;
        } NDIS_PACKET_PRIVATE, *PNDIS_PACKET_PRIVATE;

struct _NDIS_PACKET {
  NDIS_PACKET_PRIVATE Private;
  union {
    struct {
      UCHAR     MiniportReserved[8];
      UCHAR     WrapperReserved[8];
    };
    struct {
      UCHAR     MacReserved[16];
    };
  };
  UCHAR         ProtocolReserved[1];
};

typedef struct _NDIS_PACKET_PRIVATE_EXTENSION {
          LIST_ENTRY    Link;
          PNDIS_PACKET  Packet;
          ULONG         Reserved;       /* make the structure quad aligned */
        } NDIS_PACKET_PRIVATE_EXTENSION, *PNDIS_PACKET_PRIVATE_EXTENSION;

typedef enum _NDIS_INTERFACE_TYPE {
          NdisInterfaceInternal = Internal,
          NdisInterfaceIsa = Isa,
          NdisInterfaceEisa = Eisa,
          NdisInterfaceMca = MicroChannel,
          NdisInterfaceTurboChannel = TurboChannel,
          NdisInterfacePci = PCIBus,
          NdisInterfacePcMcia = PCMCIABus
        } NDIS_INTERFACE_TYPE, *PNDIS_INTERFACE_TYPE;

typedef CM_PARTIAL_RESOURCE_LIST NDIS_RESOURCE_LIST, *PNDIS_RESOURCE_LIST;

typedef enum _NDIS_REQUEST_TYPE {
          NdisRequestQueryInformation,
          NdisRequestSetInformation,
          NdisRequestQueryStatistics,
          NdisRequestOpen,              /* obsolete */
          NdisRequestClose,             /* obsolete */
          NdisRequestSend,              /* obsolete */
          NdisRequestTransferData,      /* obsolete */
          NdisRequestReset,             /* obsolete */
          NdisRequestGeneric1,
          NdisRequestGeneric2,
          NdisRequestGeneric3,
          NdisRequestGeneric4
        } NDIS_REQUEST_TYPE, *PNDIS_REQUEST_TYPE;

typedef struct _NDIS_REQUEST {
          UCHAR                 MacReserved[16];
          NDIS_REQUEST_TYPE     RequestType;
          union _DATA {
            struct _QUERY_INFORMATION {
              NDIS_OID  Oid;
              PVOID     InformationBuffer;
              UINT      InformationBufferLength;
              UINT      BytesWritten;
              UINT      BytesNeeded;
            } QUERY_INFORMATION;
            struct _SET_INFORMATION {
              NDIS_OID  Oid;
              PVOID     InformationBuffer;
              UINT      InformationBufferLength;
              UINT      BytesRead;
              UINT      BytesNeeded;
            } SET_INFORMATION;
          } DATA;
		} NDIS_REQUEST, *PNDIS_REQUEST;

typedef enum _NDIS_WORK_ITEM_TYPE {
          NdisWorkItemDpc,
          NdisWorkItemResetRequested,
          NdisWorkItemRequest,
          NdisWorkItemSend,
          NdisWorkItemHalt,
          NdisWorkItemSendLoopback,
          NdisWorkItemResetInProgress,
          NdisWorkItemTimer,
          NdisWorkItemPendingOpen,
          NdisWorkItemMiniportCallback,
          NdisMaxWorkItems
        } NDIS_WORK_ITEM_TYPE, *PNDIS_WORK_ITEM_TYPE;

typedef enum _NDIS_PARAMETER_TYPE {
          NdisParameterInteger,
          NdisParameterHexInteger,
          NdisParameterString,
          NdisParameterMultiString
        } NDIS_PARAMETER_TYPE, *PNDIS_PARAMETER_TYPE;

typedef struct _NDIS_CONFIGURATION_PARAMETER {
          NDIS_PARAMETER_TYPE ParameterType;
          union {
            ULONG IntegerData;
            NDIS_STRING StringData;
          } ParameterData;
        } NDIS_CONFIGURATION_PARAMETER, *PNDIS_CONFIGURATION_PARAMETER;

typedef enum _NDIS_PROCESSOR_TYPE {
          NdisProcessorX86,
          NdisProcessorMips,
          NdisProcessorAlpha,
          NdisProcessorPpc
        } NDIS_PROCESSOR_TYPE, *PNDIS_PROCESSOR_TYPE;

typedef enum _NDIS_ENVIRONMENT_TYPE {
          NdisEnvironmentWindows,
          NdisEnvironmentWindowsNt,
          NdisEnvironmentWindowsCe
        } NDIS_ENVIRONMENT_TYPE, *PNDIS_ENVIRONMENT_TYPE;

typedef struct _KEVENT {
          HANDLE        Event;
        } KEVENT, *PKEVENT;

typedef struct _KINTERRUPT *PKINTERRUPT;

typedef struct _KTIMER {
          LIST_ENTRY          TimerListEntry;
          PNDIS_DPC           Dpc;
          ULARGE_INTEGER      DueTime;
          LONG                Period;
        } KTIMER, *PKTIMER;

typedef struct _NDIS_PACKET_OOB_DATA {
          union {
            ULONGLONG   TimeToSend;
            ULONGLONG   TimeSent;
          };
          ULONGLONG     TimeReceived;
          UINT          HeaderSize;
          UINT          SizeMediaSpecificInfo;
          PVOID         MediaSpecificInformation;
          NDIS_STATUS   Status;
        } NDIS_PACKET_OOB_DATA, *PNDIS_PACKET_OOB_DATA;

#define NUMBER_OF_WORK_ITEM_TYPES       NdisMaxWorkItems
#define NUMBER_OF_SINGLE_WORK_ITEMS     6
#define NDIS_M_MAX_MULTI_LIST           32

/* minimum size of packets a miniport must allocate when it indicates packets
 * via NdisMIndicatePacket or NdisMCoIndicatePacket */
#define PROTOCOL_RESERVED_SIZE_IN_PACKET        16

#include <afilter.h>
#include <efilter.h>
#include <tfilter.h>
#include <ffilter.h>

typedef
  VOID
  (*PFN_FILTER_PACKET_INDICATION_HANDLER)(
    IN NDIS_HANDLE Miniport,
    IN PPNDIS_PACKET PacketArray,
    IN UINT NumberOfPackets
  );

typedef
  VOID
  (NDISAPI *ADAPTER_SHUTDOWN_HANDLER)(
    IN PVOID ShutdownContext
  );

typedef
  VOID
  (NDISAPI *PNDIS_TIMER_FUNCTION)(
    IN PVOID SystemSpecific1,
    IN PVOID FunctionContext,
    IN PVOID SystemSpecific2,
    IN PVOID SystemSpecific3
  );

typedef
  VOID
  (NDISAPI *NDIS_M_SEND_COMPLETE_HANDLER)(
    IN NDIS_HANDLE MiniportAdapterHandle,
    IN PNDIS_PACKET Packet,
    IN NDIS_STATUS Status
  );

typedef
  VOID
  (NDISAPI *NDIS_M_WAN_SEND_COMPLETE_HANDLER)(
    IN NDIS_HANDLE MiniportAdapterHandle,
    IN PNDIS_WAN_PACKET Packet,
    IN NDIS_STATUS Status
  );


typedef
  VOID
  (*NDIS_M_SEND_RESOURCES_HANDLER)(
    IN NDIS_HANDLE MiniportAdapterHandle
  );

typedef
  VOID
  (*NDIS_M_RESET_COMPLETE_HANDLER)(
    IN NDIS_HANDLE MiniportAdapterHandle,
    IN NDIS_STATUS Status,
    IN BOOLEAN AddressingReset
  );

typedef
  VOID
  (*NDIS_M_PROCESS_DEFERRED)(
    IN PNDIS_MINIPORT_BLOCK Miniport
  );

typedef
  NDIS_STATUS
  (*NDIS_M_QUEUE_WORK_ITEM)(
    IN PNDIS_MINIPORT_BLOCK Miniport,
    IN NDIS_WORK_ITEM_TYPE WorkItemType,
    IN PVOID WorkItemContext1,
    IN PVOID WorkItemContext2
  );

typedef
  NDIS_STATUS
  (*NDIS_M_QUEUE_NEW_WORK_ITEM)(
    IN PNDIS_MINIPORT_BLOCK Miniport,
    IN NDIS_WORK_ITEM_TYPE WorkItemType,
    IN PVOID WorkItemContext1,
    IN PVOID WorkItemContext2
  );

typedef
  VOID
  (*NDIS_M_DEQUEUE_WORK_ITEM)(
    IN PNDIS_MINIPORT_BLOCK Miniport,
    IN  NDIS_WORK_ITEM_TYPE WorkItemType,
    OUT PVOID *WorkItemContext1,
    OUT PVOID *WorkItemContext2
  );

/* function types for NDIS_MINIPORT_CHARACTERISTICS */
typedef
  VOID
  (NDISAPI *RECEIVE_COMPLETE_HANDLER)(
    IN NDIS_HANDLE ProtocolBindingContext
  );

typedef
  BOOLEAN
  (NDISAPI *W_CHECK_FOR_HANG_HANDLER)(
    IN NDIS_HANDLE MiniportAdapterContext
  );

typedef
  VOID
  (NDISAPI *W_DISABLE_INTERRUPT_HANDLER)(
    IN NDIS_HANDLE MiniportAdapterContext
  );

typedef
  VOID
  (NDISAPI *W_ENABLE_INTERRUPT_HANDLER)(
    IN NDIS_HANDLE MiniportAdapterContext
  );

typedef
  VOID
  (NDISAPI *W_HALT_HANDLER)(
    IN NDIS_HANDLE MiniportAdapterContext
  );

typedef
  VOID
  (NDISAPI *W_HANDLE_INTERRUPT_HANDLER)(
    IN NDIS_HANDLE MiniportAdapterContext
  );

typedef
  NDIS_STATUS
  (NDISAPI *W_INITIALIZE_HANDLER)(
    OUT PNDIS_STATUS OpenErrorStatus,
    OUT PUINT SelectedMediumIndex,
    IN PNDIS_MEDIUM MediumArray,
    IN UINT MediumArraySize,
    IN NDIS_HANDLE MiniportAdapterContext,
    IN NDIS_HANDLE WrapperConfigurationContext
  );

typedef
  VOID
  (NDISAPI *W_ISR_HANDLER)(
    OUT PBOOLEAN InterruptRecognized,
    OUT PBOOLEAN QueueMiniportHandleInterrupt,
    IN NDIS_HANDLE MiniportAdapterContext
  );

typedef
  NDIS_STATUS
  (NDISAPI *W_QUERY_INFORMATION_HANDLER)(
    IN NDIS_HANDLE MiniportAdapterContext,
    IN NDIS_OID Oid,
    IN PVOID InformationBuffer,
    IN ULONG InformationBufferLength,
    OUT PULONG BytesWritten,
    OUT PULONG BytesNeeded
  );

typedef
  NDIS_STATUS
  (NDISAPI *W_RECONFIGURE_HANDLER)(
    OUT PNDIS_STATUS OpenErrorStatus,
    IN NDIS_HANDLE MiniportAdapterContext,
    IN NDIS_HANDLE WrapperConfigurationContext
  );

typedef
  NDIS_STATUS
  (NDISAPI *W_RESET_HANDLER)(
    OUT PBOOLEAN AddressingReset,
    IN NDIS_HANDLE MiniportAdapterContext
  );

typedef
  NDIS_STATUS
  (NDISAPI *W_SEND_HANDLER)(
    IN NDIS_HANDLE  MiniportAdapterContext,
    IN PNDIS_PACKET Packet,
    IN UINT Flags
  );

typedef
NDIS_STATUS
(*WM_SEND_HANDLER)(
    IN  NDIS_HANDLE             MiniportAdapterContext,
    IN  NDIS_HANDLE             NdisLinkHandle,
    IN  PNDIS_WAN_PACKET        Packet
    );

typedef
  NDIS_STATUS
  (NDISAPI *W_SET_INFORMATION_HANDLER)(
    IN NDIS_HANDLE MiniportAdapterContext,
    IN NDIS_OID Oid,
    IN PVOID InformationBuffer,
    IN ULONG InformationBufferLength,
    OUT PULONG BytesRead,
    OUT PULONG BytesNeeded
  );

typedef
  NDIS_STATUS
  (NDISAPI *W_TRANSFER_DATA_HANDLER)(
    OUT PNDIS_PACKET Packet,
    OUT PUINT BytesTransferred,
    IN NDIS_HANDLE MiniportAdapterContext,
    IN NDIS_HANDLE MiniportReceiveContext,
    IN UINT ByteOffset,
    IN UINT BytesToTransfer
  );

typedef
NDIS_STATUS
(*WM_TRANSFER_DATA_HANDLER)(
    VOID
    );

typedef
  VOID
  (NDISAPI *W_RETURN_PACKET_HANDLER)(
    IN NDIS_HANDLE MiniportAdapterContext,
    IN PNDIS_PACKET Packet
  );

typedef
  VOID
  (NDISAPI *W_SEND_PACKETS_HANDLER)(
    IN NDIS_HANDLE MiniportAdapterContext,
    IN PPNDIS_PACKET PacketArray,
    IN UINT NumberOfPackets
  );

typedef
  VOID
  (NDISAPI *W_ALLOCATE_COMPLETE_HANDLER)(
    IN NDIS_HANDLE MiniportAdapterContext,
    IN PVOID VirtualAddress,
    IN PNDIS_PHYSICAL_ADDRESS PhysicalAddress,
    IN ULONG Length,
    IN PVOID Context
  );

typedef struct _NDIS40_MINIPORT_CHARACTERISTICS {
          UCHAR                         MajorNdisVersion;
          UCHAR                         MinorNdisVersion;
          USHORT                        Filler;
          union
          {
              UINT                      Reserved;
              UINT                      Flags;
          };
          W_CHECK_FOR_HANG_HANDLER      CheckForHangHandler;
          W_DISABLE_INTERRUPT_HANDLER   DisableInterruptHandler;
          W_ENABLE_INTERRUPT_HANDLER    EnableInterruptHandler;
          W_HALT_HANDLER                HaltHandler;
          W_HANDLE_INTERRUPT_HANDLER    HandleInterruptHandler;
          W_INITIALIZE_HANDLER          InitializeHandler;
          W_ISR_HANDLER                 ISRHandler;
          W_QUERY_INFORMATION_HANDLER   QueryInformationHandler;
          W_RECONFIGURE_HANDLER         ReconfigureHandler;
          W_RESET_HANDLER               ResetHandler;
          union
          {
              W_SEND_HANDLER            SendHandler;
              WM_SEND_HANDLER           WanSendHandler;
          };
          W_SET_INFORMATION_HANDLER     SetInformationHandler;
          union
          {
              W_TRANSFER_DATA_HANDLER   TransferDataHandler;
              WM_TRANSFER_DATA_HANDLER  WanTransferDataHandler;
          };
          W_RETURN_PACKET_HANDLER       ReturnPacketHandler;
          W_SEND_PACKETS_HANDLER        SendPacketsHandler;
          W_ALLOCATE_COMPLETE_HANDLER   AllocateCompleteHandler;
        } NDIS40_MINIPORT_CHARACTERISTICS;

typedef NDIS40_MINIPORT_CHARACTERISTICS NDIS_MINIPORT_CHARACTERISTICS;
typedef NDIS_MINIPORT_CHARACTERISTICS *PNDIS_MINIPORT_CHARACTERISTICS;
typedef NDIS_MINIPORT_CHARACTERISTICS   NDIS_WAN_MINIPORT_CHARACTERISTICS;
typedef NDIS_WAN_MINIPORT_CHARACTERISTICS * PNDIS_MINIPORT_CHARACTERISTICS;

/* function types for NDIS_PROTOCOL_CHARACTERISTICS */
typedef
  VOID
  (NDISAPI *OPEN_ADAPTER_COMPLETE_HANDLER)(
    IN NDIS_HANDLE ProtocolBindingContext,
    IN NDIS_STATUS Status,
    IN NDIS_STATUS OpenErrorStatus
  );

typedef
  VOID
  (NDISAPI *CLOSE_ADAPTER_COMPLETE_HANDLER)(
    IN NDIS_HANDLE ProtocolBindingContext,
    IN NDIS_STATUS Status
  );

typedef
  VOID
  (NDISAPI *SEND_COMPLETE_HANDLER)(
    IN NDIS_HANDLE ProtocolBindingContext,
    IN PNDIS_PACKET Packet,
    IN NDIS_STATUS Status
  );

typedef
VOID
(*WAN_SEND_COMPLETE_HANDLER) (
    IN  NDIS_HANDLE             ProtocolBindingContext,
    IN  PNDIS_WAN_PACKET        Packet,
    IN  NDIS_STATUS             Status
    );

typedef
  VOID
  (NDISAPI *TRANSFER_DATA_COMPLETE_HANDLER)(
    IN NDIS_HANDLE ProtocolBindingContext,
    IN PNDIS_PACKET Packet,
    IN NDIS_STATUS Status,
    IN UINT BytesTransferred
  );

typedef
VOID
(*WAN_TRANSFER_DATA_COMPLETE_HANDLER)(
    VOID
    );

typedef
  VOID
  (NDISAPI *RESET_COMPLETE_HANDLER)(
    IN NDIS_HANDLE ProtocolBindingContext,
    IN NDIS_STATUS Status
  );

typedef
  VOID
  (NDISAPI *REQUEST_COMPLETE_HANDLER)(
    IN NDIS_HANDLE ProtocolBindingContext,
    IN PNDIS_REQUEST NdisRequest,
    IN NDIS_STATUS Status
  );

typedef
  NDIS_STATUS
  (NDISAPI *RECEIVE_HANDLER)(
    IN NDIS_HANDLE ProtocolBindingContext,
    IN NDIS_HANDLE MacReceiveContext,
    IN PVOID HeaderBuffer,
    IN UINT HeaderBufferSize,
    IN PVOID LookAheadBuffer,
    IN UINT LookaheadBufferSize,
    IN UINT PacketSize
  );

typedef
NDIS_STATUS
(*WAN_RECEIVE_HANDLER)(
    IN  NDIS_HANDLE             NdisLinkHandle,
    IN  PUCHAR                  Packet,
    IN  ULONG                   PacketSize
    );

typedef
  VOID
  (NDISAPI *RECEIVE_COMPLETE_HANDLER)(
    IN NDIS_HANDLE ProtocolBindingContext
  );

typedef
  VOID
  (NDISAPI *STATUS_HANDLER)(
    IN NDIS_HANDLE ProtocolBindingContext,
    IN NDIS_STATUS GeneralStatus,
    IN PVOID StatusBuffer,
    IN UINT StatusBufferSize
  );

typedef
  VOID
  (NDISAPI *STATUS_COMPLETE_HANDLER)(
    IN NDIS_HANDLE ProtocolBindingContext
  );

typedef
  INT
  (NDISAPI *RECEIVE_PACKET_HANDLER)(
    IN NDIS_HANDLE ProtocolBindingContext,
    IN PNDIS_PACKET Packet
  );

typedef
  VOID
  (NDISAPI *BIND_HANDLER)(
    OUT PNDIS_STATUS Status,
    IN NDIS_HANDLE BindContext,
    IN PNDIS_STRING DeviceName,
    IN PVOID SystemSpecific1,
    IN PVOID SystemSpecific2
  );

typedef
  VOID
  (NDISAPI *UNBIND_HANDLER)(
    OUT PNDIS_STATUS Status,
    IN NDIS_HANDLE ProtocolBindingContext,
    IN NDIS_HANDLE UnbindContext
  );

typedef
  VOID
  (NDISAPI *TRANSLATE_HANDLER)(
    OUT PNDIS_STATUS Status,
    IN NDIS_HANDLE ProtocolBindingContext,
    OUT PNET_PNP_ID IdList,
    IN ULONG IdListLength,
    OUT PULONG BytesReturned
  );

typedef
  VOID
  (NDISAPI *UNLOAD_PROTOCOL_HANDLER)(VOID);

typedef struct _NDIS40_PROTOCOL_CHARACTERISTICS {
          UCHAR                         MajorNdisVersion;
          UCHAR                         MinorNdisVersion;
          union
          {
              UINT                      Reserved;
              UINT                      Flags;
          };
          OPEN_ADAPTER_COMPLETE_HANDLER OpenAdapterCompleteHandler;
          CLOSE_ADAPTER_COMPLETE_HANDLER CloseAdapterCompleteHandler;
          union 
          {
              SEND_COMPLETE_HANDLER     SendCompleteHandler;
              WAN_SEND_COMPLETE_HANDLER WanSendCompleteHandler;
          };
          union
          {
           TRANSFER_DATA_COMPLETE_HANDLER TransferDataCompleteHandler;
           WAN_TRANSFER_DATA_COMPLETE_HANDLER WanTransferDataCompleteHandler;
          };
          RESET_COMPLETE_HANDLER        ResetCompleteHandler;
          REQUEST_COMPLETE_HANDLER      RequestCompleteHandler;
          union
          {
              RECEIVE_HANDLER           ReceiveHandler;
              WAN_RECEIVE_HANDLER       WanReceiveHandler;
          };
          RECEIVE_COMPLETE_HANDLER      ReceiveCompleteHandler;
          STATUS_HANDLER                StatusHandler;
          STATUS_COMPLETE_HANDLER       StatusCompleteHandler;
          NDIS_STRING                   Name;
          RECEIVE_PACKET_HANDLER        ReceivePacketHandler;
          BIND_HANDLER                  BindAdapterHandler;
          UNBIND_HANDLER                UnbindAdapterHandler;
          TRANSLATE_HANDLER             TranslateHandler;
          UNLOAD_PROTOCOL_HANDLER       UnloadHandler;
        } NDIS40_PROTOCOL_CHARACTERISTICS;

typedef NDIS40_PROTOCOL_CHARACTERISTICS NDIS_PROTOCOL_CHARACTERISTICS;
typedef NDIS_PROTOCOL_CHARACTERISTICS *PNDIS_PROTOCOL_CHARACTERISTICS;

/* full mac driver specific */
typedef struct _NDIS_TIMER {
          KTIMER Timer;
          NDIS_DPC Dpc;
        } NDIS_TIMER, *PNDIS_TIMER;

/* function types for NDIS_MAC_CHARACTERISTICS */
typedef
  NDIS_STATUS
  (*OPEN_ADAPTER_HANDLER)(
    OUT PNDIS_STATUS OpenErrorStatus,
    OUT NDIS_HANDLE *MacBindingHandle,
    OUT PUINT SelectedMediumIndex,
    IN PNDIS_MEDIUM MediumArray,
    IN UINT MediumArraySize,
    IN NDIS_HANDLE NdisBindingContext,
    IN NDIS_HANDLE MacAdapterContext,
    IN UINT OpenOptions,
    IN PSTRING AddressingInformation OPTIONAL
  );

typedef
  NDIS_STATUS
  (*CLOSE_ADAPTER_HANDLER)(
    IN NDIS_HANDLE MacBindingHandle
  );

typedef
  NDIS_STATUS
  (*SEND_HANDLER)(
    IN NDIS_HANDLE MacBindingHandle,
    IN PNDIS_PACKET Packet
  );

typedef
NDIS_STATUS
(*WAN_SEND_HANDLER)(
    IN  NDIS_HANDLE             MacBindingHandle,
    IN  NDIS_HANDLE             LinkHandle,
    IN  PVOID                   Packet
    );

typedef
  NDIS_STATUS
  (*TRANSFER_DATA_HANDLER)(
    IN NDIS_HANDLE MacBindingHandle,
    IN NDIS_HANDLE MacReceiveContext,
    IN UINT ByteOffset,
    IN UINT BytesToTransfer,
    OUT PNDIS_PACKET Packet,
    OUT PUINT BytesTransferred
  );

typedef
  NDIS_STATUS
  (*RESET_HANDLER)(
    IN NDIS_HANDLE MacBindingHandle
  );

typedef
  NDIS_STATUS
  (*REQUEST_HANDLER)(
    IN NDIS_HANDLE MacBindingHandle,
    IN PNDIS_REQUEST NdisRequest
  );

typedef
  NDIS_STATUS
  (*QUERY_GLOBAL_STATISTICS_HANDLER)(
    IN NDIS_HANDLE MacAdapterContext,
    IN PNDIS_REQUEST NdisRequest
  );

typedef
  VOID
  (*UNLOAD_MAC_HANDLER)(
    IN NDIS_HANDLE MacMacContext
  );

typedef
  NDIS_STATUS
  (*ADD_ADAPTER_HANDLER)(
    IN NDIS_HANDLE MacMacContext,
    IN NDIS_HANDLE WrapperConfigurationContext,
    IN PNDIS_STRING AdapterName
  );

typedef
  VOID
  (*REMOVE_ADAPTER_HANDLER)(
    IN NDIS_HANDLE MacAdapterContext
  );

typedef
  VOID
  (*SEND_PACKETS_HANDLER)(
    IN NDIS_HANDLE MiniportAdapterContext,
    IN PPNDIS_PACKET PacketArray,
    IN UINT NumberOfPackets
  );

typedef struct _NDIS_MAC_CHARACTERISTICS {
          UCHAR                 MajorNdisVersion;
          UCHAR                 MinorNdisVersion;
          UINT                  Reserved;
          OPEN_ADAPTER_HANDLER  OpenAdapterHandler;
          CLOSE_ADAPTER_HANDLER CloseAdapterHandler;
          SEND_HANDLER          SendHandler;
          TRANSFER_DATA_HANDLER TransferDataHandler;
          RESET_HANDLER         ResetHandler;
          REQUEST_HANDLER       RequestHandler;
          QUERY_GLOBAL_STATISTICS_HANDLER QueryGlobalStatisticsHandler;
          UNLOAD_MAC_HANDLER    UnloadMacHandler;
          ADD_ADAPTER_HANDLER   AddAdapterHandler;
          REMOVE_ADAPTER_HANDLER RemoveAdapterHandler;
          NDIS_STRING           Name;
        } NDIS_MAC_CHARACTERISTICS, *PNDIS_MAC_CHARACTERISTICS;

typedef NDIS_MAC_CHARACTERISTICS        NDIS_WAN_MAC_CHARACTERISTICS;
typedef NDIS_WAN_MAC_CHARACTERISTICS *  PNDIS_WAN_MAC_CHARACTERISTICS;

typedef struct _NDIS_WRAPPER_HANDLE {
          PDRIVER_OBJECT        NdisWrapperDriver;
          HANDLE                NdisWrapperConfigurationHandle;
        } NDIS_WRAPPER_HANDLE, *PNDIS_WRAPPER_HANDLE;

struct _NDIS_MAC_BLOCK {
  NDIS_HANDLE                   AdapterQueue;
  NDIS_HANDLE                   MacMacContext;
  REFERENCE                     Ref;
  UINT                          Length;
  NDIS_MAC_CHARACTERISTICS      MacCharacteristics;
  PNDIS_WRAPPER_HANDLE          NdisMacInfo;
  PNDIS_MAC_BLOCK               NextMac;
  KEVENT                        AdaptersRemovedEvent;
  BOOLEAN                       Unloading;
  PCM_RESOURCE_LIST             PciAssignedResources;
  UNICODE_STRING                BaseName;
};

struct _NDIS_MINIPORT_BLOCK {
  ULONG                 NullValue;      /* to distinguish between MAC and
                                           miniport */
  PDEVICE_OBJECT        DeviceObject;   /* created by the wrapper */
  PNDIS_M_DRIVER_BLOCK  DriverHandle;   /* pointer to the driver block */
  NDIS_HANDLE           MiniportAdapterContext; /* miniport context */
  NDIS_STRING           MiniportName;   /* how miniport refers to us */
  PNDIS_M_OPEN_BLOCK    OpenQueue;      /* queue of opens for this miniport */
  PNDIS_MINIPORT_BLOCK  NextMiniport;   /* used by driver's MiniportQueue */
  REFERENCE             Ref;            /* contains spinlock for OpenQueue */
  BOOLEAN               padding1;       /* normal ints:  DO NOT REMOVE! */
  BOOLEAN               padding2;       /* processing def: DO NOT REMOVE! */

  //
  // Synchronization stuff.
  //
  // The boolean is used to lock out several DPCs from running at the
  // same time. The difficultly is if DPC A releases the spin lock
  // and DPC B tries to run, we want to defer B until after A has
  // exited.
  //
  BOOLEAN               LockAcquired;  // EXPOSED via macros. Do not move
  UCHAR                 PmodeOpens;    // Count of opens which turned on pmode/all_local

  NDIS_SPIN_LOCK        Lock;

  PNDIS_MINIPORT_INTERRUPT      Interrupt;

  ULONG                 Flags;      // flags to keep track of the miniport's state.

  //
  //Work that the miniport needs to do.
  //
  KSPIN_LOCK            WorkLock;
  SINGLE_LIST_ENTRY     WorkQueue[NUMBER_OF_WORK_ITEM_TYPES];
  SINGLE_LIST_ENTRY     WorkItemFreeQueue;

  //
  // Stuff that got deferred.
  //
  NDIS_DPC              Dpc;
  NDIS_TIMER            WakeUpDpcTimer;

  //
  // Holds media specific information.
  //
  PETH_FILTER           EthDB;    // EXPOSED via macros. Do not move
  PTR_FILTER            TrDB;    // EXPOSED via macros. Do not move
  PFDDI_FILTER          FddiDB;    // EXPOSED via macros. Do not move
  PARC_FILTER           ArcDB;    // EXPOSED via macros. Do not move

  PFN_FILTER_PACKET_INDICATION_HANDLER      PacketIndicateHandler;
  union
  {
      NDIS_M_SEND_COMPLETE_HANDLER		SendCompleteHandler;
      NDIS_M_WAN_SEND_COMPLETE_HANDLER	WanSendCompleteHandler;
  };
  NDIS_M_SEND_RESOURCES_HANDLER         SendResourcesHandler;
  NDIS_M_RESET_COMPLETE_HANDLER         ResetCompleteHandler;

  PVOID                 WrapperContext;
  NDIS_MEDIUM           MediaType;

  //
  // contains mini-port information
  //
  ULONG                 BusNumber;
  NDIS_INTERFACE_TYPE   BusType;
  NDIS_INTERFACE_TYPE   AdapterType;

  //
  // Holds the map registers for this mini-port.
  //
  ULONG                 PhysicalMapRegistersNeeded;
  ULONG                 MaximumPhysicalMapping;
  PMAP_REGISTER_ENTRY   MapRegisters;  // EXPOSED via macros. Do not move

  //
  //  WorkItem routines that can change depending on whether we
  //  are fullduplex or not.
  //
  NDIS_M_PROCESS_DEFERRED       ProcessDeferredHandler;
  NDIS_M_QUEUE_WORK_ITEM        QueueWorkItemHandler;
  NDIS_M_QUEUE_NEW_WORK_ITEM    QueueNewWorkItemHandler;
  NDIS_M_DEQUEUE_WORK_ITEM      DeQueueWorkItemHandler;

  PNDIS_TIMER           DeferredTimer;

  //
  // Resource information
  //
  PCM_RESOURCE_LIST     Resources;

  //
  //  This pointer is reserved. Used for debugging
  //
  PVOID                 Reserved;

//  PADAPTER_OBJECT       SystemAdapterObject;

  SINGLE_LIST_ENTRY     SingleWorkItems[NUMBER_OF_SINGLE_WORK_ITEMS];

  //
  //  For efficiency
  //
  W_HANDLE_INTERRUPT_HANDLER    HandleInterruptHandler;
  W_DISABLE_INTERRUPT_HANDLER   DisableInterruptHandler;
  W_ENABLE_INTERRUPT_HANDLER    EnableInterruptHandler;
  W_SEND_PACKETS_HANDLER        SendPacketsHandler;
//  NDIS_M_START_SENDS            DeferredSendHandler;

  /* Stuff above is potentially accessed by macros. Add stuff below */

  UCHAR                 TrResetRing;
  UCHAR                 ArcnetAddress;

  //
  //  This is the processor number that the miniport's
  //  interrupt DPC and timers are running on.
  //
  UCHAR                 AssignedProcessor;

  NDIS_HANDLE           ArcnetBufferPool;
  PARC_BUFFER_LIST      ArcnetFreeBufferList;
  PARC_BUFFER_LIST      ArcnetUsedBufferList;
  PUCHAR                ArcnetLookaheadBuffer;
  UINT                  CheckForHangTimeout;

  //
  // These two are used temporarily while allocating the map registers.
  //
  KEVENT                AllocationEvent;
  UINT                  CurrentMapRegister;

  //
  // Send information
  //
  NDIS_SPIN_LOCK        SendLock;
  ULONG                 SendFlags;      // Flags for send path.
  PNDIS_PACKET          FirstPacket;    // This pointer serves two purposes;
                          // it is the head of the queue of ALL
                          // packets that have been sent to
                          // the miniport, it is also the head
                          // of the packets that have been sent
                          // down to the miniport by the wrapper.
  PNDIS_PACKET          LastPacket;      // This is tail pointer for the global
                          // packet queue and this is the tail
                          // pointer to the queue of packets
                          // waiting to be sent to the miniport.
  PNDIS_PACKET          FirstPendingPacket; // This is head of the queue of packets
                          // waiting to be sent to miniport.
  PNDIS_PACKET          LastMiniportPacket; // This is the tail pointer of the
                          // queue of packets that have been
                          // sent to the miniport by the wrapper.

  PNDIS_PACKET          LoopbackHead;    // Head of loopback queue.
  PNDIS_PACKET          LoopbackTail;    // Tail of loopback queue.

  ULONG                 SendResourcesAvailable;
  PPNDIS_PACKET         PacketArray;
  UINT                  MaximumSendPackets;

  //
  // Transfer data information
  //
  PNDIS_PACKET          FirstTDPacket;
  PNDIS_PACKET          LastTDPacket;
  PNDIS_PACKET          LoopbackPacket;

  //
  // Reset information
  //
  NDIS_STATUS           ResetStatus;

  //
  // RequestInformation
  //
  PNDIS_REQUEST         PendingRequest;
  PNDIS_REQUEST         MiniportRequest;
  NDIS_STATUS           RequestStatus;
  UINT                  MaximumLongAddresses;
  UINT                  MaximumShortAddresses;
  UINT                  CurrentLookahead;
  UINT                  MaximumLookahead;
  UINT                  MacOptions;

  KEVENT                RequestEvent;
  UCHAR                 MulticastBuffer[NDIS_M_MAX_MULTI_LIST][6];

  //
  // Temp stuff for using the old NDIS functions
  //
  ULONG                 ChannelNumber;
  UINT                  NumberOfAllocatedWorkItems;
//  PNDIS_LOG             Log;

    //
	// Context associated with the intermediate driver
	//
	NDIS_HANDLE			DeviceContext;

	//
	//	Should the adapter be rebound to protocol drivers on a resume.
	//	E.g. to force DHCP to be run.
	//	0 == no rebind on resume
	//  1 == rebind on resume
	//
	DWORD				bDoAdapterRebindOnResume;

  // Needed for PnP. Upcased version. The buffer is allocated as part of the
  // NDIS_MINIPORT_BLOCK itself.
  UNICODE_STRING        BaseName;
};

struct _NDIS_M_DRIVER_BLOCK {
  PNDIS_MINIPORT_BLOCK          MiniportQueue;
  NDIS_HANDLE                   MiniportIdField;
  REFERENCE                     Ref;
  UINT                          Length;
  NDIS_MINIPORT_CHARACTERISTICS MiniportCharacteristics;
  PNDIS_WRAPPER_HANDLE          NdisDriverInfo;
  PNDIS_M_DRIVER_BLOCK          NextDriver;
  PNDIS_MAC_BLOCK               FakeMac;
  KEVENT                        MiniportsRemovedEvent;
  BOOLEAN                       Unloading;
  UNICODE_STRING                BaseName;
};

typedef struct _NDIS_PROTOCOL_FILTER {
          struct _NDIS_PROTOCOL_FILTER  *Next;
          RECEIVE_HANDLER               ReceiveHandler;
          USHORT                        Offset;
          USHORT                        Size;
          NDIS_MEDIUM                   Medium;
        } NDIS_PROTOCOL_FILTER, *PNDIS_PROTOCOL_FILTER;

typedef struct _NDIS_PROTOCOL_BLOCK {
          PNDIS_OPEN_BLOCK              OpenQueue;
          REFERENCE                     Ref;
          UINT                          Length;
          NDIS_PROTOCOL_CHARACTERISTICS ProtocolCharacteristics;
          struct _NDIS_PROTOCOL_BLOCK   *NextProtocol;
          ULONG                         MaxPatternSize;
          PNDIS_PROTOCOL_FILTER         ProtocolFilter;
        } NDIS_PROTOCOL_BLOCK, *PNDIS_PROTOCOL_BLOCK;

struct _NDIS_OPEN_BLOCK {
  PNDIS_MAC_BLOCK               MacHandle;
  NDIS_HANDLE                   MacBindingHandle;
  NDIS_HANDLE                   AdapterHandle;
  PNDIS_PROTOCOL_BLOCK          ProtocolHandle;
  NDIS_HANDLE                   ProtocolBindingContext;
  PNDIS_OPEN_BLOCK              AdapterNextOpen;
  PNDIS_OPEN_BLOCK              ProtocolNextOpen;
//  PFILE_OBJECT                  FileObject;
  BOOLEAN                       Closing;
  BOOLEAN                       Unloading;
  NDIS_HANDLE                   CloseRequestHandle;
  KSPIN_LOCK                    SpinLock;
  PNDIS_OPEN_BLOCK              NextGlobalOpen;

  union
  {
      SEND_HANDLER              SendHandler;
      WAN_SEND_HANDLER          WanSendHandler;
  };
  TRANSFER_DATA_HANDLER         TransferDataHandler;

  SEND_COMPLETE_HANDLER         SendCompleteHandler;
  TRANSFER_DATA_COMPLETE_HANDLER TransferDataCompleteHandler;
  RECEIVE_HANDLER               ReceiveHandler;
  RECEIVE_COMPLETE_HANDLER      ReceiveCompleteHandler;

  RECEIVE_HANDLER               PostNt31ReceiveHandler;
  RECEIVE_COMPLETE_HANDLER      PostNt31ReceiveCompleteHandler;

  RECEIVE_PACKET_HANDLER        ReceivePacketHandler;
  SEND_PACKETS_HANDLER          SendPacketsHandler;

  UNICODE_STRING                AdapterName;
};

struct _NDIS_M_OPEN_BLOCK {
  PNDIS_M_DRIVER_BLOCK          DriverHandle;
  PNDIS_MINIPORT_BLOCK          MiniportHandle;
  PNDIS_PROTOCOL_BLOCK          ProtocolHandle;
  PNDIS_OPEN_BLOCK              FakeOpen;
  NDIS_HANDLE                   ProtocolBindingContext;
  NDIS_HANDLE                   MiniportAdapterContext;
  PNDIS_M_OPEN_BLOCK            MiniportNextOpen;
//  PFILE_OBJECT                  FileObject;
  ULONG                         Flags;
  NDIS_HANDLE                   CloseRequestHandle;
  NDIS_HANDLE                   FilterHandle;
  NDIS_SPIN_LOCK                SpinLock;
  ULONG                         References;
  UINT                          CurrentLookahead;
  ULONG                         ProtocolOptions;

  W_SEND_HANDLER                SendHandler;
  W_TRANSFER_DATA_HANDLER       TransferDataHandler;

  SEND_COMPLETE_HANDLER         SendCompleteHandler;
  TRANSFER_DATA_COMPLETE_HANDLER TransferDataCompleteHandler;
  RECEIVE_HANDLER               ReceiveHandler;
  RECEIVE_COMPLETE_HANDLER      ReceiveCompleteHandler;
  RECEIVE_PACKET_HANDLER        ReceivePacketHandler;

  W_SEND_PACKETS_HANDLER        SendPacketsHandler;

//  W_CO_SEND_PACKETS_HANDLER     CoSendPacketsHandler;
//  W_CO_REQUEST_HANDLER          CoRequestHandler;

  BOOLEAN                       ReceivedAPacket;
  BOOLEAN                       IndicatingNow;

//  struct _NDIS_CO_AF_BLOCK      *NextAf;

  ULONG                         AddressFamilyMask;

  LIST_ENTRY                    ActiveVcHead;
  LIST_ENTRY                    FreeVcHead;

//  CO_SEND_COMPLETE_HANDLER      CoSendCompleteHandler;
//  CO_RECEIVE_PACKET_HANDLER     CoReceivePacketHandler;
};

struct _NDIS_MINIPORT_INTERRUPT {
  PKINTERRUPT                   InterruptObject;
  KSPIN_LOCK                    DpcCountLock;
  PVOID                         MiniportIdField;
  W_ISR_HANDLER                 MiniportIsr;
  W_HANDLE_INTERRUPT_HANDLER    MiniportDpc;
  NDIS_DPC                      InterruptDpc;
  PNDIS_MINIPORT_BLOCK          Miniport;
  UCHAR                         DpcCount;
  BOOLEAN                       SharedInterrupt;
  BOOLEAN                       IsrRequested;
};

typedef struct _NDIS_MINIPORT_TIMER {
          KTIMER                Timer;
          NDIS_DPC              Dpc;
          PNDIS_TIMER_FUNCTION  MiniportTimerFunction;
          PVOID                 MiniportTimerContext;
          PNDIS_MINIPORT_BLOCK  Miniport;
          struct _NDIS_MINIPORT_TIMER *NextDeferredTimer;
        } NDIS_MINIPORT_TIMER, *PNDIS_MINIPORT_TIMER;

typedef struct _MINIPORT_PENDING_OPEN {
          struct _MINIPORT_PENDING_OPEN *NextPendingOpen;
          PNDIS_HANDLE          NdisBindingHandle;
          PNDIS_MINIPORT_BLOCK  Miniport;
          PVOID                 NewOpenP;
          PVOID                 FileObject;
          NDIS_HANDLE           NdisProtocolHandle;
          NDIS_HANDLE           ProtocolBindingContext;
          PNDIS_STRING          AdapterName;
          UINT                  OpenOptions;
          PSTRING               AddressingInformation;
          ULONG                 Flags;
          NDIS_STATUS           Status;
          NDIS_STATUS           OpenErrorStatus;
#if defined(NDIS_WRAPPER)
          WORK_QUEUE_ITEM       WorkItem;
#endif
        } MINIPORT_PENDING_OPEN, *PMINIPORT_PENDING_OPEN;

/* types of memory */
#define NDIS_MEMORY_CONTIGUOUS          0x00000001
#define NDIS_MEMORY_NONCACHED           0x00000002

/* miniport attributes */
#define NDIS_ATTRIBUTE_IGNORE_PACKET_TIMEOUT    0x00000001
#define NDIS_ATTRIBUTE_IGNORE_REQUEST_TIMEOUT   0x00000002
#define NDIS_ATTRIBUTE_IGNORE_TOKEN_RING_ERRORS 0x00000004
#define NDIS_ATTRIBUTE_BUS_MASTER               0x00000008
#define NDIS_ATTRIBUTE_INTERMEDIATE_DRIVER      0x00000010
#define NDIS_ATTRIBUTE_DESERIALIZE				0x00000020
#define NDIS_ATTRIBUTE_NO_HALT_ON_SUSPEND		0x00000040

#define NDIS_ERROR_CODE                         ULONG

#define NDIS_ERROR_CODE_RESOURCE_CONFLICT       EVENT_NDIS_RESOURCE_CONFLICT
#define NDIS_ERROR_CODE_OUT_OF_RESOURCES        EVENT_NDIS_OUT_OF_RESOURCE
#define NDIS_ERROR_CODE_HARDWARE_FAILURE        EVENT_NDIS_HARDWARE_FAILURE
#define NDIS_ERROR_CODE_ADAPTER_NOT_FOUND       EVENT_NDIS_ADAPTER_NOT_FOUND
#define NDIS_ERROR_CODE_INTERRUPT_CONNECT       EVENT_NDIS_INTERRUPT_CONNECT
#define NDIS_ERROR_CODE_DRIVER_FAILURE          EVENT_NDIS_DRIVER_FAILURE
#define NDIS_ERROR_CODE_BAD_VERSION             EVENT_NDIS_BAD_VERSION
#define NDIS_ERROR_CODE_TIMEOUT                 EVENT_NDIS_TIMEOUT
#define NDIS_ERROR_CODE_NETWORK_ADDRESS         EVENT_NDIS_NETWORK_ADDRESS
#define NDIS_ERROR_CODE_UNSUPPORTED_CONFIGURATION EVENT_NDIS_UNSUPPORTED_CONFIGURATION
#define NDIS_ERROR_CODE_INVALID_VALUE_FROM_ADAPTER EVENT_NDIS_INVALID_VALUE_FROM_ADAPTER
#define NDIS_ERROR_CODE_MISSING_CONFIGURATION_PARAMETER EVENT_NDIS_MISSING_CONFIGURATION_PARAMETER
#define NDIS_ERROR_CODE_BAD_IO_BASE_ADDRESS     EVENT_NDIS_BAD_IO_BASE_ADDRESS
#define NDIS_ERROR_CODE_RECEIVE_SPACE_SMALL     EVENT_NDIS_RECEIVE_SPACE_SMALL
#define NDIS_ERROR_CODE_ADAPTER_DISABLEDi       EVENT_NDIS_ADAPTER_DISABLED

/*
 * NDIS_STATUS values
 *   Status values are 32 bit values layed out as follows:
 *
 *   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
 *   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 *  +---+-+-------------------------+-------------------------------+
 *  |Sev|C|       Facility          |               Code            |
 *  +---+-+-------------------------+-------------------------------+
 *
 *  where
 *
 *    Sev - is the severity code
 *      00 - Success
 *      01 - Informational
 *      10 - Warning
 *      11 - Error
 *
 *    C - is the Customer code flag
 *
 *    Facility - is the facility code
 *
 *    Code - is the facility's status code
 */
#define NDIS_STATUS_SUCCESS             ((NDIS_STATUS)STATUS_SUCCESS)
#define NDIS_STATUS_PENDING             ((NDIS_STATUS)STATUS_PENDING)
#define NDIS_STATUS_NOT_RECOGNIZED      ((NDIS_STATUS)0x00010001L)
#define NDIS_STATUS_NOT_COPIED          ((NDIS_STATUS)0x00010002L)
#define NDIS_STATUS_NOT_ACCEPTED        ((NDIS_STATUS)0x00010003L)
#define NDIS_STATUS_CALL_ACTIVE         ((NDIS_STATUS)0x00010007L)

#define NDIS_STATUS_ONLINE              ((NDIS_STATUS)0x40010003L)
#define NDIS_STATUS_RESET_START         ((NDIS_STATUS)0x40010004L)
#define NDIS_STATUS_RESET_END           ((NDIS_STATUS)0x40010005L)
#define NDIS_STATUS_RING_STATUS         ((NDIS_STATUS)0x40010006L)
#define NDIS_STATUS_CLOSED              ((NDIS_STATUS)0x40010007L)
#define NDIS_STATUS_WAN_LINE_UP         ((NDIS_STATUS)0x40010008L)
#define NDIS_STATUS_WAN_LINE_DOWN       ((NDIS_STATUS)0x40010009L)
#define NDIS_STATUS_WAN_FRAGMENT        ((NDIS_STATUS)0x4001000AL)
#define NDIS_STATUS_MEDIA_CONNECT       ((NDIS_STATUS)0x4001000BL)
#define NDIS_STATUS_MEDIA_DISCONNECT    ((NDIS_STATUS)0x4001000CL)
#define NDIS_STATUS_HARDWARE_LINE_UP    ((NDIS_STATUS)0x4001000DL)
#define NDIS_STATUS_HARDWARE_LINE_DOWN  ((NDIS_STATUS)0x4001000EL)
#define NDIS_STATUS_INTERFACE_UP        ((NDIS_STATUS)0x4001000FL)
#define NDIS_STATUS_INTERFACE_DOWN      ((NDIS_STATUS)0x40010010L)
#define NDIS_STATUS_MEDIA_BUSY          ((NDIS_STATUS)0x40010011L)

#define NDIS_STATUS_NOT_RESETTABLE      ((NDIS_STATUS)0x80010001L)
#define NDIS_STATUS_SOFT_ERRORS         ((NDIS_STATUS)0x80010003L)
#define NDIS_STATUS_HARD_ERRORS         ((NDIS_STATUS)0x80010004L)
#define NDIS_STATUS_BUFFER_OVERFLOW     ((NDIS_STATUS)STATUS_BUFFER_OVERFLOW)

#define NDIS_STATUS_FAILURE             ((NDIS_STATUS)STATUS_UNSUCCESSFUL)
#define NDIS_STATUS_RESOURCES           ((NDIS_STATUS)STATUS_INSUFFICIENT_RESOURCES)
#define NDIS_STATUS_NOT_SUPPORTED       ((NDIS_STATUS)STATUS_NOT_SUPPORTED)
#define NDIS_STATUS_CLOSING             ((NDIS_STATUS)0xC0010002L)
#define NDIS_STATUS_BAD_VERSION         ((NDIS_STATUS)0xC0010004L)
#define NDIS_STATUS_BAD_CHARACTERISTICS ((NDIS_STATUS)0xC0010005L)
#define NDIS_STATUS_ADAPTER_NOT_FOUND   ((NDIS_STATUS)0xC0010006L)
#define NDIS_STATUS_OPEN_FAILED         ((NDIS_STATUS)0xC0010007L)
#define NDIS_STATUS_DEVICE_FAILED       ((NDIS_STATUS)0xC0010008L)
#define NDIS_STATUS_MULTICAST_FULL      ((NDIS_STATUS)0xC0010009L)
#define NDIS_STATUS_MULTICAST_EXISTS    ((NDIS_STATUS)0xC001000AL)
#define NDIS_STATUS_MULTICAST_NOT_FOUND ((NDIS_STATUS)0xC001000BL)
#define NDIS_STATUS_REQUEST_ABORTED     ((NDIS_STATUS)0xC001000CL)
#define NDIS_STATUS_RESET_IN_PROGRESS   ((NDIS_STATUS)0xC001000DL)
#define NDIS_STATUS_CLOSING_INDICATING  ((NDIS_STATUS)0xC001000EL)
#define NDIS_STATUS_INVALID_PACKET      ((NDIS_STATUS)0xC001000FL)
#define NDIS_STATUS_OPEN_LIST_FULL      ((NDIS_STATUS)0xC0010010L)
#define NDIS_STATUS_ADAPTER_NOT_READY   ((NDIS_STATUS)0xC0010011L)
#define NDIS_STATUS_ADAPTER_NOT_OPEN    ((NDIS_STATUS)0xC0010012L)
#define NDIS_STATUS_NOT_INDICATING      ((NDIS_STATUS)0xC0010013L)
#define NDIS_STATUS_INVALID_LENGTH      ((NDIS_STATUS)0xC0010014L)
#define NDIS_STATUS_INVALID_DATA        ((NDIS_STATUS)0xC0010015L)
#define NDIS_STATUS_BUFFER_TOO_SHORT    ((NDIS_STATUS)0xC0010016L)
#define NDIS_STATUS_INVALID_OID         ((NDIS_STATUS)0xC0010017L)
#define NDIS_STATUS_ADAPTER_REMOVED     ((NDIS_STATUS)0xC0010018L)
#define NDIS_STATUS_UNSUPPORTED_MEDIA   ((NDIS_STATUS)0xC0010019L)
#define NDIS_STATUS_GROUP_ADDRESS_IN_USE ((NDIS_STATUS)0xC001001AL)
#define NDIS_STATUS_FILE_NOT_FOUND      ((NDIS_STATUS)0xC001001BL)
#define NDIS_STATUS_ERROR_READING_FILE  ((NDIS_STATUS)0xC001001CL)
#define NDIS_STATUS_ALREADY_MAPPED      ((NDIS_STATUS)0xC001001DL)
#define NDIS_STATUS_RESOURCE_CONFLICT   ((NDIS_STATUS)0xC001001EL)
#define NDIS_STATUS_NO_CABLE            ((NDIS_STATUS)0xC001001FL)

#define NDIS_STATUS_INVALID_SAP         ((NDIS_STATUS)0xC0010020L)
#define NDIS_STATUS_SAP_IN_USE          ((NDIS_STATUS)0xC0010021L)
#define NDIS_STATUS_INVALID_ADDRESS     ((NDIS_STATUS)0xC0010022L)
#define NDIS_STATUS_VC_NOT_RESERVED     ((NDIS_STATUS)0xC0010023L)
#define NDIS_STATUS_DEST_OUT_OF_ORDER   ((NDIS_STATUS)0xC0010024L)
#define NDIS_STATUS_VC_NOT_AVAILABLE    ((NDIS_STATUS)0xC0010025L)
#define NDIS_STATUS_CELLRATE_NOT_AVAILABLE ((NDIS_STATUS)0xC0010026L)
#define NDIS_STATUS_INCOMPATABLE_QOS    ((NDIS_STATUS)0xC0010027L)
#define NDIS_STATUS_AAL_PARAMS_UNSUPPORTED ((NDIS_STATUS)0xC0010028L)
#define NDIS_STATUS_NO_ROUTE_TO_DESTINATION ((NDIS_STATUS)0xC0010029L)
#define NDIS_STATUS_TOKEN_RING_OPEN_ERROR ((NDIS_STATUS)0xC0011000L)

#define NDIS_SUCCESS(_Status) NT_SUCCESS(_Status)
#define NDIS_INFORMATION(_Status) NT_INFORMATION(_Status)
#define NDIS_WARNING(_Status) NT_WARNING(_Status)
#define NDIS_ERROR(_Status) NT_ERROR(_Status)

#ifndef ARGUMENT_PRESENT
#define ARGUMENT_PRESENT(_ArgumentPointer) ((PCHAR)(_ArgumentPointer) != NULL)
#endif

/* macro to initialize an NDIS_STRING constant */
#define NDIS_STRING_CONST(_x) {sizeof(L##_x)-2, sizeof(L##_x), L##_x}

/* macro to initialize an NDIS_PHYSICAL_ADDRESS constant */
#define NDIS_PHYSICAL_ADDRESS_CONST(_Low, _High) \
         {(ULONG)(_Low), (LONG)(_High)}

/*++
VOID
NdisRawReadPortUchar(
  IN ULONG Port,
  OUT PUCHAR Data
);
--*/
#define NdisRawReadPortUchar(_Port, _Data) \
          *(_Data) = READ_PORT_UCHAR((PUCHAR)(_Port))

/*++
VOID
NdisRawReadPortUshort(
  IN ULONG Port,
  OUT PUSHORT Data
);
--*/
#define NdisRawReadPortUshort(_Port, _Data) \
          *(_Data) = READ_PORT_USHORT((PUSHORT)(_Port))

/*++
VOID
NdisRawReadPortUlong(
  IN ULONG Port,
  OUT PULONG Data
);
--*/
#define NdisRawReadPortUlong(_Port, _Data) \
          *(_Data) = READ_PORT_ULONG((PULONG)(_Port))

/*++
VOID
NdisRawReadPortBufferUchar(
  IN ULONG Port,
  OUT PUCHAR Buffer,
  IN ULONG Length
);
--*/
#define NdisRawReadPortBufferUchar(_Port, _Buffer, _Length) \
          READ_PORT_BUFFER_UCHAR((PUCHAR)(_Port), (PUCHAR)(_Buffer), _Length)

/*++
VOID
NdisRawReadPortBufferUshort(
  IN ULONG Port,
  OUT PUSHORT Buffer,
  IN ULONG Length
);
--*/
#define NdisRawReadPortBufferUshort(_Port, _Buffer, _Length) \
          READ_PORT_BUFFER_USHORT((PUSHORT)(_Port), (PUSHORT)(_Buffer), _Length)

/*++
VOID
NdisRawReadPortBufferUlong(
  IN ULONG Port,
  OUT PULONG Buffer,
  IN ULONG Length
);
--*/
#define NdisRawReadPortBufferUlong(_Port, _Buffer, _Length) \
          READ_PORT_BUFFER_ULONG((PULONG)(_Port), (PULONG)(_Buffer), _Length)

/*++
VOID
NdisRawWritePortUchar(
  IN ULONG Port,
  IN UCHAR Data
);
--*/
#define NdisRawWritePortUchar(_Port, _Data) \
          WRITE_PORT_UCHAR((PUCHAR)(_Port), (UCHAR)(_Data))

/*++
VOID
NdisRawWritePortUshort(
  IN ULONG Port,
  IN USHORT Data
);
--*/
#define NdisRawWritePortUshort(_Port, _Data) \
          WRITE_PORT_USHORT((PUSHORT)(_Port), (USHORT)(_Data))

/*++
VOID
NdisRawWritePortUlong(
  IN ULONG Port,
  IN ULONG Data
);
--*/
#define NdisRawWritePortUlong(_Port, _Data) \
          WRITE_PORT_ULONG((PULONG)(_Port), (ULONG)(_Data))

/*++
VOID
NdisRawWritePortBufferUchar(
  IN ULONG Port,
  IN PUCHAR Buffer,
  IN ULONG Length
);
--*/
#define NdisRawWritePortBufferUchar(_Port, _Buffer, _Length) \
          WRITE_PORT_BUFFER_UCHAR((PUCHAR)(_Port), (PUCHAR)(_Buffer), _Length)

/*++
VOID
NdisRawWritePortBufferUshort(
  IN ULONG Port,
  IN PUSHORT Buffer,
  IN ULONG Length
);
--*/
#define NdisRawWritePortBufferUshort(_Port, _Buffer, _Length) \
          WRITE_PORT_BUFFER_USHORT((PUSHORT)(_Port), (PUSHORT)(_Buffer), \
                                   _Length)

/*++
VOID
NdisRawWritePortBufferUlong(
  IN ULONG Port,
  IN PULONG Buffer,
  IN ULONG Length
);
--*/
#define NdisRawWritePortBufferUlong(_Port, _Buffer, _Length) \
          WRITE_PORT_BUFFER_ULONG((PULONG)(_Port), (PULONG)(_Buffer), _Length)

/*++
VOID
NdisWriteRegisterUchar(
  IN PUCHAR Register,
  IN UCHAR Data
);
--*/
#define NdisWriteRegisterUchar(_Register, _Data) \
          WRITE_REGISTER_UCHAR((PUCHAR)(_Register), (UCHAR)(_Data))

/*++
VOID
NdisWriteRegisterUshort(
  IN PUSHORT Register,
  IN USHORT Data
);
--*/
#define NdisWriteRegisterUshort(_Register, _Data) \
          WRITE_REGISTER_USHORT((PUSHORT)(_Register), (USHORT)(_Data))

/*++
VOID
NdisWriteRegisterUlong(
  IN PULONG Register,
  IN ULONG Data
);
--*/
#define NdisWriteRegisterUlong(_Register, _Data) \
          WRITE_REGISTER_ULONG((PULONG)(_Register), (ULONG)(_Data))

/*++
VOID
NdisReadRegisterUchar(
  IN PUCHAR Register,
  OUT PUCHAR Data
);
--*/
#define NdisReadRegisterUchar(_Register, _Data) \
          *((PUCHAR)(_Data)) = *((PUCHAR)(_Register))

/*++
VOID
NdisReadRegisterUshort(
  IN PUSHORT Register,
  OUT PUSHORT Data
);
--*/
#define NdisReadRegisterUshort(_Register, _Data) \
          *((PUSHORT)(_Data)) = *((PUSHORT)(_Register))

/*++
VOID
NdisReadRegisterUlong(
  IN PULONG Register,
  OUT PULONG Data
);
--*/
#define NdisReadRegisterUlong(_Register, _Data) \
          *((PULONG)(_Data)) = *((PULONG)(_Register))


#define NdisMoveMemory(_Destination, _Source, _Length) \
          RtlCopyMemory(_Destination, _Source, _Length)
#define NdisZeroMemory(_Destination, _Length) \
          RtlZeroMemory(_Destination, _Length)

#define NdisGetCacheFillSize()  HalGetDmaAlignmentRequirement()
EXPORT NDIS_STATUS NDISAPI
NdisFlushBuffer(
  IN PNDIS_BUFFER pBuffer,
  IN BOOLEAN fWriteToDevice
);

EXPORT VOID NDISAPI
NdisStallExecution(
	IN UINT MicrosecondsToStall);

#define NdisMInitializeWrapper(_NdisWrapperHandle, _SystemSpecific1, \
                               _SystemSpecific2, _SystemSpecific3) \
          NdisInitializeWrapper(_NdisWrapperHandle, _SystemSpecific1, \
                                _SystemSpecific2, _SystemSpecific3)
#define NdisUpdateSharedMemory(_M, _L, _V, _P)
#define NdisMUpdateSharedMemory(_M, _L, _V, _P) \
          NdisUpdateSharedMemory(_M, _L, _V, _P)

/* used by the filter packages for indicating receives */
#define FilterIndicateReceive( \
          _Status, \
          _NdisBindingContext, \
          _MacReceiveContext, \
          _HeaderBuffer, \
          _HeaderBufferSize, \
          _LookaheadBuffer, \
          _LookaheadBufferSize, \
          _PacketSize \
          ) \
          *(_Status) \
            = (((PNDIS_OPEN_BLOCK)(_NdisBindingContext)) \
               ->PostNt31ReceiveHandler)( \
                 ((PNDIS_OPEN_BLOCK)(_NdisBindingContext)) \
                   ->ProtocolBindingContext, \
                 _MacReceiveContext, \
                 _HeaderBuffer, \
                 _HeaderBufferSize, \
                 _LookaheadBuffer, \
                 _LookaheadBufferSize, \
                 _PacketSize)

/* used by the filter packages for indicating receive completion */
#define FilterIndicateReceiveComplete(_NdisBindingContext) \
          (((PNDIS_OPEN_BLOCK)(_NdisBindingContext)) \
           ->PostNt31ReceiveCompleteHandler)( \
             ((PNDIS_OPEN_BLOCK)(_NdisBindingContext)) \
               ->ProtocolBindingContext)

/*++
VOID
NdisMIndicateReceivePacket(
  IN NDIS_HANDLE MiniportAdapterHandle,
  IN PPNDIS_PACKET ReceivedPackets,
  IN UINT NumberOfPackets
);
--*/
#define NdisMIndicateReceivePacket(_M, _P, _N) \
          (*((PNDIS_MINIPORT_BLOCK)(_M))->PacketIndicateHandler)(_M, _P, _N)

#define NDIS_OOB_DATA_FROM_PACKET(_p) \
          (PNDIS_PACKET_OOB_DATA)((PUCHAR)(_p) \
                                  + (_p)->Private.NdisPacketOobOffset)
#define NDIS_GET_PACKET_HEADER_SIZE(_Packet) \
          ((PNDIS_PACKET_OOB_DATA)((PUCHAR)(_Packet) \
           + (_Packet)->Private.NdisPacketOobOffset))->HeaderSize
#define NDIS_GET_PACKET_STATUS(_Packet) \
          ((PNDIS_PACKET_OOB_DATA)((PUCHAR)(_Packet) \
           + (_Packet)->Private.NdisPacketOobOffset))->Status
#define NDIS_GET_PACKET_TIME_TO_SEND(_Packet) \
          ((PNDIS_PACKET_OOB_DATA)((PUCHAR)(_Packet) \
           + (_Packet)->Private.NdisPacketOobOffset))->TimeToSend
#define NDIS_GET_PACKET_TIME_SENT(_Packet) \
          ((PNDIS_PACKET_OOB_DATA)((PUCHAR)(_Packet) \
           + (_Packet)->Private.NdisPacketOobOffset))->TimeSent
#define NDIS_GET_PACKET_TIME_RECEIVED(_Packet) \
          ((PNDIS_PACKET_OOB_DATA)((PUCHAR)(_Packet) \
           + (_Packet)->Private.NdisPacketOobOffset))->TimeReceived

//
// Support for MEDIA_SPECIFIC_INFORMATION
//

// Definition for layout of the media-specific data. More than one class of media-specific
// information can be tagged onto a packet.
typedef enum _NDIS_CLASS_ID
{
    NdisClass802_3Priority,
    NdisClassWirelessWanMbxMailbox,
    NdisClassIrdaPacketInfo,
    NdisClassAtmAALInfo

} NDIS_CLASS_ID;

typedef struct _MEDIA_SPECIFIC_INFORMATION
{
    UINT            NextEntryOffset;
    NDIS_CLASS_ID   ClassId;
    UINT            Size;
    UCHAR           ClassInformation[1];

} MEDIA_SPECIFIC_INFORMATION, *PMEDIA_SPECIFIC_INFORMATION;

#define	NDIS_FLAGS_DONT_LOOPBACK				0x00000080


#define fPACKET_CONTAINS_MEDIA_SPECIFIC_INFO    0x40
#define fPACKET_ALLOCATED_BY_NDIS               0x80

#define NDIS_GET_PACKET_MEDIA_SPECIFIC_INFO(_Packet, \
                                            _pMediaSpecificInfo, \
                                            _pSizeMediaSpecificInfo) \
        { \
          if (((_Packet)->Private.NdisPacketFlags \
                & fPACKET_ALLOCATED_BY_NDIS) == 0 \
              || ((_Packet)->Private.NdisPacketFlags \
                   & fPACKET_CONTAINS_MEDIA_SPECIFIC_INFO) == 0) { \
            *(_pMediaSpecificInfo) = NULL; \
            *(_pSizeMediaSpecificInfo) = 0; \
          } else { \
            *(_pMediaSpecificInfo) \
              = ((PNDIS_PACKET_OOB_DATA)((PUCHAR)(_Packet) \
                 + (_Packet)->Private.NdisPacketOobOffset)) \
                ->MediaSpecificInformation; \
            *(_pSizeMediaSpecificInfo) \
              = ((PNDIS_PACKET_OOB_DATA)((PUCHAR)(_Packet) \
                 + (_Packet)->Private.NdisPacketOobOffset)) \
                ->SizeMediaSpecificInfo; \
          } \
        }

#define NDIS_SET_PACKET_HEADER_SIZE(_Packet, _HdrSize) \
          ((PNDIS_PACKET_OOB_DATA)((PUCHAR)(_Packet) \
           + (_Packet)->Private.NdisPacketOobOffset))->HeaderSize = _HdrSize
#define NDIS_SET_PACKET_STATUS(_Packet, _Status) \
          ((PNDIS_PACKET_OOB_DATA)((PUCHAR)(_Packet) \
           + (_Packet)->Private.NdisPacketOobOffset))->Status = _Status
#define NDIS_SET_PACKET_TIME_TO_SEND(_Packet, _TimeToSend) \
          ((PNDIS_PACKET_OOB_DATA)((PUCHAR)(_Packet) \
           + (_Packet)->Private.NdisPacketOobOffset))->TimeToSend \
          = _TimeToSend
#define NDIS_SET_PACKET_TIME_SENT(_Packet, _TimeSent) \
          ((PNDIS_PACKET_OOB_DATA)((PUCHAR)(_Packet) \
           + (_Packet)->Private.NdisPacketOobOffset))->TimeSent = _TimeSent
#define NDIS_SET_PACKET_TIME_RECEIVED(_Packet, _TimeReceived) \
          ((PNDIS_PACKET_OOB_DATA)((PUCHAR)(_Packet) \
           + (_Packet)->Private.NdisPacketOobOffset))->TimeReceived \
          = _TimeReceived
#define NDIS_SET_PACKET_MEDIA_SPECIFIC_INFO(_Packet, \
                                            _MediaSpecificInfo, \
                                            _SizeMediaSpecificInfo) \
        { \
          if (((_Packet)->Private.NdisPacketFlags \
               & fPACKET_ALLOCATED_BY_NDIS) != 0) { \
            (_Packet)->Private.NdisPacketFlags \
              |= fPACKET_CONTAINS_MEDIA_SPECIFIC_INFO; \
            ((PNDIS_PACKET_OOB_DATA)((PUCHAR)(_Packet) \
             + (_Packet)->Private.NdisPacketOobOffset)) \
            ->MediaSpecificInformation = _MediaSpecificInfo; \
            ((PNDIS_PACKET_OOB_DATA)((PUCHAR)(_Packet) \
             + (_Packet)->Private.NdisPacketOobOffset)) \
            ->SizeMediaSpecificInfo = _SizeMediaSpecificInfo; \
          } \
        }

/*++
VOID
NdisGetFirstBufferFromPacket(
  IN PNDIS_PACKET Packet,
  OUT PNDIS_BUFFER *FirstBuffer,
  OUT PVOID *FirstBufferVA,
  OUT PUINT FirstBufferLength,
  OUT PUINT TotalBufferLength
);
--*/
#define NdisGetFirstBufferFromPacket(_Packet, \
                                     _FirstBuffer, \
                                     _FirstBufferVA, \
                                     _FirstBufferLength, \
                                     _TotalBufferLength) \
        { \
          PNDIS_BUFFER _pBuf = (_Packet)->Private.Head; \
          *(_FirstBuffer) = _pBuf; \
          *(_FirstBufferVA) = _pBuf->VirtualAddress; \
          *(_FirstBufferLength) = *(_TotalBufferLength) \
            = _pBuf->BufferLength; \
          for (_pBuf=_pBuf->Next; _pBuf!=NULL; _pBuf=_pBuf->Next) \
            *(_TotalBufferLength) += _pBuf->BufferLength; \
        }

/*++
VOID
NdisFreeString(
  IN NDIS_STRING String
);
--*/
#define NdisFreeString(_String) \
          NdisFreeMemory((_String).Buffer, (_String).MaximumLength, 0)

/*++
ULONG
NdisPrintString(
  PNDIS_STRING String
);
--*/
#define NdisPrintString(_String) DbgPrint("%ls", (_String).Buffer)

/*++
ULONG
NdisGetPhysicalAddressHigh(
  IN NDIS_PHYSICAL_ADDRESS PhysicalAddress
);
--*/
#define NdisGetPhysicalAddressHigh(_PhysicalAddress) \
          ((_PhysicalAddress).HighPart)

/*++
VOID
NdisSetPhysicalAddressHigh(
  IN NDIS_PHYSICAL_ADDRESS PhysicalAddress,
  IN ULONG Value
);
--*/
#define NdisSetPhysicalAddressHigh(_PhysicalAddress, _Value) \
          (_PhysicalAddress).HighPart = _Value

/*++
ULONG
NdisGetPhysicalAddressLow(
  IN NDIS_PHYSICAL_ADDRESS PhysicalAddress
);
--*/
#define NdisGetPhysicalAddressLow(_PhysicalAddress) \
          ((_PhysicalAddress).LowPart)

/*++
VOID
NdisSetPhysicalAddressLow(
  IN NDIS_PHYSICAL_ADDRESS PhysicalAddress,
  IN ULONG Value
);
--*/
#define NdisSetPhysicalAddressLow(_PhysicalAddress, _Value) \
          (_PhysicalAddress).LowPart = _Value

/*++
PVOID
NDIS_BUFFER_LINKAGE(
  IN PNDIS_BUFFER Buffer
);
--*/
#define NDIS_BUFFER_LINKAGE(_Buffer) ((PVOID)(_Buffer)->Next)

/*++
VOID
NdisSetPacketFlags(
  IN PNDIS_PACKET Packet,
  IN UINT Flags
);
--*/
#define NdisSetPacketFlags(_Packet, _Flags) (_Packet)->Private.Flags = _Flags

/*++
UINT
NdisGetPacketFlags(
  IN PNDIS_PACKET Packet
);
--*/
#define NdisGetPacketFlags(_Packet) (_Packet)->Private.Flags

/*++
VOID
NdisRecalculatePacketCounts(
  IN OUT PNDIS_PACKET Packet
);
--*/
#define NdisRecalculatePacketCounts(_Packet) \
        { \
          PNDIS_BUFFER TmpBuffer = (_Packet)->Private.Head; \
          if (TmpBuffer) { \
            while (TmpBuffer->Next) \
              TmpBuffer = TmpBuffer->Next; \
            (_Packet)->Private.Tail = TmpBuffer; \
          } \
          (_Packet)->Private.ValidCounts = FALSE; \
        }

/*++
VOID
NdisReinitializePacket(
  IN OUT PNDIS_PACKET Packet
);
--*/
#define NdisReinitializePacket(_Packet) \
        { \
          (_Packet)->Private.Head = NULL; \
          (_Packet)->Private.ValidCounts = FALSE; \
        }

/*++
VOID
NdisChainBufferAtFront(
  IN OUT PNDIS_PACKET Packet,
  IN OUT PNDIS_BUFFER Buffer
);
--*/
#define NdisChainBufferAtFront(_Packet, _Buffer) \
        { \
          PNDIS_BUFFER TmpBuffer = _Buffer; \
          while (TmpBuffer->Next != NULL) \
            TmpBuffer = TmpBuffer->Next; \
          if ((_Packet)->Private.Head == NULL) \
            (_Packet)->Private.Tail = TmpBuffer; \
          TmpBuffer->Next = (_Packet)->Private.Head; \
          (_Packet)->Private.Head = _Buffer; \
          (_Packet)->Private.ValidCounts = FALSE; \
        }

/*++
VOID
NdisChainBufferAtBack(
  IN OUT PNDIS_PACKET Packet,
  IN OUT PNDIS_BUFFER Buffer
);
--*/
#define NdisChainBufferAtBack(_Packet, _Buffer) \
        { \
          PNDIS_BUFFER TmpBuffer = _Buffer; \
          while (TmpBuffer->Next != NULL) \
            TmpBuffer = TmpBuffer->Next; \
          if ((_Packet)->Private.Head == NULL) \
            (_Packet)->Private.Head = _Buffer; \
          else \
            (_Packet)->Private.Tail->Next = _Buffer; \
          (_Packet)->Private.Tail = TmpBuffer; \
          (_Packet)->Private.ValidCounts = FALSE; \
        }

/*++
VOID
NdisQueryPacket(
  IN PNDIS_PACKET _Packet,
  OUT PUINT _PhysicalBufferCount OPTIONAL,
  OUT PUINT _BufferCount OPTIONAL,
  OUT PNDIS_BUFFER *_FirstBuffer OPTIONAL,
  OUT PUINT _TotalPacketLength OPTIONAL
);
--*/
#define NdisQueryPacket(_Packet, \
                        _PhysicalBufferCount, \
                        _BufferCount, \
                        _FirstBuffer, \
                        _TotalPacketLength) \
        { \
          if (!(_Packet)->Private.ValidCounts \
              && ((_PhysicalBufferCount) != NULL \
                  || (_BufferCount) != NULL \
                  || (_TotalPacketLength) != NULL)) { \
            PNDIS_BUFFER _TmpBuffer = (_Packet)->Private.Head; \
            UINT _TotalLength = 0; \
            UINT _Count = 0; \
            while (_TmpBuffer != NULL) { \
              _TotalLength += _TmpBuffer->BufferLength; \
              _Count++; \
              _TmpBuffer = _TmpBuffer->Next; \
            } \
            (_Packet)->Private.Count = _Count; \
            (_Packet)->Private.TotalLength = _TotalLength; \
            (_Packet)->Private.PhysicalCount = 0; \
            (_Packet)->Private.ValidCounts = TRUE; \
          } \
          if ((_PhysicalBufferCount) != NULL) { \
            PUINT __PhysicalBufferCount = _PhysicalBufferCount; \
            *__PhysicalBufferCount = (_Packet)->Private.PhysicalCount; \
          } \
          if ((_BufferCount) != NULL) { \
            PUINT __BufferCount = _BufferCount; \
            *__BufferCount = (_Packet)->Private.Count; \
          } \
          if ((_FirstBuffer) != NULL) { \
            PNDIS_BUFFER *__FirstBuffer = _FirstBuffer; \
            *__FirstBuffer = (_Packet)->Private.Head; \
          } \
          if ((_TotalPacketLength) != NULL) { \
            PUINT __TotalPacketLength = _TotalPacketLength; \
            *__TotalPacketLength = (_Packet)->Private.TotalLength; \
          } \
        }

/*++
VOID
NdisGetNextBuffer(
  IN PNDIS_BUFFER CurrentBuffer,
  OUT PNDIS_BUFFER *NextBuffer
);
--*/
#define NdisGetNextBuffer(_CurrentBuffer, _NextBuffer) \
          *(_NextBuffer) = (_CurrentBuffer)->Next;

/*++
VOID
NdisSend(
  OUT PNDIS_STATUS Status,
  IN NDIS_HANDLE NdisBindingHandle,
  IN PNDIS_PACKET Packet
);
--*/
#define NdisSend(_S, _B, _P) \
  *(_S) = (*((PNDIS_OPEN_BLOCK)(_B))->SendHandler)( \
            ((PNDIS_OPEN_BLOCK)(_B))->MacBindingHandle, _P)

/*++
VOID
WanMiniportSend(
  OUT PNDIS_STATUS Status,
  IN NDIS_HANDLE NdisBindingHandle,
  IN NDIS_HANDLE NdisLinkHandle,
  IN PNDIS_WAN_PACKET WanPacket
);
--*/
#define WanMiniportSend(Status,												\
						NdisBindingHandle,									\
						NdisLinkHandle,										\
						WanPacket)											\
{																			\
	WanPacket->pProtocolOpenBlock = (PNDIS_OPEN_BLOCK)NdisBindingHandle;	\
	*(Status) =																\
		((((PNDIS_OPEN_BLOCK)(NdisBindingHandle))->WanSendHandler))( 		\
			((PNDIS_OPEN_BLOCK)(NdisBindingHandle))->MacBindingHandle,		\
			(NdisLinkHandle),												\
			(WanPacket));										\
}



/*++
VOID
NdisMSendComplete(
  IN NDIS_HANDLE MiniportAdapterHandle,
  IN PNDIS_PACKET Packet,
  IN NDIS_STATUS Status
);
--*/
#define NdisMSendComplete(_M, _P, _S) \
          (*((PNDIS_MINIPORT_BLOCK)(_M))->SendCompleteHandler)(_M, _P, _S)

extern
VOID
NdisMWanSendComplete(
  IN NDIS_HANDLE MiniportAdapterHandle,
  IN PNDIS_WAN_PACKET Packet,
  IN NDIS_STATUS Status
);

/*++
VOID
NdisMSendResourcesAvailable(
  IN NDIS_HANDLE MiniportAdapterHandle
);
--*/
#define NdisMSendResourcesAvailable(_M) \
  (*((PNDIS_MINIPORT_BLOCK)(_M))->SendResourcesHandler)(_M)

/*++
VOID
NdisTransferData(
  OUT PNDIS_STATUS Status,
  IN NDIS_HANDLE NdisBindingHandle,
  IN NDIS_HANDLE MacReceiveContext,
  IN UINT ByteOffset,
  IN UINT BytesToTransfer,
  IN OUT PNDIS_PACKET Packet,
  OUT PUINT BytesTransferred
);
--*/
#define NdisTransferData(_S, _B, _C, _Offset, _ToTransfer, _P, Transferred) \
          *(_S) = (*((PNDIS_OPEN_BLOCK)(_B))->TransferDataHandler)( \
                    ((PNDIS_OPEN_BLOCK)(_B))->MacBindingHandle, \
                    _C, _Offset, _ToTransfer, _P, Transferred)

/*++
VOID
NdisRequest(
  OUT PNDIS_STATUS Status,
  IN NDIS_HANDLE NdisBindingHandle,
  IN PNDIS_REQUEST NdisRequest
);
--*/
#define NdisRequest(_S, _B, _Request) \
          *(_S) = (*((PNDIS_OPEN_BLOCK)(_B))->MacHandle \
                      ->MacCharacteristics.RequestHandler)( \
                    ((PNDIS_OPEN_BLOCK)(_B))->MacBindingHandle, _Request);

/*++
VOID
NdisMEthIndicateReceive(
  IN NDIS_HANDLE MiniportAdapterHandle,
  IN NDIS_HANDLE MiniportReceiveContext,
  IN PVOID HeaderBuffer,
  IN UINT HeaderBufferSize,
  IN PVOID LookaheadBuffer,
  IN UINT LookaheadBufferSize,
  IN UINT PacketSize
);
--*/
#define NdisMEthIndicateReceive( \
          _M, _C, _HBuffer, _HSize, _LBuffer, _LSize, _PSize) \
          EthFilterDprIndicateReceive( \
            ((PNDIS_MINIPORT_BLOCK)(_M))->EthDB, \
            _C, _HBuffer, _HBuffer, _HSize, _LBuffer, _LSize, _PSize)

/*++
VOID
NdisMEthIndicateReceiveComplete(
  IN NDIS_HANDLE MiniportHandle
);
--*/
#define NdisMEthIndicateReceiveComplete(_M) \
          EthFilterDprIndicateReceiveComplete( \
            ((PNDIS_MINIPORT_BLOCK)_M)->EthDB)


/*++

VOID
NdisMTrIndicateReceive(
	IN	NDIS_HANDLE				MiniportAdapterHandle,
	IN	NDIS_HANDLE				MiniportReceiveContext,
	IN	PVOID					HeaderBuffer,
	IN	UINT					HeaderBufferSize,
	IN	PVOID					LookaheadBuffer,
	IN	UINT					LookaheadBufferSize,
	IN	UINT					PacketSize
	)

--*/
#define NdisMTrIndicateReceive( _H, _C, _B, _SZ, _L, _LSZ, _PSZ)				\
{																				\
	TrFilterDprIndicateReceive(						\
		((PNDIS_MINIPORT_BLOCK)(_H))->TrDB,										\
		_C,																		\
		_B,																		\
		_SZ,																	\
		_L,																		\
		_LSZ,																	\
		_PSZ																	\
		);																		\
}

/*++

VOID
NdisMTrIndicateReceiveComplete(
	IN	NDIS_HANDLE				MiniportHandle
	);

--*/

#define NdisMTrIndicateReceiveComplete( _H )									\
{																				\
	TrFilterDprIndicateReceiveComplete(						\
										((PNDIS_MINIPORT_BLOCK)_H)->TrDB);		\
}






/*++
VOID
NdisMResetComplete(
  IN NDIS_HANDLE MiniportAdapterHandle,
  IN NDIS_STATUS Status,
  IN BOOLEAN AddressingReset
);
--*/
#define NdisMResetComplete(_M, _S, _A) \
          (*((PNDIS_MINIPORT_BLOCK)(_M))->ResetCompleteHandler)(_M, _S, _A)

EXPORT NDIS_STATUS NDISAPI
NdisAllocateMemory(
  OUT PVOID *VirtualAddress,
  IN UINT Length,
  IN UINT MemoryFlags,
  IN NDIS_PHYSICAL_ADDRESS HighestAcceptableAddress
);

EXPORT VOID NDISAPI
NdisFreeMemory(
  IN PVOID VirtualAddress,
  IN UINT Length,
  IN UINT MemoryFlags
);

EXPORT VOID NDISAPI
NdisAllocateSpinLock(
  IN PNDIS_SPIN_LOCK SpinLock
);

EXPORT VOID NDISAPI
NdisFreeSpinLock(
  IN PNDIS_SPIN_LOCK SpinLock
);

EXPORT VOID NDISAPI
NdisAcquireSpinLock(
  IN PNDIS_SPIN_LOCK SpinLock
);

EXPORT VOID NDISAPI
NdisReleaseSpinLock(
  IN PNDIS_SPIN_LOCK SpinLock
);

EXPORT VOID NDISAPI
NdisDprAcquireSpinLock(
  IN PNDIS_SPIN_LOCK SpinLock
);

EXPORT VOID NDISAPI
NdisDprReleaseSpinLock(
  IN PNDIS_SPIN_LOCK SpinLock
);

EXPORT VOID NDISAPI
NdisMSleep(
  IN ULONG MicrosecondsToSleep
);

EXPORT VOID NDISAPI
NdisGetCurrentSystemTime(
   OUT PLONGLONG  pSystemTime
);
 
EXPORT VOID NDISAPI
NdisInitializeListHead(
    IN PLIST_ENTRY ListHead
);

#define NdisInterlockedIncrement					InterlockedIncrement
#define NdisInterlockedDecrement					InterlockedDecrement

EXPORT PLIST_ENTRY NDISAPI
NdisInterlockedInsertHeadList(
    IN  PLIST_ENTRY             ListHead,
    IN  PLIST_ENTRY             ListEntry,
    IN  PNDIS_SPIN_LOCK         SpinLock
);

EXPORT PLIST_ENTRY NDISAPI
NdisInterlockedInsertTailList(
    IN  PLIST_ENTRY             ListHead,
    IN  PLIST_ENTRY             ListEntry,
    IN  PNDIS_SPIN_LOCK         SpinLock
);

EXPORT PLIST_ENTRY NDISAPI
NdisInterlockedRemoveHeadList(
    IN  PLIST_ENTRY             ListHead,
    IN  PNDIS_SPIN_LOCK         SpinLock
);

EXPORT BOOLEAN NDISAPI
NdisMSynchronizeWithInterrupt(
  IN PNDIS_MINIPORT_INTERRUPT Interrupt,
  IN PVOID SynchronizeFunction,
  IN PVOID SynchronizeContext
);

EXPORT VOID NDISAPI
NdisMInitializeTimer(
  IN OUT PNDIS_MINIPORT_TIMER Timer,
  IN NDIS_HANDLE MiniportAdapterHandle,
  IN PNDIS_TIMER_FUNCTION TimerFunction,
  IN PVOID FunctionContext
);

EXPORT VOID NDISAPI
NdisMSetTimer(
  IN PNDIS_MINIPORT_TIMER Timer,
  IN UINT MillisecondsToDelay
);

EXPORT VOID NDISAPI
NdisMSetPeriodicTimer(
  IN PNDIS_MINIPORT_TIMER Timer,
  IN UINT MillisecondsPeriod
);

EXPORT VOID NDISAPI
NdisMCancelTimer(
  IN PNDIS_MINIPORT_TIMER Timer,
  OUT PBOOLEAN TimerCancelled
);

/* use NdisMInitializeWrapper for miniport drivers */
EXPORT VOID NDISAPI
NdisInitializeWrapper(
  OUT PNDIS_HANDLE NdisWrapperHandle,
  IN PVOID SystemSpecific1,
  IN PVOID SystemSpecific2,
  IN PVOID SystemSpecific3
);

EXPORT VOID NDISAPI
NdisTerminateWrapper(
  IN NDIS_HANDLE NdisWrapperHandle,
  IN PVOID SystemSpecific
);

EXPORT NDIS_STATUS NDISAPI
NdisMRegisterMiniport(
  IN NDIS_HANDLE NdisWrapperHandle,
  IN PNDIS_MINIPORT_CHARACTERISTICS MiniportCharacteristics,
  IN UINT CharacteristicsLength
);

EXPORT VOID NDISAPI
NdisMSetAttributes(
  IN NDIS_HANDLE MiniportAdapterHandle,
  IN NDIS_HANDLE MiniportAdapterContext,
  IN BOOLEAN BusMaster,
  IN NDIS_INTERFACE_TYPE AdapterType
);

EXPORT VOID NDISAPI
NdisMSetAttributesEx(
  IN NDIS_HANDLE MiniportAdapterHandle,
  IN NDIS_HANDLE MiniportAdapterContext,
  IN UINT CheckForHangTimeInSeconds OPTIONAL,
  IN ULONG AttributeFlags,
  IN NDIS_INTERFACE_TYPE AdapterType
);

EXPORT NDIS_STATUS NDISAPI
NdisMRegisterIoPortRange(
  OUT PVOID *PortOffset,
  IN NDIS_HANDLE MiniportAdapterHandle,
  IN UINT InitialPort,
  IN UINT NumberOfPorts
);

EXPORT VOID NDISAPI
NdisMDeregisterIoPortRange(
  IN NDIS_HANDLE MiniportAdapterHandle,
  IN UINT InitialPort,
  IN UINT NumberOfPorts,
  IN PVOID PortOffset
);

EXPORT NDIS_STATUS NDISAPI
NdisMRegisterInterrupt(
  OUT PNDIS_MINIPORT_INTERRUPT Interrupt,
  IN NDIS_HANDLE MiniportAdapterHandle,
  IN UINT InterruptVector,
  IN UINT InterruptLevel,
  IN BOOLEAN RequestIsr,
  IN BOOLEAN SharedInterrupt,
  IN NDIS_INTERRUPT_MODE InterruptMode
);

EXPORT VOID NDISAPI
NdisMDeregisterInterrupt(
  IN PNDIS_MINIPORT_INTERRUPT Interrupt
);

EXPORT NDIS_STATUS NDISAPI
NdisMMapIoSpace(
  OUT PVOID *VirtualAddress,
  IN NDIS_HANDLE MiniportAdapterHandle,
  IN NDIS_PHYSICAL_ADDRESS PhysicalAddress,
  IN UINT Length
);

EXPORT VOID NDISAPI
NdisMUnmapIoSpace(
  IN NDIS_HANDLE MiniportAdapterHandle,
  IN PVOID VirtualAddress,
  IN UINT Length
);

EXPORT NDIS_STATUS NDISAPI
NdisQueryMapRegisterCount(
  IN NDIS_INTERFACE_TYPE BusType,
  OUT PUINT  MapRegisterCount
);

EXPORT NDIS_STATUS NDISAPI
NdisMAllocateMapRegisters(
  IN NDIS_HANDLE MiniportAdapterHandle,
  IN UINT DmaChannel,
  IN BOOLEAN Dma32BitAddresses,
  IN ULONG PhysicalMapRegistersNeeded,
  IN ULONG MaximumPhysicalMapping
);

EXPORT VOID NDISAPI
NdisMFreeMapRegisters(
  IN NDIS_HANDLE MiniportAdapterHandle
);

EXPORT VOID NDISAPI
NdisMStartBufferPhysicalMapping(
  IN NDIS_HANDLE MiniportAdapterHandle,
  IN PNDIS_BUFFER Buffer,
  IN ULONG PhysicalMapRegister,
  IN BOOLEAN WriteToDevice,
  OUT PNDIS_PHYSICAL_ADDRESS_UNIT PhysicalAddressArray,
  OUT PUINT ArraySize
);

EXPORT VOID NDISAPI
NdisMCompleteBufferPhysicalMapping(
  IN NDIS_HANDLE MiniportAdapterHandle,
  IN PNDIS_BUFFER Buffer,
  IN ULONG PhysicalMapRegister
);

EXPORT VOID NDISAPI
NdisMAllocateSharedMemory(
  IN NDIS_HANDLE  MiniportAdapterHandle,
  IN ULONG  Length,
  IN BOOLEAN  Cached,
  OUT PVOID *VirtualAddress,
  OUT PNDIS_PHYSICAL_ADDRESS PhysicalAddress
);

EXPORT VOID NDISAPI
NdisMFreeSharedMemory(
  IN NDIS_HANDLE MiniportAdapterHandle,
  IN ULONG Length,
  IN BOOLEAN Cached,
  IN PVOID VirtualAddress,
  IN NDIS_PHYSICAL_ADDRESS PhysicalAddress
);

EXPORT ULONG NDISAPI
NdisReadPciSlotInformation(
  IN NDIS_HANDLE NdisAdapterHandle,
  IN ULONG SlotNumber,
  IN ULONG Offset,
  IN PVOID Buffer,
  IN ULONG Length
);

EXPORT ULONG NDISAPI
NdisWritePciSlotInformation(
  IN NDIS_HANDLE NdisAdapterHandle,
  IN ULONG SlotNumber,
  IN ULONG Offset,
  IN PVOID Buffer,
  IN ULONG Length
);

EXPORT NDIS_STATUS NDISAPI
NdisMPciAssignResources(
  IN NDIS_HANDLE MiniportHandle,
  IN ULONG SlotNumber,
  OUT PNDIS_RESOURCE_LIST *AssignedResources
);

EXPORT VOID NDISAPI
NdisRegisterProtocol(
  OUT PNDIS_STATUS Status,
  OUT PNDIS_HANDLE NdisProtocolHandle,
  IN PNDIS_PROTOCOL_CHARACTERISTICS ProtocolCharacteristics,
  IN UINT CharacteristicsLength
);

EXPORT VOID NDISAPI
NdisDeregisterProtocol(
  OUT PNDIS_STATUS Status,
  IN NDIS_HANDLE NdisProtocolHandle
);

EXPORT VOID NDISAPI
NdisOpenConfiguration(
  OUT PNDIS_STATUS Status,
  OUT PNDIS_HANDLE ConfigurationHandle,
  IN NDIS_HANDLE WrapperConfigurationContext
);

EXPORT VOID NDISAPI
NdisOpenProtocolConfiguration(
  OUT PNDIS_STATUS Status,
  OUT PNDIS_HANDLE ConfigurationHandle,
  IN PNDIS_STRING ProtocolSection
);

EXPORT VOID NDISAPI
NdisCloseConfiguration(
  IN NDIS_HANDLE ConfigurationHandle
);

EXPORT VOID NDISAPI
NdisReadConfiguration(
  OUT PNDIS_STATUS Status,
  OUT PNDIS_CONFIGURATION_PARAMETER *ParameterValue,
  IN NDIS_HANDLE ConfigurationHandle,
  IN PNDIS_STRING Keyword,
  IN NDIS_PARAMETER_TYPE ParameterType
);

EXPORT VOID NDISAPI
NdisWriteConfiguration(
  OUT PNDIS_STATUS Status,
  IN NDIS_HANDLE ConfigurationHandle,
  IN PNDIS_STRING Keyword,
  IN PNDIS_CONFIGURATION_PARAMETER ParameterValue
);

EXPORT VOID NDISAPI
NdisReadNetworkAddress(
  OUT PNDIS_STATUS Status,
  OUT PVOID *NetworkAddress,
  OUT PUINT NetworkAddressLength,
  IN NDIS_HANDLE ConfigurationHandle
);

EXPORT VOID NDISAPI
NdisAllocateBufferPool(
  OUT PNDIS_STATUS Status,
  OUT PNDIS_HANDLE PoolHandle,
  IN UINT NumberOfDescriptors
);

EXPORT VOID NDISAPI
NdisFreeBufferPool(
  IN NDIS_HANDLE PoolHandle
);

EXPORT VOID NDISAPI
NdisAllocateBuffer(
  OUT PNDIS_STATUS Status,
  OUT PNDIS_BUFFER *Buffer,
  IN NDIS_HANDLE PoolHandle,
  IN PVOID VirtualAddress,
  IN UINT Length
);

EXPORT VOID NDISAPI
NdisFreeBuffer(
  IN PNDIS_BUFFER Buffer
);

EXPORT VOID NDISAPI
NdisAdjustBufferLength(
  IN PNDIS_BUFFER Buffer,
  IN UINT Length
);

EXPORT VOID NDISAPI
NdisQueryBuffer(
  IN PNDIS_BUFFER Buffer,
  OUT PVOID *VirtualAddress OPTIONAL,
  OUT PUINT Length
);

EXPORT VOID NDISAPI
NdisQueryBufferOffset(
  IN PNDIS_BUFFER Buffer,
  OUT PUINT Offset,
  OUT PUINT Length
);

EXPORT ULONG NDISAPI
NDIS_BUFFER_TO_SPAN_PAGES(
  IN PNDIS_BUFFER Buffer
);

EXPORT VOID NDISAPI
NdisUnchainBufferAtFront(
  IN OUT PNDIS_PACKET Packet,
  OUT PNDIS_BUFFER *Buffer
);

EXPORT VOID NDISAPI
NdisUnchainBufferAtBack(
  IN OUT PNDIS_PACKET Packet,
  OUT PNDIS_BUFFER *Buffer
);

EXPORT VOID NDISAPI
NdisAllocatePacketPool(
  OUT PNDIS_STATUS Status,
  OUT PNDIS_HANDLE PoolHandle,
  IN UINT NumberOfDescriptors,
  IN UINT ProtocolReservedLength
);

EXPORT VOID NDISAPI
NdisFreePacketPool(
  IN NDIS_HANDLE PoolHandle
);

EXPORT VOID NDISAPI
NdisAllocatePacket(
  OUT PNDIS_STATUS Status,
  OUT PNDIS_PACKET *Packet,
  IN NDIS_HANDLE PoolHandle
);

EXPORT VOID NDISAPI
NdisFreePacket(
  IN PNDIS_PACKET Packet
);

EXPORT VOID NDISAPI
NdisCopyFromPacketToPacket(
  IN PNDIS_PACKET Destination,
  IN UINT DestinationOffset,
  IN UINT BytesToCopy,
  IN PNDIS_PACKET Source,
  IN UINT SourceOffset,
  OUT PUINT BytesCopied
);

EXPORT VOID NDISAPI
NdisInitializeString(
  OUT PNDIS_STRING Destination,
  IN PUCHAR Source
);

EXPORT VOID NDISAPI
NdisInitAnsiString(
  IN OUT PANSI_STRING DestinationString,
  IN PCSTR SourceString
);

EXPORT VOID NDISAPI
NdisInitUnicodeString(
  IN OUT PUNICODE_STRING DestinationString,
  IN PCWSTR SourceString
);

EXPORT NDIS_STATUS NDISAPI
NdisAnsiStringToUnicodeString(
  IN OUT PUNICODE_STRING DestinationString,
  IN PANSI_STRING SourceString
);

EXPORT NDIS_STATUS NDISAPI
NdisUnicodeStringToAnsiString(
  IN OUT PANSI_STRING DestinationString,
  IN PUNICODE_STRING SourceString
);

EXPORT BOOLEAN NDISAPI
NdisEqualString(
  IN PNDIS_STRING String1,
  IN PNDIS_STRING String2,
  IN BOOLEAN CaseInsensitive
);

EXPORT VOID NDISAPI
NdisMTransferDataComplete(
  IN NDIS_HANDLE MiniportAdapterHandle,
  IN PNDIS_PACKET Packet,
  IN NDIS_STATUS Status,
  IN UINT BytesTransferred
);

EXPORT VOID NDISAPI
NdisOpenAdapter(
  OUT PNDIS_STATUS Status,
  OUT PNDIS_STATUS OpenErrorStatus,
  OUT PNDIS_HANDLE NdisBindingHandle,
  OUT PUINT SelectedMediumIndex,
  IN PNDIS_MEDIUM MediumArray,
  IN UINT MediumArraySize,
  IN NDIS_HANDLE NdisProtocolHandle,
  IN NDIS_HANDLE ProtocolBindingContext,
  IN PNDIS_STRING AdapterName,
  IN UINT OpenOptions,
  IN PSTRING AddressingInformation OPTIONAL
);

EXPORT VOID NDISAPI
NdisCloseAdapter(
  OUT PNDIS_STATUS Status,
  IN NDIS_HANDLE NdisBindingHandle
);

EXPORT VOID NDISAPI
NdisRegisterAdapter(
	OUT PNDIS_STATUS	Status,
	IN	PWCHAR			MiniportName,
	IN	PWCHAR			AdapterInstanceName);

EXPORT VOID NDISAPI
NdisDeregisterAdapter(
	OUT PNDIS_STATUS	pStatus,
	IN	PWCHAR			wszAdapterInstanceName);

EXPORT VOID NDISAPI
NdisRebindProtocolsToAdapter(
	OUT	PNDIS_STATUS	pStatus,
	IN	PWCHAR			wszAdapterInstanceName,
	IN	PWCHAR			wszProtocolName			OPTIONAL);

EXPORT VOID NDISAPI
NdisBindProtocolsToAdapter(
	OUT	PNDIS_STATUS	pStatus,
	IN	PWCHAR			wszAdapterInstanceName,
	IN	PWCHAR			wszProtocolName			OPTIONAL);

EXPORT VOID NDISAPI
NdisUnbindProtocolsFromAdapter(
	OUT	PNDIS_STATUS	pStatus,
	IN	PWCHAR			wszAdapterInstanceName,
	IN	PWCHAR			wszProtocolName			OPTIONAL);

EXPORT VOID NDISAPI
NdisReturnPackets(
  IN PNDIS_PACKET *PacketsToReturn,
  IN UINT NumberOfPackets
);

EXPORT VOID NDISAPI
NdisReset(
  OUT PNDIS_STATUS Status,
  IN NDIS_HANDLE NdisBindingHandle
);

EXPORT VOID NDISAPI
NdisMIndicateStatus(
  IN NDIS_HANDLE MiniportAdapterHandle,
  IN NDIS_STATUS GeneralStatus,
  IN PVOID StatusBuffer,
  IN UINT StatusBufferSize
);

EXPORT VOID NDISAPI
NdisMIndicateStatusComplete(
  IN NDIS_HANDLE MiniportAdapterHandle
);

EXPORT VOID NDISAPI
NdisMQueryInformationComplete(
  IN NDIS_HANDLE MiniportAdapterHandle,
  IN NDIS_STATUS Status
);

EXPORT VOID NDISAPI
NdisMSetInformationComplete(
  IN NDIS_HANDLE MiniportAdapterHandle,
  IN NDIS_STATUS Status
);

EXPORT VOID NDISAPI
NdisMRebindProtocolsToAdapter(
	IN NDIS_HANDLE MiniportAdapterHandle
);


/* dummy routines */
EXPORT VOID NDISAPI
NdisWriteErrorLogEntry(
  IN NDIS_HANDLE NdisAdapterHandle,
  IN NDIS_ERROR_CODE ErrorCode,
  IN ULONG NumberOfErrorValues,
  ...
);

EXPORT VOID NDISAPI
NdisMRegisterAdapterShutdownHandler(
  IN NDIS_HANDLE MiniportHandle,
  IN PVOID ShutdownContext,
  IN ADAPTER_SHUTDOWN_HANDLER ShutdownHandler
);

EXPORT VOID NDISAPI
NdisMDeregisterAdapterShutdownHandler(
  IN NDIS_HANDLE MiniportHandle
);

EXPORT VOID NDISAPI
NdisMWanIndicateReceive(
	OUT PNDIS_STATUS					Status,
	IN	NDIS_HANDLE						MiniportAdapterHandle,
	IN	NDIS_HANDLE						NdisLinkContext,
	IN	PUCHAR							Packet,
	IN	ULONG							PacketSize
	);

EXPORT VOID NDISAPI
NdisMWanIndicateReceiveComplete(
	IN	NDIS_HANDLE						MiniportAdapterHandle,
	IN	NDIS_HANDLE						NdisLinkContext
	);

//
// Routines for intermediate miniport drivers.
//

EXPORT
BOOLEAN NDISAPI
NdisIMSwitchToMiniport(
    IN  NDIS_HANDLE             MiniportAdapterHandle,
    OUT PNDIS_HANDLE            SwitchHandle
    );

EXPORT
VOID NDISAPI
NdisIMRevertBack(
    IN  NDIS_HANDLE             MiniportAdapterHandle,
    IN  NDIS_HANDLE             SwitchHandle
    );


EXPORT
NDIS_STATUS
NdisIMRegisterLayeredMiniport(
	IN	NDIS_HANDLE				NdisWrapperHandle,
	IN	PNDIS_MINIPORT_CHARACTERISTICS MiniportCharacteristics,
	IN	UINT					CharacteristicsLength,
	OUT PNDIS_HANDLE			DriverHandle
	);

NDIS_STATUS
NdisIMInitializeDeviceInstanceEx(
	IN	NDIS_HANDLE		DriverHandle,
	IN	PNDIS_STRING	DeviceInstanceName,
	IN	NDIS_HANDLE		DeviceContext
	);

NDIS_STATUS
NdisIMInitializeDeviceInstance(
	IN	NDIS_HANDLE		DriverHandle,
	IN	PNDIS_STRING	DeviceInstance
	);

NDIS_STATUS
NdisIMDeInitializeDeviceInstance(
	IN	NDIS_HANDLE				NdisMiniportHandle
	);

NDIS_HANDLE
NdisIMGetDeviceContext(
	IN	NDIS_HANDLE				MiniportAdapterHandle
    );

/*++

VOID
NdisCreateLookaheadBufferFromSharedMemory(
	IN	PVOID					pSharedMemory,
	IN	UINT					LookaheadLength,
	OUT PVOID *					pLookaheadBuffer
	);

--*/

#define NdisCreateLookaheadBufferFromSharedMemory(_S, _L, _B)	((*(_B)) = (_S))

/*++

VOID
NdisDestroyLookaheadBufferFromSharedMemory(
	IN	PVOID					pLookaheadBuffer
	);

--*/

#define NdisDestroyLookaheadBufferFromSharedMemory(_B)



#if defined(_M_IX86)
#define NdisMoveMappedMemory(_Destination, _Source, _Length) \
          RtlCopyMemory(_Destination, _Source, _Length)
#define NdisZeroMappedMemory(_Destination, _Length) \
          RtlZeroMemory(_Destination, _Length)
#else

#define NdisMoveMappedMemory(Destination,Source,Length)						\
{																			\
	PUCHAR _Src = (Source);													\
	PUCHAR _Dest = (Destination);											\
	PUCHAR _End = _Dest + (Length);											\
	while (_Dest < _End)													\
	{																		\
		*_Dest++ = *_Src++;													\
	}																		\
}

#define NdisZeroMappedMemory(Destination,Length)							\
{																			\
	PUCHAR _Dest = (Destination);											\
	PUCHAR _End = _Dest + (Length);											\
	while (_Dest < _End)													\
	{																		\
		*_Dest++ = 0;														\
	}																		\
}

#endif /* _M_IX86 */

#define NdisMoveToMappedMemory(_Destination, _Source, _Length) \
          NdisMoveMappedMemory(_Destination, _Source, _Length)
#define NdisMoveFromMappedMemory(_Destination, _Source, _Length) \
          NdisMoveMappedMemory(_Destination, _Source, _Length)

/* ASSERT and ASSERTMSG are standard NT DDK macros
   and should not be defined anywhere else */
#if BINARY_COMPATIBLE
#if defined(ASSERT)
#undef ASSERT
#endif
#if defined(ASSERTMSG)
#undef ASSERTMSG
#endif
#endif

#if DBG
#ifndef KdBreakPoint
#define KdBreakPoint() DbgBreakPoint()
#endif
#define KdPrint(_x) DbgPrint _x
#ifndef KdPrint
#endif

VOID
RtlAssert(
  IN PVOID FailedAssertion,
  IN PVOID FileName,
  IN ULONG LineNumber,
  IN PCHAR Message OPTIONAL
);
#if !defined(ASSERT)
#define ASSERT(_exp) \
          if (!(_exp)) {\
            RtlAssert(#_exp, __FILE__, __LINE__, NULL); \
          }
#endif
#if !defined(ASSERTMSG)
#define ASSERTMSG(_msg, _exp) \
          if (!(_exp)) {\
            RtlAssert(#_exp, __FILE__, __LINE__, _msg); \
          }
#endif
#else /* DBG */
#define KdBreakPoint()
#define KdPrint(_x)

#if !defined(ASSERT)
#define ASSERT(_exp)
#endif
#if !defined(ASSERTMSG)
#define ASSERTMSG(_exp, _msg)
#endif
#endif /* DBG */

#endif /* _NDIS_H_ */
