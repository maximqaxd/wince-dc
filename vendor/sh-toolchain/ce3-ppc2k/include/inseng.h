 //=--------------------------------------------------------------------------=
// inseng.h
//=--------------------------------------------------------------------------=
// Copyright 1995-1996 Microsoft Corporation.  All Rights Reserved.
//
//
// interface declaration for the InstallEngine control.
//
#ifndef _INSENG_H_

#ifdef __cplusplus
extern "C"{
#endif

#define MAX_ID_LENGTH            48
#define MAX_DISPLAYNAME_LENGTH  128


#define ICI_NOTINSTALLED          0
#define ICI_INSTALLED             1
#define ICI_NEWVERSIONAVAILABLE   2
#define ICI_UNKNOWN               3
#define ICI_NOTINITIALIZED        0xffffffff

#define ABORTINSTALL_NORMAL       0
#define ABORTINSTALL_IMMEADIATE   1


#define ENGINESTATUS_NOTREADY     0
#define ENGINESTATUS_LOADING      1
#define ENGINESTATUS_INSTALLING   2
#define ENGINESTATUS_READY        3

#define SETACTION_NONE            0
#define SETACTION_INSTALL         1

#define INSTALLOPTIONS_NOCACHE     0x00000001
#define INSTALLOPTIONS_DOWNLOAD    0x00000002
#define INSTALLOPTIONS_INSTALL     0x00000004

#define EXECUTEJOB_SILENT              0x00000001
#define EXECUTEJOB_DELETE_JOB          0x00000002

#define EXECUTEJOB_VERIFYFILES         0x00000008
#define EXECUTEJOB_IGNORETRUST         0x00000010
#define EXECUTEJOB_IGNOREDOWNLOADERROR 0x00000020
#define EXECUTEJOB_DONTALLOWCANCEL     0x00000040


#define E_FILESMISSING             _HRESULT_TYPEDEF_(0x80100003L)

HRESULT WINAPI CheckTrust(LPCSTR pszFilename, HWND hwndForUI, BOOL bShowBadUI);
HRESULT WINAPI CheckTrustEx(LPCSTR szURL, LPCSTR szFilename, HWND hwndForUI, BOOL bShowBadUI, DWORD dwReserved);
HRESULT WINAPI PurgeDownloadDir(LPCSTR pszDir);
HRESULT WINAPI CheckForVersionConflict();

typedef struct
{
   DWORD cbSize;
   DWORD dwInstallSize;
   DWORD dwWinDriveSize;
   DWORD dwDownloadSize;
   DWORD dwDependancySize;
   DWORD dwInstallDriveReq;
   DWORD dwWinDriveReq;
   DWORD dwDownloadDriveReq;
   CHAR  chWinDrive;
   CHAR  chInstallDrive;
   CHAR  chDownloadDrive;
} COMPONENT_SIZES;

typedef struct
{
   DWORD cbSize;
   DWORD dwDownloadKBRemaining;
   DWORD dwInstallKBRemaining;
   DWORD dwDownloadSecsRemaining;
   DWORD dwInstallSecsRemaining;
} INSTALLPROGRESS;


enum InstallStatus
{ 
   INSTALLSTATUS_INITIALIZING,
   INSTALLSTATUS_DEPENDENCY,
   INSTALLSTATUS_DOWNLOADING,
   INSTALLSTATUS_COPYING,
   INSTALLSTATUS_RETRYING,
   INSTALLSTATUS_CHECKINGTRUST,
   INSTALLSTATUS_EXTRACTING,
   INSTALLSTATUS_RUNNING,
   INSTALLSTATUS_FINISHED,
   INSTALLSTATUS_DOWNLOADFINISHED
};

// defines for engine problems  (OnEngineProblem)
#define ENGINEPROBLEM_DOWNLOADFAIL   0x00000001


// Actions particular to ENGINEPROBLEM_DOWNLOAD
#define DOWNLOADFAIL_RETRY   0x00000001


#define STOPINSTALL_REBOOTNEEDED  0x00000001


DEFINE_GUID(IID_IInstallEngineCallback,0x6E449685L,0xC509,0x11CF,0xAA,0xFA,0x00,0xAA,0x00,0xB6,0x01,0x5C);
   
#undef INTERFACE
#define INTERFACE IInstallEngineCallback

DECLARE_INTERFACE_(IInstallEngineCallback, IUnknown)
{
   // *** IUnknown methods ***
   STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
   STDMETHOD_(ULONG,Release) (THIS) PURE;
   
   
   STDMETHOD(OnEngineStatusChange)(THIS_ DWORD dwEngineStatus, DWORD substatus) PURE;
   STDMETHOD(OnStartInstall)(THIS_ DWORD dwDLSize, DWORD dwInstallSize) PURE;
   STDMETHOD(OnStartComponent)(THIS_ LPCSTR pszID, DWORD dwDLSize, DWORD dwInstallSize, LPCSTR pszString) PURE;
   STDMETHOD(OnComponentProgress)(THIS_ LPCSTR pszID, DWORD dwPhase, LPCSTR pszString, LPCSTR pszMsgString, ULONG progress, ULONG themax) PURE;
   STDMETHOD(OnStopComponent)(THIS_ LPCSTR pszID, HRESULT hError, DWORD dwPhase, LPCSTR pszString, DWORD dwStatus) PURE;
   STDMETHOD(OnStopInstall)(THIS_ HRESULT hrError, LPCSTR szError, DWORD dwStatus) PURE;
   STDMETHOD(OnEngineProblem)(THIS_ DWORD dwEngineProblem, LPDWORD dwAction) PURE;
};



DEFINE_GUID(IID_IInstallEngine,0x6E449684L,0xC509,0x11CF,0xAA,0xFA,0x00,0xAA,0x00,0xB6,0x01,0x5C);

#undef INTERFACE
#define INTERFACE IInstallEngine

DECLARE_INTERFACE_(IInstallEngine , IUnknown)
{
     // *** IUnknown methods ***
   STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
   STDMETHOD_(ULONG,Release) (THIS) PURE;

   // Methods to set engine up for install
   STDMETHOD(GetEngineStatus)(THIS_ DWORD *theenginestatus) PURE;
   STDMETHOD(SetCifFile)(THIS_ LPCSTR pszCabName, LPCSTR pszCifName) PURE;
   STDMETHOD(DownloadComponents)(THIS_ DWORD dwFlags) PURE;
   STDMETHOD(InstallComponents)(THIS_ DWORD dwFlags) PURE;
   STDMETHOD(EnumInstallIDs)(THIS_ UINT uIndex, LPSTR *ppszID) PURE;
   STDMETHOD(EnumDownloadIDs)(THIS_ UINT uIndex, LPSTR *ppszID) PURE;
   STDMETHOD(IsComponentInstalled)(THIS_ LPCSTR pszID, DWORD *pdwStatus) PURE;
   STDMETHOD(RegisterInstallEngineCallback)(THIS_ IInstallEngineCallback *pcb) PURE;
   STDMETHOD(UnregisterInstallEngineCallback)(THIS) PURE;
   STDMETHOD(SetAction)(THIS_ LPCSTR pszID, DWORD dwAction, DWORD dwPriority) PURE;
   STDMETHOD(GetSizes)(THIS_ LPCSTR pszID, COMPONENT_SIZES *pSizes) PURE; 
   STDMETHOD(LaunchExtraCommand)(THIS_ LPCSTR pszInfName, LPCSTR pszSection) PURE;
   STDMETHOD(GetDisplayName)(THIS_ LPCSTR pszID, LPSTR *ppszName) PURE;

   // Info about the install (should be structure to fill in
   //   like GetBindInfo (GetInstallInfo)
   STDMETHOD(SetBaseUrl)(THIS_ LPCSTR pszBaseName) PURE;
   STDMETHOD(SetDownloadDir)(THIS_ LPCSTR pszDownloadDir) PURE;
   STDMETHOD(SetInstallDrive)(THIS_ CHAR chDrive) PURE;
   STDMETHOD(SetInstallOptions)(THIS_ DWORD dwInsFlag) PURE;
   STDMETHOD(SetHWND)(THIS_ HWND hForUI) PURE;
   STDMETHOD(SetIStream)(THIS_ IStream *pstm) PURE;


   // Engine control during installation (seperate interface?)
   STDMETHOD(Abort)(THIS_ DWORD dwFlags) PURE;
   STDMETHOD(Suspend)(THIS) PURE;
   STDMETHOD(Resume)(THIS) PURE;

};

DEFINE_GUID(IID_IInstallEngineTiming,0x6E449687L,0xC509,0x11CF,0xAA,0xFA,0x00,0xAA,0x00,0xB6,0x01,0x5C);

#undef INTERFACE
#define INTERFACE IInstallEngineTiming

DECLARE_INTERFACE_(IInstallEngineTiming , IUnknown)
{
     // *** IUnknown methods ***
   STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
   STDMETHOD_(ULONG,Release) (THIS) PURE;

   STDMETHOD(GetRates)(THIS_ DWORD *pdwDownload, DWORD *pdwInstall) PURE;
   STDMETHOD(GetInstallProgress)(THIS_ INSTALLPROGRESS *pinsprog) PURE;
};


DEFINE_GUID(CLSID_InstallEngine,0x6E449686L,0xC509,0x11CF,0xAA,0xFA,0x00,0xAA,0x00,0xB6,0x01,0x5C);


//  The site manager interface

typedef struct
{
   UINT cbSize;
   LPSTR pszLang;
} SITEQUERYPARAMS;

typedef struct
{
   UINT cbSize;
   LPSTR pszUrl;
   LPSTR pszFriendlyName;
   LPSTR pszLang;
   LPSTR pszRegion;
} DOWNLOADSITE;


// {BFC880F3-7484-11d0-8309-00AA00B6015C}
DEFINE_GUID(IID_IDownloadSite, 
0xbfc880f3, 0x7484, 0x11d0, 0x83, 0x9, 0x0, 0xaa, 0x0, 0xb6, 0x1, 0x5c);

#undef INTERFACE
#define INTERFACE IDownloadSite

DECLARE_INTERFACE_(IDownloadSite , IUnknown)
{
     // *** IUnknown methods ***
   STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
   STDMETHOD_(ULONG,Release) (THIS) PURE;

   STDMETHOD(GetData)(THIS_ DOWNLOADSITE **pds) PURE;
};

// {BFC880F0-7484-11d0-8309-00AA00B6015C}
DEFINE_GUID(IID_IDownloadSiteMgr, 
0xbfc880f0, 0x7484, 0x11d0, 0x83, 0x9, 0x0, 0xaa, 0x0, 0xb6, 0x1, 0x5c);

#undef INTERFACE
#define INTERFACE IDownloadSiteMgr

DECLARE_INTERFACE_(IDownloadSiteMgr , IUnknown)
{
     // *** IUnknown methods ***
   STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
   STDMETHOD_(ULONG,Release) (THIS) PURE;

   STDMETHOD(Initialize)(THIS_ LPCSTR pszUrl, SITEQUERYPARAMS *pqp) PURE;
   STDMETHOD(EnumSites)(THIS_ DWORD dwIndex, IDownloadSite **pds) PURE;
};

// {BFC880F1-7484-11d0-8309-00AA00B6015C}
DEFINE_GUID(CLSID_DownloadSiteMgr, 
0xbfc880f1, 0x7484, 0x11d0, 0x83, 0x9, 0x0, 0xaa, 0x0, 0xb6, 0x1, 0x5c);

#ifdef __cplusplus
}
#endif

#define _INSENG_H_
#endif // 
