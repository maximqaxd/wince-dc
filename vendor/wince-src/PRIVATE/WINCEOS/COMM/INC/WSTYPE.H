/**********************************************************************/
/**                        Microsoft Windows                         **/
/**Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.**/
/**********************************************************************/

/*
    wstype.h

	This file contains global type definitons for the AFVXD VxD.


*/


#ifndef _WSTYPE_H_
#define _WSTYPE_H_


//
//  Enumerated type for system capacity.
//

typedef enum _SYSTEM_SIZE
{
    SmallSystem = 1,
    MediumSystem,
    LargeSystem

} SYSTEM_SIZE, FAR * LPSYSTEM_SIZE;


//
//  Structure signatures are only defined in DEBUG builds.
//

#ifdef DEBUG
#define DEBUG_SIGNATURE DWORD Signature;
#else   // !DEBUG
#define DEBUG_SIGNATURE
#endif  // DEBUG


//
//  Make life with TdiQueryInformation( TDI_QUERY_ADDRESS_INFO )
//  a little easier...
//

typedef struct _TA_IP_ADDRESS_INFO
{
    ULONG ActivityCount;
    TA_IP_ADDRESS IpAddr;

} TA_IP_ADDRESS_INFO, FAR * LPTA_IP_ADDRESS_INFO;


//
//  A TCP/UDP port.
//

typedef WORD    PORT, FAR * LPPORT;


//
//  Forward references.
//

typedef struct _SOCK_INFO      SOCK_INFO,      FAR * LPSOCK_INFO;
typedef struct _VXD_BUFFER     VXD_BUFFER,     FAR * LPVXD_BUFFER;
typedef struct _VXD_QUEUES     VXD_QUEUES,     FAR * LPVXD_QUEUES;
typedef struct _VXD_ENDPOINT   VXD_ENDPOINT,   FAR * LPVXD_ENDPOINT;
typedef struct _VXD_CONNECTION VXD_CONNECTION, FAR * LPVXD_CONNECTION;
typedef struct _VXD_TRACKER    VXD_TRACKER,    FAR * LPVXD_TRACKER;
typedef struct _VXD_STACK      VXD_STACK,      FAR * LPVXD_STACK;


// when declaring an array that has a variable size, use this
// as a placeholder.
#define ANYSIZE_ARRAY   1



//
// One of these is allocated for each stack VxD.
// This information was originally stored in "globals.c."
//


typedef struct _VXD_STACK
{

//
//  Structure signature, for safety's sake.
//

    DEBUG_SIGNATURE

//
// 	The associated transport's device name
//

    PWSTR                   TransportName;

//
//  The list of address family/socket type/protocol triples
//  supported by this stack DLL.
//

    PWINSOCK_MAPPING        Mapping;


//
//  TDI Dispatch Table for the transport.
//

    TDIDispatchTable      * TdiDispatch;

//
//  Address size information.
//

    DWORD                   MinSockaddrLength;
    DWORD                   MaxSockaddrLength;
    DWORD                   MinTdiAddressLength;
    DWORD                   MaxTdiAddressLength;

//
//  Linked lists of all sockets, endpoints, and connections
//  created by this VxD.
//

    LIST_ENTRY              OpenSocketList;
    LIST_ENTRY              OpenEndpointList;
    LIST_ENTRY              OpenConnectionList;

//
//  Linked list of buffer objects associated with failed sends.
//  Whenever a send fails due to insufficient resources within
//  the TCP/IP stack, we put the buffer on this queue & retry
//  later.
//

    LIST_ENTRY              SendRetryList;

//
//  Count of pending sends.
//

    DWORD                   PendingSendCount;

//
//  Linked list of socket objects awaiting buffer space.  Blocking
//  sockets are enqueued here if they fail to acquire sufficient
//  buffer space during send().
//

    LIST_ENTRY              SendsWaitingForBuffersList;
	
    WCHAR                   Names[1];

}VXD_STACK,	FAR * LPVXD_STACK;

