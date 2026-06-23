/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.02.88 */
/* at Tue Dec 14 22:38:44 1999
 */
/* Compiler settings for .\optary.idl:
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

#ifndef __optary_h__
#define __optary_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IOptionArray_FWD_DEFINED__
#define __IOptionArray_FWD_DEFINED__
typedef interface IOptionArray IOptionArray;
#endif 	/* __IOptionArray_FWD_DEFINED__ */


#ifndef __IHtmlLoadOptions_FWD_DEFINED__
#define __IHtmlLoadOptions_FWD_DEFINED__
typedef interface IHtmlLoadOptions IHtmlLoadOptions;
#endif 	/* __IHtmlLoadOptions_FWD_DEFINED__ */


/* header files for imported files */
#include "objidl.h"
#include "oleidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/****************************************
 * Generated header for interface: __MIDL_itf_optary_0000
 * at Tue Dec 14 22:38:44 1999
 * using MIDL 3.02.88
 ****************************************/
/* [local] */ 


//=--------------------------------------------------------------------------=
// optary.h
//=--------------------------------------------------------------------------=
// (C) Copyright 1995-1997 Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=

#pragma comment(lib,"uuid.lib")

//---------------------------------------------------------------------------=
// IOptionArray Interface.


#ifndef _LPOPTIONARRAY_DEFINED
#define _LPOPTIONARRAY_DEFINED


extern RPC_IF_HANDLE __MIDL_itf_optary_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_optary_0000_v0_0_s_ifspec;

#ifndef __IOptionArray_INTERFACE_DEFINED__
#define __IOptionArray_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IOptionArray
 * at Tue Dec 14 22:38:44 1999
 * using MIDL 3.02.88
 ****************************************/
/* [unique][uuid][object][local] */ 


typedef /* [unique] */ IOptionArray __RPC_FAR *LPOPTIONARRAY;


EXTERN_C const IID IID_IOptionArray;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("22b6d492-0f88-11d1-ba19-00c04fd912d0")
    IOptionArray : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE QueryOption( 
            /* [in] */ DWORD dwOption,
            /* [size_is][out] */ LPVOID pBuffer,
            /* [out][in] */ ULONG __RPC_FAR *pcbBuf) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetOption( 
            /* [in] */ DWORD dwOption,
            /* [size_is][in] */ LPVOID pBuffer,
            /* [in] */ ULONG cbBuf) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IOptionArrayVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IOptionArray __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IOptionArray __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IOptionArray __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryOption )( 
            IOptionArray __RPC_FAR * This,
            /* [in] */ DWORD dwOption,
            /* [size_is][out] */ LPVOID pBuffer,
            /* [out][in] */ ULONG __RPC_FAR *pcbBuf);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetOption )( 
            IOptionArray __RPC_FAR * This,
            /* [in] */ DWORD dwOption,
            /* [size_is][in] */ LPVOID pBuffer,
            /* [in] */ ULONG cbBuf);
        
        END_INTERFACE
    } IOptionArrayVtbl;

    interface IOptionArray
    {
        CONST_VTBL struct IOptionArrayVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IOptionArray_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IOptionArray_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IOptionArray_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IOptionArray_QueryOption(This,dwOption,pBuffer,pcbBuf)	\
    (This)->lpVtbl -> QueryOption(This,dwOption,pBuffer,pcbBuf)

#define IOptionArray_SetOption(This,dwOption,pBuffer,cbBuf)	\
    (This)->lpVtbl -> SetOption(This,dwOption,pBuffer,cbBuf)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IOptionArray_QueryOption_Proxy( 
    IOptionArray __RPC_FAR * This,
    /* [in] */ DWORD dwOption,
    /* [size_is][out] */ LPVOID pBuffer,
    /* [out][in] */ ULONG __RPC_FAR *pcbBuf);


void __RPC_STUB IOptionArray_QueryOption_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IOptionArray_SetOption_Proxy( 
    IOptionArray __RPC_FAR * This,
    /* [in] */ DWORD dwOption,
    /* [size_is][in] */ LPVOID pBuffer,
    /* [in] */ ULONG cbBuf);


void __RPC_STUB IOptionArray_SetOption_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IOptionArray_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL_itf_optary_0082
 * at Tue Dec 14 22:38:44 1999
 * using MIDL 3.02.88
 ****************************************/
/* [local] */ 


// HTMLLoadOptions CLSID
EXTERN_C const CLSID CLSID_HTMLLoadOptions; // {18845040-0fa5-11d1-ba19-00c04fd912d0}


extern RPC_IF_HANDLE __MIDL_itf_optary_0082_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_optary_0082_v0_0_s_ifspec;

#ifndef __IHtmlLoadOptions_INTERFACE_DEFINED__
#define __IHtmlLoadOptions_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IHtmlLoadOptions
 * at Tue Dec 14 22:38:44 1999
 * using MIDL 3.02.88
 ****************************************/
/* [unique][uuid][object][local] */ 


typedef /* [public] */ 
enum __MIDL_IHtmlLoadOptions_0001
    {	HTMLLOADOPTION_CODEPAGE	= 0
    }	HTMLLOADOPTION;


EXTERN_C const IID IID_IHtmlLoadOptions;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("a71a0808-0f88-11d1-ba19-00c04fd912d0")
    IHtmlLoadOptions : public IOptionArray
    {
    public:
    };
    
#else 	/* C style interface */

    typedef struct IHtmlLoadOptionsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IHtmlLoadOptions __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IHtmlLoadOptions __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IHtmlLoadOptions __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryOption )( 
            IHtmlLoadOptions __RPC_FAR * This,
            /* [in] */ DWORD dwOption,
            /* [size_is][out] */ LPVOID pBuffer,
            /* [out][in] */ ULONG __RPC_FAR *pcbBuf);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetOption )( 
            IHtmlLoadOptions __RPC_FAR * This,
            /* [in] */ DWORD dwOption,
            /* [size_is][in] */ LPVOID pBuffer,
            /* [in] */ ULONG cbBuf);
        
        END_INTERFACE
    } IHtmlLoadOptionsVtbl;

    interface IHtmlLoadOptions
    {
        CONST_VTBL struct IHtmlLoadOptionsVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IHtmlLoadOptions_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IHtmlLoadOptions_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IHtmlLoadOptions_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IHtmlLoadOptions_QueryOption(This,dwOption,pBuffer,pcbBuf)	\
    (This)->lpVtbl -> QueryOption(This,dwOption,pBuffer,pcbBuf)

#define IHtmlLoadOptions_SetOption(This,dwOption,pBuffer,cbBuf)	\
    (This)->lpVtbl -> SetOption(This,dwOption,pBuffer,cbBuf)


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IHtmlLoadOptions_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL_itf_optary_0083
 * at Tue Dec 14 22:38:44 1999
 * using MIDL 3.02.88
 ****************************************/
/* [local] */ 


#endif


extern RPC_IF_HANDLE __MIDL_itf_optary_0083_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_optary_0083_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
