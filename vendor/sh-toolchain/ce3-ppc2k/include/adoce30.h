/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.02.88 */
/* at Tue Nov 09 09:56:36 1999
 */
/* Compiler settings for .\adoce30.idl:
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

#ifndef __adoce30_h__
#define __adoce30_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef ___Collection_FWD_DEFINED__
#define ___Collection_FWD_DEFINED__
typedef interface _Collection _Collection;
#endif 	/* ___Collection_FWD_DEFINED__ */


#ifndef __Field_FWD_DEFINED__
#define __Field_FWD_DEFINED__
typedef interface Field Field;
#endif 	/* __Field_FWD_DEFINED__ */


#ifndef __Fields_FWD_DEFINED__
#define __Fields_FWD_DEFINED__
typedef interface Fields Fields;
#endif 	/* __Fields_FWD_DEFINED__ */


#ifndef __Property_FWD_DEFINED__
#define __Property_FWD_DEFINED__
typedef interface Property Property;
#endif 	/* __Property_FWD_DEFINED__ */


#ifndef __Properties_FWD_DEFINED__
#define __Properties_FWD_DEFINED__
typedef interface Properties Properties;
#endif 	/* __Properties_FWD_DEFINED__ */


#ifndef __Error_FWD_DEFINED__
#define __Error_FWD_DEFINED__
typedef interface Error Error;
#endif 	/* __Error_FWD_DEFINED__ */


#ifndef __Errors_FWD_DEFINED__
#define __Errors_FWD_DEFINED__
typedef interface Errors Errors;
#endif 	/* __Errors_FWD_DEFINED__ */


#ifndef ___Recordset_FWD_DEFINED__
#define ___Recordset_FWD_DEFINED__
typedef interface _Recordset _Recordset;
#endif 	/* ___Recordset_FWD_DEFINED__ */


#ifndef ___Connection_FWD_DEFINED__
#define ___Connection_FWD_DEFINED__
typedef interface _Connection _Connection;
#endif 	/* ___Connection_FWD_DEFINED__ */


#ifndef __Recordset_FWD_DEFINED__
#define __Recordset_FWD_DEFINED__

#ifdef __cplusplus
typedef class Recordset Recordset;
#else
typedef struct Recordset Recordset;
#endif /* __cplusplus */

#endif 	/* __Recordset_FWD_DEFINED__ */


#ifndef __Connection_FWD_DEFINED__
#define __Connection_FWD_DEFINED__

#ifdef __cplusplus
typedef class Connection Connection;
#else
typedef struct Connection Connection;
#endif /* __cplusplus */

#endif 	/* __Connection_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/****************************************
 * Generated header for interface: __MIDL_itf_adoce30_0000
 * at Tue Nov 09 09:56:36 1999
 * using MIDL 3.02.88
 ****************************************/
/* [local] */ 



typedef /* [public][uuid] */ 
enum __MIDL___MIDL_itf_adoce30_0000_0001
    {	adGetRowsRest	= -1
    }	GetRowsOptionEnum;

typedef /* [uuid] */ 
enum SeekEnum
    {	adSeekFirstEQ	= 1,
	adSeekLastEQ	= 2,
	adSeekAfterEQ	= 4,
	adSeekAfter	= 8,
	adSeekBeforeEQ	= 16,
	adSeekBefore	= 32
    }	SeekEnum;

typedef /* [uuid] */ 
enum SearchDirectionEnum
    {	adSearchForward	= 1,
	adSearchBackward	= -1
    }	SearchDirectionEnum;

typedef /* [uuid] */ 
enum IsolationLevelEnum
    {	adXactUnspecified	= -1,
	adXactChaos	= 16,
	adXactReadUncommitted	= 256,
	adXactBrowse	= 256,
	adXactCursorStability	= 4096,
	adXactReadCommitted	= 4096,
	adXactRepeatableRead	= 65536,
	adXactSerializable	= 1048576,
	adXactIsolated	= 1048576
    }	IsolationLevelEnum;

typedef /* [uuid] */ 
enum CursorTypeEnum
    {	adOpenUnspecified	= -1,
	adOpenForwardOnly	= 0,
	adOpenKeyset	= 1,
	adOpenDynamic	= 2,
	adOpenStatic	= 3
    }	CursorTypeEnum;

typedef /* [uuid] */ 
enum PositionEnum
    {	adPosUnknown	= -1,
	adPosBOF	= -2,
	adPosEOF	= -3
    }	PositionEnum;

typedef /* [uuid] */ 
enum DataTypeEnum
    {	adBigInt	= 20,
	adBinary	= 128,
	adBoolean	= 11,
	adBSTR	= 8,
	adChar	= 129,
	adCurrency	= 6,
	adDate	= 7,
	adDBDate	= 133,
	adDBTime	= 134,
	adDBTimeStamp	= 135,
	adDecimal	= 14,
	adDouble	= 5,
	adEmpty	= 0,
	adError	= 10,
	adGUID	= 72,
	adIDispatch	= 9,
	adInteger	= 3,
	adIUnknown	= 13,
	adLongVarBinary	= 205,
	adLongVarChar	= 201,
	adLongVarWChar	= 203,
	adNumeric	= 131,
	adSingle	= 4,
	adSmallInt	= 2,
	adTinyInt	= 16,
	adUnsignedBigInt	= 21,
	adUnsignedInt	= 19,
	adUnsignedSmallInt	= 18,
	adUnsignedTinyInt	= 17,
	adUserDefined	= 132,
	adVarBinary	= 204,
	adVarChar	= 200,
	adVariant	= 12,
	adVarWChar	= 202,
	adWChar	= 130
    }	DataTypeEnum;

typedef /* [uuid] */ 
enum CursorOptionEnum
    {	adAddNew	= 0x1000400,
	adApproxPosition	= 0x4000,
	adBookmark	= 0x2000,
	adDelete	= 0x1000800,
	adHoldRecords	= 0x100,
	adMovePrevious	= 0x200,
	adResync	= 0x20000,
	adUpdate	= 0x1008000,
	adUpdateBatch	= 0x10000,
	adFind	= 0x80000,
	adSeek	= 0x400000,
	adIndex	= 0x800000
    }	CursorOptionEnum;

typedef /* [uuid] */ 
enum LockTypeEnum
    {	adLockUnspecified	= -1,
	adLockReadOnly	= 1,
	adLockPessimistic	= 2,
	adLockOptimistic	= 3
    }	LockTypeEnum;

typedef /* [uuid] */ 
enum EditModeEnum
    {	adEditNone	= 0,
	adEditInProgress	= 0x1,
	adEditAdd	= 0x2
    }	EditModeEnum;

typedef /* [uuid] */ 
enum FieldAttributeEnum
    {	adFldMayDefer	= 2,
	adFldUpdatable	= 4,
	adFldUnknownUpdatable	= 8,
	adFldFixed	= 16,
	adFldIsNullable	= 32,
	adFldMayBeNull	= 64,
	adFldLong	= 128,
	adFldRowID	= 256,
	adFldRowVersion	= 512,
	adFldCacheDeferred	= 4096,
	adFldNegativeScale	= 16384,
	adFldKeyColumn	= 32768
    }	FieldAttributeEnum;

typedef /* [uuid] */ 
enum CursorLocationEnum
    {	adUseNone	= 1,
	adUseServer	= 2
    }	CursorLocationEnum;

typedef /* [uuid] */ 
enum ConnectModeEnum
    {	adModeUnknown	= 0,
	adModeRead	= 1,
	adModeWrite	= 2,
	adModeReadWrite	= 3,
	adModeShareDenyRead	= 4,
	adModeShareDenyWrite	= 8,
	adModeShareExclusive	= 12,
	adModeShareDenyNone	= 16
    }	ConnectModeEnum;

typedef /* [uuid] */ 
enum SchemaEnum
    {	adSchemaProviderSpecific	= -1,
	adSchemaAsserts	= 0,
	adSchemaCatalogs	= 1,
	adSchemaCharacterSets	= 2,
	adSchemaCollations	= 3,
	adSchemaColumns	= 4,
	adSchemaCheckConstraints	= 5,
	adSchemaConstraintColumnUsage	= 6,
	adSchemaConstraintTableUsage	= 7,
	adSchemaKeyColumnUsage	= 8,
	adSchemaReferentialContraints	= 9,
	adSchemaReferentialConstraints	= 9,
	adSchemaTableConstraints	= 10,
	adSchemaColumnsDomainUsage	= 11,
	adSchemaIndexes	= 12,
	adSchemaColumnPrivileges	= 13,
	adSchemaTablePrivileges	= 14,
	adSchemaUsagePrivileges	= 15,
	adSchemaProcedures	= 16,
	adSchemaSchemata	= 17,
	adSchemaSQLLanguages	= 18,
	adSchemaStatistics	= 19,
	adSchemaTables	= 20,
	adSchemaTranslations	= 21,
	adSchemaProviderTypes	= 22,
	adSchemaViews	= 23,
	adSchemaViewColumnUsage	= 24,
	adSchemaViewTableUsage	= 25,
	adSchemaProcedureParameters	= 26,
	adSchemaForeignKeys	= 27,
	adSchemaPrimaryKeys	= 28,
	adSchemaProcedureColumns	= 29,
	adSchemaDBInfoKeywords	= 30,
	adSchemaDBInfoLiterals	= 31,
	adSchemaCubes	= 32,
	adSchemaDimensions	= 33,
	adSchemaHierarchies	= 34,
	adSchemaLevels	= 35,
	adSchemaMeasures	= 36,
	adSchemaProperties	= 37,
	adSchemaMembers	= 38,
	adSchemaTrustees	= 39
    }	SchemaEnum;



extern RPC_IF_HANDLE __MIDL_itf_adoce30_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_adoce30_0000_v0_0_s_ifspec;

#ifndef ___Collection_INTERFACE_DEFINED__
#define ___Collection_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: _Collection
 * at Tue Nov 09 09:56:36 1999
 * using MIDL 3.02.88
 ****************************************/
/* [unique][helpstring][dual][uuid][object] */ 



