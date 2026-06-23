/*****************************************************************************/
/**                   Microsoft Windows                                     **/
/**   Copyright (c) 1999-2000 Microsoft Corporation.  All rights reserved.  **/
/*****************************************************************************/

/*
    autonet.c

  DESCRIPTION:
    Automatic IP configuration functions


*/

#include "dhcpp.h"
#include "dhcp.h"
#include "protocol.h"
#include "icmpif.h"

#define DHCP_IPAUTOCONFIGURATION_ATTEMPTS   20

void ProcessAutoIP(DhcpInfo *pDhcp);
extern DhcpInfo **_FindDhcp(DhcpInfo *pDhcp, PTSTR pName);  // dhcp.c
extern void FreeDhcpInfo(DhcpInfo *pDhcp);          // dhcp.c
extern void CloseDhcpSocket(DhcpInfo *pDhcp);       // dhcp.c
extern STATUS SetDhcpConfig(DhcpInfo *pDhcp);       // dhcp.c
extern STATUS PutNetUp(DhcpInfo *pDhcp);            // dhcp.c
extern BOOL CanUseCachedLease(DhcpInfo * pDhcp);    // dhcp.c
extern void NotifyXxxChange(HANDLE hEvent);           // dhcp.c
extern HANDLE	g_hAddrChangeEvent;                 // dhcp.c

#define DHCP_ICMP_WAIT_TIME     1000
#define DHCP_ICMP_RCV_BUF_SIZE  0x1000
#define DHCP_ICMP_SEND_MESSAGE  "DHCPC"

typedef ULONG (* PFN_VXDECHOREQUEST)(void  * InBuf, ulong * InBufLen, void  * OutBuf, ulong * OutBufLen);

//
// Ping the specified IP address with "DHCPC".
//
BOOL
PingDhcp(
    DWORD dwIPAddr
    )
{
    DWORD inlen;
    DWORD outlen;
    DWORD status;
    BYTE ReqBuffer[sizeof(ICMP_ECHO_REQUEST)+8];
    PICMP_ECHO_REQUEST pReq;
    BYTE ReplyBuffer[DHCP_ICMP_RCV_BUF_SIZE];
    PICMP_ECHO_REPLY EchoReplies;
    PFN_VXDECHOREQUEST pfnEchoRequest;
    HANDLE hTcpstk;
    BOOL bRet;
#ifdef DEBUG
    TCHAR Addr[32];
    TCHAR Reply[32];
#endif

    DEBUGMSG(ZONE_FUNCTION|ZONE_AUTOIP, (L"DHCP:+PingDhcp\n"));

    hTcpstk = LoadLibrary(L"tcpstk.dll");
    if (hTcpstk == NULL) {
        DEBUGMSG(ZONE_FUNCTION|ZONE_AUTOIP, (L"DHCP:-PingDhcp: LoadLibrary(tcpstk.dll) failed\n"));
        return FALSE;
    }

    bRet = FALSE;
    pfnEchoRequest = (PFN_VXDECHOREQUEST)GetProcAddress(hTcpstk, L"VXDEchoRequest");
    if (pfnEchoRequest == NULL) {
        DEBUGMSG(ZONE_FUNCTION|ZONE_AUTOIP, (L"DHCP:PingDhcp: GetProcAddress(VXDEchoRequest) failed\n"));
        goto pd_exit;
    }

    pReq = (PICMP_ECHO_REQUEST)ReqBuffer;
    pReq->Address = dwIPAddr;
    pReq->Timeout = DHCP_ICMP_WAIT_TIME;
    pReq->DataOffset = sizeof(ICMP_ECHO_REQUEST);
    pReq->DataSize = strlen(DHCP_ICMP_SEND_MESSAGE);
    strcpy(ReqBuffer+sizeof(ICMP_ECHO_REQUEST), DHCP_ICMP_SEND_MESSAGE);
    pReq->OptionsValid = 0;
    pReq->Ttl = 1;
    pReq->Tos = 0;
    pReq->Flags = 0;
    pReq->OptionsOffset = 0;
    pReq->OptionsSize = 0;

    inlen = sizeof(ReqBuffer);
    outlen = sizeof(ReplyBuffer);

    status = pfnEchoRequest(
                (void *)ReqBuffer,
                &inlen,
                (void *)ReplyBuffer,
                &outlen
                );

    if (status) {
        DEBUGMSG(ZONE_AUTOIP, (L"DHCP:PingDhcp: VXDEchoRequest failed %d\n", status));
        goto pd_exit;
    }

    EchoReplies = (PICMP_ECHO_REPLY)ReplyBuffer;
    if (EchoReplies->Status) {
        DEBUGMSG(ZONE_AUTOIP, (L"DHCP:PingDhcp: Echo status = %d\n", EchoReplies->Status));
        goto pd_exit;
    }

    if (EchoReplies->Address != dwIPAddr) {
        DEBUGMSG(ZONE_AUTOIP, (L"DHCP:PingDhcp: Echo IPAddr(%s) != Request IPAddr(%s)\n",
            AddrToString(EchoReplies->Status, Reply), AddrToString(dwIPAddr, Addr)));
        goto pd_exit;
    }

    bRet = TRUE;

pd_exit:
    FreeLibrary(hTcpstk);
    DEBUGMSG(ZONE_FUNCTION|ZONE_AUTOIP, (L"DHCP:-PingDhcp returning %s\n", bRet ? L"TRUE" : L"FALSE"));
    return bRet;
}   // PingDhcp

