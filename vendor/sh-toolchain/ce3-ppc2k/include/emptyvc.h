/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.02.88 */
/* at Tue Dec 14 22:39:12 1999
 */
/* Compiler settings for .\emptyvc.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: none
*/
//@@MIDL_FILE_HEADING(  )
#include "rpc.h"
#include "rpcndr.h"
#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __emptyvc_h__
#define __emptyvc_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IEmptyVolumeCacheCallBack_FWD_DEFINED__
#define __IEmptyVolumeCacheCallBack_FWD_DEFINED__
typedef interface IEmptyVolumeCacheCallBack IEmptyVolumeCacheCallBack;
#endif 	/* __IEmptyVolumeCacheCallBack_FWD_DEFINED__ */


#ifndef __IEmptyVolumeCache_FWD_DEFINED__
#define __IEmptyVolumeCache_FWD_DEFINED__
typedef interface IEmptyVolumeCache IEmptyVolumeCache;
#endif 	/* __IEmptyVolumeCache_FWD_DEFINED__ */


/* header files for imported files */
#include "objidl.h"
#include "oleidl.h"
#include "oaidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/****************************************
 * Generated header for interface: __MIDL_itf_emptyvc_0000
 * at Tue Dec 14 22:39:12 1999
 * using MIDL 3.02.88
 ****************************************/
/* [local] */ 


//=--------------------------------------------------------------------------=
// emptyvc.h
//=--------------------------------------------------------------------------=
// (C) Copyright 1996-1997 Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=

#pragma comment(lib,"uuid.lib")

//---------------------------------------------------------------------------=
// Empty Volume Cache Interfaces.


// IEmptyVolumeCache Flags
#define EVCF_HASSETTINGS             0x0001
#define EVCF_ENABLEBYDEFAULT         0x0002
#define EVCF_REMOVEFROMLIST          0x0004
#define EVCF_ENABLEBYDEFAULT_AUTO    0x0008
#define EVCF_DONTSHOWIFZERO          0x0010
#define EVCF_SETTINGSMODE            0x0020
#define EVCF_OUTOFDISKSPACE          0x0040

// IEmptyVolumeCacheCallBack Flags
#define EVCCBF_LASTNOTIFICATION  0x0001

////////////////////////////////////////////////////////////////////////////
//  Interface Definitions
#ifndef _LPEMPTYVOLUMECACHECALLBACK_DEFINED
#define _LPEMPTYVOLUMECACHECALLBACK_DEFINED


extern RPC_IF_HANDLE __MIDL_itf_emptyvc_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_emptyvc_0000_v0_0_s_ifspec;

#ifndef __IEmptyVolumeCacheCallBack_INTERFACE_DEFINED__
#define __IEmptyVolumeCacheCallBack_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IEmptyVolumeCacheCallBack
 * at Tue Dec 14 22:39:12 1999
 * using MIDL 3.02.88
 ****************************************/
/* [unique][uuid][object] */ 


typedef /* [unique] */ IEmptyVolumeCacheCallBack __RPC_FAR *LPEMPTYVOLUMECACHECALLBACK;