EXTERN_C const IID IID__Collection;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("113033F3-F682-11D2-BB62-00C04F680ACC")
    _Collection : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;
        
        virtual /* [id][restricted] */ HRESULT STDMETHODCALLTYPE _NewEnum( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppvObject) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Refresh( void) = 0;
        
        virtual /* [propget][hidden] */ HRESULT STDMETHODCALLTYPE get_Element( 
            VARIANT __RPC_FAR *pvar,
            int index) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct _CollectionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            _Collection __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            _Collection __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            _Collection __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            _Collection __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            _Collection __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            _Collection __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            _Collection __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Count )( 
            _Collection __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][restricted] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *_NewEnum )( 
            _Collection __RPC_FAR * This,
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppvObject);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Refresh )( 
            _Collection __RPC_FAR * This);
        
        /* [propget][hidden] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Element )( 
            _Collection __RPC_FAR * This,
            VARIANT __RPC_FAR *pvar,
            int index);
        
        END_INTERFACE
    } _CollectionVtbl;

    interface _Collection
    {
        CONST_VTBL struct _CollectionVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define _Collection_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define _Collection_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define _Collection_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define _Collection_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define _Collection_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define _Collection_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define _Collection_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define _Collection_get_Count(This,pVal)	\
    (This)->lpVtbl -> get_Count(This,pVal)

#define _Collection__NewEnum(This,ppvObject)	\
    (This)->lpVtbl -> _NewEnum(This,ppvObject)

#define _Collection_Refresh(This)	\
    (This)->lpVtbl -> Refresh(This)

#define _Collection_get_Element(This,pvar,index)	\
    (This)->lpVtbl -> get_Element(This,pvar,index)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE _Collection_get_Count_Proxy( 
    _Collection __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB _Collection_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][restricted] */ HRESULT STDMETHODCALLTYPE _Collection__NewEnum_Proxy( 
    _Collection __RPC_FAR * This,
    /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppvObject);


void __RPC_STUB _Collection__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE _Collection_Refresh_Proxy( 
    _Collection __RPC_FAR * This);


void __RPC_STUB _Collection_Refresh_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][hidden] */ HRESULT STDMETHODCALLTYPE _Collection_get_Element_Proxy( 
    _Collection __RPC_FAR * This,
    VARIANT __RPC_FAR *pvar,
    int index);


void __RPC_STUB _Collection_get_Element_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* ___Collection_INTERFACE_DEFINED__ */


#ifndef __Field_INTERFACE_DEFINED__
#define __Field_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: Field
 * at Tue Nov 09 09:56:36 1999
 * using MIDL 3.02.88
 ****************************************/
/* [unique][helpstring][oleautomation][nonextensible][dual][uuid][object] */ 



