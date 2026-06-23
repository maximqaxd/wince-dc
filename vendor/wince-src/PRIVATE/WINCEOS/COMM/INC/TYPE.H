/**********************************************************************/
/**                        Microsoft Windows                         **/
/**Copyright (c) 1994-2000 Microsoft Corporation.  All rights reserved.**/
/**********************************************************************/

/*
    type.h

    This file contains global type definitons for WINSOCK.DLL and
    WSOCK32.DLL.


*/


#ifndef _TYPE_H_
#define _TYPE_H_

#include "ipexport.h"


#if !defined(WIN32)

//
//  Various types conspicuously absent from 16-bit windows.h.
//

#define CHAR        char
#define INT         int

#endif  // !WIN32


//
//  Make things a little more consistent.
//

typedef struct netent NETENT;
typedef struct netent FAR * LPNETENT;


//
//  Per-thread info: We allocate a thread local storage index.  The data
//  stored at this index is a pointer to one of these.
//

#pragma pack(4)

typedef struct _SOCK_THREAD {
    CHAR            st_ntoa_buffer[20];

    //
    // database-specific (hosts, protocol, services, networks) fields
    //

    //
    // per-thread hostent data areas:
    //
    //      GETHOST_host
    //          hostent structure returned from gethostbyY calls. String fields
    //          point into GETHOST_HOSTDB
    //
    //      GETHOST_HOSTDB
    //          contains the line from the hosts file containing the requested
    //          host entry
    //

#define MAXALIASES		15		// We only support 5 aliases
#define AVG_HOST_LEN	40		// To calculate buff space
#define MAXADDRS		15		// Max of 5 IP addrs
    HOSTENT GETHOST_host;
	// The array of host alias names
    LPSTR GETHOST_host_aliases[MAXALIASES+1];
	// Some buffer space for host name and alias names
	char GETHOST_hostbuf[(MAXALIASES+1)*AVG_HOST_LEN];
	// The array of addresses
    LPSTR GETHOST_h_addr_ptrs[MAXADDRS + 1];
	// Some space for the addresses
    unsigned char GETHOST_hostaddr[MAXADDRS*sizeof(IPAddr)];

    //
    // per-thread protoent data areas:
    //
    //      GETPROTO_proto
    //          the protoent structure returned from getprotobyY calls.
    //          String fields in the protoent point into GETPROTO_PROTODB
    //
    //      GETPROTO_proto_aliases
    //          array of pointers to protocol aliases. The aliases are in
    //          GETPROTO_PROTODB
    //
    //      GETPROTO_PROTODB
    //          contains the line from the protocol file containing the
    //          requested protocol entry
    //

//    PROTOENT GETPROTO_proto;
//    LPSTR GETPROTO_proto_aliases[MAXALIASES];
//    char GETPROTO_PROTODB[PROTODB_SIZE];

    //
    // per-thread servent data areas:
    //
    //      GETSERV_serv
    //          servent structure returned from getservbyY calls. String fields
    //          point into GETSERV_SERVDB
    //
    //      GETSERV_serv_aliases
    //          array of pointers to service aliases. The aliases are in
    //          GETSERV_SERVDB
    //
    //      GETSERV_SERVDB
    //          contains the line read from services file that contains the
    //          requested service entry
    //

//    SERVENT GETSERV_serv;
//    LPSTR GETSERV_serv_aliases[MAXALIASES];
//    char GETSERV_SERVDB[SERVDB_SIZE];

    //
    // per-thread netent data areas:
    //
    //      GETNET_net
    //          netent structure returned from getnetbyY calls. String fields
    //          point into GETNET_NETDB
    //
    //      GETNET_net_aliases
    //          array of pointers to net aliases. The aliases are in
    //          GETNET_NETDB
    //
    //      GETNET_NETDB
    //          contains the line read from networks file that contains the
    //          requested net entry
    //

//    NETENT GETNET_net;
//    LPSTR GETNET_net_aliases[MAXALIASES];
//    char GETNET_NETDB[NETDB_SIZE];

    //
    // per-thread resolver data areas:
    //

//    struct state R_INIT__res;

//    char INTOA_Buffer[18];
//    SOCKET DnrSocketHandle;
//    BOOLEAN IsBlocking;
//    BOOLEAN IoCancelled;
//    BOOLEAN ProcessingGetXByY;
//    BOOLEAN GetXByYCancelled;
//    BOOLEAN EnableWinsNameResolution;
//    BOOLEAN DisableWinsNameResolution;
//    SOCKET SocketHandle;
//    HANDLE EventHandle;
//    ULONG CreateOptions;
//    INT DnrErrorCode;

} SOCK_THREAD;