#ifdef DEBUG
#define STACK_SIGNATURE          (DWORD)'katS'
#define STACK_SIGNATURE_X        (DWORD)'katX'
#define INIT_STACK_SIG(h)        ((h)->Signature = STACK_SIGNATURE)
#define KILL_STACK_SIG(h)        ((h)->Signature = STACK_SIGNATURE_X)
#define IS_VALID_STACK(h)        (((h) != NULL) && ((h)->Signature == STACK_SIGNATURE))
#else   // !DEBUG
#define INIT_STACK_SIG(h)        (void)(h)
#define KILL_STACK_SIG(h)        (void)(h)
#define IS_VALID_STACK(h)        ((void)(h), TRUE)
#endif  // DEBUG

#define NAMES_TO_VXD_STACK(n) CONTAINING_RECORD((n),VXD_STACK,Names[0])
#define STACK_FROM_SOCKET(s) (s)->Stack
#define SOCKET_SET_STACK(s,h) ( (s)->Stack = (h) )
#define STACK_FROM_CONNECTION(c) (c)->Stack
#define CONNECTION_SET_STACK(c,h) ( (c)->Stack = (h) )
#define STACK_FROM_ENDPOINT(e) (e)->Stack
#define ENDPOINT_SET_STACK(e,h) ( (e)->Stack = (h) )
#define STACK_FROM_QUEUE(q) (q)->OwningSocket->Stack

//
//  A buffer object.
//

typedef struct _VXD_BUFFER
{
    //
    //  Structure signature, for safety's sake.
    //

    DEBUG_SIGNATURE

    //
    //  Buffer queue links.  The semantics of these links depends
    //  on the implied state of the buffer (free, acquired for send,
    //  or acquired for recv):
    //
    //  Free:
    //
    //      GlobalFreeBufferList - Links the buffer into a global pool
    //          of free buffer objects.
    //
    //  Acquired for send():
    //
    //      SendRetryBufferList - If the send failed due to insufficient
    //          resources within TDI, this links the buffer into a
    //          global pool of failed send requests awaiting retry.
    //          Otherwise, this field is unused.
    //
    //      SendPendingBufferList - Links the buffer into a per-socket
    //          pool of pending send requests.
    //
    //  Acquired for recv():
    //
    //      RecvGenericBufferList - Links the buffer into a per-socket
    //          pool of all received data.
    //
    //      RecvSpecificBufferList - Links the buffer into a per-socket
    //          pool of all received data of the same type (normal vs.
    //          expedited).
    //

    LIST_ENTRY          BufferList0;
    LIST_ENTRY          BufferList1;

#define GlobalFreeBufferList    BufferList0

#define SendRetryBufferList     BufferList0
#define SendPendingBufferList   BufferList1

#define RecvGenericBufferList   BufferList0
#define RecvSpecificBufferList  BufferList1

    //
    //  The owning buffer list.  When this object is released,
    //  it will be put back on the list indicated by this pointer.
    //
    //  If this field is NULL, then this object does not belong
    //  on any buffer list.  When released, instead of being enqueued
    //  onto the appropriate buffer list, it will be freed.
    //

    PLIST_ENTRY         OwningBufferList;

    //
    //  The free count for buffers of this size.  This field is
    //  NULL for objects that don't belong on any buffer list.
    //

    LPDWORD             FreeCount;

    //
    //  The socket that owns this buffer.
    //

    LPSOCK_INFO         OwningSocket;

    //
    //  Private flags.
    //
    //  BUFF_FLAG_EXPEDITED will be set if this buffer contains
    //  expedited data.
    //

    DWORD               Flags;

    //
    //  The size of the valid data within this buffer object.
    //

    DWORD               ValidDataLength;

    //
    //  The total number of BYTEs allocated to this
    //  buffer object.
    //

    DWORD               AllocatedLength;

    //
    //  Head & tail pointers/offsets.  Data is written to the
    //  buffer @ the head, and removed @ the tail.  When the
    //  tail "catches up" with the head, the buffer object
    //  (and associated storage objects) should be released.
    //

    DWORD               TailOffset;

    //
    //  The local or remote address (for datagrams only).
    //

    PTRANSPORT_ADDRESS  DatagramAddress;

    //
    //  Information for TDI send/receive requests.
    //

    TDI_CONNECTION_INFORMATION  TdiConnectionInfo;

    //
    //  TDI receive event context for asynchronous copy completion.
    //

    EventRcvBuffer      TdiReceiveBuffer;

    //
    //  Pre-created NDIS_BUFFER structure.
    //

    NDIS_BUFFER         NdisBuffer;

    //
    //  The actual buffer.
    //

    BYTE                *Buffer;

    //
    // Holds DatagramAddress and the actual Buffer.
    // 

    BYTE                Data[ANYSIZE_ARRAY];

};