//
// If there were no DHCP servers and this interface has a leased IP addr, then ping
// the default gateway to make sure we are on the same subnet. This will avoid going
// to auto IP prematurely.
//
// This function will bring up the interface and leave it up if the default gateway
// could be ping'd.
//
BOOL
CouldPingGateway(
    DhcpInfo * pDhcp
    )
{
#ifdef DEBUG
    TCHAR Addr[32];
    TCHAR Gateway[32];
#endif

    DEBUGMSG(ZONE_FUNCTION|ZONE_AUTOIP, (L"DHCP:+CouldPingGateway\n"));

    if ((pDhcp->IPAddr == 0) || (pDhcp->Gateway == 0)) {
        DEBUGMSG(ZONE_FUNCTION|ZONE_AUTOIP, (L"DHCP:-CouldPingGateway: IPAddr = %s, Gateway = %s\n",
            AddrToString(pDhcp->IPAddr, Addr), AddrToString(pDhcp->Gateway, Gateway)));
        return FALSE;
    }

    if (!CanUseCachedLease(pDhcp)) {
        DEBUGMSG(ZONE_FUNCTION|ZONE_AUTOIP, (L"DHCP:-CouldPingGateway: In T2 stage!\n"));
        return FALSE;
    }

    //CallNetMsgBox(NULL, NMB_FL_OK, NETUI_GETNETSTR_CACHED_LEASE);

    if (PutNetUp(pDhcp) != DHCP_SUCCESS) {
        DEBUGMSG(ZONE_FUNCTION|ZONE_AUTOIP, (L"DHCP:-CouldPingGateway: PutNetUp(%s) failed\n",
            AddrToString(pDhcp->IPAddr, Addr)));
        return FALSE;
    }

    if (PingDhcp(pDhcp->Gateway)) {
        DEBUGMSG(ZONE_FUNCTION|ZONE_AUTOIP, (L"DHCP:-CouldPingGateway: Received response from default gateway\n"));
        return TRUE;
    }

    DEBUGMSG(ZONE_FUNCTION|ZONE_AUTOIP, (L"DHCP:-CouldPingGateway: No response to ping\n"));    
    TakeNetDown(pDhcp, FALSE, TRUE);      // retain IPAddr
    return FALSE;

}   // CouldPingGateway


