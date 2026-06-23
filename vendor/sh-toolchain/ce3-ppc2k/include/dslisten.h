/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.02.88 */
/* at Tue Dec 14 22:39:01 1999
 */
/* Compiler settings for .\dslisten.idl:
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

#ifndef __dslisten_h__
#define __dslisten_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IDATASRCListener_FWD_DEFINED__
#define __IDATASRCListener_FWD_DEFINED__
typedef interface IDATASRCListener IDATASRCListener;
#endif 	/* __IDATASRCListener_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __IDATASRCListener_INTERFACE_DEFINED__
#define __IDATASRCListener_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IDATASRCListener
 * at Tue Dec 14 22:39:01 1999
 * using MIDL 3.02.88
 ****************************************/
/* [uuid][version][object][local] */ 



EXTERN_C const IID IID_IDATASRCListener;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("3050F380-98B5-11CF-BB82-00AA00BDCE0B")
    IDATASRCListener : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE datasrcChanged( 
            /* [in] */ BSTR bstrQualifier,
            /* [in] */ BOOL fDataAvail) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDATASRCListenerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDATASRCListener __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDATASRCListener __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDATASRCListener __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *datasrcChanged )( 
            IDATASRCListener __RPC_FAR * This,
            /* [in] */ BSTR bstrQualifier,
            /* [in] */ BOOL fDataAvail);
        
        END_INTERFACE
    } IDATASRCListenerVtbl;

    interface IDATASRCListener
    {
        CONST_VTBL struct IDATASRCListenerVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDATASRCListener_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDATASRCListener_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDATASRCListener_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDATASRCListener_datasrcChanged(This,bstrQualifier,fDataAvail)	\
    (This)->lpVtbl -> datasrcChanged(This,bstrQualifier,fDataAvail)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDATASRCListener_datasrcChanged_Proxy( 
    IDATASRCListener __RPC_FAR * This,
    /* [in] */ BSTR bstrQualifier,
    /* [in] */ BOOL fDataAvail);


void __RPC_STUB IDATASRCListener_datasrcChanged_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDATASRCListener_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