#define VXD_BUFFER_SIZE ( sizeof(VXD_BUFFER) + gMaxTdiAddressLength - ANYSIZE_ARRAY )

#ifdef DEBUG
#define BUFF_SIGNATURE          (DWORD)'fFuB'
#define BUFF_SIGNATURE_X        (DWORD)'fubX'
#define INIT_BUFF_SIG(p)        ((p)->Signature = BUFF_SIGNATURE)
#define KILL_BUFF_SIG(p)        ((p)->Signature = BUFF_SIGNATURE_X)
#define IS_VALID_BUFF(p)        (((p) != NULL) && ((p)->Signature == BUFF_SIGNATURE))
#else   // !DEBUG
#define INIT_BUFF_SIG(p)        (void)(p)
#define KILL_BUFF_SIG(p)        (void)(p)
#define IS_VALID_BUFF(p)        ((void)(p), TRUE)
#endif  // DEBUG


//
//  Send & receive queues.
//

typedef struct _VXD_QUEUES
{
    //
    //  Structure signature, for safety's sake.
    //

    DEBUG_SIGNATURE

    //
    //  The owning socket.  This is used to add a new
    //  reference to the socket just before we issue
    //  a receive request to TDI to retrieve data
    //  buffered within the transport.
    //

    LPSOCK_INFO         OwningSocket;

    //
    //  Used to implement send/receive buffer quotas.
    //

    DWORD               SendBufferSizeQuota;
    DWORD               SendBytesPending;

    DWORD               ReceiveBufferSizeQuota;
    DWORD               ReceiveBytesQueued;

    //
    //  The number of BYTEs queued for recv() in the transport.
    //

    DWORD               ReceiveNormalBytes;
    DWORD               ReceiveNormalFlags;
    DWORD               ReceiveExpeditedBytes;
    DWORD               ReceiveExpeditedFlags;

    //
    //  The send & receive buffers queues.
    //
    //  All sends pending with TDI are queued onto SendPendingQueue.
    //
    //  All receive data is queued onto RecvGenericQueue.  All
    //  normal data is queued onto RecvNormalQueue, all expedited
    //  data is queue onto RecvExpeditedQueue.
    //

    LIST_ENTRY          SendPendingQueue;
    LIST_ENTRY          RecvGenericQueue;
    LIST_ENTRY          RecvNormalQueue;
    LIST_ENTRY          RecvExpeditedQueue;

    //
    //  Additional context for receive completion.
    //
    //  CONNECTION-ORIENTED SOCKETS ONLY
    //

    LPVOID              RecvContext;

};

#ifdef DEBUG
#define QUEU_SIGNATURE          (DWORD)'uEuQ'
#define QUEU_SIGNATURE_X        (DWORD)'euqX'
#define INIT_QUEU_SIG(p)        ((p)->Signature = QUEU_SIGNATURE)
#define KILL_QUEU_SIG(p)        ((p)->Signature = QUEU_SIGNATURE_X)
#define IS_VALID_QUEU(p)        (((p) != NULL) && ((p)->Signature == QUEU_SIGNATURE))
#else   // !DEBUG
#define INIT_QUEU_SIG(p)        (void)(p)
#define KILL_QUEU_SIG(p)        (void)(p)
#define IS_VALID_QUEU(p)        ((void)(p), TRUE)
#endif  // DEBUG


//
//  Define a socket descriptor as viewed by this provider VxD.
//

