/*****************************************************************************/
/**                                Microsoft Windows                        **/
/**  Copyright (c) 1997-2000 Microsoft Corporation.  All rights reserved.   **/
/*****************************************************************************/

/*
    packet.c

  DESCRIPTION:
    DHCP packet routines


*/

#include "dhcpp.h"
#include "dhcp.h"
#include "protocol.h"

#ifdef DEBUG
LPWSTR
PacketType(
    uchar Type
    )
{
    switch (Type) {
    case DHCPDISCOVER:  return L"DISCOVER";
    case DHCPREQUEST:   return L"REQUEST";
    case DHCPINFORM:    return L"INFORM";
    case DHCPDECLINE:   return L"DECLINE";
    case DHCPRELEASE:   return L"RELEASE";

    default:        return L"UNKNOWN";
    }
}
#endif

void BuildDhcpPkt(DhcpInfo *pDhcp, DhcpPkt *pPkt, uchar Type, int Flags, 
                    uchar *aOptionList, int *pcPkt) {
    int        Cookie = MAGIC_COOKIE;
    uchar    *p, *pEnd;

    DEBUGMSG(ZONE_INIT, (TEXT("+BuildDhcpPkt: %s pDhcp 0x%X/0x%X\r\n"),
            PacketType(Type), pDhcp, pDhcp->Socket));

    pEnd = pPkt->aOptions + OPTIONS_LEN;    // byte right after end of pkt
    if (Flags & NEW_PKT_FL) {
        pPkt->Op = 1;
        pPkt->Htype = 1;    // 10Mb Ethernet
        pPkt->Hlen = ETH_ADDR_LEN;
        pPkt->Hops = 0;
        // now we getXid here - OM
        pPkt->Xid = GetXid(pDhcp->PhysAddr);
        pPkt->Xid = net_long(pPkt->Xid);
        // Secs, Flags, Yiaddr, Siaddr, Giaddr should all be set to 0
        pPkt->Secs = pPkt->Flags = pPkt->Ciaddr = pPkt->Yiaddr = 
            pPkt->Siaddr = pPkt->Giaddr = 0;    // erase any previous value
        memset(pPkt->aChaddr, 0, CHADDR_LEN + SNAME_LEN + FILE_LEN);
        memcpy(pPkt->aChaddr, pDhcp->PhysAddr, ETH_ADDR_LEN);
        memcpy(pPkt->aOptions, &Cookie, 4);
    }
    if ((Flags & RIP_PKT_FL) == 0)
        pPkt->Ciaddr = pDhcp->IPAddr;

    p = pPkt->aOptions + 4;
    // Set the Message Type
    *p++ = DHCP_MSG_TYPE_OP;
    *p++ = 1;                // option length
    *p++ = Type;

    switch(Type) {
    case DHCPREQUEST:
        // Server Identifier
        if (Flags & SID_PKT_FL) {
            *p++ = DHCP_SERVER_ID_OP;
            *p++ = 4;
            memcpy(p, &pDhcp->DhcpServer, 4);
            p += 4;
        }

        // no break, fall-thru

    case DHCPDISCOVER:

        // Requested Addr
        if (pDhcp->IPAddr && (Flags & RIP_PKT_FL)) {
            *p++ = DHCP_REQ_IP_OP;
            *p++ = 4;
            memcpy(p, &pDhcp->IPAddr, 4);
            p += 4;
        }

        // IP address lease should also be done before DHCPINFORM

        // no break, fall-thru

    case DHCPINFORM:

        // Set the Client Id
        *p++ = DHCP_CLIENT_ID_OP;
        *p++ = 7;                // op len
        *p++ = 1;                // Type - 10Mb Ethernet
        memcpy(p, &pDhcp->PhysAddr, ETH_ADDR_LEN);    
        p += ETH_ADDR_LEN;

        /* Host Name Option
        *p++ = DHCP_HOST_NAME_OP;
        *p++ = 0; // len of name
        memcpy();
        */

        // Parameter Request List
        if (aOptionList && aOptionList[0] &&
            aOptionList[0] <= (pEnd - p)) {
            *p++ = DHCP_PARAMETER_REQ_OP;
            memcpy (p, aOptionList, aOptionList[0] + 1);    // +1 for len
            p += aOptionList[0] + 1;    // +1 for length
        }

        if (pDhcp->pSendOptions) {
            memcpy(p, pDhcp->pSendOptions, pDhcp->cSendOptions);
            p += pDhcp->cSendOptions;
        }

        // End Option
        *p++ = DHCP_END_OP;
        
        break;

    case DHCPDECLINE:
        // Requested Addr
        *p++ = DHCP_REQ_IP_OP;
        *p++ = 4;
        memcpy(p, &pDhcp->IPAddr, 4);
        p += 4;

        // no break, fall-thru

    case DHCPRELEASE:

        // Set the Client Id
        *p++ = DHCP_CLIENT_ID_OP;
        *p++ = 7;                // op len
        *p++ = 1;                // Type - 10Mb Ethernet
        memcpy(p, pDhcp->PhysAddr, ETH_ADDR_LEN);
        p += ETH_ADDR_LEN;
                    
        // Server Identifier
        *p++ = DHCP_SERVER_ID_OP;
        *p++ = 4;
        memcpy(p, &pDhcp->DhcpServer, 4);
        p += 4;

        // we should also do Message Option

        // End Option
        *p++ = DHCP_END_OP;

        break;

    default:
        DEBUGMSG(ZONE_ERROR, 
            (TEXT("\t!BuildDhcpPkt: unknown packet type (%d) requested\r\n"),
            Type));
        break;
    }    // switch(Type)

    if (p < pEnd)
        memset(p, DHCP_PAD_OP, pEnd - p);
    
    if (pcPkt)
        *pcPkt = p - (char *)pPkt;

    DEBUGMSG(ZONE_INIT, (TEXT("-BuildDhcpPkt: pDhcp 0x%X/0x%X Len %d\r\n"),
            pDhcp, pDhcp->Socket, (p - (char *)pPkt)));

}    // BuildDhcpPkt()