#pragma pack()

typedef struct _SOCK_THREAD FAR * LPSOCK_THREAD;


//
//  Per-process info.
//

typedef struct _SOCK_PROCESS {
    LONG            NumStartups;        // Number of times WSAStartup called.

#if !defined(WIN32)

    SOCK_THREAD     ThreadInfo;         // Under Win16, one thread per process.
    HTASK           OwningTask;         // Task ID.
    LIST_ENTRY      TaskList;           // Links into task list.

#endif  // !WIN32

} SOCK_PROCESS;
typedef struct _SOCK_PROCESS FAR * LPSOCK_PROCESS;


//
//  Miscellaneous types.
//

typedef ULONG FAR * LPULONG;
typedef INT   FAR * LPINT;
typedef UINT  FAR * LPUINT;


//
// manifests for database names
//

#define DB_HOSTS    "hosts"
#define DB_PROTOCOL "protocol"
#define DB_SERVICES "services"
#define DB_NETWORKS "networks"

#define MAX_DB_FILE DB_SERVICES
#define MAX_DB_FILE_NAME_LENGTH (sizeof(MAX_DB_FILE) - 1)

//
// per-thread data items: use these macros to access individual per-thread
// variables.
//
// ASSUMES enterAPI has been called and that we have a valid pointer to the
// per-thread data structure in a pointer variable called pThread
//
// A reasonable assumption methinks
//

#define SockThreadIoCancelled           pThread->IoCancelled
#define SockThreadProcessingGetXByY     pThread->ProcessingGetXByY
#define SockThreadGetXByYCancelled      pThread->GetXByYCancelled
#define SockDnrSocket                   pThread->DnrSocketHandle
#define SockEnableWinsNameResolution    pThread->EnableWinsNameResolution
#define SockDisableWinsNameResolution   pThread->DisableWinsNameResolution
#define SockThreadDnrErrorCode          pThread->DnrErrorCode

//
// generic per-thread variable access method
//
// ASSUMES pThread is a valid pointer to the current thread's SOCK_THREAD
// structure
//

#define ACCESS_THREAD_DATA(field, struct)   (pThread->## struct ## _ ## field)
#define _res ACCESS_THREAD_DATA( _res, R_INIT )

#ifdef UNDER_CE
#define GET_THREAD_DATA(p)
#else
#define GET_THREAD_DATA(p)  p = OsGetThreadInfo()
#endif

//
// _WINSOCK_CONTEXT_BLOCK - used in async calls (only gethostby...)
//

typedef struct _WINSOCK_CONTEXT_BLOCK {

    LIST_ENTRY AsyncThreadQueueListEntry;
    HANDLE TaskHandle;
    UINT OpCode;

    union {

        struct {
            HWND hWnd;
            unsigned int wMsg;
            LPCHAR Filter;
            int Length;
            int Type;
            LPCHAR Buffer;
            int BufferLength;
        } AsyncGetHost;

    } Overlay;

} WINSOCK_CONTEXT_BLOCK, FAR * PWINSOCK_CONTEXT_BLOCK;

//
// prototypes using WINSOCK_CONTEXT_BLOCK
//

PWINSOCK_CONTEXT_BLOCK
SockAllocateContextBlock (
    VOID
    );

VOID
SockFreeContextBlock (
    IN PWINSOCK_CONTEXT_BLOCK ContextBlock
    );

VOID
SockQueueRequestToAsyncThread(
    IN PWINSOCK_CONTEXT_BLOCK ContextBlock
    );

//
// Opcodes for processing by the winsock asynchronous processing
// thread.
//

#define WS_OPCODE_GET_HOST_BY_ADDR    0x01
#define WS_OPCODE_GET_HOST_BY_NAME    0x02

#define WS_OPCODE_TERMINATE           0x10

#define ALLOCATE_HEAP DllAllocMem
#define FREE_HEAP DllFreeMem

//
// Manifests for SockWaitForSingleObject().
//

#define SOCK_ALWAYS_CALL_BLOCKING_HOOK        1
#define SOCK_CONDITIONALLY_CALL_BLOCKING_HOOK 2
#define SOCK_NEVER_CALL_BLOCKING_HOOK         3

#define SOCK_NO_TIMEOUT      4
#define SOCK_SEND_TIMEOUT    5
#define SOCK_RECEIVE_TIMEOUT 6

#endif  // _TYPE_H_

