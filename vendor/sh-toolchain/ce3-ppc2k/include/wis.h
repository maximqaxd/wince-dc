/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.02.88 */
/* at Tue Sep 22 18:40:44 1998
 */
/* Compiler settings for .\wis.idl:
    Os (OptLev=s), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: none
*/
//@@MIDL_FILE_HEADING(  )
#include "rpc.h"
#include "rpcndr.h"
#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __wis_h__
#define __wis_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IRouteList_FWD_DEFINED__
#define __IRouteList_FWD_DEFINED__
typedef interface IRouteList IRouteList;
#endif 	/* __IRouteList_FWD_DEFINED__ */


#ifndef __IWSEventLog_FWD_DEFINED__
#define __IWSEventLog_FWD_DEFINED__
typedef interface IWSEventLog IWSEventLog;
#endif 	/* __IWSEventLog_FWD_DEFINED__ */


#ifndef __IWISModuleInformation_FWD_DEFINED__
#define __IWISModuleInformation_FWD_DEFINED__
typedef interface IWISModuleInformation IWISModuleInformation;
#endif 	/* __IWISModuleInformation_FWD_DEFINED__ */


#ifndef __ITranslator_FWD_DEFINED__
#define __ITranslator_FWD_DEFINED__
typedef interface ITranslator ITranslator;
#endif 	/* __ITranslator_FWD_DEFINED__ */


#ifndef __IAcquisition_FWD_DEFINED__
#define __IAcquisition_FWD_DEFINED__
typedef interface IAcquisition IAcquisition;
#endif 	/* __IAcquisition_FWD_DEFINED__ */


#ifndef __ITransmitter_FWD_DEFINED__
#define __ITransmitter_FWD_DEFINED__
typedef interface ITransmitter ITransmitter;
#endif 	/* __ITransmitter_FWD_DEFINED__ */


#ifndef __IWISAppFilter_FWD_DEFINED__
#define __IWISAppFilter_FWD_DEFINED__
typedef interface IWISAppFilter IWISAppFilter;
#endif 	/* __IWISAppFilter_FWD_DEFINED__ */


#ifndef __IPropPageUI_FWD_DEFINED__
#define __IPropPageUI_FWD_DEFINED__
typedef interface IPropPageUI IPropPageUI;
#endif 	/* __IPropPageUI_FWD_DEFINED__ */


/* header files for imported files */
#include "unknwn.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/****************************************
 * Generated header for interface: __MIDL_itf_wis_0000
 * at Tue Sep 22 18:40:44 1998
 * using MIDL 3.02.88
 ****************************************/
/* [local] */ 


#include <WinCrypt.h>
#include <rio.h>
typedef 
enum RouteDir_e
    {	DIR_RECEIVE	= 0,
	DIR_TRANSMIT	= DIR_RECEIVE + 1
    }	ROUTEDIR_E;

typedef struct _route_node ROUTE_NODE;

typedef ROUTE_NODE __RPC_FAR *LPROUTE_NODE;

struct  _route_node
    {
    DWORD dwTagLen;
    BYTE __RPC_FAR *pTag;
    LPVOID pInterface;
    LPCLSID pClsid;
    LPIID pIid;
    LPROUTE_NODE pNext;
    LPROUTE_NODE pPrev;
    };
typedef struct  _route_list
    {
    DWORD dwNumTags;
    LPROUTE_NODE pHeadRL;
    LPROUTE_NODE pCurRL;
    ROUTEDIR_E Dir;
    }	ROUTE_LIST;

typedef struct _route_list __RPC_FAR *LPROUTE_LIST;

typedef const ROUTE_LIST __RPC_FAR *LPCROUTE_LIST;



extern RPC_IF_HANDLE __MIDL_itf_wis_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_wis_0000_v0_0_s_ifspec;

#ifndef __IRouteList_INTERFACE_DEFINED__
#define __IRouteList_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IRouteList
 * at Tue Sep 22 18:40:44 1998
 * using MIDL 3.02.88
 ****************************************/
/* [object][unique][helpstring][uuid][local] */ 