void Tvert(uchar c, TCHAR *p) {
    if (c >= 100)
        p++;
    if (c >= 10)
        p++;
    *(p+1) = TEXT('\0');

    do {
        *p-- = TEXT('0') + (c % 10);
    } while (c /= 10);

}


#define MIN_LEASE_TIME 1

STATUS TranslatePkt(DhcpInfo *pDhcp, DhcpPkt *pPkt, int cPkt, uchar *pType,
                    unsigned int Xid) {
    STATUS    Status = DHCP_FAILURE;  // assume failure
    int        i, fSaveReg, Cookie = MAGIC_COOKIE;
    uchar    *p, Type = 0;
    TCHAR    szBuf[MAX_REG_STR];
    HKEY    hKey;
    LONG    hRes;
    DWORD    cData;

    uint    NewSubnetMask,  fSubnetMask;
    uint    NewGateway,     fGateway;
    uint    NewLease,       fLease;
    uint    NewServer,      fServer;
    uint    NewT1,          fT1;
    uint    NewT2,          fT2;
    uint    NewDNS[2],      fDNS;
    uint    NewWins[2],     fWins;
    uchar * pnew;

    DEBUGMSG(ZONE_RECV, 
        (TEXT("DHCP:+TranslatePkt: pDhcp %X: OP %d, Htype %d, Xid %x/%x\r\n"), 
        pDhcp, pPkt->Op, pPkt->Htype, pPkt->Xid, Xid));

    fSubnetMask = fGateway = fLease = fServer = fT1 = fT2 = fDNS = fWins = fSaveReg = FALSE;

    if (cPkt >= (sizeof(*pPkt) - OPTIONS_LEN + 4) &&
    	2 == pPkt->Op &&
        1 == pPkt->Htype &&
        Xid == pPkt->Xid &&
        0 == memcmp(&Cookie, pPkt->aOptions, 4) &&
        ((pDhcp->Flags & NO_MAC_COMPARE_FL) || (pPkt->Hlen == 6 && 
            (0 == memcmp(pDhcp->PhysAddr, pPkt->aChaddr, 6))))) {

	    pDhcp->SFlags |= DHCPSTATE_SAW_DHCP_SRV;    // There is a DHCP server on the net.
        
        //
        // Scan through the options once without storing config options so we can
        // perform some sanity checks first.
        // (The number of options checked will increase as we get more input from QA and customers)
        //
        p = &pPkt->aOptions[4];
        while (p < (char *)pPkt + cPkt) {
            i = 4;
            pnew = NULL;
            switch (*p) {
            //
            // These are the 4 byte (i=4) cases
            //
            case DHCP_SUBNET_OP:
                pnew = (uchar *) &NewSubnetMask;
                fSubnetMask++;
                break;
            case DHCP_IP_LEASE_OP:
                pnew = (uchar *) &NewLease;
                fLease++;
                break;
            case DHCP_SERVER_ID_OP:
                pnew = (uchar *) &NewServer;
                fServer++;
                break;
            case DHCP_ROUTER_OP:
                pnew = (uchar *) &NewGateway;
                fGateway++;
                break;
            case DHCP_T1_VALUE_OP:
                pnew = (uchar *) &NewT1;
                fT1++;
                break;
            case DHCP_T2_VALUE_OP:
                pnew = (uchar *) &NewT2;
                fT2++;
                break;

            //
            // Up to 8 byte options
            //
            case DHCP_DNS_OP: 
                NewDNS[0] = 0;
                NewDNS[1] = 0;
                if ((i = p[1]) > 8) {
                    i = 8;
                }
                pnew = (uchar *)NewDNS;
                fDNS++;
                break;

            case DHCP_NBT_SRVR_OP:
                NewWins[0] = 0;
                NewWins[1] = 0;
                if ((i = p[1]) > 8) {
                    i = 8;
                }
                pnew = (uchar *)NewWins;
                fWins++;
                break;

            //
            // Special cases
            //
            case DHCP_MSG_TYPE_OP:
                Type = p[2];
                if (*pType && Type != *pType) {
                    Status = DHCP_SUCCESS;
                    DEBUGMSG(ZONE_RECV, (TEXT("DHCP:TranslatePkt: Type = %d, expecting %d\n"), Type, *pType));
                    goto BreakLoop;
                }
                break;

            case DHCP_MSFT_AUTOCONF:
                if (p[1] != 1) {
                    DEBUGMSG(ZONE_RECV, (TEXT("DHCP:TranslatePkt: AutoConfig Option Len = %d, expecting 1\n"), p[1]));
                    goto BreakLoop;
                }
                break;

            case DHCP_PAD_OP:
                p++;
                continue;
                break;

            case DHCP_END_OP:
                goto VerifyOptions;
                break;
            }

            if (pnew) {
                // verify length
                if (p + i > (char *)pPkt + cPkt) {
                    DEBUGMSG(ZONE_RECV, (TEXT("DHCP:TranslatePkt: Length %d too long for option %d\n"), i, *p));
                    goto BreakLoop;
                }

                memcpy(pnew, p+2, i);
            }
            p += p[1] + 2;
        }

VerifyOptions:

        //
        // Make sure config options seem reasonable
        //
        if (fSubnetMask) {
            if (NewSubnetMask == 0) {
                DEBUGMSG(ZONE_RECV, (TEXT("DHCP:TranslatePkt: Invalid subnet mask %d\n"), NewSubnetMask));
                goto BreakLoop;
            }
        }

        if (fLease) {
            if (NewLease < MIN_LEASE_TIME && Type == DHCPACK) {
                DEBUGMSG(ZONE_RECV, (TEXT("DHCP:TranslatePkt: Invalid lease time %d < %d\n"), NewLease, MIN_LEASE_TIME));
                goto BreakLoop;
            }
        } else {
            if (Type == DHCPACK) {
                DEBUGMSG(ZONE_RECV, (TEXT("DHCP:TranslatePkt: No lease specified in DHCPACK!\n")));
                goto BreakLoop;
            }
        }

        if (fServer) {
            if (NewServer == 0) {
                DEBUGMSG(ZONE_RECV, (TEXT("DHCP:TranslatePkt: Invalid server id %d\n"), NewServer));
                goto BreakLoop;
            }
        }

        //
        // Make sure the ipaddr and subnet mask match if this is a renewal
        //
        if (*pType == DHCPACK && pDhcp->IPAddr) {
            if (pDhcp->IPAddr != pPkt->Yiaddr) {
                DEBUGMSG(ZONE_RECV,(TEXT("DHCP:TranslatePkt: Mismatch in IP addr - rcvd 0x%X, expect 0x%X\r\n"),
                                    pPkt->Yiaddr,pDhcp->IPAddr));
                goto BreakLoop;
            }
            if (pDhcp->SubnetMask != NewSubnetMask) {
                DEBUGMSG(ZONE_RECV,(TEXT("DHCP:TranslatePkt: Mismatch in subnet - rcvd 0x%X, expect 0x%X\r\n"),
                                    NewSubnetMask,pDhcp->SubnetMask));
                goto BreakLoop;
            }
        }

        //
        // Open registry key to save new values
        //
        if (DHCPACK == Type) {
            _tcscpy (szBuf, COMM_REG_KEY);
            _tcscat (szBuf, pDhcp->Name);
            _tcscat (szBuf, TEXT("\\Parms\\TcpIp\\DhcpOptions"));
            hRes = RegOpenKeyEx (HKEY_LOCAL_MACHINE, szBuf, 0, 0, 
                &hKey);
            if (ERROR_SUCCESS == hRes) {
                DEBUGMSG(ZONE_RECV, 
                    (TEXT("DHCP:TranslatePacket: Save in registry\r\n")));
                fSaveReg = TRUE;
            }
        }

        //
        // Now run through the options again and store away results
        //
        p = &pPkt->aOptions[4];
        while (p < (char *)pPkt + cPkt) {
            switch(*p) {
            case DHCP_PAD_OP:
                p++;
                continue;
                break;
            case DHCP_END_OP:
                Status = DHCP_SUCCESS;
                goto BreakLoop;
                break;
            case DHCP_SUBNET_OP:
                pDhcp->SubnetMask = NewSubnetMask;
                break;
            case DHCP_ROUTER_OP:
                pDhcp->Gateway = NewGateway;
                break;
            case DHCP_DNS_OP:
                pDhcp->DNS[0] = NewDNS[0];
                pDhcp->DNS[1] = NewDNS[1];
                break;
            case DHCP_DOMAIN_NAME_OP:
                if (p[1] > MAX_DOMAIN_NAME)
                    p[1] = (uchar)MAX_DOMAIN_NAME;
                memcpy(pDhcp->aDomainName, p+1, p[1] + 1);
                break;
            case DHCP_NBT_SRVR_OP:
                pDhcp->WinsServer[0] = NewWins[0];
                pDhcp->WinsServer[1] = NewWins[1];
                break;
            case DHCP_IP_LEASE_OP:
                pDhcp->Lease = net_long(NewLease);
                break;
            case DHCP_SERVER_ID_OP:
                pDhcp->DhcpServer = NewServer;
                break;
            case DHCP_T1_VALUE_OP:
                pDhcp->T1 = net_long(NewT1);
                break;
            case DHCP_T2_VALUE_OP:
                pDhcp->T2 = net_long(NewT2);
                break;

            case DHCP_MSFT_AUTOCONF:
                if (p[2]) {
                    pDhcp->Flags |= AUTO_IP_ENABLED_FL;
                } else {
                    pDhcp->Flags &= ~AUTO_IP_ENABLED_FL;
                }
                break;

            case DHCP_OVERLOAD_OP:
                ASSERT(0); 
                break;

            case DHCP_MSG_TYPE_OP:
            case DHCP_MESSAGE_OP:       // NYI
            case DHCP_NBT_NODE_TYPE_OP: // NYI
            case DHCP_HOST_NAME_OP:     // NYI
                break;

            default:
                DEBUGMSG(ZONE_RECV, 
                    (TEXT("DHCP:TranslatePkt: Unexpected option %d\r\n"), *p));
                break;
            }    // switch(*p)

            if (fSaveReg) {
                Tvert(*p, szBuf);
                cData = 0;
                hRes = RegQueryValueEx(hKey, szBuf, NULL, NULL, NULL, &cData);
                if (ERROR_SUCCESS == hRes && cData)
                    SetRegBinaryValue(hKey, szBuf, p, p[1] + 2);
            }

            p += p[1] + 2;

        }    // while (...)

        DEBUGMSG(ZONE_RECV, (TEXT("DHCP:TranslatePkt: No DHCP_END_OP in packet!!\n")));

BreakLoop:        
        if (Status == DHCP_SUCCESS && (Type == DHCPOFFER || Type == DHCPACK)) {
            if (fLease) {
                if (! fT1)
                    pDhcp->T1 = pDhcp->Lease >> 1;      // default half lease time
                if (! fT2)
                    pDhcp->T2 = (pDhcp->Lease * 7) >> 3;    // dft: 7/8 lease time
                memcpy(&pDhcp->IPAddr, &pPkt->Yiaddr, 4);
            }
        }

    } else {
#if DEBUG

		if (cPkt < (sizeof(*pPkt) - OPTIONS_LEN + 4))
			DEBUGMSG(ZONE_RECV, 
				(TEXT("DHCP:Translate: packet too small %d\r\n"),
				cPkt));
		// we can continue--since pPkt is a ptr to our own packet which we know is
		// larger than the minimum dhcp packet

        if (2 != pPkt->Op) 
            DEBUGMSG(ZONE_RECV, (TEXT("DHCP:TranslatePkt: Op %d != 2\n"), 
	            pPkt->Op));
        if (1 != pPkt->Htype)
            DEBUGMSG(ZONE_RECV, 
	            (TEXT("DHCP:TranslatePkt: Htype %d != 1\n"), pPkt->Htype));
        if (Xid != pPkt->Xid)
            DEBUGMSG(ZONE_RECV, 
	            (TEXT("DHCP:TranslatePkt: Xid 0x%x != 0x%x\n"), pPkt->Xid, Xid));
        if (0 != memcmp(&Cookie, pPkt->aOptions, 4))
            DEBUGMSG(ZONE_RECV, 
	            (TEXT("DHCP:TranslatePkt: Cookie %B%B%B%B != %X\n"),
	            pPkt->aOptions[3], pPkt->aOptions[2], pPkt->aOptions[1], 
	            pPkt->aOptions[0], Cookie));

#endif                                 
    }

    if (fSaveReg)
        RegCloseKey(hKey);

    if (pType)
        *pType = Type;
    
    DEBUGMSG(ZONE_RECV, (TEXT("DHCP:-TranslatePkt: pDhcp %X Type %d, Ret: %d\r\n"),
        pDhcp, Type, Status));
    return Status;

}    // TranslatePkt()


