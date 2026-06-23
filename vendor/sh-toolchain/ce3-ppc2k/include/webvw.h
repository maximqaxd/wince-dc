/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.02.88 */
/* at Tue Dec 14 22:39:07 1999
 */
/* Compiler settings for .\webvw.idl:
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

#ifndef __webvw_h__
#define __webvw_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IThumbCtl_FWD_DEFINED__
#define __IThumbCtl_FWD_DEFINED__
typedef interface IThumbCtl IThumbCtl;
#endif 	/* __IThumbCtl_FWD_DEFINED__ */


#ifndef __IWebViewFolderIcon_FWD_DEFINED__
#define __IWebViewFolderIcon_FWD_DEFINED__
typedef interface IWebViewFolderIcon IWebViewFolderIcon;
#endif 	/* __IWebViewFolderIcon_FWD_DEFINED__ */


#ifndef __DThumbCtlEvents_FWD_DEFINED__
#define __DThumbCtlEvents_FWD_DEFINED__
typedef interface DThumbCtlEvents DThumbCtlEvents;
#endif 	/* __DThumbCtlEvents_FWD_DEFINED__ */


#ifndef __ThumbCtl_FWD_DEFINED__
#define __ThumbCtl_FWD_DEFINED__

#ifdef __cplusplus
typedef class ThumbCtl ThumbCtl;
#else
typedef struct ThumbCtl ThumbCtl;
#endif /* __cplusplus */

#endif 	/* __ThumbCtl_FWD_DEFINED__ */


#ifndef __WebViewFolderIcon_FWD_DEFINED__
#define __WebViewFolderIcon_FWD_DEFINED__

#ifdef __cplusplus
typedef class WebViewFolderIcon WebViewFolderIcon;
#else
typedef struct WebViewFolderIcon WebViewFolderIcon;
#endif /* __cplusplus */

#endif 	/* __WebViewFolderIcon_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __IThumbCtl_INTERFACE_DEFINED__
#define __IThumbCtl_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IThumbCtl
 * at Tue Dec 14 22:39:07 1999
 * using MIDL 3.02.88
 ****************************************/
/* [unique][helpstring][dual][uuid][object] */ 



