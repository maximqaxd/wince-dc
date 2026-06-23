/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.02.88 */
/* at Tue Dec 14 22:39:11 1999
 */
/* Compiler settings for .\webcheck.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: none
*/
//@@MIDL_FILE_HEADING(  )
#include "rpc.h"
#include "rpcndr.h"

#ifndef __webcheck_h__
#define __webcheck_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __WebCheck_FWD_DEFINED__
#define __WebCheck_FWD_DEFINED__

#ifdef __cplusplus
typedef class WebCheck WebCheck;
#else
typedef struct WebCheck WebCheck;
#endif /* __cplusplus */

#endif 	/* __WebCheck_FWD_DEFINED__ */


#ifndef __WebCrawlerAgent_FWD_DEFINED__
#define __WebCrawlerAgent_FWD_DEFINED__

#ifdef __cplusplus
typedef class WebCrawlerAgent WebCrawlerAgent;
#else
typedef struct WebCrawlerAgent WebCrawlerAgent;
#endif /* __cplusplus */

#endif 	/* __WebCrawlerAgent_FWD_DEFINED__ */


#ifndef __ChannelAgent_FWD_DEFINED__
#define __ChannelAgent_FWD_DEFINED__

#ifdef __cplusplus
typedef class ChannelAgent ChannelAgent;
#else
typedef struct ChannelAgent ChannelAgent;
#endif /* __cplusplus */

#endif 	/* __ChannelAgent_FWD_DEFINED__ */


#ifndef __MailAgent_FWD_DEFINED__
#define __MailAgent_FWD_DEFINED__

#ifdef __cplusplus
typedef class MailAgent MailAgent;
#else
typedef struct MailAgent MailAgent;
#endif /* __cplusplus */

#endif 	/* __MailAgent_FWD_DEFINED__ */


#ifndef __OfflineTrayAgent_FWD_DEFINED__
#define __OfflineTrayAgent_FWD_DEFINED__

#ifdef __cplusplus
typedef class OfflineTrayAgent OfflineTrayAgent;
#else
typedef struct OfflineTrayAgent OfflineTrayAgent;
#endif /* __cplusplus */

#endif 	/* __OfflineTrayAgent_FWD_DEFINED__ */


#ifndef __ConnectionAgent_FWD_DEFINED__
#define __ConnectionAgent_FWD_DEFINED__

#ifdef __cplusplus
typedef class ConnectionAgent ConnectionAgent;
#else
typedef struct ConnectionAgent ConnectionAgent;
#endif /* __cplusplus */

#endif 	/* __ConnectionAgent_FWD_DEFINED__ */


#ifndef __PostAgent_FWD_DEFINED__
#define __PostAgent_FWD_DEFINED__

#ifdef __cplusplus
typedef class PostAgent PostAgent;
#else
typedef struct PostAgent PostAgent;
#endif /* __cplusplus */

#endif 	/* __PostAgent_FWD_DEFINED__ */


#ifndef __CDLAgent_FWD_DEFINED__
#define __CDLAgent_FWD_DEFINED__

#ifdef __cplusplus
typedef class CDLAgent CDLAgent;
#else
typedef struct CDLAgent CDLAgent;
#endif /* __cplusplus */

#endif 	/* __CDLAgent_FWD_DEFINED__ */


/* header files for imported files */
#include "unknwn.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/****************************************
 * Generated header for interface: __MIDL_itf_webcheck_0000
 * at Tue Dec 14 22:39:11 1999
 * using MIDL 3.02.88
 ****************************************/
/* [local] */ 


// Private File
// This file is not included in the Internet SDK
// Use msnotify or subsmgr headers for public interfaces
extern const IID CLSID_WebCheckDefaultProcess;               
#include <subsmgr.h>


extern RPC_IF_HANDLE __MIDL_itf_webcheck_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_webcheck_0000_v0_0_s_ifspec;


#ifndef __WebCheck_LIBRARY_DEFINED__
#define __WebCheck_LIBRARY_DEFINED__

/****************************************
 * Generated header for library: WebCheck
 * at Tue Dec 14 22:39:11 1999
 * using MIDL 3.02.88
 ****************************************/
/* [version][lcid][helpstring][uuid] */ 



EXTERN_C const IID LIBID_WebCheck;

EXTERN_C const CLSID CLSID_WebCheck;

#ifdef __cplusplus

class DECLSPEC_UUID("E6FB5E20-DE35-11CF-9C87-00AA005127ED")
WebCheck;
#endif

EXTERN_C const CLSID CLSID_WebCrawlerAgent;

#ifdef __cplusplus

class DECLSPEC_UUID("08165EA0-E946-11CF-9C87-00AA005127ED")
WebCrawlerAgent;
#endif

EXTERN_C const CLSID CLSID_ChannelAgent;

#ifdef __cplusplus

class DECLSPEC_UUID("E3A8BDE6-ABCE-11d0-BC4B-00C04FD929DB")
ChannelAgent;
#endif

EXTERN_C const CLSID CLSID_MailAgent;

#ifdef __cplusplus

class DECLSPEC_UUID("140c9ab0-69ca-11d0-adb8-00c04fd75d13")
MailAgent;
#endif

EXTERN_C const CLSID CLSID_OfflineTrayAgent;

#ifdef __cplusplus

class DECLSPEC_UUID("E8BB6DC0-6B4E-11d0-92DB-00A0C90C2BD7")
OfflineTrayAgent;
#endif

EXTERN_C const CLSID CLSID_ConnectionAgent;

#ifdef __cplusplus

class DECLSPEC_UUID("E6CC6978-6B6E-11D0-BECA-00C04FD940BE")
ConnectionAgent;
#endif

EXTERN_C const CLSID CLSID_PostAgent;

#ifdef __cplusplus

class DECLSPEC_UUID("d8bd2030-6fC9-11d0-864f-00aa006809d9")
PostAgent;
#endif

EXTERN_C const CLSID CLSID_CDLAgent;

#ifdef __cplusplus

class DECLSPEC_UUID("7D559C10-9FE9-11d0-93F7-00AA0059CE02")
CDLAgent;
#endif
#endif /* __WebCheck_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