STATUS DhcpSendDown(DhcpInfo *pDhcp, DhcpPkt *pPkt, int cPkt, int DestIP) {
    SOCKADDR_IN SockAddr;
    STATUS        Status;
    int            cSent;

    if (cPkt < 300)
        cPkt = 300;

    memset(&SockAddr, 0, sizeof(SockAddr));
    SockAddr.sin_family = AF_INET;
    SockAddr.sin_port = net_short(DHCP_SERVER_PORT);
    SockAddr.sin_addr.S_un.S_addr = DestIP;
    
    Status = CallSock(Send)(pDhcp->Socket, (char *)pPkt, cPkt,
        0, (SOCKADDR *)&SockAddr, sizeof(SockAddr), &cSent, NULL);

    DEBUGMSG(ZONE_SEND, 
        (TEXT("*DhcpSendDown: Dest %d/%x, Sent %d/%d Status %d\r\n"),
        DHCP_SERVER_PORT, pDhcp->IPAddr, cSent, 
        cPkt, Status));

    return Status;
}    // DhcpSendDown()


#define MAX_LOOP    4

STATUS SendDhcpPkt(DhcpInfo *pDhcp, DhcpPkt *pPkt, int cPkt, 
                   uchar WaitforType, int Flags) {
    STATUS        Status = DHCP_FAILURE;
    int            cLoop, cTimeLeft, cTimeout, cMultiplier;
    int            cFuzz = 0;
    DWORD        cBeginTime, cZeroTime;
    struct timeval    Timeval;
    LPSOCK_INFO    Sock;
    SOCK_LIST    ReadList;
    DhcpPkt        RcvPkt;
    int            cRcvd, cRcvPkt;
    uchar        Type;
    int            fSend = TRUE;
    int            cMaxLoop;
    int            DestIP;

    DEBUGMSG(ZONE_SEND, (TEXT("+SendDhcpPkt: pDhcp %X pPkt %X\r\n"), 
        pDhcp, pPkt));

    Sock = pDhcp->Socket;
    if (BCAST_FL & Flags)
        DestIP = INADDR_BROADCAST;
    else
        DestIP = pDhcp->DhcpServer;

    cRcvPkt = sizeof(RcvPkt);
    CTEFreeLock(&pDhcp->Lock, 0);

	cMaxLoop = Flags & LOOP_MASK_FL;

    cLoop = 0;

    if (pDhcp->Flags & AUTO_IP_ENABLED_FL) {
        cTimeout = 1500;
        cMultiplier = 1;
    } else {
        cTimeout = 4000;
        cMultiplier = 2;
    }

    cZeroTime = GetTickCount();
    // add random fuzz of +/- 1 sec.
    cFuzz = (GetTickCount() % 2000) - 1000;
    cTimeLeft = cTimeout + cFuzz;
    pDhcp->SFlags &= ~DHCPSTATE_SAW_DHCP_SRV;
    do {
        cBeginTime = GetTickCount();
        
        if (fSend) {
            pPkt->Secs = (unsigned short)((cBeginTime - cZeroTime) / 1000);
            if (Status = DhcpSendDown(pDhcp, pPkt, cPkt, DestIP))
                break;
        }

        // wait only 8 secs (2 x min time) after last packet
        if ((cLoop != 1) && ((cLoop + 1) == cMaxLoop) && (cTimeLeft > 8000))
            cTimeLeft = 8000 + cFuzz;
        
        ASSERT((cTimeLeft >= 0) && (cTimeLeft < 62000));
        if (cTimeLeft < 0)
            cTimeLeft = 0;
        if (cTimeLeft > 62000)
            cTimeLeft = 62000;

        // do select w/ timeout
        Timeval.tv_sec = cTimeLeft / 1000;
        Timeval.tv_usec = (cTimeLeft % 1000)*1000;
        
        ReadList.hSocket = 0;
        ReadList.Socket = Sock;
        ReadList.EventMask = READ_EVENTS;

        Status = CallAfd(Select)(1, &ReadList, 0, NULL, 0, NULL, &Timeval, NULL);

        if (Status)
            break;
        if (ReadList.Socket) {    // there is data
            cRcvPkt = sizeof(RcvPkt);
            
            Status = CallSock(Recv)(Sock, (char *)&RcvPkt, cRcvPkt, 0, NULL, NULL, &cRcvd, NULL);

            if (Status) {
                DEBUGMSG(ZONE_SEND, (TEXT("SendDhcpPkt: Recv() failed %d\n"), Status));
                break;
            }

            Type = WaitforType;
            Status = TranslatePkt(pDhcp, &RcvPkt, cRcvd, &Type, pPkt->Xid);

            if (DHCP_SUCCESS == Status) {
                if (Type == WaitforType)
                    break;
                else if (DHCPNACK == Type) {
                    Status = DHCP_NACK;
                    break;
                }
            }
            // this should work as long as the difference is not huge
            cTimeLeft -= GetTickCount() - cBeginTime;
        } else    {    // Select timed out
            DEBUGMSG(ZONE_SEND, (TEXT("SendDhcpPkt: Select() timed out\n")));                        
            Status = DHCP_FAILURE;
            cTimeLeft = 0;
        }

        if (cTimeLeft <= 0) {
            cFuzz = (GetTickCount() % 2000) - 1000;
            cTimeLeft = (cTimeout *= cMultiplier) + cFuzz;
            cLoop++;
            fSend = TRUE;
        } else
            fSend = FALSE;

    } while (cLoop < cMaxLoop);

    DEBUGMSG(ZONE_SEND, (TEXT("-SendDhcpPkt: pDhcp %X pPkt %X Ret: %d\r\n"), 
        pDhcp, pPkt, Status));
    
    CTEGetLock(&pDhcp->Lock, 0);

    return Status;

}    // SendDhcpPkt()



