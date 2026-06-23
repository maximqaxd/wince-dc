/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.02.88 */
/* at Tue Dec 14 22:38:55 1999
 */
/* Compiler settings for .\htiface.idl:
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

#ifndef __htiface_h__
#define __htiface_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __ITargetFrame_FWD_DEFINED__
#define __ITargetFrame_FWD_DEFINED__
typedef interface ITargetFrame ITargetFrame;
#endif 	/* __ITargetFrame_FWD_DEFINED__ */


#ifndef __ITargetEmbedding_FWD_DEFINED__
#define __ITargetEmbedding_FWD_DEFINED__
typedef interface ITargetEmbedding ITargetEmbedding;
#endif 	/* __ITargetEmbedding_FWD_DEFINED__ */


#ifndef __ITargetFramePriv_FWD_DEFINED__
#define __ITargetFramePriv_FWD_DEFINED__
typedef interface ITargetFramePriv ITargetFramePriv;
#endif 	/* __ITargetFramePriv_FWD_DEFINED__ */


/* header files for imported files */
#include "objidl.h"
#include "oleidl.h"
#include "urlmon.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/****************************************
 * Generated header for interface: __MIDL_itf_htiface_0000
 * at Tue Dec 14 22:38:55 1999
 * using MIDL 3.02.88
 ****************************************/
/* [local] */ 


//=--------------------------------------------------------------------------=
// HTIface.h
//=--------------------------------------------------------------------------=
// (C) Copyright 1995-1997 Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=

#pragma comment(lib,"uuid.lib")

//--------------------------------------------------------------------------
// OLE Hyperlinking ITargetFrame Interfaces.

#ifndef _LPTARGETFRAME2_DEFINED														
#include "htiframe.h"														
#endif // _LPTARGETFRAME2_DEFINED														


EXTERN_C const IID IID_ITargetFrame;
EXTERN_C const IID IID_ITargetEmbedding;
EXTERN_C const IID IID_ITargetFramePriv;
#ifndef _LPTARGETFRAME_DEFINED
#define _LPTARGETFRAME_DEFINED


extern RPC_IF_HANDLE __MIDL_itf_htiface_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_htiface_0000_v0_0_s_ifspec;

#ifndef __ITargetFrame_INTERFACE_DEFINED__
#define __ITargetFrame_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ITargetFrame
 * at Tue Dec 14 22:38:55 1999
 * using MIDL 3.02.88
 ****************************************/
/* [unique][uuid][object] */ 


typedef /* [unique] */ ITargetFrame __RPC_FAR *LPTARGETFRAME;

typedef /* [public] */ 
enum __MIDL_ITargetFrame_0001
    {	NAVIGATEFRAME_FL_RECORD	= 0x1,
	NAVIGATEFRAME_FL_POST	= 0x2,
	NAVIGATEFRAME_FL_NO_DOC_CACHE	= 0x4,
	NAVIGATEFRAME_FL_NO_IMAGE_CACHE	= 0x8,
	NAVIGATEFRAME_FL_AUTH_FAIL_CACHE_OK	= 0x10,
	NAVIGATEFRAME_FL_SENDING_FROM_FORM	= 0x20,
	NAVIGATEFRAME_FL_REALLY_SENDING_FROM_FORM	= 0x40
    }	NAVIGATEFRAME_FLAGS;

typedef struct  tagNavigateData
    {
    ULONG ulTarget;
    ULONG ulURL;
    ULONG ulRefURL;
    ULONG ulPostData;
    DWORD dwFlags;
    }	NAVIGATEDATA;