typedef struct _SOCK_INFO
{
    //
    //  Structure signature, for safety's sake.
    //

    DEBUG_SIGNATURE

    LIST_ENTRY  si_socket_list;     // list of all active sockets
	LIST_ENTRY	si_notify_list;		// list of notification objects PEGNOTIY for us
	DWORD		si_error;			// we put errors which crop up, here...

    LINGER      si_linger;          // linger options
    LPVOID      si_localaddr;       // local address for this socket
    LPVOID      si_remoteaddr;      // size of the local address
    DWORD       si_localaddrlen;    // remote (peer) address for this socket
    DWORD       si_remoteaddrlen;   // size of the remote address
    DWORD       si_family;          // address family
    DWORD       si_type;            // socket type
    DWORD       si_protocol;        // protocol
    DWORD       si_sendbufsize;     // send buffer size
    DWORD       si_recvbufsize;     // receive buffer size
    DWORD       si_flags;           // internal state status
    DWORD       si_options;         // setsockopt() boolean options
    DWORD       si_max_connects;    // max number of connects outstanding
    DWORD       si_num_connects;    // number of connects waiting to be accepted
    DWORD       si_state;           // current state
    DWORD       si_ready;           // events ready to notify
    DWORD       si_disabled_events; // disabled [async]select events
    DWORD       si_owner_pid;       // owning process id (VM handle in 16 bits)
									// PEG- This has the ProcessID of the owner
    DWORD       si_handle;          // this socket's handle
	DWORD       si_recvtimeout;		// receive timeout (ms)
	DWORD       si_sendtimeout;		// send timeout (ms)


	//
	// The stack associated with this socket
	//

	LPVXD_STACK		Stack;

//
//  The bit mask of notification events in which the stack
//  VxD is interested.
//

    DWORD               NotificationEvents;

//
//  The Stack DLL's socket context.
//

    PVOID               StackSocketContext;

    //
    //  Global list of open sockets.
    //

    LIST_ENTRY          OpenSocketList;

    //
    //  Reference count.  This is the number of outstanding
    //  reasons why we cannot delete this structure.  When
    //  this value drops to zero, the structure gets deleted.
    //

    DWORD               References;

    //
    //  Private flags.
    //

    DWORD               Flags;

    //
    //  The endpoint associated with this socket.
    //

    LPVXD_ENDPOINT      Endpoint;

    //
    //  The active connection for this socket (if any).
    //

    LPVXD_CONNECTION    ActiveConnection;

    //
    //  The send/receive queues associated with this socket.
    //  These queues will by owned by either the Endpoint
    //  (for datagram sockets) or the ActiveConnection (for
    //  connected stream sockets).
    //

    LPVXD_QUEUES        SendReceiveQueues;

    //
    //  The following fields are used to restart a send that
    //  was blocked due to insufficient buffer space or buffer
    //  quota.
    //

    LIST_ENTRY          SendPendingList;
    LPBYTE              SendPendingBuffer;
    DWORD               SendPendingBufferLength;
    DWORD               SendPendingFlags;
    //ULONG               SendPendingIpAddress;
    //USHORT              SendPendingIpPort;

    PTRANSPORT_ADDRESS  SendPendingAddress;

    PTRANSPORT_ADDRESS  ScratchAddress;

    BOOL                SendPendingAddressValid;

    //
    //  IP Multicast support.
    //

    DWORD               MulticastTtl;
    DWORD               MulticastIf;

    BOOL                InRecv;                 /* wmz */
    int                 RecvLen;                /* wmz */
    NDIS_BUFFER         RecvNDISBuff;           /* wmz */

    //
    // Holds SendPendingAddress.
    //

    BYTE                Data[ANYSIZE_ARRAY];

#ifdef DEBUG
    DWORD               EndOfSocketSignature;
#endif  // DEBUG
} SOCK_INFO;

//
//  Values for si_state.
//

#define SI_STATE_FIRST          1
#define SI_STATE_OPEN           1
#define SI_STATE_BOUND          2
#define SI_STATE_LISTENING      3
#define SI_STATE_PEND_ACCEPT    4
#define SI_STATE_CONNECTING     5
#define SI_STATE_CONNECTED      6
#define SI_STATE_DISCONNECTED   7
#define SI_STATE_CLOSING        8       // "gracefully" closing
#define SI_STATE_CLOSED         9
#define SI_STATE_NO_PROVIDER	10		// PnP provider unloaded beneath us
#define SI_STATE_LAST           10


//
//  Bit definitions for si_flags.
//

#define SI_FLAG_CONNRESET       0x0001
#define SI_FLAG_CONNDOWN        0x0002	// read data available, but disconnected
#define SI_FLAG_VALID_MASK      0x0003


//
//  Bit definitions for si_options.
//

