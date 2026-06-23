/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.02.88 */
/* at Mon Dec 07 11:02:04 1998
 */
/* Compiler settings for pimstore.idl:
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

#ifndef __pimstore_h__
#define __pimstore_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IException_FWD_DEFINED__
#define __IException_FWD_DEFINED__
typedef interface IException IException;
#endif 	/* __IException_FWD_DEFINED__ */


#ifndef __IExceptions_FWD_DEFINED__
#define __IExceptions_FWD_DEFINED__
typedef interface IExceptions IExceptions;
#endif 	/* __IExceptions_FWD_DEFINED__ */


#ifndef __ITimeZone_FWD_DEFINED__
#define __ITimeZone_FWD_DEFINED__
typedef interface ITimeZone ITimeZone;
#endif 	/* __ITimeZone_FWD_DEFINED__ */


#ifndef __IRecurrencePattern_FWD_DEFINED__
#define __IRecurrencePattern_FWD_DEFINED__
typedef interface IRecurrencePattern IRecurrencePattern;
#endif 	/* __IRecurrencePattern_FWD_DEFINED__ */


#ifndef __IRecipient_FWD_DEFINED__
#define __IRecipient_FWD_DEFINED__
typedef interface IRecipient IRecipient;
#endif 	/* __IRecipient_FWD_DEFINED__ */


#ifndef __IPOlRecipient_FWD_DEFINED__
#define __IPOlRecipient_FWD_DEFINED__
typedef interface IPOlRecipient IPOlRecipient;
#endif 	/* __IPOlRecipient_FWD_DEFINED__ */


#ifndef __IRecipients_FWD_DEFINED__
#define __IRecipients_FWD_DEFINED__
typedef interface IRecipients IRecipients;
#endif 	/* __IRecipients_FWD_DEFINED__ */


#ifndef __IPOutlookItemCollection_FWD_DEFINED__
#define __IPOutlookItemCollection_FWD_DEFINED__
typedef interface IPOutlookItemCollection IPOutlookItemCollection;
#endif 	/* __IPOutlookItemCollection_FWD_DEFINED__ */


#ifndef __IPOlItems_FWD_DEFINED__
#define __IPOlItems_FWD_DEFINED__
typedef interface IPOlItems IPOlItems;
#endif 	/* __IPOlItems_FWD_DEFINED__ */


#ifndef __IContact_FWD_DEFINED__
#define __IContact_FWD_DEFINED__
typedef interface IContact IContact;
#endif 	/* __IContact_FWD_DEFINED__ */


#ifndef __ITask_FWD_DEFINED__
#define __ITask_FWD_DEFINED__
typedef interface ITask ITask;
#endif 	/* __ITask_FWD_DEFINED__ */


#ifndef __IAppointment_FWD_DEFINED__
#define __IAppointment_FWD_DEFINED__
typedef interface IAppointment IAppointment;
#endif 	/* __IAppointment_FWD_DEFINED__ */


#ifndef __ICity_FWD_DEFINED__
#define __ICity_FWD_DEFINED__
typedef interface ICity ICity;
#endif 	/* __ICity_FWD_DEFINED__ */


#ifndef __IFolder_FWD_DEFINED__
#define __IFolder_FWD_DEFINED__
typedef interface IFolder IFolder;
#endif 	/* __IFolder_FWD_DEFINED__ */


#ifndef __IPOutlookApp_FWD_DEFINED__
#define __IPOutlookApp_FWD_DEFINED__
typedef interface IPOutlookApp IPOutlookApp;
#endif 	/* __IPOutlookApp_FWD_DEFINED__ */


#ifndef __Exception_FWD_DEFINED__
#define __Exception_FWD_DEFINED__

#ifdef __cplusplus
typedef class Exception Exception;
#else
typedef struct Exception Exception;
#endif /* __cplusplus */

#endif 	/* __Exception_FWD_DEFINED__ */


#ifndef __Exceptions_FWD_DEFINED__
#define __Exceptions_FWD_DEFINED__

#ifdef __cplusplus
typedef class Exceptions Exceptions;
#else
typedef struct Exceptions Exceptions;
#endif /* __cplusplus */

#endif 	/* __Exceptions_FWD_DEFINED__ */


#ifndef __TimeZone_FWD_DEFINED__
#define __TimeZone_FWD_DEFINED__

#ifdef __cplusplus
typedef class TimeZone TimeZone;
#else
typedef struct TimeZone TimeZone;
#endif /* __cplusplus */

#endif 	/* __TimeZone_FWD_DEFINED__ */


#ifndef __RecurrencePattern_FWD_DEFINED__
#define __RecurrencePattern_FWD_DEFINED__

#ifdef __cplusplus
typedef class RecurrencePattern RecurrencePattern;
#else
typedef struct RecurrencePattern RecurrencePattern;
#endif /* __cplusplus */

#endif 	/* __RecurrencePattern_FWD_DEFINED__ */


#ifndef __Recipient_FWD_DEFINED__
#define __Recipient_FWD_DEFINED__

#ifdef __cplusplus
typedef class Recipient Recipient;
#else
typedef struct Recipient Recipient;
#endif /* __cplusplus */

#endif 	/* __Recipient_FWD_DEFINED__ */


#ifndef __Recipients_FWD_DEFINED__
#define __Recipients_FWD_DEFINED__

#ifdef __cplusplus
typedef class Recipients Recipients;
#else
typedef struct Recipients Recipients;
#endif /* __cplusplus */

#endif 	/* __Recipients_FWD_DEFINED__ */


#ifndef __Items_FWD_DEFINED__
#define __Items_FWD_DEFINED__

#ifdef __cplusplus
typedef class Items Items;
#else
typedef struct Items Items;
#endif /* __cplusplus */

#endif 	/* __Items_FWD_DEFINED__ */


#ifndef __ContactItem_FWD_DEFINED__
#define __ContactItem_FWD_DEFINED__

#ifdef __cplusplus
typedef class ContactItem ContactItem;
#else
typedef struct ContactItem ContactItem;
#endif /* __cplusplus */

#endif 	/* __ContactItem_FWD_DEFINED__ */


#ifndef __TaskItem_FWD_DEFINED__
#define __TaskItem_FWD_DEFINED__

#ifdef __cplusplus
typedef class TaskItem TaskItem;
#else
typedef struct TaskItem TaskItem;
#endif /* __cplusplus */

#endif 	/* __TaskItem_FWD_DEFINED__ */


#ifndef __AppointmentItem_FWD_DEFINED__
#define __AppointmentItem_FWD_DEFINED__

#ifdef __cplusplus
typedef class AppointmentItem AppointmentItem;
#else
typedef struct AppointmentItem AppointmentItem;
#endif /* __cplusplus */

#endif 	/* __AppointmentItem_FWD_DEFINED__ */


#ifndef __CityItem_FWD_DEFINED__
#define __CityItem_FWD_DEFINED__

#ifdef __cplusplus
typedef class CityItem CityItem;
#else
typedef struct CityItem CityItem;
#endif /* __cplusplus */

#endif 	/* __CityItem_FWD_DEFINED__ */


#ifndef __Folder_FWD_DEFINED__
#define __Folder_FWD_DEFINED__

#ifdef __cplusplus
typedef class Folder Folder;
#else
typedef struct Folder Folder;
#endif /* __cplusplus */

#endif 	/* __Folder_FWD_DEFINED__ */


#ifndef __Application_FWD_DEFINED__
#define __Application_FWD_DEFINED__

#ifdef __cplusplus
typedef class Application Application;
#else
typedef struct Application Application;
#endif /* __cplusplus */

#endif 	/* __Application_FWD_DEFINED__ */


/* header files for imported files */
#include "unknwn.h"
#include "oaidl.h"

/****************************************
 * Generated header for interface: __MIDL_itf_pimstore_0000
 * at Mon Dec 07 11:02:04 1998
 * using MIDL 3.02.88
 ****************************************/
/* [local] */ 


#ifndef __IException_INTERFACE_DEFINED__
#define __IException_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IException
 * at Mon Dec 07 11:02:04 1998
 * using MIDL 3.02.88
 ****************************************/
/* [dual][full][helpstring][uuid][object] */ 


DEFINE_GUID(IID_IException, 0xb47398d0, 0x3b73, 0x11d2, 0x8f, 0x1b, 0x0, 0x0, 0xf8, 0x7a, 0x43, 0x35);

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("B47398D0-3B73-11d2-8F1B-0000F87A4335")
    IException : public IDispatch
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_AppointmentItem( 
            /* [retval][out] */ IAppointment __RPC_FAR *__RPC_FAR *ppAppt) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_OriginalDate( 
            /* [retval][out] */ DATE __RPC_FAR *pdate) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Deleted( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pfDeleted) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Application( 
            /* [retval][out] */ IPOutlookApp __RPC_FAR *__RPC_FAR *polApp) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IExceptionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IException __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IException __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IException __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IException __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IException __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IException __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IException __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_AppointmentItem )( 
            IException __RPC_FAR * This,
            /* [retval][out] */ IAppointment __RPC_FAR *__RPC_FAR *ppAppt);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_OriginalDate )( 
            IException __RPC_FAR * This,
            /* [retval][out] */ DATE __RPC_FAR *pdate);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Deleted )( 
            IException __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pfDeleted);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Application )( 
            IException __RPC_FAR * This,
            /* [retval][out] */ IPOutlookApp __RPC_FAR *__RPC_FAR *polApp);
        
        END_INTERFACE
    } IExceptionVtbl;

    interface IException
    {
        CONST_VTBL struct IExceptionVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IException_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IException_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IException_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IException_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IException_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IException_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IException_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IException_get_AppointmentItem(This,ppAppt)	\
    (This)->lpVtbl -> get_AppointmentItem(This,ppAppt)

#define IException_get_OriginalDate(This,pdate)	\
    (This)->lpVtbl -> get_OriginalDate(This,pdate)

#define IException_get_Deleted(This,pfDeleted)	\
    (This)->lpVtbl -> get_Deleted(This,pfDeleted)

#define IException_get_Application(This,polApp)	\
    (This)->lpVtbl -> get_Application(This,polApp)

#endif /* COBJMACROS */


#endif 	/* C style interface */

#endif 	/* __IException_INTERFACE_DEFINED__ */


#ifndef __IExceptions_INTERFACE_DEFINED__
#define __IExceptions_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IExceptions
 * at Mon Dec 07 11:02:04 1998
 * using MIDL 3.02.88
 ****************************************/
/* [dual][full][helpstring][uuid][object] */ 



DEFINE_GUID(IID_IExceptions, 0xb47398d1, 0x3b73, 0x11d2, 0x8f, 0x1b, 0x0, 0x0, 0xf8, 0x7a, 0x43, 0x35);

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("B47398D1-3B73-11d2-8F1B-0000F87A4335")
    IExceptions : public IDispatch
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Item( 
            /* [in] */ int iIndex,
            /* [retval][out] */ IException __RPC_FAR *__RPC_FAR *ppExcept) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ int __RPC_FAR *pnCount) = 0;
            
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppEnumerator) = 0;
                    
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Application( 
            /* [retval][out] */ IPOutlookApp __RPC_FAR *__RPC_FAR *polApp) = 0;
                
    };
    