//
// Schedule a CTE timer so ProcessAutoIP can look for a DHCP server again.
//
void
StartAutoIPTimer(
    DhcpInfo * pDhcp
    )
{
    FILETIME Ft;
    FILETIME CurTime;

    Ft.dwLowDateTime = pDhcp->AutoInterval;
    Ft.dwHighDateTime = 0;
    mul64_32_64(&Ft, TEN_M, &Ft);

    GetCurrentFT(&CurTime);
    add64_64_64(&CurTime, &Ft, &Ft);

    CTEStartFTimer(&pDhcp->Timer, Ft, (CTEEventRtn)ProcessAutoIP, pDhcp);
    DEBUGMSG(ZONE_AUTOIP, (L"DHCP:StartAutoIPTimer - AutoIP event scheduled\n"));
}

//
// When we autoconfigure our IP address we must continuously look for a DHCP server appearing on our net.
// The default interval is 5 minutes.
//
void
ProcessAutoIP(
    DhcpInfo *pDhcp
    )
{
    int		cPkt;
    STATUS	Status;
    DhcpPkt	Pkt;
    uint	IPAddr;
	DhcpInfo	**ppDhcp;

    DEBUGMSG(ZONE_FUNCTION|ZONE_AUTOIP, (L"DHCP:+ProcessAutoIP\n"));

    Status = DHCP_SUCCESS;
	if (*(ppDhcp = _FindDhcp(pDhcp, NULL))) {
        if (SetDHCPNTE(pDhcp)) {
            if (DHCP_SUCCESS == (Status = DhcpInitSock(pDhcp, 0))) {
    
                //
                // Save current auto config IP address and set dhcp IP address to 0, to follow spec.
                // Neither ciaddr nor DHCP_REQ_IP_OP should reflect the auto config IP address; they
                // should both be 0.
                //
                IPAddr = pDhcp->IPAddr;
                pDhcp->IPAddr = 0;
                BuildDhcpPkt(pDhcp, &Pkt, DHCPDISCOVER, NEW_PKT_FL, pDhcp->ReqOptions, &cPkt);
                pDhcp->IPAddr = IPAddr;
        
                Status = SendDhcpPkt(pDhcp, &Pkt, cPkt, DHCPOFFER, ONCE_FL|BCAST_FL);
                if (DHCP_SUCCESS == Status) {
                    //
                    // If the OFFER is from a DHCP allocator, then we should request our auto config IP address
                    // It is a DHCP allocator if it is on the same subnet as auto config.
                    //
                    if ((pDhcp->IPAddr & pDhcp->SubnetMask) == pDhcp->AutoSubnet) {
                        pDhcp->IPAddr = IPAddr;
                        BuildDhcpPkt(pDhcp, &Pkt, DHCPREQUEST, RIP_PKT_FL, pDhcp->ReqOptions, &cPkt);
                        Status = SendDhcpPkt(pDhcp, &Pkt, cPkt, DHCPACK, 
							BCAST_FL | DFT_LOOP_FL);
                        
						if (DHCP_SUCCESS == Status) {
                            DEBUGMSG(ZONE_AUTOIP, (L"DHCP:ProcessAutoIP - Leased auto cfg IP addr with ICS DHCP allocater/server.\n"));
                            ClearDHCPNTE(pDhcp);
                            CloseDhcpSocket(pDhcp);
                            CTEFreeLock(&pDhcp->Lock, 0);
                            return;

                        } else {
                            Status = DHCP_SUCCESS;  // Error, fall through to switch-over case
                            DEBUGMSG(ZONE_AUTOIP, (L"DHCP:ProcessAutoIP - Trouble with ICS DHCP allocater/server.\n"));
                        }
                    }
                }

                ClearDHCPNTE(pDhcp);
                CloseDhcpSocket(pDhcp);
        
                if (DHCP_SUCCESS == Status) {
                    //
                    // A DHCP server has appeared on the net. We need to discard the auto IP address and DHCP for one.
                    //
                    DEBUGMSG(ZONE_AUTOIP, (L"DHCP:ProcessAutoIP - Saw a DHCP server, discarding auto IP\n"));
    

					*ppDhcp = pDhcp->pNext;
					CTEFreeLock(&v_GlobalListLock, 0);
    
                    //
                    // Remember what was in this DHCPOFFER so RequestDHCPAddr doesn't send another DISCOVER
                    //
                    SetDhcpConfig(pDhcp);
    
					pDhcp->IPAddr = IPAddr;
                    TakeNetDown(pDhcp, FALSE, TRUE);  // FALSE == retain IPAddr from the DHCPOFFER

                    RequestDHCPAddr(pDhcp->Name, pDhcp->Nte, pDhcp->NteContext, 
						pDhcp->PhysAddr, ETH_ADDR_LEN);

					CTEFreeLock(&pDhcp->Lock, 0);
					FreeDhcpInfo(pDhcp);

                    DEBUGMSG(ZONE_FUNCTION|ZONE_AUTOIP, (L"DHCP:-ProcessAutoIP\n"));
                    return;
                } else {
                    //
                    // No DHCP server on this net, reschedule this timer to keep checking.
                    //
                    StartAutoIPTimer(pDhcp);
                }
            } else {
                DEBUGMSG(ZONE_AUTOIP, (L"DHCP:ProcessAutoIP - DhcpInitSock failed!\n"));
            }
        } else {
            DEBUGMSG(ZONE_AUTOIP, (L"DHCP:ProcessAutoIP - SetDHCPNTE failed!\n"));
        }
        CTEFreeLock(&v_GlobalListLock, 0);
        CTEFreeLock(&pDhcp->Lock, 0);

    } else {
		CTEFreeLock(&v_GlobalListLock, 0);
        DEBUGMSG(ZONE_AUTOIP, (L"DHCP:ProcessAutoIP - FindDhcp failed!\n"));
    }
    DEBUGMSG(ZONE_FUNCTION|ZONE_AUTOIP, (L"DHCP:-ProcessAutoIP\n"));
}   // ProcessAutoIP