EXTERN_C const IID IID_IEmptyVolumeCacheCallBack;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("6E793361-73C6-11D0-8469-00AA00442901")
    IEmptyVolumeCacheCallBack : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ScanProgress( 
            /* [in] */ DWORDLONG dwlSpaceUsed,
            /* [in] */ DWORD dwFlags,
            /* [in] */ LPCWSTR pcwszStatus) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE PurgeProgress( 
            /* [in] */ DWORDLONG dwlSpaceFreed,
            /* [in] */ DWORDLONG dwlSpaceToFree,
            /* [in] */ DWORD dwFlags,
            /* [in] */ LPCWSTR pcwszStatus) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEmptyVolumeCacheCallBackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IEmptyVolumeCacheCallBack __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IEmptyVolumeCacheCallBack __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IEmptyVolumeCacheCallBack __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ScanProgress )( 
            IEmptyVolumeCacheCallBack __RPC_FAR * This,
            /* [in] */ DWORDLONG dwlSpaceUsed,
            /* [in] */ DWORD dwFlags,
            /* [in] */ LPCWSTR pcwszStatus);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *PurgeProgress )( 
            IEmptyVolumeCacheCallBack __RPC_FAR * This,
            /* [in] */ DWORDLONG dwlSpaceFreed,
            /* [in] */ DWORDLONG dwlSpaceToFree,
            /* [in] */ DWORD dwFlags,
            /* [in] */ LPCWSTR pcwszStatus);
        
        END_INTERFACE
    } IEmptyVolumeCacheCallBackVtbl;

    interface IEmptyVolumeCacheCallBack
    {
        CONST_VTBL struct IEmptyVolumeCacheCallBackVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEmptyVolumeCacheCallBack_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEmptyVolumeCacheCallBack_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEmptyVolumeCacheCallBack_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEmptyVolumeCacheCallBack_ScanProgress(This,dwlSpaceUsed,dwFlags,pcwszStatus)	\
    (This)->lpVtbl -> ScanProgress(This,dwlSpaceUsed,dwFlags,pcwszStatus)

#define IEmptyVolumeCacheCallBack_PurgeProgress(This,dwlSpaceFreed,dwlSpaceToFree,dwFlags,pcwszStatus)	\
    (This)->lpVtbl -> PurgeProgress(This,dwlSpaceFreed,dwlSpaceToFree,dwFlags,pcwszStatus)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEmptyVolumeCacheCallBack_ScanProgress_Proxy( 
    IEmptyVolumeCacheCallBack __RPC_FAR * This,
    /* [in] */ DWORDLONG dwlSpaceUsed,
    /* [in] */ DWORD dwFlags,
    /* [in] */ LPCWSTR pcwszStatus);


void __RPC_STUB IEmptyVolumeCacheCallBack_ScanProgress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEmptyVolumeCacheCallBack_PurgeProgress_Proxy( 
    IEmptyVolumeCacheCallBack __RPC_FAR * This,
    /* [in] */ DWORDLONG dwlSpaceFreed,
    /* [in] */ DWORDLONG dwlSpaceToFree,
    /* [in] */ DWORD dwFlags,
    /* [in] */ LPCWSTR pcwszStatus);


void __RPC_STUB IEmptyVolumeCacheCallBack_PurgeProgress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEmptyVolumeCacheCallBack_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL_itf_emptyvc_0100
 * at Tue Dec 14 22:39:12 1999
 * using MIDL 3.02.88
 ****************************************/
/* [local] */ 


#endif
#ifndef _LPEMPTYVOLUMECACHE_DEFINED
#define _LPEMPTYVOLUMECACHE_DEFINED


extern RPC_IF_HANDLE __MIDL_itf_emptyvc_0100_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_emptyvc_0100_v0_0_s_ifspec;

#ifndef __IEmptyVolumeCache_INTERFACE_DEFINED__
#define __IEmptyVolumeCache_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IEmptyVolumeCache
 * at Tue Dec 14 22:39:12 1999
 * using MIDL 3.02.88
 ****************************************/
/* [unique][uuid][object] */ 


typedef /* [unique] */ IEmptyVolumeCache __RPC_FAR *LPEMPTYVOLUMECACHE;


EXTERN_C const IID IID_IEmptyVolumeCache;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("8FCE5227-04DA-11d1-A004-00805F8ABE06")
    IEmptyVolumeCache : public IUnknown
    {
    public:
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE Initialize( 
            /* [in] */ HKEY hkRegKey,
            /* [in] */ LPCWSTR pcwszVolume,
            /* [out] */ LPWSTR __RPC_FAR *ppwszDisplayName,
            /* [out] */ LPWSTR __RPC_FAR *ppwszDescription,
            /* [out] */ DWORD __RPC_FAR *pdwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSpaceUsed( 
            /* [out] */ DWORDLONG __RPC_FAR *pdwlSpaceUsed,
            /* [in] */ IEmptyVolumeCacheCallBack __RPC_FAR *picb) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Purge( 
            /* [in] */ DWORDLONG dwlSpaceToFree,
            /* [in] */ IEmptyVolumeCacheCallBack __RPC_FAR *picb) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ShowProperties( 
            /* [in] */ HWND hwnd) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Deactivate( 
            /* [out] */ DWORD __RPC_FAR *pdwFlags) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEmptyVolumeCacheVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IEmptyVolumeCache __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IEmptyVolumeCache __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IEmptyVolumeCache __RPC_FAR * This);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Initialize )( 
            IEmptyVolumeCache __RPC_FAR * This,
            /* [in] */ HKEY hkRegKey,
            /* [in] */ LPCWSTR pcwszVolume,
            /* [out] */ LPWSTR __RPC_FAR *ppwszDisplayName,
            /* [out] */ LPWSTR __RPC_FAR *ppwszDescription,
            /* [out] */ DWORD __RPC_FAR *pdwFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSpaceUsed )( 
            IEmptyVolumeCache __RPC_FAR * This,
            /* [out] */ DWORDLONG __RPC_FAR *pdwlSpaceUsed,
            /* [in] */ IEmptyVolumeCacheCallBack __RPC_FAR *picb);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Purge )( 
            IEmptyVolumeCache __RPC_FAR * This,
            /* [in] */ DWORDLONG dwlSpaceToFree,
            /* [in] */ IEmptyVolumeCacheCallBack __RPC_FAR *picb);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ShowProperties )( 
            IEmptyVolumeCache __RPC_FAR * This,
            /* [in] */ HWND hwnd);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Deactivate )( 
            IEmptyVolumeCache __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwFlags);
        
        END_INTERFACE
    } IEmptyVolumeCacheVtbl;

    interface IEmptyVolumeCache
    {
        CONST_VTBL struct IEmptyVolumeCacheVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEmptyVolumeCache_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEmptyVolumeCache_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEmptyVolumeCache_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEmptyVolumeCache_Initialize(This,hkRegKey,pcwszVolume,ppwszDisplayName,ppwszDescription,pdwFlags)	\
    (This)->lpVtbl -> Initialize(This,hkRegKey,pcwszVolume,ppwszDisplayName,ppwszDescription,pdwFlags)

#define IEmptyVolumeCache_GetSpaceUsed(This,pdwlSpaceUsed,picb)	\
    (This)->lpVtbl -> GetSpaceUsed(This,pdwlSpaceUsed,picb)

#define IEmptyVolumeCache_Purge(This,dwlSpaceToFree,picb)	\
    (This)->lpVtbl -> Purge(This,dwlSpaceToFree,picb)

#define IEmptyVolumeCache_ShowProperties(This,hwnd)	\
    (This)->lpVtbl -> ShowProperties(This,hwnd)

#define IEmptyVolumeCache_Deactivate(This,pdwFlags)	\
    (This)->lpVtbl -> Deactivate(This,pdwFlags)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [local] */ HRESULT STDMETHODCALLTYPE IEmptyVolumeCache_Initialize_Proxy( 
    IEmptyVolumeCache __RPC_FAR * This,
    /* [in] */ HKEY hkRegKey,
    /* [in] */ LPCWSTR pcwszVolume,
    /* [out] */ LPWSTR __RPC_FAR *ppwszDisplayName,
    /* [out] */ LPWSTR __RPC_FAR *ppwszDescription,
    /* [out] */ DWORD __RPC_FAR *pdwFlags);


void __RPC_STUB IEmptyVolumeCache_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEmptyVolumeCache_GetSpaceUsed_Proxy( 
    IEmptyVolumeCache __RPC_FAR * This,
    /* [out] */ DWORDLONG __RPC_FAR *pdwlSpaceUsed,
    /* [in] */ IEmptyVolumeCacheCallBack __RPC_FAR *picb);


void __RPC_STUB IEmptyVolumeCache_GetSpaceUsed_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEmptyVolumeCache_Purge_Proxy( 
    IEmptyVolumeCache __RPC_FAR * This,
    /* [in] */ DWORDLONG dwlSpaceToFree,
    /* [in] */ IEmptyVolumeCacheCallBack __RPC_FAR *picb);


void __RPC_STUB IEmptyVolumeCache_Purge_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEmptyVolumeCache_ShowProperties_Proxy( 
    IEmptyVolumeCache __RPC_FAR * This,
    /* [in] */ HWND hwnd);


void __RPC_STUB IEmptyVolumeCache_ShowProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEmptyVolumeCache_Deactivate_Proxy( 
    IEmptyVolumeCache __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwFlags);


void __RPC_STUB IEmptyVolumeCache_Deactivate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEmptyVolumeCache_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL_itf_emptyvc_0101
 * at Tue Dec 14 22:39:12 1999
 * using MIDL 3.02.88
 ****************************************/
/* [local] */ 


#endif


extern RPC_IF_HANDLE __MIDL_itf_emptyvc_0101_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_emptyvc_0101_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  HWND_UserSize(     unsigned long __RPC_FAR *, unsigned long            , HWND __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  HWND_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, HWND __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  HWND_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, HWND __RPC_FAR * ); 
void                      __RPC_USER  HWND_UserFree(     unsigned long __RPC_FAR *, HWND __RPC_FAR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