#else 	/* C style interface */

    typedef struct IExceptionsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IExceptions __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IExceptions __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IExceptions __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IExceptions __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IExceptions __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IExceptions __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IExceptions __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Item )( 
            IExceptions __RPC_FAR * This,
            /* [in] */ int iIndex,
            /* [retval][out] */ IException __RPC_FAR *__RPC_FAR *ppExcept);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Count )( 
            IExceptions __RPC_FAR * This,
            /* [retval][out] */ int __RPC_FAR *pnCount);

         /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get__NewEnum )( 
            IExceptions __RPC_FAR * This,
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppEnumerator);
                    
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Application )( 
            IExceptions __RPC_FAR * This,
            /* [retval][out] */ IPOutlookApp __RPC_FAR *__RPC_FAR *polApp);
        
        END_INTERFACE
    } IExceptionsVtbl;

    interface IExceptions
    {
        CONST_VTBL struct IExceptionsVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IExceptions_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IExceptions_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IExceptions_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IExceptions_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IExceptions_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IExceptions_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IExceptions_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IExceptions_Item(This,iIndex,ppExcept)	\
    (This)->lpVtbl -> Item(This,iIndex,ppExcept)

#define IExceptions_get_Count(This,pnCount)	\
    (This)->lpVtbl -> get_Count(This,pnCount)

#define IExceptions_get__NewEnum(This,ppEnumerator)	\
    (This)->lpVtbl -> get__NewEnum(This,ppEnumerator)

#define IExceptions_get_Application(This,polApp)	\
    (This)->lpVtbl -> get_Application(This,polApp)
    
#endif /* COBJMACROS */


#endif 	/* C style interface */

#endif 	/* __IExceptions_INTERFACE_DEFINED__ */


#ifndef __ITimeZone_INTERFACE_DEFINED__
#define __ITimeZone_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ITimeZone
 * at Mon Dec 07 11:02:04 1998
 * using MIDL 3.02.88
 ****************************************/
/* [dual][full][helpstring][uuid][object] */ 


DEFINE_GUID(IID_ITimeZone, 0x78b27290, 0x5256, 0x11d2, 0x8f, 0x1b, 0x0, 0x0, 0xf8, 0x7a, 0x43, 0x35);

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("78B27290-5256-11d2-8F1B-0000F87A4335")
    ITimeZone : public IDispatch
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Bias( 
            /* [retval][out] */ long __RPC_FAR *plBias) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_SupportsDST( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pfSupportsDST) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_IsStandardAbsoluteDate( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pfAbsolute) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_IsDaylightAbsoluteDate( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pfAbsolute) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_StandardBias( 
            /* [retval][out] */ long __RPC_FAR *plBias) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_StandardName( 
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_StandardDate( 
            /* [retval][out] */ DATE __RPC_FAR *pDate) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_StandardDayOfWeekMask( 
            /* [retval][out] */ long __RPC_FAR *plMask) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_StandardInstance( 
            /* [retval][out] */ long __RPC_FAR *plInstance) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_StandardMonthOfYear( 
            /* [retval][out] */ long __RPC_FAR *plMonth) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_DaylightBias( 
            /* [retval][out] */ long __RPC_FAR *plBias) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_DaylightName( 
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_DaylightDate( 
            /* [retval][out] */ DATE __RPC_FAR *pDate) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_DaylightDayOfWeekMask( 
            /* [retval][out] */ long __RPC_FAR *plMask) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_DaylightInstance( 
            /* [retval][out] */ long __RPC_FAR *plInstance) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_DaylightMonthOfYear( 
            /* [retval][out] */ long __RPC_FAR *plMonth) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Application( 
            /* [retval][out] */ IPOutlookApp __RPC_FAR *__RPC_FAR *polApp) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITimeZoneVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITimeZone __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITimeZone __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITimeZone __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ITimeZone __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ITimeZone __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ITimeZone __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ITimeZone __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Bias )( 
            ITimeZone __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plBias);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_SupportsDST )( 
            ITimeZone __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pfSupportsDST);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_IsStandardAbsoluteDate )( 
            ITimeZone __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pfAbsolute);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_IsDaylightAbsoluteDate )( 
            ITimeZone __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pfAbsolute);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_StandardBias )( 
            ITimeZone __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plBias);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_StandardName )( 
            ITimeZone __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_StandardDate )( 
            ITimeZone __RPC_FAR * This,
            /* [retval][out] */ DATE __RPC_FAR *pDate);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_StandardDayOfWeekMask )( 
            ITimeZone __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plMask);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_StandardInstance )( 
            ITimeZone __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plInstance);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_StandardMonthOfYear )( 
            ITimeZone __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plMonth);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DaylightBias )( 
            ITimeZone __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plBias);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DaylightName )( 
            ITimeZone __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DaylightDate )( 
            ITimeZone __RPC_FAR * This,
            /* [retval][out] */ DATE __RPC_FAR *pDate);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DaylightDayOfWeekMask )( 
            ITimeZone __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plMask);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DaylightInstance )( 
            ITimeZone __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plInstance);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DaylightMonthOfYear )( 
            ITimeZone __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plMonth);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Application )( 
            ITimeZone __RPC_FAR * This,
            /* [retval][out] */ IPOutlookApp __RPC_FAR *__RPC_FAR *polApp);
        
        END_INTERFACE
    } ITimeZoneVtbl;

    interface ITimeZone
    {
        CONST_VTBL struct ITimeZoneVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITimeZone_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITimeZone_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITimeZone_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITimeZone_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITimeZone_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITimeZone_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITimeZone_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITimeZone_get_Bias(This,plBias)	\
    (This)->lpVtbl -> get_Bias(This,plBias)

#define ITimeZone_get_SupportsDST(This,pfSupportsDST)	\
    (This)->lpVtbl -> get_SupportsDST(This,pfSupportsDST)

#define ITimeZone_get_IsStandardAbsoluteDate(This,pfAbsolute)	\
    (This)->lpVtbl -> get_IsStandardAbsoluteDate(This,pfAbsolute)

#define ITimeZone_get_IsDaylightAbsoluteDate(This,pfAbsolute)	\
    (This)->lpVtbl -> get_IsDaylightAbsoluteDate(This,pfAbsolute)

#define ITimeZone_get_StandardBias(This,plBias)	\
    (This)->lpVtbl -> get_StandardBias(This,plBias)

#define ITimeZone_get_StandardName(This,ppwsz)	\
    (This)->lpVtbl -> get_StandardName(This,ppwsz)

#define ITimeZone_get_StandardDate(This,pDate)	\
    (This)->lpVtbl -> get_StandardDate(This,pDate)

#define ITimeZone_get_StandardDayOfWeekMask(This,plMask)	\
    (This)->lpVtbl -> get_StandardDayOfWeekMask(This,plMask)

#define ITimeZone_get_StandardInstance(This,plInstance)	\
    (This)->lpVtbl -> get_StandardInstance(This,plInstance)

#define ITimeZone_get_StandardMonthOfYear(This,plMonth)	\
    (This)->lpVtbl -> get_StandardMonthOfYear(This,plMonth)

#define ITimeZone_get_DaylightBias(This,plBias)	\
    (This)->lpVtbl -> get_DaylightBias(This,plBias)

#define ITimeZone_get_DaylightName(This,ppwsz)	\
    (This)->lpVtbl -> get_DaylightName(This,ppwsz)

#define ITimeZone_get_DaylightDate(This,pDate)	\
    (This)->lpVtbl -> get_DaylightDate(This,pDate)

#define ITimeZone_get_DaylightDayOfWeekMask(This,plMask)	\
    (This)->lpVtbl -> get_DaylightDayOfWeekMask(This,plMask)

#define ITimeZone_get_DaylightInstance(This,plInstance)	\
    (This)->lpVtbl -> get_DaylightInstance(This,plInstance)

#define ITimeZone_get_DaylightMonthOfYear(This,plMonth)	\
    (This)->lpVtbl -> get_DaylightMonthOfYear(This,plMonth)

#define ITimeZone_get_Application(This,polApp)	\
    (This)->lpVtbl -> get_Application(This,polApp)

#endif /* COBJMACROS */


#endif 	/* C style interface */

#endif 	/* __ITimeZone_INTERFACE_DEFINED__ */


#ifndef __IRecurrencePattern_INTERFACE_DEFINED__
#define __IRecurrencePattern_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IRecurrencePattern
 * at Mon Dec 07 11:02:04 1998
 * using MIDL 3.02.88
 ****************************************/
/* [dual][full][helpstring][uuid][object] */ 



DEFINE_GUID(IID_IRecurrencePattern, 0x38f47300, 0x270f, 0x11d2, 0x8f, 0x18, 0x0, 0x0, 0xf8, 0x7a, 0x43, 0x35);

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("38F47300-270F-11d2-8F18-0000F87A4335")
    IRecurrencePattern : public IDispatch
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_RecurrenceType( 
            /* [retval][out] */ long __RPC_FAR *plRecType) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_PatternStartDate( 
            /* [retval][out] */ DATE __RPC_FAR *pst) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_StartTime( 
            /* [retval][out] */ DATE __RPC_FAR *pst) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_EndTime( 
            /* [retval][out] */ DATE __RPC_FAR *pst) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_PatternEndDate( 
            /* [retval][out] */ DATE __RPC_FAR *pst) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_NoEndDate( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pfNoEndDate) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Occurrences( 
            /* [retval][out] */ long __RPC_FAR *plOccurrences) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Interval( 
            /* [retval][out] */ long __RPC_FAR *plInterval) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_DayOfWeekMask( 
            /* [retval][out] */ long __RPC_FAR *plMask) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_DayOfMonth( 
            /* [retval][out] */ long __RPC_FAR *plDay) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Instance( 
            /* [retval][out] */ long __RPC_FAR *plInstance) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Duration( 
            /* [retval][out] */ long __RPC_FAR *plDuration) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_MonthOfYear( 
            /* [retval][out] */ long __RPC_FAR *plMask) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_RecurrenceType( 
            /* [in] */ long lRecType) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_PatternStartDate( 
            /* [in] */ DATE st) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_StartTime( 
            /* [in] */ DATE st) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_EndTime( 
            /* [in] */ DATE st) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_PatternEndDate( 
            /* [in] */ DATE st) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_NoEndDate( 
            /* [in] */ VARIANT_BOOL fNoEndDate) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Occurrences( 
            /* [in] */ long lOccurrences) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Interval( 
            /* [in] */ long lInterval) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_DayOfWeekMask( 
            /* [in] */ long lMask) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_DayOfMonth( 
            /* [in] */ long lDay) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Instance( 
            /* [in] */ long lInstance) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Duration( 
            /* [in] */ long lDuration) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_MonthOfYear( 
            /* [in] */ long lMask) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Exceptions( 
            /* [retval][out] */ IExceptions __RPC_FAR *__RPC_FAR *ppExceptions) = 0;
               
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Application( 
            /* [retval][out] */ IPOutlookApp __RPC_FAR *__RPC_FAR *polApp) = 0;
 
        virtual HRESULT STDMETHODCALLTYPE GetOccurrence( 
            /* [in] */ DATE date,
            /* [retval][out] */ IAppointment __RPC_FAR *__RPC_FAR *ppAppt) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRecurrencePatternVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRecurrencePattern __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRecurrencePattern __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRecurrencePattern __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IRecurrencePattern __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IRecurrencePattern __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IRecurrencePattern __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IRecurrencePattern __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_RecurrenceType )( 
            IRecurrencePattern __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plRecType);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_PatternStartDate )( 
            IRecurrencePattern __RPC_FAR * This,
            /* [retval][out] */ DATE __RPC_FAR *pst);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_StartTime )( 
            IRecurrencePattern __RPC_FAR * This,
            /* [retval][out] */ DATE __RPC_FAR *pst);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_EndTime )( 
            IRecurrencePattern __RPC_FAR * This,
            /* [retval][out] */ DATE __RPC_FAR *pst);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_PatternEndDate )( 
            IRecurrencePattern __RPC_FAR * This,
            /* [retval][out] */ DATE __RPC_FAR *pst);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_NoEndDate )( 
            IRecurrencePattern __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pfNoEndDate);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Occurrences )( 
            IRecurrencePattern __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plOccurrences);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Interval )( 
            IRecurrencePattern __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plInterval);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DayOfWeekMask )( 
            IRecurrencePattern __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plMask);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DayOfMonth )( 
            IRecurrencePattern __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plDay);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Instance )( 
            IRecurrencePattern __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plInstance);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Duration )( 
            IRecurrencePattern __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plDuration);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_MonthOfYear )( 
            IRecurrencePattern __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plMask);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_RecurrenceType )( 
            IRecurrencePattern __RPC_FAR * This,
            /* [in] */ long lRecType);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_PatternStartDate )( 
            IRecurrencePattern __RPC_FAR * This,
            /* [in] */ DATE st);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_StartTime )( 
            IRecurrencePattern __RPC_FAR * This,
            /* [in] */ DATE st);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_EndTime )( 
            IRecurrencePattern __RPC_FAR * This,
            /* [in] */ DATE st);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_PatternEndDate )( 
            IRecurrencePattern __RPC_FAR * This,
            /* [in] */ DATE st);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_NoEndDate )( 
            IRecurrencePattern __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL fNoEndDate);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Occurrences )( 
            IRecurrencePattern __RPC_FAR * This,
            /* [in] */ long lOccurrences);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Interval )( 
            IRecurrencePattern __RPC_FAR * This,
            /* [in] */ long lInterval);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_DayOfWeekMask )( 
            IRecurrencePattern __RPC_FAR * This,
            /* [in] */ long lMask);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_DayOfMonth )( 
            IRecurrencePattern __RPC_FAR * This,
            /* [in] */ long lDay);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Instance )( 
            IRecurrencePattern __RPC_FAR * This,
            /* [in] */ long lInstance);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Duration )( 
            IRecurrencePattern __RPC_FAR * This,
            /* [in] */ long lDuration);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_MonthOfYear )( 
            IRecurrencePattern __RPC_FAR * This,
            /* [in] */ long lMask);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Exceptions )( 
            IRecurrencePattern __RPC_FAR * This,
            /* [retval][out] */ IExceptions __RPC_FAR *__RPC_FAR *ppExceptions);
              
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Application )( 
            IRecurrencePattern __RPC_FAR * This,
            /* [retval][out] */ IPOutlookApp __RPC_FAR *__RPC_FAR *polApp);
  
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetOccurrence )( 
            IRecurrencePattern __RPC_FAR * This,
            /* [in] */ DATE date,
            /* [retval][out] */ IAppointment __RPC_FAR *__RPC_FAR *ppAppt);
        
        END_INTERFACE
    } IRecurrencePatternVtbl;

    interface IRecurrencePattern
    {
        CONST_VTBL struct IRecurrencePatternVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRecurrencePattern_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRecurrencePattern_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRecurrencePattern_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRecurrencePattern_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IRecurrencePattern_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IRecurrencePattern_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IRecurrencePattern_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IRecurrencePattern_get_RecurrenceType(This,plRecType)	\
    (This)->lpVtbl -> get_RecurrenceType(This,plRecType)

#define IRecurrencePattern_get_PatternStartDate(This,pst)	\
    (This)->lpVtbl -> get_PatternStartDate(This,pst)

#define IRecurrencePattern_get_StartTime(This,pst)	\
    (This)->lpVtbl -> get_StartTime(This,pst)

#define IRecurrencePattern_get_EndTime(This,pst)	\
    (This)->lpVtbl -> get_EndTime(This,pst)

#define IRecurrencePattern_get_PatternEndDate(This,pst)	\
    (This)->lpVtbl -> get_PatternEndDate(This,pst)

#define IRecurrencePattern_get_NoEndDate(This,pfNoEndDate)	\
    (This)->lpVtbl -> get_NoEndDate(This,pfNoEndDate)

#define IRecurrencePattern_get_Occurrences(This,plOccurrences)	\
    (This)->lpVtbl -> get_Occurrences(This,plOccurrences)

#define IRecurrencePattern_get_Interval(This,plInterval)	\
    (This)->lpVtbl -> get_Interval(This,plInterval)

#define IRecurrencePattern_get_DayOfWeekMask(This,plMask)	\
    (This)->lpVtbl -> get_DayOfWeekMask(This,plMask)

#define IRecurrencePattern_get_DayOfMonth(This,plDay)	\
    (This)->lpVtbl -> get_DayOfMonth(This,plDay)

#define IRecurrencePattern_get_Instance(This,plInstance)	\
    (This)->lpVtbl -> get_Instance(This,plInstance)

#define IRecurrencePattern_get_Duration(This,plDuration)	\
    (This)->lpVtbl -> get_Duration(This,plDuration)

#define IRecurrencePattern_get_MonthOfYear(This,plMask)	\
    (This)->lpVtbl -> get_MonthOfYear(This,plMask)

#define IRecurrencePattern_put_RecurrenceType(This,lRecType)	\
    (This)->lpVtbl -> put_RecurrenceType(This,lRecType)

#define IRecurrencePattern_put_PatternStartDate(This,st)	\
    (This)->lpVtbl -> put_PatternStartDate(This,st)

#define IRecurrencePattern_put_StartTime(This,st)	\
    (This)->lpVtbl -> put_StartTime(This,st)

#define IRecurrencePattern_put_EndTime(This,st)	\
    (This)->lpVtbl -> put_EndTime(This,st)

#define IRecurrencePattern_put_PatternEndDate(This,st)	\
    (This)->lpVtbl -> put_PatternEndDate(This,st)

#define IRecurrencePattern_put_NoEndDate(This,fNoEndDate)	\
    (This)->lpVtbl -> put_NoEndDate(This,fNoEndDate)

#define IRecurrencePattern_put_Occurrences(This,lOccurrences)	\
    (This)->lpVtbl -> put_Occurrences(This,lOccurrences)

#define IRecurrencePattern_put_Interval(This,lInterval)	\
    (This)->lpVtbl -> put_Interval(This,lInterval)

#define IRecurrencePattern_put_DayOfWeekMask(This,lMask)	\
    (This)->lpVtbl -> put_DayOfWeekMask(This,lMask)

#define IRecurrencePattern_put_DayOfMonth(This,lDay)	\
    (This)->lpVtbl -> put_DayOfMonth(This,lDay)

#define IRecurrencePattern_put_Instance(This,lInstance)	\
    (This)->lpVtbl -> put_Instance(This,lInstance)

#define IRecurrencePattern_put_Duration(This,lDuration)	\
    (This)->lpVtbl -> put_Duration(This,lDuration)

#define IRecurrencePattern_put_MonthOfYear(This,lMask)	\
    (This)->lpVtbl -> put_MonthOfYear(This,lMask)

#define IRecurrencePattern_get_Exceptions(This,ppExceptions)	\
    (This)->lpVtbl -> get_Exceptions(This,ppExceptions)

#define IRecurrencePattern_get_Application(This,polApp)	\
    (This)->lpVtbl -> get_Application(This,polApp)

#define IRecurrencePattern_GetOccurrence(This,date,ppAppt)	\
    (This)->lpVtbl -> GetOccurrence(This,date,ppAppt)

#endif /* COBJMACROS */


#endif 	/* C style interface */

#endif 	/* __IRecurrencePattern_INTERFACE_DEFINED__ */


#ifndef __IRecipient_INTERFACE_DEFINED__
#define __IRecipient_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IRecipient
 * at Mon Dec 07 11:02:04 1998
 * using MIDL 3.02.88
 ****************************************/
/* [dual][full][helpstring][uuid][object] */ 



DEFINE_GUID(IID_IRecipient, 0x7e136be0, 0x5240, 0x11d2, 0x8f, 0x1b, 0x0, 0x0, 0xf8, 0x7a, 0x43, 0x35);

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("7E136BE0-5240-11d2-8F1B-0000F87A4335")
    IRecipient : public IDispatch
    {
    public:
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Address( 
            /* [in] */ BSTR pwsz) = 0;
                
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Address( 
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz) = 0;
                
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Application( 
            /* [retval][out] */ IPOutlookApp __RPC_FAR *__RPC_FAR *polApp) = 0;

    };
    
#else 	/* C style interface */

    typedef struct IRecipientVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRecipient __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRecipient __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRecipient __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IRecipient __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IRecipient __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IRecipient __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IRecipient __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Address )( 
            IRecipient __RPC_FAR * This,
            /* [in] */ BSTR pwsz);
                
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Address )( 
            IRecipient __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Name )( 
            IRecipient __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Application )( 
            IRecipient __RPC_FAR * This,
            /* [retval][out] */ IPOutlookApp __RPC_FAR *__RPC_FAR *polApp);
        
        END_INTERFACE
    } IRecipientVtbl;

    interface IRecipient
    {
        CONST_VTBL struct IRecipientVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRecipient_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRecipient_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRecipient_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRecipient_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IRecipient_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IRecipient_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IRecipient_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IRecipient_put_Address(This,pwsz)	\
    (This)->lpVtbl -> put_Address(This,pwsz)

#define IRecipient_get_Address(This,ppwsz)	\
    (This)->lpVtbl -> get_Address(This,ppwsz)

#define IRecipient_get_Name(This,ppwsz)	\
    (This)->lpVtbl -> get_Name(This,ppwsz)

#define IRecipient_get_Application(This,polApp)	\
    (This)->lpVtbl -> get_Application(This,polApp)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* __IRecipient_INTERFACE_DEFINED__ */


#ifndef __IPOlRecipient_INTERFACE_DEFINED__
#define __IPOlRecipient_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IPOlRecipient
 * at Fri Aug 13 12:48:03 1999
 * using MIDL 3.02.88
 ****************************************/
/* [dual][full][helpstring][uuid][object] */ 



DEFINE_GUID(IID_IPOlRecipient, 0xA11C6E30, 0x51B5, 0x11d3, 0x8f, 0x39, 0x0, 0x0, 0xf8, 0x7a, 0x43, 0x35);

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("A11C6E30-51B5-11d3-8F39-0000F87A4335")
    IPOlRecipient : public IRecipient
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Resolve( 
            /* [defaultvalue][in] */ VARIANT_BOOL fShowDialog,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pfResolved) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPOlRecipientVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IPOlRecipient __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IPOlRecipient __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IPOlRecipient __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IPOlRecipient __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IPOlRecipient __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IPOlRecipient __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IPOlRecipient __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Address )( 
            IPOlRecipient __RPC_FAR * This,
            /* [in] */ BSTR pwsz);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Address )( 
            IPOlRecipient __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Name )( 
            IPOlRecipient __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Application )( 
            IPOlRecipient __RPC_FAR * This,
            /* [retval][out] */ IPOutlookApp __RPC_FAR *__RPC_FAR *polApp);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Resolve )( 
            IPOlRecipient __RPC_FAR * This,
            /* [defaultvalue][in] */ VARIANT_BOOL fShowDialog,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pfResolved);
        
        END_INTERFACE
    } IPOlRecipientVtbl;

    interface IPOlRecipient
    {
        CONST_VTBL struct IPOlRecipientVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPOlRecipient_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPOlRecipient_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPOlRecipient_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPOlRecipient_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IPOlRecipient_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IPOlRecipient_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IPOlRecipient_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IPOlRecipient_put_Address(This,pwsz)	\
    (This)->lpVtbl -> put_Address(This,pwsz)

#define IPOlRecipient_get_Address(This,ppwsz)	\
    (This)->lpVtbl -> get_Address(This,ppwsz)

#define IPOlRecipient_get_Name(This,ppwsz)	\
    (This)->lpVtbl -> get_Name(This,ppwsz)

#define IPOlRecipient_get_Application(This,polApp)	\
    (This)->lpVtbl -> get_Application(This,polApp)


#define IPOlRecipient_Resolve(This,fShowDialog,pfResolved)	\
    (This)->lpVtbl -> Resolve(This,fShowDialog,pfResolved)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* __IPOlRecipient_INTERFACE_DEFINED__ */


#ifndef __IRecipients_INTERFACE_DEFINED__
#define __IRecipients_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IRecipients
 * at Mon Dec 07 11:02:04 1998
 * using MIDL 3.02.88
 ****************************************/
/* [dual][full][helpstring][uuid][object] */ 



DEFINE_GUID(IID_IRecipients, 0x76065ae0, 0x2347, 0x11d2, 0x8f, 0x18, 0x0, 0x0, 0xf8, 0x7a, 0x43, 0x35);

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("76065AE0-2347-11d2-8F18-0000F87A4335")
    IRecipients : public IDispatch
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Add( 
            /* [in] */ BSTR pwszName,
            /* [retval][out] */ IRecipient __RPC_FAR *__RPC_FAR *pRecip) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Item( 
            /* [in] */ int iIndex,
            /* [retval][out] */ IRecipient __RPC_FAR *__RPC_FAR *pRecip) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Remove( 
            /* [in] */ int iIndex) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ int __RPC_FAR *pnCount) = 0;

        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppEnumerator) = 0;
                    
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Application( 
            /* [retval][out] */ IPOutlookApp __RPC_FAR *__RPC_FAR *polApp) = 0;

    };
    