extern DhcpInfo	*v_pCurrent;

//
// This function is called by the IP component to indicate the status of a gratuitous ARP
// (i.e. "no response" is good. A response usually implies an IP address conflict)
//
void
ARPResult(
    IPAddr IPAddr,
    uint Result
    )
{
    DhcpInfo	*pDhcp;
#ifdef DEBUG
    TCHAR Addr[32];
#endif

    DEBUGMSG(ZONE_AUTOIP, (L"DHCP:+ARPResult\n"));

    CTEGetLock(&v_GlobalListLock, 0);
    if (pDhcp = v_pCurrent) {
        if (pDhcp->IPAddr == IPAddr) {
            pDhcp->ARPResult = Result;
            if (pDhcp->ARPEvent) {
                DEBUGMSG(ZONE_AUTOIP, (L"DHCP:ARPResult - setting event\n"));
                SetEvent(pDhcp->ARPEvent);
            }
        } else {
            DEBUGMSG(ZONE_AUTOIP, (L"DHCP:ARPResult - IPAddress %s != Current IPAddress\n", AddrToString(IPAddr, Addr)));
        }
    } else {
        DEBUGMSG(ZONE_AUTOIP, (L"DHCP:ARPResult - v_pCurrent == NULL\n"));
    }
    CTEFreeLock(&v_GlobalListLock, 0);
    
    DEBUGMSG(ZONE_AUTOIP, (L"DHCP:-ARPResult\n"));
}   // AutoIP_ARPResult


#define GRAND_HASHING_RETRIES 5 

