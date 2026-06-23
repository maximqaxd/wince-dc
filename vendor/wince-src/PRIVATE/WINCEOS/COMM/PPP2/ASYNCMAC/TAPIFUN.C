/* Copyright (c) 1999-2000 Microsoft Corporation.  All rights reserved. */
#include "windows.h"
#include "ndis.h"
#include "ndiswan.h"
#include "tapi.h"
#include "ndistapi.h"
#include "asyncmac.h"
#include "windev.h"		// For IsAPIReady()

#define DEV_CLASS_COMM_DATAMODEM TEXT("comm/datamodem")

// ----------------------------------------------------------------
//
//
//	@func	DWORD	|	TapiLineGetID |
//			Get the handle to the underlying serial port.
//
//	@rdesc	Returns the port handle or INVALID_HANDLE_VALUE.
//
//	@parm	HCALL	|	hCall	| The handle to the call
//
//	@comm
//			This function will get the handle to the underlying port of
//			a TAPI call.  This can be used by ReadFile/WriteFile etc.
//
//	@ex		An example of how to use this function follows |
//			No Example
//
//
HANDLE
TapiLineGetID(HLINE hLine, HCALL hCall)
{
	LPVARSTRING	pVStr;
	long 		lReturn;
	DWORD		dwNeededSize;
	DWORD		dwPortID = (DWORD)INVALID_HANDLE_VALUE;

	DEBUGMSG (ZONE_FUNCTION,
			  (TEXT("+TapilineGetID(0x%X, 0x%X)\r\n"), hLine, hCall));

	dwNeededSize = sizeof(VARSTRING);
	while (1) {
		pVStr = (LPVARSTRING)LocalAlloc (LPTR, dwNeededSize);
		if (NULL == pVStr) {
			break;
		}
		pVStr->dwTotalSize = dwNeededSize;
		lReturn = lineGetID(hLine, 0, hCall, LINECALLSELECT_CALL,
							pVStr, DEV_CLASS_COMM_DATAMODEM);

		if (lReturn) {
			DEBUGMSG (ZONE_ERROR,
					 (TEXT(" lineGetID returned 0x%X\r\n"),
					  lReturn));
			LocalFree (pVStr);
			pVStr = NULL;
			break;
		}

		if (pVStr->dwNeededSize > pVStr->dwTotalSize) {
			dwNeededSize = pVStr->dwNeededSize;
			LocalFree (pVStr);
			pVStr = NULL;
			continue;
		}
		break;
	}

	if (pVStr) {
		dwPortID = *((LPDWORD)((LPBYTE)pVStr + pVStr->dwStringOffset));
		DEBUGMSG (ZONE_FUNCTION,
				  (TEXT(" TapilineGetID ID=0x%X\r\n"), dwPortID));
		LocalFree (pVStr);
	}
	DEBUGMSG (ZONE_FUNCTION,
			  (TEXT("-TapilineGetID Returning 0x%X\r\n"), dwPortID));
	return (HANDLE)dwPortID;
}

void
SendLineDown(
	PASYNCMAC_OPEN_LINE pOpenLine)
{
	NDIS_MAC_LINE_DOWN	MacLineDown;
	PASYNCMAC_ADAPTER	pAdapter;

	ASSERT (CHK_AOL(pOpenLine));
	pAdapter = pOpenLine->pAdapter;
	ASSERT (CHK_AA(pAdapter));

	if (!(pOpenLine->dwFlags & AOL_FLAGS_SENT_LINE_UP))
	{
		RETAILMSG (1, (TEXT("SendLineDown: Can't send line down if we never sent a line up\n")));
	}
	else
	{
		memset ((char *)&MacLineDown, 0, sizeof(MacLineDown));
		MacLineDown.NdisLinkContext = pOpenLine->hNdisLinkContext;

		NdisMIndicateStatus (pAdapter->hMiniportAdapter,
							 NDIS_STATUS_WAN_LINE_DOWN,
							 &MacLineDown, sizeof(MacLineDown));
		
		// Turn off the flag
		pOpenLine->dwFlags &= ~(AOL_FLAGS_SENT_LINE_UP);
	}	
}