#define SI_OPT_BROADCAST        0x0001
#define SI_OPT_DEBUG            0x0002
#define SI_OPT_DONTROUTE        0x0004
#define SI_OPT_KEEPALIVE        0x0008
#define SI_OPT_OOBINLINE        0x0010
#define SI_OPT_REUSEADDR        0x0020
#define SI_OPT_STOPSENDS        0x0040
#define SI_OPT_STOPRECVS        0x0080
#define SI_OPT_BLOCKING         0x0100
#define SI_OPT_VALID_MASK       0x01FF


	
#define SOCK_INFO_SIZE \
    ( sizeof ( SOCK_INFO ) \
      + Stack->MaxTdiAddressLength \
      + Stack->MaxTdiAddressLength \
      - ANYSIZE_ARRAY )

#ifdef DEBUG
#define SOCK_SIGNATURE          (DWORD)'kCoS'
#define SOCK_SIGNATURE_X        (DWORD)'cosX'
#define INIT_SOCK_SIG(p)        ((p)->Signature = SOCK_SIGNATURE)
#define KILL_SOCK_SIG(p)        ((p)->Signature = SOCK_SIGNATURE_X)
#define IS_VALID_SOCK(p)        (((p) != NULL) && ((p)->Signature == SOCK_SIGNATURE))
#else   // !DEBUG
#define INIT_SOCK_SIG(p)        (void)(p)
#define KILL_SOCK_SIG(p)        (void)(p)
#define IS_VALID_SOCK(p)        ((void)(p), TRUE)
#endif  // DEBUG


//
//  Possible endpoint states.
//
//  N.B.  For each state, VXD_ENDPOINT_STATE_RECEIVE_OK is set if it is
//        allowable to accept receive data on an endpoint in that state.
//

#define VXD_ENDPOINT_STATE_RECEIVE_OK   0x40000000

typedef enum _VXD_ENDPOINT_STATE
{
    VxdEndpointStateOpen        = 1,
    VxdEndpointStateBound       = 2 | VXD_ENDPOINT_STATE_RECEIVE_OK,
    VxdEndpointStateListening   = 3 | VXD_ENDPOINT_STATE_RECEIVE_OK,
    VxdEndpointStateClosing     = 4

} VXD_ENDPOINT_STATE, FAR * LPVXD_ENDPOINT_STATE;


//
//  An endpoint.
//

typedef struct _VXD_ENDPOINT
{
    //
    //  Structure signature, for safety's sake.
    //

    DEBUG_SIGNATURE


	//
	// The stack associated with this endpoint.
	//

	LPVXD_STACK		Stack;

	//
    //  Global list of active endpoints.
    //

    LIST_ENTRY          OpenEndpointList;

    //
    //  Reference count.  This is the number of outstanding
    //  reasons why we cannot delete this structure.  When
    //  this value drops to zero, the structure gets deleted.
    //

    DWORD               References;

    //
    //  Private flags.
    //

    DWORD               Flags;

    //
    //  The TDI address object associated with this endpoint.
    //

    HANDLE              AddressObject;

    //
    //  Current state.
    //

    VXD_ENDPOINT_STATE  State;

    //
    //  The socket that was bound to this endpoint.
    //

    LPSOCK_INFO         BoundSocket;

    //
    //  The local address for this endpoint.
    //

    PSOCKADDR           LocalAddress;

    DWORD               LocalAddressLength;

    //
    //  The number connection objects that reference this endpoint.
    //
    //  CONNECTION-ORIENTED SOCKETS ONLY
    //

    DWORD               ConnectedConnections;

    //
    //  The number of failed connection object allocations
    //  since the last successful replenish.  Whenever
    //  EndpReplenishIdleConnections is called, it tries to add
    //  this many connection objects to the idle queue.
    //
    //  CONNECTION-ORIENTED SOCKETS ONLY
    //

    DWORD               FailedConnections;

    //
    //  The current & maximum number idle connection objects
    //  queued on this endpoint.
    //
    //  CONNECTION-ORIENTED SOCKETS ONLY
    //

    DWORD               NumQueuedConnections;
    DWORD               MaxQueuedConnections;

    //
    //  Queue of idle connection objects.
    //
    //  CONNECTION-ORIENTED SOCKETS ONLY
    //

    LIST_ENTRY          IdleConnectionQueue;

    //
    //  Queue of active connections objects.
    //
    //  CONNECTION-ORIENTED SOCKETS ONLY
    //

    LIST_ENTRY          ActiveConnectionQueue;

    //
    //  The send & receive queues associated with this endpoint.
    //
    //  DATAGRAM SOCKETS ONLY
    //

    LPVXD_QUEUES        SendReceiveQueues;

    //
    // Holds LocalAddress.
    //

    BYTE                Data[ANYSIZE_ARRAY];

};

