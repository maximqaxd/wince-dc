// -----------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//  Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.
//  
// -----------------------------------------------------------------------------
//
//  msacmdrv.h     Audio Compression Manager Public Header File for Drivers
//
// -----------------------------------------------------------------------------

#ifndef _INC_ACMDRV
#define _INC_ACMDRV         /* #defined if msacmdrv.h has been included */

#if !defined(_INC_ACM)
#ifndef RC_INVOKED
#error MSACM.H to be included first
#endif
#endif

#ifndef RC_INVOKED
#pragma pack(1)             /* Assume byte packing throughout */
#endif  /* RC_INVOKED */

#ifdef __cplusplus
extern "C" {                /* Assume C declarations for C++ */
#endif  /* __cplusplus */



#ifndef MMNODRV

#ifndef DRV_LOAD

/* Driver messages */
#define DRV_LOAD                0x0001
#define DRV_ENABLE              0x0002
#define DRV_OPEN                0x0003
#define DRV_CLOSE               0x0004
#define DRV_DISABLE             0x0005
#define DRV_FREE                0x0006
#define DRV_CONFIGURE           0x0007
#define DRV_QUERYCONFIGURE      0x0008
#define DRV_INSTALL             0x0009
#define DRV_REMOVE              0x000A
#define DRV_EXITSESSION         0x000B
#define DRV_POWER               0x000F
#define DRV_RESERVED            0x0800
#define DRV_USER                0x4000

typedef struct DRVCONFIGINFOEX {
    DWORD   dwDCISize;
    LPCWSTR  lpszDCISectionName;
    LPCWSTR  lpszDCIAliasName;
    DWORD    dnDevNode;
} DRVCONFIGINFOEX, *PDRVCONFIGINFOEX, NEAR *NPDRVCONFIGINFOEX, FAR *LPDRVCONFIGINFOEX;

/* LPARAM of DRV_CONFIGURE message */
typedef struct tagDRVCONFIGINFO {
    DWORD   dwDCISize;
    LPCWSTR  lpszDCISectionName;
    LPCWSTR  lpszDCIAliasName;
} DRVCONFIGINFO, *PDRVCONFIGINFO, NEAR *NPDRVCONFIGINFO, FAR *LPDRVCONFIGINFO;

/* Supported return values for DRV_CONFIGURE message */
#define DRVCNF_CANCEL           0x0000
#define DRVCNF_OK               0x0001
#define DRVCNF_RESTART          0x0002

/* return values from DriverProc() function */
#define DRV_CANCEL             DRVCNF_CANCEL
#define DRV_OK                 DRVCNF_OK
#define DRV_RESTART            DRVCNF_RESTART

#endif  // ifndef DRV_LOAD

#endif  /* ifndef MMNODRV */




//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  ACM Driver Version:
//
//  the version is a 32 bit number that is broken into three parts as 
//  follows:
//
//      bits 24 - 31:   8 bit _major_ version number
//      bits 16 - 23:   8 bit _minor_ version number
//      bits  0 - 15:   16 bit build number
//
//  this is then displayed as follows:
//
//      bMajor = (BYTE)(dwVersion >> 24)
//      bMinor = (BYTE)(dwVersion >> 16) & 
//      wBuild = LOWORD(dwVersion)
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

#define MAKE_ACM_VERSION(mjr, mnr, bld) (((long)(mjr)<<24)| \
                                         ((long)(mnr)<<16)| \
                                         ((long)bld))



//
//
//
//
//
#define ACMDRVOPENDESC_SECTIONNAME_CHARS    

typedef struct tACMDRVOPENDESC
{
    DWORD           cbStruct;       // sizeof(ACMDRVOPENDESC)
    FOURCC          fccType;        // 'audc'
    FOURCC          fccComp;        // sub-type (not used--must be 0)
    DWORD           dwVersion;      // current version of ACM opening you
    DWORD           dwFlags;        //
    DWORD           dwError;        // result from DRV_OPEN request
    LPCTSTR         pszSectionName; // see DRVCONFIGINFO.lpszDCISectionName
    LPCTSTR         pszAliasName;   // see DRVCONFIGINFO.lpszDCIAliasName
    DWORD           dnDevNode;      // devnode id for pnp drivers.

} ACMDRVOPENDESC, *PACMDRVOPENDESC, FAR *LPACMDRVOPENDESC;


//
//
//
//
//
typedef struct tACMDRVSTREAMINSTANCE
{
    DWORD               cbStruct;
    LPWAVEFORMATEX      pwfxSrc;
    LPWAVEFORMATEX      pwfxDst;
    LPWAVEFILTER        pwfltr;
    DWORD               dwCallback;
    DWORD               dwInstance;
    DWORD               fdwOpen;
    DWORD               fdwDriver;
    DWORD               dwDriver;
    HACMSTREAM          has;

} ACMDRVSTREAMINSTANCE, *PACMDRVSTREAMINSTANCE, FAR *LPACMDRVSTREAMINSTANCE;


