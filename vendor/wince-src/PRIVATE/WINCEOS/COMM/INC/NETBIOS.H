
/**********************************************************************/
/**                        Microsoft Windows                         **/
/**Copyright (c) 1996-2000 Microsoft Corporation.  All rights reserved.**/
/**********************************************************************/

/*
	netbios.h
	
	description

*/

#define NCBNAMSIZ	16

// Network Control Block structure
typedef struct NCB {
	DWORD		ReturnCode;
	DWORD		cTotal;
	USHORT		Command;
	int			LSN;
	BYTE		CallName[NCBNAMSIZ];
	BYTE		Name[NCBNAMSIZ];
	UCHAR		LanaNum;
	// SHORT	NameNum;	// requested- not yet added
} NCB, *PNCB;

typedef void (*PFN_NET_NOTIFY)(uchar cLana, int Flags, int Unused);
// Flags for the Net Notify function
#define LANA_UP_FL		0x01
#define ETHER_LANA_FL	0x02


// OpCodes for the Netbios Function

#define	NB_REGISTER			0x01	// this is a must for all Helper funcs
#define NB_RESERVED			0x02	// reserved for all helper funcs

#define NB_ADD_RESOLVER		0x03	// internal use by afd
#define NB_NET_NOTIFY		0x04	// call back fn for net notifications
// note; need a notification mechanism

#define NB_ADD_NAME			0x08
#define NB_ADD_GROUP_NAME	0x09
#define NB_DELETE_NAME		0x0a

#define NB_FIND_NAME		0x10
#define NB_LISTEN			0x11
#define NB_CALL				0x12
#define NB_RECEIVE			0x13
#define NB_RECEIVE_ANY		0x14
#define NB_SEND				0x15	// send & sendnoack to be same
#define NB_SEND_NOACK		0x16
#define NB_CHAIN_SEND		0x17	// not supported
#define NB_CHAIN_SEND_NOACK	0x18	// not supported
#define NB_HANGUP			0x19
#define NB_HANGUP_ANY		0x1a	// hangs up all Ssn under a given name
#define NB_SESSION_STATUS	0x1b

#define NB_RECEIVE_DG		0x20
#define NB_SEND_DG			0x21
#define NB_RECEIVE_BCAST_DG	0x22
#define NB_SEND_BCAST_DG	0x23

#define NB_ADAPTER_STATUS	0x30
#define NB_RESET			0x31
#define NB_CANCEL			0x32
#define NB_TRACE			0x33	// not supported
// UNLINK definitely not supported!

// ACTION, ENUM, LANST_ALERT - NT extensions not supported

