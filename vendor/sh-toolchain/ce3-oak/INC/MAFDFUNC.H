/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

Module Name:  

mafdfunc.h

Abstract:  

Notes: 


--*/
  
#ifndef _MAFDFUNC_H_
#define _MAFDFUNC_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef WINCEMACRO
#error WINCEMACRO not defined __FILE__
#endif

#define 	AFDSocket IMPLICIT_DECL (SOCKHAND, SH_COMM, 2,		\
  					(DWORD AddressFamily, 						\
  					 DWORD SocketType, 							\
  					 DWORD Protocol))
  
#define		AFDControl IMPLICIT_DECL (DWORD, SH_COMM, 3,    	\
  					(DWORD Protocol,                        	\
  					 DWORD Action,								\
  					 LPVOID InputBuffer,						\
  					 LPDWORD InputBufferLength,					\
  					 LPVOID OutputBuffer,						\
  					 LPDWORD OutputBufferLength))
  
#define		AFDEnumProtocolsW IMPLICIT_DECL (DWORD, SH_COMM, 4, \
  					 (LPDWORD   lpiProtocols, 					\
  					  LPVOID	lpProtocolBuffer,				\
  					  LPDWORD	lpdwBufferLength))
  
#define		AFDGetHostentByAttr IMPLICIT_DECL (DWORD, SH_COMM, 7,\
  					(LPSOCK_THREAD 			pThread,			\
  					 LPSTR					Name,				\
  					 LPBYTE					Address))	
  
#define		AFDAddIPHostent IMPLICIT_DECL (DWORD, SH_COMM, 8,	\
  					(LPWSTR		 			Hostname,			\
  					 char					**IpAddr,				\
  					 LPWSTR					Aliases,			\
  					 LPFILETIME				lpFileTime))	
  
#define		RasIOControl IMPLICIT_DECL (DWORD, SH_COMM, 9,		\
  					(LPVOID 				hRasConn,			\
  					 DWORD 					dwCode,				\
  					 PBYTE 					pBufIn,				\
  					 DWORD 					dwLenIn, 			\
  					 PBYTE 				    pBufOut, 			\
  					 DWORD 					dwLenOut,			\
  					 PDWORD					pdwActualOut))
  
#define		AFDSelect IMPLICIT_DECL (DWORD, SH_COMM, 10,		\
  					(UINT					ReadCount,			\
  					 LPSOCK_LIST			ReadList,			\
  					 UINT					WriteCount,			\
  					 LPSOCK_LIST			WriteList,			\
  					 UINT					ExceptCount,		\
  					 LPSOCK_LIST			ExceptList,			\
  					 const struct timeval	*timeout,			\
  					 CRITICAL_SECTION		*pDllCS))
  

#define NEW_RESOLVER	1

#ifdef NEW_RESOLVER

#define		AFDAddInterface IMPLICIT_DECL (DWORD, SH_COMM, 19,	\
  					(PTSTR					pAdapter,			\
					 VOID					*Nte,				\
					 DWORD					CONTEXT,			\
					 int					Flags,				\
					 uint					IpAddr,				\
					 uint					SubnetMask,			\
					 int					cDns,				\
					 uint					*pDns,				\
					 int					cWins,				\
					 uint					*pWins))

#else  					 				
#define		AFDAddResolver IMPLICIT_DECL (DWORD, SH_COMM, 19,	\
  					(DWORD					CONTEXT,			\
  					 BOOL					fDNS,				\
  					 BOOL					fDelete,			\
  					 DWORD					IpAddr))
  
#endif

#define		RasDial IMPLICIT_DECL (DWORD, SH_COMM, 5,			\
					(LPRASDIALEXTENSIONS 	dialExtensions,		\
					 LPTSTR		    		phoneBookPath,		\
					 LPRASDIALPARAMS    	rasDialParam,		\
					 DWORD		    		NotifierType,		\
					 LPVOID		    		notifier,			\
					 LPHRASCONN	    		pRasConn))	