//
//  NOTE! this structure must match the ACMSTREAMHEADER in msacm.h but
//  defines more information for the driver writing convenience
//
typedef struct tACMDRVSTREAMHEADER FAR *LPACMDRVSTREAMHEADER;
typedef struct tACMDRVSTREAMHEADER
{
    DWORD                   cbStruct;
    DWORD                   fdwStatus;
    DWORD                   dwUser;
    LPBYTE                  pbSrc;
    DWORD                   cbSrcLength;
    DWORD                   cbSrcLengthUsed;
    DWORD                   dwSrcUser;
    LPBYTE                  pbDst;
    DWORD                   cbDstLength;
    DWORD                   cbDstLengthUsed;
    DWORD                   dwDstUser;

    DWORD                   fdwConvert;     // flags passed from convert func
    LPACMDRVSTREAMHEADER    padshNext;      // for async driver queueing
    DWORD                   fdwDriver;      // driver instance flags
    DWORD                   dwDriver;       // driver instance data

    //
    //  all remaining fields are used by the ACM for bookkeeping purposes.
    //  an ACM driver should never use these fields (though than can be
    //  helpful for debugging)--note that the meaning of these fields
    //  may change, so do NOT rely on them in shipping code.
    //
    DWORD                   fdwPrepared;
    DWORD                   dwPrepared;
    LPBYTE                  pbPreparedSrc;
    DWORD                   cbPreparedSrcLength;
    LPBYTE                  pbPreparedDst;
    DWORD                   cbPreparedDstLength;

} ACMDRVSTREAMHEADER, *PACMDRVSTREAMHEADER;


//
//  structure for ACMDM_STREAM_SIZE message
//
//
typedef struct tACMDRVSTREAMSIZE
{
    DWORD               cbStruct;
    DWORD               fdwSize;
    DWORD               cbSrcLength;
    DWORD               cbDstLength;

} ACMDRVSTREAMSIZE, *PACMDRVSTREAMSIZE, FAR *LPACMDRVSTREAMSIZE;



//
//  structure containing the information for the ACMDM_FORMAT_SUGGEST message
//
//
typedef struct tACMDRVFORMATSUGGEST
{
    DWORD               cbStruct;           // sizeof(ACMDRVFORMATSUGGEST)
    DWORD               fdwSuggest;         // Suggest flags
    LPWAVEFORMATEX      pwfxSrc;            // Source Format
    DWORD               cbwfxSrc;           // Source Size
    LPWAVEFORMATEX      pwfxDst;            // Dest format
    DWORD               cbwfxDst;           // Dest Size

} ACMDRVFORMATSUGGEST, *PACMDRVFORMATSUGGEST, FAR *LPACMDRVFORMATSUGGEST;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  ACM Driver Messages
//
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

#define ACMDM_DRIVER_NOTIFY             (ACMDM_BASE + 1)
#define ACMDM_DRIVER_DETAILS            (ACMDM_BASE + 10)

#define ACMDM_HARDWARE_WAVE_CAPS_INPUT  (ACMDM_BASE + 20)
#define ACMDM_HARDWARE_WAVE_CAPS_OUTPUT (ACMDM_BASE + 21)

#define ACMDM_FORMATTAG_DETAILS         (ACMDM_BASE + 25)
#define ACMDM_FORMAT_DETAILS            (ACMDM_BASE + 26)
#define ACMDM_FORMAT_SUGGEST            (ACMDM_BASE + 27)

#define ACMDM_FILTERTAG_DETAILS         (ACMDM_BASE + 50)
#define ACMDM_FILTER_DETAILS            (ACMDM_BASE + 51)

#define ACMDM_STREAM_OPEN               (ACMDM_BASE + 76)
#define ACMDM_STREAM_CLOSE              (ACMDM_BASE + 77)
#define ACMDM_STREAM_SIZE               (ACMDM_BASE + 78)
#define ACMDM_STREAM_CONVERT            (ACMDM_BASE + 79)
#define ACMDM_STREAM_RESET              (ACMDM_BASE + 80)
#define ACMDM_STREAM_PREPARE            (ACMDM_BASE + 81)
#define ACMDM_STREAM_UNPREPARE          (ACMDM_BASE + 82)
#define ACMDM_STREAM_UPDATE             (ACMDM_BASE + 83)


#ifndef RC_INVOKED
#pragma pack()          /* Revert to default packing */
#endif  /* RC_INVOKED */

#ifdef __cplusplus
}                       /* End of extern "C" { */
#endif  /* __cplusplus */

#endif  /* _INC_ACMDRV */
