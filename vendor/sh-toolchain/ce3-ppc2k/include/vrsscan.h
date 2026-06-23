/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.02.88 */
/* at Tue Dec 14 22:38:59 1999
 */
/* Compiler settings for .\vrsscan.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: none
*/
//@@MIDL_FILE_HEADING(  )
#include "rpc.h"
#include "rpcndr.h"

#ifndef __vrsscan_h__
#define __vrsscan_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IVirusScanner_FWD_DEFINED__
#define __IVirusScanner_FWD_DEFINED__
typedef interface IVirusScanner IVirusScanner;
#endif 	/* __IVirusScanner_FWD_DEFINED__ */


#ifndef __IRegisterVirusScanEngine_FWD_DEFINED__
#define __IRegisterVirusScanEngine_FWD_DEFINED__
typedef interface IRegisterVirusScanEngine IRegisterVirusScanEngine;
#endif 	/* __IRegisterVirusScanEngine_FWD_DEFINED__ */


#ifndef __IVirusScanEngine_FWD_DEFINED__
#define __IVirusScanEngine_FWD_DEFINED__
typedef interface IVirusScanEngine IVirusScanEngine;
#endif 	/* __IVirusScanEngine_FWD_DEFINED__ */


#ifndef __VirusScan_FWD_DEFINED__
#define __VirusScan_FWD_DEFINED__

#ifdef __cplusplus
typedef class VirusScan VirusScan;
#else
typedef struct VirusScan VirusScan;
#endif /* __cplusplus */

#endif 	/* __VirusScan_FWD_DEFINED__ */


/* header files for imported files */
#include "objidl.h"
#include "oleidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/****************************************
 * Generated header for interface: __MIDL_itf_vrsscan_0000
 * at Tue Dec 14 22:38:59 1999
 * using MIDL 3.02.88
 ****************************************/
/* [local] */ 







extern RPC_IF_HANDLE __MIDL_itf_vrsscan_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_vrsscan_0000_v0_0_s_ifspec;


#ifndef __VIRUSSCAN_LIBRARY_DEFINED__
#define __VIRUSSCAN_LIBRARY_DEFINED__

/****************************************
 * Generated header for library: VIRUSSCAN
 * at Tue Dec 14 22:38:59 1999
 * using MIDL 3.02.88
 ****************************************/
/* [version][lcid][helpstring][uuid] */ 


//   Virus Scan Error codes
#define  VSCAN_E_NOPROVIDERS      _HRESULT_TYPEDEF_(0x800D1001)
#define  VSCAN_E_CHECKPARTIAL     _HRESULT_TYPEDEF_(0x800D1002)
#define  VSCAN_E_CHECKFAIL        _HRESULT_TYPEDEF_(0x800D1003)
#define  VSCAN_E_DELETEFAIL       _HRESULT_TYPEDEF_(0x800D1004)

// Virus Scanner Capabilities flags
#define  VSC_LIKESISTREAM      0x00000001
#define  VSC_LIKESFILE         0x00000002
#define  VSC_LIKESHGLOBAL      0x00000004
#define  VSC_HASMOREINFO       0x00000008

//   Virus Scan Engine Error codes
#define  VSE_E_UNSUPPORTEDINPUTTYPE      _HRESULT_TYPEDEF_(0x800D2001)
#define  VSE_E_CHECKFAIL                 _HRESULT_TYPEDEF_(0x800D2002)

#define	MAX_DESCRIPTION	( 128 )

#define	MAX_URL_LENGTH	( 2048 )

typedef struct  _tagVIRUSINFO
    {
    DWORD cbSize;
    WCHAR wszVendorDescription[ 128 ];
    HICON hVendorIcon;
    WCHAR wszVendorContactInfo[ 2048 ];
    WCHAR wszVirusName[ 128 ];
    WCHAR wszVirusDescription[ 128 ];
    }	VIRUSINFO;

typedef /* [unique] */ VIRUSINFO __RPC_FAR *LPVIRUSINFO;


#define    SFV_DONTDOUI             0x00000002                                
#define    SFV_DELETE               0x00000004                                
#define    SFV_WANTVENDORICON       0x00000008                                
#define    SFV_ENGINE_DOUI          0x00010000                                

#define REGISTERSCANNER_REREGISTER     1

EXTERN_C const IID LIBID_VIRUSSCAN;

#ifndef __IVirusScanner_INTERFACE_DEFINED__
#define __IVirusScanner_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IVirusScanner
 * at Tue Dec 14 22:38:59 1999
 * using MIDL 3.02.88
 ****************************************/
/* [uuid][object][local] */ 