#define VXD_ENDPOINT_SIZE ( sizeof(VXD_ENDPOINT) + Stack->MaxSockaddrLength - ANYSIZE_ARRAY )

#ifdef DEBUG
#define ENDP_SIGNATURE          (DWORD)'pDnE'
#define ENDP_SIGNATURE_X        (DWORD)'dneX'
#define INIT_ENDP_SIG(p)        ((p)->Signature = ENDP_SIGNATURE)
#define KILL_ENDP_SIG(p)        ((p)->Signature = ENDP_SIGNATURE_X)
#define IS_VALID_ENDP(p)        (((p) != NULL) && ((p)->Signature == ENDP_SIGNATURE))
#else   // !DEBUG
#define INIT_ENDP_SIG(p)        (void)(p)
#define KILL_ENDP_SIG(p)        (void)(p)
#define IS_VALID_ENDP(p)        ((void)(p), TRUE)
#endif  // DEBUG


//
//  Possible connection states.
//
//  N.B.  For each state, VXD_CONNECT_STATE_NEED_DISCONNECT
//        is set if a connection object in that state requires
//        a disconnect before being destroyed.
//
//        VXD_CONNECT_STATE_RECEIVE_OK is set if data may be
//        received on an connection object in that state.
//
//        VXD_CONNECT_STATE_DEAD is set if the the connection is
//        dead (meaning the peer has disconnected/aborted).
//

#define VXD_CONNECT_STATE_NEED_DISCONNECT   0x40000000
#define VXD_CONNECT_STATE_RECEIVE_OK        0x20000000
#define VXD_CONNECT_STATE_DEAD              0x10000000

typedef enum _VXD_CONNECT_STATE
{
    VxdConnectionStateFree         = 1,
    VxdConnectionStateUnaccepted   = 2 | VXD_CONNECT_STATE_NEED_DISCONNECT
                                       | VXD_CONNECT_STATE_RECEIVE_OK,
    VxdConnectionStateReturned     = 3 | VXD_CONNECT_STATE_NEED_DISCONNECT
                                       | VXD_CONNECT_STATE_RECEIVE_OK,
    VxdConnectionStateConnected    = 4 | VXD_CONNECT_STATE_NEED_DISCONNECT
                                       | VXD_CONNECT_STATE_RECEIVE_OK,
    VxdConnectionStateDisconnected = 5 | VXD_CONNECT_STATE_NEED_DISCONNECT
                                       | VXD_CONNECT_STATE_DEAD,
    VxdConnectionStateSendShutdown = 6 | VXD_CONNECT_STATE_RECEIVE_OK,
    VxdConnectionStateBothShutdown = 7 | VXD_CONNECT_STATE_DEAD,
    VxdConnectionStateAborted      = 8 | VXD_CONNECT_STATE_DEAD,
    VxdConnectionStateClosing      = 9

} VXD_CONNECT_STATE, FAR * LPVXD_CONNECT_STATE;


//
//  A connection.
//

