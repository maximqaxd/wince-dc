/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.02.88 */
/* at Tue Dec 14 22:38:58 1999
 */
/* Compiler settings for .\procdm.idl:
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

#ifndef __procdm_h__
#define __procdm_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IProcessDebugManager2_FWD_DEFINED__
#define __IProcessDebugManager2_FWD_DEFINED__
typedef interface IProcessDebugManager2 IProcessDebugManager2;
#endif 	/* __IProcessDebugManager2_FWD_DEFINED__ */


#ifndef __ProcessDebugManager_FWD_DEFINED__
#define __ProcessDebugManager_FWD_DEFINED__

#ifdef __cplusplus
typedef class ProcessDebugManager ProcessDebugManager;
#else
typedef struct ProcessDebugManager ProcessDebugManager;
#endif /* __cplusplus */

#endif 	/* __ProcessDebugManager_FWD_DEFINED__ */


#ifndef __DebugHelper_FWD_DEFINED__
#define __DebugHelper_FWD_DEFINED__

#ifdef __cplusplus
typedef class DebugHelper DebugHelper;
#else
typedef struct DebugHelper DebugHelper;
#endif /* __cplusplus */

#endif 	/* __DebugHelper_FWD_DEFINED__ */


#ifndef __CDebugDocumentHelper_FWD_DEFINED__
#define __CDebugDocumentHelper_FWD_DEFINED__

#ifdef __cplusplus
typedef class CDebugDocumentHelper CDebugDocumentHelper;
#else
typedef struct CDebugDocumentHelper CDebugDocumentHelper;
#endif /* __cplusplus */

#endif 	/* __CDebugDocumentHelper_FWD_DEFINED__ */


/* header files for imported files */
#include "activdbg.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __IProcessDebugManager2_INTERFACE_DEFINED__
#define __IProcessDebugManager2_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IProcessDebugManager2
 * at Tue Dec 14 22:38:58 1999
 * using MIDL 3.02.88
 ****************************************/
/* [unique][helpstring][uuid][object] */ 



EXTERN_C const IID IID_IProcessDebugManager2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("78a51820-51f4-11d0-8f20-00805f2cd064")
    IProcessDebugManager2 : public IUnknown
    {
    public:
    };
    
#else 	/* C style interface */

    typedef struct IProcessDebugManager2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IProcessDebugManager2 __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IProcessDebugManager2 __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IProcessDebugManager2 __RPC_FAR * This);
        
        END_INTERFACE
    } IProcessDebugManager2Vtbl;

    interface IProcessDebugManager2
    {
        CONST_VTBL struct IProcessDebugManager2Vtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IProcessDebugManager2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IProcessDebugManager2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IProcessDebugManager2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IProcessDebugManager2_INTERFACE_DEFINED__ */



#ifndef __PROCDMLib_LIBRARY_DEFINED__
#define __PROCDMLib_LIBRARY_DEFINED__

/****************************************
 * Generated header for library: PROCDMLib
 * at Tue Dec 14 22:38:58 1999
 * using MIDL 3.02.88
 ****************************************/
/* [helpstring][version][uuid] */ 


EXTERN_C const CLSID CLSID_CDebugDocumentHelper;

EXTERN_C const IID LIBID_PROCDMLib;

EXTERN_C const CLSID CLSID_ProcessDebugManager;

#ifdef __cplusplus

class DECLSPEC_UUID("78a51822-51f4-11d0-8f20-00805f2cd064")
ProcessDebugManager;
#endif

EXTERN_C const CLSID CLSID_DebugHelper;

#ifdef __cplusplus

class DECLSPEC_UUID("0BFCC060-8C1D-11d0-ACCD-00AA0060275C")
DebugHelper;
#endif

EXTERN_C const CLSID CLSID_CDebugDocumentHelper;

#ifdef __cplusplus

class DECLSPEC_UUID("83B8BCA6-687C-11D0-A405-00AA0060275C")
CDebugDocumentHelper;
#endif
#endif /* __PROCDMLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