EXTERN_C const IID IID_IVirusScanner;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("4589BEE0-B4B1-11CF-AAFA-00AA00B6015C")
    IVirusScanner : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ScanForVirus( 
            /* [in] */ HWND hWnd,
            /* [in] */ STGMEDIUM __RPC_FAR *pstgMedium,
            /* [in] */ LPWSTR pwszItemDescription,
            /* [in] */ DWORD dwFlags,
            /* [out][in] */ LPVIRUSINFO pVirusInfo) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IVirusScannerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IVirusScanner __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IVirusScanner __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IVirusScanner __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ScanForVirus )( 
            IVirusScanner __RPC_FAR * This,
            /* [in] */ HWND hWnd,
            /* [in] */ STGMEDIUM __RPC_FAR *pstgMedium,
            /* [in] */ LPWSTR pwszItemDescription,
            /* [in] */ DWORD dwFlags,
            /* [out][in] */ LPVIRUSINFO pVirusInfo);
        
        END_INTERFACE
    } IVirusScannerVtbl;

    interface IVirusScanner
    {
        CONST_VTBL struct IVirusScannerVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IVirusScanner_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IVirusScanner_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IVirusScanner_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IVirusScanner_ScanForVirus(This,hWnd,pstgMedium,pwszItemDescription,dwFlags,pVirusInfo)	\
    (This)->lpVtbl -> ScanForVirus(This,hWnd,pstgMedium,pwszItemDescription,dwFlags,pVirusInfo)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IVirusScanner_ScanForVirus_Proxy( 
    IVirusScanner __RPC_FAR * This,
    /* [in] */ HWND hWnd,
    /* [in] */ STGMEDIUM __RPC_FAR *pstgMedium,
    /* [in] */ LPWSTR pwszItemDescription,
    /* [in] */ DWORD dwFlags,
    /* [out][in] */ LPVIRUSINFO pVirusInfo);


void __RPC_STUB IVirusScanner_ScanForVirus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IVirusScanner_INTERFACE_DEFINED__ */


#ifndef __IRegisterVirusScanEngine_INTERFACE_DEFINED__
#define __IRegisterVirusScanEngine_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IRegisterVirusScanEngine
 * at Tue Dec 14 22:38:59 1999
 * using MIDL 3.02.88
 ****************************************/
/* [uuid][object][local] */ 



EXTERN_C const IID IID_IRegisterVirusScanEngine;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("0825E060-B961-11CF-AAFA-00AA00B6015C")
    IRegisterVirusScanEngine : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE RegisterScanEngine( 
            /* [in] */ REFCLSID rclsid,
            /* [in] */ LPWSTR pwszDescription,
            /* [in] */ DWORD dwFlags,
            /* [in] */ DWORD dwReserved,
            /* [out][in] */ DWORD __RPC_FAR *pdwCookie) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnRegisterScanEngine( 
            /* [in] */ REFCLSID rclsid,
            /* [in] */ LPWSTR pwszDescription,
            /* [in] */ DWORD dwFlags,
            /* [in] */ DWORD dwReserved,
            /* [in] */ DWORD dwCookie) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRegisterVirusScanEngineVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRegisterVirusScanEngine __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRegisterVirusScanEngine __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRegisterVirusScanEngine __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RegisterScanEngine )( 
            IRegisterVirusScanEngine __RPC_FAR * This,
            /* [in] */ REFCLSID rclsid,
            /* [in] */ LPWSTR pwszDescription,
            /* [in] */ DWORD dwFlags,
            /* [in] */ DWORD dwReserved,
            /* [out][in] */ DWORD __RPC_FAR *pdwCookie);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UnRegisterScanEngine )( 
            IRegisterVirusScanEngine __RPC_FAR * This,
            /* [in] */ REFCLSID rclsid,
            /* [in] */ LPWSTR pwszDescription,
            /* [in] */ DWORD dwFlags,
            /* [in] */ DWORD dwReserved,
            /* [in] */ DWORD dwCookie);
        
        END_INTERFACE
    } IRegisterVirusScanEngineVtbl;

    interface IRegisterVirusScanEngine
    {
        CONST_VTBL struct IRegisterVirusScanEngineVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRegisterVirusScanEngine_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRegisterVirusScanEngine_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRegisterVirusScanEngine_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRegisterVirusScanEngine_RegisterScanEngine(This,rclsid,pwszDescription,dwFlags,dwReserved,pdwCookie)	\
    (This)->lpVtbl -> RegisterScanEngine(This,rclsid,pwszDescription,dwFlags,dwReserved,pdwCookie)

#define IRegisterVirusScanEngine_UnRegisterScanEngine(This,rclsid,pwszDescription,dwFlags,dwReserved,dwCookie)	\
    (This)->lpVtbl -> UnRegisterScanEngine(This,rclsid,pwszDescription,dwFlags,dwReserved,dwCookie)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IRegisterVirusScanEngine_RegisterScanEngine_Proxy( 
    IRegisterVirusScanEngine __RPC_FAR * This,
    /* [in] */ REFCLSID rclsid,
    /* [in] */ LPWSTR pwszDescription,
    /* [in] */ DWORD dwFlags,
    /* [in] */ DWORD dwReserved,
    /* [out][in] */ DWORD __RPC_FAR *pdwCookie);


void __RPC_STUB IRegisterVirusScanEngine_RegisterScanEngine_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRegisterVirusScanEngine_UnRegisterScanEngine_Proxy( 
    IRegisterVirusScanEngine __RPC_FAR * This,
    /* [in] */ REFCLSID rclsid,
    /* [in] */ LPWSTR pwszDescription,
    /* [in] */ DWORD dwFlags,
    /* [in] */ DWORD dwReserved,
    /* [in] */ DWORD dwCookie);


void __RPC_STUB IRegisterVirusScanEngine_UnRegisterScanEngine_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRegisterVirusScanEngine_INTERFACE_DEFINED__ */


#ifndef __IVirusScanEngine_INTERFACE_DEFINED__
#define __IVirusScanEngine_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IVirusScanEngine
 * at Tue Dec 14 22:38:59 1999
 * using MIDL 3.02.88
 ****************************************/
/* [uuid][object][local] */ 



EXTERN_C const IID IID_IVirusScanEngine;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("083DB180-B4A8-11CF-AAFA-00AA00B6015C")
    IVirusScanEngine : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ScanForVirus( 
            /* [in] */ HWND hWnd,
            /* [in] */ STGMEDIUM __RPC_FAR *pstgMedium,
            /* [in] */ LPWSTR pwszItemDescription,
            /* [in] */ DWORD dwFlags,
            /* [in] */ DWORD dwReserved,
            /* [out][in] */ LPVIRUSINFO pVirusInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DisplayCustomInfo( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IVirusScanEngineVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IVirusScanEngine __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IVirusScanEngine __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IVirusScanEngine __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ScanForVirus )( 
            IVirusScanEngine __RPC_FAR * This,
            /* [in] */ HWND hWnd,
            /* [in] */ STGMEDIUM __RPC_FAR *pstgMedium,
            /* [in] */ LPWSTR pwszItemDescription,
            /* [in] */ DWORD dwFlags,
            /* [in] */ DWORD dwReserved,
            /* [out][in] */ LPVIRUSINFO pVirusInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DisplayCustomInfo )( 
            IVirusScanEngine __RPC_FAR * This);
        
        END_INTERFACE
    } IVirusScanEngineVtbl;

    interface IVirusScanEngine
    {
        CONST_VTBL struct IVirusScanEngineVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IVirusScanEngine_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IVirusScanEngine_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IVirusScanEngine_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IVirusScanEngine_ScanForVirus(This,hWnd,pstgMedium,pwszItemDescription,dwFlags,dwReserved,pVirusInfo)	\
    (This)->lpVtbl -> ScanForVirus(This,hWnd,pstgMedium,pwszItemDescription,dwFlags,dwReserved,pVirusInfo)

#define IVirusScanEngine_DisplayCustomInfo(This)	\
    (This)->lpVtbl -> DisplayCustomInfo(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IVirusScanEngine_ScanForVirus_Proxy( 
    IVirusScanEngine __RPC_FAR * This,
    /* [in] */ HWND hWnd,
    /* [in] */ STGMEDIUM __RPC_FAR *pstgMedium,
    /* [in] */ LPWSTR pwszItemDescription,
    /* [in] */ DWORD dwFlags,
    /* [in] */ DWORD dwReserved,
    /* [out][in] */ LPVIRUSINFO pVirusInfo);


void __RPC_STUB IVirusScanEngine_ScanForVirus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IVirusScanEngine_DisplayCustomInfo_Proxy( 
    IVirusScanEngine __RPC_FAR * This);


void __RPC_STUB IVirusScanEngine_DisplayCustomInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IVirusScanEngine_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_VirusScan;

#ifdef __cplusplus

class DECLSPEC_UUID("E88E5DE0-BD3E-11CF-AAFA-00AA00B6015C")
VirusScan;
#endif
#endif /* __VIRUSSCAN_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