#else 	/* C style interface */

    typedef struct IRecipientsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRecipients __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRecipients __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRecipients __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IRecipients __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IRecipients __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IRecipients __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IRecipients __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Add )( 
            IRecipients __RPC_FAR * This,
            /* [in] */ BSTR pwszName,
            /* [retval][out] */ IRecipient __RPC_FAR *__RPC_FAR *pRecip);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Item )( 
            IRecipients __RPC_FAR * This,
            /* [in] */ int iIndex,
            /* [retval][out] */ IRecipient __RPC_FAR *__RPC_FAR *pRecip);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Remove )( 
            IRecipients __RPC_FAR * This,
            /* [in] */ int iIndex);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Count )( 
            IRecipients __RPC_FAR * This,
            /* [retval][out] */ int __RPC_FAR *pnCount);

        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get__NewEnum )( 
            IRecipients __RPC_FAR * This,
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppEnumerator);
            
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Application )( 
            IRecipients __RPC_FAR * This,
            /* [retval][out] */ IPOutlookApp __RPC_FAR *__RPC_FAR *polApp);
        
        END_INTERFACE
    } IRecipientsVtbl;

    interface IRecipients
    {
        CONST_VTBL struct IRecipientsVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRecipients_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRecipients_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRecipients_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRecipients_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IRecipients_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IRecipients_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IRecipients_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IRecipients_Add(This,pwszName,pRecip)	\
    (This)->lpVtbl -> Add(This,pwszName,pRecip)

#define IRecipients_Item(This,iIndex,pRecip)	\
    (This)->lpVtbl -> Item(This,iIndex,pRecip)

#define IRecipients_Remove(This,iIndex)	\
    (This)->lpVtbl -> Remove(This,iIndex)

#define IRecipients_get_Count(This,pnCount)	\
    (This)->lpVtbl -> get_Count(This,pnCount)

#define IRecipients_get__NewEnum(This,ppEnumerator)	\
    (This)->lpVtbl -> get__NewEnum(This,ppEnumerator)

#define IRecipients_get_Application(This,polApp)	\
    (This)->lpVtbl -> get_Application(This,polApp)
    
#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* __IRecipients_INTERFACE_DEFINED__ */


#ifndef __IPOutlookItemCollection_INTERFACE_DEFINED__
#define __IPOutlookItemCollection_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IPOutlookItemCollection
 * at Mon Dec 07 11:02:04 1998
 * using MIDL 3.02.88
 ****************************************/
/* [dual][full][helpstring][uuid][object] */ 



DEFINE_GUID(IID_IPOutlookItemCollection, 0xf06748c0, 0x21a5, 0x11d2, 0x8f, 0x18, 0x0, 0x0, 0xf8, 0x7a, 0x43, 0x35);

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("F06748C0-21A5-11d2-8F18-0000F87A4335")
    IPOutlookItemCollection : public IDispatch
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Add( 
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppolItem) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ int __RPC_FAR *pnCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Find( 
            /* [in] */ BSTR pwszRestriction,
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppItem) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FindNext( 
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppItem) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Item( 
            /* [in] */ int iIndex,
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppolItem) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Remove( 
            /* [in] */ int iIndex) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Restrict( 
            /* [in] */ BSTR pwszRestriction,
            /* [retval][out] */ IPOutlookItemCollection __RPC_FAR *__RPC_FAR *ppolItems) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Sort( 
            /* [in] */ BSTR pwszProperty,
            /* [in] */ VARIANT_BOOL fDescending) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_IncludeRecurrences( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pfIncludeRecurrences) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_IncludeRecurrences( 
            /* [in] */ VARIANT_BOOL fIncludeRecurrences) = 0;

        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppEnumerator) = 0;
                    
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Application( 
            /* [retval][out] */ IPOutlookApp __RPC_FAR *__RPC_FAR *polApp) = 0;

    };
    