EXTERN_C const IID IID_IThumbCtl;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("e8accae0-23e6-11d1-9e88-00c04fdcab92")
    IThumbCtl : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE displayFile( 
            BSTR bsFileName,
            /* [out][retval] */ VARIANT_BOOL __RPC_FAR *__MIDL_0016) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE haveThumbnail( 
            /* [out][retval] */ VARIANT_BOOL __RPC_FAR *__MIDL_0017) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_freeSpace( 
            /* [out][retval] */ BSTR __RPC_FAR *__MIDL_0018) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_usedSpace( 
            /* [out][retval] */ BSTR __RPC_FAR *__MIDL_0019) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_totalSpace( 
            /* [out][retval] */ BSTR __RPC_FAR *__MIDL_0020) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IThumbCtlVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IThumbCtl __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IThumbCtl __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IThumbCtl __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IThumbCtl __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IThumbCtl __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IThumbCtl __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IThumbCtl __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *displayFile )( 
            IThumbCtl __RPC_FAR * This,
            BSTR bsFileName,
            /* [out][retval] */ VARIANT_BOOL __RPC_FAR *__MIDL_0016);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *haveThumbnail )( 
            IThumbCtl __RPC_FAR * This,
            /* [out][retval] */ VARIANT_BOOL __RPC_FAR *__MIDL_0017);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_freeSpace )( 
            IThumbCtl __RPC_FAR * This,
            /* [out][retval] */ BSTR __RPC_FAR *__MIDL_0018);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_usedSpace )( 
            IThumbCtl __RPC_FAR * This,
            /* [out][retval] */ BSTR __RPC_FAR *__MIDL_0019);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_totalSpace )( 
            IThumbCtl __RPC_FAR * This,
            /* [out][retval] */ BSTR __RPC_FAR *__MIDL_0020);
        
        END_INTERFACE
    } IThumbCtlVtbl;

    interface IThumbCtl
    {
        CONST_VTBL struct IThumbCtlVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IThumbCtl_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IThumbCtl_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IThumbCtl_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IThumbCtl_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IThumbCtl_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IThumbCtl_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IThumbCtl_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IThumbCtl_displayFile(This,bsFileName,__MIDL_0016)	\
    (This)->lpVtbl -> displayFile(This,bsFileName,__MIDL_0016)

#define IThumbCtl_haveThumbnail(This,__MIDL_0017)	\
    (This)->lpVtbl -> haveThumbnail(This,__MIDL_0017)

#define IThumbCtl_get_freeSpace(This,__MIDL_0018)	\
    (This)->lpVtbl -> get_freeSpace(This,__MIDL_0018)

#define IThumbCtl_get_usedSpace(This,__MIDL_0019)	\
    (This)->lpVtbl -> get_usedSpace(This,__MIDL_0019)

#define IThumbCtl_get_totalSpace(This,__MIDL_0020)	\
    (This)->lpVtbl -> get_totalSpace(This,__MIDL_0020)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IThumbCtl_displayFile_Proxy( 
    IThumbCtl __RPC_FAR * This,
    BSTR bsFileName,
    /* [out][retval] */ VARIANT_BOOL __RPC_FAR *__MIDL_0016);


void __RPC_STUB IThumbCtl_displayFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IThumbCtl_haveThumbnail_Proxy( 
    IThumbCtl __RPC_FAR * This,
    /* [out][retval] */ VARIANT_BOOL __RPC_FAR *__MIDL_0017);


void __RPC_STUB IThumbCtl_haveThumbnail_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IThumbCtl_get_freeSpace_Proxy( 
    IThumbCtl __RPC_FAR * This,
    /* [out][retval] */ BSTR __RPC_FAR *__MIDL_0018);


void __RPC_STUB IThumbCtl_get_freeSpace_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IThumbCtl_get_usedSpace_Proxy( 
    IThumbCtl __RPC_FAR * This,
    /* [out][retval] */ BSTR __RPC_FAR *__MIDL_0019);


void __RPC_STUB IThumbCtl_get_usedSpace_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IThumbCtl_get_totalSpace_Proxy( 
    IThumbCtl __RPC_FAR * This,
    /* [out][retval] */ BSTR __RPC_FAR *__MIDL_0020);


void __RPC_STUB IThumbCtl_get_totalSpace_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IThumbCtl_INTERFACE_DEFINED__ */


#ifndef __IWebViewFolderIcon_INTERFACE_DEFINED__
#define __IWebViewFolderIcon_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IWebViewFolderIcon
 * at Tue Dec 14 22:39:07 1999
 * using MIDL 3.02.88
 ****************************************/
/* [unique][helpstring][dual][uuid][object] */ 



EXTERN_C const IID IID_IWebViewFolderIcon;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("e52b4910-3eb2-11d1-83e8-00a0c90dc849")
    IWebViewFolderIcon : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_scale( 
            /* [out][retval] */ int __RPC_FAR *__MIDL_0021) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_scale( 
            /* [in] */ int __MIDL_0022) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWebViewFolderIconVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWebViewFolderIcon __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWebViewFolderIcon __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWebViewFolderIcon __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IWebViewFolderIcon __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IWebViewFolderIcon __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IWebViewFolderIcon __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IWebViewFolderIcon __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_scale )( 
            IWebViewFolderIcon __RPC_FAR * This,
            /* [out][retval] */ int __RPC_FAR *__MIDL_0021);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_scale )( 
            IWebViewFolderIcon __RPC_FAR * This,
            /* [in] */ int __MIDL_0022);
        
        END_INTERFACE
    } IWebViewFolderIconVtbl;

    interface IWebViewFolderIcon
    {
        CONST_VTBL struct IWebViewFolderIconVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWebViewFolderIcon_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWebViewFolderIcon_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWebViewFolderIcon_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWebViewFolderIcon_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IWebViewFolderIcon_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IWebViewFolderIcon_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IWebViewFolderIcon_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IWebViewFolderIcon_get_scale(This,__MIDL_0021)	\
    (This)->lpVtbl -> get_scale(This,__MIDL_0021)

#define IWebViewFolderIcon_put_scale(This,__MIDL_0022)	\
    (This)->lpVtbl -> put_scale(This,__MIDL_0022)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IWebViewFolderIcon_get_scale_Proxy( 
    IWebViewFolderIcon __RPC_FAR * This,
    /* [out][retval] */ int __RPC_FAR *__MIDL_0021);


void __RPC_STUB IWebViewFolderIcon_get_scale_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IWebViewFolderIcon_put_scale_Proxy( 
    IWebViewFolderIcon __RPC_FAR * This,
    /* [in] */ int __MIDL_0022);


void __RPC_STUB IWebViewFolderIcon_put_scale_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWebViewFolderIcon_INTERFACE_DEFINED__ */



#ifndef __WEBVWLib_LIBRARY_DEFINED__
#define __WEBVWLib_LIBRARY_DEFINED__

/****************************************
 * Generated header for library: WEBVWLib
 * at Tue Dec 14 22:39:07 1999
 * using MIDL 3.02.88
 ****************************************/
/* [helpstring][version][uuid] */ 



EXTERN_C const IID LIBID_WEBVWLib;

#ifndef __DThumbCtlEvents_DISPINTERFACE_DEFINED__
#define __DThumbCtlEvents_DISPINTERFACE_DEFINED__

/****************************************
 * Generated header for dispinterface: DThumbCtlEvents
 * at Tue Dec 14 22:39:07 1999
 * using MIDL 3.02.88
 ****************************************/
/* [helpstring][uuid] */ 



EXTERN_C const IID DIID_DThumbCtlEvents;

#if defined(__cplusplus) && !defined(CINTERFACE)

    interface DECLSPEC_UUID("58d6f4b0-181d-11d1-9e88-00c04fdcab92")
    DThumbCtlEvents : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct DThumbCtlEventsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            DThumbCtlEvents __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            DThumbCtlEvents __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            DThumbCtlEvents __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            DThumbCtlEvents __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            DThumbCtlEvents __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            DThumbCtlEvents __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            DThumbCtlEvents __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        END_INTERFACE
    } DThumbCtlEventsVtbl;

    interface DThumbCtlEvents
    {
        CONST_VTBL struct DThumbCtlEventsVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define DThumbCtlEvents_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define DThumbCtlEvents_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define DThumbCtlEvents_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define DThumbCtlEvents_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define DThumbCtlEvents_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define DThumbCtlEvents_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define DThumbCtlEvents_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* __DThumbCtlEvents_DISPINTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_ThumbCtl;

#ifdef __cplusplus

class DECLSPEC_UUID("1d2b4f40-1f10-11d1-9e88-00c04fdcab92")
ThumbCtl;
#endif

EXTERN_C const CLSID CLSID_WebViewFolderIcon;

#ifdef __cplusplus

class DECLSPEC_UUID("e5df9d10-3b52-11d1-83e8-00a0c90dc849")
WebViewFolderIcon;
#endif
#endif /* __WEBVWLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long __RPC_FAR *, unsigned long            , BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long __RPC_FAR *, BSTR __RPC_FAR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