//================================================================================
//  GrandHashing does all the work.  It updates seed and returns an ip address
//  is in the specified subnet (with required mask) based on the random # and the
//  hardware address.
//================================================================================
uint                            //  Ip address o/p after hashing
GrandHashing(
    IN      uchar * HwAddress,  //  Hardware addres of the card
    IN      uint    HwLen,      //  Hardware length
    IN OUT  uint *  Seed,       //  In: orig value, Out: final value
    IN      uint    Mask,       //  Subnet mask to generate ip addr in
    IN      uint    Subnet      //  Subnet address to generate ip addr in
    )
{
    DWORD   Hash, Shift;
    uint    Result;
    int     cRetries;
    uchar * Addr;
    uint    Len;

    if (*Seed == 0) {
        *Seed = (uint)CeGetRandomSeed();
    }

    for (cRetries = GRAND_HASHING_RETRIES; cRetries; cRetries--) {
        Addr = HwAddress;
        Len = HwLen;
        *Seed = (*Seed)*1103515245 + 12345 ;//  Random # generator as in K&R
        Hash = (*Seed) >> 16 ;
        Hash <<= 16;
        *Seed = (*Seed)*1103515245 + 12345 ;//  Random # generator as in K&R
        Hash += (*Seed) >> 16;
    
        // Now Hash contains the 32 bit Random # we need.
        Shift = Hash % sizeof(uint) ;      //  Decide the # of bytes to shift hw-address
    
        while( Len -- ) {
            Hash += (*Addr++) << (8 * Shift);
            Shift = (Shift + 1 )% sizeof(uint);
        }

        Result = Hash & ~Mask;

        // Check for bcast addresses
        if (Result && (Result != ~Mask)) {
            return Result | Subnet;       //  Now restrict hash to be in required subnet
        }
    }

    // In the highly unlikely case that 5 consecutive hashes yielded bcast addresses,
    // return a non-bcast address
    return (GRAND_HASHING_RETRIES & ~Mask) | Subnet;
}