// -----------------------------------------------------------------------------
//
//  FUNCTION: lineCallbackFunc(..)
//
//  PURPOSE: Receive asynchronous TAPI events
//
//  PARAMETERS:
//    dwDevice  - Device associated with the event, if any
//    dwMsg     - TAPI event that occurred.
//    dwCallbackInstance - User defined data supplied when opening the line.
//    dwParam1  - dwMsg specific information
//    dwParam2  - dwMsg specific information
//    dwParam3  - dwMsg specific information
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//    This is the function where all asynchronous events will come.
//    Almost all events will be specific to an open line, but a few
//    will be general TAPI events (such as LINE_REINIT).
//
//
// -----------------------------------------------------------------------------
void CALLBACK
lineCallbackFunc(DWORD dwDevice, DWORD dwMsg, DWORD dwCallbackInstance,
                 DWORD dwParam1, DWORD dwParam2, DWORD dwParam3)
{
	PASYNCMAC_OPEN_LINE	pOpenLine = (PASYNCMAC_OPEN_LINE)dwCallbackInstance;
	PASYNCMAC_ADAPTER	pAdapter;
	NDIS_TAPI_EVENT		TapiEvent;
	NDIS_MAC_LINE_UP	MacLineUp;
	DWORD				dwID;

    DEBUGMSG (ZONE_FUNCTION,
              (TEXT("+lineCallbackFunc(0x%X, %d, %d, 0x%X, 0x%X, 0x%X)\r\n"),
               dwDevice, dwMsg, dwCallbackInstance,
               dwParam1, dwParam2, dwParam3));

	TapiEvent.htLine = 0;
	TapiEvent.htCall = 0;
	TapiEvent.ulMsg = dwMsg;
	TapiEvent.ulParam1 = dwParam1;
	TapiEvent.ulParam2 = dwParam2;
	TapiEvent.ulParam3 = dwParam3;

	if (dwMsg == LINE_CREATE)
	{
		extern PASYNCMAC_ADAPTER	v_pAdapter;

		// A new TAPI device like a PCMCIA card modem has been created.
		// Note that TAPI devices never go away, so their is no LINE_DESTROY message.
		DEBUGMSG (ZONE_FUNCTION, (TEXT(" lineCallbackFunc: LINE_CREATE\n")));

		if (v_pAdapter != NULL)
		{
			// Inform the protocol that the count of devices has changed.

			NdisMIndicateStatus (v_pAdapter->hMiniportAdapter,
								 NDIS_STATUS_TAPI_INDICATION,
								 &TapiEvent, sizeof(NDIS_TAPI_EVENT));
		}
		DEBUGMSG (ZONE_FUNCTION, (TEXT("-lineCallbackFunc\n")));
		return;
	}

	if (0 == dwCallbackInstance) {
		DEBUGMSG (ZONE_ERROR, (TEXT("-lineCallbackFunc: dwCallbackInstance==NULL\r\n")));
		return;
	}

	if (NULL == (pOpenLine = GetOpenLinePtr ((void *)dwCallbackInstance))) {
		DEBUGMSG (ZONE_ERROR, (TEXT("-lineCallbackFunc: dwCallbackInstance(0x%X) not a valid pOpenLine\r\n"),
							  dwCallbackInstance));
		return;
	}

	ASSERT (CHK_AOL(pOpenLine));
	pAdapter = pOpenLine->pAdapter;
	ASSERT (CHK_AA(pAdapter));

	switch (dwMsg)
	{
	case LINE_CALLINFO:
		// Information about the call has changed.
		break;

	case LINE_CALLSTATE :
		// Save away the callstate
		pOpenLine->dwCallState = dwParam1;
		switch (dwParam1) {
		case LINECALLSTATE_CONNECTED :
			DEBUGMSG(ZONE_INTERFACE, (TEXT("LINECALLSTATE_CONNECTED\n")));
			// Let's get the handle to the serial port.
			pOpenLine->hPort = TapiLineGetID(pOpenLine->hLine, pOpenLine->hCall);

			ASSERT (pOpenLine->hRxThrd == NULL);

			// Keep track of the line up.
			pOpenLine->dwFlags |= AOL_FLAGS_SENT_LINE_UP;
			
			pOpenLine->hRxThrd = CreateThread (NULL, 0, MacRxThread, pOpenLine, 0, &dwID);

			ASSERT (pOpenLine->hRxThrd != INVALID_HANDLE_VALUE);

			// Set the receive thread priority
			CeSetThreadPriority(pOpenLine->hRxThrd, pAdapter->dwRecvThreadPrio);

			memset ((char *)&MacLineUp, 0, sizeof(MacLineUp));
			MacLineUp.LinkSpeed = pOpenLine->dwBaudRate;
			//	TODO	MacLineUp.Quality =
			MacLineUp.SendWindow = 1;
			MacLineUp.ConnectionWrapperID = (NDIS_HANDLE)pOpenLine->htCall;
			MacLineUp.NdisLinkHandle = pOpenLine;

			NdisMIndicateStatus (pAdapter->hMiniportAdapter,
								 NDIS_STATUS_WAN_LINE_UP,
								 &MacLineUp, sizeof(MacLineUp));
			
			// Save the NdisLinkContext;
			pOpenLine->hNdisLinkContext = MacLineUp.NdisLinkContext;

			break;

		case LINECALLSTATE_IDLE :
			DEBUGMSG(ZONE_INTERFACE, (TEXT("ASYNCMAC: LINECALLSTATE_IDLE\n")));
			if (pOpenLine->dwFlags & AOL_FLAGS_SENT_LINE_UP) {
				SendLineDown (pOpenLine);
				// Kill recv thread.
				while (pOpenLine->hRxThrd) {
					DEBUGMSG (ZONE_TAPI, (TEXT(" lineCallbackFunc: Calling SetCommMask\n")));
					if (SetCommMask (pOpenLine->hPort, 0) == FALSE)
					{
						// Port is invalid
						DEBUGMSG (ZONE_ERROR, (TEXT(" lineCallbackFunc: SetCommMask failed, error=%d\n"), GetLastError()));
						break;
					}
					Sleep (20);
				}
			}
			break;

		case LINECALLSTATE_DISCONNECTED:
			DEBUGMSG (ZONE_FUNCTION, (TEXT(" lineCallbackFunc: LINECALLSTATE_DISCONNECTED\n")));
			break;

		default :
			DEBUGMSG (1, (TEXT(" lineCallbackFunc: Unhandled LINE_CALLSTATE Callback %d\n"), dwParam1));
			break;
		}
		break;
	case LINE_REPLY :
        //
        // dwParam2 has the error code.  If non-zero then something bad has happened.
        //
        if (dwParam2) {
			// Tell the caller that this failed.
//            SetEvent (s_p->TapiEvent);
        }
		break;

	default :
		DEBUGMSG (1, (TEXT(" lineCallbackFunc: Unhandled callback %d\r\n"), dwMsg));
		break;
	}

	// Let's just let the NDISWAN layer know what happened
	if (pOpenLine->hCall) {

		// Check again just in case we had to wait for the lock......
		if (pOpenLine->hCall) {
			TapiEvent.htLine = pOpenLine->htLine;
			TapiEvent.htCall = pOpenLine->htCall;

			NdisMIndicateStatus (pAdapter->hMiniportAdapter,
								 NDIS_STATUS_TAPI_INDICATION,
								 &TapiEvent, sizeof(NDIS_TAPI_EVENT));
		}

	}
	ReleaseOpenLinePtr (pOpenLine);
    DEBUGMSG (ZONE_FUNCTION, (TEXT("-lineCallbackFunc\r\n")));
    return;
}

