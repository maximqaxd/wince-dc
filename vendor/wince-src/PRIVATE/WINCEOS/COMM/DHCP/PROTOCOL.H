/*****************************************************************************/
/**								Microsoft Windows							**/
/**   Copyright (c) 1997-2000 Microsoft Corporation.  All rights reserved.  **/
/*****************************************************************************/

/*
	protocol.h

  DESCRIPTION:
	contains the DHCP protocol structures and def's


*/
#ifndef _PROTOCOL_H_
#define _PROTOCOL_H_

#define CHADDR_LEN	16
#define SNAME_LEN	64
#define FILE_LEN	128
#define OPTIONS_LEN	312

typedef struct DhcpPkt {
	
	unsigned char	Op;
	unsigned char	Htype;
	unsigned char	Hlen;
	unsigned char	Hops;
	unsigned int	Xid;
	unsigned short	Secs;
	unsigned short	Flags;
	unsigned int	Ciaddr;
	unsigned int	Yiaddr;
	unsigned int	Siaddr;
	unsigned int	Giaddr;
	unsigned char	aChaddr[CHADDR_LEN];
	unsigned char	aSname[SNAME_LEN];
	unsigned char	aFile[FILE_LEN];
	unsigned char	aOptions[OPTIONS_LEN];

} DhcpPkt;

// we're being little-endian specific here

//	Flags
#define DHCP_BCAST_FL	0x0008


//	DHCP Message Types (after option DHCP_MSG_TYPE_OP)
#define DHCPDISCOVER	1
#define DHCPOFFER		2	
#define DHCPREQUEST		3
#define DHCPDECLINE		4
#define DHCPACK			5
#define DHCPNACK		6
#define DHCPRELEASE		7
#define DHCPINFORM		8


//	Function Declarations
void BuildDhcpPkt(DhcpInfo *pDhcp, DhcpPkt *pPkt, uchar Type, int fNew, 
					uchar *aOptionList, int *pcPkt);
STATUS SendDhcpPkt(DhcpInfo *pDhcp, DhcpPkt *pPkt, int cPkt, 
				   uchar WaitforType, int Flags);

STATUS TranslatePkt(DhcpInfo *pDhcp, DhcpPkt *pPkt, int cPkt, uchar *pType,
					unsigned int Xid);

// Flags for BuildDhcpPkt
#define NEW_PKT_FL	0x01
#define SID_PKT_FL	0x02	// Server Id flag
#define RIP_PKT_FL	0x04	// Requested IP addr option

// Flags for SendDhcpPkt
// first byte is number of times to loop...
#define LOOP_MASK_FL		0x00ff
#define ONCE_FL				0x0001
#define DFT_LOOP_FL			0x0004
#define BCAST_FL			0x0100


//	Magic Cookie in Little Endian format
#define MAGIC_COOKIE	0x63538263

//	Option Definitions (in decimal)
#define DHCP_PAD_OP				0
#define DHCP_END_OP				255

#define DHCP_SUBNET_OP			1
#define DHCP_ROUTER_OP			3
#define DHCP_DNS_OP				6
#define DHCP_HOST_NAME_OP		12
#define DHCP_DOMAIN_NAME_OP		15
#define DHCP_NBT_SRVR_OP		44
#define DHCP_NBT_NODE_TYPE_OP		46
#define DHCP_REQ_IP_OP			50
#define DHCP_IP_LEASE_OP		51
#define DHCP_OVERLOAD_OP		52
#define DHCP_MSG_TYPE_OP		53
#define DHCP_SERVER_ID_OP		54
#define DHCP_PARAMETER_REQ_OP	55
#define DHCP_MESSAGE_OP			56
#define DHCP_T1_VALUE_OP		58
#define DHCP_T2_VALUE_OP		59
#define DHCP_CLIENT_ID_OP		61
#define DHCP_MSFT_AUTOCONF      251


//	Unused Options (decimal)
#if 0

#define DHCP_TIME_OFFSET_OP			2
#define DHCP_TIME_SRVR_OP			4
#define DHCP_NAME_SRVR_OP			5
#define DHCP_LOG_SRVR_OP			7
#define DHCP_COOKIE_SRVR_OP			8
#define DHCP_LPR_SRVR_OP			9
#define DHCP_IMPRESS_SRVR_OP		10
#define DHCP_RES_LOC_SRVR_OP		11	// Resource location server option
#define DHCP_BOOTFILE_SIZE_OP		13
#define DHCP_MERIT_DUMP_FILE_OP		14
#define DHCP_SWAP_SRVR_OP			16
#define DHCP_ROOT_PATH_OP			17
#define DHCP_EXT_PATH_OP			18
#define DHCP_IP_FORWARDING_OP		19
#define DHCP_NONLOCAL_SRCROUTE_OP	20
#define DHCP_POLICY_FILTER_OP		21
#define DHCP_MAX_DGRAM_REASSEMBLE_SIZE_OP	22
#define DHCP_IP_TTL_OP				23
#define DHCP_PATH_MTU_TO_OP			24
#define DHCP_PATH_MTU_PLATEAU_OP	25
#define DHCP_INTERFACE_MTU_OP		26
#define DHCP_ALL_SUBNETS_LOCAL_OP	27
#define DHCP_BCAST_ADDR_OP			28
#define DHCP_MASK_DISCOVERY_OP		29
#define DHCP_MASK_SUPPLIER_OP		30
#define DHCP_ROUTER_DISCOVERY_OP	31
#define DHCP_ROUTER_SOLICIT_ADDR_OP	32
#define DHCP_STATIC_ROUTE_OP		33
#define DHCP_TRAILER_ENCAPSULATE_OP	34
#define DHCP_ARP_CACHE_TO_OP			35
#define DHCP_ETHERNET_ENCAPSULATE_OP	36
#define DHCP_TCP_TTL_OP					37
#define DHCP_TCP_KEEPALIVE_INTVL_OP		38
#define DHCP_TCP_KEEPALIVE_GARBAGE_OP	39
#define DHCP_NIS_DOMAIN_OP				40
#define DHCP_NETWORK_INFO_SRVRS_OP	41
#define DHCP_NETWORK_TIME_SRVRS_OP	42
#define DHCP_VENDOR_SPECIFIC_OP		43
#define DHCP_NBDD_OP				45
#define DHCP_NBT_SCOPE_OP			47
#define DHCP_XWINDOW_FONT_SRVR_OP	48
#define DHCP_XWINDOW_DISPLAY_MGR_OP	49
#define DHCP_NISP_DOMAIN_OP			64
#define DHCP_NISP_SRVRS_OP			65
#define DHCP_MOBILE_IP_AGENT_OP		68
#define DHCP_SMTP_SRVR_OP			69
#define DHCP_POP3_SRVR_OP			70
#define DHCP_NNTP_SRVR_OP			71
#define DHCP_WWW_SRVR_OP			72
#define DHCP_DEFAULT_FINGER_OP		73
#define DHCP_DEFAULT_IRC_SRVR_OP	74	//	default Internet relay chat server
#define DHCP_STREETTALK_SRVR_OP		75
#define DHCP_STDA_SRVR_OP			76
#define DHCP_TFTP_SRVR_NAME_OP		66
#define DHCP_BOOTFILE_NAME_OP		67
#define DHCP_MAX_DHCP_MSG_OP		57
#define DHCP_VENDOR_CLASS_ID_OP		60


#endif	// 0

#endif	// _PROTOCOL_H_