EXTERN_C const IID IID_ITargetFrame;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("d5f78c80-5252-11cf-90fa-00AA0042106e")
    ITargetFrame : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetFrameName( 
            /* [in] */ LPCWSTR pszFrameName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFrameName( 
            /* [out] */ LPWSTR __RPC_FAR *ppszFrameName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetParentFrame( 
            /* [out] */ IUnknown __RPC_FAR *__RPC_FAR *ppunkParent) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FindFrame( 
            /* [in] */ LPCWSTR pszTargetName,
            /* [in] */ IUnknown __RPC_FAR *ppunkContextFrame,
            /* [in] */ DWORD dwFlags,
            /* [out] */ IUnknown __RPC_FAR *__RPC_FAR *ppunkTargetFrame) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetFrameSrc( 
            /* [in] */ LPCWSTR pszFrameSrc) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFrameSrc( 
            /* [out] */ LPWSTR __RPC_FAR *ppszFrameSrc) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFramesContainer( 
            /* [out] */ IOleContainer __RPC_FAR *__RPC_FAR *ppContainer) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetFrameOptions( 
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFrameOptions( 
            /* [out] */ DWORD __RPC_FAR *pdwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetFrameMargins( 
            /* [in] */ DWORD dwWidth,
            /* [in] */ DWORD dwHeight) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFrameMargins( 
            /* [out] */ DWORD __RPC_FAR *pdwWidth,
            /* [out] */ DWORD __RPC_FAR *pdwHeight) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoteNavigate( 
            /* [in] */ ULONG cLength,
            /* [size_is][in] */ ULONG __RPC_FAR *pulData) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnChildFrameActivate( 
            /* [in] */ IUnknown __RPC_FAR *pUnkChildFrame) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnChildFrameDeactivate( 
            /* [in] */ IUnknown __RPC_FAR *pUnkChildFrame) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITargetFrameVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITargetFrame __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITargetFrame __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITargetFrame __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetFrameName )( 
            ITargetFrame __RPC_FAR * This,
            /* [in] */ LPCWSTR pszFrameName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetFrameName )( 
            ITargetFrame __RPC_FAR * This,
            /* [out] */ LPWSTR __RPC_FAR *ppszFrameName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetParentFrame )( 
            ITargetFrame __RPC_FAR * This,
            /* [out] */ IUnknown __RPC_FAR *__RPC_FAR *ppunkParent);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FindFrame )( 
            ITargetFrame __RPC_FAR * This,
            /* [in] */ LPCWSTR pszTargetName,
            /* [in] */ IUnknown __RPC_FAR *ppunkContextFrame,
            /* [in] */ DWORD dwFlags,
            /* [out] */ IUnknown __RPC_FAR *__RPC_FAR *ppunkTargetFrame);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetFrameSrc )( 
            ITargetFrame __RPC_FAR * This,
            /* [in] */ LPCWSTR pszFrameSrc);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetFrameSrc )( 
            ITargetFrame __RPC_FAR * This,
            /* [out] */ LPWSTR __RPC_FAR *ppszFrameSrc);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetFramesContainer )( 
            ITargetFrame __RPC_FAR * This,
            /* [out] */ IOleContainer __RPC_FAR *__RPC_FAR *ppContainer);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetFrameOptions )( 
            ITargetFrame __RPC_FAR * This,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetFrameOptions )( 
            ITargetFrame __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetFrameMargins )( 
            ITargetFrame __RPC_FAR * This,
            /* [in] */ DWORD dwWidth,
            /* [in] */ DWORD dwHeight);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetFrameMargins )( 
            ITargetFrame __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwWidth,
            /* [out] */ DWORD __RPC_FAR *pdwHeight);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RemoteNavigate )( 
            ITargetFrame __RPC_FAR * This,
            /* [in] */ ULONG cLength,
            /* [size_is][in] */ ULONG __RPC_FAR *pulData);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnChildFrameActivate )( 
            ITargetFrame __RPC_FAR * This,
            /* [in] */ IUnknown __RPC_FAR *pUnkChildFrame);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnChildFrameDeactivate )( 
            ITargetFrame __RPC_FAR * This,
            /* [in] */ IUnknown __RPC_FAR *pUnkChildFrame);
        
        END_INTERFACE
    } ITargetFrameVtbl;

    interface ITargetFrame
    {
        CONST_VTBL struct ITargetFrameVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITargetFrame_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITargetFrame_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITargetFrame_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITargetFrame_SetFrameName(This,pszFrameName)	\
    (This)->lpVtbl -> SetFrameName(This,pszFrameName)

#define ITargetFrame_GetFrameName(This,ppszFrameName)	\
    (This)->lpVtbl -> GetFrameName(This,ppszFrameName)

#define ITargetFrame_GetParentFrame(This,ppunkParent)	\
    (This)->lpVtbl -> GetParentFrame(This,ppunkParent)

#define ITargetFrame_FindFrame(This,pszTargetName,ppunkContextFrame,dwFlags,ppunkTargetFrame)	\
    (This)->lpVtbl -> FindFrame(This,pszTargetName,ppunkContextFrame,dwFlags,ppunkTargetFrame)

#define ITargetFrame_SetFrameSrc(This,pszFrameSrc)	\
    (This)->lpVtbl -> SetFrameSrc(This,pszFrameSrc)

#define ITargetFrame_GetFrameSrc(This,ppszFrameSrc)	\
    (This)->lpVtbl -> GetFrameSrc(This,ppszFrameSrc)

#define ITargetFrame_GetFramesContainer(This,ppContainer)	\
    (This)->lpVtbl -> GetFramesContainer(This,ppContainer)

#define ITargetFrame_SetFrameOptions(This,dwFlags)	\
    (This)->lpVtbl -> SetFrameOptions(This,dwFlags)

#define ITargetFrame_GetFrameOptions(This,pdwFlags)	\
    (This)->lpVtbl -> GetFrameOptions(This,pdwFlags)

#define ITargetFrame_SetFrameMargins(This,dwWidth,dwHeight)	\
    (This)->lpVtbl -> SetFrameMargins(This,dwWidth,dwHeight)

#define ITargetFrame_GetFrameMargins(This,pdwWidth,pdwHeight)	\
    (This)->lpVtbl -> GetFrameMargins(This,pdwWidth,pdwHeight)

#define ITargetFrame_RemoteNavigate(This,cLength,pulData)	\
    (This)->lpVtbl -> RemoteNavigate(This,cLength,pulData)

#define ITargetFrame_OnChildFrameActivate(This,pUnkChildFrame)	\
    (This)->lpVtbl -> OnChildFrameActivate(This,pUnkChildFrame)

#define ITargetFrame_OnChildFrameDeactivate(This,pUnkChildFrame)	\
    (This)->lpVtbl -> OnChildFrameDeactivate(This,pUnkChildFrame)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITargetFrame_SetFrameName_Proxy( 
    ITargetFrame __RPC_FAR * This,
    /* [in] */ LPCWSTR pszFrameName);


void __RPC_STUB ITargetFrame_SetFrameName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITargetFrame_GetFrameName_Proxy( 
    ITargetFrame __RPC_FAR * This,
    /* [out] */ LPWSTR __RPC_FAR *ppszFrameName);


void __RPC_STUB ITargetFrame_GetFrameName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITargetFrame_GetParentFrame_Proxy( 
    ITargetFrame __RPC_FAR * This,
    /* [out] */ IUnknown __RPC_FAR *__RPC_FAR *ppunkParent);


void __RPC_STUB ITargetFrame_GetParentFrame_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITargetFrame_FindFrame_Proxy( 
    ITargetFrame __RPC_FAR * This,
    /* [in] */ LPCWSTR pszTargetName,
    /* [in] */ IUnknown __RPC_FAR *ppunkContextFrame,
    /* [in] */ DWORD dwFlags,
    /* [out] */ IUnknown __RPC_FAR *__RPC_FAR *ppunkTargetFrame);


void __RPC_STUB ITargetFrame_FindFrame_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITargetFrame_SetFrameSrc_Proxy( 
    ITargetFrame __RPC_FAR * This,
    /* [in] */ LPCWSTR pszFrameSrc);


void __RPC_STUB ITargetFrame_SetFrameSrc_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITargetFrame_GetFrameSrc_Proxy( 
    ITargetFrame __RPC_FAR * This,
    /* [out] */ LPWSTR __RPC_FAR *ppszFrameSrc);


void __RPC_STUB ITargetFrame_GetFrameSrc_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITargetFrame_GetFramesContainer_Proxy( 
    ITargetFrame __RPC_FAR * This,
    /* [out] */ IOleContainer __RPC_FAR *__RPC_FAR *ppContainer);


void __RPC_STUB ITargetFrame_GetFramesContainer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITargetFrame_SetFrameOptions_Proxy( 
    ITargetFrame __RPC_FAR * This,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB ITargetFrame_SetFrameOptions_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITargetFrame_GetFrameOptions_Proxy( 
    ITargetFrame __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwFlags);


void __RPC_STUB ITargetFrame_GetFrameOptions_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITargetFrame_SetFrameMargins_Proxy( 
    ITargetFrame __RPC_FAR * This,
    /* [in] */ DWORD dwWidth,
    /* [in] */ DWORD dwHeight);


void __RPC_STUB ITargetFrame_SetFrameMargins_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITargetFrame_GetFrameMargins_Proxy( 
    ITargetFrame __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwWidth,
    /* [out] */ DWORD __RPC_FAR *pdwHeight);


void __RPC_STUB ITargetFrame_GetFrameMargins_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITargetFrame_RemoteNavigate_Proxy( 
    ITargetFrame __RPC_FAR * This,
    /* [in] */ ULONG cLength,
    /* [size_is][in] */ ULONG __RPC_FAR *pulData);


void __RPC_STUB ITargetFrame_RemoteNavigate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITargetFrame_OnChildFrameActivate_Proxy( 
    ITargetFrame __RPC_FAR * This,
    /* [in] */ IUnknown __RPC_FAR *pUnkChildFrame);


void __RPC_STUB ITargetFrame_OnChildFrameActivate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITargetFrame_OnChildFrameDeactivate_Proxy( 
    ITargetFrame __RPC_FAR * This,
    /* [in] */ IUnknown __RPC_FAR *pUnkChildFrame);


void __RPC_STUB ITargetFrame_OnChildFrameDeactivate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITargetFrame_INTERFACE_DEFINED__ */


#ifndef __ITargetEmbedding_INTERFACE_DEFINED__
#define __ITargetEmbedding_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ITargetEmbedding
 * at Tue Dec 14 22:38:55 1999
 * using MIDL 3.02.88
 ****************************************/
/* [unique][uuid][object] */ 


typedef /* [unique] */ ITargetEmbedding __RPC_FAR *LPTARGETEMBEDDING;


EXTERN_C const IID IID_ITargetEmbedding;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("548793C0-9E74-11cf-9655-00A0C9034923")
    ITargetEmbedding : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetTargetFrame( 
            /* [out] */ ITargetFrame __RPC_FAR *__RPC_FAR *ppTargetFrame) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITargetEmbeddingVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITargetEmbedding __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITargetEmbedding __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITargetEmbedding __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTargetFrame )( 
            ITargetEmbedding __RPC_FAR * This,
            /* [out] */ ITargetFrame __RPC_FAR *__RPC_FAR *ppTargetFrame);
        
        END_INTERFACE
    } ITargetEmbeddingVtbl;

    interface ITargetEmbedding
    {
        CONST_VTBL struct ITargetEmbeddingVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITargetEmbedding_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITargetEmbedding_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITargetEmbedding_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITargetEmbedding_GetTargetFrame(This,ppTargetFrame)	\
    (This)->lpVtbl -> GetTargetFrame(This,ppTargetFrame)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITargetEmbedding_GetTargetFrame_Proxy( 
    ITargetEmbedding __RPC_FAR * This,
    /* [out] */ ITargetFrame __RPC_FAR *__RPC_FAR *ppTargetFrame);


void __RPC_STUB ITargetEmbedding_GetTargetFrame_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITargetEmbedding_INTERFACE_DEFINED__ */


#ifndef __ITargetFramePriv_INTERFACE_DEFINED__
#define __ITargetFramePriv_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ITargetFramePriv
 * at Tue Dec 14 22:38:55 1999
 * using MIDL 3.02.88
 ****************************************/
/* [unique][uuid][object] */ 


typedef /* [unique] */ ITargetFramePriv __RPC_FAR *LPTARGETFRAMEPRIV;


EXTERN_C const IID IID_ITargetFramePriv;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("9216E421-2BF5-11d0-82B4-00A0C90C29C5")
    ITargetFramePriv : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE FindFrameDownwards( 
            /* [in] */ LPCWSTR pszTargetName,
            /* [in] */ DWORD dwFlags,
            /* [out] */ IUnknown __RPC_FAR *__RPC_FAR *ppunkTargetFrame) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FindFrameInContext( 
            /* [in] */ LPCWSTR pszTargetName,
            /* [in] */ IUnknown __RPC_FAR *punkContextFrame,
            /* [in] */ DWORD dwFlags,
            /* [out] */ IUnknown __RPC_FAR *__RPC_FAR *ppunkTargetFrame) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnChildFrameActivate( 
            /* [in] */ IUnknown __RPC_FAR *pUnkChildFrame) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnChildFrameDeactivate( 
            /* [in] */ IUnknown __RPC_FAR *pUnkChildFrame) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE NavigateHack( 
            /* [in] */ DWORD grfHLNF,
            /* [unique][in] */ LPBC pbc,
            /* [unique][in] */ IBindStatusCallback __RPC_FAR *pibsc,
            /* [unique][in] */ LPCWSTR pszTargetName,
            /* [in] */ LPCWSTR pszUrl,
            /* [unique][in] */ LPCWSTR pszLocation) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FindBrowserByIndex( 
            /* [in] */ DWORD dwID,
            /* [out] */ IUnknown __RPC_FAR *__RPC_FAR *ppunkBrowser) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITargetFramePrivVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITargetFramePriv __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITargetFramePriv __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITargetFramePriv __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FindFrameDownwards )( 
            ITargetFramePriv __RPC_FAR * This,
            /* [in] */ LPCWSTR pszTargetName,
            /* [in] */ DWORD dwFlags,
            /* [out] */ IUnknown __RPC_FAR *__RPC_FAR *ppunkTargetFrame);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FindFrameInContext )( 
            ITargetFramePriv __RPC_FAR * This,
            /* [in] */ LPCWSTR pszTargetName,
            /* [in] */ IUnknown __RPC_FAR *punkContextFrame,
            /* [in] */ DWORD dwFlags,
            /* [out] */ IUnknown __RPC_FAR *__RPC_FAR *ppunkTargetFrame);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnChildFrameActivate )( 
            ITargetFramePriv __RPC_FAR * This,
            /* [in] */ IUnknown __RPC_FAR *pUnkChildFrame);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnChildFrameDeactivate )( 
            ITargetFramePriv __RPC_FAR * This,
            /* [in] */ IUnknown __RPC_FAR *pUnkChildFrame);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *NavigateHack )( 
            ITargetFramePriv __RPC_FAR * This,
            /* [in] */ DWORD grfHLNF,
            /* [unique][in] */ LPBC pbc,
            /* [unique][in] */ IBindStatusCallback __RPC_FAR *pibsc,
            /* [unique][in] */ LPCWSTR pszTargetName,
            /* [in] */ LPCWSTR pszUrl,
            /* [unique][in] */ LPCWSTR pszLocation);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FindBrowserByIndex )( 
            ITargetFramePriv __RPC_FAR * This,
            /* [in] */ DWORD dwID,
            /* [out] */ IUnknown __RPC_FAR *__RPC_FAR *ppunkBrowser);
        
        END_INTERFACE
    } ITargetFramePrivVtbl;

    interface ITargetFramePriv
    {
        CONST_VTBL struct ITargetFramePrivVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITargetFramePriv_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITargetFramePriv_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITargetFramePriv_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITargetFramePriv_FindFrameDownwards(This,pszTargetName,dwFlags,ppunkTargetFrame)	\
    (This)->lpVtbl -> FindFrameDownwards(This,pszTargetName,dwFlags,ppunkTargetFrame)

#define ITargetFramePriv_FindFrameInContext(This,pszTargetName,punkContextFrame,dwFlags,ppunkTargetFrame)	\
    (This)->lpVtbl -> FindFrameInContext(This,pszTargetName,punkContextFrame,dwFlags,ppunkTargetFrame)

#define ITargetFramePriv_OnChildFrameActivate(This,pUnkChildFrame)	\
    (This)->lpVtbl -> OnChildFrameActivate(This,pUnkChildFrame)

#define ITargetFramePriv_OnChildFrameDeactivate(This,pUnkChildFrame)	\
    (This)->lpVtbl -> OnChildFrameDeactivate(This,pUnkChildFrame)

#define ITargetFramePriv_NavigateHack(This,grfHLNF,pbc,pibsc,pszTargetName,pszUrl,pszLocation)	\
    (This)->lpVtbl -> NavigateHack(This,grfHLNF,pbc,pibsc,pszTargetName,pszUrl,pszLocation)

#define ITargetFramePriv_FindBrowserByIndex(This,dwID,ppunkBrowser)	\
    (This)->lpVtbl -> FindBrowserByIndex(This,dwID,ppunkBrowser)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITargetFramePriv_FindFrameDownwards_Proxy( 
    ITargetFramePriv __RPC_FAR * This,
    /* [in] */ LPCWSTR pszTargetName,
    /* [in] */ DWORD dwFlags,
    /* [out] */ IUnknown __RPC_FAR *__RPC_FAR *ppunkTargetFrame);


void __RPC_STUB ITargetFramePriv_FindFrameDownwards_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITargetFramePriv_FindFrameInContext_Proxy( 
    ITargetFramePriv __RPC_FAR * This,
    /* [in] */ LPCWSTR pszTargetName,
    /* [in] */ IUnknown __RPC_FAR *punkContextFrame,
    /* [in] */ DWORD dwFlags,
    /* [out] */ IUnknown __RPC_FAR *__RPC_FAR *ppunkTargetFrame);


void __RPC_STUB ITargetFramePriv_FindFrameInContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITargetFramePriv_OnChildFrameActivate_Proxy( 
    ITargetFramePriv __RPC_FAR * This,
    /* [in] */ IUnknown __RPC_FAR *pUnkChildFrame);


void __RPC_STUB ITargetFramePriv_OnChildFrameActivate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITargetFramePriv_OnChildFrameDeactivate_Proxy( 
    ITargetFramePriv __RPC_FAR * This,
    /* [in] */ IUnknown __RPC_FAR *pUnkChildFrame);


void __RPC_STUB ITargetFramePriv_OnChildFrameDeactivate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITargetFramePriv_NavigateHack_Proxy( 
    ITargetFramePriv __RPC_FAR * This,
    /* [in] */ DWORD grfHLNF,
    /* [unique][in] */ LPBC pbc,
    /* [unique][in] */ IBindStatusCallback __RPC_FAR *pibsc,
    /* [unique][in] */ LPCWSTR pszTargetName,
    /* [in] */ LPCWSTR pszUrl,
    /* [unique][in] */ LPCWSTR pszLocation);


void __RPC_STUB ITargetFramePriv_NavigateHack_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITargetFramePriv_FindBrowserByIndex_Proxy( 
    ITargetFramePriv __RPC_FAR * This,
    /* [in] */ DWORD dwID,
    /* [out] */ IUnknown __RPC_FAR *__RPC_FAR *ppunkBrowser);


void __RPC_STUB ITargetFramePriv_FindBrowserByIndex_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITargetFramePriv_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL_itf_htiface_0214
 * at Tue Dec 14 22:38:55 1999
 * using MIDL 3.02.88
 ****************************************/
/* [local] */ 


#endif


extern RPC_IF_HANDLE __MIDL_itf_htiface_0214_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_htiface_0214_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
