/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.02.88 */
/* at Tue Dec 14 22:39:03 1999
 */
/* Compiler settings for .\prgsnk.idl:
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

#ifndef __prgsnk_h__
#define __prgsnk_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IProgSink_FWD_DEFINED__
#define __IProgSink_FWD_DEFINED__
typedef interface IProgSink IProgSink;
#endif 	/* __IProgSink_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "oleidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __IProgSink_INTERFACE_DEFINED__
#define __IProgSink_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IProgSink
 * at Tue Dec 14 22:39:03 1999
 * using MIDL 3.02.88
 ****************************************/
/* [local][unique][uuid][object] */ 



EXTERN_C const IID IID_IProgSink;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("3050f371-98b5-11cf-bb82-00aa00bdce0b")
    IProgSink : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AddProgress( 
            /* [in] */ DWORD dwClass,
            /* [out] */ DWORD __RPC_FAR *pdwCookie) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetProgress( 
            /* [in] */ DWORD dwCookie,
            /* [in] */ DWORD dwFlags,
            /* [in] */ DWORD dwState,
            /* [in] */ LPCTSTR pchText,
            /* [in] */ DWORD dwIds,
            /* [in] */ DWORD dwPos,
            /* [in] */ DWORD dwMax) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DelProgress( 
            /* [in] */ DWORD dwCookie) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IProgSinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IProgSink __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IProgSink __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IProgSink __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddProgress )( 
            IProgSink __RPC_FAR * This,
            /* [in] */ DWORD dwClass,
            /* [out] */ DWORD __RPC_FAR *pdwCookie);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetProgress )( 
            IProgSink __RPC_FAR * This,
            /* [in] */ DWORD dwCookie,
            /* [in] */ DWORD dwFlags,
            /* [in] */ DWORD dwState,
            /* [in] */ LPCTSTR pchText,
            /* [in] */ DWORD dwIds,
            /* [in] */ DWORD dwPos,
            /* [in] */ DWORD dwMax);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DelProgress )( 
            IProgSink __RPC_FAR * This,
            /* [in] */ DWORD dwCookie);
        
        END_INTERFACE
    } IProgSinkVtbl;

    interface IProgSink
    {
        CONST_VTBL struct IProgSinkVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IProgSink_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IProgSink_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IProgSink_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IProgSink_AddProgress(This,dwClass,pdwCookie)	\
    (This)->lpVtbl -> AddProgress(This,dwClass,pdwCookie)

#define IProgSink_SetProgress(This,dwCookie,dwFlags,dwState,pchText,dwIds,dwPos,dwMax)	\
    (This)->lpVtbl -> SetProgress(This,dwCookie,dwFlags,dwState,pchText,dwIds,dwPos,dwMax)

#define IProgSink_DelProgress(This,dwCookie)	\
    (This)->lpVtbl -> DelProgress(This,dwCookie)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IProgSink_AddProgress_Proxy( 
    IProgSink __RPC_FAR * This,
    /* [in] */ DWORD dwClass,
    /* [out] */ DWORD __RPC_FAR *pdwCookie);


void __RPC_STUB IProgSink_AddProgress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IProgSink_SetProgress_Proxy( 
    IProgSink __RPC_FAR * This,
    /* [in] */ DWORD dwCookie,
    /* [in] */ DWORD dwFlags,
    /* [in] */ DWORD dwState,
    /* [in] */ LPCTSTR pchText,
    /* [in] */ DWORD dwIds,
    /* [in] */ DWORD dwPos,
    /* [in] */ DWORD dwMax);


void __RPC_STUB IProgSink_SetProgress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IProgSink_DelProgress_Proxy( 
    IProgSink __RPC_FAR * This,
    /* [in] */ DWORD dwCookie);


void __RPC_STUB IProgSink_DelProgress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IProgSink_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL_itf_prgsnk_0099
 * at Tue Dec 14 22:39:03 1999
 * using MIDL 3.02.88
 ****************************************/
/* [local] */ 


#define PROGSINK_CLASS_FORWARDED    0x80000000
#define PROGSINK_CLASS_NOSPIN       0x40000000
#define PROGSINK_CLASS_HTML         0x00000000
#define PROGSINK_CLASS_MULTIMEDIA   0x00000001
#define PROGSINK_CLASS_CONTROL      0x00000002
#define PROGSINK_CLASS_DATABIND     0x00000003
#define PROGSINK_CLASS_OTHER        0x00000004
#define PROGSINK_CLASS_NOREMAIN     0x00000005
#define PROGSINK_CLASS_FRAME        0x00000006

#define PROGSINK_STATE_IDLE         0x00000000
#define PROGSINK_STATE_FINISHING    0x00000001
#define PROGSINK_STATE_CONNECTING   0x00000002
#define PROGSINK_STATE_LOADING      0x00000003

#define PROGSINK_SET_STATE          0x00000001
#define PROGSINK_SET_TEXT           0x00000002
#define PROGSINK_SET_IDS            0x00000004
#define PROGSINK_SET_POS            0x00000008
#define PROGSINK_SET_MAX            0x00000010



extern RPC_IF_HANDLE __MIDL_itf_prgsnk_0099_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_prgsnk_0099_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