//
// This function is called when the DHCP client has not received any response from a DHCP server
//
STATUS
AutoIP(
    DhcpInfo *pDhcp
    )
{
    uint i;
    uint Result;
    uint Seed;
    uint HwLen;
    uint AttemptedAddress;
    uint Mask;
    uint Subnet;
    uchar * HwAddress;
    uint SaveIPAddr;
    uint AutoInterval;
#ifdef DEBUG
    TCHAR Addr[32];
#endif

    DEBUGMSG(ZONE_FUNCTION|ZONE_AUTOIP, (L"DHCP:+AutoIP\n"));

    pDhcp->SFlags &= ~DHCPSTATE_AUTO_CFG_IP;    // IPAddr not autoconfigured.

    // If there is no DHCP server out there and we have a valid IPAddr, ping the default
    // gateway to make sure we are still on the same subnet.
    if (CouldPingGateway(pDhcp)) {
        DEBUGMSG(ZONE_FUNCTION|ZONE_AUTOIP, (L"DHCP:-AutoIP - Still on DHCP server subnet\n"));
        return DHCP_SUCCESS;
    }

    Mask      = pDhcp->AutoMask;
    Subnet    = pDhcp->AutoSubnet;
    HwAddress = pDhcp->PhysAddr;
    HwLen     = sizeof(pDhcp->PhysAddr);
    Seed      = pDhcp->AutoSeed;
    SaveIPAddr = pDhcp->IPAddr;
    Result = ERROR_GEN_FAILURE;

    //
    // Tell tcpstk we are doing DHCP + AutoCfg
    //
    if (!(*pfnSetDHCPNTE)(pDhcp->NteContext, NULL, NULL, &Seed)) {
        DEBUGMSG(ZONE_AUTOIP, (L"DHCP:AutoIP - SetDHCPNTE failed!\n"));
        goto ai_exit;
    }

    if (pDhcp->AutoIPAddr) {
        //
        // Use our previously autoconfigured IP address
        //
        AttemptedAddress = pDhcp->AutoIPAddr;
    } else {
        AttemptedAddress = GrandHashing( HwAddress, HwLen, &Seed, Mask, Subnet );
    }

    i = DHCP_IPAUTOCONFIGURATION_ATTEMPTS;

    do {
        if (pDhcp->SFlags & DHCPSTATE_SAW_DHCP_SRV) {
            //
            // If we ever see a DHCP server on our net, then abort IP autoconfiguration immediately
            //
            DEBUGMSG(ZONE_AUTOIP, (L"DHCP:AutoIP:DHCP server on net. Aborting autoconfig\n"));
            goto ai_exit;
        }

        DEBUGMSG(ZONE_AUTOIP, (L"DHCP:AutoIP:Trying autoconfig address: %s\n", AddrToString(AttemptedAddress, Addr)));

        if ((AttemptedAddress & DHCP_RESERVED_AUTOCFG_MASK) == DHCP_RESERVED_AUTOCFG_SUBNET) {
            // address is in reserved range, dont use it..
            goto ai_next;
        } 

        pDhcp->ARPResult = ERROR_SUCCESS;
        pDhcp->IPAddr = AttemptedAddress;
        ResetEvent(pDhcp->ARPEvent);

        //
        // Among other things, setting the NTE addr causes ARP to probe for existence of this IP addr.
        // Just wait a few seconds to see if it is a dup.
        //
        if ((*pfnIPSetNTEAddr)((ushort)pDhcp->NteContext, NULL, pDhcp->IPAddr, Mask, 0)) {
            WaitForSingleObject(pDhcp->ARPEvent, ARP_RESPONSE_WAIT);
            if (ERROR_SUCCESS == pDhcp->ARPResult) {
                AutoInterval = pDhcp->AutoInterval; // Remember interval
                pDhcp->AutoInterval = 2;            // Send an additional DHCP discover right away.
                pDhcp->AutoIPAddr   = AttemptedAddress;
                pDhcp->AutoSeed     = Seed;    
                pDhcp->SubnetMask   = Mask;
                pDhcp->SFlags |= DHCPSTATE_AUTO_CFG_IP;     // IPAddr is autoconfigured.
                StartAutoIPTimer(pDhcp);
                pDhcp->AutoInterval = AutoInterval; // Restore interval
                CallAfd(AddInterface)(pDhcp->Name, pDhcp->Nte, 
                	pDhcp->NteContext, 0, pDhcp->IPAddr, 
                    pDhcp->SubnetMask, 0, pDhcp->DNS, 0, pDhcp->WinsServer); 
                NotifyXxxChange(g_hAddrChangeEvent);
                Result = DHCP_SUCCESS;
                break;
            } else {
                //
                // Tell IP to stop using the address and mark this NTE for another try
                //
                (*pfnIPSetNTEAddr)((ushort)pDhcp->NteContext, NULL, 0, 0, 0);
                (*pfnSetDHCPNTE)(pDhcp->NteContext, NULL, NULL, &Seed);
            }
        } else {
            DEBUGMSG(ZONE_AUTOIP, (L"DHCP:AutoIP - IPSetNTEAddr failed!\n"));
            goto ai_exit;
        }
        
ai_next:
        AttemptedAddress = GrandHashing( HwAddress, HwLen, &Seed, Mask, Subnet );
        i --;

    } while (i);

    if (0 == i) {                      //  Tried everything and still could not match
        (*pfnIPSetNTEAddr)((ushort)pDhcp->NteContext, NULL, 0, 0, 0);
        DEBUGMSG(ZONE_AUTOIP, (L"DHCP:AutoIP - Gave up autoconfiguration\n"));
    }

ai_exit:
    if (Result) {
        pDhcp->IPAddr = SaveIPAddr;
    }
    ClearDHCPNTE(pDhcp);
    DEBUGMSG(ZONE_FUNCTION|ZONE_AUTOIP, (L"DHCP:-AutoIP %d\n", Result));
    return Result;
}   // AutoIP