VOID NDISAPI
DoLineInitialize(PVOID SystemSpecific1,
				 PVOID FunctionContext,
				 PVOID SystemSpecific2,
				 PVOID SystemSpecific3)
{
	PASYNCMAC_ADAPTER	pAdapter = (PASYNCMAC_ADAPTER)FunctionContext;
	long				lReturn;
	NDIS_TAPI_EVENT		TapiEvent;
	DWORD				i;
	LINEEXTENSIONID		lineExtensionID;

	if (IsAPIReady(SH_TAPI)) {
		// Sleep an extra 5 seconds anyway, why?  I don't know but it seems
		// like a good idea, this allows all of the initial devices to be
		// created.
		Sleep(5000);
		lReturn = lineInitialize (&(pAdapter->hLineApp),
								  v_hInstance, lineCallbackFunc, 
								  TEXT("ASYNCMAC"), &(pAdapter->dwNumDevs));

		if (lReturn == 0) {
			DEBUGMSG (1, (TEXT("lineInitialize say's there's %d devices\r\n"),
						  pAdapter->dwNumDevs));

			// Let's figure out the version number to use?
			// We'll just ask for device 0?
			if (lineNegotiateAPIVersion(pAdapter->hLineApp, 0,
				TAPI_CURRENT_VERSION, TAPI_CURRENT_VERSION,  
				&(pAdapter->dwAPIVersion), &lineExtensionID))  {
				DEBUGMSG (1, (TEXT("Error from lineNegotiateAPIVersion?\r\n")));
			}
			

			// Now inform PPP that we have new devices
			memset ((char *)&TapiEvent, 0, sizeof(TapiEvent));
			//TapiEvent.htLine = 0;
			//TapiEvent.htCall = 0;
			TapiEvent.ulMsg = LINE_CREATE;
			for (i=0; i < pAdapter->dwNumDevs; i++) {
				// Indicate device num
				TapiEvent.ulParam1 = i;
				NdisMIndicateStatus(pAdapter->hMiniportAdapter, NDIS_STATUS_TAPI_INDICATION, &TapiEvent, sizeof(TapiEvent));
			}
		}

		DEBUGMSG (ZONE_ERROR && lReturn,
				  (TEXT("ASYNCMAC: Error %d from lineInitialize\r\n")));

	} else {
		DEBUGMSG (1, (TEXT("ASYNCMAC:Tapi not ready yet... Resetting Timer\r\n")));
		NdisMSetTimer (&(pAdapter->ntLineInit), 1000);
	}
}