EXTERN_C const IID IID_Field;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("113033F4-F682-11D2-BB62-00C04F680ACC")
    Field : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_ActualSize( 
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Attributes( 
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_DefinedSize( 
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Type( 
            /* [retval][out] */ DataTypeEnum __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_UnderlyingValue( 
            /* [retval][out] */ VARIANT __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Precision( 
            /* [retval][out] */ short __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_NumericScale( 
            /* [retval][out] */ short __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Value( 
            /* [retval][out] */ VARIANT __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Value( 
            /* [in] */ VARIANT newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct FieldVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            Field __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            Field __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            Field __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            Field __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            Field __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            Field __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            Field __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ActualSize )( 
            Field __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Attributes )( 
            Field __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DefinedSize )( 
            Field __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Name )( 
            Field __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Type )( 
            Field __RPC_FAR * This,
            /* [retval][out] */ DataTypeEnum __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_UnderlyingValue )( 
            Field __RPC_FAR * This,
            /* [retval][out] */ VARIANT __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Precision )( 
            Field __RPC_FAR * This,
            /* [retval][out] */ short __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_NumericScale )( 
            Field __RPC_FAR * This,
            /* [retval][out] */ short __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Value )( 
            Field __RPC_FAR * This,
            /* [retval][out] */ VARIANT __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Value )( 
            Field __RPC_FAR * This,
            /* [in] */ VARIANT newVal);
        
        END_INTERFACE
    } FieldVtbl;

    interface Field
    {
        CONST_VTBL struct FieldVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define Field_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define Field_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define Field_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define Field_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define Field_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define Field_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define Field_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define Field_get_ActualSize(This,pVal)	\
    (This)->lpVtbl -> get_ActualSize(This,pVal)

#define Field_get_Attributes(This,pVal)	\
    (This)->lpVtbl -> get_Attributes(This,pVal)

#define Field_get_DefinedSize(This,pVal)	\
    (This)->lpVtbl -> get_DefinedSize(This,pVal)

#define Field_get_Name(This,pVal)	\
    (This)->lpVtbl -> get_Name(This,pVal)

#define Field_get_Type(This,pVal)	\
    (This)->lpVtbl -> get_Type(This,pVal)

#define Field_get_UnderlyingValue(This,pVal)	\
    (This)->lpVtbl -> get_UnderlyingValue(This,pVal)

#define Field_get_Precision(This,pVal)	\
    (This)->lpVtbl -> get_Precision(This,pVal)

#define Field_get_NumericScale(This,pVal)	\
    (This)->lpVtbl -> get_NumericScale(This,pVal)

#define Field_get_Value(This,pVal)	\
    (This)->lpVtbl -> get_Value(This,pVal)

#define Field_put_Value(This,newVal)	\
    (This)->lpVtbl -> put_Value(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE Field_get_ActualSize_Proxy( 
    Field __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB Field_get_ActualSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE Field_get_Attributes_Proxy( 
    Field __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB Field_get_Attributes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE Field_get_DefinedSize_Proxy( 
    Field __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB Field_get_DefinedSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE Field_get_Name_Proxy( 
    Field __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB Field_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE Field_get_Type_Proxy( 
    Field __RPC_FAR * This,
    /* [retval][out] */ DataTypeEnum __RPC_FAR *pVal);


void __RPC_STUB Field_get_Type_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE Field_get_UnderlyingValue_Proxy( 
    Field __RPC_FAR * This,
    /* [retval][out] */ VARIANT __RPC_FAR *pVal);


void __RPC_STUB Field_get_UnderlyingValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE Field_get_Precision_Proxy( 
    Field __RPC_FAR * This,
    /* [retval][out] */ short __RPC_FAR *pVal);


void __RPC_STUB Field_get_Precision_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE Field_get_NumericScale_Proxy( 
    Field __RPC_FAR * This,
    /* [retval][out] */ short __RPC_FAR *pVal);


void __RPC_STUB Field_get_NumericScale_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE Field_get_Value_Proxy( 
    Field __RPC_FAR * This,
    /* [retval][out] */ VARIANT __RPC_FAR *pVal);


void __RPC_STUB Field_get_Value_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE Field_put_Value_Proxy( 
    Field __RPC_FAR * This,
    /* [in] */ VARIANT newVal);


void __RPC_STUB Field_put_Value_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __Field_INTERFACE_DEFINED__ */


#ifndef __Fields_INTERFACE_DEFINED__
#define __Fields_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: Fields
 * at Tue Nov 09 09:56:36 1999
 * using MIDL 3.02.88
 ****************************************/
/* [unique][helpstring][dual][uuid][object] */ 



EXTERN_C const IID IID_Fields;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("113033F5-F682-11D2-BB62-00C04F680ACC")
    Fields : public _Collection
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ VARIANT varIndex,
            /* [retval][out] */ Field __RPC_FAR *__RPC_FAR *pVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct FieldsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            Fields __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            Fields __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            Fields __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            Fields __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            Fields __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            Fields __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            Fields __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Count )( 
            Fields __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][restricted] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *_NewEnum )( 
            Fields __RPC_FAR * This,
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppvObject);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Refresh )( 
            Fields __RPC_FAR * This);
        
        /* [propget][hidden] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Element )( 
            Fields __RPC_FAR * This,
            VARIANT __RPC_FAR *pvar,
            int index);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Item )( 
            Fields __RPC_FAR * This,
            /* [in] */ VARIANT varIndex,
            /* [retval][out] */ Field __RPC_FAR *__RPC_FAR *pVal);
        
        END_INTERFACE
    } FieldsVtbl;

    interface Fields
    {
        CONST_VTBL struct FieldsVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define Fields_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define Fields_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define Fields_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define Fields_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define Fields_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define Fields_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define Fields_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define Fields_get_Count(This,pVal)	\
    (This)->lpVtbl -> get_Count(This,pVal)

#define Fields__NewEnum(This,ppvObject)	\
    (This)->lpVtbl -> _NewEnum(This,ppvObject)

#define Fields_Refresh(This)	\
    (This)->lpVtbl -> Refresh(This)

#define Fields_get_Element(This,pvar,index)	\
    (This)->lpVtbl -> get_Element(This,pvar,index)


#define Fields_get_Item(This,varIndex,pVal)	\
    (This)->lpVtbl -> get_Item(This,varIndex,pVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE Fields_get_Item_Proxy( 
    Fields __RPC_FAR * This,
    /* [in] */ VARIANT varIndex,
    /* [retval][out] */ Field __RPC_FAR *__RPC_FAR *pVal);


void __RPC_STUB Fields_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __Fields_INTERFACE_DEFINED__ */


#ifndef __Property_INTERFACE_DEFINED__
#define __Property_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: Property
 * at Tue Nov 09 09:56:36 1999
 * using MIDL 3.02.88
 ****************************************/
/* [unique][helpstring][dual][uuid][object] */ 



EXTERN_C const IID IID_Property;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("113033EE-F682-11D2-BB62-00C04F680ACC")
    Property : public IDispatch
    {
    public:
        virtual /* [propget][id] */ HRESULT __stdcall get_Value( 
            /* [retval][out] */ VARIANT __RPC_FAR *pval) = 0;
        
        virtual /* [propput][id] */ HRESULT __stdcall put_Value( 
            /* [in] */ VARIANT pval) = 0;
        
        virtual /* [propget][id] */ HRESULT __stdcall get_Name( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstr) = 0;
        
        virtual /* [propget][id] */ HRESULT __stdcall get_Type( 
            /* [retval][out] */ DataTypeEnum __RPC_FAR *ptype) = 0;
        
        virtual /* [propget][id] */ HRESULT __stdcall get_Attributes( 
            /* [retval][out] */ long __RPC_FAR *plAttributes) = 0;
        
        virtual /* [propput][id] */ HRESULT __stdcall put_Attributes( 
            /* [in] */ long plAttributes) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct PropertyVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            Property __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            Property __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            Property __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            Property __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            Property __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            Property __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            Property __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [propget][id] */ HRESULT ( __stdcall __RPC_FAR *get_Value )( 
            Property __RPC_FAR * This,
            /* [retval][out] */ VARIANT __RPC_FAR *pval);
        
        /* [propput][id] */ HRESULT ( __stdcall __RPC_FAR *put_Value )( 
            Property __RPC_FAR * This,
            /* [in] */ VARIANT pval);
        
        /* [propget][id] */ HRESULT ( __stdcall __RPC_FAR *get_Name )( 
            Property __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstr);
        
        /* [propget][id] */ HRESULT ( __stdcall __RPC_FAR *get_Type )( 
            Property __RPC_FAR * This,
            /* [retval][out] */ DataTypeEnum __RPC_FAR *ptype);
        
        /* [propget][id] */ HRESULT ( __stdcall __RPC_FAR *get_Attributes )( 
            Property __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plAttributes);
        
        /* [propput][id] */ HRESULT ( __stdcall __RPC_FAR *put_Attributes )( 
            Property __RPC_FAR * This,
            /* [in] */ long plAttributes);
        
        END_INTERFACE
    } PropertyVtbl;

    interface Property
    {
        CONST_VTBL struct PropertyVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define Property_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define Property_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define Property_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define Property_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define Property_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define Property_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define Property_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define Property_get_Value(This,pval)	\
    (This)->lpVtbl -> get_Value(This,pval)

#define Property_put_Value(This,pval)	\
    (This)->lpVtbl -> put_Value(This,pval)

#define Property_get_Name(This,pbstr)	\
    (This)->lpVtbl -> get_Name(This,pbstr)

#define Property_get_Type(This,ptype)	\
    (This)->lpVtbl -> get_Type(This,ptype)

#define Property_get_Attributes(This,plAttributes)	\
    (This)->lpVtbl -> get_Attributes(This,plAttributes)

#define Property_put_Attributes(This,plAttributes)	\
    (This)->lpVtbl -> put_Attributes(This,plAttributes)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [propget][id] */ HRESULT __stdcall Property_get_Value_Proxy( 
    Property __RPC_FAR * This,
    /* [retval][out] */ VARIANT __RPC_FAR *pval);


void __RPC_STUB Property_get_Value_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT __stdcall Property_put_Value_Proxy( 
    Property __RPC_FAR * This,
    /* [in] */ VARIANT pval);


void __RPC_STUB Property_put_Value_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT __stdcall Property_get_Name_Proxy( 
    Property __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstr);


void __RPC_STUB Property_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT __stdcall Property_get_Type_Proxy( 
    Property __RPC_FAR * This,
    /* [retval][out] */ DataTypeEnum __RPC_FAR *ptype);


void __RPC_STUB Property_get_Type_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT __stdcall Property_get_Attributes_Proxy( 
    Property __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plAttributes);


void __RPC_STUB Property_get_Attributes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT __stdcall Property_put_Attributes_Proxy( 
    Property __RPC_FAR * This,
    /* [in] */ long plAttributes);


void __RPC_STUB Property_put_Attributes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __Property_INTERFACE_DEFINED__ */


#ifndef __Properties_INTERFACE_DEFINED__
#define __Properties_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: Properties
 * at Tue Nov 09 09:56:36 1999
 * using MIDL 3.02.88
 ****************************************/
/* [unique][helpstring][dual][uuid][object] */ 



EXTERN_C const IID IID_Properties;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("113033ED-F682-11D2-BB62-00C04F680ACC")
    Properties : public _Collection
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ VARIANT varIndex,
            /* [retval][out] */ Property __RPC_FAR *__RPC_FAR *pVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct PropertiesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            Properties __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            Properties __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            Properties __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            Properties __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            Properties __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            Properties __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            Properties __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Count )( 
            Properties __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][restricted] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *_NewEnum )( 
            Properties __RPC_FAR * This,
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppvObject);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Refresh )( 
            Properties __RPC_FAR * This);
        
        /* [propget][hidden] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Element )( 
            Properties __RPC_FAR * This,
            VARIANT __RPC_FAR *pvar,
            int index);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Item )( 
            Properties __RPC_FAR * This,
            /* [in] */ VARIANT varIndex,
            /* [retval][out] */ Property __RPC_FAR *__RPC_FAR *pVal);
        
        END_INTERFACE
    } PropertiesVtbl;

    interface Properties
    {
        CONST_VTBL struct PropertiesVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define Properties_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define Properties_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define Properties_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define Properties_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define Properties_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define Properties_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define Properties_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define Properties_get_Count(This,pVal)	\
    (This)->lpVtbl -> get_Count(This,pVal)

#define Properties__NewEnum(This,ppvObject)	\
    (This)->lpVtbl -> _NewEnum(This,ppvObject)

#define Properties_Refresh(This)	\
    (This)->lpVtbl -> Refresh(This)

#define Properties_get_Element(This,pvar,index)	\
    (This)->lpVtbl -> get_Element(This,pvar,index)


#define Properties_get_Item(This,varIndex,pVal)	\
    (This)->lpVtbl -> get_Item(This,varIndex,pVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE Properties_get_Item_Proxy( 
    Properties __RPC_FAR * This,
    /* [in] */ VARIANT varIndex,
    /* [retval][out] */ Property __RPC_FAR *__RPC_FAR *pVal);


void __RPC_STUB Properties_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __Properties_INTERFACE_DEFINED__ */


#ifndef __Error_INTERFACE_DEFINED__
#define __Error_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: Error
 * at Tue Nov 09 09:56:36 1999
 * using MIDL 3.02.88
 ****************************************/
/* [unique][helpstring][dual][uuid][object] */ 



EXTERN_C const IID IID_Error;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("113033F0-F682-11D2-BB62-00C04F680ACC")
    Error : public IDispatch
    {
    public:
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Description( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstr) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Number( 
            /* [retval][out] */ long __RPC_FAR *pl) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Source( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstr) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_HelpFile( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstr) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_HelpContext( 
            /* [retval][out] */ DWORD __RPC_FAR *pl) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_NativeError( 
            /* [retval][out] */ long __RPC_FAR *pl) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ErrorVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            Error __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            Error __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            Error __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            Error __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            Error __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            Error __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            Error __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Description )( 
            Error __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstr);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Number )( 
            Error __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pl);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Source )( 
            Error __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstr);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_HelpFile )( 
            Error __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstr);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_HelpContext )( 
            Error __RPC_FAR * This,
            /* [retval][out] */ DWORD __RPC_FAR *pl);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_NativeError )( 
            Error __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pl);
        
        END_INTERFACE
    } ErrorVtbl;

    interface Error
    {
        CONST_VTBL struct ErrorVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define Error_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define Error_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define Error_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define Error_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define Error_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define Error_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define Error_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define Error_get_Description(This,pbstr)	\
    (This)->lpVtbl -> get_Description(This,pbstr)

#define Error_get_Number(This,pl)	\
    (This)->lpVtbl -> get_Number(This,pl)

#define Error_get_Source(This,pbstr)	\
    (This)->lpVtbl -> get_Source(This,pbstr)

#define Error_get_HelpFile(This,pbstr)	\
    (This)->lpVtbl -> get_HelpFile(This,pbstr)

#define Error_get_HelpContext(This,pl)	\
    (This)->lpVtbl -> get_HelpContext(This,pl)

#define Error_get_NativeError(This,pl)	\
    (This)->lpVtbl -> get_NativeError(This,pl)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [propget][id] */ HRESULT STDMETHODCALLTYPE Error_get_Description_Proxy( 
    Error __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstr);


void __RPC_STUB Error_get_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE Error_get_Number_Proxy( 
    Error __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pl);


void __RPC_STUB Error_get_Number_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE Error_get_Source_Proxy( 
    Error __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstr);


void __RPC_STUB Error_get_Source_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE Error_get_HelpFile_Proxy( 
    Error __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstr);


void __RPC_STUB Error_get_HelpFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE Error_get_HelpContext_Proxy( 
    Error __RPC_FAR * This,
    /* [retval][out] */ DWORD __RPC_FAR *pl);


void __RPC_STUB Error_get_HelpContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE Error_get_NativeError_Proxy( 
    Error __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pl);


void __RPC_STUB Error_get_NativeError_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __Error_INTERFACE_DEFINED__ */


#ifndef __Errors_INTERFACE_DEFINED__
#define __Errors_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: Errors
 * at Tue Nov 09 09:56:36 1999
 * using MIDL 3.02.88
 ****************************************/
/* [unique][helpstring][dual][uuid][object] */ 



EXTERN_C const IID IID_Errors;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("113033EF-F682-11D2-BB62-00C04F680ACC")
    Errors : public _Collection
    {
    public:
        virtual /* [propget][id] */ HRESULT __stdcall get_Item( 
            /* [in] */ VARIANT Index,
            /* [retval][out] */ Error __RPC_FAR *__RPC_FAR *ppvObject) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Clear( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ErrorsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            Errors __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            Errors __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            Errors __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            Errors __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            Errors __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            Errors __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            Errors __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Count )( 
            Errors __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][restricted] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *_NewEnum )( 
            Errors __RPC_FAR * This,
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppvObject);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Refresh )( 
            Errors __RPC_FAR * This);
        
        /* [propget][hidden] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Element )( 
            Errors __RPC_FAR * This,
            VARIANT __RPC_FAR *pvar,
            int index);
        
        /* [propget][id] */ HRESULT ( __stdcall __RPC_FAR *get_Item )( 
            Errors __RPC_FAR * This,
            /* [in] */ VARIANT Index,
            /* [retval][out] */ Error __RPC_FAR *__RPC_FAR *ppvObject);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clear )( 
            Errors __RPC_FAR * This);
        
        END_INTERFACE
    } ErrorsVtbl;

    interface Errors
    {
        CONST_VTBL struct ErrorsVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define Errors_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define Errors_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define Errors_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define Errors_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define Errors_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define Errors_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define Errors_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define Errors_get_Count(This,pVal)	\
    (This)->lpVtbl -> get_Count(This,pVal)

#define Errors__NewEnum(This,ppvObject)	\
    (This)->lpVtbl -> _NewEnum(This,ppvObject)

#define Errors_Refresh(This)	\
    (This)->lpVtbl -> Refresh(This)

#define Errors_get_Element(This,pvar,index)	\
    (This)->lpVtbl -> get_Element(This,pvar,index)


#define Errors_get_Item(This,Index,ppvObject)	\
    (This)->lpVtbl -> get_Item(This,Index,ppvObject)

#define Errors_Clear(This)	\
    (This)->lpVtbl -> Clear(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [propget][id] */ HRESULT __stdcall Errors_get_Item_Proxy( 
    Errors __RPC_FAR * This,
    /* [in] */ VARIANT Index,
    /* [retval][out] */ Error __RPC_FAR *__RPC_FAR *ppvObject);


void __RPC_STUB Errors_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE Errors_Clear_Proxy( 
    Errors __RPC_FAR * This);


void __RPC_STUB Errors_Clear_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __Errors_INTERFACE_DEFINED__ */


#ifndef ___Recordset_INTERFACE_DEFINED__
#define ___Recordset_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: _Recordset
 * at Tue Nov 09 09:56:36 1999
 * using MIDL 3.02.88
 ****************************************/
/* [unique][helpstring][dual][uuid][object] */ 



EXTERN_C const IID IID__Recordset;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("113033F6-F682-11D2-BB62-00C04F680ACC")
    _Recordset : public IDispatch
    {
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE AddNew( 
            /* [optional][in] */ VARIANT FieldList,
            /* [optional][in] */ VARIANT Values) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE CancelUpdate( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Clone( 
            /* [defaultvalue][optional][in] */ LockTypeEnum LockType,
            /* [out][retval] */ _Recordset __RPC_FAR *__RPC_FAR *ppvObject) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Close( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Delete( 
            /* [defaultvalue][in] */ long AffectRecords) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetRows( 
            /* [defaultvalue][in] */ long Rows,
            /* [optional][in] */ VARIANT Start,
            /* [optional][in] */ VARIANT Fields,
            /* [out][retval] */ VARIANT __RPC_FAR *pvar) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Move( 
            /* [in] */ long NumRecords,
            /* [optional][in] */ VARIANT Start) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE MoveFirst( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE MoveLast( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE MoveNext( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE MovePrevious( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Open( 
            /* [optional][in] */ VARIANT Source,
            /* [optional][in] */ VARIANT ActiveConnection,
            /* [defaultvalue][in] */ CursorTypeEnum CursorType,
            /* [defaultvalue][in] */ LockTypeEnum LockType,
            /* [defaultvalue][in] */ long Options) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Requery( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Supports( 
            /* [in] */ CursorOptionEnum CursorOptions,
            /* [out][retval] */ VARIANT_BOOL __RPC_FAR *pb) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Update( 
            /* [optional][in] */ VARIANT Fields,
            /* [optional][in] */ VARIANT Values) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_AbsolutePage( 
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_AbsolutePage( 
            /* [in] */ long newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_AbsolutePosition( 
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_AbsolutePosition( 
            /* [in] */ long newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_BOF( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Bookmark( 
            /* [retval][out] */ VARIANT __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Bookmark( 
            /* [in] */ VARIANT newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_CacheSize( 
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_CacheSize( 
            /* [in] */ long newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_CursorType( 
            /* [retval][out] */ CursorTypeEnum __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_CursorType( 
            /* [in] */ CursorTypeEnum newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_EditMode( 
            /* [retval][out] */ EditModeEnum __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_EOF( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Fields( 
            /* [retval][out] */ Fields __RPC_FAR *__RPC_FAR *pvObject) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_LockType( 
            /* [retval][out] */ LockTypeEnum __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_LockType( 
            /* [in] */ LockTypeEnum newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_PageCount( 
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_PageSize( 
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_PageSize( 
            /* [in] */ long newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_RecordCount( 
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Source( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Source( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [id][propputref] */ HRESULT STDMETHODCALLTYPE putref_ActiveConnection( 
            /* [in] */ VARIANT newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_ActiveConnection( 
            /* [retval][out] */ VARIANT __RPC_FAR *pConn) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_ActiveConnection( 
            /* [in] */ VARIANT newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_ErrorDescription( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Find( 
            /* [in] */ BSTR Criteria,
            /* [defaultvalue][optional][in] */ long SkipRecords,
            /* [defaultvalue][optional][in] */ SearchDirectionEnum SearchDirection,
            /* [optional][in] */ VARIANT Start) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Seek( 
            /* [in] */ VARIANT KeyValues,
            /* [defaultvalue][optional][in] */ SeekEnum SeekOption) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Index( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstrIndex) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Index( 
            /* [in] */ BSTR pbstrIndex) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Filter( 
            /* [retval][out] */ VARIANT __RPC_FAR *Criteria) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Filter( 
            /* [in] */ VARIANT Criteria) = 0;
        
        virtual /* [id][propget] */ HRESULT __stdcall get_Properties( 
            /* [retval][out] */ Properties __RPC_FAR *__RPC_FAR *ppProperties) = 0;
        
        virtual /* [id][propget] */ HRESULT __stdcall get_State( 
            /* [retval][out] */ long __RPC_FAR *plObjState) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct _RecordsetVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            _Recordset __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            _Recordset __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            _Recordset __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            _Recordset __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            _Recordset __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            _Recordset __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            _Recordset __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddNew )( 
            _Recordset __RPC_FAR * This,
            /* [optional][in] */ VARIANT FieldList,
            /* [optional][in] */ VARIANT Values);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CancelUpdate )( 
            _Recordset __RPC_FAR * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            _Recordset __RPC_FAR * This,
            /* [defaultvalue][optional][in] */ LockTypeEnum LockType,
            /* [out][retval] */ _Recordset __RPC_FAR *__RPC_FAR *ppvObject);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Close )( 
            _Recordset __RPC_FAR * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Delete )( 
            _Recordset __RPC_FAR * This,
            /* [defaultvalue][in] */ long AffectRecords);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetRows )( 
            _Recordset __RPC_FAR * This,
            /* [defaultvalue][in] */ long Rows,
            /* [optional][in] */ VARIANT Start,
            /* [optional][in] */ VARIANT Fields,
            /* [out][retval] */ VARIANT __RPC_FAR *pvar);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Move )( 
            _Recordset __RPC_FAR * This,
            /* [in] */ long NumRecords,
            /* [optional][in] */ VARIANT Start);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *MoveFirst )( 
            _Recordset __RPC_FAR * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *MoveLast )( 
            _Recordset __RPC_FAR * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *MoveNext )( 
            _Recordset __RPC_FAR * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *MovePrevious )( 
            _Recordset __RPC_FAR * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Open )( 
            _Recordset __RPC_FAR * This,
            /* [optional][in] */ VARIANT Source,
            /* [optional][in] */ VARIANT ActiveConnection,
            /* [defaultvalue][in] */ CursorTypeEnum CursorType,
            /* [defaultvalue][in] */ LockTypeEnum LockType,
            /* [defaultvalue][in] */ long Options);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Requery )( 
            _Recordset __RPC_FAR * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Supports )( 
            _Recordset __RPC_FAR * This,
            /* [in] */ CursorOptionEnum CursorOptions,
            /* [out][retval] */ VARIANT_BOOL __RPC_FAR *pb);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Update )( 
            _Recordset __RPC_FAR * This,
            /* [optional][in] */ VARIANT Fields,
            /* [optional][in] */ VARIANT Values);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_AbsolutePage )( 
            _Recordset __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_AbsolutePage )( 
            _Recordset __RPC_FAR * This,
            /* [in] */ long newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_AbsolutePosition )( 
            _Recordset __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_AbsolutePosition )( 
            _Recordset __RPC_FAR * This,
            /* [in] */ long newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_BOF )( 
            _Recordset __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Bookmark )( 
            _Recordset __RPC_FAR * This,
            /* [retval][out] */ VARIANT __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Bookmark )( 
            _Recordset __RPC_FAR * This,
            /* [in] */ VARIANT newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CacheSize )( 
            _Recordset __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_CacheSize )( 
            _Recordset __RPC_FAR * This,
            /* [in] */ long newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CursorType )( 
            _Recordset __RPC_FAR * This,
            /* [retval][out] */ CursorTypeEnum __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_CursorType )( 
            _Recordset __RPC_FAR * This,
            /* [in] */ CursorTypeEnum newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_EditMode )( 
            _Recordset __RPC_FAR * This,
            /* [retval][out] */ EditModeEnum __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_EOF )( 
            _Recordset __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Fields )( 
            _Recordset __RPC_FAR * This,
            /* [retval][out] */ Fields __RPC_FAR *__RPC_FAR *pvObject);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_LockType )( 
            _Recordset __RPC_FAR * This,
            /* [retval][out] */ LockTypeEnum __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_LockType )( 
            _Recordset __RPC_FAR * This,
            /* [in] */ LockTypeEnum newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_PageCount )( 
            _Recordset __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_PageSize )( 
            _Recordset __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_PageSize )( 
            _Recordset __RPC_FAR * This,
            /* [in] */ long newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_RecordCount )( 
            _Recordset __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Source )( 
            _Recordset __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Source )( 
            _Recordset __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [id][propputref] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *putref_ActiveConnection )( 
            _Recordset __RPC_FAR * This,
            /* [in] */ VARIANT newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ActiveConnection )( 
            _Recordset __RPC_FAR * This,
            /* [retval][out] */ VARIANT __RPC_FAR *pConn);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ActiveConnection )( 
            _Recordset __RPC_FAR * This,
            /* [in] */ VARIANT newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ErrorDescription )( 
            _Recordset __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Find )( 
            _Recordset __RPC_FAR * This,
            /* [in] */ BSTR Criteria,
            /* [defaultvalue][optional][in] */ long SkipRecords,
            /* [defaultvalue][optional][in] */ SearchDirectionEnum SearchDirection,
            /* [optional][in] */ VARIANT Start);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Seek )( 
            _Recordset __RPC_FAR * This,
            /* [in] */ VARIANT KeyValues,
            /* [defaultvalue][optional][in] */ SeekEnum SeekOption);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Index )( 
            _Recordset __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrIndex);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Index )( 
            _Recordset __RPC_FAR * This,
            /* [in] */ BSTR pbstrIndex);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Filter )( 
            _Recordset __RPC_FAR * This,
            /* [retval][out] */ VARIANT __RPC_FAR *Criteria);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Filter )( 
            _Recordset __RPC_FAR * This,
            /* [in] */ VARIANT Criteria);
        
        /* [id][propget] */ HRESULT ( __stdcall __RPC_FAR *get_Properties )( 
            _Recordset __RPC_FAR * This,
            /* [retval][out] */ Properties __RPC_FAR *__RPC_FAR *ppProperties);
        
        /* [id][propget] */ HRESULT ( __stdcall __RPC_FAR *get_State )( 
            _Recordset __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plObjState);
        
        END_INTERFACE
    } _RecordsetVtbl;

    interface _Recordset
    {
        CONST_VTBL struct _RecordsetVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define _Recordset_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define _Recordset_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define _Recordset_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define _Recordset_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define _Recordset_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define _Recordset_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define _Recordset_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define _Recordset_AddNew(This,FieldList,Values)	\
    (This)->lpVtbl -> AddNew(This,FieldList,Values)

#define _Recordset_CancelUpdate(This)	\
    (This)->lpVtbl -> CancelUpdate(This)

#define _Recordset_Clone(This,LockType,ppvObject)	\
    (This)->lpVtbl -> Clone(This,LockType,ppvObject)

#define _Recordset_Close(This)	\
    (This)->lpVtbl -> Close(This)

#define _Recordset_Delete(This,AffectRecords)	\
    (This)->lpVtbl -> Delete(This,AffectRecords)

#define _Recordset_GetRows(This,Rows,Start,Fields,pvar)	\
    (This)->lpVtbl -> GetRows(This,Rows,Start,Fields,pvar)

#define _Recordset_Move(This,NumRecords,Start)	\
    (This)->lpVtbl -> Move(This,NumRecords,Start)

#define _Recordset_MoveFirst(This)	\
    (This)->lpVtbl -> MoveFirst(This)

#define _Recordset_MoveLast(This)	\
    (This)->lpVtbl -> MoveLast(This)

#define _Recordset_MoveNext(This)	\
    (This)->lpVtbl -> MoveNext(This)

#define _Recordset_MovePrevious(This)	\
    (This)->lpVtbl -> MovePrevious(This)

#define _Recordset_Open(This,Source,ActiveConnection,CursorType,LockType,Options)	\
    (This)->lpVtbl -> Open(This,Source,ActiveConnection,CursorType,LockType,Options)

#define _Recordset_Requery(This)	\
    (This)->lpVtbl -> Requery(This)

#define _Recordset_Supports(This,CursorOptions,pb)	\
    (This)->lpVtbl -> Supports(This,CursorOptions,pb)

#define _Recordset_Update(This,Fields,Values)	\
    (This)->lpVtbl -> Update(This,Fields,Values)

#define _Recordset_get_AbsolutePage(This,pVal)	\
    (This)->lpVtbl -> get_AbsolutePage(This,pVal)

#define _Recordset_put_AbsolutePage(This,newVal)	\
    (This)->lpVtbl -> put_AbsolutePage(This,newVal)

#define _Recordset_get_AbsolutePosition(This,pVal)	\
    (This)->lpVtbl -> get_AbsolutePosition(This,pVal)

#define _Recordset_put_AbsolutePosition(This,newVal)	\
    (This)->lpVtbl -> put_AbsolutePosition(This,newVal)

#define _Recordset_get_BOF(This,pVal)	\
    (This)->lpVtbl -> get_BOF(This,pVal)

#define _Recordset_get_Bookmark(This,pVal)	\
    (This)->lpVtbl -> get_Bookmark(This,pVal)

#define _Recordset_put_Bookmark(This,newVal)	\
    (This)->lpVtbl -> put_Bookmark(This,newVal)

#define _Recordset_get_CacheSize(This,pVal)	\
    (This)->lpVtbl -> get_CacheSize(This,pVal)

#define _Recordset_put_CacheSize(This,newVal)	\
    (This)->lpVtbl -> put_CacheSize(This,newVal)

#define _Recordset_get_CursorType(This,pVal)	\
    (This)->lpVtbl -> get_CursorType(This,pVal)

#define _Recordset_put_CursorType(This,newVal)	\
    (This)->lpVtbl -> put_CursorType(This,newVal)

#define _Recordset_get_EditMode(This,pVal)	\
    (This)->lpVtbl -> get_EditMode(This,pVal)

#define _Recordset_get_EOF(This,pVal)	\
    (This)->lpVtbl -> get_EOF(This,pVal)

#define _Recordset_get_Fields(This,pvObject)	\
    (This)->lpVtbl -> get_Fields(This,pvObject)

#define _Recordset_get_LockType(This,pVal)	\
    (This)->lpVtbl -> get_LockType(This,pVal)

#define _Recordset_put_LockType(This,newVal)	\
    (This)->lpVtbl -> put_LockType(This,newVal)

#define _Recordset_get_PageCount(This,pVal)	\
    (This)->lpVtbl -> get_PageCount(This,pVal)

#define _Recordset_get_PageSize(This,pVal)	\
    (This)->lpVtbl -> get_PageSize(This,pVal)

#define _Recordset_put_PageSize(This,newVal)	\
    (This)->lpVtbl -> put_PageSize(This,newVal)

#define _Recordset_get_RecordCount(This,pVal)	\
    (This)->lpVtbl -> get_RecordCount(This,pVal)

#define _Recordset_get_Source(This,pVal)	\
    (This)->lpVtbl -> get_Source(This,pVal)

#define _Recordset_put_Source(This,newVal)	\
    (This)->lpVtbl -> put_Source(This,newVal)

#define _Recordset_putref_ActiveConnection(This,newVal)	\
    (This)->lpVtbl -> putref_ActiveConnection(This,newVal)

#define _Recordset_get_ActiveConnection(This,pConn)	\
    (This)->lpVtbl -> get_ActiveConnection(This,pConn)

#define _Recordset_put_ActiveConnection(This,newVal)	\
    (This)->lpVtbl -> put_ActiveConnection(This,newVal)

#define _Recordset_get_ErrorDescription(This,pVal)	\
    (This)->lpVtbl -> get_ErrorDescription(This,pVal)

#define _Recordset_Find(This,Criteria,SkipRecords,SearchDirection,Start)	\
    (This)->lpVtbl -> Find(This,Criteria,SkipRecords,SearchDirection,Start)

#define _Recordset_Seek(This,KeyValues,SeekOption)	\
    (This)->lpVtbl -> Seek(This,KeyValues,SeekOption)

#define _Recordset_get_Index(This,pbstrIndex)	\
    (This)->lpVtbl -> get_Index(This,pbstrIndex)

#define _Recordset_put_Index(This,pbstrIndex)	\
    (This)->lpVtbl -> put_Index(This,pbstrIndex)

#define _Recordset_get_Filter(This,Criteria)	\
    (This)->lpVtbl -> get_Filter(This,Criteria)

#define _Recordset_put_Filter(This,Criteria)	\
    (This)->lpVtbl -> put_Filter(This,Criteria)

#define _Recordset_get_Properties(This,ppProperties)	\
    (This)->lpVtbl -> get_Properties(This,ppProperties)

#define _Recordset_get_State(This,plObjState)	\
    (This)->lpVtbl -> get_State(This,plObjState)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id] */ HRESULT STDMETHODCALLTYPE _Recordset_AddNew_Proxy( 
    _Recordset __RPC_FAR * This,
    /* [optional][in] */ VARIANT FieldList,
    /* [optional][in] */ VARIANT Values);


void __RPC_STUB _Recordset_AddNew_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE _Recordset_CancelUpdate_Proxy( 
    _Recordset __RPC_FAR * This);


void __RPC_STUB _Recordset_CancelUpdate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE _Recordset_Clone_Proxy( 
    _Recordset __RPC_FAR * This,
    /* [defaultvalue][optional][in] */ LockTypeEnum LockType,
    /* [out][retval] */ _Recordset __RPC_FAR *__RPC_FAR *ppvObject);


void __RPC_STUB _Recordset_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE _Recordset_Close_Proxy( 
    _Recordset __RPC_FAR * This);


void __RPC_STUB _Recordset_Close_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE _Recordset_Delete_Proxy( 
    _Recordset __RPC_FAR * This,
    /* [defaultvalue][in] */ long AffectRecords);


void __RPC_STUB _Recordset_Delete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE _Recordset_GetRows_Proxy( 
    _Recordset __RPC_FAR * This,
    /* [defaultvalue][in] */ long Rows,
    /* [optional][in] */ VARIANT Start,
    /* [optional][in] */ VARIANT Fields,
    /* [out][retval] */ VARIANT __RPC_FAR *pvar);


void __RPC_STUB _Recordset_GetRows_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE _Recordset_Move_Proxy( 
    _Recordset __RPC_FAR * This,
    /* [in] */ long NumRecords,
    /* [optional][in] */ VARIANT Start);


void __RPC_STUB _Recordset_Move_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE _Recordset_MoveFirst_Proxy( 
    _Recordset __RPC_FAR * This);


void __RPC_STUB _Recordset_MoveFirst_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE _Recordset_MoveLast_Proxy( 
    _Recordset __RPC_FAR * This);


void __RPC_STUB _Recordset_MoveLast_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE _Recordset_MoveNext_Proxy( 
    _Recordset __RPC_FAR * This);


void __RPC_STUB _Recordset_MoveNext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE _Recordset_MovePrevious_Proxy( 
    _Recordset __RPC_FAR * This);


void __RPC_STUB _Recordset_MovePrevious_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE _Recordset_Open_Proxy( 
    _Recordset __RPC_FAR * This,
    /* [optional][in] */ VARIANT Source,
    /* [optional][in] */ VARIANT ActiveConnection,
    /* [defaultvalue][in] */ CursorTypeEnum CursorType,
    /* [defaultvalue][in] */ LockTypeEnum LockType,
    /* [defaultvalue][in] */ long Options);


void __RPC_STUB _Recordset_Open_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE _Recordset_Requery_Proxy( 
    _Recordset __RPC_FAR * This);


void __RPC_STUB _Recordset_Requery_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE _Recordset_Supports_Proxy( 
    _Recordset __RPC_FAR * This,
    /* [in] */ CursorOptionEnum CursorOptions,
    /* [out][retval] */ VARIANT_BOOL __RPC_FAR *pb);


void __RPC_STUB _Recordset_Supports_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE _Recordset_Update_Proxy( 
    _Recordset __RPC_FAR * This,
    /* [optional][in] */ VARIANT Fields,
    /* [optional][in] */ VARIANT Values);


void __RPC_STUB _Recordset_Update_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE _Recordset_get_AbsolutePage_Proxy( 
    _Recordset __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB _Recordset_get_AbsolutePage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE _Recordset_put_AbsolutePage_Proxy( 
    _Recordset __RPC_FAR * This,
    /* [in] */ long newVal);


void __RPC_STUB _Recordset_put_AbsolutePage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE _Recordset_get_AbsolutePosition_Proxy( 
    _Recordset __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB _Recordset_get_AbsolutePosition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE _Recordset_put_AbsolutePosition_Proxy( 
    _Recordset __RPC_FAR * This,
    /* [in] */ long newVal);


void __RPC_STUB _Recordset_put_AbsolutePosition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE _Recordset_get_BOF_Proxy( 
    _Recordset __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pVal);


void __RPC_STUB _Recordset_get_BOF_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE _Recordset_get_Bookmark_Proxy( 
    _Recordset __RPC_FAR * This,
    /* [retval][out] */ VARIANT __RPC_FAR *pVal);


void __RPC_STUB _Recordset_get_Bookmark_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE _Recordset_put_Bookmark_Proxy( 
    _Recordset __RPC_FAR * This,
    /* [in] */ VARIANT newVal);


void __RPC_STUB _Recordset_put_Bookmark_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE _Recordset_get_CacheSize_Proxy( 
    _Recordset __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB _Recordset_get_CacheSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE _Recordset_put_CacheSize_Proxy( 
    _Recordset __RPC_FAR * This,
    /* [in] */ long newVal);


void __RPC_STUB _Recordset_put_CacheSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE _Recordset_get_CursorType_Proxy( 
    _Recordset __RPC_FAR * This,
    /* [retval][out] */ CursorTypeEnum __RPC_FAR *pVal);


void __RPC_STUB _Recordset_get_CursorType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE _Recordset_put_CursorType_Proxy( 
    _Recordset __RPC_FAR * This,
    /* [in] */ CursorTypeEnum newVal);


void __RPC_STUB _Recordset_put_CursorType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE _Recordset_get_EditMode_Proxy( 
    _Recordset __RPC_FAR * This,
    /* [retval][out] */ EditModeEnum __RPC_FAR *pVal);


void __RPC_STUB _Recordset_get_EditMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE _Recordset_get_EOF_Proxy( 
    _Recordset __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pVal);


void __RPC_STUB _Recordset_get_EOF_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE _Recordset_get_Fields_Proxy( 
    _Recordset __RPC_FAR * This,
    /* [retval][out] */ Fields __RPC_FAR *__RPC_FAR *pvObject);


void __RPC_STUB _Recordset_get_Fields_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE _Recordset_get_LockType_Proxy( 
    _Recordset __RPC_FAR * This,
    /* [retval][out] */ LockTypeEnum __RPC_FAR *pVal);


void __RPC_STUB _Recordset_get_LockType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE _Recordset_put_LockType_Proxy( 
    _Recordset __RPC_FAR * This,
    /* [in] */ LockTypeEnum newVal);


void __RPC_STUB _Recordset_put_LockType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE _Recordset_get_PageCount_Proxy( 
    _Recordset __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB _Recordset_get_PageCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE _Recordset_get_PageSize_Proxy( 
    _Recordset __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB _Recordset_get_PageSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE _Recordset_put_PageSize_Proxy( 
    _Recordset __RPC_FAR * This,
    /* [in] */ long newVal);


void __RPC_STUB _Recordset_put_PageSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE _Recordset_get_RecordCount_Proxy( 
    _Recordset __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB _Recordset_get_RecordCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE _Recordset_get_Source_Proxy( 
    _Recordset __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB _Recordset_get_Source_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE _Recordset_put_Source_Proxy( 
    _Recordset __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB _Recordset_put_Source_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propputref] */ HRESULT STDMETHODCALLTYPE _Recordset_putref_ActiveConnection_Proxy( 
    _Recordset __RPC_FAR * This,
    /* [in] */ VARIANT newVal);


void __RPC_STUB _Recordset_putref_ActiveConnection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE _Recordset_get_ActiveConnection_Proxy( 
    _Recordset __RPC_FAR * This,
    /* [retval][out] */ VARIANT __RPC_FAR *pConn);


void __RPC_STUB _Recordset_get_ActiveConnection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE _Recordset_put_ActiveConnection_Proxy( 
    _Recordset __RPC_FAR * This,
    /* [in] */ VARIANT newVal);


void __RPC_STUB _Recordset_put_ActiveConnection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE _Recordset_get_ErrorDescription_Proxy( 
    _Recordset __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB _Recordset_get_ErrorDescription_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE _Recordset_Find_Proxy( 
    _Recordset __RPC_FAR * This,
    /* [in] */ BSTR Criteria,
    /* [defaultvalue][optional][in] */ long SkipRecords,
    /* [defaultvalue][optional][in] */ SearchDirectionEnum SearchDirection,
    /* [optional][in] */ VARIANT Start);


void __RPC_STUB _Recordset_Find_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE _Recordset_Seek_Proxy( 
    _Recordset __RPC_FAR * This,
    /* [in] */ VARIANT KeyValues,
    /* [defaultvalue][optional][in] */ SeekEnum SeekOption);


void __RPC_STUB _Recordset_Seek_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE _Recordset_get_Index_Proxy( 
    _Recordset __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstrIndex);


void __RPC_STUB _Recordset_get_Index_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE _Recordset_put_Index_Proxy( 
    _Recordset __RPC_FAR * This,
    /* [in] */ BSTR pbstrIndex);


void __RPC_STUB _Recordset_put_Index_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE _Recordset_get_Filter_Proxy( 
    _Recordset __RPC_FAR * This,
    /* [retval][out] */ VARIANT __RPC_FAR *Criteria);


void __RPC_STUB _Recordset_get_Filter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE _Recordset_put_Filter_Proxy( 
    _Recordset __RPC_FAR * This,
    /* [in] */ VARIANT Criteria);


void __RPC_STUB _Recordset_put_Filter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT __stdcall _Recordset_get_Properties_Proxy( 
    _Recordset __RPC_FAR * This,
    /* [retval][out] */ Properties __RPC_FAR *__RPC_FAR *ppProperties);


void __RPC_STUB _Recordset_get_Properties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT __stdcall _Recordset_get_State_Proxy( 
    _Recordset __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plObjState);


void __RPC_STUB _Recordset_get_State_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* ___Recordset_INTERFACE_DEFINED__ */


#ifndef ___Connection_INTERFACE_DEFINED__
#define ___Connection_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: _Connection
 * at Tue Nov 09 09:56:36 1999
 * using MIDL 3.02.88
 ****************************************/
/* [unique][helpstring][dual][uuid][object] */ 



EXTERN_C const IID IID__Connection;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("113033DE-F682-11D2-BB62-00C04F680ACC")
    _Connection : public IDispatch
    {
    public:
        virtual /* [propget][id] */ HRESULT __stdcall get_ConnectionString( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstr) = 0;
        
        virtual /* [propput][id] */ HRESULT __stdcall put_ConnectionString( 
            /* [in] */ BSTR pbstr) = 0;
        
        virtual /* [propget][id] */ HRESULT __stdcall get_Version( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstr) = 0;
        
        virtual /* [id] */ HRESULT __stdcall Close( void) = 0;
        
        virtual /* [id] */ HRESULT __stdcall Execute( 
            /* [in] */ BSTR CommandText,
            /* [optional][out] */ VARIANT __RPC_FAR *RecordsAffected,
            /* [defaultvalue][optional][in] */ long Options,
            /* [retval][out] */ _Recordset __RPC_FAR *__RPC_FAR *ppiRset) = 0;
        
        virtual /* [id] */ HRESULT __stdcall BeginTrans( 
            /* [retval][out] */ long __RPC_FAR *TransactionLevel) = 0;
        
        virtual /* [id] */ HRESULT __stdcall CommitTrans( void) = 0;
        
        virtual /* [id] */ HRESULT __stdcall RollbackTrans( void) = 0;
        
        virtual /* [id] */ HRESULT __stdcall Open( 
            /* [defaultvalue][optional][in] */ BSTR ConnectionString,
            /* [defaultvalue][optional][in] */ BSTR UserID,
            /* [defaultvalue][optional][in] */ BSTR Password,
            /* [defaultvalue][optional][in] */ long Options) = 0;
        
        virtual /* [propget][id] */ HRESULT __stdcall get_Errors( 
            /* [retval][out] */ Errors __RPC_FAR *__RPC_FAR *ppErrors) = 0;
        
        virtual /* [propget][id] */ HRESULT __stdcall get_DefaultDatabase( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstr) = 0;
        
        virtual /* [propput][id] */ HRESULT __stdcall put_DefaultDatabase( 
            /* [in] */ BSTR pbstr) = 0;
        
        virtual /* [propget][id] */ HRESULT __stdcall get_IsolationLevel( 
            /* [retval][out] */ IsolationLevelEnum __RPC_FAR *Level) = 0;
        
        virtual /* [propput][id] */ HRESULT __stdcall put_IsolationLevel( 
            /* [in] */ IsolationLevelEnum Level) = 0;
        
        virtual /* [propget][id] */ HRESULT __stdcall get_Attributes( 
            /* [retval][out] */ long __RPC_FAR *plAttr) = 0;
        
        virtual /* [propput][id] */ HRESULT __stdcall put_Attributes( 
            /* [in] */ long plAttr) = 0;
        
        virtual /* [propget][id] */ HRESULT __stdcall get_CursorLocation( 
            /* [retval][out] */ CursorLocationEnum __RPC_FAR *plCursorLoc) = 0;
        
        virtual /* [propput][id] */ HRESULT __stdcall put_CursorLocation( 
            /* [in] */ CursorLocationEnum plCursorLoc) = 0;
        
        virtual /* [propget][id] */ HRESULT __stdcall get_Mode( 
            /* [retval][out] */ ConnectModeEnum __RPC_FAR *plMode) = 0;
        
        virtual /* [propput][id] */ HRESULT __stdcall put_Mode( 
            /* [in] */ ConnectModeEnum plMode) = 0;
        
        virtual /* [propget][id] */ HRESULT __stdcall get_Provider( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstr) = 0;
        
        virtual /* [propput][id] */ HRESULT __stdcall put_Provider( 
            /* [in] */ BSTR pbstr) = 0;
        
        virtual /* [propget][id] */ HRESULT __stdcall get_State( 
            /* [retval][out] */ long __RPC_FAR *plObjState) = 0;
        
        virtual /* [propget][id] */ HRESULT __stdcall get_Properties( 
            /* [retval][out] */ Properties __RPC_FAR *__RPC_FAR *ppProperties) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct _ConnectionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            _Connection __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            _Connection __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            _Connection __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            _Connection __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            _Connection __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            _Connection __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            _Connection __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [propget][id] */ HRESULT ( __stdcall __RPC_FAR *get_ConnectionString )( 
            _Connection __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstr);
        
        /* [propput][id] */ HRESULT ( __stdcall __RPC_FAR *put_ConnectionString )( 
            _Connection __RPC_FAR * This,
            /* [in] */ BSTR pbstr);
        
        /* [propget][id] */ HRESULT ( __stdcall __RPC_FAR *get_Version )( 
            _Connection __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstr);
        
        /* [id] */ HRESULT ( __stdcall __RPC_FAR *Close )( 
            _Connection __RPC_FAR * This);
        
        /* [id] */ HRESULT ( __stdcall __RPC_FAR *Execute )( 
            _Connection __RPC_FAR * This,
            /* [in] */ BSTR CommandText,
            /* [optional][out] */ VARIANT __RPC_FAR *RecordsAffected,
            /* [defaultvalue][optional][in] */ long Options,
            /* [retval][out] */ _Recordset __RPC_FAR *__RPC_FAR *ppiRset);
        
        /* [id] */ HRESULT ( __stdcall __RPC_FAR *BeginTrans )( 
            _Connection __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *TransactionLevel);
        
        /* [id] */ HRESULT ( __stdcall __RPC_FAR *CommitTrans )( 
            _Connection __RPC_FAR * This);
        
        /* [id] */ HRESULT ( __stdcall __RPC_FAR *RollbackTrans )( 
            _Connection __RPC_FAR * This);
        
        /* [id] */ HRESULT ( __stdcall __RPC_FAR *Open )( 
            _Connection __RPC_FAR * This,
            /* [defaultvalue][optional][in] */ BSTR ConnectionString,
            /* [defaultvalue][optional][in] */ BSTR UserID,
            /* [defaultvalue][optional][in] */ BSTR Password,
            /* [defaultvalue][optional][in] */ long Options);
        
        /* [propget][id] */ HRESULT ( __stdcall __RPC_FAR *get_Errors )( 
            _Connection __RPC_FAR * This,
            /* [retval][out] */ Errors __RPC_FAR *__RPC_FAR *ppErrors);
        
        /* [propget][id] */ HRESULT ( __stdcall __RPC_FAR *get_DefaultDatabase )( 
            _Connection __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstr);
        
        /* [propput][id] */ HRESULT ( __stdcall __RPC_FAR *put_DefaultDatabase )( 
            _Connection __RPC_FAR * This,
            /* [in] */ BSTR pbstr);
        
        /* [propget][id] */ HRESULT ( __stdcall __RPC_FAR *get_IsolationLevel )( 
            _Connection __RPC_FAR * This,
            /* [retval][out] */ IsolationLevelEnum __RPC_FAR *Level);
        
        /* [propput][id] */ HRESULT ( __stdcall __RPC_FAR *put_IsolationLevel )( 
            _Connection __RPC_FAR * This,
            /* [in] */ IsolationLevelEnum Level);
        
        /* [propget][id] */ HRESULT ( __stdcall __RPC_FAR *get_Attributes )( 
            _Connection __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plAttr);
        
        /* [propput][id] */ HRESULT ( __stdcall __RPC_FAR *put_Attributes )( 
            _Connection __RPC_FAR * This,
            /* [in] */ long plAttr);
        
        /* [propget][id] */ HRESULT ( __stdcall __RPC_FAR *get_CursorLocation )( 
            _Connection __RPC_FAR * This,
            /* [retval][out] */ CursorLocationEnum __RPC_FAR *plCursorLoc);
        
        /* [propput][id] */ HRESULT ( __stdcall __RPC_FAR *put_CursorLocation )( 
            _Connection __RPC_FAR * This,
            /* [in] */ CursorLocationEnum plCursorLoc);
        
        /* [propget][id] */ HRESULT ( __stdcall __RPC_FAR *get_Mode )( 
            _Connection __RPC_FAR * This,
            /* [retval][out] */ ConnectModeEnum __RPC_FAR *plMode);
        
        /* [propput][id] */ HRESULT ( __stdcall __RPC_FAR *put_Mode )( 
            _Connection __RPC_FAR * This,
            /* [in] */ ConnectModeEnum plMode);
        
        /* [propget][id] */ HRESULT ( __stdcall __RPC_FAR *get_Provider )( 
            _Connection __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstr);
        
        /* [propput][id] */ HRESULT ( __stdcall __RPC_FAR *put_Provider )( 
            _Connection __RPC_FAR * This,
            /* [in] */ BSTR pbstr);
        
        /* [propget][id] */ HRESULT ( __stdcall __RPC_FAR *get_State )( 
            _Connection __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plObjState);
        
        /* [propget][id] */ HRESULT ( __stdcall __RPC_FAR *get_Properties )( 
            _Connection __RPC_FAR * This,
            /* [retval][out] */ Properties __RPC_FAR *__RPC_FAR *ppProperties);
        
        END_INTERFACE
    } _ConnectionVtbl;

    interface _Connection
    {
        CONST_VTBL struct _ConnectionVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define _Connection_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define _Connection_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define _Connection_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define _Connection_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define _Connection_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define _Connection_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define _Connection_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define _Connection_get_ConnectionString(This,pbstr)	\
    (This)->lpVtbl -> get_ConnectionString(This,pbstr)

#define _Connection_put_ConnectionString(This,pbstr)	\
    (This)->lpVtbl -> put_ConnectionString(This,pbstr)

#define _Connection_get_Version(This,pbstr)	\
    (This)->lpVtbl -> get_Version(This,pbstr)

#define _Connection_Close(This)	\
    (This)->lpVtbl -> Close(This)

#define _Connection_Execute(This,CommandText,RecordsAffected,Options,ppiRset)	\
    (This)->lpVtbl -> Execute(This,CommandText,RecordsAffected,Options,ppiRset)

#define _Connection_BeginTrans(This,TransactionLevel)	\
    (This)->lpVtbl -> BeginTrans(This,TransactionLevel)

#define _Connection_CommitTrans(This)	\
    (This)->lpVtbl -> CommitTrans(This)

#define _Connection_RollbackTrans(This)	\
    (This)->lpVtbl -> RollbackTrans(This)

#define _Connection_Open(This,ConnectionString,UserID,Password,Options)	\
    (This)->lpVtbl -> Open(This,ConnectionString,UserID,Password,Options)

#define _Connection_get_Errors(This,ppErrors)	\
    (This)->lpVtbl -> get_Errors(This,ppErrors)

#define _Connection_get_DefaultDatabase(This,pbstr)	\
    (This)->lpVtbl -> get_DefaultDatabase(This,pbstr)

#define _Connection_put_DefaultDatabase(This,pbstr)	\
    (This)->lpVtbl -> put_DefaultDatabase(This,pbstr)

#define _Connection_get_IsolationLevel(This,Level)	\
    (This)->lpVtbl -> get_IsolationLevel(This,Level)

#define _Connection_put_IsolationLevel(This,Level)	\
    (This)->lpVtbl -> put_IsolationLevel(This,Level)

#define _Connection_get_Attributes(This,plAttr)	\
    (This)->lpVtbl -> get_Attributes(This,plAttr)

#define _Connection_put_Attributes(This,plAttr)	\
    (This)->lpVtbl -> put_Attributes(This,plAttr)

#define _Connection_get_CursorLocation(This,plCursorLoc)	\
    (This)->lpVtbl -> get_CursorLocation(This,plCursorLoc)

#define _Connection_put_CursorLocation(This,plCursorLoc)	\
    (This)->lpVtbl -> put_CursorLocation(This,plCursorLoc)

#define _Connection_get_Mode(This,plMode)	\
    (This)->lpVtbl -> get_Mode(This,plMode)

#define _Connection_put_Mode(This,plMode)	\
    (This)->lpVtbl -> put_Mode(This,plMode)

#define _Connection_get_Provider(This,pbstr)	\
    (This)->lpVtbl -> get_Provider(This,pbstr)

#define _Connection_put_Provider(This,pbstr)	\
    (This)->lpVtbl -> put_Provider(This,pbstr)

#define _Connection_get_State(This,plObjState)	\
    (This)->lpVtbl -> get_State(This,plObjState)

#define _Connection_get_Properties(This,ppProperties)	\
    (This)->lpVtbl -> get_Properties(This,ppProperties)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [propget][id] */ HRESULT __stdcall _Connection_get_ConnectionString_Proxy( 
    _Connection __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstr);


void __RPC_STUB _Connection_get_ConnectionString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT __stdcall _Connection_put_ConnectionString_Proxy( 
    _Connection __RPC_FAR * This,
    /* [in] */ BSTR pbstr);


void __RPC_STUB _Connection_put_ConnectionString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT __stdcall _Connection_get_Version_Proxy( 
    _Connection __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstr);


void __RPC_STUB _Connection_get_Version_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT __stdcall _Connection_Close_Proxy( 
    _Connection __RPC_FAR * This);


void __RPC_STUB _Connection_Close_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT __stdcall _Connection_Execute_Proxy( 
    _Connection __RPC_FAR * This,
    /* [in] */ BSTR CommandText,
    /* [optional][out] */ VARIANT __RPC_FAR *RecordsAffected,
    /* [defaultvalue][optional][in] */ long Options,
    /* [retval][out] */ _Recordset __RPC_FAR *__RPC_FAR *ppiRset);


void __RPC_STUB _Connection_Execute_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT __stdcall _Connection_BeginTrans_Proxy( 
    _Connection __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *TransactionLevel);


void __RPC_STUB _Connection_BeginTrans_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT __stdcall _Connection_CommitTrans_Proxy( 
    _Connection __RPC_FAR * This);


void __RPC_STUB _Connection_CommitTrans_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT __stdcall _Connection_RollbackTrans_Proxy( 
    _Connection __RPC_FAR * This);


void __RPC_STUB _Connection_RollbackTrans_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT __stdcall _Connection_Open_Proxy( 
    _Connection __RPC_FAR * This,
    /* [defaultvalue][optional][in] */ BSTR ConnectionString,
    /* [defaultvalue][optional][in] */ BSTR UserID,
    /* [defaultvalue][optional][in] */ BSTR Password,
    /* [defaultvalue][optional][in] */ long Options);


void __RPC_STUB _Connection_Open_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT __stdcall _Connection_get_Errors_Proxy( 
    _Connection __RPC_FAR * This,
    /* [retval][out] */ Errors __RPC_FAR *__RPC_FAR *ppErrors);


void __RPC_STUB _Connection_get_Errors_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT __stdcall _Connection_get_DefaultDatabase_Proxy( 
    _Connection __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstr);


void __RPC_STUB _Connection_get_DefaultDatabase_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT __stdcall _Connection_put_DefaultDatabase_Proxy( 
    _Connection __RPC_FAR * This,
    /* [in] */ BSTR pbstr);


void __RPC_STUB _Connection_put_DefaultDatabase_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT __stdcall _Connection_get_IsolationLevel_Proxy( 
    _Connection __RPC_FAR * This,
    /* [retval][out] */ IsolationLevelEnum __RPC_FAR *Level);


void __RPC_STUB _Connection_get_IsolationLevel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT __stdcall _Connection_put_IsolationLevel_Proxy( 
    _Connection __RPC_FAR * This,
    /* [in] */ IsolationLevelEnum Level);


void __RPC_STUB _Connection_put_IsolationLevel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT __stdcall _Connection_get_Attributes_Proxy( 
    _Connection __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plAttr);


void __RPC_STUB _Connection_get_Attributes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT __stdcall _Connection_put_Attributes_Proxy( 
    _Connection __RPC_FAR * This,
    /* [in] */ long plAttr);


void __RPC_STUB _Connection_put_Attributes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT __stdcall _Connection_get_CursorLocation_Proxy( 
    _Connection __RPC_FAR * This,
    /* [retval][out] */ CursorLocationEnum __RPC_FAR *plCursorLoc);


void __RPC_STUB _Connection_get_CursorLocation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT __stdcall _Connection_put_CursorLocation_Proxy( 
    _Connection __RPC_FAR * This,
    /* [in] */ CursorLocationEnum plCursorLoc);


void __RPC_STUB _Connection_put_CursorLocation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT __stdcall _Connection_get_Mode_Proxy( 
    _Connection __RPC_FAR * This,
    /* [retval][out] */ ConnectModeEnum __RPC_FAR *plMode);


void __RPC_STUB _Connection_get_Mode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT __stdcall _Connection_put_Mode_Proxy( 
    _Connection __RPC_FAR * This,
    /* [in] */ ConnectModeEnum plMode);


void __RPC_STUB _Connection_put_Mode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT __stdcall _Connection_get_Provider_Proxy( 
    _Connection __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstr);


void __RPC_STUB _Connection_get_Provider_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT __stdcall _Connection_put_Provider_Proxy( 
    _Connection __RPC_FAR * This,
    /* [in] */ BSTR pbstr);


void __RPC_STUB _Connection_put_Provider_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT __stdcall _Connection_get_State_Proxy( 
    _Connection __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plObjState);


void __RPC_STUB _Connection_get_State_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT __stdcall _Connection_get_Properties_Proxy( 
    _Connection __RPC_FAR * This,
    /* [retval][out] */ Properties __RPC_FAR *__RPC_FAR *ppProperties);


void __RPC_STUB _Connection_get_Properties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* ___Connection_INTERFACE_DEFINED__ */



#ifndef __ADOCE_LIBRARY_DEFINED__
#define __ADOCE_LIBRARY_DEFINED__

/****************************************
 * Generated header for library: ADOCE
 * at Tue Nov 09 09:56:36 1999
 * using MIDL 3.02.88
 ****************************************/
/* [helpstring][version][uuid] */ 



enum BookmarkEnum
    {	adBookmarkCurrent	= 0,
	adBookmarkFirst	= 1,
	adBookmarkLast	= 2
    };

enum FilterGroupEnum
    {	adFilterNone	= 0
    };

enum PropertyAttributesEnum
    {	adPropNotSupported	= 0,
	adPropRequired	= 1,
	adPropOptional	= 2,
	adPropRead	= 512,
	adPropWrite	= 1024
    };

enum XactAttributeEnum
    {	adXactCommitRetaining	= 131072,
	adXactAbortRetaining	= 262144
    };

enum ObjectStateEnum
    {	adStateClosed	= 0,
	adStateOpen	= 1
    };

enum ConnectPropmtEnum
    {	adPromptAlways	= 1,
	adPromptComplete	= 2,
	adPromptCompleteRequired	= 3,
	adPromptNever	= 4
    };
typedef /* [uuid] */ 
enum CommandEnum
    {	adCmdUnspecified	= -1,
	adCmdText	= 1,
	adCmdTable	= 2,
	adCmdStoredProc	= 4,
	adCmdUnknown	= 8,
	adCmdTableDirect	= 512
    }	CommandEnum;

typedef /* [uuid] */ 
enum ErrorValueEnum
    {	adErrInvalidArgument	= 0xbb9,
	adErrNoCurrentRecord	= 0xbcd,
	adErrIllegalOperation	= 0xc93,
	adErrInTransaction	= 0xcae,
	adErrFeatureNotAvailable	= 0xcb3,
	adErrItemNotFound	= 0xcc1,
	adErrObjectInCollection	= 0xd27,
	adErrObjectNotSet	= 0xd5c,
	adErrDataConversion	= 0xd5d,
	adErrObjectClosed	= 0xe78,
	adErrObjectOpen	= 0xe79,
	adErrProviderNotFound	= 0xe7a,
	adErrBoundToCommand	= 0xe7b,
	adErrInvalidParamInfo	= 0xe7c,
	adErrInvalidConnection	= 0xe7d
    }	ErrorValueEnum;


EXTERN_C const IID LIBID_ADOCE;

EXTERN_C const CLSID CLSID_Recordset;

#ifdef __cplusplus

class DECLSPEC_UUID("113033F8-F682-11D2-BB62-00C04F680ACC")
Recordset;
#endif

EXTERN_C const CLSID CLSID_Connection;

#ifdef __cplusplus

class DECLSPEC_UUID("113033F9-F682-11D2-BB62-00C04F680ACC")
Connection;
#endif
#endif /* __ADOCE_LIBRARY_DEFINED__ */

/****************************************
 * Generated header for interface: __MIDL_itf_adoce30_0102
 * at Tue Nov 09 09:56:36 1999
 * using MIDL 3.02.88
 ****************************************/
/* [local] */ 


// backwards compatibility helpers

typedef _Recordset		IADOCERecordset;
typedef _Connection		IADOCEConnection;
typedef _Collection		IADOCECollection;
typedef Properties		IADOCEProperties;
typedef Property			IADOCEProperty;
typedef Errors			IADOCEErrors;
typedef Error			IADOCEError;
typedef Fields			IADOCEFields;
typedef Field			IADOCEField;

#define CLSID_ADOCERecordset CLSID_Recordset
#define CLSID_ADOCEConnection CLSID_Connection

#define IID_IADOCERecordset IID__Recordset
#define IID_IADOCEConnection IID__Connection
#define IID_IADOCECollection IID__Collection
#define IID_IADOCEProperties IID_Properties
#define IID_IADOCEProperty IID_Property
#define IID_IADOCEErrors IID_Errors
#define IID_IADOCEError IID_Error
#define IID_IADOCEFields IID_Fields
#define IID_IADOCEField IID_Field


extern RPC_IF_HANDLE __MIDL_itf_adoce30_0102_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_adoce30_0102_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long __RPC_FAR *, unsigned long            , BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long __RPC_FAR *, BSTR __RPC_FAR * ); 

unsigned long             __RPC_USER  VARIANT_UserSize(     unsigned long __RPC_FAR *, unsigned long            , VARIANT __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  VARIANT_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, VARIANT __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  VARIANT_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, VARIANT __RPC_FAR * ); 
void                      __RPC_USER  VARIANT_UserFree(     unsigned long __RPC_FAR *, VARIANT __RPC_FAR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