typedef struct _VXD_CONNECTION
{
    //
    //  Structure signature, for safety's sake.
    //

    DEBUG_SIGNATURE

	//
	// The stack associated with this endpoint.
	//

	LPVXD_STACK		Stack;

    //
    //  Global list of active connections.
    //

    LIST_ENTRY          OpenConnectionList;

    //
    //  Reference count.  This is the number of outstanding
    //  reasons why we cannot delete this structure.  When
    //  this value drops to zero, the structure gets deleted.
    //

    DWORD               References;

    //
    //  Private flags.
    //

    DWORD               Flags;

    //
    //  The socket that "owns" this connection.
    //

    LPSOCK_INFO         OwningSocket;

    //
    //  The endpoint that contains the address object associated
    //  with this connection.
    //

    LPVXD_ENDPOINT      OwningEndpoint;

    //
    //  Current state.
    //

    VXD_CONNECT_STATE   State;

    //
    //  The local & remote addresses for this connection.
    //

    PSOCKADDR           LocalAddress;
    PSOCKADDR           RemoteAddress;

    DWORD               LocalAddressLength;
    DWORD               RemoteAddressLength;

    //
    //  The TDI context associated with this connection.
    //
    //  CONNECTION-ORIENTED SOCKETS ONLY
    //

    CONNECTION_CONTEXT  ConnectionContext;

    //
    //  Links into a connection queue.
    //
    //  CONNECTION-ORIENTED SOCKETS ONLY
    //

    LIST_ENTRY          ConnectionQueue;

    //
    //  Send and receive queues.
    //
    //  CONNECTION-ORIENTED SOCKETS ONLY
    //

    LPVXD_QUEUES        SendReceiveQueues;

    int                 IndicatedNotAccepted;   /* wmz */

    //
    // Holds LocalAddress and RemoteAddress.
    //

    BYTE                Data[ANYSIZE_ARRAY];

};

#define VXD_CONNECTION_SIZE(StackVxd) \
    ( sizeof ( VXD_CONNECTION ) + ( 2 * (StackVxd)->MaxSockaddrLength ) - ANYSIZE_ARRAY )

#ifdef DEBUG
#define CONN_SIGNATURE          (DWORD)'nNoC'
#define CONN_SIGNATURE_X        (DWORD)'nocX'
#define INIT_CONN_SIG(p)        ((p)->Signature = CONN_SIGNATURE)
#define KILL_CONN_SIG(p)        ((p)->Signature = CONN_SIGNATURE_X)
#define IS_VALID_CONN(p)        (((p) != NULL) && ((p)->Signature == CONN_SIGNATURE))
#else   // !DEBUG
#define INIT_CONN_SIG(p)        (void)(p)
#define KILL_CONN_SIG(p)        (void)(p)
#define IS_VALID_CONN(p)        ((void)(p), TRUE)
#endif  // DEBUG


//
//  One of these structures is created before calling TdiConnect.
//  These structures keep track of the connection data until the
//  TdiConnect API completes.
//

typedef struct _VXD_TRACKER
{
    //
    //  Structure signature, for safety's sake.
    //

    DEBUG_SIGNATURE

    //
    //  Connection information.
    //

    TDI_CONNECTION_INFORMATION  RequestInfo;
    TDI_CONNECTION_INFORMATION  ReturnedInfo;

    //
    //  The target connection address.
    //

    PTRANSPORT_ADDRESS    RemoteAddress;

    //
    //  The connection object associated with this tracker.  We
    //  "precreate" the connection object when we create the tracker
    //  object.  This way, we don't need to hit the heap during
    //  TdiConnect's completion handler.
    //

    LPVXD_CONNECTION            Connection;

    //
    // Holds RemoteAddress.
    //

    BYTE                Data[ANYSIZE_ARRAY];

};

#define VXD_TRACKER_SIZE ( sizeof ( VXD_TRACKER ) + Stack->MaxTdiAddressLength - ANYSIZE_ARRAY )

#ifdef DEBUG
#define TRAC_SIGNATURE          (DWORD)'cArT'
#define TRAC_SIGNATURE_X        (DWORD)'artX'
#define INIT_TRAC_SIG(p)        ((p)->Signature = TRAC_SIGNATURE)
#define KILL_TRAC_SIG(p)        ((p)->Signature = TRAC_SIGNATURE_X)
#define IS_VALID_TRAC(p)        (((p) != NULL) && ((p)->Signature == TRAC_SIGNATURE))
#else   // !DEBUG
#define INIT_TRAC_SIG(p)        (void)(p)
#define KILL_TRAC_SIG(p)        (void)(p)
#define IS_VALID_TRAC(p)        ((void)(p), TRUE)
#endif  // DEBUG


//
//  Just to make things a little prettier...
//

typedef TDI_ADDRESS_IP FAR *    LPTDI_ADDRESS_IP;
typedef TDI_STATUS FAR *        LPTDI_STATUS;
typedef TA_IP_ADDRESS FAR *     LPTA_IP_ADDRESS;
typedef TRANSPORT_ADDRESS FAR * LPTRANSPORT_ADDRESS;
#define Address00               Address[0].Address[0]


#endif  // _WSTYPE_H_