#else 	/* C style interface */

    typedef struct IPOutlookItemCollectionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IPOutlookItemCollection __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IPOutlookItemCollection __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IPOutlookItemCollection __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IPOutlookItemCollection __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IPOutlookItemCollection __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IPOutlookItemCollection __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IPOutlookItemCollection __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Add )( 
            IPOutlookItemCollection __RPC_FAR * This,
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppolItem);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Count )( 
            IPOutlookItemCollection __RPC_FAR * This,
            /* [retval][out] */ int __RPC_FAR *pnCount);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Find )( 
            IPOutlookItemCollection __RPC_FAR * This,
            /* [in] */ BSTR pwszRestriction,
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppItem);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FindNext )( 
            IPOutlookItemCollection __RPC_FAR * This,
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppItem);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Item )( 
            IPOutlookItemCollection __RPC_FAR * This,
            /* [in] */ int iIndex,
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppolItem);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Remove )( 
            IPOutlookItemCollection __RPC_FAR * This,
            /* [in] */ int iIndex);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Restrict )( 
            IPOutlookItemCollection __RPC_FAR * This,
            /* [in] */ BSTR pwszRestriction,
            /* [retval][out] */ IPOutlookItemCollection __RPC_FAR *__RPC_FAR *ppolItems);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Sort )( 
            IPOutlookItemCollection __RPC_FAR * This,
            /* [in] */ BSTR pwszProperty,
            /* [in] */ VARIANT_BOOL fDescending);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_IncludeRecurrences )( 
            IPOutlookItemCollection __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pfIncludeRecurrences);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_IncludeRecurrences )( 
            IPOutlookItemCollection __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL fIncludeRecurrences);

        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get__NewEnum )( 
            IPOutlookItemCollection __RPC_FAR * This,
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppEnumerator);
            
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Application )( 
            IPOutlookItemCollection __RPC_FAR * This,
            /* [retval][out] */ IPOutlookApp __RPC_FAR *__RPC_FAR *polApp);
        
        END_INTERFACE
    } IPOutlookItemCollectionVtbl;

    interface IPOutlookItemCollection
    {
        CONST_VTBL struct IPOutlookItemCollectionVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPOutlookItemCollection_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPOutlookItemCollection_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPOutlookItemCollection_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPOutlookItemCollection_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IPOutlookItemCollection_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IPOutlookItemCollection_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IPOutlookItemCollection_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IPOutlookItemCollection_Add(This,ppolItem)	\
    (This)->lpVtbl -> Add(This,ppolItem)

#define IPOutlookItemCollection_get_Count(This,pnCount)	\
    (This)->lpVtbl -> get_Count(This,pnCount)

#define IPOutlookItemCollection_Find(This,pwszRestriction,ppItem)	\
    (This)->lpVtbl -> Find(This,pwszRestriction,ppItem)

#define IPOutlookItemCollection_FindNext(This,ppItem)	\
    (This)->lpVtbl -> FindNext(This,ppItem)

#define IPOutlookItemCollection_Item(This,iIndex,ppolItem)	\
    (This)->lpVtbl -> Item(This,iIndex,ppolItem)

#define IPOutlookItemCollection_Remove(This,iIndex)	\
    (This)->lpVtbl -> Remove(This,iIndex)

#define IPOutlookItemCollection_Restrict(This,pwszRestriction,ppolItems)	\
    (This)->lpVtbl -> Restrict(This,pwszRestriction,ppolItems)

#define IPOutlookItemCollection_Sort(This,pwszProperty,fDescending)	\
    (This)->lpVtbl -> Sort(This,pwszProperty,fDescending)

#define IPOutlookItemCollection_get_IncludeRecurrences(This,pfIncludeRecurrences)	\
    (This)->lpVtbl -> get_IncludeRecurrences(This,pfIncludeRecurrences)

#define IPOutlookItemCollection_put_IncludeRecurrences(This,fIncludeRecurrences)	\
    (This)->lpVtbl -> put_IncludeRecurrences(This,fIncludeRecurrences)

#define IPOutlookItemCollection_get__NewEnum(This,ppEnumerator)	\
    (This)->lpVtbl -> get__NewEnum(This,ppEnumerator)
    
#define IPOutlookItemCollection_get_Application(This,polApp)	\
    (This)->lpVtbl -> get_Application(This,polApp)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* __IPOutlookItemCollection_INTERFACE_DEFINED__ */


#ifndef __IPOlItems_INTERFACE_DEFINED__
#define __IPOlItems_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IPOlItems
 * at Mon Aug 16 12:34:34 1999
 * using MIDL 3.02.88
 ****************************************/
/* [dual][full][helpstring][uuid][object] */ 


DEFINE_GUID(IID_IPOlItems, 0x6E3DBE90, 0x5411, 0x11d3, 0x8F, 0x39, 0x0, 0x0, 0xF8, 0x7A, 0x43, 0x35);

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("6E3DBE90-5411-11d3-8F39-0000F87A4335")
    IPOlItems : public IPOutlookItemCollection
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetColumns( 
            /* [in] */ BSTR Columns) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPOlItemsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IPOlItems __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IPOlItems __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IPOlItems __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IPOlItems __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IPOlItems __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IPOlItems __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IPOlItems __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Add )( 
            IPOlItems __RPC_FAR * This,
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppolItem);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Count )( 
            IPOlItems __RPC_FAR * This,
            /* [retval][out] */ int __RPC_FAR *pnCount);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Find )( 
            IPOlItems __RPC_FAR * This,
            /* [in] */ BSTR pwszRestriction,
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppItem);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FindNext )( 
            IPOlItems __RPC_FAR * This,
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppItem);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Item )( 
            IPOlItems __RPC_FAR * This,
            /* [in] */ int iIndex,
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppolItem);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Remove )( 
            IPOlItems __RPC_FAR * This,
            /* [in] */ int iIndex);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Restrict )( 
            IPOlItems __RPC_FAR * This,
            /* [in] */ BSTR pwszRestriction,
            /* [retval][out] */ IPOutlookItemCollection __RPC_FAR *__RPC_FAR *ppolItems);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Sort )( 
            IPOlItems __RPC_FAR * This,
            /* [in] */ BSTR pwszProperty,
            /* [defaultvalue][in] */ VARIANT_BOOL fDescending);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_IncludeRecurrences )( 
            IPOlItems __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pfIncludeRecurrences);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_IncludeRecurrences )( 
            IPOlItems __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL fIncludeRecurrences);
        
        /* [id][restricted][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get__NewEnum )( 
            IPOlItems __RPC_FAR * This,
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppEnumerator);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Application )( 
            IPOlItems __RPC_FAR * This,
            /* [retval][out] */ IPOutlookApp __RPC_FAR *__RPC_FAR *polApp);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetColumns )( 
            IPOlItems __RPC_FAR * This,
            /* [in] */ BSTR Columns);
        
        END_INTERFACE
    } IPOlItemsVtbl;

    interface IPOlItems
    {
        CONST_VTBL struct IPOlItemsVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPOlItems_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPOlItems_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPOlItems_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPOlItems_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IPOlItems_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IPOlItems_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IPOlItems_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IPOlItems_Add(This,ppolItem)	\
    (This)->lpVtbl -> Add(This,ppolItem)

#define IPOlItems_get_Count(This,pnCount)	\
    (This)->lpVtbl -> get_Count(This,pnCount)

#define IPOlItems_Find(This,pwszRestriction,ppItem)	\
    (This)->lpVtbl -> Find(This,pwszRestriction,ppItem)

#define IPOlItems_FindNext(This,ppItem)	\
    (This)->lpVtbl -> FindNext(This,ppItem)

#define IPOlItems_Item(This,iIndex,ppolItem)	\
    (This)->lpVtbl -> Item(This,iIndex,ppolItem)

#define IPOlItems_Remove(This,iIndex)	\
    (This)->lpVtbl -> Remove(This,iIndex)

#define IPOlItems_Restrict(This,pwszRestriction,ppolItems)	\
    (This)->lpVtbl -> Restrict(This,pwszRestriction,ppolItems)

#define IPOlItems_Sort(This,pwszProperty,fDescending)	\
    (This)->lpVtbl -> Sort(This,pwszProperty,fDescending)

#define IPOlItems_get_IncludeRecurrences(This,pfIncludeRecurrences)	\
    (This)->lpVtbl -> get_IncludeRecurrences(This,pfIncludeRecurrences)

#define IPOlItems_put_IncludeRecurrences(This,fIncludeRecurrences)	\
    (This)->lpVtbl -> put_IncludeRecurrences(This,fIncludeRecurrences)

#define IPOlItems_get__NewEnum(This,ppEnumerator)	\
    (This)->lpVtbl -> get__NewEnum(This,ppEnumerator)

#define IPOlItems_get_Application(This,polApp)	\
    (This)->lpVtbl -> get_Application(This,polApp)


#define IPOlItems_SetColumns(This,Columns)	\
    (This)->lpVtbl -> SetColumns(This,Columns)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* __IPOlItems_INTERFACE_DEFINED__ */

#ifndef __IContact_INTERFACE_DEFINED__
#define __IContact_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IContact
 * at Mon Dec 07 11:02:04 1998
 * using MIDL 3.02.88
 ****************************************/
/* [dual][full][helpstring][uuid][object] */ 


DEFINE_GUID(IID_IContact, 0x7f804e40, 0x2010, 0x11d2, 0x8f, 0x18, 0x0, 0x0, 0xf8, 0x7a, 0x43, 0x35);

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("7F804E40-2010-11d2-8F18-0000F87A4335")
    IContact : public IDispatch
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Birthday( 
            /* [retval][out] */ DATE __RPC_FAR *pst) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Anniversary( 
            /* [retval][out] */ DATE __RPC_FAR *pst) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_BusinessFaxNumber( 
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_CompanyName( 
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Department( 
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Email1Address( 
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_MobileTelephoneNumber( 
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_OfficeLocation( 
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_PagerNumber( 
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_BusinessTelephoneNumber( 
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_JobTitle( 
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_HomeTelephoneNumber( 
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Email2Address( 
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Spouse( 
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Email3Address( 
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Home2TelephoneNumber( 
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_HomeFaxNumber( 
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_CarTelephoneNumber( 
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_AssistantName( 
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_AssistantTelephoneNumber( 
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Children( 
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Categories( 
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_WebPage( 
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Business2TelephoneNumber( 
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Title( 
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_FirstName( 
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_MiddleName( 
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_LastName( 
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Suffix( 
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_HomeAddressStreet( 
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_HomeAddressCity( 
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_HomeAddressState( 
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_HomeAddressPostalCode( 
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_HomeAddressCountry( 
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_OtherAddressStreet( 
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_OtherAddressCity( 
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_OtherAddressState( 
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_OtherAddressPostalCode( 
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_OtherAddressCountry( 
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_BusinessAddressStreet( 
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_BusinessAddressCity( 
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_BusinessAddressState( 
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_BusinessAddressPostalCode( 
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_BusinessAddressCountry( 
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_RadioTelephoneNumber( 
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_FileAs( 
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Body( 
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_YomiCompanyName( 
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_YomiFirstName( 
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_YomiLastName( 
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Birthday( 
            /* [in] */ DATE st) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Anniversary( 
            /* [in] */ DATE st) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_BusinessFaxNumber( 
            /* [in] */ BSTR pwsz) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_CompanyName( 
            /* [in] */ BSTR pwsz) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Department( 
            /* [in] */ BSTR pwsz) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Email1Address( 
            /* [in] */ BSTR pwsz) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_MobileTelephoneNumber( 
            /* [in] */ BSTR pwsz) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_OfficeLocation( 
            /* [in] */ BSTR pwsz) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_PagerNumber( 
            /* [in] */ BSTR pwsz) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_BusinessTelephoneNumber( 
            /* [in] */ BSTR pwsz) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_JobTitle( 
            /* [in] */ BSTR pwsz) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_HomeTelephoneNumber( 
            /* [in] */ BSTR pwsz) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Email2Address( 
            /* [in] */ BSTR pwsz) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Spouse( 
            /* [in] */ BSTR pwsz) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Email3Address( 
            /* [in] */ BSTR pwsz) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Home2TelephoneNumber( 
            /* [in] */ BSTR pwsz) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_HomeFaxNumber( 
            /* [in] */ BSTR pwsz) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_CarTelephoneNumber( 
            /* [in] */ BSTR pwsz) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_AssistantName( 
            /* [in] */ BSTR pwsz) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_AssistantTelephoneNumber( 
            /* [in] */ BSTR pwsz) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Children( 
            /* [in] */ BSTR pwsz) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Categories( 
            /* [in] */ BSTR pwsz) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_WebPage( 
            /* [in] */ BSTR pwsz) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Business2TelephoneNumber( 
            /* [in] */ BSTR pwsz) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Title( 
            /* [in] */ BSTR pwsz) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_FirstName( 
            /* [in] */ BSTR pwsz) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_MiddleName( 
            /* [in] */ BSTR pwsz) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_LastName( 
            /* [in] */ BSTR pwsz) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Suffix( 
            /* [in] */ BSTR pwsz) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_HomeAddressStreet( 
            /* [in] */ BSTR pwsz) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_HomeAddressCity( 
            /* [in] */ BSTR pwsz) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_HomeAddressState( 
            /* [in] */ BSTR pwsz) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_HomeAddressPostalCode( 
            /* [in] */ BSTR pwsz) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_HomeAddressCountry( 
            /* [in] */ BSTR pwsz) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_OtherAddressStreet( 
            /* [in] */ BSTR pwsz) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_OtherAddressCity( 
            /* [in] */ BSTR pwsz) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_OtherAddressState( 
            /* [in] */ BSTR pwsz) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_OtherAddressPostalCode( 
            /* [in] */ BSTR pwsz) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_OtherAddressCountry( 
            /* [in] */ BSTR pwsz) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_BusinessAddressStreet( 
            /* [in] */ BSTR pwsz) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_BusinessAddressCity( 
            /* [in] */ BSTR pwsz) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_BusinessAddressState( 
            /* [in] */ BSTR pwsz) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_BusinessAddressPostalCode( 
            /* [in] */ BSTR pwsz) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_BusinessAddressCountry( 
            /* [in] */ BSTR pwsz) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_RadioTelephoneNumber( 
            /* [in] */ BSTR pwsz) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_FileAs( 
            /* [in] */ BSTR pwsz) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Body( 
            /* [in] */ BSTR pwsz) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_YomiCompanyName( 
            /* [in] */ BSTR pwsz) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_YomiFirstName( 
            /* [in] */ BSTR pwsz) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_YomiLastName( 
            /* [in] */ BSTR pwsz) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Save( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Delete( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Copy( 
            /* [retval][out] */ IContact __RPC_FAR *__RPC_FAR *ppolCopy) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Display( void) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Oid( 
            /* [retval][out] */ long __RPC_FAR *poid) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_BodyInk( 
            /* [in] */ CEBLOB *pcebl) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_BodyInk( 
            /* [retval][out] */ CEBLOB __RPC_FAR *pcebl) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Application( 
            /* [retval][out] */ IPOutlookApp __RPC_FAR *__RPC_FAR *polApp) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IContactVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IContact __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IContact __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IContact __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IContact __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IContact __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IContact __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IContact __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Birthday )( 
            IContact __RPC_FAR * This,
            /* [retval][out] */ DATE __RPC_FAR *pst);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Anniversary )( 
            IContact __RPC_FAR * This,
            /* [retval][out] */ DATE __RPC_FAR *pst);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_BusinessFaxNumber )( 
            IContact __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CompanyName )( 
            IContact __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Department )( 
            IContact __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Email1Address )( 
            IContact __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_MobileTelephoneNumber )( 
            IContact __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_OfficeLocation )( 
            IContact __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_PagerNumber )( 
            IContact __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_BusinessTelephoneNumber )( 
            IContact __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_JobTitle )( 
            IContact __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_HomeTelephoneNumber )( 
            IContact __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Email2Address )( 
            IContact __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Spouse )( 
            IContact __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Email3Address )( 
            IContact __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Home2TelephoneNumber )( 
            IContact __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_HomeFaxNumber )( 
            IContact __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CarTelephoneNumber )( 
            IContact __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_AssistantName )( 
            IContact __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_AssistantTelephoneNumber )( 
            IContact __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Children )( 
            IContact __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Categories )( 
            IContact __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_WebPage )( 
            IContact __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Business2TelephoneNumber )( 
            IContact __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Title )( 
            IContact __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_FirstName )( 
            IContact __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_MiddleName )( 
            IContact __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_LastName )( 
            IContact __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Suffix )( 
            IContact __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_HomeAddressStreet )( 
            IContact __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_HomeAddressCity )( 
            IContact __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_HomeAddressState )( 
            IContact __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_HomeAddressPostalCode )( 
            IContact __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_HomeAddressCountry )( 
            IContact __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_OtherAddressStreet )( 
            IContact __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_OtherAddressCity )( 
            IContact __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_OtherAddressState )( 
            IContact __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_OtherAddressPostalCode )( 
            IContact __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_OtherAddressCountry )( 
            IContact __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_BusinessAddressStreet )( 
            IContact __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_BusinessAddressCity )( 
            IContact __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_BusinessAddressState )( 
            IContact __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_BusinessAddressPostalCode )( 
            IContact __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_BusinessAddressCountry )( 
            IContact __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_RadioTelephoneNumber )( 
            IContact __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_FileAs )( 
            IContact __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Body )( 
            IContact __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_YomiCompanyName )( 
            IContact __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_YomiFirstName )( 
            IContact __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_YomiLastName )( 
            IContact __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Birthday )( 
            IContact __RPC_FAR * This,
            /* [in] */ DATE st);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Anniversary )( 
            IContact __RPC_FAR * This,
            /* [in] */ DATE st);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_BusinessFaxNumber )( 
            IContact __RPC_FAR * This,
            /* [in] */ BSTR pwsz);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_CompanyName )( 
            IContact __RPC_FAR * This,
            /* [in] */ BSTR pwsz);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Department )( 
            IContact __RPC_FAR * This,
            /* [in] */ BSTR pwsz);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Email1Address )( 
            IContact __RPC_FAR * This,
            /* [in] */ BSTR pwsz);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_MobileTelephoneNumber )( 
            IContact __RPC_FAR * This,
            /* [in] */ BSTR pwsz);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_OfficeLocation )( 
            IContact __RPC_FAR * This,
            /* [in] */ BSTR pwsz);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_PagerNumber )( 
            IContact __RPC_FAR * This,
            /* [in] */ BSTR pwsz);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_BusinessTelephoneNumber )( 
            IContact __RPC_FAR * This,
            /* [in] */ BSTR pwsz);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_JobTitle )( 
            IContact __RPC_FAR * This,
            /* [in] */ BSTR pwsz);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_HomeTelephoneNumber )( 
            IContact __RPC_FAR * This,
            /* [in] */ BSTR pwsz);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Email2Address )( 
            IContact __RPC_FAR * This,
            /* [in] */ BSTR pwsz);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Spouse )( 
            IContact __RPC_FAR * This,
            /* [in] */ BSTR pwsz);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Email3Address )( 
            IContact __RPC_FAR * This,
            /* [in] */ BSTR pwsz);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Home2TelephoneNumber )( 
            IContact __RPC_FAR * This,
            /* [in] */ BSTR pwsz);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_HomeFaxNumber )( 
            IContact __RPC_FAR * This,
            /* [in] */ BSTR pwsz);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_CarTelephoneNumber )( 
            IContact __RPC_FAR * This,
            /* [in] */ BSTR pwsz);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_AssistantName )( 
            IContact __RPC_FAR * This,
            /* [in] */ BSTR pwsz);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_AssistantTelephoneNumber )( 
            IContact __RPC_FAR * This,
            /* [in] */ BSTR pwsz);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Children )( 
            IContact __RPC_FAR * This,
            /* [in] */ BSTR pwsz);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Categories )( 
            IContact __RPC_FAR * This,
            /* [in] */ BSTR pwsz);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_WebPage )( 
            IContact __RPC_FAR * This,
            /* [in] */ BSTR pwsz);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Business2TelephoneNumber )( 
            IContact __RPC_FAR * This,
            /* [in] */ BSTR pwsz);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Title )( 
            IContact __RPC_FAR * This,
            /* [in] */ BSTR pwsz);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_FirstName )( 
            IContact __RPC_FAR * This,
            /* [in] */ BSTR pwsz);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_MiddleName )( 
            IContact __RPC_FAR * This,
            /* [in] */ BSTR pwsz);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_LastName )( 
            IContact __RPC_FAR * This,
            /* [in] */ BSTR pwsz);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Suffix )( 
            IContact __RPC_FAR * This,
            /* [in] */ BSTR pwsz);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_HomeAddressStreet )( 
            IContact __RPC_FAR * This,
            /* [in] */ BSTR pwsz);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_HomeAddressCity )( 
            IContact __RPC_FAR * This,
            /* [in] */ BSTR pwsz);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_HomeAddressState )( 
            IContact __RPC_FAR * This,
            /* [in] */ BSTR pwsz);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_HomeAddressPostalCode )( 
            IContact __RPC_FAR * This,
            /* [in] */ BSTR pwsz);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_HomeAddressCountry )( 
            IContact __RPC_FAR * This,
            /* [in] */ BSTR pwsz);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_OtherAddressStreet )( 
            IContact __RPC_FAR * This,
            /* [in] */ BSTR pwsz);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_OtherAddressCity )( 
            IContact __RPC_FAR * This,
            /* [in] */ BSTR pwsz);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_OtherAddressState )( 
            IContact __RPC_FAR * This,
            /* [in] */ BSTR pwsz);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_OtherAddressPostalCode )( 
            IContact __RPC_FAR * This,
            /* [in] */ BSTR pwsz);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_OtherAddressCountry )( 
            IContact __RPC_FAR * This,
            /* [in] */ BSTR pwsz);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_BusinessAddressStreet )( 
            IContact __RPC_FAR * This,
            /* [in] */ BSTR pwsz);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_BusinessAddressCity )( 
            IContact __RPC_FAR * This,
            /* [in] */ BSTR pwsz);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_BusinessAddressState )( 
            IContact __RPC_FAR * This,
            /* [in] */ BSTR pwsz);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_BusinessAddressPostalCode )( 
            IContact __RPC_FAR * This,
            /* [in] */ BSTR pwsz);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_BusinessAddressCountry )( 
            IContact __RPC_FAR * This,
            /* [in] */ BSTR pwsz);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_RadioTelephoneNumber )( 
            IContact __RPC_FAR * This,
            /* [in] */ BSTR pwsz);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_FileAs )( 
            IContact __RPC_FAR * This,
            /* [in] */ BSTR pwsz);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Body )( 
            IContact __RPC_FAR * This,
            /* [in] */ BSTR pwsz);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_YomiCompanyName )( 
            IContact __RPC_FAR * This,
            /* [in] */ BSTR pwsz);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_YomiFirstName )( 
            IContact __RPC_FAR * This,
            /* [in] */ BSTR pwsz);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_YomiLastName )( 
            IContact __RPC_FAR * This,
            /* [in] */ BSTR pwsz);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Save )( 
            IContact __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Delete )( 
            IContact __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Copy )( 
            IContact __RPC_FAR * This,
            /* [retval][out] */ IContact __RPC_FAR *__RPC_FAR *ppolCopy);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Display )( 
            IContact __RPC_FAR * This);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Oid )( 
            IContact __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *poid);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_BodyInk )( 
            IContact __RPC_FAR * This,
            /* [in] */ CEBLOB *pcebl);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_BodyInk )( 
            IContact __RPC_FAR * This,
            /* [retval][out] */ CEBLOB __RPC_FAR *pcebl);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Application )( 
            IContact __RPC_FAR * This,
            /* [retval][out] */ IPOutlookApp __RPC_FAR *__RPC_FAR *polApp);
        
        END_INTERFACE
    } IContactVtbl;

    interface IContact
    {
        CONST_VTBL struct IContactVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IContact_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IContact_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IContact_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IContact_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IContact_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IContact_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IContact_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IContact_get_Birthday(This,pst)	\
    (This)->lpVtbl -> get_Birthday(This,pst)

#define IContact_get_Anniversary(This,pst)	\
    (This)->lpVtbl -> get_Anniversary(This,pst)

#define IContact_get_BusinessFaxNumber(This,ppwsz)	\
    (This)->lpVtbl -> get_BusinessFaxNumber(This,ppwsz)

#define IContact_get_CompanyName(This,ppwsz)	\
    (This)->lpVtbl -> get_CompanyName(This,ppwsz)

#define IContact_get_Department(This,ppwsz)	\
    (This)->lpVtbl -> get_Department(This,ppwsz)

#define IContact_get_Email1Address(This,ppwsz)	\
    (This)->lpVtbl -> get_Email1Address(This,ppwsz)

#define IContact_get_MobileTelephoneNumber(This,ppwsz)	\
    (This)->lpVtbl -> get_MobileTelephoneNumber(This,ppwsz)

#define IContact_get_OfficeLocation(This,ppwsz)	\
    (This)->lpVtbl -> get_OfficeLocation(This,ppwsz)

#define IContact_get_PagerNumber(This,ppwsz)	\
    (This)->lpVtbl -> get_PagerNumber(This,ppwsz)

#define IContact_get_BusinessTelephoneNumber(This,ppwsz)	\
    (This)->lpVtbl -> get_BusinessTelephoneNumber(This,ppwsz)

#define IContact_get_JobTitle(This,ppwsz)	\
    (This)->lpVtbl -> get_JobTitle(This,ppwsz)

#define IContact_get_HomeTelephoneNumber(This,ppwsz)	\
    (This)->lpVtbl -> get_HomeTelephoneNumber(This,ppwsz)

#define IContact_get_Email2Address(This,ppwsz)	\
    (This)->lpVtbl -> get_Email2Address(This,ppwsz)

#define IContact_get_Spouse(This,ppwsz)	\
    (This)->lpVtbl -> get_Spouse(This,ppwsz)

#define IContact_get_Email3Address(This,ppwsz)	\
    (This)->lpVtbl -> get_Email3Address(This,ppwsz)

#define IContact_get_Home2TelephoneNumber(This,ppwsz)	\
    (This)->lpVtbl -> get_Home2TelephoneNumber(This,ppwsz)

#define IContact_get_HomeFaxNumber(This,ppwsz)	\
    (This)->lpVtbl -> get_HomeFaxNumber(This,ppwsz)

#define IContact_get_CarTelephoneNumber(This,ppwsz)	\
    (This)->lpVtbl -> get_CarTelephoneNumber(This,ppwsz)

#define IContact_get_AssistantName(This,ppwsz)	\
    (This)->lpVtbl -> get_AssistantName(This,ppwsz)

#define IContact_get_AssistantTelephoneNumber(This,ppwsz)	\
    (This)->lpVtbl -> get_AssistantTelephoneNumber(This,ppwsz)

#define IContact_get_Children(This,ppwsz)	\
    (This)->lpVtbl -> get_Children(This,ppwsz)

#define IContact_get_Categories(This,ppwsz)	\
    (This)->lpVtbl -> get_Categories(This,ppwsz)

#define IContact_get_WebPage(This,ppwsz)	\
    (This)->lpVtbl -> get_WebPage(This,ppwsz)

#define IContact_get_Business2TelephoneNumber(This,ppwsz)	\
    (This)->lpVtbl -> get_Business2TelephoneNumber(This,ppwsz)

#define IContact_get_Title(This,ppwsz)	\
    (This)->lpVtbl -> get_Title(This,ppwsz)

#define IContact_get_FirstName(This,ppwsz)	\
    (This)->lpVtbl -> get_FirstName(This,ppwsz)

#define IContact_get_MiddleName(This,ppwsz)	\
    (This)->lpVtbl -> get_MiddleName(This,ppwsz)

#define IContact_get_LastName(This,ppwsz)	\
    (This)->lpVtbl -> get_LastName(This,ppwsz)

#define IContact_get_Suffix(This,ppwsz)	\
    (This)->lpVtbl -> get_Suffix(This,ppwsz)

#define IContact_get_HomeAddressStreet(This,ppwsz)	\
    (This)->lpVtbl -> get_HomeAddressStreet(This,ppwsz)

#define IContact_get_HomeAddressCity(This,ppwsz)	\
    (This)->lpVtbl -> get_HomeAddressCity(This,ppwsz)

#define IContact_get_HomeAddressState(This,ppwsz)	\
    (This)->lpVtbl -> get_HomeAddressState(This,ppwsz)

#define IContact_get_HomeAddressPostalCode(This,ppwsz)	\
    (This)->lpVtbl -> get_HomeAddressPostalCode(This,ppwsz)

#define IContact_get_HomeAddressCountry(This,ppwsz)	\
    (This)->lpVtbl -> get_HomeAddressCountry(This,ppwsz)

#define IContact_get_OtherAddressStreet(This,ppwsz)	\
    (This)->lpVtbl -> get_OtherAddressStreet(This,ppwsz)

#define IContact_get_OtherAddressCity(This,ppwsz)	\
    (This)->lpVtbl -> get_OtherAddressCity(This,ppwsz)

#define IContact_get_OtherAddressState(This,ppwsz)	\
    (This)->lpVtbl -> get_OtherAddressState(This,ppwsz)

#define IContact_get_OtherAddressPostalCode(This,ppwsz)	\
    (This)->lpVtbl -> get_OtherAddressPostalCode(This,ppwsz)

#define IContact_get_OtherAddressCountry(This,ppwsz)	\
    (This)->lpVtbl -> get_OtherAddressCountry(This,ppwsz)

#define IContact_get_BusinessAddressStreet(This,ppwsz)	\
    (This)->lpVtbl -> get_BusinessAddressStreet(This,ppwsz)

#define IContact_get_BusinessAddressCity(This,ppwsz)	\
    (This)->lpVtbl -> get_BusinessAddressCity(This,ppwsz)

#define IContact_get_BusinessAddressState(This,ppwsz)	\
    (This)->lpVtbl -> get_BusinessAddressState(This,ppwsz)

#define IContact_get_BusinessAddressPostalCode(This,ppwsz)	\
    (This)->lpVtbl -> get_BusinessAddressPostalCode(This,ppwsz)

#define IContact_get_BusinessAddressCountry(This,ppwsz)	\
    (This)->lpVtbl -> get_BusinessAddressCountry(This,ppwsz)

#define IContact_get_RadioTelephoneNumber(This,ppwsz)	\
    (This)->lpVtbl -> get_RadioTelephoneNumber(This,ppwsz)

#define IContact_get_FileAs(This,ppwsz)	\
    (This)->lpVtbl -> get_FileAs(This,ppwsz)

#define IContact_get_Body(This,ppwsz)	\
    (This)->lpVtbl -> get_Body(This,ppwsz)

#define IContact_get_YomiCompanyName(This,ppwsz)	\
    (This)->lpVtbl -> get_YomiCompanyName(This,ppwsz)

#define IContact_get_YomiFirstName(This,ppwsz)	\
    (This)->lpVtbl -> get_YomiFirstName(This,ppwsz)

#define IContact_get_YomiLastName(This,ppwsz)	\
    (This)->lpVtbl -> get_YomiLastName(This,ppwsz)

#define IContact_put_Birthday(This,st)	\
    (This)->lpVtbl -> put_Birthday(This,st)

#define IContact_put_Anniversary(This,st)	\
    (This)->lpVtbl -> put_Anniversary(This,st)

#define IContact_put_BusinessFaxNumber(This,pwsz)	\
    (This)->lpVtbl -> put_BusinessFaxNumber(This,pwsz)

#define IContact_put_CompanyName(This,pwsz)	\
    (This)->lpVtbl -> put_CompanyName(This,pwsz)

#define IContact_put_Department(This,pwsz)	\
    (This)->lpVtbl -> put_Department(This,pwsz)

#define IContact_put_Email1Address(This,pwsz)	\
    (This)->lpVtbl -> put_Email1Address(This,pwsz)

#define IContact_put_MobileTelephoneNumber(This,pwsz)	\
    (This)->lpVtbl -> put_MobileTelephoneNumber(This,pwsz)

#define IContact_put_OfficeLocation(This,pwsz)	\
    (This)->lpVtbl -> put_OfficeLocation(This,pwsz)

#define IContact_put_PagerNumber(This,pwsz)	\
    (This)->lpVtbl -> put_PagerNumber(This,pwsz)

#define IContact_put_BusinessTelephoneNumber(This,pwsz)	\
    (This)->lpVtbl -> put_BusinessTelephoneNumber(This,pwsz)

#define IContact_put_JobTitle(This,pwsz)	\
    (This)->lpVtbl -> put_JobTitle(This,pwsz)

#define IContact_put_HomeTelephoneNumber(This,pwsz)	\
    (This)->lpVtbl -> put_HomeTelephoneNumber(This,pwsz)

#define IContact_put_Email2Address(This,pwsz)	\
    (This)->lpVtbl -> put_Email2Address(This,pwsz)

#define IContact_put_Spouse(This,pwsz)	\
    (This)->lpVtbl -> put_Spouse(This,pwsz)

#define IContact_put_Email3Address(This,pwsz)	\
    (This)->lpVtbl -> put_Email3Address(This,pwsz)

#define IContact_put_Home2TelephoneNumber(This,pwsz)	\
    (This)->lpVtbl -> put_Home2TelephoneNumber(This,pwsz)

#define IContact_put_HomeFaxNumber(This,pwsz)	\
    (This)->lpVtbl -> put_HomeFaxNumber(This,pwsz)

#define IContact_put_CarTelephoneNumber(This,pwsz)	\
    (This)->lpVtbl -> put_CarTelephoneNumber(This,pwsz)

#define IContact_put_AssistantName(This,pwsz)	\
    (This)->lpVtbl -> put_AssistantName(This,pwsz)

#define IContact_put_AssistantTelephoneNumber(This,pwsz)	\
    (This)->lpVtbl -> put_AssistantTelephoneNumber(This,pwsz)

#define IContact_put_Children(This,pwsz)	\
    (This)->lpVtbl -> put_Children(This,pwsz)

#define IContact_put_Categories(This,pwsz)	\
    (This)->lpVtbl -> put_Categories(This,pwsz)

#define IContact_put_WebPage(This,pwsz)	\
    (This)->lpVtbl -> put_WebPage(This,pwsz)

#define IContact_put_Business2TelephoneNumber(This,pwsz)	\
    (This)->lpVtbl -> put_Business2TelephoneNumber(This,pwsz)

#define IContact_put_Title(This,pwsz)	\
    (This)->lpVtbl -> put_Title(This,pwsz)

#define IContact_put_FirstName(This,pwsz)	\
    (This)->lpVtbl -> put_FirstName(This,pwsz)

#define IContact_put_MiddleName(This,pwsz)	\
    (This)->lpVtbl -> put_MiddleName(This,pwsz)

#define IContact_put_LastName(This,pwsz)	\
    (This)->lpVtbl -> put_LastName(This,pwsz)

#define IContact_put_Suffix(This,pwsz)	\
    (This)->lpVtbl -> put_Suffix(This,pwsz)

#define IContact_put_HomeAddressStreet(This,pwsz)	\
    (This)->lpVtbl -> put_HomeAddressStreet(This,pwsz)

#define IContact_put_HomeAddressCity(This,pwsz)	\
    (This)->lpVtbl -> put_HomeAddressCity(This,pwsz)

#define IContact_put_HomeAddressState(This,pwsz)	\
    (This)->lpVtbl -> put_HomeAddressState(This,pwsz)

#define IContact_put_HomeAddressPostalCode(This,pwsz)	\
    (This)->lpVtbl -> put_HomeAddressPostalCode(This,pwsz)

#define IContact_put_HomeAddressCountry(This,pwsz)	\
    (This)->lpVtbl -> put_HomeAddressCountry(This,pwsz)

#define IContact_put_OtherAddressStreet(This,pwsz)	\
    (This)->lpVtbl -> put_OtherAddressStreet(This,pwsz)

#define IContact_put_OtherAddressCity(This,pwsz)	\
    (This)->lpVtbl -> put_OtherAddressCity(This,pwsz)

#define IContact_put_OtherAddressState(This,pwsz)	\
    (This)->lpVtbl -> put_OtherAddressState(This,pwsz)

#define IContact_put_OtherAddressPostalCode(This,pwsz)	\
    (This)->lpVtbl -> put_OtherAddressPostalCode(This,pwsz)

#define IContact_put_OtherAddressCountry(This,pwsz)	\
    (This)->lpVtbl -> put_OtherAddressCountry(This,pwsz)

#define IContact_put_BusinessAddressStreet(This,pwsz)	\
    (This)->lpVtbl -> put_BusinessAddressStreet(This,pwsz)

#define IContact_put_BusinessAddressCity(This,pwsz)	\
    (This)->lpVtbl -> put_BusinessAddressCity(This,pwsz)

#define IContact_put_BusinessAddressState(This,pwsz)	\
    (This)->lpVtbl -> put_BusinessAddressState(This,pwsz)

#define IContact_put_BusinessAddressPostalCode(This,pwsz)	\
    (This)->lpVtbl -> put_BusinessAddressPostalCode(This,pwsz)

#define IContact_put_BusinessAddressCountry(This,pwsz)	\
    (This)->lpVtbl -> put_BusinessAddressCountry(This,pwsz)

#define IContact_put_RadioTelephoneNumber(This,pwsz)	\
    (This)->lpVtbl -> put_RadioTelephoneNumber(This,pwsz)

#define IContact_put_FileAs(This,pwsz)	\
    (This)->lpVtbl -> put_FileAs(This,pwsz)

#define IContact_put_Body(This,pwsz)	\
    (This)->lpVtbl -> put_Body(This,pwsz)

#define IContact_put_YomiCompanyName(This,pwsz)	\
    (This)->lpVtbl -> put_YomiCompanyName(This,pwsz)

#define IContact_put_YomiFirstName(This,pwsz)	\
    (This)->lpVtbl -> put_YomiFirstName(This,pwsz)

#define IContact_put_YomiLastName(This,pwsz)	\
    (This)->lpVtbl -> put_YomiLastName(This,pwsz)

#define IContact_Save(This)	\
    (This)->lpVtbl -> Save(This)

#define IContact_Delete(This)	\
    (This)->lpVtbl -> Delete(This)

#define IContact_Copy(This,ppolCopy)	\
    (This)->lpVtbl -> Copy(This,ppolCopy)

#define IContact_Display(This)	\
    (This)->lpVtbl -> Display(This)

#define IContact_get_Oid(This,poid)	\
    (This)->lpVtbl -> get_Oid(This,poid)

#define IContact_put_BodyInk(This,pcebl)	\
    (This)->lpVtbl -> put_BodyInk(This,pcebl)

#define IContact_get_BodyInk(This,pcebl)	\
    (This)->lpVtbl -> get_BodyInk(This,pcebl)

#define IContact_get_Application(This,polApp)	\
    (This)->lpVtbl -> get_Application(This,polApp)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* __IContact_INTERFACE_DEFINED__ */


#ifndef __ITask_INTERFACE_DEFINED__
#define __ITask_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ITask
 * at Mon Dec 07 11:02:04 1998
 * using MIDL 3.02.88
 ****************************************/
/* [dual][full][helpstring][uuid][object] */ 



DEFINE_GUID(IID_ITask, 0x37c78ce0, 0x202c, 0x11d2, 0x8f, 0x18, 0x0, 0x0, 0xf8, 0x7a, 0x43, 0x35);

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("37C78CE0-202C-11d2-8F18-0000F87A4335")
    ITask : public IDispatch
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ClearRecurrencePattern( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRecurrencePattern( 
            /* [retval][out] */ IRecurrencePattern __RPC_FAR *__RPC_FAR *ppRecPattern) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_IsRecurring( 
            VARIANT_BOOL __RPC_FAR *pfIsRecurring) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Subject( 
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Categories( 
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_StartDate( 
            /* [retval][out] */ DATE __RPC_FAR *pst) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_DueDate( 
            /* [retval][out] */ DATE __RPC_FAR *pst) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_DateCompleted( 
            /* [retval][out] */ DATE __RPC_FAR *pst) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Importance( 
            /* [retval][out] */ long __RPC_FAR *pdwPriority) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Complete( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pfCompleted) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Sensitivity( 
            /* [retval][out] */ long __RPC_FAR *plSensitivity) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_TeamTask( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pfTeamTask) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Body( 
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ReminderSet( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pfReminderSet) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ReminderSoundFile( 
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ReminderTime( 
            /* [retval][out] */ DATE __RPC_FAR *pst) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ReminderOptions( 
            /* [retval][out] */ long __RPC_FAR *pdwOptions) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Subject( 
            /* [in] */ BSTR pwsz) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Categories( 
            /* [in] */ BSTR pwsz) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_StartDate( 
            /* [in] */ DATE st) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_DueDate( 
            /* [in] */ DATE st) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Importance( 
            /* [in] */ long dwPriority) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Complete( 
            /* [in] */ VARIANT_BOOL fCompleted) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Sensitivity( 
            /* [in] */ long lSensitivity) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_TeamTask( 
            /* [in] */ VARIANT_BOOL fTeamTask) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Body( 
            /* [in] */ BSTR pwsz) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_ReminderSet( 
            /* [in] */ VARIANT_BOOL fReminderSet) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_ReminderSoundFile( 
            /* [in] */ BSTR pwsz) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_ReminderTime( 
            /* [in] */ DATE st) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_ReminderOptions( 
            /* [in] */ long dwOptions) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Save( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Delete( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SkipRecurrence( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Copy( 
            /* [retval][out] */ ITask __RPC_FAR *__RPC_FAR *ppolCopy) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Display( void) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Oid( 
            /* [retval][out] */ long __RPC_FAR *poid) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_BodyInk( 
            /* [in] */ CEBLOB *pcebl) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_BodyInk( 
            /* [retval][out] */ CEBLOB __RPC_FAR *pcebl) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Application( 
            /* [retval][out] */ IPOutlookApp __RPC_FAR *__RPC_FAR *polApp) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITaskVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITask __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITask __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITask __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ITask __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ITask __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ITask __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ITask __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ClearRecurrencePattern )( 
            ITask __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetRecurrencePattern )( 
            ITask __RPC_FAR * This,
            /* [retval][out] */ IRecurrencePattern __RPC_FAR *__RPC_FAR *ppRecPattern);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_IsRecurring )( 
            ITask __RPC_FAR * This,
            VARIANT_BOOL __RPC_FAR *pfIsRecurring);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Subject )( 
            ITask __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Categories )( 
            ITask __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_StartDate )( 
            ITask __RPC_FAR * This,
            /* [retval][out] */ DATE __RPC_FAR *pst);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DueDate )( 
            ITask __RPC_FAR * This,
            /* [retval][out] */ DATE __RPC_FAR *pst);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DateCompleted )( 
            ITask __RPC_FAR * This,
            /* [retval][out] */ DATE __RPC_FAR *pst);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Importance )( 
            ITask __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pdwPriority);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Complete )( 
            ITask __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pfCompleted);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Sensitivity )( 
            ITask __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plSensitivity);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_TeamTask )( 
            ITask __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pfTeamTask);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Body )( 
            ITask __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ReminderSet )( 
            ITask __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pfReminderSet);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ReminderSoundFile )( 
            ITask __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ReminderTime )( 
            ITask __RPC_FAR * This,
            /* [retval][out] */ DATE __RPC_FAR *pst);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ReminderOptions )( 
            ITask __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pdwOptions);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Subject )( 
            ITask __RPC_FAR * This,
            /* [in] */ BSTR pwsz);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Categories )( 
            ITask __RPC_FAR * This,
            /* [in] */ BSTR pwsz);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_StartDate )( 
            ITask __RPC_FAR * This,
            /* [in] */ DATE st);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_DueDate )( 
            ITask __RPC_FAR * This,
            /* [in] */ DATE st);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Importance )( 
            ITask __RPC_FAR * This,
            /* [in] */ long dwPriority);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Complete )( 
            ITask __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL fCompleted);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Sensitivity )( 
            ITask __RPC_FAR * This,
            /* [in] */ long lSensitivity);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_TeamTask )( 
            ITask __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL fTeamTask);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Body )( 
            ITask __RPC_FAR * This,
            /* [in] */ BSTR pwsz);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ReminderSet )( 
            ITask __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL fReminderSet);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ReminderSoundFile )( 
            ITask __RPC_FAR * This,
            /* [in] */ BSTR pwsz);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ReminderTime )( 
            ITask __RPC_FAR * This,
            /* [in] */ DATE st);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ReminderOptions )( 
            ITask __RPC_FAR * This,
            /* [in] */ long dwOptions);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Save )( 
            ITask __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Delete )( 
            ITask __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SkipRecurrence )( 
            ITask __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Copy )( 
            ITask __RPC_FAR * This,
            /* [retval][out] */ ITask __RPC_FAR *__RPC_FAR *ppolCopy);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Display )( 
            ITask __RPC_FAR * This);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Oid )( 
            ITask __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *poid);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_BodyInk )( 
            ITask __RPC_FAR * This,
            /* [in] */ CEBLOB *pcebl);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_BodyInk )( 
            ITask __RPC_FAR * This,
            /* [retval][out] */ CEBLOB __RPC_FAR *pcebl);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Application )( 
            ITask __RPC_FAR * This,
            /* [retval][out] */ IPOutlookApp __RPC_FAR *__RPC_FAR *polApp);
        
        END_INTERFACE
    } ITaskVtbl;

    interface ITask
    {
        CONST_VTBL struct ITaskVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITask_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITask_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITask_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITask_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITask_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITask_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITask_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITask_ClearRecurrencePattern(This)	\
    (This)->lpVtbl -> ClearRecurrencePattern(This)

#define ITask_GetRecurrencePattern(This,ppRecPattern)	\
    (This)->lpVtbl -> GetRecurrencePattern(This,ppRecPattern)

#define ITask_get_IsRecurring(This,pfIsRecurring)	\
    (This)->lpVtbl -> get_IsRecurring(This,pfIsRecurring)

#define ITask_get_Subject(This,ppwsz)	\
    (This)->lpVtbl -> get_Subject(This,ppwsz)

#define ITask_get_Categories(This,ppwsz)	\
    (This)->lpVtbl -> get_Categories(This,ppwsz)

#define ITask_get_StartDate(This,pst)	\
    (This)->lpVtbl -> get_StartDate(This,pst)

#define ITask_get_DueDate(This,pst)	\
    (This)->lpVtbl -> get_DueDate(This,pst)

#define ITask_get_DateCompleted(This,pst)	\
    (This)->lpVtbl -> get_DateCompleted(This,pst)

#define ITask_get_Importance(This,pdwPriority)	\
    (This)->lpVtbl -> get_Importance(This,pdwPriority)

#define ITask_get_Complete(This,pfCompleted)	\
    (This)->lpVtbl -> get_Complete(This,pfCompleted)

#define ITask_get_Sensitivity(This,plSensitivity)	\
    (This)->lpVtbl -> get_Sensitivity(This,plSensitivity)

#define ITask_get_TeamTask(This,pfTeamTask)	\
    (This)->lpVtbl -> get_TeamTask(This,pfTeamTask)

#define ITask_get_Body(This,ppwsz)	\
    (This)->lpVtbl -> get_Body(This,ppwsz)

#define ITask_get_ReminderSet(This,pfReminderSet)	\
    (This)->lpVtbl -> get_ReminderSet(This,pfReminderSet)

#define ITask_get_ReminderSoundFile(This,ppwsz)	\
    (This)->lpVtbl -> get_ReminderSoundFile(This,ppwsz)

#define ITask_get_ReminderTime(This,pst)	\
    (This)->lpVtbl -> get_ReminderTime(This,pst)

#define ITask_get_ReminderOptions(This,pdwOptions)	\
    (This)->lpVtbl -> get_ReminderOptions(This,pdwOptions)

#define ITask_put_Subject(This,pwsz)	\
    (This)->lpVtbl -> put_Subject(This,pwsz)

#define ITask_put_Categories(This,pwsz)	\
    (This)->lpVtbl -> put_Categories(This,pwsz)

#define ITask_put_StartDate(This,st)	\
    (This)->lpVtbl -> put_StartDate(This,st)

#define ITask_put_DueDate(This,st)	\
    (This)->lpVtbl -> put_DueDate(This,st)

#define ITask_put_Importance(This,dwPriority)	\
    (This)->lpVtbl -> put_Importance(This,dwPriority)

#define ITask_put_Complete(This,fCompleted)	\
    (This)->lpVtbl -> put_Complete(This,fCompleted)

#define ITask_put_Sensitivity(This,lSensitivity)	\
    (This)->lpVtbl -> put_Sensitivity(This,lSensitivity)

#define ITask_put_TeamTask(This,fTeamTask)	\
    (This)->lpVtbl -> put_TeamTask(This,fTeamTask)

#define ITask_put_Body(This,pwsz)	\
    (This)->lpVtbl -> put_Body(This,pwsz)

#define ITask_put_ReminderSet(This,fReminderSet)	\
    (This)->lpVtbl -> put_ReminderSet(This,fReminderSet)

#define ITask_put_ReminderSoundFile(This,pwsz)	\
    (This)->lpVtbl -> put_ReminderSoundFile(This,pwsz)

#define ITask_put_ReminderTime(This,st)	\
    (This)->lpVtbl -> put_ReminderTime(This,st)

#define ITask_put_ReminderOptions(This,dwOptions)	\
    (This)->lpVtbl -> put_ReminderOptions(This,dwOptions)

#define ITask_Save(This)	\
    (This)->lpVtbl -> Save(This)

#define ITask_Delete(This)	\
    (This)->lpVtbl -> Delete(This)

#define ITask_SkipRecurrence(This)	\
    (This)->lpVtbl -> SkipRecurrence(This)

#define ITask_Copy(This,ppolCopy)	\
    (This)->lpVtbl -> Copy(This,ppolCopy)

#define ITask_Display(This)	\
    (This)->lpVtbl -> Display(This)

#define ITask_get_Oid(This,poid)	\
    (This)->lpVtbl -> get_Oid(This,poid)

#define ITask_put_BodyInk(This,pcebl)	\
    (This)->lpVtbl -> put_BodyInk(This,pcebl)

#define ITask_get_BodyInk(This,pcebl)	\
    (This)->lpVtbl -> get_BodyInk(This,pcebl)

#define ITask_get_Application(This,polApp)	\
    (This)->lpVtbl -> get_Application(This,polApp)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* __ITask_INTERFACE_DEFINED__ */


#ifndef __IAppointment_INTERFACE_DEFINED__
#define __IAppointment_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IAppointment
 * at Mon Dec 07 11:02:04 1998
 * using MIDL 3.02.88
 ****************************************/
/* [dual][full][helpstring][uuid][object] */ 



DEFINE_GUID(IID_IAppointment, 0x5b43f691, 0x202c, 0x11d2, 0x8f, 0x18, 0x0, 0x0, 0xf8, 0x7a, 0x43, 0x35);

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("5B43F691-202C-11d2-8F18-0000F87A4335")
    IAppointment : public IDispatch
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ClearRecurrencePattern( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRecurrencePattern( 
            /* [retval][out] */ IRecurrencePattern __RPC_FAR *__RPC_FAR *ppRecPattern) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_IsRecurring( 
            VARIANT_BOOL __RPC_FAR *pfIsRecurring) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Subject( 
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Location( 
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Categories( 
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Start( 
            /* [retval][out] */ DATE __RPC_FAR *pst) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Duration( 
            /* [retval][out] */ long __RPC_FAR *pnLen) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_End( 
            /* [retval][out] */ DATE __RPC_FAR *pst) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_AllDayEvent( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pfAllDay) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_BusyStatus( 
            /* [retval][out] */ long __RPC_FAR *pnState) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Sensitivity( 
            /* [retval][out] */ long __RPC_FAR *plSensitivity) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Body( 
            /* [retval][out] */ BSTR __RPC_FAR *pwsz) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Recipients( 
            /* [retval][out] */ IRecipients __RPC_FAR *__RPC_FAR *pRecipients) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_MeetingStatus( 
            /* [retval][out] */ long __RPC_FAR *pnStatus) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ReminderSet( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pfReminderSet) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ReminderSoundFile( 
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ReminderMinutesBeforeStart( 
            /* [retval][out] */ long __RPC_FAR *plMinutes) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ReminderOptions( 
            /* [retval][out] */ long __RPC_FAR *pdwOptions) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Subject( 
            BSTR pwsz) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Location( 
            BSTR pwsz) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Categories( 
            BSTR pwsz) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Start( 
            DATE st) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Duration( 
            long nLen) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_End( 
            DATE st) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_AllDayEvent( 
            VARIANT_BOOL fAllDay) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_BusyStatus( 
            long nState) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Sensitivity( 
            long lSensitivity) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Body( 
            BSTR pwsz) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_ReminderSet( 
            VARIANT_BOOL fReminderSet) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_ReminderSoundFile( 
            BSTR pwsz) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_ReminderMinutesBeforeStart( 
            long lMinutes) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_ReminderOptions( 
            long dwOptions) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Save( void) = 0;

        virtual HRESULT STDMETHODCALLTYPE Send( void) = 0;

        virtual HRESULT STDMETHODCALLTYPE Delete( void) = 0;

        virtual HRESULT STDMETHODCALLTYPE Cancel( void) = 0;

        virtual HRESULT STDMETHODCALLTYPE Copy( 
            /* [retval][out] */ IAppointment __RPC_FAR *__RPC_FAR *ppolCopy) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Display( void) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Oid( 
            /* [retval][out] */ long __RPC_FAR *poid) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_BodyInk( 
            /* [in] */ CEBLOB *pcebl) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_BodyInk( 
            /* [retval][out] */ CEBLOB __RPC_FAR *pcebl) = 0;
                
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Application( 
            /* [retval][out] */ IPOutlookApp __RPC_FAR *__RPC_FAR *polApp) = 0;

    };
    
#else 	/* C style interface */

    typedef struct IAppointmentVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IAppointment __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IAppointment __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IAppointment __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IAppointment __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IAppointment __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IAppointment __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IAppointment __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ClearRecurrencePattern )( 
            IAppointment __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetRecurrencePattern )( 
            IAppointment __RPC_FAR * This,
            /* [retval][out] */ IRecurrencePattern __RPC_FAR *__RPC_FAR *ppRecPattern);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_IsRecurring )( 
            IAppointment __RPC_FAR * This,
            VARIANT_BOOL __RPC_FAR *pfIsRecurring);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Subject )( 
            IAppointment __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Location )( 
            IAppointment __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Categories )( 
            IAppointment __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Start )( 
            IAppointment __RPC_FAR * This,
            /* [retval][out] */ DATE __RPC_FAR *pst);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Duration )( 
            IAppointment __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pnLen);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_End )( 
            IAppointment __RPC_FAR * This,
            /* [retval][out] */ DATE __RPC_FAR *pst);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_AllDayEvent )( 
            IAppointment __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pfAllDay);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_BusyStatus )( 
            IAppointment __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pnState);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Sensitivity )( 
            IAppointment __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plSensitivity);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Body )( 
            IAppointment __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pwsz);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Recipients )( 
            IAppointment __RPC_FAR * This,
            /* [retval][out] */ IRecipients __RPC_FAR *__RPC_FAR *pRecipients);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_MeetingStatus )( 
            IAppointment __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pnStatus);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ReminderSet )( 
            IAppointment __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pfReminderSet);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ReminderSoundFile )( 
            IAppointment __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ReminderMinutesBeforeStart )( 
            IAppointment __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plMinutes);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ReminderOptions )( 
            IAppointment __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pdwOptions);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Subject )( 
            IAppointment __RPC_FAR * This,
            BSTR pwsz);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Location )( 
            IAppointment __RPC_FAR * This,
            BSTR pwsz);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Categories )( 
            IAppointment __RPC_FAR * This,
            BSTR pwsz);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Start )( 
            IAppointment __RPC_FAR * This,
            DATE st);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Duration )( 
            IAppointment __RPC_FAR * This,
            long nLen);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_End )( 
            IAppointment __RPC_FAR * This,
            DATE st);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_AllDayEvent )( 
            IAppointment __RPC_FAR * This,
            VARIANT_BOOL fAllDay);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_BusyStatus )( 
            IAppointment __RPC_FAR * This,
            long nState);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Sensitivity )( 
            IAppointment __RPC_FAR * This,
            long lSensitivity);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Body )( 
            IAppointment __RPC_FAR * This,
            BSTR pwsz);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ReminderSet )( 
            IAppointment __RPC_FAR * This,
            VARIANT_BOOL fReminderSet);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ReminderSoundFile )( 
            IAppointment __RPC_FAR * This,
            BSTR pwsz);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ReminderMinutesBeforeStart )( 
            IAppointment __RPC_FAR * This,
            long lMinutes);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ReminderOptions )( 
            IAppointment __RPC_FAR * This,
            long dwOptions);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Save )( 
            IAppointment __RPC_FAR * This);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Send )( 
            IAppointment __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Delete )( 
            IAppointment __RPC_FAR * This);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Cancel )( 
            IAppointment __RPC_FAR * This);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Copy )( 
            IAppointment __RPC_FAR * This,
            /* [retval][out] */ IAppointment __RPC_FAR *__RPC_FAR *ppolCopy);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Display )( 
            IAppointment __RPC_FAR * This);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Oid )( 
            IAppointment __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *poid);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_BodyInk )( 
            IAppointment __RPC_FAR * This,
            /* [in] */ CEBLOB *pcebl);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_BodyInk )( 
            IAppointment __RPC_FAR * This,
            /* [retval][out] */ CEBLOB __RPC_FAR *pcebl);
             
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Application )( 
            IAppointment __RPC_FAR * This,
            /* [retval][out] */ IPOutlookApp __RPC_FAR *__RPC_FAR *polApp);
   
        END_INTERFACE
    } IAppointmentVtbl;

    interface IAppointment
    {
        CONST_VTBL struct IAppointmentVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAppointment_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAppointment_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAppointment_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAppointment_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IAppointment_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IAppointment_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IAppointment_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IAppointment_ClearRecurrencePattern(This)	\
    (This)->lpVtbl -> ClearRecurrencePattern(This)

#define IAppointment_GetRecurrencePattern(This,ppRecPattern)	\
    (This)->lpVtbl -> GetRecurrencePattern(This,ppRecPattern)

#define IAppointment_get_IsRecurring(This,pfIsRecurring)	\
    (This)->lpVtbl -> get_IsRecurring(This,pfIsRecurring)

#define IAppointment_get_Subject(This,ppwsz)	\
    (This)->lpVtbl -> get_Subject(This,ppwsz)

#define IAppointment_get_Location(This,ppwsz)	\
    (This)->lpVtbl -> get_Location(This,ppwsz)

#define IAppointment_get_Categories(This,ppwsz)	\
    (This)->lpVtbl -> get_Categories(This,ppwsz)

#define IAppointment_get_Start(This,pst)	\
    (This)->lpVtbl -> get_Start(This,pst)

#define IAppointment_get_Duration(This,pnLen)	\
    (This)->lpVtbl -> get_Duration(This,pnLen)

#define IAppointment_get_End(This,pst)	\
    (This)->lpVtbl -> get_End(This,pst)

#define IAppointment_get_AllDayEvent(This,pfAllDay)	\
    (This)->lpVtbl -> get_AllDayEvent(This,pfAllDay)

#define IAppointment_get_BusyStatus(This,pnState)	\
    (This)->lpVtbl -> get_BusyStatus(This,pnState)

#define IAppointment_get_Sensitivity(This,plSensitivity)	\
    (This)->lpVtbl -> get_Sensitivity(This,plSensitivity)

#define IAppointment_get_Body(This,pwsz)	\
    (This)->lpVtbl -> get_Body(This,pwsz)

#define IAppointment_get_Recipients(This,pRecipients)	\
    (This)->lpVtbl -> get_Recipients(This,pRecipients)

#define IAppointment_get_MeetingStatus(This,pnStatus)	\
    (This)->lpVtbl -> get_MeetingStatus(This,pnStatus)

#define IAppointment_get_ReminderSet(This,pfReminderSet)	\
    (This)->lpVtbl -> get_ReminderSet(This,pfReminderSet)

#define IAppointment_get_ReminderSoundFile(This,ppwsz)	\
    (This)->lpVtbl -> get_ReminderSoundFile(This,ppwsz)

#define IAppointment_get_ReminderMinutesBeforeStart(This,plMinutes)	\
    (This)->lpVtbl -> get_ReminderMinutesBeforeStart(This,plMinutes)

#define IAppointment_get_ReminderOptions(This,pdwOptions)	\
    (This)->lpVtbl -> get_ReminderOptions(This,pdwOptions)

#define IAppointment_put_Subject(This,pwsz)	\
    (This)->lpVtbl -> put_Subject(This,pwsz)

#define IAppointment_put_Location(This,pwsz)	\
    (This)->lpVtbl -> put_Location(This,pwsz)

#define IAppointment_put_Categories(This,pwsz)	\
    (This)->lpVtbl -> put_Categories(This,pwsz)

#define IAppointment_put_Start(This,st)	\
    (This)->lpVtbl -> put_Start(This,st)

#define IAppointment_put_Duration(This,nLen)	\
    (This)->lpVtbl -> put_Duration(This,nLen)

#define IAppointment_put_End(This,st)	\
    (This)->lpVtbl -> put_End(This,st)

#define IAppointment_put_AllDayEvent(This,fAllDay)	\
    (This)->lpVtbl -> put_AllDayEvent(This,fAllDay)

#define IAppointment_put_BusyStatus(This,nState)	\
    (This)->lpVtbl -> put_BusyStatus(This,nState)

#define IAppointment_put_Sensitivity(This,lSensitivity)	\
    (This)->lpVtbl -> put_Sensitivity(This,lSensitivity)

#define IAppointment_put_Body(This,pwsz)	\
    (This)->lpVtbl -> put_Body(This,pwsz)

#define IAppointment_put_ReminderSet(This,fReminderSet)	\
    (This)->lpVtbl -> put_ReminderSet(This,fReminderSet)

#define IAppointment_put_ReminderSoundFile(This,pwsz)	\
    (This)->lpVtbl -> put_ReminderSoundFile(This,pwsz)

#define IAppointment_put_ReminderMinutesBeforeStart(This,lMinutes)	\
    (This)->lpVtbl -> put_ReminderMinutesBeforeStart(This,lMinutes)

#define IAppointment_put_ReminderOptions(This,dwOptions)	\
    (This)->lpVtbl -> put_ReminderOptions(This,dwOptions)

#define IAppointment_Save(This)	\
    (This)->lpVtbl -> Save(This)

#define IAppointment_Send(This)	\
    (This)->lpVtbl -> Send(This)

#define IAppointment_Delete(This)	\
    (This)->lpVtbl -> Delete(This)

#define IAppointment_Cancel(This)	\
    (This)->lpVtbl -> Cancel(This)

#define IAppointment_Copy(This,ppolCopy)	\
    (This)->lpVtbl -> Copy(This,ppolCopy)

#define IAppointment_Display(This)	\
    (This)->lpVtbl -> Display(This)

#define IAppointment_get_Oid(This,poid)	\
    (This)->lpVtbl -> get_Oid(This,poid)

#define IAppointment_put_BodyInk(This,pcebl)	\
    (This)->lpVtbl -> put_BodyInk(This,pcebl)

#define IAppointment_get_BodyInk(This,pcebl)	\
    (This)->lpVtbl -> get_BodyInk(This,pcebl)

#define IAppointment_get_Application(This,polApp)	\
    (This)->lpVtbl -> get_Application(This,polApp)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* __IAppointment_INTERFACE_DEFINED__ */


#ifndef __ICity_INTERFACE_DEFINED__
#define __ICity_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ICity
 * at Mon Dec 07 11:02:04 1998
 * using MIDL 3.02.88
 ****************************************/
/* [dual][full][helpstring][uuid][object] */ 



DEFINE_GUID(IID_ICity, 0xc83c5e90, 0x3d1b, 0x11d2, 0x8f, 0x1b, 0x0, 0x0, 0xf8, 0x7a, 0x43, 0x35);

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("C83C5E90-3D1B-11d2-8F1B-0000F87A4335")
    ICity : public IDispatch
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Longitude( 
            /* [retval][out] */ long __RPC_FAR *pcLongitude) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Latitude( 
            /* [retval][out] */ long __RPC_FAR *pcLatitude) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_TimezoneIndex( 
            /* [retval][out] */ long __RPC_FAR *pcTimezone) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_AirportCode( 
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_CountryPhoneCode( 
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_AreaCode( 
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Country( 
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_InROM( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pfInROM) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Longitude( 
            /* [in] */ long cLongitude) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Latitude( 
            /* [in] */ long cLatitude) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_TimezoneIndex( 
            /* [in] */ long cTimezone) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_AirportCode( 
            /* [in] */ BSTR pwsz) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_CountryPhoneCode( 
            /* [in] */ BSTR pwsz) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_AreaCode( 
            /* [in] */ BSTR pwsz) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Name( 
            /* [in] */ BSTR pwsz) = 0;

        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Country( 
            /* [in] */ BSTR pwsz) = 0;

        virtual HRESULT STDMETHODCALLTYPE Save( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Delete( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Copy( 
            /* [retval][out] */ ICity __RPC_FAR *__RPC_FAR *ppolCopy) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Application( 
            /* [retval][out] */ IPOutlookApp __RPC_FAR *__RPC_FAR *polApp) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICityVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ICity __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ICity __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ICity __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ICity __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ICity __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ICity __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ICity __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Longitude )( 
            ICity __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pcLongitude);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Latitude )( 
            ICity __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pcLatitude);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_TimezoneIndex )( 
            ICity __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pcTimezone);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_AirportCode )( 
            ICity __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CountryPhoneCode )( 
            ICity __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_AreaCode )( 
            ICity __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Name )( 
            ICity __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Country )( 
            ICity __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppwsz);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_InROM )( 
            ICity __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pfInROM);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Longitude )( 
            ICity __RPC_FAR * This,
            /* [in] */ long cLongitude);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Latitude )( 
            ICity __RPC_FAR * This,
            /* [in] */ long cLatitude);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_TimezoneIndex )( 
            ICity __RPC_FAR * This,
            /* [in] */ long cTimezone);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_AirportCode )( 
            ICity __RPC_FAR * This,
            /* [in] */ BSTR pwsz);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_CountryPhoneCode )( 
            ICity __RPC_FAR * This,
            /* [in] */ BSTR pwsz);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_AreaCode )( 
            ICity __RPC_FAR * This,
            /* [in] */ BSTR pwsz);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Name )( 
            ICity __RPC_FAR * This,
            /* [in] */ BSTR pwsz);

        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Country )( 
            ICity __RPC_FAR * This,
            /* [in] */ BSTR pwsz);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Save )( 
            ICity __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Delete )( 
            ICity __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Copy )( 
            ICity __RPC_FAR * This,
            /* [retval][out] */ ICity __RPC_FAR *__RPC_FAR *ppolCopy);
             
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Application )( 
            ICity __RPC_FAR * This,
            /* [retval][out] */ IPOutlookApp __RPC_FAR *__RPC_FAR *polApp);
   
        END_INTERFACE
    } ICityVtbl;

    interface ICity
    {
        CONST_VTBL struct ICityVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICity_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICity_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICity_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICity_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ICity_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ICity_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ICity_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ICity_get_Longitude(This,pcLongitude)	\
    (This)->lpVtbl -> get_Longitude(This,pcLongitude)

#define ICity_get_Latitude(This,pcLatitude)	\
    (This)->lpVtbl -> get_Latitude(This,pcLatitude)

#define ICity_get_TimezoneIndex(This,pcTimezone)	\
    (This)->lpVtbl -> get_TimezoneIndex(This,pcTimezone)

#define ICity_get_AirportCode(This,ppwsz)	\
    (This)->lpVtbl -> get_AirportCode(This,ppwsz)

#define ICity_get_CountryPhoneCode(This,ppwsz)	\
    (This)->lpVtbl -> get_CountryPhoneCode(This,ppwsz)

#define ICity_get_AreaCode(This,ppwsz)	\
    (This)->lpVtbl -> get_AreaCode(This,ppwsz)

#define ICity_get_Name(This,ppwsz)	\
    (This)->lpVtbl -> get_Name(This,ppwsz)

#define ICity_get_Country(This,ppwsz)	\
    (This)->lpVtbl -> get_Country(This,ppwsz)

#define ICity_get_InROM(This,pfInROM)	\
    (This)->lpVtbl -> get_InROM(This,pfInROM)

#define ICity_put_Longitude(This,cLongitude)	\
    (This)->lpVtbl -> put_Longitude(This,cLongitude)

#define ICity_put_Latitude(This,cLatitude)	\
    (This)->lpVtbl -> put_Latitude(This,cLatitude)

#define ICity_put_TimezoneIndex(This,cTimezone)	\
    (This)->lpVtbl -> put_TimezoneIndex(This,cTimezone)

#define ICity_put_AirportCode(This,pwsz)	\
    (This)->lpVtbl -> put_AirportCode(This,pwsz)

#define ICity_put_CountryPhoneCode(This,pwsz)	\
    (This)->lpVtbl -> put_CountryPhoneCode(This,pwsz)

#define ICity_put_AreaCode(This,pwsz)	\
    (This)->lpVtbl -> put_AreaCode(This,pwsz)

#define ICity_put_Name(This,pwsz)	\
    (This)->lpVtbl -> put_Name(This,pwsz)

#define ICity_put_Country(This,pwsz)	\
    (This)->lpVtbl -> put_Country(This,pwsz)

#define ICity_Save(This)	\
    (This)->lpVtbl -> Save(This)

#define ICity_Delete(This)	\
    (This)->lpVtbl -> Delete(This)

#define ICity_Copy(This,ppolCopy)	\
    (This)->lpVtbl -> Copy(This,ppolCopy)

#define ICity_get_Application(This,polApp)	\
    (This)->lpVtbl -> get_Application(This,polApp)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* __ICity_INTERFACE_DEFINED__ */


#ifndef __IFolder_INTERFACE_DEFINED__
#define __IFolder_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IFolder
 * at Mon Dec 07 11:02:04 1998
 * using MIDL 3.02.88
 ****************************************/
/* [dual][full][helpstring][uuid][object] */ 


DEFINE_GUID(IID_IFolder, 0x5058f20, 0x20be, 0x11d2, 0x8f, 0x18, 0x0, 0x0, 0xf8, 0x7a, 0x43, 0x35);

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("05058F20-20BE-11d2-8F18-0000F87A4335")
    IFolder : public IDispatch
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Items( 
            /* [retval][out] */ IPOutlookItemCollection __RPC_FAR *__RPC_FAR *ppolItems) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_DefaultItemType( 
            /* [retval][out] */ int __RPC_FAR *polItem) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Application( 
            IPOutlookApp __RPC_FAR *__RPC_FAR *polApp) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddItemToInfraredFolder( 
            /* [in] */ int olItem,
            /* [in] */ IDispatch __RPC_FAR *polItem) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SendToInfrared( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReceiveFromInfrared( 
            /* [retval][out] */ IPOutlookItemCollection __RPC_FAR *__RPC_FAR *ppItems) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IFolderVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IFolder __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IFolder __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IFolder __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IFolder __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IFolder __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IFolder __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IFolder __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Items )( 
            IFolder __RPC_FAR * This,
            /* [retval][out] */ IPOutlookItemCollection __RPC_FAR *__RPC_FAR *ppolItems);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DefaultItemType )( 
            IFolder __RPC_FAR * This,
            /* [retval][out] */ int __RPC_FAR *polItem);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Application )( 
            IFolder __RPC_FAR * This,
            IPOutlookApp __RPC_FAR *__RPC_FAR *polApp);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddItemToInfraredFolder )( 
            IFolder __RPC_FAR * This,
            /* [in] */ int olItem,
            /* [in] */ IDispatch __RPC_FAR *polItem);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SendToInfrared )( 
            IFolder __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ReceiveFromInfrared )( 
            IFolder __RPC_FAR * This,
            /* [retval][out] */ IPOutlookItemCollection __RPC_FAR *__RPC_FAR *ppItems);
        
        END_INTERFACE
    } IFolderVtbl;

    interface IFolder
    {
        CONST_VTBL struct IFolderVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFolder_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IFolder_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IFolder_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IFolder_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IFolder_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IFolder_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IFolder_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IFolder_get_Items(This,ppolItems)	\
    (This)->lpVtbl -> get_Items(This,ppolItems)

#define IFolder_get_DefaultItemType(This,polItem)	\
    (This)->lpVtbl -> get_DefaultItemType(This,polItem)

#define IFolder_get_Application(This,polApp)	\
    (This)->lpVtbl -> get_Application(This,polApp)

#define IFolder_AddItemToInfraredFolder(This,olItem,polItem)	\
    (This)->lpVtbl -> AddItemToInfraredFolder(This,olItem,polItem)

#define IFolder_SendToInfrared(This)	\
    (This)->lpVtbl -> SendToInfrared(This)

#define IFolder_ReceiveFromInfrared(This,ppItems)	\
    (This)->lpVtbl -> ReceiveFromInfrared(This,ppItems)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* __IFolder_INTERFACE_DEFINED__ */


#ifndef __IPOutlookApp_INTERFACE_DEFINED__
#define __IPOutlookApp_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IPOutlookApp
 * at Mon Dec 07 11:02:04 1998
 * using MIDL 3.02.88
 ****************************************/
/* [dual][full][helpstring][uuid][object] */ 



DEFINE_GUID(IID_IPOutlookApp, 0x05058F22, 0x20BE, 0x11d2, 0x8F, 0x18, 0x00, 0x00, 0xF8, 0x7A, 0x43, 0x35);

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("05058F22-20BE-11d2-8F18-0000F87A4335")
    IPOutlookApp : public IDispatch
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Logon( 
            /* [in] */ long hWnd) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Logoff( void) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Version( 
            /* [retval][out] */ BSTR __RPC_FAR *ppwszVersion) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDefaultFolder( 
            /* [in] */ int olFolder,
            /* [retval][out] */ IFolder __RPC_FAR *__RPC_FAR *ppIFolder) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateItem( 
            /* [in] */ int olItem,
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppPOutlookItem) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetItemFromOid( 
            /* [in] */ long oid,
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppPOutlookItem) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_HomeCity( 
            /* [retval][out] */ ICity __RPC_FAR *__RPC_FAR *ppHomeCity) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_HomeCity( 
            /* [in] */ ICity __RPC_FAR *pHomeCity) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_VisitingCity( 
            /* [retval][out] */ ICity __RPC_FAR *__RPC_FAR *ppVisitingCity) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_VisitingCity( 
            /* [in] */ ICity __RPC_FAR *pVisitingCity) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_CurrentCityIndex( 
            /* [retval][out] */ long __RPC_FAR *pnolCity) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_CurrentCityIndex( 
            /* [in] */ long olCity) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReceiveFromInfrared( void) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_OutlookCompatible( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pfCompat) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTimeZoneFromIndex( 
            /* [in] */ int cTimezone,
            /* [retval][out] */ ITimeZone __RPC_FAR *__RPC_FAR *ppTz) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTimeZoneInformationFromIndex( 
            /* [in] */ int cTimezone,
            /* [retval][out] */ TIME_ZONE_INFORMATION __RPC_FAR *ptzInfo) = 0;

        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Application( 
            /* [retval][out] */ IPOutlookApp __RPC_FAR *__RPC_FAR *polApp) = 0;

        virtual /* [hidden] */ HRESULT STDMETHODCALLTYPE SysFreeString( 
            /* [in] */ BSTR bstr) = 0;   
                    
        virtual /* [hidden] */ HRESULT STDMETHODCALLTYPE VariantTimeToSystemTime( 
            /* [in] */ DATE date,
            /* [out] */ SYSTEMTIME __RPC_FAR *pst) = 0;
        
        virtual /* [hidden] */ HRESULT STDMETHODCALLTYPE SystemTimeToVariantTime( 
            /* [in] */ SYSTEMTIME __RPC_FAR *pst,
            /* [out] */ DATE __RPC_FAR *pdate) = 0;
    };
    