EXTERN_C const IID IID_IRouteList;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("DAB64DA0-9C27-11d1-AC28-00C04FCCAF8B")
    IRouteList : public IUnknown
    {
    public:
        virtual DWORD STDMETHODCALLTYPE GetRL_Length( void) = 0;
        
        virtual DWORD STDMETHODCALLTYPE GetRL( 
            /* [out][in] */ BYTE __RPC_FAR *pTagList,
            /* [in] */ DWORD dwSize) = 0;
        
        virtual DWORD STDMETHODCALLTYPE SetRL( 
            /* [in] */ BYTE __RPC_FAR *pTagList) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CompairCurrentTag( 
            /* [in] */ BYTE __RPC_FAR *pTag) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Head( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Next( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Prev( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Tail( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Prepend( 
            /* [in] */ BYTE __RPC_FAR *lpTag) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Append( 
            /* [in] */ BYTE __RPC_FAR *lpTag) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE InsertBefore( 
            /* [in] */ BYTE __RPC_FAR *lpTag) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE InsertAfter( 
            /* [in] */ BYTE __RPC_FAR *lpTag) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoveCur( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoveAll( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoveHead( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoveTail( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Initialize( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE InitializeCur( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRouteListVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRouteList __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRouteList __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRouteList __RPC_FAR * This);
        
        DWORD ( STDMETHODCALLTYPE __RPC_FAR *GetRL_Length )( 
            IRouteList __RPC_FAR * This);
        
        DWORD ( STDMETHODCALLTYPE __RPC_FAR *GetRL )( 
            IRouteList __RPC_FAR * This,
            /* [out][in] */ BYTE __RPC_FAR *pTagList,
            /* [in] */ DWORD dwSize);
        
        DWORD ( STDMETHODCALLTYPE __RPC_FAR *SetRL )( 
            IRouteList __RPC_FAR * This,
            /* [in] */ BYTE __RPC_FAR *pTagList);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CompairCurrentTag )( 
            IRouteList __RPC_FAR * This,
            /* [in] */ BYTE __RPC_FAR *pTag);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Head )( 
            IRouteList __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IRouteList __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Prev )( 
            IRouteList __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Tail )( 
            IRouteList __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Prepend )( 
            IRouteList __RPC_FAR * This,
            /* [in] */ BYTE __RPC_FAR *lpTag);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Append )( 
            IRouteList __RPC_FAR * This,
            /* [in] */ BYTE __RPC_FAR *lpTag);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *InsertBefore )( 
            IRouteList __RPC_FAR * This,
            /* [in] */ BYTE __RPC_FAR *lpTag);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *InsertAfter )( 
            IRouteList __RPC_FAR * This,
            /* [in] */ BYTE __RPC_FAR *lpTag);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RemoveCur )( 
            IRouteList __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RemoveAll )( 
            IRouteList __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RemoveHead )( 
            IRouteList __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RemoveTail )( 
            IRouteList __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Initialize )( 
            IRouteList __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *InitializeCur )( 
            IRouteList __RPC_FAR * This);
        
        END_INTERFACE
    } IRouteListVtbl;

    interface IRouteList
    {
        CONST_VTBL struct IRouteListVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRouteList_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRouteList_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRouteList_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRouteList_GetRL_Length(This)	\
    (This)->lpVtbl -> GetRL_Length(This)

#define IRouteList_GetRL(This,pTagList,dwSize)	\
    (This)->lpVtbl -> GetRL(This,pTagList,dwSize)

#define IRouteList_SetRL(This,pTagList)	\
    (This)->lpVtbl -> SetRL(This,pTagList)

#define IRouteList_CompairCurrentTag(This,pTag)	\
    (This)->lpVtbl -> CompairCurrentTag(This,pTag)

#define IRouteList_Head(This)	\
    (This)->lpVtbl -> Head(This)

#define IRouteList_Next(This)	\
    (This)->lpVtbl -> Next(This)

#define IRouteList_Prev(This)	\
    (This)->lpVtbl -> Prev(This)

#define IRouteList_Tail(This)	\
    (This)->lpVtbl -> Tail(This)

#define IRouteList_Prepend(This,lpTag)	\
    (This)->lpVtbl -> Prepend(This,lpTag)

#define IRouteList_Append(This,lpTag)	\
    (This)->lpVtbl -> Append(This,lpTag)

#define IRouteList_InsertBefore(This,lpTag)	\
    (This)->lpVtbl -> InsertBefore(This,lpTag)

#define IRouteList_InsertAfter(This,lpTag)	\
    (This)->lpVtbl -> InsertAfter(This,lpTag)

#define IRouteList_RemoveCur(This)	\
    (This)->lpVtbl -> RemoveCur(This)

#define IRouteList_RemoveAll(This)	\
    (This)->lpVtbl -> RemoveAll(This)

#define IRouteList_RemoveHead(This)	\
    (This)->lpVtbl -> RemoveHead(This)

#define IRouteList_RemoveTail(This)	\
    (This)->lpVtbl -> RemoveTail(This)

#define IRouteList_Initialize(This)	\
    (This)->lpVtbl -> Initialize(This)

#define IRouteList_InitializeCur(This)	\
    (This)->lpVtbl -> InitializeCur(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



DWORD STDMETHODCALLTYPE IRouteList_GetRL_Length_Proxy( 
    IRouteList __RPC_FAR * This);


void __RPC_STUB IRouteList_GetRL_Length_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


DWORD STDMETHODCALLTYPE IRouteList_GetRL_Proxy( 
    IRouteList __RPC_FAR * This,
    /* [out][in] */ BYTE __RPC_FAR *pTagList,
    /* [in] */ DWORD dwSize);


void __RPC_STUB IRouteList_GetRL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


DWORD STDMETHODCALLTYPE IRouteList_SetRL_Proxy( 
    IRouteList __RPC_FAR * This,
    /* [in] */ BYTE __RPC_FAR *pTagList);


void __RPC_STUB IRouteList_SetRL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRouteList_CompairCurrentTag_Proxy( 
    IRouteList __RPC_FAR * This,
    /* [in] */ BYTE __RPC_FAR *pTag);


void __RPC_STUB IRouteList_CompairCurrentTag_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRouteList_Head_Proxy( 
    IRouteList __RPC_FAR * This);


void __RPC_STUB IRouteList_Head_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRouteList_Next_Proxy( 
    IRouteList __RPC_FAR * This);


void __RPC_STUB IRouteList_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRouteList_Prev_Proxy( 
    IRouteList __RPC_FAR * This);


void __RPC_STUB IRouteList_Prev_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRouteList_Tail_Proxy( 
    IRouteList __RPC_FAR * This);


void __RPC_STUB IRouteList_Tail_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRouteList_Prepend_Proxy( 
    IRouteList __RPC_FAR * This,
    /* [in] */ BYTE __RPC_FAR *lpTag);


void __RPC_STUB IRouteList_Prepend_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRouteList_Append_Proxy( 
    IRouteList __RPC_FAR * This,
    /* [in] */ BYTE __RPC_FAR *lpTag);


void __RPC_STUB IRouteList_Append_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRouteList_InsertBefore_Proxy( 
    IRouteList __RPC_FAR * This,
    /* [in] */ BYTE __RPC_FAR *lpTag);


void __RPC_STUB IRouteList_InsertBefore_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRouteList_InsertAfter_Proxy( 
    IRouteList __RPC_FAR * This,
    /* [in] */ BYTE __RPC_FAR *lpTag);


void __RPC_STUB IRouteList_InsertAfter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRouteList_RemoveCur_Proxy( 
    IRouteList __RPC_FAR * This);


void __RPC_STUB IRouteList_RemoveCur_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRouteList_RemoveAll_Proxy( 
    IRouteList __RPC_FAR * This);


void __RPC_STUB IRouteList_RemoveAll_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRouteList_RemoveHead_Proxy( 
    IRouteList __RPC_FAR * This);


void __RPC_STUB IRouteList_RemoveHead_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRouteList_RemoveTail_Proxy( 
    IRouteList __RPC_FAR * This);


void __RPC_STUB IRouteList_RemoveTail_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRouteList_Initialize_Proxy( 
    IRouteList __RPC_FAR * This);


void __RPC_STUB IRouteList_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRouteList_InitializeCur_Proxy( 
    IRouteList __RPC_FAR * This);


void __RPC_STUB IRouteList_InitializeCur_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRouteList_INTERFACE_DEFINED__ */


#ifndef __IWSEventLog_INTERFACE_DEFINED__
#define __IWSEventLog_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IWSEventLog
 * at Tue Sep 22 18:40:44 1998
 * using MIDL 3.02.88
 ****************************************/
/* [object][unique][helpstring][uuid][local] */ 



EXTERN_C const IID IID_IWSEventLog;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("31B2B920-EB78-11d1-8005-0000F803FFBE")
    IWSEventLog : public IUnknown
    {
    public:
        virtual BOOL STDMETHODCALLTYPE WriteEventLog( 
            /* [in] */ LPCWSTR lpwszSource,
            /* [in] */ LPCWSTR lpwszEvent,
            /* [in] */ LPCWSTR lpwszData) = 0;
        
        virtual void STDMETHODCALLTYPE ShowLastError( 
            /* [in] */ LPCTSTR lpszRoutine,
            /* [in] */ LPCTSTR lpszCallName) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWSEventLogVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWSEventLog __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWSEventLog __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWSEventLog __RPC_FAR * This);
        
        BOOL ( STDMETHODCALLTYPE __RPC_FAR *WriteEventLog )( 
            IWSEventLog __RPC_FAR * This,
            /* [in] */ LPCWSTR lpwszSource,
            /* [in] */ LPCWSTR lpwszEvent,
            /* [in] */ LPCWSTR lpwszData);
        
        void ( STDMETHODCALLTYPE __RPC_FAR *ShowLastError )( 
            IWSEventLog __RPC_FAR * This,
            /* [in] */ LPCTSTR lpszRoutine,
            /* [in] */ LPCTSTR lpszCallName);
        
        END_INTERFACE
    } IWSEventLogVtbl;

    interface IWSEventLog
    {
        CONST_VTBL struct IWSEventLogVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWSEventLog_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWSEventLog_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWSEventLog_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWSEventLog_WriteEventLog(This,lpwszSource,lpwszEvent,lpwszData)	\
    (This)->lpVtbl -> WriteEventLog(This,lpwszSource,lpwszEvent,lpwszData)

#define IWSEventLog_ShowLastError(This,lpszRoutine,lpszCallName)	\
    (This)->lpVtbl -> ShowLastError(This,lpszRoutine,lpszCallName)

#endif /* COBJMACROS */


#endif 	/* C style interface */



BOOL STDMETHODCALLTYPE IWSEventLog_WriteEventLog_Proxy( 
    IWSEventLog __RPC_FAR * This,
    /* [in] */ LPCWSTR lpwszSource,
    /* [in] */ LPCWSTR lpwszEvent,
    /* [in] */ LPCWSTR lpwszData);


void __RPC_STUB IWSEventLog_WriteEventLog_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


void STDMETHODCALLTYPE IWSEventLog_ShowLastError_Proxy( 
    IWSEventLog __RPC_FAR * This,
    /* [in] */ LPCTSTR lpszRoutine,
    /* [in] */ LPCTSTR lpszCallName);


void __RPC_STUB IWSEventLog_ShowLastError_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWSEventLog_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL_itf_wis_0007
 * at Tue Sep 22 18:40:44 1998
 * using MIDL 3.02.88
 ****************************************/
/* [local] */ 


#define MAX_SOCKET_DATA_LEN 1024
#define ROUTER_PORT 0xAA0


extern RPC_IF_HANDLE __MIDL_itf_wis_0007_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_wis_0007_v0_0_s_ifspec;

#ifndef __IWisTypes_INTERFACE_DEFINED__
#define __IWisTypes_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IWisTypes
 * at Tue Sep 22 18:40:44 1998
 * using MIDL 3.02.88
 ****************************************/
/* [auto_handle][unique][version][uuid] */ 


typedef struct  _socketinfo
    {
    BYTE ChannelNameLen;
    BYTE Source;
    DWORD Id;
    }	SOCKETINFO;

typedef struct _socketinfo __RPC_FAR *LPSOCKETINFO;

#define WIS_SOURCE_UNKNOWN  0
#define WIS_SOURCE_RADIO    1
#define WIS_SOURCE_DESKTOP  2
#define WIS_SOURCE_MODEM    3
#define WIS_SOURCE_IR       4
#define WIS_XMIT_PACKET_REPEAT  0x00000001
#define WIS_NO_UID_IF_NO_PACKAGING_NEEDED  0x00000002
typedef struct  _xmitinfo
    {
    DWORD cbSize;
    DWORD cmInterval;
    DWORD cbMaxPerInterval;
    DWORD cbMaxBlock;
    DWORD csInterBlockGap;
    DWORD csInterMessageGap;
    DWORD fSettings;
    DWORD dwRepeatCount;
    }	XMITINFO;

typedef struct _xmitinfo __RPC_FAR *LPXMITINFO;

#define WIS_ACQUIRE   0x00000001
#define WIS_TRANSMIT  0x00000002
#define WIS_BULK		 0x00000004
typedef struct  _chaninfo
    {
    DWORD cbSize;
    HWND hParent;
    LPCWSTR szDataFeedId;
    BYTE GroupCode;
    LPCWSTR szBaseConfigKey;
    LPCWSTR szXmitBufferPath;
    LPCWSTR szOrignalName;
    DWORD dwOrignalSize;
    DWORD dwFlags;
    HANDLE hShutdownEvent;
    HANDLE hPauseEvent;
    XMITINFO Xmit;
    }	CHANINFO;

typedef struct _chaninfo __RPC_FAR *LPCHANINFO;

typedef struct  _xtramsginfo
    {
    DWORD cbSize;
    BYTE MsgType;
    BYTE MsgPriority;
    BYTE MsgFlags;
    BYTE NumParts;
    WORD wErrorFlags;
    WORD wMsgSequenceNumber;
    }	XTRAMSGINFO;

typedef struct _xtramsginfo __RPC_FAR *LPXTRAMSGINFO;

typedef struct  _msginfo
    {
    DWORD cbSize;
    LPCWSTR pszFileName;
    LPCWSTR pszErrFileName;
    LPCWSTR pszResponseFileName;
    LPCWSTR pszOEMFileName;
    LPCWSTR pszFolderName;
    LPCWSTR pszChannelName;
    LPCWSTR pszId;
    LPCWSTR pszDeviceName;
    BYTE AddressTagLen;
    BYTE __RPC_FAR *pAddressTag;
    BYTE GroupTagLen;
    BYTE __RPC_FAR *pGroupTag;
    BYTE FilterLen;
    BYTE __RPC_FAR *pFilterBytes;
    LPCROUTE_LIST pRL;
    IRouteList __RPC_FAR *pRLControl;
    HANDLE hRadio;
    BYTE Source;
    BYTE Device;
    SYSTEMTIME DateTime;
    LPROUTE_NODE pReceiver;
    LPROUTE_NODE pPackager;
    LPROUTE_NODE pXmitter;
    DWORD dwReturnAddressInfoLen;
    LPVOID pReturnAddressInfo;
    LPCHANINFO lpChan;
    IWSEventLog __RPC_FAR *pWSEventLog;
    XTRAMSGINFO __RPC_FAR *pXtraMsgInfo;
    }	MSGINFO;

typedef struct _msginfo __RPC_FAR *LPMSGINFO;

#define PGM_CRYPTO_PROVIDER     (PROV_RSA_FULL)
#define PGM_CRYPTO_HASH_ALGID   (CALG_SHA)
#define PGM_CRYPTO_HASH_FLAGS   (0)
#define PGM_CRYPTO_ALG_ID       (CALG_RC4)
#define PGM_CRYPTO_FLAGS        (CRYPT_EXPORTABLE)
#define PGM_CRYPTO_OPTIONS_USE_DEFAULTS     0x0000
#define PGM_CRYPTO_OPTIONS_USE_GIVEN        0x0001
void DwordToNetworkOrder(BYTE *pbDest, DWORD dwSrc);
void NetworkOrderToDword(DWORD *pdwDest, BYTE *pbSrc);
void WordToNetworkOrder(BYTE *pbDest, WORD wSrc);
void NetworkOrderToWord(WORD *pwDest, BYTE *pbSrc);
HRESULT PgmSignBlob(
    LPBYTE pbBaseKey,
    DWORD  dwBaseKeyLen,
    LPBYTE pbUniqueData,
    DWORD dwUniqueDataLen,
    LPBYTE pbBlob,
    DWORD dwBlobLen,
    HCRYPTPROV hCryptoProv,
    DWORD dwHashAlgId,
    DWORD dwHashFlags,
    LPBYTE pbSignature,
    DWORD dwMaxSignatureSize,
    DWORD *pdwActualBytesWritten
    );
HCRYPTKEY PgmGetCryptKey (
    LPBYTE pbBaseKey,
    DWORD  dwBaseKeyLen,
    LPBYTE pbUniqueData,
    DWORD dwUniqueDataLen,
    HCRYPTPROV hCryptoProv,
    DWORD dwHashAlgId,
    DWORD dwHashFlags,
    DWORD dwCryptoAlgId,
    DWORD dwCryptoFlags
    );
HRESULT PgmEncryptAndSignBlob(
    LPBYTE pbBaseKey,
    DWORD  dwBaseKeyLen,
    LPBYTE pbUniqueData,
    DWORD dwUniqueDataLen,
    LPBYTE pbBlob,
    DWORD dwBlobLen,
    HCRYPTPROV hCryptoProv,
    DWORD dwHashAlgId,
    DWORD dwHashFlags,
    DWORD dwCryptoAlgId,
    DWORD dwCryptoFlags,
    LPBYTE pbOutputBlob,
    DWORD dwMaxOutputBlobSize,
    DWORD *pdwActualBytesWritten
    );
HRESULT PgmVerifyAndDecryptBlob(
    LPBYTE pbBaseKey,
    DWORD  dwBaseKeyLen,
    LPBYTE pbBlob,
    DWORD dwBlobLen,
    HCRYPTPROV hCryptoProv,
    DWORD dwHashAlgId,
    DWORD dwHashFlags,
    DWORD dwCryptoAlgId,
    DWORD dwCryptoFlags,
    LPBYTE pbOutputBlob,
    DWORD dwMaxOutputBlobSize,
    DWORD *pdwActualBytesWritten
    );
 
typedef struct _radio_crypt  {           
    WORD wStructSize;                    
    DWORD dwMemberValidMask;             
    HCRYPTPROV hCryptoProv;              
    DWORD dwCryptoFlags;                 
    DWORD dwCryptoAlgId;                 
    BYTE AddressTagLen;                  
    BYTE GroupTagLen;                    
    WORD wMsgSpecificDataLen;            
//  BYTE AddressTag[AddressTagLen];      
//  BYTE GroupTag[GroupTagLen];          
} RADIO_CRYPT, *LPRADIO_CRYPT;           
 
BOOL DeriveEncryptionKey (
    LPRADIO_CRYPT pSecurity,
    BYTE *pbKeyValue,
    DWORD dwKeySize,
    HCRYPTKEY *phKey
);


extern RPC_IF_HANDLE IWisTypes_v0_1_c_ifspec;
extern RPC_IF_HANDLE IWisTypes_v0_1_s_ifspec;
#endif /* __IWisTypes_INTERFACE_DEFINED__ */

/****************************************
 * Generated header for interface: __MIDL_itf_wis_0008
 * at Tue Sep 22 18:40:44 1998
 * using MIDL 3.02.88
 ****************************************/
/* [local] */ 


typedef struct  _MODULE_INFO
    {
    DWORD dwTagSize;
    BYTE __RPC_FAR *lpTag;
    LPCWSTR szType;
    LPCWSTR szFriendlyName;
    LPCWSTR szDiscription;
    LPCWSTR szVersion;
    LPCWSTR szManufacturer;
    }	MODULE_INFO;

typedef struct _MODULE_INFO __RPC_FAR *LPMODULE_INFO;



extern RPC_IF_HANDLE __MIDL_itf_wis_0008_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_wis_0008_v0_0_s_ifspec;

#ifndef __IWISModuleInformation_INTERFACE_DEFINED__
#define __IWISModuleInformation_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IWISModuleInformation
 * at Tue Sep 22 18:40:44 1998
 * using MIDL 3.02.88
 ****************************************/
/* [object][unique][helpstring][uuid][local] */ 



EXTERN_C const IID IID_IWISModuleInformation;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("637AF7C0-99BC-11d1-AC28-00C04FCCAF8B")
    IWISModuleInformation : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetInfo( 
            LPMODULE_INFO pInfo) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWISModuleInformationVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWISModuleInformation __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWISModuleInformation __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWISModuleInformation __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetInfo )( 
            IWISModuleInformation __RPC_FAR * This,
            LPMODULE_INFO pInfo);
        
        END_INTERFACE
    } IWISModuleInformationVtbl;

    interface IWISModuleInformation
    {
        CONST_VTBL struct IWISModuleInformationVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWISModuleInformation_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWISModuleInformation_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWISModuleInformation_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWISModuleInformation_GetInfo(This,pInfo)	\
    (This)->lpVtbl -> GetInfo(This,pInfo)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWISModuleInformation_GetInfo_Proxy( 
    IWISModuleInformation __RPC_FAR * This,
    LPMODULE_INFO pInfo);


void __RPC_STUB IWISModuleInformation_GetInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWISModuleInformation_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL_itf_wis_0009
 * at Tue Sep 22 18:40:44 1998
 * using MIDL 3.02.88
 ****************************************/
/* [local] */ 


#define TRANS_CONTINUE           0x200
#define TRANS_STOP               0x201
#define TRANS_S_CONTINUE     MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_ITF, TRANS_CONTINUE) 
#define TRANS_E_CONTINUE     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, TRANS_CONTINUE) 
#define TRANS_S_STOP         MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_ITF, TRANS_STOP) 
#define TRANS_E_STOP         MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, TRANS_STOP) 
#define TRANS_CONTINUE_PROCESSING(hresult)  (HRESULT_CODE(hresult) == TRANS_CONTINUE)
#define TRANS_STOP_PROCESSING(hresult)      (HRESULT_CODE(hresult) == TRANS_STOP)


extern RPC_IF_HANDLE __MIDL_itf_wis_0009_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_wis_0009_v0_0_s_ifspec;

#ifndef __ITranslator_INTERFACE_DEFINED__
#define __ITranslator_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ITranslator
 * at Tue Sep 22 18:40:44 1998
 * using MIDL 3.02.88
 ****************************************/
/* [object][unique][helpstring][uuid][local] */ 



EXTERN_C const IID IID_ITranslator;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("CB5ACD52-FE25-11d0-AABD-00A0C90A8F90")
    ITranslator : public IWISModuleInformation
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ProcessMSG( 
            /* [out][in] */ LPMSGINFO pMSG) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetLastError( 
            /* [out][in] */ LPWSTR pszError,
            /* [in] */ DWORD cbSize) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITranslatorVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITranslator __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITranslator __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITranslator __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetInfo )( 
            ITranslator __RPC_FAR * This,
            LPMODULE_INFO pInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ProcessMSG )( 
            ITranslator __RPC_FAR * This,
            /* [out][in] */ LPMSGINFO pMSG);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetLastError )( 
            ITranslator __RPC_FAR * This,
            /* [out][in] */ LPWSTR pszError,
            /* [in] */ DWORD cbSize);
        
        END_INTERFACE
    } ITranslatorVtbl;

    interface ITranslator
    {
        CONST_VTBL struct ITranslatorVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITranslator_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITranslator_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITranslator_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITranslator_GetInfo(This,pInfo)	\
    (This)->lpVtbl -> GetInfo(This,pInfo)


#define ITranslator_ProcessMSG(This,pMSG)	\
    (This)->lpVtbl -> ProcessMSG(This,pMSG)

#define ITranslator_GetLastError(This,pszError,cbSize)	\
    (This)->lpVtbl -> GetLastError(This,pszError,cbSize)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITranslator_ProcessMSG_Proxy( 
    ITranslator __RPC_FAR * This,
    /* [out][in] */ LPMSGINFO pMSG);


void __RPC_STUB ITranslator_ProcessMSG_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITranslator_GetLastError_Proxy( 
    ITranslator __RPC_FAR * This,
    /* [out][in] */ LPWSTR pszError,
    /* [in] */ DWORD cbSize);


void __RPC_STUB ITranslator_GetLastError_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITranslator_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL_itf_wis_0010
 * at Tue Sep 22 18:40:44 1998
 * using MIDL 3.02.88
 ****************************************/
/* [local] */ 


#define ACQUIS_E_NO_MORE_DATA    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x200) 
#define ACQUIS_E_UNKNOWN_ID      MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x201) 


extern RPC_IF_HANDLE __MIDL_itf_wis_0010_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_wis_0010_v0_0_s_ifspec;

#ifndef __IAcquisition_INTERFACE_DEFINED__
#define __IAcquisition_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IAcquisition
 * at Tue Sep 22 18:40:44 1998
 * using MIDL 3.02.88
 ****************************************/
/* [object][unique][helpstring][uuid][local] */ 



EXTERN_C const IID IID_IAcquisition;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("4A092560-1E52-11d1-AACF-00A0C90A8F90")
    IAcquisition : public IWISModuleInformation
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Acquire( 
            /* [out][in] */ LPMSGINFO pMSG) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFirst( 
            /* [out][in] */ LPMSGINFO pMSG) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNext( 
            /* [out][in] */ LPMSGINFO pMSG) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetLastError( 
            /* [out][in] */ LPWSTR pszError,
            /* [in] */ DWORD cbSize) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAcquisitionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IAcquisition __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IAcquisition __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IAcquisition __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetInfo )( 
            IAcquisition __RPC_FAR * This,
            LPMODULE_INFO pInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Acquire )( 
            IAcquisition __RPC_FAR * This,
            /* [out][in] */ LPMSGINFO pMSG);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetFirst )( 
            IAcquisition __RPC_FAR * This,
            /* [out][in] */ LPMSGINFO pMSG);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNext )( 
            IAcquisition __RPC_FAR * This,
            /* [out][in] */ LPMSGINFO pMSG);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetLastError )( 
            IAcquisition __RPC_FAR * This,
            /* [out][in] */ LPWSTR pszError,
            /* [in] */ DWORD cbSize);
        
        END_INTERFACE
    } IAcquisitionVtbl;

    interface IAcquisition
    {
        CONST_VTBL struct IAcquisitionVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAcquisition_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAcquisition_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAcquisition_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAcquisition_GetInfo(This,pInfo)	\
    (This)->lpVtbl -> GetInfo(This,pInfo)


#define IAcquisition_Acquire(This,pMSG)	\
    (This)->lpVtbl -> Acquire(This,pMSG)

#define IAcquisition_GetFirst(This,pMSG)	\
    (This)->lpVtbl -> GetFirst(This,pMSG)

#define IAcquisition_GetNext(This,pMSG)	\
    (This)->lpVtbl -> GetNext(This,pMSG)

#define IAcquisition_GetLastError(This,pszError,cbSize)	\
    (This)->lpVtbl -> GetLastError(This,pszError,cbSize)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IAcquisition_Acquire_Proxy( 
    IAcquisition __RPC_FAR * This,
    /* [out][in] */ LPMSGINFO pMSG);


void __RPC_STUB IAcquisition_Acquire_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAcquisition_GetFirst_Proxy( 
    IAcquisition __RPC_FAR * This,
    /* [out][in] */ LPMSGINFO pMSG);


void __RPC_STUB IAcquisition_GetFirst_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAcquisition_GetNext_Proxy( 
    IAcquisition __RPC_FAR * This,
    /* [out][in] */ LPMSGINFO pMSG);


void __RPC_STUB IAcquisition_GetNext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAcquisition_GetLastError_Proxy( 
    IAcquisition __RPC_FAR * This,
    /* [out][in] */ LPWSTR pszError,
    /* [in] */ DWORD cbSize);


void __RPC_STUB IAcquisition_GetLastError_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAcquisition_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL_itf_wis_0011
 * at Tue Sep 22 18:40:44 1998
 * using MIDL 3.02.88
 ****************************************/
/* [local] */ 


#define XMIT_E_CONNECTION_LOST   MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x200) 
#define XMIT_E_CANT_CONNECT      MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x201) 
#define XMIT_E_NOT_CONNECTED MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x202)
#define XMIT_E_SEND_ERROR        MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x203)
#define XMIT_E_BAD_INPUT_FILE    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x204)


extern RPC_IF_HANDLE __MIDL_itf_wis_0011_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_wis_0011_v0_0_s_ifspec;

#ifndef __ITransmitter_INTERFACE_DEFINED__
#define __ITransmitter_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ITransmitter
 * at Tue Sep 22 18:40:44 1998
 * using MIDL 3.02.88
 ****************************************/
/* [object][unique][helpstring][uuid][local] */ 



EXTERN_C const IID IID_ITransmitter;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("2288A560-1E3D-11d1-AACF-00A0C90A8F90")
    ITransmitter : public IWISModuleInformation
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE MakeConnection( 
            /* [out][in] */ LPMSGINFO pMSG) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SendFile( 
            /* [out][in] */ LPMSGINFO pMSG) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CloseConnection( 
            /* [out][in] */ LPMSGINFO pMSG) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetLastError( 
            /* [out][in] */ LPWSTR pszError,
            /* [in] */ DWORD cbSize) = 0;
        
        virtual BOOL STDMETHODCALLTYPE IsConnected( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITransmitterVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITransmitter __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITransmitter __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITransmitter __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetInfo )( 
            ITransmitter __RPC_FAR * This,
            LPMODULE_INFO pInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *MakeConnection )( 
            ITransmitter __RPC_FAR * This,
            /* [out][in] */ LPMSGINFO pMSG);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SendFile )( 
            ITransmitter __RPC_FAR * This,
            /* [out][in] */ LPMSGINFO pMSG);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CloseConnection )( 
            ITransmitter __RPC_FAR * This,
            /* [out][in] */ LPMSGINFO pMSG);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetLastError )( 
            ITransmitter __RPC_FAR * This,
            /* [out][in] */ LPWSTR pszError,
            /* [in] */ DWORD cbSize);
        
        BOOL ( STDMETHODCALLTYPE __RPC_FAR *IsConnected )( 
            ITransmitter __RPC_FAR * This);
        
        END_INTERFACE
    } ITransmitterVtbl;

    interface ITransmitter
    {
        CONST_VTBL struct ITransmitterVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITransmitter_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITransmitter_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITransmitter_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITransmitter_GetInfo(This,pInfo)	\
    (This)->lpVtbl -> GetInfo(This,pInfo)


#define ITransmitter_MakeConnection(This,pMSG)	\
    (This)->lpVtbl -> MakeConnection(This,pMSG)

#define ITransmitter_SendFile(This,pMSG)	\
    (This)->lpVtbl -> SendFile(This,pMSG)

#define ITransmitter_CloseConnection(This,pMSG)	\
    (This)->lpVtbl -> CloseConnection(This,pMSG)

#define ITransmitter_GetLastError(This,pszError,cbSize)	\
    (This)->lpVtbl -> GetLastError(This,pszError,cbSize)

#define ITransmitter_IsConnected(This)	\
    (This)->lpVtbl -> IsConnected(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITransmitter_MakeConnection_Proxy( 
    ITransmitter __RPC_FAR * This,
    /* [out][in] */ LPMSGINFO pMSG);


void __RPC_STUB ITransmitter_MakeConnection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITransmitter_SendFile_Proxy( 
    ITransmitter __RPC_FAR * This,
    /* [out][in] */ LPMSGINFO pMSG);


void __RPC_STUB ITransmitter_SendFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITransmitter_CloseConnection_Proxy( 
    ITransmitter __RPC_FAR * This,
    /* [out][in] */ LPMSGINFO pMSG);


void __RPC_STUB ITransmitter_CloseConnection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITransmitter_GetLastError_Proxy( 
    ITransmitter __RPC_FAR * This,
    /* [out][in] */ LPWSTR pszError,
    /* [in] */ DWORD cbSize);


void __RPC_STUB ITransmitter_GetLastError_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


BOOL STDMETHODCALLTYPE ITransmitter_IsConnected_Proxy( 
    ITransmitter __RPC_FAR * This);


void __RPC_STUB ITransmitter_IsConnected_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITransmitter_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL_itf_wis_0012
 * at Tue Sep 22 18:40:44 1998
 * using MIDL 3.02.88
 ****************************************/
/* [local] */ 


typedef 
enum _WIS_FILTER_TABLE
    {	WIS_FT_ACCEPT_ALL	= 0,
	WIS_FT_GENERAL_SHORT_STRING	= 1,
	WIS_FT_ACCEPT_GROUP_ONLY	= 0xfe,
	WIS_FT_ACCEPT_NONE	= 0xff
    }	WIS_FILTER_TABLE;



extern RPC_IF_HANDLE __MIDL_itf_wis_0012_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_wis_0012_v0_0_s_ifspec;

#ifndef __IWISAppFilter_INTERFACE_DEFINED__
#define __IWISAppFilter_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IWISAppFilter
 * at Tue Sep 22 18:40:44 1998
 * using MIDL 3.02.88
 ****************************************/
/* [object][unique][helpstring][uuid][local] */ 



EXTERN_C const IID IID_IWISAppFilter;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("A38339F0-8CFF-11d1-AB4B-00A0C90A8F90")
    IWISAppFilter : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE IsGroupTagRegistered( 
            /* [in] */ LPWSTR szGroupTag,
            /* [out] */ BOOL __RPC_FAR *fResult,
            /* [out][in] */ LPDWORD pdwDevice) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateFilterTable( 
            /* [out][in] */ HANDLE __RPC_FAR *phFTable,
            /* [in] */ DWORD dwNumEntries,
            /* [in] */ WIS_FILTER_TABLE wftFilterType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetHashValue( 
            /* [in] */ HANDLE hFTable,
            /* [in] */ LPWSTR szValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetPreferences( 
            /* [in] */ HANDLE hFTable,
            /* [in] */ DWORD dwPreferances) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RegisterFilterTable( 
            /* [in] */ HANDLE hFTable,
            /* [in] */ LPWSTR szGroupTag,
            /* [in] */ DWORD dwDevice) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DestroyFilterTable( 
            /* [in] */ HANDLE hFTable) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWISAppFilterVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWISAppFilter __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWISAppFilter __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWISAppFilter __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsGroupTagRegistered )( 
            IWISAppFilter __RPC_FAR * This,
            /* [in] */ LPWSTR szGroupTag,
            /* [out] */ BOOL __RPC_FAR *fResult,
            /* [out][in] */ LPDWORD pdwDevice);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateFilterTable )( 
            IWISAppFilter __RPC_FAR * This,
            /* [out][in] */ HANDLE __RPC_FAR *phFTable,
            /* [in] */ DWORD dwNumEntries,
            /* [in] */ WIS_FILTER_TABLE wftFilterType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetHashValue )( 
            IWISAppFilter __RPC_FAR * This,
            /* [in] */ HANDLE hFTable,
            /* [in] */ LPWSTR szValue);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetPreferences )( 
            IWISAppFilter __RPC_FAR * This,
            /* [in] */ HANDLE hFTable,
            /* [in] */ DWORD dwPreferances);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RegisterFilterTable )( 
            IWISAppFilter __RPC_FAR * This,
            /* [in] */ HANDLE hFTable,
            /* [in] */ LPWSTR szGroupTag,
            /* [in] */ DWORD dwDevice);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DestroyFilterTable )( 
            IWISAppFilter __RPC_FAR * This,
            /* [in] */ HANDLE hFTable);
        
        END_INTERFACE
    } IWISAppFilterVtbl;

    interface IWISAppFilter
    {
        CONST_VTBL struct IWISAppFilterVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWISAppFilter_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWISAppFilter_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWISAppFilter_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWISAppFilter_IsGroupTagRegistered(This,szGroupTag,fResult,pdwDevice)	\
    (This)->lpVtbl -> IsGroupTagRegistered(This,szGroupTag,fResult,pdwDevice)

#define IWISAppFilter_CreateFilterTable(This,phFTable,dwNumEntries,wftFilterType)	\
    (This)->lpVtbl -> CreateFilterTable(This,phFTable,dwNumEntries,wftFilterType)

#define IWISAppFilter_SetHashValue(This,hFTable,szValue)	\
    (This)->lpVtbl -> SetHashValue(This,hFTable,szValue)

#define IWISAppFilter_SetPreferences(This,hFTable,dwPreferances)	\
    (This)->lpVtbl -> SetPreferences(This,hFTable,dwPreferances)

#define IWISAppFilter_RegisterFilterTable(This,hFTable,szGroupTag,dwDevice)	\
    (This)->lpVtbl -> RegisterFilterTable(This,hFTable,szGroupTag,dwDevice)

#define IWISAppFilter_DestroyFilterTable(This,hFTable)	\
    (This)->lpVtbl -> DestroyFilterTable(This,hFTable)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWISAppFilter_IsGroupTagRegistered_Proxy( 
    IWISAppFilter __RPC_FAR * This,
    /* [in] */ LPWSTR szGroupTag,
    /* [out] */ BOOL __RPC_FAR *fResult,
    /* [out][in] */ LPDWORD pdwDevice);


void __RPC_STUB IWISAppFilter_IsGroupTagRegistered_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWISAppFilter_CreateFilterTable_Proxy( 
    IWISAppFilter __RPC_FAR * This,
    /* [out][in] */ HANDLE __RPC_FAR *phFTable,
    /* [in] */ DWORD dwNumEntries,
    /* [in] */ WIS_FILTER_TABLE wftFilterType);


void __RPC_STUB IWISAppFilter_CreateFilterTable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWISAppFilter_SetHashValue_Proxy( 
    IWISAppFilter __RPC_FAR * This,
    /* [in] */ HANDLE hFTable,
    /* [in] */ LPWSTR szValue);


void __RPC_STUB IWISAppFilter_SetHashValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWISAppFilter_SetPreferences_Proxy( 
    IWISAppFilter __RPC_FAR * This,
    /* [in] */ HANDLE hFTable,
    /* [in] */ DWORD dwPreferances);


void __RPC_STUB IWISAppFilter_SetPreferences_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWISAppFilter_RegisterFilterTable_Proxy( 
    IWISAppFilter __RPC_FAR * This,
    /* [in] */ HANDLE hFTable,
    /* [in] */ LPWSTR szGroupTag,
    /* [in] */ DWORD dwDevice);


void __RPC_STUB IWISAppFilter_RegisterFilterTable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWISAppFilter_DestroyFilterTable_Proxy( 
    IWISAppFilter __RPC_FAR * This,
    /* [in] */ HANDLE hFTable);


void __RPC_STUB IWISAppFilter_DestroyFilterTable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWISAppFilter_INTERFACE_DEFINED__ */


#ifndef __IPropPageUI_INTERFACE_DEFINED__
#define __IPropPageUI_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IPropPageUI
 * at Tue Sep 22 18:40:44 1998
 * using MIDL 3.02.88
 ****************************************/
/* [object][unique][helpstring][uuid][local] */ 



EXTERN_C const IID IID_IPropPageUI;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("7B2685C1-CF11-11d1-8D4D-006097C51826")
    IPropPageUI : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetFirstPropSheetPage( 
            HANDLE __RPC_FAR *hPropSheetPage) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetNextPropSheetPage( 
            HANDLE __RPC_FAR *hPropSheetPage) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE PutOpenRegKey( 
            HKEY hOpenRegKey) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE PutTextHwnd( 
            HWND hwndText) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPropPageUIVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IPropPageUI __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IPropPageUI __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IPropPageUI __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetFirstPropSheetPage )( 
            IPropPageUI __RPC_FAR * This,
            HANDLE __RPC_FAR *hPropSheetPage);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNextPropSheetPage )( 
            IPropPageUI __RPC_FAR * This,
            HANDLE __RPC_FAR *hPropSheetPage);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *PutOpenRegKey )( 
            IPropPageUI __RPC_FAR * This,
            HKEY hOpenRegKey);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *PutTextHwnd )( 
            IPropPageUI __RPC_FAR * This,
            HWND hwndText);
        
        END_INTERFACE
    } IPropPageUIVtbl;

    interface IPropPageUI
    {
        CONST_VTBL struct IPropPageUIVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPropPageUI_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPropPageUI_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPropPageUI_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPropPageUI_GetFirstPropSheetPage(This,hPropSheetPage)	\
    (This)->lpVtbl -> GetFirstPropSheetPage(This,hPropSheetPage)

#define IPropPageUI_GetNextPropSheetPage(This,hPropSheetPage)	\
    (This)->lpVtbl -> GetNextPropSheetPage(This,hPropSheetPage)

#define IPropPageUI_PutOpenRegKey(This,hOpenRegKey)	\
    (This)->lpVtbl -> PutOpenRegKey(This,hOpenRegKey)

#define IPropPageUI_PutTextHwnd(This,hwndText)	\
    (This)->lpVtbl -> PutTextHwnd(This,hwndText)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IPropPageUI_GetFirstPropSheetPage_Proxy( 
    IPropPageUI __RPC_FAR * This,
    HANDLE __RPC_FAR *hPropSheetPage);


void __RPC_STUB IPropPageUI_GetFirstPropSheetPage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IPropPageUI_GetNextPropSheetPage_Proxy( 
    IPropPageUI __RPC_FAR * This,
    HANDLE __RPC_FAR *hPropSheetPage);


void __RPC_STUB IPropPageUI_GetNextPropSheetPage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IPropPageUI_PutOpenRegKey_Proxy( 
    IPropPageUI __RPC_FAR * This,
    HKEY hOpenRegKey);


void __RPC_STUB IPropPageUI_PutOpenRegKey_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IPropPageUI_PutTextHwnd_Proxy( 
    IPropPageUI __RPC_FAR * This,
    HWND hwndText);


void __RPC_STUB IPropPageUI_PutTextHwnd_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPropPageUI_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