#define		RasHangup IMPLICIT_DECL (DWORD, SH_COMM, 6,			\
					(HRASCONN				Session))	

#define		RasEnumEntries IMPLICIT_DECL (DWORD, SH_COMM, 11,	\
					(LPWSTR					Reserved,			\
					 LPWSTR					lpszPhoneBookPath,	\
					 LPRASENTRYNAME			lprasentryname,		\
					 LPDWORD				lpcb,				\
					 LPDWORD				lpcEntries))
					 				
#define		RasGetEntryDialParams IMPLICIT_DECL (DWORD, SH_COMM, 12,\
					(LPWSTR					lpszPhoneBook,		\
					 LPRASDIALPARAMS		lpRasDialParams,	\
					 LPBOOL					lpfPassword))
					 				
#define		RasSetEntryDialParams IMPLICIT_DECL (DWORD, SH_COMM, 13,\
					(LPWSTR					lpszPhoneBook,		\
					 LPRASDIALPARAMS		lpRasDialParams,	\
					 BOOL					fRemovePassword))

#define		RasGetEntryProperties IMPLICIT_DECL (DWORD, SH_COMM, 14,\
					(LPWSTR					lpszPhoneBook,		\
					 LPWSTR					szEntry,			\
					 LPBYTE					lpbEntry,			\
					 LPDWORD				lpdwEntrySize,		\
					 LPBYTE					lpb,				\
					 LPDWORD				lpdwSize))
					 				
#define		RasSetEntryProperties IMPLICIT_DECL (DWORD, SH_COMM, 15,\
					(LPWSTR					lpszPhoneBook,		\
					 LPWSTR					szEntry,			\
					 LPBYTE					lpbEntry,			\
					 DWORD					dwEntrySize,		\
					 LPBYTE					lpb,				\
					 DWORD					dwSize))

#define		RasValidateEntryName IMPLICIT_DECL (DWORD, SH_COMM, 16, \
					(LPWSTR					lpszPhonebook,		\
					 LPWSTR					lpszEntry))

#define		RasDeleteEntry IMPLICIT_DECL (DWORD, SH_COMM, 17,	\
					(LPWSTR					lpszPhonebook,		\
					 LPWSTR					lpszEntry))

#define		RasRenameEntry IMPLICIT_DECL (DWORD, SH_COMM, 18,	\
					(LPWSTR					lpszPhonebook,		\
					 LPWSTR					lpszOldEntry,		\
					 LPWSTR					lpszNewEntry))
					 				
#define		RasEnumConnections IMPLICIT_DECL (DWORD, SH_COMM, 20,	\
					(LPRASCONN				lprasconn,				\
					 LPDWORD				lpcb,					\
					 LPDWORD				lpcConnections))

#define		RasGetConnectStatus IMPLICIT_DECL (DWORD, SH_COMM, 21,	\
					(HRASCONN				rasconn,				\
					 LPRASCONNSTATUS		lprasconnstatus))

#define     RasGetEntryDevConfig IMPLICIT_DECL (DWORD, SH_COMM, 22, \
                    (LPCTSTR                szPhonebook,            \
                     LPCTSTR                szEntry,                \
                     LPDWORD                pdwDeviceID,            \
                     LPDWORD                pdwSize,                \
                     LPVARSTRING            pDeviceConfig))

#define     RasSetEntryDevConfig IMPLICIT_DECL (DWORD, SH_COMM, 23, \
                    (LPCTSTR                szPhonebook,            \
                     LPCTSTR                szEntry,                \
                     DWORD                  dwDeviceID,             \
                     LPVARSTRING            lpDeviceConfig))

#define		NETbios IMPLICIT_DECL (DWORD, SH_COMM, 24,		\
					(DWORD				x1,					\
					 DWORD				dwOpCode,			\
					 PVOID				pNCB,				\
					 DWORD				cBuf1,		\
					 PBYTE				pBuf1,				\
					 DWORD				cBuf2,				\
					 PDWORD				pBuf2))

#ifdef __cplusplus
}
#endif

#endif // __MAFDFUNC.H