#else 	/* C style interface */

    typedef struct IPOutlookAppVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IPOutlookApp __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IPOutlookApp __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IPOutlookApp __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IPOutlookApp __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IPOutlookApp __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IPOutlookApp __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IPOutlookApp __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Logon )( 
            IPOutlookApp __RPC_FAR * This,
            /* [in] */ long hWnd);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Logoff )( 
            IPOutlookApp __RPC_FAR * This);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Version )( 
            IPOutlookApp __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppwszVersion);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDefaultFolder )( 
            IPOutlookApp __RPC_FAR * This,
            /* [in] */ int olFolder,
            /* [retval][out] */ IFolder __RPC_FAR *__RPC_FAR *ppIFolder);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateItem )( 
            IPOutlookApp __RPC_FAR * This,
            /* [in] */ int olItem,
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppPOutlookItem);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetItemFromOid )( 
            IPOutlookApp __RPC_FAR * This,
            /* [in] */ long oid,
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppPOutlookItem);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_HomeCity )( 
            IPOutlookApp __RPC_FAR * This,
            /* [retval][out] */ ICity __RPC_FAR *__RPC_FAR *ppHomeCity);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_HomeCity )( 
            IPOutlookApp __RPC_FAR * This,
            /* [in] */ ICity __RPC_FAR *pHomeCity);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_VisitingCity )( 
            IPOutlookApp __RPC_FAR * This,
            /* [retval][out] */ ICity __RPC_FAR *__RPC_FAR *ppVisitingCity);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_VisitingCity )( 
            IPOutlookApp __RPC_FAR * This,
            /* [in] */ ICity __RPC_FAR *pVisitingCity);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CurrentCityIndex )( 
            IPOutlookApp __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pnolCity);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_CurrentCityIndex )( 
            IPOutlookApp __RPC_FAR * This,
            /* [in] */ long olCity);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ReceiveFromInfrared )( 
            IPOutlookApp __RPC_FAR * This);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_OutlookCompatible )( 
            IPOutlookApp __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pfCompat);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTimeZoneFromIndex )( 
            IPOutlookApp __RPC_FAR * This,
            /* [in] */ int cTimezone,
            /* [retval][out] */ ITimeZone __RPC_FAR *__RPC_FAR *ppTz);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTimeZoneInformationFromIndex )( 
            IPOutlookApp __RPC_FAR * This,
            /* [in] */ int cTimezone,
            /* [retval][out] */ TIME_ZONE_INFORMATION __RPC_FAR *ptzInfo);

        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Application )( 
            IFolder __RPC_FAR * This,
            IPOutlookApp __RPC_FAR *__RPC_FAR *polApp);

        /* [hidden] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SysFreeString )( 
            IPOutlookApp __RPC_FAR * This,
            /* [in] */ BSTR bstr);
        
        /* [hidden] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *VariantTimeToSystemTime )( 
            IPOutlookApp __RPC_FAR * This,
            /* [in] */ DATE date,
            /* [out] */ SYSTEMTIME __RPC_FAR *pst);
        
        /* [hidden] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SystemTimeToVariantTime )( 
            IPOutlookApp __RPC_FAR * This,
            /* [in] */ SYSTEMTIME __RPC_FAR *pst,
            /* [out] */ DATE __RPC_FAR *pdate);
            
        END_INTERFACE
    } IPOutlookAppVtbl;

    interface IPOutlookApp
    {
        CONST_VTBL struct IPOutlookAppVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPOutlookApp_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPOutlookApp_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPOutlookApp_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPOutlookApp_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IPOutlookApp_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IPOutlookApp_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IPOutlookApp_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IPOutlookApp_Logon(This,hWnd)	\
    (This)->lpVtbl -> Logon(This,hWnd)

#define IPOutlookApp_Logoff(This)	\
    (This)->lpVtbl -> Logoff(This)

#define IPOutlookApp_get_Version(This,ppwszVersion)	\
    (This)->lpVtbl -> get_Version(This,ppwszVersion)

#define IPOutlookApp_GetDefaultFolder(This,olFolder,ppIFolder)	\
    (This)->lpVtbl -> GetDefaultFolder(This,olFolder,ppIFolder)

#define IPOutlookApp_CreateItem(This,olItem,ppPOutlookItem)	\
    (This)->lpVtbl -> CreateItem(This,olItem,ppPOutlookItem)

#define IPOutlookApp_GetItemFromOid(This,oid,ppPOutlookItem)	\
    (This)->lpVtbl -> GetItemFromOid(This,oid,ppPOutlookItem)

#define IPOutlookApp_get_HomeCity(This,ppHomeCity)	\
    (This)->lpVtbl -> get_HomeCity(This,ppHomeCity)

#define IPOutlookApp_put_HomeCity(This,pHomeCity)	\
    (This)->lpVtbl -> put_HomeCity(This,pHomeCity)

#define IPOutlookApp_get_VisitingCity(This,ppVisitingCity)	\
    (This)->lpVtbl -> get_VisitingCity(This,ppVisitingCity)

#define IPOutlookApp_put_VisitingCity(This,pVisitingCity)	\
    (This)->lpVtbl -> put_VisitingCity(This,pVisitingCity)

#define IPOutlookApp_get_CurrentCityIndex(This,pnolCity)	\
    (This)->lpVtbl -> get_CurrentCityIndex(This,pnolCity)

#define IPOutlookApp_put_CurrentCityIndex(This,olCity)	\
    (This)->lpVtbl -> put_CurrentCityIndex(This,olCity)

#define IPOutlookApp_ReceiveFromInfrared(This)	\
    (This)->lpVtbl -> ReceiveFromInfrared(This)

#define IPOutlookApp_get_OutlookCompatible(This,pfCompat)	\
    (This)->lpVtbl -> get_OutlookCompatible(This,pfCompat)

#define IPOutlookApp_GetTimeZoneFromIndex(This,cTimezone,ppTz)	\
    (This)->lpVtbl -> GetTimeZoneFromIndex(This,cTimezone,ppTz)

#define IPOutlookApp_GetTimeZoneInformationFromIndex(This,cTimezone,ptzInfo)	\
    (This)->lpVtbl -> GetTimeZoneInformationFromIndex(This,cTimezone,ptzInfo)

#define IPOutlookApp_get_Application(This,polApp)	\
    (This)->lpVtbl -> get_Application(This,polApp)

#define IPOutlookApp_SysFreeString(This,bstr)	\
    (This)->lpVtbl -> SysFreeString(This,bstr)

#define IPOutlookApp_VariantTimeToSystemTime(This,date,pst)	\
    (This)->lpVtbl -> VariantTimeToSystemTime(This,date,pst)

#define IPOutlookApp_SystemTimeToVariantTime(This,pst,pdate)	\
    (This)->lpVtbl -> SystemTimeToVariantTime(This,pst,pdate)

    
#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* __IPOutlookApp_INTERFACE_DEFINED__ */



#ifndef __PocketOutlook_LIBRARY_DEFINED__
#define __PocketOutlook_LIBRARY_DEFINED__

/****************************************
 * Generated header for library: PocketOutlook
 * at Mon Dec 07 11:02:04 1998
 * using MIDL 3.02.88
 ****************************************/
/* [helpstring][version][uuid] */ 

// Maximum/min dates for calendar entries
#define CAL_MAXDATE     ((DATE) 401768)     // 12/31/2999
#define CAL_MINDATE     ((DATE) 0)          // 12/30/1899
#define DATE_NONE       ((DATE) 949998)     // 1/1/4501

// Custom error codes
#define E_CLOCKRUNNING          MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 100)
#define E_CITYINROM             MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 101)
#define E_FIELDTOOLARGE         MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 102)
#define E_INVALIDREMINDERTIME   MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 103)
#define E_INVALIDDATES          MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 104)
#define E_ALLDAYMEETING         MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 105)
#define E_OVERLAPPINGEXCEPTION  MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 106)
#define E_CANTCHANGEDATE        MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 107)
#define E_EXCEPTIONSAMEDAY      MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 108)
#define E_UNWANTEDITEM          MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 109)


typedef /* [v1_enum] */ 
enum OlImportance
    {	olImportanceLow	= 0,
	olImportanceNormal	= 1,
	olImportanceHigh	= 2
    }	OlImportance;

typedef /* [v1_enum] */ 
enum OlDefaultFolders
    {	olFolderCalendar	= 9,
	olFolderContacts	= 10,
	olFolderTasks	= 13,
	olFolderCities	= 101,
	olFolderInfrared	= 102
    }	OlDefaultFolders;

typedef /* [v1_enum] */ 
enum OlItemType
    {	olAppointmentItem	= 1,
	olContactItem	= 2,
	olTaskItem	= 3,
	olCityItem	= 102
    }	OlItemType;

typedef /* [v1_enum] */ 
enum OlReminderOptions
    {	olLED	= 1,
	olVibrate	= 2,
	olDialog	= 4,
	olSound	= 8,
	olRepeat	= 16
    }	OlReminderOptions;

typedef /* [v1_enum] */ 
enum OlBusyStatus
    {	olFree	= 0,
	olTentative	= 1,
	olBusy	= 2,
	olOutOfOffice	= 3
    }	OlBusyStatus;

typedef /* [v1_enum] */ 
enum OlMeetingStatus
    {	olNonMeeting	= 0,
	olMeeting	= 1
    }	OlMeetingStatus;

typedef /* [v1_enum] */ 
enum OlCurrentCity
    {	olHomeCity	= 0,
	olVisitingCity	= 1
    }	OlCurrentCity;

typedef /* [v1_enum] */ 
enum OlRecurrenceType
    {	olRecursDaily	= 0,
	olRecursWeekly	= 1,
	olRecursMonthly	= 2,
	olRecursMonthNth	= 3,
	olRecursYearly	= 5,
	olRecursYearNth	= 6
    }	OlRecurrenceType;

typedef /* [v1_enum] */ 
enum OlDaysOfWeek
    {	olSunday	= 1,
	olMonday	= 2,
	olTuesday	= 4,
	olWednesday	= 8,
	olThursday	= 16,
	olFriday	= 32,
	olSaturday	= 64
    }	OlDaysOfWeek;

typedef /* [v1_enum] */ 
enum OlSensitivity
    {	olNormal	= 0,
	olPrivate	= 2
    }	OlSensitivity;


DEFINE_GUID(LIBID_PocketOutlook, 0x4e130e40, 0x7dbe, 0x11d2, 0x8f, 0x23, 0x0, 0x0, 0xf8, 0x7a, 0x43, 0x35);

DEFINE_GUID(CLSID_Exception, 0xb47398d2, 0x3b73, 0x11d2, 0x8f, 0x1b, 0x0, 0x0, 0xf8, 0x7a, 0x43, 0x35);

#ifdef __cplusplus

class DECLSPEC_UUID("B47398D2-3B73-11d2-8F1B-0000F87A4335")
Exception;
#endif

DEFINE_GUID(CLSID_Exceptions, 0xb47398d3, 0x3b73, 0x11d2, 0x8f, 0x1b, 0x0, 0x0, 0xf8, 0x7a, 0x43, 0x35);

#ifdef __cplusplus

class DECLSPEC_UUID("B47398D3-3B73-11d2-8F1B-0000F87A4335")
Exceptions;
#endif

DEFINE_GUID(CLSID_TimeZone, 0x78b27291, 0x5256, 0x11d2, 0x8f, 0x1b, 0x0, 0x0, 0xf8, 0x7a, 0x43, 0x35);

#ifdef __cplusplus

class DECLSPEC_UUID("78B27291-5256-11d2-8F1B-0000F87A4335")
TimeZone;
#endif

DEFINE_GUID(CLSID_RecurrencePattern, 0x38f47301, 0x270f, 0x11d2, 0x8f, 0x18, 0x0, 0x0, 0xf8, 0x7a, 0x43, 0x35);

#ifdef __cplusplus

class DECLSPEC_UUID("38F47301-270F-11d2-8F18-0000F87A4335")
RecurrencePattern;
#endif

DEFINE_GUID(CLSID_Recipient, 0x7e136be1, 0x5240, 0x11d2, 0x8f, 0x1b, 0x0, 0x0, 0xf8, 0x7a, 0x43, 0x35);

#ifdef __cplusplus

class DECLSPEC_UUID("7E136BE1-5240-11d2-8F1B-0000F87A4335")
Recipient;
#endif

DEFINE_GUID(CLSID_Recipients, 0x76065ae1, 0x2347, 0x11d2, 0x8f, 0x18, 0x0, 0x0, 0xf8, 0x7a, 0x43, 0x35);

#ifdef __cplusplus

class DECLSPEC_UUID("76065AE1-2347-11d2-8F18-0000F87A4335")
Recipients;
#endif

DEFINE_GUID(CLSID_Items, 0xf06748c1, 0x21a5, 0x11d2, 0x8f, 0x18, 0x0, 0x0, 0xf8, 0x7a, 0x43, 0x35);

#ifdef __cplusplus

class DECLSPEC_UUID("F06748C1-21A5-11d2-8F18-0000F87A4335")
Items;
#endif

DEFINE_GUID(CLSID_ContactItem, 0x430539d0, 0x2017, 0x11d2, 0x8f, 0x18, 0x0, 0x0, 0xf8, 0x7a, 0x43, 0x35);

#ifdef __cplusplus

class DECLSPEC_UUID("430539d0-2017-11d2-8f18-0000f87a4335")
ContactItem;
#endif

DEFINE_GUID(CLSID_TaskItem, 0x5b43f690, 0x202c, 0x11d2, 0x8f, 0x18, 0x0, 0x0, 0xf8, 0x7a, 0x43, 0x35);

#ifdef __cplusplus

class DECLSPEC_UUID("5B43F690-202C-11d2-8F18-0000F87A4335")
TaskItem;
#endif

DEFINE_GUID(CLSID_AppointmentItem, 0x5b43f692, 0x202c, 0x11d2, 0x8f, 0x18, 0x0, 0x0, 0xf8, 0x7a, 0x43, 0x35);

#ifdef __cplusplus

class DECLSPEC_UUID("5B43F692-202C-11d2-8F18-0000F87A4335")
AppointmentItem;
#endif

DEFINE_GUID(CLSID_CityItem, 0xc83c5e91, 0x3d1b, 0x11d2, 0x8f, 0x1b, 0x0, 0x0, 0xf8, 0x7a, 0x43, 0x35);

#ifdef __cplusplus

class DECLSPEC_UUID("C83C5E91-3D1B-11d2-8F1B-0000F87A4335")
CityItem;
#endif

DEFINE_GUID(CLSID_Folder, 0x5058f21, 0x20be, 0x11d2, 0x8f, 0x18, 0x0, 0x0, 0xf8, 0x7a, 0x43, 0x35);

#ifdef __cplusplus

class DECLSPEC_UUID("05058F21-20BE-11d2-8F18-0000F87A4335")
Folder;
#endif

DEFINE_GUID(CLSID_Application, 0x05058F23, 0x20BE, 0x11d2, 0x8F, 0x18, 0x00, 0x00, 0xF8, 0x7A, 0x43, 0x35);

#ifdef __cplusplus

class DECLSPEC_UUID("05058F23-20BE-11d2-8F18-0000F87A4335")
Application;
#endif
#endif /* __PocketOutlook_LIBRARY_DEFINED__ */

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
