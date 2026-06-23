/*
 *              NK Kernel loader code
 *
 *              Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.
 *
 * Module Name:
 *
 *              loader.c
 *
 * Abstract:
 *
 *              This file implements the NK kernel loader for EXE/DLL
 *
 */

/* The loader is derived from the Win32 executable file format spec.  The only
   interesting thing is how we load a process.  When we load a process, we
   simply schedule a thread in a new process contect.  That thread (in coredll)
   knows how to read in an executable, and does so.  Then it traps into the kernel
   and starts running in the new process.  Most of the code below is simply a direct
   implimentation of the executable file format specification */

#include "kernel.h"

#define SYSTEMDIR L"\\Windows\\"
#define SYSTEMDIRLEN 9

struct KDataStruct *PtrKData;

fslog_t *LogPtr;

FREEINFO FreeInfo[2];
MEMORYINFO MemoryInfo;

ROMChain_t FirstROM;
ROMChain_t *ROMChain, *OEMRomChain;

extern CRITICAL_SECTION VAcs, DbgApiCS, LLcs, ModListcs, PagerCS;

DWORD MainMemoryEndAddress, PagedInCount;

typedef void (* NFC_t)(DWORD, DWORD, DWORD, DWORD);
NFC_t pNotifyForceCleanboot;
extern Name *pPath;

ROMHDR *const volatile pTOC = (ROMHDR *)-1;     // Gets replaced by RomLoader with real address

// ROM Header extension.  The ROM loader (RomImage) will set the pExtensions field of the table
// of contents to point to this structure.  This structure contains the PID and a extra field to point
// to further extensions.
const ROMPID RomExt = {
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	NULL
};

DWORD Decompress(LPBYTE BufIn, DWORD InSize, LPBYTE BufOut, DWORD OutSize, DWORD skip);
void GoToUserTime(void);
void GoToKernTime(void);

int currcrumb = -1;

#define SetBreadcrumb(P) ((P)->breadcrumb |= (1<<currcrumb))
#define ClearBreadcrumb(P) ((P)->breadcrumb &= ~(1<<currcrumb))
#define HasBreadcrumb(P) ((P)->breadcrumb & (1<<currcrumb))

_inline BOOL InitBreadcrumbs() {
	if (currcrumb == 31) {
		DEBUGCHK(0);
		return FALSE;
	}
	currcrumb++;
	return TRUE;
}

void FinishBreadcrumbs() {
	PMODULE pMod;
	DEBUGCHK(currcrumb != -1);
	DEBUGCHK(currcrumb < 32);
	for (pMod = pModList; pMod; pMod = pMod->pMod)
		ClearBreadcrumb(pMod);
	currcrumb--;
}

WCHAR lowerW(WCHAR ch) {
	return ((ch >= 'A') && (ch <= 'Z')) ? (ch - 'A' + 'a') : ch;
}

int strcmponeiW(const wchar_t *pwc1, const wchar_t *pwc2) {
	while (*pwc1 && (lowerW(*pwc1) == *pwc2)) {
		pwc1++;
		pwc2++;
	}
	return (*pwc1 ? 1 : 0);
}

int strcmpdllnameW(LPCWSTR src, LPCWSTR tgt) {
	while (*src && (*src == lowerW(*tgt))) {
		src++;
		tgt++;
	}
	return ((*tgt && strcmponeiW(tgt,L".dll") && strcmponeiW(tgt,L".cpl")) || (*src && memcmp(src,L".dll",10) && memcmp(src,L".cpl",10))) ? 1 : 0;
}

int strcmpiAandW(LPCHAR lpa, LPWSTR lpu) {
	while (*lpa && (lowerW((WCHAR)*lpa) == lowerW(*lpu))) {
		lpa++;
		lpu++;
	}
	return ((*lpa || *lpu) ? 1 : 0);
}

int kstrncmpi(LPWSTR str1, LPWSTR str2, int count) {
	wchar_t f,l;
	if (!count)
		return 0;
	do {
		f = lowerW(*str1++);
		l = lowerW(*str2++);
	} while (--count && f && (f == l));
	return (int)(f - l);
}

int kstrcmpi(LPWSTR str1, LPWSTR str2) {
	wchar_t f,l;
    do  {
		f = lowerW(*str1++);
		l = lowerW(*str2++);
    } while (f && (f == l));
    return (int)(f - l);
}

void kstrcpyW(LPWSTR p1, LPCWSTR p2) {
	while (*p2)
		*p1++ = *p2++;
	*p1 = 0;
}

typedef BOOL (* OEMLoadInit_t)(LPWSTR lpszName);
typedef BOOL (* OEMLoadModule_t)(LPBYTE lpData, DWORD cbData);

OEMLoadInit_t pOEMLoadInit;
OEMLoadModule_t pOEMLoadModule;

ERRFALSE(!OEM_CERTIFY_FALSE);

// returns getlasterror code, or 0 for success

DWORD VerifyBinary(openexe_t *oeptr, LPWSTR lpszName, LPBYTE pbTrustLevel) {
#define VBSIZE 1024
	BYTE Buf[VBSIZE];
	int iTrust;
	DWORD len, pos, size, bytesread;
	if ((oeptr->filetype == FT_ROMIMAGE) || !pOEMLoadInit || !pOEMLoadModule)
		return 0;
	if (!pOEMLoadInit(lpszName))
		return (DWORD)NTE_BAD_SIGNATURE;
	if (oeptr->filetype == FT_PPFSFILE)
		len = rlseek(oeptr->hppfs,0,2);
	else
		len = SetFilePointer(oeptr->hf,0,0,2);
	for (pos = 0; pos < len; pos += size) {
		size = ((len - pos) > VBSIZE ? VBSIZE : (len - pos));
		if (oeptr->filetype == FT_PPFSFILE) {
			if (rreadseek(oeptr->hppfs,Buf,size,pos) != (int)size)
				return (DWORD)NTE_BAD_SIGNATURE;
		} else {
			if (!ReadFileWithSeek(oeptr->hf,Buf,size,&bytesread,0,pos,0) || (bytesread != size))
				return (DWORD)NTE_BAD_SIGNATURE;
		}
		if (!pOEMLoadModule(Buf,size))
			return (DWORD)NTE_BAD_SIGNATURE;
	}
	if (!(iTrust = pOEMLoadModule(0,0)))
		return (DWORD)NTE_BAD_SIGNATURE;
	if (iTrust != OEM_CERTIFY_TRUST)
		*pbTrustLevel = KERN_TRUST_RUN;
	return 0;
}

BOOL OpenExe(LPWSTR lpszName, openexe_t *oeptr, BOOL bIsDLL, BOOL bAllowPaging) {
	static OEinfo_t OEinfo;
	return SafeOpenExe(lpszName,oeptr,bIsDLL,bAllowPaging,&OEinfo);
}

BOOL SafeOpenExe(LPWSTR lpszName, openexe_t *oeptr, BOOL bIsDLL, BOOL bAllowPaging, OEinfo_t *poeinfo) {
	CEGUID sysguid;
	TOCentry *tocptr;
	int loop, plainlen, totlen, copylen;
	LPWSTR nameptr, p2;
	BOOL inWin, isAbs;
	DWORD dwLastError;
	DWORD bytesread;
	HANDLE hFind;
	DEBUGMSG(ZONE_OPENEXE,(TEXT("OpenExe: %s\r\n"),lpszName));
	dwLastError = KGetLastError(pCurThread);
	isAbs = ((*lpszName == '\\') || (*lpszName == '/'));
	totlen = strlenW(lpszName);
	nameptr = lpszName + totlen;
	while ((nameptr != lpszName) && (*nameptr != '\\') && (*nameptr != '/'))
		nameptr--;
	if ((*nameptr == '\\') || (*nameptr == '/'))
		nameptr++;
	inWin = 0;
	if ((nameptr != lpszName) && (nameptr != lpszName+1)) {
		if (((nameptr == lpszName + SYSTEMDIRLEN) && !kstrncmpi(lpszName,SYSTEMDIR,SYSTEMDIRLEN)) ||
			((nameptr == lpszName + SYSTEMDIRLEN - 1) && !kstrncmpi(lpszName,SYSTEMDIR+1,SYSTEMDIRLEN-1)))
			inWin = 1;
		else {
			memcpy(poeinfo->tmpname,lpszName,(nameptr-lpszName-1)*sizeof(WCHAR));
			poeinfo->tmpname[nameptr-lpszName-1] = 0;
			hFind = FindFirstFile(poeinfo->tmpname,&poeinfo->wfd);
			if (hFind != INVALID_HANDLE_VALUE) {
				CloseHandle(hFind);
				if (poeinfo->wfd.dwOID == 1)
					inWin = 1;
			}
		}
	}
	p2 = (LPWSTR)poeinfo->plainname;
	while (*nameptr && (p2 - poeinfo->plainname < MAX_PATH-1))
		*p2++ = *nameptr++;
	*p2 = 0;
	plainlen = p2 - poeinfo->plainname;
	DEBUGMSG(ZONE_OPENEXE,(TEXT("OpenExe: plain name %s\r\n"),poeinfo->plainname));
	oeptr->bIsOID = 1;
	oeptr->lpName = 0;
	if (!(oeptr->lpName = AllocName(MAX_PATH*2))) {
		KSetLastError(pCurThread,dwLastError);
		return 0;
	}
	// check the file system
	if (SystemAPISets[SH_FILESYS_APIS]) {
		if (!isAbs) {
			// if dll, check in directory exe launched from
			if (bIsDLL) {
				PPROCESS pProc;
				pProc = (pCurThread->pcstkTop ? pCurThread->pcstkTop->pprcLast : pCurProc);
				if (pProc->oe.filetype != FT_ROMIMAGE) {
					if (pProc->oe.bIsOID) {
						CREATE_SYSTEMGUID(&sysguid);						
						if (!(CeOidGetInfoEx(&sysguid,pProc->oe.ceOid,&poeinfo->ceoi)) || (poeinfo->ceoi.wObjType != OBJTYPE_FILE))
							goto DoWinDir;
					} else
						kstrcpyW(poeinfo->ceoi.infFile.szFileName,pProc->oe.lpName->name);
					nameptr = poeinfo->ceoi.infFile.szFileName + strlenW(poeinfo->ceoi.infFile.szFileName);
					while ((*nameptr != '\\') && (*nameptr != '/') && (nameptr != poeinfo->ceoi.infFile.szFileName))
						nameptr--;
					if ((nameptr != poeinfo->ceoi.infFile.szFileName) && (MAX_PATH - 1 - (nameptr + 1 - poeinfo->ceoi.infFile.szFileName) >= plainlen)) {
						memcpy(nameptr+1,poeinfo->plainname,plainlen*sizeof(WCHAR));
						nameptr[plainlen+1] = 0;
						if ((oeptr->hf = CreateFileW(poeinfo->ceoi.infFile.szFileName,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,0,0)) != INVALID_HANDLE_VALUE) {
							if (GetFileInformationByHandle(oeptr->hf,&poeinfo->bhfi) && (poeinfo->bhfi.dwOID != (ULONG)INVALID_HANDLE_VALUE)) {
								FreeName(oeptr->lpName);
								oeptr->ceOid = poeinfo->bhfi.dwOID;
							} else {
								LPName pName;
								oeptr->bIsOID = 0;
								if (pName = AllocName((strlenW(poeinfo->ceoi.infFile.szFileName)+1)*2)) {
									FreeName(oeptr->lpName);
									oeptr->lpName = pName;
								}
								kstrcpyW(oeptr->lpName->name,poeinfo->ceoi.infFile.szFileName);
							}
							oeptr->filetype = FT_OBJSTORE;
							oeptr->pagemode = (bAllowPaging && !(pTOC->ulKernelFlags & KFLAG_DISALLOW_PAGING) &&
								ReadFileWithSeek(oeptr->hf,0,0,0,0,0,0)) ? PM_FULLPAGING : PM_NOPAGING;
							if ((SetFilePointer(oeptr->hf,0x3c,0,FILE_BEGIN) != 0x3c) ||
								!ReadFile(oeptr->hf,(LPBYTE)&oeptr->offset,4,&bytesread,0) || (bytesread != 4)) {
								CloseExe(oeptr);
								return 0;
							}
							KSetLastError(pCurThread,dwLastError);
							return 1;
						}
					}
				}
			}
DoWinDir:
			oeptr->hf = INVALID_HANDLE_VALUE;
			// check for file as path from root
			if (plainlen != totlen) {
				poeinfo->tmpname[0] = '\\';
				copylen = (totlen < MAX_PATH - 2 ? totlen : MAX_PATH - 2);
				memcpy((LPWSTR)poeinfo->tmpname+1,lpszName,totlen*sizeof(WCHAR));
				poeinfo->tmpname[1+copylen] = 0;
				oeptr->hf = CreateFileW((LPWSTR)poeinfo->tmpname,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,0,0);
			}
			if (oeptr->hf == INVALID_HANDLE_VALUE) {
				// check for file in windows directory
				memcpy(poeinfo->tmpname,SYSTEMDIR,SYSTEMDIRLEN*sizeof(WCHAR));
				copylen = (plainlen + SYSTEMDIRLEN < MAX_PATH-1) ? plainlen : MAX_PATH - 2 - SYSTEMDIRLEN;
				memcpy((LPWSTR)poeinfo->tmpname+SYSTEMDIRLEN,poeinfo->plainname,copylen*sizeof(WCHAR));
				poeinfo->tmpname[SYSTEMDIRLEN+copylen] = 0;
				oeptr->hf = CreateFileW((LPWSTR)poeinfo->tmpname,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,0,0);
			}
			if (oeptr->hf == INVALID_HANDLE_VALUE) {
				// check for file in root directory
				poeinfo->tmpname[0] = '\\';
				copylen = (plainlen + 1 < MAX_PATH-1) ? plainlen : MAX_PATH - 3;
				memcpy((LPWSTR)poeinfo->tmpname+1,poeinfo->plainname,copylen*sizeof(WCHAR));
				poeinfo->tmpname[1+copylen] = 0;
				oeptr->hf = CreateFileW((LPWSTR)poeinfo->tmpname,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,0,0);
			}
			if (oeptr->hf != INVALID_HANDLE_VALUE) {
				if (GetFileInformationByHandle(oeptr->hf,&poeinfo->bhfi) && (poeinfo->bhfi.dwOID != (ULONG)INVALID_HANDLE_VALUE)) {
					FreeName(oeptr->lpName);
					oeptr->ceOid = poeinfo->bhfi.dwOID;
				} else {
					LPName pName;
					oeptr->bIsOID = 0;
					if (pName = AllocName((strlenW(poeinfo->tmpname)+1)*2)) {
						FreeName(oeptr->lpName);
						oeptr->lpName = pName;
					}
					kstrcpyW(oeptr->lpName->name,poeinfo->tmpname);
				}
			}
		} else {
			// check in main file system (will route to mounted if part of path)
			if ((oeptr->hf = CreateFileW((LPWSTR)lpszName,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,0,0)) != INVALID_HANDLE_VALUE) {
				if (GetFileInformationByHandle(oeptr->hf,&poeinfo->bhfi) && (poeinfo->bhfi.dwOID != (ULONG)INVALID_HANDLE_VALUE)) {
					FreeName(oeptr->lpName);
					oeptr->ceOid = poeinfo->bhfi.dwOID;
				} else {
					LPName pName;
					oeptr->bIsOID = 0;
					if (pName = AllocName((strlenW(lpszName)+1)*2)) {
						FreeName(oeptr->lpName);
						oeptr->lpName = pName;
					}
					kstrcpyW(oeptr->lpName->name,lpszName);
				}
			}
		}
		if (oeptr->hf != INVALID_HANDLE_VALUE) {
			DEBUGMSG(ZONE_OPENEXE,(TEXT("OpenExe %s: OSTORE: %8.8lx\r\n"),lpszName,oeptr->hf));
			oeptr->filetype = FT_OBJSTORE;
			oeptr->pagemode = (bAllowPaging && !(pTOC->ulKernelFlags & KFLAG_DISALLOW_PAGING) &&
				ReadFileWithSeek(oeptr->hf,0,0,0,0,0,0)) ? PM_FULLPAGING : PM_NOPAGING;
			if ((SetFilePointer(oeptr->hf,0x3c,0,FILE_BEGIN) != 0x3c) ||
				!ReadFile(oeptr->hf,(LPBYTE)&oeptr->offset,4,&bytesread,0) || (bytesread != 4)) {
				CloseExe(oeptr);
				return 0;
			}
			KSetLastError(pCurThread,dwLastError);
			return 1;
		}
	}
	if ((plainlen == totlen) || inWin) {
		ROMChain_t *pROM;
		for (pROM = ROMChain; pROM; pROM = pROM->pNext) {
			// check ROM for any copy
			tocptr = (TOCentry *)(pROM->pTOC+1);  // toc entries follow the header
			for (loop = 0; loop < (int)pROM->pTOC->nummods; loop++) {
				if (!strcmpiAandW(tocptr->lpszFileName,poeinfo->plainname)) {
					DEBUGMSG(ZONE_OPENEXE,(TEXT("OpenExe %s: ROM: %8.8lx\r\n"),lpszName,tocptr));
					oeptr->tocptr = tocptr;
					oeptr->filetype = FT_ROMIMAGE;
					oeptr->pagemode = (bAllowPaging && !(pTOC->ulKernelFlags & KFLAG_DISALLOW_PAGING)) ? PM_FULLPAGING : PM_NOPAGING;
					KSetLastError(pCurThread,dwLastError);
					if (oeptr->lpName)
						FreeName(oeptr->lpName,);
					return 1;
				}
				tocptr++;
			}
		}
	}
	if (!isAbs && pPath) {
		LPWSTR pTrav = pPath->name;
		int len;
		while (*pTrav) {
			len = strlenW(pTrav);
			memcpy(poeinfo->tmpname,pTrav,len*sizeof(WCHAR));
			copylen = (totlen + len < MAX_PATH-1) ? totlen : MAX_PATH - 2 - len;
			memcpy((LPWSTR)poeinfo->tmpname+len,lpszName,copylen*sizeof(WCHAR));
			poeinfo->tmpname[len+copylen] = 0;
			if ((oeptr->hf = CreateFileW((LPWSTR)poeinfo->tmpname,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,0,0)) != INVALID_HANDLE_VALUE) {
				if (GetFileInformationByHandle(oeptr->hf,&poeinfo->bhfi) && (poeinfo->bhfi.dwOID != (ULONG)INVALID_HANDLE_VALUE)) {
					FreeName(oeptr->lpName);
					oeptr->ceOid = poeinfo->bhfi.dwOID;
				} else {
					LPName pName;
					oeptr->bIsOID = 0;
					if (pName = AllocName((strlenW(poeinfo->tmpname)+1)*2)) {
						FreeName(oeptr->lpName);
						oeptr->lpName = pName;
					}
					kstrcpyW(oeptr->lpName->name,poeinfo->tmpname);
				}
				oeptr->filetype = FT_OBJSTORE;
				oeptr->pagemode = (bAllowPaging && !(pTOC->ulKernelFlags & KFLAG_DISALLOW_PAGING) &&
					ReadFileWithSeek(oeptr->hf,0,0,0,0,0,0)) ? PM_FULLPAGING : PM_NOPAGING;
				if ((SetFilePointer(oeptr->hf,0x3c,0,FILE_BEGIN) != 0x3c) ||
					!ReadFile(oeptr->hf,(LPBYTE)&oeptr->offset,4,&bytesread,0) || (bytesread != 4)) {
					CloseExe(oeptr);
					return 0;
				}
				KSetLastError(pCurThread,dwLastError);
				return 1;
			}
			pTrav += len + 1;
		}
	}
	if ((plainlen == totlen) || inWin) {
		// check PPFS, if string is short (PPFS doesn't like big names)
		if (plainlen < 128) {
			oeptr->hppfs = ropen(poeinfo->plainname,0x10000);
			if (oeptr->hppfs != HFILE_ERROR) {
				LPName pName;
				RETAILMSG(1,(L"Kernel Loader: Using PPFS to load file %s\r\n",lpszName));
				oeptr->filetype = FT_PPFSFILE;
				oeptr->pagemode = (bAllowPaging && !(pTOC->ulKernelFlags & KFLAG_DISALLOW_PAGING)) ? PM_FULLPAGING : PM_NOPAGING;
				if ((rlseek(oeptr->hppfs,0x3c,0) != 0x3c) ||
					(rread(oeptr->hppfs,(LPBYTE)&oeptr->offset,4) != 4)) {
					CloseExe(oeptr);
					return 0;
				}
				oeptr->bIsOID = 0;
				if (pName = AllocName((strlenW(poeinfo->plainname)+9+1)*2)) {
					FreeName(oeptr->lpName);
					oeptr->lpName = pName;
				}
				memcpy(oeptr->lpName->name,L"\\Windows\\",9*sizeof(WCHAR));
				kstrcpyW(&oeptr->lpName->name[9],poeinfo->plainname);
				KSetLastError(pCurThread,dwLastError);
				return 1;
			}
		}
	}
	DEBUGMSG(ZONE_OPENEXE,(TEXT("OpenExe %s: failed!\r\n"),lpszName));
	// If not, it failed!
	if (oeptr->lpName)
		FreeName(oeptr->lpName);
	return 0;
}

void CloseExe(openexe_t *oeptr) {
	CALLBACKINFO cbi;
	if (oeptr && oeptr->filetype) {
		cbi.hProc = ProcArray[0].hProc;
		if (oeptr->filetype == FT_PPFSFILE) {
			cbi.pfn = (FARPROC)rclose;
			cbi.pvArg0 = (LPVOID)oeptr->hppfs;
			PerformCallBack4Int(&cbi);
		} else if (oeptr->filetype == FT_OBJSTORE) {
			cbi.pfn = (FARPROC)CloseHandle;
			cbi.pvArg0 = oeptr->hf;
			PerformCallBack4Int(&cbi);
		}
		if (!oeptr->bIsOID && oeptr->lpName)
			FreeName(oeptr->lpName);
		oeptr->lpName = 0;
		oeptr->filetype = 0;
	}
}

/*
	@doc INTERNAL
	@func void | CloseProcOE | Closes down the internal file handle for the process
	@comm Used by apis.c in final process termination before doing final kernel syscall
*/

void SC_CloseProcOE(DWORD bCloseFile) {
	THREAD *pdbgr, *ptrav;
	DEBUGMSG(ZONE_ENTRY,(L"SC_CloseProcOE entry\r\n"));
	if (bCloseFile == 2) {
		if (!GET_DYING(pCurThread))
			SetUserInfo(hCurThread,KGetLastError(pCurThread));
		SET_DYING(pCurThread);
		SET_DEAD(pCurThread);
		DEBUGMSG(ZONE_ENTRY,(L"SC_CloseProcOE exit (2)\r\n"));
		return;
	}
	EnterCriticalSection(&DbgApiCS);
	if (pCurThread->pThrdDbg && pCurThread->pThrdDbg->hEvent) {
		if (pCurProc->hDbgrThrd) {
			pdbgr = HandleToThread(pCurProc->hDbgrThrd);
			if (pdbgr->pThrdDbg->hFirstThrd == hCurThread)
				pdbgr->pThrdDbg->hFirstThrd = pCurThread->pThrdDbg->hNextThrd;
			else {
				ptrav = HandleToThread(pdbgr->pThrdDbg->hFirstThrd);
				while (ptrav->pThrdDbg->hNextThrd != hCurThread)
					ptrav = HandleToThread(ptrav->pThrdDbg->hNextThrd);
					ptrav->pThrdDbg->hNextThrd = pCurThread->pThrdDbg->hNextThrd;
			}
		}
		pCurThread->pThrdDbg->dbginfo.dwDebugEventCode = 0;
		SetEvent(pCurThread->pThrdDbg->hEvent);
		CloseHandle(pCurThread->pThrdDbg->hEvent);
		pCurThread->pThrdDbg->hEvent = 0;
		pCurThread->pThrdDbg->dbginfo.dwDebugEventCode = 0;
		if (pCurThread->pThrdDbg->hBlockEvent) {
			CloseHandle(pCurThread->pThrdDbg->hBlockEvent);
			pCurThread->pThrdDbg->hBlockEvent = 0;
		}
	}
	LeaveCriticalSection(&DbgApiCS);
	if (bCloseFile) {
		KDUpdateSymbols(((DWORD)pCurProc->BasePtr)+1, TRUE);
		CloseExe(&pCurProc->oe);
		CloseAllCrits();
	}
	DEBUGMSG(ZONE_ENTRY,(L"SC_CloseProcOE exit\r\n"));
}

#ifdef DEBUG

LPWSTR e32str[STD_EXTRA] = {
	TEXT("EXP"),TEXT("IMP"),TEXT("RES"),TEXT("EXC"),TEXT("SEC"),TEXT("FIX"),TEXT("DEB"),TEXT("IMD"),
	TEXT("MSP"),TEXT("TLS"),TEXT("CBK"),TEXT("RS1"),TEXT("RS2"),TEXT("RS3"),TEXT("RS4"),TEXT("RS5"),
};

// Dump e32 header

void DumpHeader(e32_exe *e32) {
	int loop;
	DEBUGMSG(ZONE_LOADER1,(TEXT("  e32_magic      : %8.8lx\r\n"),*(LPDWORD)&e32->e32_magic));
	DEBUGMSG(ZONE_LOADER1,(TEXT("  e32_cpu        : %8.8lx\r\n"),e32->e32_cpu));
	DEBUGMSG(ZONE_LOADER1,(TEXT("  e32_objcnt     : %8.8lx\r\n"),e32->e32_objcnt));
	DEBUGMSG(ZONE_LOADER1,(TEXT("  e32_symtaboff  : %8.8lx\r\n"),e32->e32_symtaboff));
	DEBUGMSG(ZONE_LOADER1,(TEXT("  e32_symcount   : %8.8lx\r\n"),e32->e32_symcount));
	DEBUGMSG(ZONE_LOADER1,(TEXT("  e32_timestamp  : %8.8lx\r\n"),e32->e32_timestamp));
	DEBUGMSG(ZONE_LOADER1,(TEXT("  e32_opthdrsize : %8.8lx\r\n"),e32->e32_opthdrsize));
	DEBUGMSG(ZONE_LOADER1,(TEXT("  e32_imageflags : %8.8lx\r\n"),e32->e32_imageflags));
	DEBUGMSG(ZONE_LOADER1,(TEXT("  e32_coffmagic  : %8.8lx\r\n"),e32->e32_coffmagic));
	DEBUGMSG(ZONE_LOADER1,(TEXT("  e32_linkmajor  : %8.8lx\r\n"),e32->e32_linkmajor));
	DEBUGMSG(ZONE_LOADER1,(TEXT("  e32_linkminor  : %8.8lx\r\n"),e32->e32_linkminor));
	DEBUGMSG(ZONE_LOADER1,(TEXT("  e32_codesize   : %8.8lx\r\n"),e32->e32_codesize));
	DEBUGMSG(ZONE_LOADER1,(TEXT("  e32_initdsize  : %8.8lx\r\n"),e32->e32_initdsize));
	DEBUGMSG(ZONE_LOADER1,(TEXT("  e32_uninitdsize: %8.8lx\r\n"),e32->e32_uninitdsize));
	DEBUGMSG(ZONE_LOADER1,(TEXT("  e32_entryrva   : %8.8lx\r\n"),e32->e32_entryrva));
	DEBUGMSG(ZONE_LOADER1,(TEXT("  e32_codebase   : %8.8lx\r\n"),e32->e32_codebase));
	DEBUGMSG(ZONE_LOADER1,(TEXT("  e32_database   : %8.8lx\r\n"),e32->e32_database));
	DEBUGMSG(ZONE_LOADER1,(TEXT("  e32_vbase      : %8.8lx\r\n"),e32->e32_vbase));
	DEBUGMSG(ZONE_LOADER1,(TEXT("  e32_objalign   : %8.8lx\r\n"),e32->e32_objalign));
	DEBUGMSG(ZONE_LOADER1,(TEXT("  e32_filealign  : %8.8lx\r\n"),e32->e32_filealign));
	DEBUGMSG(ZONE_LOADER1,(TEXT("  e32_osmajor    : %8.8lx\r\n"),e32->e32_osmajor));
	DEBUGMSG(ZONE_LOADER1,(TEXT("  e32_osminor    : %8.8lx\r\n"),e32->e32_osminor));
	DEBUGMSG(ZONE_LOADER1,(TEXT("  e32_usermajor  : %8.8lx\r\n"),e32->e32_usermajor));
	DEBUGMSG(ZONE_LOADER1,(TEXT("  e32_userminor  : %8.8lx\r\n"),e32->e32_userminor));
	DEBUGMSG(ZONE_LOADER1,(TEXT("  e32_subsysmajor: %8.8lx\r\n"),e32->e32_subsysmajor));
	DEBUGMSG(ZONE_LOADER1,(TEXT("  e32_subsysminor: %8.8lx\r\n"),e32->e32_subsysminor));
	DEBUGMSG(ZONE_LOADER1,(TEXT("  e32_vsize      : %8.8lx\r\n"),e32->e32_vsize));
	DEBUGMSG(ZONE_LOADER1,(TEXT("  e32_hdrsize    : %8.8lx\r\n"),e32->e32_hdrsize));
	DEBUGMSG(ZONE_LOADER1,(TEXT("  e32_filechksum : %8.8lx\r\n"),e32->e32_filechksum));
	DEBUGMSG(ZONE_LOADER1,(TEXT("  e32_subsys     : %8.8lx\r\n"),e32->e32_subsys));
	DEBUGMSG(ZONE_LOADER1,(TEXT("  e32_dllflags   : %8.8lx\r\n"),e32->e32_dllflags));
	DEBUGMSG(ZONE_LOADER1,(TEXT("  e32_stackmax   : %8.8lx\r\n"),e32->e32_stackmax));
	DEBUGMSG(ZONE_LOADER1,(TEXT("  e32_stackinit  : %8.8lx\r\n"),e32->e32_stackinit));
	DEBUGMSG(ZONE_LOADER1,(TEXT("  e32_heapmax    : %8.8lx\r\n"),e32->e32_heapmax));
	DEBUGMSG(ZONE_LOADER1,(TEXT("  e32_heapinit   : %8.8lx\r\n"),e32->e32_heapinit));
	DEBUGMSG(ZONE_LOADER1,(TEXT("  e32_hdrextra   : %8.8lx\r\n"),e32->e32_hdrextra));
	for (loop = 0; loop < STD_EXTRA; loop++)
		DEBUGMSG(ZONE_LOADER1,(TEXT("  e32_unit:%3.3s	 : %8.8lx %8.8lx\r\n"),
			e32str[loop],e32->e32_unit[loop].rva,e32->e32_unit[loop].size));
}

// Dump o32 header

void DumpSection(o32_obj *o32, o32_lite *o32l) {
	ulong loop;
	DEBUGMSG(ZONE_LOADER1,(TEXT("Section %a:\r\n"),o32->o32_name));
	DEBUGMSG(ZONE_LOADER1,(TEXT("  o32_vsize      : %8.8lx\r\n"),o32->o32_vsize));
	DEBUGMSG(ZONE_LOADER1,(TEXT("  o32_rva        : %8.8lx\r\n"),o32->o32_rva));
	DEBUGMSG(ZONE_LOADER1,(TEXT("  o32_psize      : %8.8lx\r\n"),o32->o32_psize));
	DEBUGMSG(ZONE_LOADER1,(TEXT("  o32_dataptr    : %8.8lx\r\n"),o32->o32_dataptr));
	DEBUGMSG(ZONE_LOADER1,(TEXT("  o32_realaddr   : %8.8lx\r\n"),o32->o32_realaddr));
	DEBUGMSG(ZONE_LOADER1,(TEXT("  o32_flags      : %8.8lx\r\n"),o32->o32_flags));
	DEBUGMSG(ZONE_LOADER1,(TEXT("  o32l_vsize     : %8.8lx\r\n"),o32l->o32_vsize));
	DEBUGMSG(ZONE_LOADER1,(TEXT("  o32l_rva       : %8.8lx\r\n"),o32l->o32_rva));
	DEBUGMSG(ZONE_LOADER1,(TEXT("  o32l_realaddr  : %8.8lx\r\n"),o32l->o32_realaddr));
	DEBUGMSG(ZONE_LOADER1,(TEXT("  o32l_flags     : %8.8lx\r\n"),o32l->o32_flags));
	for (loop = 0; loop+16 <= o32l->o32_vsize; loop+=16) {
		DEBUGMSG(ZONE_LOADER2,(TEXT(" %8.8lx: %8.8lx %8.8lx %8.8lx %8.8lx\r\n"),
			loop,
			*(LPDWORD)(o32l->o32_realaddr+loop),
			*(LPDWORD)(o32l->o32_realaddr+loop+4),
			*(LPDWORD)(o32l->o32_realaddr+loop+8),
			*(LPDWORD)(o32l->o32_realaddr+loop+12)));
		}
	if (loop != o32l->o32_vsize) {
		DEBUGMSG(ZONE_LOADER2,(TEXT(" %8.8lx:"),loop));
		while (loop+4 <= o32l->o32_vsize) {
			DEBUGMSG(ZONE_LOADER2,(TEXT(" %8.8lx"),*(LPDWORD)(o32l->o32_realaddr+loop)));
			loop += 4;
		}
		if (loop+2 <= o32l->o32_vsize)
			DEBUGMSG(ZONE_LOADER2,(TEXT(" %4.4lx"),(DWORD)*(LPWORD)(o32l->o32_realaddr+loop)));
		DEBUGMSG(ZONE_LOADER2,(TEXT("\r\n")));
	}
}

#endif

void SC_SetCleanRebootFlag(void) {
	DEBUGMSG(ZONE_ENTRY,(L"SC_SetCleanRebootFlag entry\r\n"));
	ERRORMSG(pCurProc->bTrustLevel != KERN_TRUST_FULL,(L"SC_SetCleanRebootFlag failed due to insufficient trust\r\n"));
	if (pCurProc->bTrustLevel == KERN_TRUST_FULL)
		LogPtr->magic1 = 0;
	DEBUGMSG(ZONE_ENTRY,(L"SC_SetCleanRebootFlag exit\r\n"));
}

// Relocate the kernel

void KernelRelocate(ROMHDR *const pTOC)
{
	ULONG loop;
	COPYentry *cptr;
#ifdef DEBUG    
	if (pTOC == (ROMHDR *const) 0xffffffff) {
		OEMWriteDebugString(TEXT("ERROR: Kernel must be part of ROM image!\r\n"));
		while (1) ; 
	}
#endif
	KInfoTable[KINX_PTOC] = (long)pTOC;
	// This is where the data sections become valid... don't read globals until after this
	for (loop = 0; loop < pTOC->ulCopyEntries; loop++) {
		cptr = (COPYentry *)(pTOC->ulCopyOffset + loop*sizeof(COPYentry));
		if (cptr->ulCopyLen)
			memcpy((LPVOID)cptr->ulDest,(LPVOID)cptr->ulSource,cptr->ulCopyLen);
		if (cptr->ulCopyLen != cptr->ulDestLen)
			memset((LPVOID)(cptr->ulDest+cptr->ulCopyLen),0,cptr->ulDestLen-cptr->ulCopyLen);
	}
	// Now you can read global variables...
	PtrKData = &KData;	// Set this to allow displaying the kpage from the KD.
	MainMemoryEndAddress = pTOC->ulRAMEnd;
	FirstROM.pTOC = pTOC;
	FirstROM.pNext = 0;
	ROMChain = &FirstROM;
}

void SC_NotifyForceCleanboot(void) {
	if (pNotifyForceCleanboot)
		pNotifyForceCleanboot(LogPtr->fsmemblk[0].startptr,LogPtr->fsmemblk[0].length,
			LogPtr->fsmemblk[1].startptr,LogPtr->fsmemblk[1].length);
}

// should match same define in filesys\heap\heap.c
#define MAX_FILESYSTEM_HEAP_SIZE 256*1024*1024

DWORD CalcFSPages(DWORD pages) {
	DWORD fspages;
	if (pages <= 2*1024*1024/PAGE_SIZE) {
		fspages = (pages*(pTOC->ulFSRamPercent&0xff))/256;
	} else {
		fspages = ((2*1024*1024/PAGE_SIZE)*(pTOC->ulFSRamPercent&0xff))/256;
		if (pages <= 4*1024*1024/PAGE_SIZE) {
			fspages += ((pages-2*1024*1024/PAGE_SIZE)*((pTOC->ulFSRamPercent>>8)&0xff))/256;
		} else {
			fspages += ((2*1024*1024/PAGE_SIZE)*((pTOC->ulFSRamPercent>>8)&0xff))/256;
			if (pages <= 6*1024*1024/PAGE_SIZE) {
				fspages += ((pages-4*1024*1024/PAGE_SIZE)*((pTOC->ulFSRamPercent>>16)&0xff))/256;
			} else {
				fspages += ((2*1024*1024/PAGE_SIZE)*((pTOC->ulFSRamPercent>>16)&0xff))/256;
				fspages += ((pages-6*1024*1024/PAGE_SIZE)*((pTOC->ulFSRamPercent>>24)&0xff))/256;
			}
		}
	}
	return fspages;
}

DWORD CarveMem(LPBYTE pMem, DWORD dwExt, DWORD dwLen, DWORD dwFSPages) {
	DWORD dwPages, dwNow, loop;
	LPBYTE pCur, pPage;
	dwPages = dwLen / PAGE_SIZE;
	pCur = pMem + dwExt;
	while (dwPages && dwFSPages) {
		pPage = pCur;
		pCur += PAGE_SIZE;
		dwPages--;
		*(LPBYTE *)pPage = LogPtr->pFSList;
		LogPtr->pFSList = pPage;
		dwNow = dwPages;
		if (dwNow > dwFSPages)
			dwNow = dwFSPages;
		if (dwNow > (PAGE_SIZE/4) - 2)
			dwNow = (PAGE_SIZE/4) - 2;
		*((LPDWORD)pPage+1) = dwNow;
		for (loop = 0; loop < dwNow; loop++) {
			*((LPDWORD)pPage+2+loop) = (DWORD)pCur;
			pCur += PAGE_SIZE;
		}
		dwPages -= dwNow;
		dwFSPages -= dwNow;
	}
	return dwFSPages;
}

void RemovePage(DWORD dwAddr);

void GrabFSPages(void) {
	LPBYTE pPageList;
	DWORD loop,max;
	pPageList = LogPtr->pFSList;
	while (pPageList) {
		max = *((LPDWORD)pPageList+1);
		RemovePage((DWORD)pPageList);
		for (loop = 0; loop < max; loop++)
			RemovePage(*((LPDWORD)pPageList+2+loop));
		pPageList = *(LPBYTE *)pPageList;
	}
}



// need to avoid kernel stack overflow while system starting
#ifdef SHIP_BUILD

#define RTLMSG(cond,printf_exp) ((void)0)

#else

#define RTLMSG(cond,printf_exp)   \
   ((cond)?(_NKDbgPrintfW printf_exp),1:0)

//extern int NKwvsprintfW(LPWSTR lpOut, LPCWSTR lpFmt, va_list lpParms, int maxchars);
static VOID _NKvDbgPrintfW(LPCWSTR lpszFmt, va_list lpParms) {
	static WCHAR rgchBuf[384];
	NKwvsprintfW(rgchBuf, lpszFmt, lpParms, sizeof(rgchBuf)/sizeof(WCHAR));
	OutputDebugStringW(rgchBuf);
}

static void _NKDbgPrintfW(LPCWSTR lpszFmt, ...)  {
    va_list arglist;
    va_start(arglist, lpszFmt);
    _NKvDbgPrintfW(lpszFmt, arglist);
    va_end(arglist);
}

#endif	// SHIP_BUILD

void KernelFindMemory(void) {
	ROMChain_t *pList;
	// *MUST* make the pointer uncached ( thus or in 0x20000000)
	if (OEMRomChain) {
		pList = OEMRomChain;
		while (pList->pNext)
			pList = pList->pNext;
		pList->pNext = ROMChain;
		ROMChain = OEMRomChain;
	}
	LogPtr = (fslog_t *)(pTOC->ulRAMFree | 0x20000000);
	RTLMSG(1,(L"Booting Windows CE version %d.%2.2d\r\n",CE_VERSION_MAJOR,CE_VERSION_MINOR));
	if (LogPtr->version != CURRENT_FSLOG_VERSION) {
		RTLMSG(1,(L"\r\nOld or invalid version stamp in kernel structures - starting clean!\r\n"));
		LogPtr->magic1 = LogPtr->magic2 = 0;
		LogPtr->version = CURRENT_FSLOG_VERSION;
	}
	if (LogPtr->magic1 != LOG_MAGIC) {
		DWORD fspages, memst, memlen, mainpages, secondpages;
		mainpages = (PAGEALIGN_DOWN(MainMemoryEndAddress) - PAGEALIGN_UP(pTOC->ulRAMFree))/PAGE_SIZE - 4096/PAGE_SIZE;
		LogPtr->fsmemblk[0].startptr = PAGEALIGN_UP(pTOC->ulRAMFree) + 4096;
		LogPtr->fsmemblk[0].extension = PAGEALIGN_UP(mainpages);
		mainpages -= LogPtr->fsmemblk[0].extension/PAGE_SIZE;
		LogPtr->fsmemblk[0].length = mainpages * PAGE_SIZE;
		LogPtr->fsmemblk[1].startptr = 0;
		LogPtr->fsmemblk[1].length = 0;
		LogPtr->fsmemblk[1].extension = 0;
		LogPtr->magic2 = 0;
		if (OEMGetExtensionDRAM(&memst,&memlen) && (memlen > (memst - PAGEALIGN_UP(memst)))) {
			memlen -= (PAGEALIGN_UP(memst) - memst);
			memst = PAGEALIGN_UP(memst);
			memlen = PAGEALIGN_DOWN(memlen);
			secondpages = memlen/PAGE_SIZE;
			LogPtr->fsmemblk[1].startptr = memst;
			LogPtr->fsmemblk[1].extension = PAGEALIGN_UP(secondpages);
			secondpages -= LogPtr->fsmemblk[1].extension/PAGE_SIZE;
			LogPtr->fsmemblk[1].length = secondpages * PAGE_SIZE;
		} else
			secondpages = 0;
		fspages = CalcFSPages(mainpages+secondpages);
		if (fspages * PAGE_SIZE > MAX_FILESYSTEM_HEAP_SIZE)
			fspages = MAX_FILESYSTEM_HEAP_SIZE/PAGE_SIZE;
		RTLMSG(1,(L"Configuring: Primary pages: %d, Secondary pages: %d, Filesystem pages = %d\r\n",
			mainpages,secondpages,fspages));

		LogPtr->pFSList = 0;

		fspages = CarveMem((LPBYTE)LogPtr->fsmemblk[0].startptr,LogPtr->fsmemblk[0].extension,LogPtr->fsmemblk[0].length,fspages);	
		fspages = CarveMem((LPBYTE)LogPtr->fsmemblk[1].startptr,LogPtr->fsmemblk[1].extension,LogPtr->fsmemblk[1].length,fspages);	
		SC_CacheSync(0);
		LogPtr->magic1 = LOG_MAGIC;
RTLMSG(1,(L"\r\nBooting kernel with clean memory configuration:\r\n"));
	} else {
RTLMSG(1,(L"\r\nBooting kernel with existing memory configuration:\r\n"));
	}
RTLMSG(1,(L"First : %8.8lx start, %8.8lx extension, %8.8lx length\r\n",
	LogPtr->fsmemblk[0].startptr,LogPtr->fsmemblk[0].extension,LogPtr->fsmemblk[0].length));
RTLMSG(1,(L"Second: %8.8lx start, %8.8lx extension, %8.8lx length\r\n",
	LogPtr->fsmemblk[1].startptr,LogPtr->fsmemblk[1].extension,LogPtr->fsmemblk[1].length));
	LogPtr->pKList = 0;
	MemoryInfo.pKData = (LPVOID)pTOC->ulRAMStart;
	MemoryInfo.pKEnd  = (LPVOID)MainMemoryEndAddress;
	if (LogPtr->fsmemblk[1].length) {
		if (LogPtr->fsmemblk[1].startptr < (DWORD)MemoryInfo.pKData)
			MemoryInfo.pKData = (LPVOID)LogPtr->fsmemblk[1].startptr;
		if (LogPtr->fsmemblk[1].startptr + LogPtr->fsmemblk[1].extension + LogPtr->fsmemblk[1].length > (DWORD)MemoryInfo.pKEnd)
			MemoryInfo.pKEnd = (LPVOID)(LogPtr->fsmemblk[1].startptr + LogPtr->fsmemblk[1].extension + LogPtr->fsmemblk[1].length);
	}
	MemoryInfo.cFi = 0;
	MemoryInfo.pFi = &FreeInfo[0];
	if (LogPtr->fsmemblk[1].length) {
		FreeInfo[MemoryInfo.cFi].pUseMap = (LPBYTE)LogPtr->fsmemblk[1].startptr;
		memset((LPVOID)LogPtr->fsmemblk[1].startptr,0,LogPtr->fsmemblk[1].extension);
		FreeInfo[MemoryInfo.cFi].paStart = GetPFN(LogPtr->fsmemblk[1].startptr+LogPtr->fsmemblk[1].extension-PAGE_SIZE)+PFN_INCR;
		FreeInfo[MemoryInfo.cFi].paEnd = FreeInfo[MemoryInfo.cFi].paStart + PFN_INCR * (LogPtr->fsmemblk[1].length/PAGE_SIZE);
		MemoryInfo.cFi++;
	}
	if (LogPtr->fsmemblk[0].length) {
		FreeInfo[MemoryInfo.cFi].pUseMap = (LPBYTE)LogPtr->fsmemblk[0].startptr;
		memset((LPVOID)LogPtr->fsmemblk[0].startptr,0,LogPtr->fsmemblk[0].extension);
		FreeInfo[MemoryInfo.cFi].paStart = GetPFN(LogPtr->fsmemblk[0].startptr+LogPtr->fsmemblk[0].extension-PAGE_SIZE)+PFN_INCR;
		FreeInfo[MemoryInfo.cFi].paEnd = FreeInfo[MemoryInfo.cFi].paStart + PFN_INCR * (LogPtr->fsmemblk[0].length/PAGE_SIZE);
		MemoryInfo.cFi++;
	}
	if (MemoryInfo.cFi != 2)
		memset(&FreeInfo[1],0,sizeof(FreeInfo[1]));
	GrabFSPages();
	KInfoTable[KINX_GWESHEAPINFO] = 0;
	KInfoTable[KINX_TIMEZONEBIAS] = 0;
}

// Relocate a page

BOOL RelocatePage(e32_lite *eptr, o32_lite *optr, ulong BasePtr, ulong BaseAdj, DWORD pMem, DWORD pData, DWORD prevdw) {
	DWORD Comp1, Comp2;
#if defined(MIPS16SUPPORT)
	DWORD PrevPage;
#endif
	struct info *blockptr, *blockstart;
	ulong blocksize;
	LPDWORD FixupAddress;
#ifndef x86
	LPWORD FixupAddressHi;
	BOOL MatchedReflo=FALSE;
#endif
	DWORD FixupValue;
	LPWORD currptr;
	DWORD offset;
	LeaveCriticalSection(&PagerCS);
	if (!(blocksize = eptr->e32_unit[FIX].size)) { // No relocations
		EnterCriticalSection(&PagerCS);
		return TRUE;
	}
	blockstart = blockptr = (struct info *)(ZeroPtr(BasePtr)+BaseAdj+eptr->e32_unit[FIX].rva);
	if (BaseAdj != ProcArray[0].dwVMBase)
		BaseAdj = 0;	// for processes
	DEBUGMSG(ZONE_LOADER1,(TEXT("Relocations: BasePtr = %8.8lx, BaseAdj = %8.8lx, VBase = %8.8lx, pMem = %8.8lx, pData = %8.8lx\r\n"),
		BasePtr,BaseAdj, eptr->e32_vbase, pMem, pData));
	if (!(offset = BasePtr - BaseAdj - eptr->e32_vbase)) {
		EnterCriticalSection(&PagerCS);
		return TRUE;                                                    // no adjustment needed
	}
	DEBUGMSG(ZONE_LOADER1,(TEXT("RelocatePage: Offset is %8.8lx\r\n"),offset));
	if ((ZeroPtr(pMem) >= ZeroPtr(BasePtr+eptr->e32_unit[FIX].rva)) &&
		(ZeroPtr(pMem) < ZeroPtr(BasePtr+eptr->e32_unit[FIX].rva+eptr->e32_unit[FIX].size))) {
		EnterCriticalSection(&PagerCS);
		return TRUE;
	}
	while (((ulong)blockptr < (ulong)blockstart + blocksize) && blockptr->size) {
		currptr = (LPWORD)(((ulong)blockptr)+sizeof(struct info));
		if ((ulong)currptr >= ((ulong)blockptr+blockptr->size)) {
			blockptr = (struct info *)(((ulong)blockptr)+blockptr->size);
			continue;
		}
#if defined(x86) || defined(MIPS16SUPPORT)
#define RELOC_LIMIT 8192
#else
#define RELOC_LIMIT 4096
#endif
		if ((ZeroPtr(BasePtr+blockptr->rva) > ZeroPtr(pMem)) || (ZeroPtr(BasePtr+blockptr->rva)+RELOC_LIMIT <= ZeroPtr(pMem))) {
			blockptr = (struct info *)(((ulong)blockptr)+blockptr->size);
			continue;
		}
		goto fixuppage;
	}
	EnterCriticalSection(&PagerCS);
	return TRUE;
fixuppage:
	DEBUGMSG(ZONE_LOADER1,(L"Fixing up %8.8lx %8.8lx, %8.8lx\r\n",blockptr->rva,optr->o32_rva,optr->o32_realaddr));
	while ((ulong)currptr < ((ulong)blockptr+blockptr->size)) {
#ifdef x86
		Comp1 = ZeroPtr(pMem);
		Comp2 = ZeroPtr(BasePtr + blockptr->rva);
		Comp1 = (Comp2 + 0x1000 == Comp1) ? 1 : 0;
#else
		Comp1 = ZeroPtr(pMem) - ZeroPtr(BasePtr + blockptr->rva);
		Comp2 = (DWORD)(*currptr&0xfff);
#if defined(MIPS16SUPPORT)
		// Comp1 is relative start of page being relocated
		// Comp2 is relative address of fixup
		// For MIPS16 jump, deal with fixups on this page or the preceding page
		// For all other fixups, deal with this page only
		if (*currptr>>12 == IMAGE_REL_BASED_MIPS_JMPADDR16) {
			if ((Comp1 > Comp2 + PAGE_SIZE) || (Comp1 + PAGE_SIZE <= Comp2)) {
				currptr++;
				continue;
			}
			// PrevPage: is fixup located on the page preceding the one being relocated?
			PrevPage = (Comp1 > Comp2);
			// Comp1: is fixup located in the block preceding the one that contains the current page?
			Comp1 = PrevPage && ((Comp1 & 0xfff) == 0);
		} else {
#endif
			if ((Comp1 > Comp2) || (Comp1 + PAGE_SIZE <= Comp2)) {
				if (*currptr>>12 == IMAGE_REL_BASED_HIGHADJ)
					currptr++;
				currptr++;
				continue;
			}
#if defined(MIPS16SUPPORT)
			// Comp1: is fixup located in the block preceding the one that contains the current page? (No.)
			Comp1 = 0;
		}
#endif
#endif
#if defined(x86) || defined(MIPS16SUPPORT)
		FixupAddress = (LPDWORD)(((pData&0xfffff000) + (*currptr&0xfff)) - 4096*Comp1);
#else
		FixupAddress = (LPDWORD)((pData&0xfffff000) + (*currptr&0xfff));
#endif
		DEBUGMSG(ZONE_LOADER2,(TEXT("type %d, low %8.8lx, page %8.8lx, addr %8.8lx, op %8.8lx\r\n"),
			(*currptr>>12)&0xf, (*currptr)&0x0fff,blockptr->rva,FixupAddress, *FixupAddress));
		switch (*currptr>>12) {
			case IMAGE_REL_BASED_ABSOLUTE: // Absolute - no fixup required.
				break;
#ifdef x86
			case IMAGE_REL_BASED_HIGHLOW: // Word - (32-bits) relocate the entire address.
				if (Comp1 && (((DWORD)FixupAddress & 0xfff) > 0xffc)) {
					switch ((DWORD)FixupAddress & 0x3) {
						case 1:
							FixupValue = (prevdw>>8) + (*((LPBYTE)FixupAddress+3) <<24 ) + offset;
							*((LPBYTE)FixupAddress+3) = (BYTE)(FixupValue >> 24);
							break;
						case 2:
							FixupValue = (prevdw>>16) + (*((LPWORD)FixupAddress+1) << 16) + offset;
							*((LPWORD)FixupAddress+1) = (WORD)(FixupValue >> 16);
							break;
						case 3:
							FixupValue = (prevdw>>24) + ((*(LPDWORD)((LPBYTE)FixupAddress+1)) << 8) + offset;
							*(LPWORD)((LPBYTE)FixupAddress+1) = (WORD)(FixupValue >> 8);
							*((LPBYTE)FixupAddress+3) = (BYTE)(FixupValue >> 24);
							break;
					}
				} else if (!Comp1) {
					if (((DWORD)FixupAddress & 0xfff) > 0xffc) {
						switch ((DWORD)FixupAddress & 0x3) {
							case 1:
								FixupValue = *(LPWORD)FixupAddress + (((DWORD)*((LPBYTE)FixupAddress+2))<<16) + offset;
								*(LPWORD)FixupAddress = (WORD)FixupValue;
								*((LPBYTE)FixupAddress+2) = (BYTE)(FixupValue>>16);
								break;
							case 2:
							    *(LPWORD)FixupAddress += (WORD)offset;
								break;
							case 3:
							    *(LPBYTE)FixupAddress += (BYTE)offset;
							    break;
						}
					} else
					    *FixupAddress += offset;
				}
			    break;
#else
			case IMAGE_REL_BASED_HIGH: // Save the address and go to get REF_LO paired with this one
				FixupAddressHi = (LPWORD)FixupAddress;
				MatchedReflo = TRUE;
				break;
			case IMAGE_REL_BASED_LOW: // Low - (16-bit) relocate high part too.
				if (MatchedReflo) {
					FixupValue = (DWORD)(long)((*FixupAddressHi) << 16) +
						*(LPWORD)FixupAddress + offset;
					*FixupAddressHi = (short)((FixupValue + 0x8000) >> 16);
					MatchedReflo = FALSE;
				} else
					FixupValue = *(short *)FixupAddress + offset;
				*(LPWORD)FixupAddress = (WORD)(FixupValue & 0xffff);
			    break;
			case IMAGE_REL_BASED_HIGHLOW: // Word - (32-bits) relocate the entire address.
					if ((DWORD)FixupAddress & 0x3)
					    *(UNALIGNED DWORD *)FixupAddress += (DWORD)offset;
					else
					    *FixupAddress += (DWORD)offset;
			    break;
			case IMAGE_REL_BASED_HIGHADJ: // 32 bit relocation of 16 bit high word, sign extended
				DEBUGMSG(ZONE_LOADER2,(TEXT("Grabbing extra data %8.8lx\r\n"),*(currptr+1)));
				*(LPWORD)FixupAddress += (WORD)((*(short *)(++currptr)+offset+0x8000)>>16);
				break;
			case IMAGE_REL_BASED_MIPS_JMPADDR: // jump to 26 bit target (shifted left 2)
			    FixupValue = ((*FixupAddress)&0x03ffffff) + (offset>>2);
				*FixupAddress = (*FixupAddress & 0xfc000000) | (FixupValue & 0x03ffffff);
				break;
#if defined(MIPS16SUPPORT)
			case IMAGE_REL_BASED_MIPS_JMPADDR16: // MIPS16 jal/jalx to 26 bit target (shifted left 2)
				if (PrevPage && (((DWORD)FixupAddress & (PAGE_SIZE-1)) == PAGE_SIZE-2)) {
					// Relocation is on previous page, crossing into the current one
					// Do this page's portion == last 2 bytes of jal/jalx == least-signif 16 bits of address
					*((LPWORD)FixupAddress+1) += (WORD)(offset >> 2);
					break;
				} else if (!PrevPage) {
					// Relocation is on this page. Put the most-signif bits in FixupValue
					FixupValue = (*(LPWORD)FixupAddress) & 0x03ff;
					FixupValue = ((FixupValue >> 5) | ((FixupValue & 0x1f) << 5)) << 16;
					if (((DWORD)FixupAddress & (PAGE_SIZE-1)) == PAGE_SIZE-2) {
						// We're at the end of the page. prevdw has the 2 bytes we peeked at on the next page,
						// so use them instead of loading them from FixupAddress+1
						FixupValue |= (WORD)prevdw;
						FixupValue += offset >> 2;
					} else {
						// All 32 bits are on this page. Go ahead and fixup last 2 bytes
						FixupValue |= *((LPWORD)FixupAddress+1);
						FixupValue += offset >> 2;
						*((LPWORD)FixupAddress+1) = (WORD)(FixupValue & 0xffff);
					}
					// In either case, we now have the right bits for the upper part of the address
					// Rescramble them and put them in the first 2 bytes of the fixup
					FixupValue = (FixupValue >> 16) & 0x3ff;
					*(LPWORD)FixupAddress = (WORD)((*(LPWORD)FixupAddress & 0xfc00) | (FixupValue >> 5) | ((FixupValue & 0x1f) << 5));
			    }
			    break;
#endif
#endif
			default :
				DEBUGMSG(ZONE_LOADER1,(TEXT("Not doing fixup type %d\r\n"),*currptr>>12));
				DEBUGCHK(0);
				break;
		}
		DEBUGMSG(ZONE_LOADER2,(TEXT("reloc complete, new op %8.8lx\r\n"),*FixupAddress));
		currptr++;
	}
#if defined(x86) || defined(MIPS16SUPPORT)
	if (Comp1) {
		blockptr = (struct info *)(((ulong)blockptr)+blockptr->size);
		if (((ulong)blockptr < (ulong)blockstart + blocksize) && blockptr->size) {
			currptr = (LPWORD)(((ulong)blockptr)+sizeof(struct info));
			if ((ulong)currptr < ((ulong)blockptr+blockptr->size))
				if ((ZeroPtr(BasePtr+blockptr->rva) <= ZeroPtr(pMem)) && (ZeroPtr(BasePtr+blockptr->rva)+4096 > ZeroPtr(pMem)))
					goto fixuppage;
		}
	}
#endif
	EnterCriticalSection(&PagerCS);
	return TRUE;
}

// Relocate a DLL or EXE

BOOL Relocate(e32_lite *eptr, o32_lite *oarry, ulong BasePtr, ulong BaseAdj) {
	o32_lite *dataptr;
	struct info *blockptr, *blockstart;
	ulong blocksize;
	LPDWORD FixupAddress;
	LPWORD FixupAddressHi, currptr;
	WORD curroff;
	DWORD FixupValue, offset;
	BOOL MatchedReflo=FALSE;
	int loop;

	if (!(blocksize = eptr->e32_unit[FIX].size)) // No relocations
		return TRUE;

	DEBUGMSG(ZONE_LOADER1,(TEXT("Relocations: BasePtr = %8.8lx, BaseAdj = %8.8lx, VBase = %8.8lx\r\n"),
		BasePtr,BaseAdj, eptr->e32_vbase));
	if (!(offset = BasePtr - BaseAdj - eptr->e32_vbase))
		return TRUE;                                                    // no adjustment needed
	DEBUGMSG(ZONE_LOADER1,(TEXT("Relocate: Offset is %8.8lx\r\n"),offset));
	blockstart = blockptr = (struct info *)(BasePtr+eptr->e32_unit[FIX].rva);
	while (((ulong)blockptr < (ulong)blockstart + blocksize) && blockptr->size) {
		currptr = (LPWORD)(((ulong)blockptr)+sizeof(struct info));
		if ((ulong)currptr >= ((ulong)blockptr+blockptr->size)) {
			blockptr = (struct info *)(((ulong)blockptr)+blockptr->size);
			continue;
		}
		dataptr = 0;
		while ((ulong)currptr < ((ulong)blockptr+blockptr->size)) {
			curroff = *currptr&0xfff;
			if (!curroff && !blockptr->rva) {
				currptr++;
				continue;
			}
			if (!dataptr || (dataptr->o32_rva > blockptr->rva + curroff) ||
					(dataptr->o32_rva + dataptr->o32_vsize <= blockptr->rva + curroff)) {
				for (loop = 0; loop < eptr->e32_objcnt; loop++) {
					dataptr = &oarry[loop];
					if ((dataptr->o32_rva <= blockptr->rva + curroff) &&
						(dataptr->o32_rva+dataptr->o32_vsize > blockptr->rva + curroff))
						break;
				}
			}
			DEBUGCHK(loop != eptr->e32_objcnt);
			FixupAddress = (LPDWORD)(blockptr->rva - dataptr->o32_rva + curroff + dataptr->o32_realaddr);
			DEBUGMSG(ZONE_LOADER2,(TEXT("type %d, low %8.8lx, page %8.8lx, addr %8.8lx, op %8.8lx\r\n"),
				(*currptr>>12)&0xf, (*currptr)&0x0fff,blockptr->rva,FixupAddress,*FixupAddress));
			switch (*currptr>>12) {
				case IMAGE_REL_BASED_ABSOLUTE: // Absolute - no fixup required.
					break;
				case IMAGE_REL_BASED_HIGH: // Save the address and go to get REF_LO paired with this one
					FixupAddressHi = (LPWORD)FixupAddress;
					MatchedReflo = TRUE;
					break;
				case IMAGE_REL_BASED_LOW: // Low - (16-bit) relocate high part too.
					if (MatchedReflo) {
						FixupValue = (DWORD)(long)((*FixupAddressHi) << 16) +
							*(LPWORD)FixupAddress + offset;
						*FixupAddressHi = (short)((FixupValue + 0x8000) >> 16);
						MatchedReflo = FALSE;
					} else
						FixupValue = *(short *)FixupAddress + offset;
					*(LPWORD)FixupAddress = (WORD)(FixupValue & 0xffff);
				    break;
				case IMAGE_REL_BASED_HIGHLOW: // Word - (32-bits) relocate the entire address.
					if ((DWORD)FixupAddress & 0x3)
					    *(UNALIGNED DWORD *)FixupAddress += (DWORD)offset;
					else
					    *FixupAddress += (DWORD)offset;
				    break;
				case IMAGE_REL_BASED_HIGHADJ: // 32 bit relocation of 16 bit high word, sign extended
					DEBUGMSG(ZONE_LOADER2,(TEXT("Grabbing extra data %8.8lx\r\n"),*(currptr+1)));
					*(LPWORD)FixupAddress += (WORD)((*(short *)(++currptr)+offset+0x8000)>>16);
					break;
				case IMAGE_REL_BASED_MIPS_JMPADDR: // jump to 26 bit target (shifted left 2)
				    FixupValue = ((*FixupAddress)&0x03ffffff) + (offset>>2);
					*FixupAddress = (*FixupAddress & 0xfc000000) | (FixupValue & 0x03ffffff);
					break;
#if defined(MIPS16SUPPORT)
                case IMAGE_REL_BASED_MIPS_JMPADDR16: // MIPS16 jal/jalx to 26 bit target (shifted left 2)
                    FixupValue = (*(LPWORD)FixupAddress) & 0x03ff;
                    FixupValue = (((FixupValue >> 5) | ((FixupValue & 0x1f) << 5)) << 16) | *((LPWORD)FixupAddress+1);
                    FixupValue += offset >> 2;
                    *((LPWORD)FixupAddress+1) = (WORD)(FixupValue & 0xffff);
                    FixupValue = (FixupValue >> 16) & 0x3ff;
                    *(LPWORD)FixupAddress = (WORD) ((*(LPWORD)FixupAddress & 0x1c00) | (FixupValue >> 5) | ((FixupValue & 0x1f) << 5));
                    break;
#endif
				default :
					DEBUGMSG(ZONE_LOADER1,(TEXT("Not doing fixup type %d\r\n"),*currptr>>12));
					DEBUGCHK(0);
					break;
			}
			DEBUGMSG(ZONE_LOADER2,(TEXT("reloc complete, new op %8.8lx\r\n"),*FixupAddress));
			currptr++;
		}
		blockptr = (struct info *)(((ulong)blockptr)+blockptr->size);
	}
	return TRUE;
}

DWORD ResolveImpOrd(PMODULE pMod, DWORD ord);
DWORD ResolveImpStr(PMODULE pMod, LPCSTR str);
DWORD ResolveImpHintStr(PMODULE pMod, DWORD hint, LPCHAR str);

#define MAX_AFE_FILESIZE 32

DWORD Katoi(LPCHAR str) {
	DWORD retval = 0;
	while (*str) {
		retval = retval * 10 + (*str-'0');
		str++;
	}
	return retval;
}

// Get address from export entry

DWORD AddrFromEat(PMODULE pMod, DWORD eat) {
	WCHAR filename[MAX_AFE_FILESIZE];
	PMODULE pMod2;
	LPCHAR str;
	int loop;
	if ((eat < pMod->e32.e32_unit[EXP].rva) ||
		(eat >= pMod->e32.e32_unit[EXP].rva + pMod->e32.e32_unit[EXP].size))
		return eat+ (pMod->DbgFlags & DBG_IS_DEBUGGER ? (DWORD)pMod->BasePtr : ZeroPtr(pMod->BasePtr));
	else {
		str = (LPCHAR)(eat + (ulong)pMod->BasePtr);
		for (loop = 0; loop < MAX_AFE_FILESIZE-1; loop++)
			if ((filename[loop] = (WCHAR)*str++) == (WCHAR)'.')
				break;
		filename[loop] = 0;
		if (!(pMod2 = LoadOneLibraryPart2(filename,1,0)))
			return 0;
		if (*str == '#')
			return ResolveImpOrd(pMod2,Katoi(str));
		else
			return ResolveImpStr(pMod2,str);
	}
}

// Get address from ordinal

DWORD ResolveImpOrd(PMODULE pMod, DWORD ord) {
	struct ExpHdr *expptr;
	LPDWORD eatptr;
	DWORD hint;
	DWORD retval;
	if (!pMod->e32.e32_unit[EXP].rva)
		return 0;
	expptr = (struct ExpHdr *)((ulong)pMod->BasePtr+pMod->e32.e32_unit[EXP].rva);
	eatptr = (LPDWORD)(expptr->exp_eat + (ulong)pMod->BasePtr);
	hint = ord - expptr->exp_ordbase;
	retval = (hint >= expptr->exp_eatcnt ? 0 : AddrFromEat(pMod,eatptr[hint]));
//	ERRORMSG(!retval,(TEXT("Can't find ordinal %d in module %s\r\n"),ord,pMod->lpszModName));
	return retval;
}

// Get address from string

DWORD ResolveImpStr(PMODULE pMod, LPCSTR str) {
	struct ExpHdr *expptr;
	LPCHAR *nameptr;
	LPDWORD eatptr;
	LPWORD ordptr;
	DWORD retval;
	ulong loop;
	if (!pMod->e32.e32_unit[EXP].rva)
		return 0;
	expptr = (struct ExpHdr *)((ulong)pMod->BasePtr+pMod->e32.e32_unit[EXP].rva);
	nameptr = (LPCHAR *)(expptr->exp_name + (ulong)pMod->BasePtr);
	eatptr = (LPDWORD)(expptr->exp_eat + (ulong)pMod->BasePtr);
	ordptr = (LPWORD)(expptr->exp_ordinal + (ulong)pMod->BasePtr);
	for (loop = 0; loop < expptr->exp_namecnt; loop++)
		if (!strcmp(str,nameptr[loop]+(ulong)pMod->BasePtr))
			break;
	if (loop == expptr->exp_namecnt)
		retval = 0;
	else
		retval = (loop >= expptr->exp_eatcnt ? 0 : AddrFromEat(pMod,eatptr[ordptr[loop]]));
//	ERRORMSG(!retval,(TEXT("Can't find import %a in module %s\r\n"),str,pMod->lpszModName));
	return retval;
}

// Get address from hint and string

DWORD ResolveImpHintStr(PMODULE pMod, DWORD hint, LPCHAR str) {
	struct ExpHdr *expptr;
	LPCHAR *nameptr;
	LPDWORD eatptr;
	LPWORD ordptr;
	DWORD retval;
	if (!pMod->e32.e32_unit[EXP].rva)
		return 0;
	expptr = (struct ExpHdr *)((ulong)pMod->BasePtr+pMod->e32.e32_unit[EXP].rva);
	nameptr = (LPCHAR *)(expptr->exp_name + (ulong)pMod->BasePtr);
	eatptr = (LPDWORD)(expptr->exp_eat + (ulong)pMod->BasePtr);
	ordptr = (LPWORD)(expptr->exp_ordinal + (ulong)pMod->BasePtr);
	if ((hint >= expptr->exp_namecnt) || (strcmp(str,nameptr[hint] + (ulong)pMod->BasePtr)))
		retval = ResolveImpStr(pMod,str);
	else
		retval = AddrFromEat(pMod,eatptr[ordptr[hint]]);
//	ERRORMSG(!retval,(TEXT("Can't find import %a (hint %d) in %s\r\n"),str,hint,pMod->lpszModName));
	return retval;
}

// Increment process reference count to module, return old count

WORD IncRefCount(PMODULE pMod) {
	if (!(pMod->inuse & (1<<pCurProc->procnum))) {
		pMod->inuse |= (1<<pCurProc->procnum);
		pMod->calledfunc &= ~(1<<pCurProc->procnum);
	}
	return pMod->refcnt[pCurProc->procnum]++;
}

// Decrement process reference count to module, return new count

WORD DecRefCount(PMODULE pMod) {
	if (!(--pMod->refcnt[pCurProc->procnum]))
		pMod->inuse &= ~(1<<pCurProc->procnum);
	return pMod->refcnt[pCurProc->procnum];
}

typedef BOOL (*entry_t)(HANDLE,DWORD,LPVOID);
typedef BOOL (*comentry_t)(HANDLE,DWORD,LPVOID,LPVOID,DWORD,DWORD);

// Remove module from linked list

void UnlinkModule(PMODULE pMod) {
	PMODULE ptr1, ptr2;
	EnterCriticalSection(&ModListcs);
	if (pModList == pMod)
		pModList = pMod->pMod;
	else if (pModList) {
		ptr1 = pModList;
		ptr2 = pModList->pMod;
		while (ptr2 && (ptr2 != pMod)) {
			ptr1 = ptr2;
			ptr2 = ptr2->pMod;
		}
		if (ptr2)
			ptr1->pMod = ptr2->pMod;
	}
	LeaveCriticalSection(&ModListcs);
}

// Unmap module from process

void UnCopyRegions(PMODULE pMod) {
	long basediff = pCurProc->dwVMBase - ProcArray[0].dwVMBase;
	VirtualFree((LPVOID)((long)pMod->BasePtr+basediff),pMod->e32.e32_vsize,MEM_DECOMMIT|0x80000000);
	VirtualFree((LPVOID)((long)pMod->BasePtr+basediff),0,MEM_RELEASE);
}

void FreeModuleMemory(PMODULE pMod) {
	UnlinkModule(pMod);
	VirtualFree(pMod->BasePtr,pMod->e32.e32_vsize,MEM_DECOMMIT|0x80000000);
	if (ZeroPtr(pMod->BasePtr) < pTOC->dllfirst)
		VirtualFree(pMod->BasePtr,0,MEM_RELEASE);
	if ((ZeroPtr(pMod->BasePtr) > ZeroPtr(pMod->lpszModName)) ||
		(ZeroPtr(pMod->BasePtr) + pMod->e32.e32_vsize <= ZeroPtr(pMod->lpszModName)))
		VirtualFree(pMod->lpszModName,0,MEM_RELEASE);
	if ((ZeroPtr(pMod->BasePtr) > ZeroPtr(pMod->o32_ptr)) ||
		(ZeroPtr(pMod->BasePtr) + pMod->e32.e32_vsize <= ZeroPtr(pMod->o32_ptr)))
		VirtualFree(pMod->o32_ptr,0,MEM_RELEASE);
	CloseExe(&pMod->oe);
	FreeMem(pMod,HEAP_MODULE);
}

void FreeLibraryByName(LPCHAR lpszName);

// Free library from proc (by name), zeroing reference count

VOID FreeLibraryFromProc(PMODULE pMod, PPROCESS pproc) {
	if (pMod->refcnt[pproc->procnum]) {
		pMod->refcnt[pproc->procnum] = 0;
		pMod->inuse &= ~(1 << pproc->procnum);
		if (SystemAPISets[SH_PATCHER])
			FreeDllPatch(pproc,pMod);
		if (!pMod->inuse) {
			KDUpdateSymbols(((DWORD)pMod->BasePtr)+1, TRUE);
			CELOG_ModuleFree(pCurProc->hProc, (HANDLE)pMod, TRUE);
			FreeModuleMemory(pMod);
		}	
	}
}

// Free all libraries from proc

VOID FreeAllProcLibraries(PPROCESS pProc) {
	PMODULE pMod, pNext;
	EnterCriticalSection(&LLcs);
	pMod = pModList;
	while (pMod) {
		pNext = pMod->pMod;
		FreeLibraryFromProc(pMod,pProc);
		pMod = pNext;
	}
	LeaveCriticalSection(&LLcs);
}

// Pass Reason/Reserved to DLL entry point for pMod

BOOL CallDLLEntry(PMODULE pMod, DWORD Reason, LPVOID Reserved) {
	BOOL retval = TRUE;
	DWORD LastError = KGetLastError(pCurThread);
	DWORD dwOldMode;
	if (pMod->startip && !(pMod->wFlags & DONT_RESOLVE_DLL_REFERENCES)) {
		if ((dwOldMode = GET_TIMEMODE(pCurThread)) == TIMEMODE_KERNEL)
			GoToUserTime();
		if (Reason == DLL_PROCESS_ATTACH) {
			if (pMod->calledfunc & (1<<pCurProc->procnum))
				goto DontCall;
			pMod->calledfunc |= (1<<pCurProc->procnum);
		} else if (Reason == DLL_PROCESS_DETACH) {
			if (!(pMod->calledfunc & (1<<pCurProc->procnum)))
				goto DontCall;
			pMod->calledfunc &= ~(1<<pCurProc->procnum);
		}
		__try {
			if (pMod->e32.e32_sect14rva)
				retval = ((comentry_t)pMod->startip)((HANDLE)pMod,Reason,Reserved,
					(LPVOID)ZeroPtr(pMod->BasePtr),pMod->e32.e32_sect14rva,pMod->e32.e32_sect14size);
			else
				retval = ((entry_t)pMod->startip)((HANDLE)pMod,Reason,Reserved);
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			retval = FALSE;
		}
DontCall:		
		if (dwOldMode == TIMEMODE_KERNEL)
			GoToUserTime();
	}
	KSetLastError(pCurThread,LastError);
	return retval;
}

BOOL UnDoDepends(PMODULE pMod);

// Decrement ref count on pMod (from hCurProc), freeing if needed

BOOL FreeOneLibraryPart2(PMODULE pMod, BOOL bCallEntry) {
	struct ImpHdr *blockptr;
	CELOG_ModuleFree(pCurProc->hProc, (HANDLE)pMod, FALSE);
	if (HasBreadcrumb(pMod))
		return TRUE;
	SetBreadcrumb(pMod);
	if (HasModRefProcPtr(pMod,pCurProc)) {
		if (!DecRefCount(pMod)) {
			if (bCallEntry)
				CallDLLEntry(pMod,DLL_PROCESS_DETACH,0);
			if (pMod->e32.e32_sect14size)
				FreeLibraryByName("mscoree.dll");
			pMod->dwNoNotify &= ~(1 << pCurProc->procnum);
			if(pMod->pmodResource) {
				FreeOneLibraryPart2(pMod, 0); // DONT call dllentry of RES dll
				pMod->pmodResource = 0;
			}
			if (!(pMod->wFlags & DONT_RESOLVE_DLL_REFERENCES))
				UnDoDepends(pMod);
			UnCopyRegions(pMod);
        	if (SystemAPISets[SH_PATCHER])
        	    FreeDllPatch(pCurProc, pMod);
			if (pCurThread->pThrdDbg && ProcStarted(pCurProc) && pCurThread->pThrdDbg->hEvent) {
				pCurThread->pThrdDbg->dbginfo.dwProcessId = (DWORD)hCurProc;
				pCurThread->pThrdDbg->dbginfo.dwThreadId = (DWORD)hCurThread;
				pCurThread->pThrdDbg->dbginfo.dwDebugEventCode = UNLOAD_DLL_DEBUG_EVENT;
				pCurThread->pThrdDbg->dbginfo.u.UnloadDll.lpBaseOfDll = (LPVOID)ZeroPtr(pMod->BasePtr);
				SetEvent(pCurThread->pThrdDbg->hEvent);
				SC_WaitForMultiple(1,&pCurThread->pThrdDbg->hBlockEvent,FALSE,INFINITE);
			}

		} else {
			if (!(pMod->wFlags & DONT_RESOLVE_DLL_REFERENCES))
				UnDoDepends(pMod);
		}
		if (!pMod->inuse) {
			CELOG_ModuleFree(pCurProc->hProc, (HANDLE)pMod, TRUE);
			KDUpdateSymbols(((DWORD)pMod->BasePtr)+1, TRUE);
			if (pMod->e32.e32_unit[IMP].rva) {
				blockptr = (struct ImpHdr *)((long)pMod->BasePtr+pMod->e32.e32_unit[IMP].rva);
				while (blockptr->imp_lookup) {
					FreeLibraryByName((LPCHAR)pMod->BasePtr+blockptr->imp_dllname);
					blockptr++;
				}
			}
			





			FreeModuleMemory(pMod);
		}
		return TRUE;
	} else
		return FALSE;
}

BOOL FreeOneLibrary(PMODULE pMod, BOOL bCallEntry) {
	BOOL retval = FALSE;
	if (!InitBreadcrumbs())
		KSetLastError(pCurThread,ERROR_NOT_ENOUGH_MEMORY);
	else {
		retval = FreeOneLibraryPart2(pMod, bCallEntry);
		FinishBreadcrumbs();
	}
	return retval;
}

// Decrement ref count on library, freeing if needed

void FreeLibraryByName(LPCHAR lpszName) {
	PMODULE pMod;
	LPWSTR pTrav1;
	LPBYTE pTrav2;
	for (pMod = pModList; pMod; pMod = pMod->pMod) {
		for (pTrav1 = pMod->lpszModName, pTrav2 = lpszName; *pTrav1 && (*pTrav1 == (WCHAR)*pTrav2); pTrav1++, pTrav2++)
			;
		if (*pTrav1 == (WCHAR)*pTrav2) {
			FreeOneLibraryPart2(pMod, 1);
			return;
		}
	}
}

void KAsciiToUnicode(LPWSTR wchptr, LPBYTE chptr, int maxlen) {
	while ((maxlen-- > 1) && *chptr) {
		*wchptr++ = (WCHAR)*chptr++;
	}
	*wchptr = 0;
}

void KUnicodeToAscii(LPCHAR chptr, LPCWSTR wchptr, int maxlen) {
	while ((maxlen-- > 1) && *wchptr) {
		*chptr++ = (CHAR)*wchptr++;
	}
	*chptr = 0;
}

// Do imports from an exe/dll

#define MAX_DLLNAME_LEN 32

DWORD DoImports(e32_lite *eptr, o32_lite *oarry, ulong BaseAddr, ulong BaseAdj) {
	ulong blocksize;
	LPDWORD ltptr, atptr;
	PMODULE pMod;
	DWORD retval, loop;
	WCHAR ucptr[MAX_DLLNAME_LEN];
	struct ImpHdr *blockptr, *blockstart;
	struct ImpProc *impptr;
	if (!(blocksize = eptr->e32_unit[IMP].size)) // No relocations
		return 0;
	blockstart = blockptr = (struct ImpHdr *)(BaseAddr+eptr->e32_unit[IMP].rva);
	while (blockptr->imp_lookup) {
		KAsciiToUnicode(ucptr,(LPCHAR)BaseAddr+blockptr->imp_dllname,MAX_DLLNAME_LEN);
		pMod = LoadOneLibraryPart2(ucptr,1,0);
		if (!pMod) {
			if (!(retval = GetLastError()))
				retval = ERROR_OUTOFMEMORY;
			while (blockstart != blockptr) {
				FreeLibraryByName((LPBYTE)BaseAddr+blockstart->imp_dllname);
				blockstart++;
			}
			return retval;
		}
		DEBUGMSG(ZONE_LOADER1,(TEXT("Doing imports from module %a (pMod %8.8lx)\r\n"),
			BaseAddr+blockptr->imp_dllname,pMod));
		ltptr = (LPDWORD)(BaseAddr+blockptr->imp_lookup);
		atptr = (LPDWORD)(BaseAddr+blockptr->imp_address);
		while (*ltptr) {
			if (*ltptr & 0x80000000)
				retval = ResolveImpOrd(pMod,*ltptr&0x7fffffff);
			else {
				impptr = (struct ImpProc *)((*ltptr&0x7fffffff)+BaseAddr);	
				retval = ResolveImpHintStr(pMod,impptr->ip_hint,(LPBYTE)impptr->ip_name);
			}
			if (*atptr != retval) {
				__try {
					*atptr = retval;
				} __except (EXCEPTION_EXECUTE_HANDLER) {
					// this is the case for the -optidata linker flag, where the idata is in the .rdata
					// section, and thus read-only
					o32_lite *dataptr;
					VirtualFree(atptr,4,MEM_DECOMMIT);
					for (loop = 0; loop < eptr->e32_objcnt; loop++) {
						dataptr = &oarry[loop];
						if ((dataptr->o32_realaddr <= (DWORD)atptr) && (dataptr->o32_realaddr+dataptr->o32_vsize > (DWORD)atptr)) {
							dataptr->o32_access = PAGE_EXECUTE_READWRITE;
							break;
						}
					}
					if (loop == eptr->e32_objcnt)
						return ERROR_BAD_EXE_FORMAT;
					*atptr = retval;
				}
			} else
				DEBUGCHK(*atptr == retval);
			ltptr++;
			atptr++;
		}
		blockptr++;
	}
	return 0;
}

// Adjust permissions on regions, decommitting discardable ones

BOOL AdjustRegions(e32_lite *eptr, o32_lite *oarry) {
	int loop;
	DWORD oldprot;
	for (loop = 0; loop < eptr->e32_objcnt; loop++) {
		if (oarry->o32_flags & IMAGE_SCN_MEM_DISCARDABLE) {
			VirtualFree((LPVOID)oarry->o32_realaddr,oarry->o32_vsize,MEM_DECOMMIT);
			oarry->o32_realaddr = 0;
		} else
			VirtualProtect((LPVOID)oarry->o32_realaddr,oarry->o32_vsize,oarry->o32_access,&oldprot);
		oarry++;
	}
	return TRUE;
}

BOOL GetNameFromOE(CEOIDINFO *pceoi, openexe_t *oeptr) {
	LPWSTR pstr;
	LPBYTE pbyte;
	BOOL retval = 1;
	__try {
		if (oeptr->filetype == FT_ROMIMAGE) {
			memcpy(pceoi->infFile.szFileName,L"\\Windows\\",9*sizeof(WCHAR));
			pstr = &pceoi->infFile.szFileName[9];
			pbyte = oeptr->tocptr->lpszFileName;
			while (*pbyte)
				*pstr++ = (WCHAR)*pbyte++;
			*pstr = 0;
		} else if (oeptr->bIsOID) {
			CEGUID sysguid;
			CREATE_SYSTEMGUID(&sysguid);						
			if (!(CeOidGetInfoEx(&sysguid,oeptr->ceOid,pceoi)) || (pceoi->wObjType != OBJTYPE_FILE))
				retval = 0;
		} else
			kstrcpyW(pceoi->infFile.szFileName,oeptr->lpName->name);
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		retval = 0;
	}
	return retval;
}

BOOL ReopenExe(openexe_t *oeptr, LPCWSTR pExeName, BOOL bAllowPaging) {
	DWORD bytesread;
	BY_HANDLE_FILE_INFORMATION bhfi;
	if (!(oeptr->lpName = AllocName(MAX_PATH)))
		return 0;
	if ((oeptr->hf = CreateFileW(pExeName,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,0,0)) == INVALID_HANDLE_VALUE) {
		FreeName(oeptr->lpName);
		return 0;
	}
	oeptr->bIsOID = 1;
	oeptr->filetype = FT_OBJSTORE;
	if (GetFileInformationByHandle(oeptr->hf,&bhfi) && (bhfi.dwOID != (ULONG)INVALID_HANDLE_VALUE)) {
		FreeName(oeptr->lpName);
		oeptr->ceOid = bhfi.dwOID;
	} else {
		LPName pName;
		oeptr->bIsOID = 0;
		if (pName = AllocName((strlenW(pExeName)+1)*2)) {
			FreeName(oeptr->lpName);
			oeptr->lpName = pName;
		}
		kstrcpyW(oeptr->lpName->name,pExeName);
	}
	oeptr->pagemode = (bAllowPaging && !(pTOC->ulKernelFlags & KFLAG_DISALLOW_PAGING) &&
		ReadFileWithSeek(oeptr->hf,0,0,0,0,0,0)) ? PM_FULLPAGING : PM_NOPAGING;
	if ((SetFilePointer(oeptr->hf,0x3c,0,FILE_BEGIN) != 0x3c) ||
		!ReadFile(oeptr->hf,(LPBYTE)&oeptr->offset,4,&bytesread,0) || (bytesread != 4)) {
		CloseExe(oeptr);
		return 0;
	}
	return 1;
}

openexe_t globoe;
BOOL globpageallowed;

HANDLE JitOpenFile(LPCWSTR pName) {
	CALLBACKINFO cbi;
	cbi.hProc = ProcArray[0].hProc;
	cbi.pfn = (FARPROC)ReopenExe;
	cbi.pvArg0 = (LPVOID)&globoe;
	if (!PerformCallBack4Int(&cbi,pName,globpageallowed))
		return INVALID_HANDLE_VALUE;
	if (globoe.filetype != FT_OBJSTORE) {
		CloseExe(&globoe);
		return INVALID_HANDLE_VALUE;
	}
	return globoe.hf;
}

void JitCloseFile(HANDLE hFile) {
	DEBUGCHK(globoe.hf == hFile);
	CloseExe(&globoe);
}

// Load E32 header

DWORD ModuleJit(LPCWSTR, LPWSTR, HANDLE *);
// defined in kmisc.c
extern BOOL fJitIsPresent;

DWORD LoadE32(openexe_t *oeptr, e32_lite *eptr, DWORD *lpflags, DWORD *lpEntry, BOOL bIgnoreCPU, BOOL bAllowPaging, LPBYTE pbTrustLevel) {
	e32_exe e32;
	e32_exe *e32ptr;
	e32_rom *e32rptr;
	int bytesread;
	CEOIDINFO ceoi;
	CALLBACKINFO cbi;
top:
	if (oeptr->filetype == FT_PPFSFILE) {
		DEBUGMSG(ZONE_LOADER1,(TEXT("PEBase at %8.8lx\r\n"),oeptr->offset));
		if (rlseek(oeptr->hppfs,oeptr->offset,0) != (int)oeptr->offset)
			return ERROR_BAD_EXE_FORMAT;
		if (rread(oeptr->hppfs,(LPBYTE)&e32,sizeof(e32_exe)) != sizeof(e32_exe))
			return ERROR_BAD_EXE_FORMAT;
		e32ptr = &e32;
	} else if (oeptr->filetype == FT_OBJSTORE) {
		DEBUGMSG(ZONE_LOADER1,(TEXT("PEBase at %8.8lx\r\n"),oeptr->offset));
		if (SetFilePointer(oeptr->hf,oeptr->offset,0,FILE_BEGIN) != oeptr->offset)
			return ERROR_BAD_EXE_FORMAT;
		if (!ReadFile(oeptr->hf,(LPBYTE)&e32,sizeof(e32_exe),&bytesread,0) || (bytesread != sizeof(e32_exe)))
			return ERROR_BAD_EXE_FORMAT;
		e32ptr = &e32;
	} else {
		e32rptr = (struct e32_rom *)(oeptr->tocptr->ulE32Offset);
		eptr->e32_objcnt     = e32rptr->e32_objcnt;
		if (lpflags)
			*lpflags		 = e32rptr->e32_imageflags;
		if (lpEntry)
			*lpEntry		 = e32rptr->e32_entryrva;
		eptr->e32_vbase      = e32rptr->e32_vbase;
		eptr->e32_vsize      = e32rptr->e32_vsize;
		if (e32rptr->e32_subsys == IMAGE_SUBSYSTEM_WINDOWS_CE_GUI) {
			eptr->e32_cevermajor = (BYTE)e32rptr->e32_subsysmajor;
			eptr->e32_ceverminor = (BYTE)e32rptr->e32_subsysminor;
		} else {
			eptr->e32_cevermajor = 1;
			eptr->e32_ceverminor = 0;
		}
		eptr->e32_stackmax = e32rptr->e32_stackmax;
		memcpy(&eptr->e32_unit[0],&e32rptr->e32_unit[0],
			sizeof(struct info)*LITE_EXTRA);
		eptr->e32_sect14rva = e32rptr->e32_sect14rva;
		eptr->e32_sect14size = e32rptr->e32_sect14size;
		return 0;
	}
	if (*(LPDWORD)e32ptr->e32_magic != 0x4550)
		return ERROR_BAD_EXE_FORMAT;

	if (!bIgnoreCPU && fJitIsPresent && e32ptr->e32_unit[14].rva) {
		if (GetNameFromOE(&ceoi,oeptr)) {
			DWORD dwRet;
			CloseExe(oeptr);
			globpageallowed = bAllowPaging;
			dwRet = ModuleJit(ceoi.infFile.szFileName,0,&oeptr->hf);
			switch (dwRet) {
                /*
                    We use -1 in the case where ModuleJit is a stub or
                    when the source file is not a CEF file. We want to
                    behave as if the translator was never invoked
                    since URT files will get dealt with in CreateNewProc,
                    long after the CEF translators are done.
                */
                case -1:
                    break;
				case OEM_CERTIFY_FALSE:
					return NTE_BAD_SIGNATURE;
				case OEM_CERTIFY_TRUST:
				case OEM_CERTIFY_RUN:
					if (pbTrustLevel)
						*pbTrustLevel = (BYTE)dwRet;
					memcpy(oeptr,&globoe,sizeof(openexe_t));
					goto top;
				default:
					cbi.hProc = ProcArray[0].hProc;
					cbi.pfn = (FARPROC)ReopenExe;
					cbi.pvArg0 = (LPVOID)oeptr;
					if (!PerformCallBack4Int(&cbi,ceoi.infFile.szFileName,bAllowPaging))
						return ERROR_BAD_EXE_FORMAT;
			}
		}
	}

	if (!bIgnoreCPU && !e32ptr->e32_unit[14].rva && (e32ptr->e32_cpu != THISCPUID)) {
#if defined(MIPS16SUPPORT)
		if (e32ptr->e32_cpu != IMAGE_FILE_MACHINE_MIPS16)
			return ERROR_BAD_EXE_FORMAT;
#elif defined(THUMBSUPPORT)
		if (e32ptr->e32_cpu != IMAGE_FILE_MACHINE_THUMB)
			return ERROR_BAD_EXE_FORMAT;
#else
		return ERROR_BAD_EXE_FORMAT;
#endif
	}
	eptr->e32_objcnt     = e32ptr->e32_objcnt;
	if (lpflags)
		*lpflags		 = e32ptr->e32_imageflags;
	if (lpEntry)
		*lpEntry		 = e32ptr->e32_entryrva;
	eptr->e32_vbase      = e32ptr->e32_vbase;
	eptr->e32_vsize      = e32ptr->e32_vsize;
	if (e32ptr->e32_subsys == IMAGE_SUBSYSTEM_WINDOWS_CE_GUI) {
		eptr->e32_cevermajor = (BYTE)e32ptr->e32_subsysmajor;
		eptr->e32_ceverminor = (BYTE)e32ptr->e32_subsysminor;
	} else {
		eptr->e32_cevermajor = 1;
		eptr->e32_ceverminor = 0;
	}
	if ((eptr->e32_cevermajor > 2) || ((eptr->e32_cevermajor == 2) && (eptr->e32_ceverminor >= 2)))
		eptr->e32_stackmax = e32ptr->e32_stackmax;
	else
		eptr->e32_stackmax = 64*1024; // backward compatibility
	if ((eptr->e32_cevermajor > CE_VERSION_MAJOR) ||
		((eptr->e32_cevermajor == CE_VERSION_MAJOR) && (eptr->e32_ceverminor > CE_VERSION_MINOR))) {
		return ERROR_OLD_WIN_VERSION;
	}
	eptr->e32_sect14rva = e32ptr->e32_unit[14].rva;
	eptr->e32_sect14size = e32ptr->e32_unit[14].size;
	memcpy(&eptr->e32_unit[0],&e32ptr->e32_unit[0],
		sizeof(struct info)*LITE_EXTRA);
	return 0;
}

BOOL PageInPage(openexe_t *oeptr, LPVOID pMem2, DWORD offset, o32_lite *o32ptr, DWORD size) {
	DWORD bytesread;
	BOOL retval = TRUE;
	DEBUGCHK(PagerCS.OwnerThread == hCurThread);
	LeaveCriticalSection(&PagerCS);
	if (oeptr->filetype == FT_PPFSFILE) {
		if ((o32ptr->o32_psize > offset) &&
			(rreadseek(oeptr->hppfs,pMem2,min(o32ptr->o32_psize - offset,size),o32ptr->o32_dataptr + offset) != (int)min(o32ptr->o32_psize - offset, size)))
			retval = FALSE;
	} else if (oeptr->filetype == FT_OBJSTORE) {
		if ((o32ptr->o32_psize > offset) &&
			 (!ReadFileWithSeek(oeptr->hf,pMem2,min(o32ptr->o32_psize - offset,size),&bytesread,0,o32ptr->o32_dataptr + offset,0) ||
			  (bytesread != min(o32ptr->o32_psize - offset, size))))
			retval = FALSE;
	} else {
		retval = FALSE;
		DEBUGCHK(0);
	}
	EnterCriticalSection(&PagerCS);
	return retval;
}

int PageInProcess(PPROCESS pProc, DWORD addr) {
	e32_lite *eptr = &pProc->e32;
	o32_lite *oarry = pProc->o32_ptr;
	o32_lite *optr;
	o32_rom *o32rp;
	openexe_t *oeptr = &pProc->oe;
	int loop;
	LPVOID pMem, pMem2;
	DWORD low, high, offset, zaddr, prevdw;
	BOOL retval = 2;
	MEMORY_BASIC_INFORMATION mbi;
	addr = addr & ~(PAGE_SIZE-1);
	zaddr = addr;
	for (loop = 0; loop < eptr->e32_objcnt; loop++) {
		optr = &oarry[loop];
		low = optr->o32_realaddr;
		high = ((low + optr->o32_vsize) + (PAGE_SIZE-1))&~(PAGE_SIZE-1);
		if ((zaddr >= low) && (zaddr < high)) {
			offset = zaddr - low;
			if ((oeptr->filetype == FT_PPFSFILE) || (oeptr->filetype == FT_OBJSTORE)) {
				if (!(pMem = VirtualAlloc((LPVOID)ProcArray[0].dwVMBase,0x10000,MEM_RESERVE,PAGE_NOACCESS))) {
					DEBUGMSG(ZONE_PAGING,(L"    PI: VA1 Failed!\r\n"));
					break;
				}
				pMem2 = ((LPVOID)((DWORD)pMem + ((low+offset)&0xffff)));
				if (!VirtualAlloc(pMem2,PAGE_SIZE,MEM_COMMIT,PAGE_EXECUTE_READWRITE)) {
					DEBUGMSG(ZONE_PAGING,(L"    PI: VA Failed!\r\n"));
					VirtualFree(pMem,0,MEM_RELEASE);
					break;
				}
#ifdef x86
				if (offset < 4)
					prevdw = 0;
				else if (!PageInPage(oeptr,&prevdw,offset-4,optr,4)) {
					DEBUGMSG(ZONE_PAGING,(L"    PI: PIP Failed 2!\r\n"));
					VirtualFree(pMem2,PAGE_SIZE,MEM_DECOMMIT);
					VirtualFree(pMem,0,MEM_RELEASE);
					retval = 3;
					break;
				}
#elif defined(MIPS16SUPPORT)
				// If we're not on the last page, peek at the first 2 bytes of the next page;
				// we may need them for a relocation that crosses into the next page.
				if (low+offset+PAGE_SIZE >= high)
					prevdw = 0;
				else if (!PageInPage(oeptr,&prevdw,offset+PAGE_SIZE,optr,2)) {
					DEBUGMSG(ZONE_PAGING,(L"    PI: PIP Failed 2!\r\n"));
					VirtualFree(pMem2,PAGE_SIZE,MEM_DECOMMIT);
					VirtualFree(pMem,0,MEM_RELEASE);
					retval = 3;
					break;
				}
#else
				prevdw = 0;
#endif
				if (!PageInPage(oeptr,pMem2,offset,optr,PAGE_SIZE)) {
					DEBUGMSG(ZONE_PAGING,(L"    PI: PIP Failed!\r\n"));
					retval = 3;
				} else if (RelocatePage(eptr,optr,(ulong)pProc->BasePtr,pProc->dwVMBase,low+offset,(DWORD)pMem2,prevdw)) {
					if (!VirtualQuery((LPVOID)MapPtrProc((low+offset),pProc),&mbi,sizeof(mbi))) {
						retval = 0;
						DEBUGMSG(ZONE_PAGING,(L"  Page in failure in VQ 2!\r\n"));
					} else if (mbi.State != MEM_COMMIT) {
					    SC_CacheSync(CACHE_SYNC_INSTRUCTIONS);
						if (VirtualCopy((LPVOID)MapPtrProc((low+offset),pProc),pMem2,PAGE_SIZE,optr->o32_access))
							retval = 1;
						else {
							retval = 0;
							DEBUGMSG(ZONE_PAGING,(L"  Page in failure in VC 1!\r\n"));
						}
					} else
						retval = 1;
				} else {
					DEBUGMSG(ZONE_PAGING,(L"  Page in failure in RP!\r\n"));
					retval = 0;
				}
				if (!VirtualFree(pMem2,PAGE_SIZE,MEM_DECOMMIT))
					DEBUGMSG(ZONE_PAGING,(L"  Page in failure in VF1!\r\n"));
				if (!VirtualFree(pMem,0,MEM_RELEASE))
					DEBUGMSG(ZONE_PAGING,(L"  Page in failure in VF2!\r\n"));
			} else {
				o32rp = (o32_rom *)(oeptr->tocptr->ulO32Offset+loop*sizeof(o32_rom));
				if ((optr->o32_vsize > o32rp->o32_psize) || (optr->o32_flags & (IMAGE_SCN_MEM_WRITE | IMAGE_SCN_COMPRESSED)) ||
					(o32rp->o32_dataptr & (PAGE_SIZE-1))) {
					if (!(pMem = VirtualAlloc((LPVOID)ProcArray[0].dwVMBase,0x10000,MEM_RESERVE,PAGE_NOACCESS))) {
						DEBUGMSG(ZONE_PAGING,(L"    PI: VA1 Failed!\r\n"));
						break;
					}
					pMem2 = ((LPVOID)((DWORD)pMem + ((low+offset)&0xffff)));
					if (!VirtualAlloc(pMem2,PAGE_SIZE,MEM_COMMIT,PAGE_EXECUTE_READWRITE)) {
						DEBUGMSG(ZONE_PAGING,(L"    PI: VA Failed!\r\n"));
						VirtualFree(pMem,0,MEM_RELEASE);
						break;
					}
					DEBUGMSG(ZONE_PAGING,(TEXT("memcopying!\r\n")));
					if (optr->o32_flags & IMAGE_SCN_COMPRESSED)
						Decompress((LPVOID)(o32rp->o32_dataptr),o32rp->o32_psize,pMem2,PAGE_SIZE,offset);
					else if (o32rp->o32_psize > offset)
						memcpy((LPVOID)pMem2,(LPVOID)(o32rp->o32_dataptr+offset),min(o32rp->o32_psize-offset,PAGE_SIZE));
				    SC_CacheSync(CACHE_SYNC_INSTRUCTIONS);
					if (VirtualCopy((LPVOID)MapPtrProc(low+offset,pProc),pMem2,PAGE_SIZE,optr->o32_access))
						retval = 1;
					else
						DEBUGMSG(ZONE_PAGING,(L"  Page in failure in VC 2!\r\n"));
					if (!VirtualFree(pMem2,PAGE_SIZE,MEM_DECOMMIT))
						DEBUGMSG(ZONE_PAGING,(L"  Page in failure in VF1!\r\n"));
					if (!VirtualFree(pMem,0,MEM_RELEASE))
						DEBUGMSG(ZONE_PAGING,(L"  Page in failure in VF2!\r\n"));
				} else {
					DEBUGMSG(ZONE_PAGING,(TEXT("virtualcopying 1!\r\n")));
			    	if (VirtualCopy((LPVOID)MapPtrProc(low+offset,pProc),(LPVOID)(o32rp->o32_dataptr+offset),PAGE_SIZE, optr->o32_access))
			    		retval = 1;
			    }
			}
			break;
		}
	}
	if (loop == eptr->e32_objcnt) {
		DEBUGMSG(ZONE_PAGING,(L"     Could not find section to page in!\r\n"));
		retval = 0;
	}
	if (retval == 1)
		PagedInCount++;
	return retval;
}

int PageInModule(PMODULE pMod, DWORD addr) {
	e32_lite *eptr = &pMod->e32;
	o32_lite *oarry = pMod->o32_ptr;
	o32_lite *optr;
	o32_rom *o32rp;
	openexe_t *oeptr = &pMod->oe;
	int loop;
	LPVOID pMem, pMem2;
	DWORD low, high, offset, zaddr, prevdw, loaded = 0;
	BOOL retval = 2;
	MEMORY_BASIC_INFORMATION mbi;
	addr = addr & ~(PAGE_SIZE-1);
	zaddr = ZeroPtr(addr);
	for (loop = 0; loop < eptr->e32_objcnt; loop++) {
		optr = &oarry[loop];
		low = ZeroPtr(optr->o32_realaddr);
		high = ((low + optr->o32_vsize) + (PAGE_SIZE-1))&~(PAGE_SIZE-1);
		if ((zaddr >= low) && (zaddr < high)) {
			offset = zaddr - low;
			VirtualQuery((LPVOID)(low+offset+ProcArray[0].dwVMBase),&mbi,sizeof(mbi));
			DEBUGCHK(mbi.State != MEM_FREE);
			if (mbi.State != MEM_COMMIT) {
				loaded = 1;
				if ((oeptr->filetype == FT_PPFSFILE) || (oeptr->filetype == FT_OBJSTORE)) {
					if (!(pMem = VirtualAlloc((LPVOID)ProcArray[0].dwVMBase,0x10000,MEM_RESERVE,PAGE_NOACCESS))) {
						DEBUGMSG(ZONE_PAGING,(L"    PI: VA1 Failed!\r\n"));
						break;
					}
					pMem2 = ((LPVOID)((DWORD)pMem + ((low+offset)&0xffff)));
					if (!VirtualAlloc(pMem2,PAGE_SIZE,MEM_COMMIT,PAGE_EXECUTE_READWRITE)) {
						DEBUGMSG(ZONE_PAGING,(L"    PI: VA Failed!\r\n"));
						VirtualFree(pMem,0,MEM_RELEASE);
						break;
					}
#ifdef x86
					if (offset < 4)
						prevdw = 0;
					else if (!PageInPage(oeptr,&prevdw,offset-4,optr,4)) {
						DEBUGMSG(ZONE_PAGING,(L"    PI: PIP Failed 2!\r\n"));
						VirtualFree(pMem2,PAGE_SIZE,MEM_DECOMMIT);
						VirtualFree(pMem,0,MEM_RELEASE);
						retval = 0;
						break;
					}
#elif defined(MIPS16SUPPORT)
					// If we're not on the last page, peek at the first 2 bytes of the next page;
					// we may need them for a relocation that crosses into the next page.
					if (low+offset+PAGE_SIZE >= high)
						prevdw = 0;
					else if (!PageInPage(oeptr,&prevdw,offset+PAGE_SIZE,optr,2)) {
						DEBUGMSG(ZONE_PAGING,(L"    PI: PIP Failed 2!\r\n"));
						VirtualFree(pMem2,PAGE_SIZE,MEM_DECOMMIT);
						VirtualFree(pMem,0,MEM_RELEASE);
						retval = 0;
						break;
					}
#else
					prevdw = 0;
#endif
					if (!PageInPage(oeptr,pMem2,offset,optr,PAGE_SIZE)) {
						DEBUGMSG(ZONE_PAGING,(L"    PI: PIP Failed!\r\n"));
						retval = 0;
					} else if ((pMod->wFlags & LOAD_LIBRARY_AS_DATAFILE) || RelocatePage(eptr,optr,(ulong)pMod->BasePtr,ProcArray[0].dwVMBase,low+offset,(DWORD)pMem2,prevdw)) {
						if (!VirtualQuery((LPVOID)(low+offset+ProcArray[0].dwVMBase),&mbi,sizeof(mbi))) {
							retval = 0;
							DEBUGMSG(ZONE_PAGING,(L"  Page in failure in VQ 1!\r\n"));
						} else if (mbi.State != MEM_COMMIT) {
						    SC_CacheSync(CACHE_SYNC_INSTRUCTIONS);
							if (VirtualCopy((LPVOID)(low+offset+ProcArray[0].dwVMBase),pMem2,PAGE_SIZE,optr->o32_access)) {
								retval = 1;
							} else {
								retval = 0;
								DEBUGMSG(ZONE_PAGING,(L"  Page in failure in VC 3!\r\n"));
							}
						} else
							retval = 1;
					} else {
						DEBUGMSG(ZONE_PAGING,(L"  Page in failure in RP!\r\n"));
						retval = 0;
					}
					if (!VirtualFree(pMem2,PAGE_SIZE,MEM_DECOMMIT))
						DEBUGMSG(ZONE_PAGING,(L"  Page in failure in VF1!\r\n"));
					if (!VirtualFree(pMem,0,MEM_RELEASE))
						DEBUGMSG(ZONE_PAGING,(L"  Page in failure in VF2!\r\n"));
				} else {
					o32rp = (o32_rom *)(oeptr->tocptr->ulO32Offset+loop*sizeof(o32_rom));
					if ((optr->o32_vsize > o32rp->o32_psize) || (optr->o32_flags & (IMAGE_SCN_MEM_WRITE | IMAGE_SCN_COMPRESSED)) ||
						(o32rp->o32_dataptr & (PAGE_SIZE-1))) {
						if (!(pMem = VirtualAlloc((LPVOID)ProcArray[0].dwVMBase,0x10000,MEM_RESERVE,PAGE_NOACCESS))) {
							DEBUGMSG(ZONE_PAGING,(L"    PI: VA1 Failed!\r\n"));
							break;
						}
						pMem2 = ((LPVOID)((DWORD)pMem + ((low+offset)&0xffff)));
						if (!VirtualAlloc(pMem2,PAGE_SIZE,MEM_COMMIT,PAGE_EXECUTE_READWRITE)) {
							DEBUGMSG(ZONE_PAGING,(L"    PI: VA Failed!\r\n"));
							VirtualFree(pMem,0,MEM_RELEASE);
							break;
						}
						DEBUGMSG(ZONE_PAGING,(TEXT("memcopying!\r\n")));
						if (optr->o32_flags & IMAGE_SCN_COMPRESSED)
							Decompress((LPVOID)(o32rp->o32_dataptr),o32rp->o32_psize,pMem2,PAGE_SIZE,offset);
						else if (o32rp->o32_psize > offset)
							memcpy((LPVOID)pMem2,(LPVOID)(o32rp->o32_dataptr+offset),min(o32rp->o32_psize-offset,PAGE_SIZE));
					    SC_CacheSync(CACHE_SYNC_INSTRUCTIONS);
						if (VirtualCopy((LPVOID)(low+offset+ProcArray[0].dwVMBase),pMem2,PAGE_SIZE,optr->o32_access))
							retval = 1;
						else
							DEBUGMSG(ZONE_PAGING,(L"  Page in failure in VC 4!\r\n"));
						if (!VirtualFree(pMem2,PAGE_SIZE,MEM_DECOMMIT))
							DEBUGMSG(ZONE_PAGING,(L"  Page in failure in VF1!\r\n"));
						if (!VirtualFree(pMem,0,MEM_RELEASE))
							DEBUGMSG(ZONE_PAGING,(L"  Page in failure in VF2!\r\n"));
					} else {
						DEBUGMSG(ZONE_PAGING,(TEXT("virtualcopying 2!\r\n")));
				    	if (VirtualCopy((LPVOID)(low+offset+ProcArray[0].dwVMBase),(LPVOID)(o32rp->o32_dataptr+offset),PAGE_SIZE, optr->o32_access))
				    		retval = 1;
				    }
				}
			} else
				retval = 1;
			if ((retval == 1) && (addr != low+offset+ProcArray[0].dwVMBase)) {
				if (!(optr->o32_flags & IMAGE_SCN_MEM_WRITE) ||
					 (optr->o32_flags & IMAGE_SCN_MEM_SHARED) ||
					 (optr->o32_rva == pMod->e32.e32_unit[IMP].rva)) {
				    SC_CacheSync(CACHE_SYNC_INSTRUCTIONS);
					if (!(VirtualCopy((LPVOID)addr,(LPVOID)(low+offset+ProcArray[0].dwVMBase),
						PAGE_SIZE,optr->o32_access))) {
						DEBUGMSG(ZONE_PAGING,(L"CM: Failed 1\r\n"));
						retval = 2;
					}
				} else {
					if (loaded && (pMod->oe.filetype == FT_ROMIMAGE)) {
						DEBUGCHK(((DWORD)pMod->o32_ptr / PAGE_SIZE) != ((low+offset+ProcArray[0].dwVMBase)/PAGE_SIZE));
						DEBUGCHK(((DWORD)pMod->lpszModName / PAGE_SIZE) != ((low+offset+ProcArray[0].dwVMBase)/PAGE_SIZE));
					    SC_CacheSync(CACHE_SYNC_INSTRUCTIONS);
						if (!VirtualCopy((LPVOID)addr,(LPVOID)(low+offset+ProcArray[0].dwVMBase),PAGE_SIZE,optr->o32_access)) {
							DEBUGMSG(ZONE_PAGING,(L"CM: Failed 5\r\n"));
							VirtualFree(pMem,0,MEM_RELEASE);
							retval = 2;
						} else {
							if (!VirtualFree((LPVOID)(low+offset+ProcArray[0].dwVMBase),PAGE_SIZE,MEM_DECOMMIT))
								DEBUGMSG(ZONE_PAGING,(L"  Page in failure in VF3x!\r\n"));
						}
					} else {
						if (!(pMem = VirtualAlloc((LPVOID)ProcArray[0].dwVMBase,0x10000,MEM_RESERVE,PAGE_NOACCESS))) {
							DEBUGMSG(ZONE_PAGING,(L"CM: Failed 2\r\n"));
							retval = 2;
							break;
						}
						pMem2 = ((LPVOID)((DWORD)pMem + ((low+offset)&0xffff)));
						if (!VirtualAlloc(pMem2,PAGE_SIZE,MEM_COMMIT,PAGE_EXECUTE_READWRITE)) {
							DEBUGMSG(ZONE_PAGING,(L"CM: Failed 3\r\n"));
							VirtualFree(pMem,0,MEM_RELEASE);
							retval = 2;
							break;
						}
						memcpy(pMem2,(LPVOID)(low+offset+ProcArray[0].dwVMBase),PAGE_SIZE);
					    SC_CacheSync(CACHE_SYNC_INSTRUCTIONS);
						if (!VirtualCopy((LPVOID)addr,pMem2,PAGE_SIZE,optr->o32_access)) {
							DEBUGMSG(ZONE_PAGING,(L"CM: Failed 4\r\n"));
							VirtualFree(pMem,0,MEM_RELEASE);
							retval = 2;
						}
						if (!VirtualFree(pMem2,PAGE_SIZE,MEM_DECOMMIT))
							DEBUGMSG(ZONE_PAGING,(L"  Page in failure in VF1x!\r\n"));
						if (!VirtualFree(pMem,0,MEM_RELEASE))
							DEBUGMSG(ZONE_PAGING,(L"  Page in failure in VF2x!\r\n"));
					}
				}
			}
			break;
		}
	}
	if (loop == eptr->e32_objcnt) {
		DEBUGMSG(ZONE_PAGING,(L"     Could not find section to page in!\r\n"));
		retval = 0;
	}
	DEBUGMSG(ZONE_PAGING && !retval,(L"PIM: Failure!\r\n"));
	if (retval == 1)
		PagedInCount++;
	return retval;
}

DWORD AccessFromFlags(DWORD flags) {
	if (flags & IMAGE_SCN_MEM_WRITE)
		return (flags & IMAGE_SCN_MEM_EXECUTE ? PAGE_EXECUTE_READWRITE : PAGE_READWRITE);
	else
		return (flags & IMAGE_SCN_MEM_EXECUTE ? PAGE_EXECUTE_READ : PAGE_READONLY);
}

void UnloadMod(PMODULE pMod) {
	int loop, loop2;
	o32_lite *optr;
	if (pMod->oe.pagemode == PM_FULLPAGING) {
		for (loop = 0; loop < pMod->e32.e32_objcnt; loop++) {
			optr = &pMod->o32_ptr[loop];
			if (!(optr->o32_flags & IMAGE_SCN_MEM_WRITE) &&
				((pMod->oe.filetype != FT_ROMIMAGE) || (optr->o32_flags & IMAGE_SCN_COMPRESSED) ||
				 ((o32_rom *)(pMod->oe.tocptr->ulO32Offset+loop*sizeof(o32_rom)))->o32_dataptr & (PAGE_SIZE-1))) {
				EnterCriticalSection(&VAcs);
				VirtualFree(MapPtrProc(ZeroPtr(optr->o32_realaddr),&ProcArray[0]),optr->o32_vsize,MEM_DECOMMIT);
				for (loop2 = 1; loop2 < MAX_PROCESSES; loop2++)
					if (HasModRefProcIndex(pMod,loop2))
						VirtualFree(MapPtrProc(ZeroPtr(optr->o32_realaddr),&ProcArray[loop2]),optr->o32_vsize,MEM_DECOMMIT);
				LeaveCriticalSection(&VAcs);
			}
		}
	}
}

void UnloadExe(PPROCESS pProc) {
	int loop;
	o32_lite *optr;
	if (pProc->oe.pagemode == PM_FULLPAGING) {
		for (loop = 0; loop < pProc->e32.e32_objcnt; loop++) {
			optr = &pProc->o32_ptr[loop];
			if (!(optr->o32_flags & IMAGE_SCN_MEM_WRITE) &&
				((pProc->oe.filetype != FT_ROMIMAGE) || (optr->o32_flags & IMAGE_SCN_COMPRESSED) ||
				 ((o32_rom *)(pProc->oe.tocptr->ulO32Offset+loop*sizeof(o32_rom)))->o32_dataptr & (PAGE_SIZE-1)))
				VirtualFree(MapPtrProc(ZeroPtr(optr->o32_realaddr),pProc),optr->o32_vsize,MEM_DECOMMIT);
		}
	}
}

// Load all O32 headers

DWORD LoadO32(openexe_t *oeptr, e32_lite *eptr, o32_lite *oarry, ulong BasePtr) {
	int loop, bytesread;
	o32_obj o32;
	o32_rom *o32rp;
	o32_lite *optr;
	for (loop = 0; loop < eptr->e32_objcnt; loop++) {
		optr = &oarry[loop];
		if (oeptr->filetype == FT_PPFSFILE) {
			if (rlseek(oeptr->hppfs,oeptr->offset+sizeof(e32_exe)+loop*sizeof(o32_obj),0) !=
				(int)(oeptr->offset+sizeof(e32_exe)+loop*sizeof(o32_obj)))
				return ERROR_BAD_EXE_FORMAT;
			if (rread(oeptr->hppfs,(LPBYTE)&o32,sizeof(o32_obj)) != sizeof(o32_obj))
				return ERROR_BAD_EXE_FORMAT;
			optr->o32_realaddr = BasePtr + o32.o32_rva;
			optr->o32_vsize = o32.o32_vsize;
			optr->o32_rva = o32.o32_rva;
			optr->o32_flags = o32.o32_flags;
			optr->o32_psize = o32.o32_psize;
			optr->o32_dataptr = o32.o32_dataptr;
			if (optr->o32_rva == eptr->e32_unit[RES].rva)
				optr->o32_flags &= ~IMAGE_SCN_MEM_DISCARDABLE;
			optr->o32_access = AccessFromFlags(optr->o32_flags);
			if (oeptr->pagemode == PM_NOPAGING) {
				if (rlseek(oeptr->hppfs,o32.o32_dataptr,0) != (int)o32.o32_dataptr)
					return ERROR_BAD_EXE_FORMAT;
			    if (!(VirtualAlloc((LPVOID)optr->o32_realaddr,optr->o32_vsize,MEM_COMMIT,
					PAGE_EXECUTE_READWRITE)))
					return ERROR_OUTOFMEMORY;
				if (rread(oeptr->hppfs,(LPBYTE)optr->o32_realaddr,min(o32.o32_psize,optr->o32_vsize)) !=
					(int)min(o32.o32_psize,optr->o32_vsize))
					return ERROR_BAD_EXE_FORMAT;
			} else if (o32.o32_psize) {
				if ((DWORD)rlseek(oeptr->hppfs,0,FILE_END) < o32.o32_dataptr + o32.o32_psize)
					return ERROR_BAD_EXE_FORMAT;
			}
		} else if (oeptr->filetype == FT_OBJSTORE) {
			if (SetFilePointer(oeptr->hf,oeptr->offset+sizeof(e32_exe)+loop*sizeof(o32_obj),0,FILE_BEGIN) != oeptr->offset+sizeof(e32_exe)+loop*sizeof(o32_obj))
				return ERROR_BAD_EXE_FORMAT;
			if (!ReadFile(oeptr->hf,(LPBYTE)&o32,sizeof(o32_obj),&bytesread,0) || (bytesread != sizeof(o32_obj)))
				return ERROR_BAD_EXE_FORMAT;
			optr->o32_realaddr = BasePtr + o32.o32_rva;
			optr->o32_vsize = o32.o32_vsize;
			optr->o32_rva = o32.o32_rva;
			optr->o32_flags = o32.o32_flags;
			optr->o32_psize = o32.o32_psize;
			optr->o32_dataptr = o32.o32_dataptr;
			if (optr->o32_rva == eptr->e32_unit[RES].rva)
				optr->o32_flags &= ~IMAGE_SCN_MEM_DISCARDABLE;
			optr->o32_access = AccessFromFlags(optr->o32_flags);
			if (oeptr->pagemode == PM_NOPAGING) {
				if (SetFilePointer(oeptr->hf,o32.o32_dataptr,0,FILE_BEGIN) != o32.o32_dataptr)
					return ERROR_BAD_EXE_FORMAT;
			    if (!(VirtualAlloc((LPVOID)optr->o32_realaddr,optr->o32_vsize,MEM_COMMIT,
					PAGE_EXECUTE_READWRITE)))
					return ERROR_OUTOFMEMORY;
				if (!ReadFile(oeptr->hf,(LPBYTE)optr->o32_realaddr,min(o32.o32_psize,optr->o32_vsize),&bytesread,0) ||
					(bytesread != (int)min(o32.o32_psize,optr->o32_vsize)))
					return ERROR_BAD_EXE_FORMAT;
			} else if (o32.o32_psize) {
				if (SetFilePointer(oeptr->hf,0,0,FILE_END) < o32.o32_dataptr + o32.o32_psize)
					return ERROR_BAD_EXE_FORMAT;
			}
		} else {
			o32rp = (o32_rom *)(oeptr->tocptr->ulO32Offset+loop*sizeof(o32_rom));
			optr->o32_realaddr = o32rp->o32_realaddr;
			optr->o32_vsize = o32rp->o32_vsize;
			optr->o32_psize = o32rp->o32_psize;
			optr->o32_rva = o32rp->o32_rva;
			optr->o32_flags = o32rp->o32_flags;
			if (optr->o32_rva == eptr->e32_unit[RES].rva)
				optr->o32_flags &= ~IMAGE_SCN_MEM_DISCARDABLE;
			optr->o32_access = AccessFromFlags(optr->o32_flags);
			if ((optr->o32_vsize > o32rp->o32_psize) || (optr->o32_flags & IMAGE_SCN_MEM_WRITE) || (o32rp->o32_dataptr & (PAGE_SIZE-1))) {
				if (oeptr->pagemode == PM_NOPAGING) {
					DEBUGMSG(ZONE_LOADER1,(TEXT("memcopying!\r\n")));
			    	if (!(VirtualAlloc((LPVOID)optr->o32_realaddr,optr->o32_vsize,MEM_COMMIT,
						PAGE_EXECUTE_READWRITE)))
						return ERROR_OUTOFMEMORY;
					if (o32rp->o32_psize) {
						if (optr->o32_flags & IMAGE_SCN_COMPRESSED)
							Decompress((LPVOID)(o32rp->o32_dataptr),o32rp->o32_psize,
								(PUCHAR)optr->o32_realaddr,optr->o32_vsize,0);
						else
							memcpy((LPVOID)optr->o32_realaddr,(LPVOID)(o32rp->o32_dataptr),min(o32rp->o32_psize,optr->o32_vsize));
					}
				}
			} else {
				DEBUGMSG(ZONE_LOADER1,(TEXT("virtualcopying 3!\r\n")));
				if (!(VirtualCopy((LPVOID)optr->o32_realaddr,(LPVOID)(o32rp->o32_dataptr),optr->o32_vsize, optr->o32_access)))
					return ERROR_OUTOFMEMORY;
			}
		}
	}
	return 0;
}

// Find starting IP for EXE (or entrypoint for DLL)

ulong FindEntryPoint(DWORD entry, e32_lite *eptr, o32_lite *oarry) {
	o32_lite *optr;
	int loop;
	for (loop = 0; loop < eptr->e32_objcnt; loop++) {
		optr = &oarry[loop];
		if ((entry >= optr->o32_rva) ||
			(entry < optr->o32_rva+optr->o32_vsize))
			return optr->o32_realaddr + entry - optr->o32_rva;
	}
	return 0;
}

BOOL OpenAnExe(LPWSTR exename, openexe_t *oe, BOOL bAllowPaging) {
	static WCHAR fname[MAX_PATH];
	DWORD len;
	len = strlenW(exename);
	if (OpenExe(exename,oe,0,bAllowPaging))
		return TRUE;
	if (len + 4 < MAX_PATH) {
		memcpy(fname,exename,len*sizeof(WCHAR));
		memcpy(fname+len,L".exe",8);
		fname[len+4] = 0;
		return OpenExe(fname,oe,0,bAllowPaging);
	}
	return FALSE;
}

int OpenADll(LPWSTR dllname, openexe_t *oe, BOOL bAllowPaging) {
	static WCHAR fname[MAX_PATH];
	DWORD len;
	len = strlenW(dllname);
	if (OpenExe(dllname,oe,1,bAllowPaging))
		return 2;		// has not been extended
	if (len + 4 < MAX_PATH) {
		memcpy(fname,dllname,len*sizeof(WCHAR));
		memcpy(fname+len,L".dll",8);
		fname[len+4] = 0;
		if (OpenExe(fname,oe,1,bAllowPaging))
			return 1;	// has been extended
	}
	return 0;
}

DWORD CalcStackSize(DWORD cbStack) {
	if (!cbStack)
		cbStack = pCurProc->e32.e32_stackmax;
	cbStack = (cbStack + PAGE_SIZE - 1) & ~(PAGE_SIZE-1);
	if (cbStack < (MIN_STACK_RESERVE*2))
		cbStack = (MIN_STACK_RESERVE*2);
	if (cbStack > MAX_STACK_RESERVE)
		cbStack = MAX_STACK_RESERVE;
	return cbStack;
}

void LoadSwitch(LSInfo_t *plsi) {
	KCALLPROFON(46);
	pCurThread->bSuspendCnt = 1;
	DEBUGCHK(!((pCurThread->wInfo >> DEBUG_LOOPCNT_SHIFT) & 1));
	SET_RUNSTATE(pCurThread,RUNSTATE_BLOCKED);
	RunList.pth = 0;
	SetReschedule();
	DEBUGCHK(plsi->pHelper->bSuspendCnt == 1);
	--(plsi->pHelper->bSuspendCnt);
	MakeRun(plsi->pHelper);
	KCALLPROFOFF(46);
}

void FinishLoadSwitch(LSInfo_t *plsi) {
	DWORD retval;
	LPVOID stkbase;
	PTHREAD pthread;
	SETCURKEY((DWORD)-1);
	pthread = HandleToThread(plsi->hth);
	DEBUGCHK(pthread);
	MDCreateMainThread2(pthread,plsi->cbStack,
		(LPVOID)(pthread->pOwnerProc->e32.e32_sect14rva ? CTBFf : MTBFf),
		(LPVOID)plsi->startip,TH_UMODE,
		(ULONG)pthread->pOwnerProc->hProc,0,(plsi->cmdlen+1)*sizeof(WCHAR),
		(plsi->namelen+1)*sizeof(WCHAR),
		pthread->pOwnerProc->e32.e32_sect14rva ? (DWORD)pthread->pOwnerProc : SW_SHOW);
	pthread->aky = pthread->pOwnerProc->aky;
	AddAccess(&pthread->aky,ProcArray[0].aky);
	if (!(plsi->flags & CREATE_SUSPENDED))
		KCall((PKFN)ThreadResume,pthread);
	SetEvent(plsi->hEvent);
	stkbase = plsi->lpOldStack; // need temp since plsi points to the memory we're freeing
	retval = VirtualFree(stkbase,CNP_STACK_SIZE,MEM_DECOMMIT);
	DEBUGCHK(retval);
	retval = VirtualFree(stkbase,0,MEM_RELEASE);
	DEBUGCHK(retval);
	KCall((PKFN)KillSpecialThread);
	DEBUGCHK(0);
}

// Startup thread for new process (does loading)

void CreateNewProc(ulong nextfunc, ulong param) {
	e32_lite *eptr;
	LPVOID lpStack = 0;
	LPDWORD pOldTLS;
	DWORD flags, entry, loop, inRom, dwretval;
	LPWSTR procname, uptr;
	CALLBACKINFO cbi;
	LPProcStartInfo lppsi = (LPProcStartInfo)param;
	LSInfo_t lsi;
	ulong startip;
	DEBUGCHK(RunList.pth == pCurThread);
	eptr = &pCurProc->e32;
	DEBUGMSG(ZONE_LOADER1,(TEXT("CreateNewProc %s (base %8.8lx)\r\n"),lppsi->lpszImageName,pCurProc->dwVMBase));
	if (pCurThread->pThrdDbg && pCurThread->pThrdDbg->hEvent) {
		SC_SetHandleOwner(pCurThread->pThrdDbg->hEvent,hCurProc);
		SC_SetHandleOwner(pCurThread->pThrdDbg->hBlockEvent,hCurProc);
	}
	EnterCriticalSection(&LLcs);
	lppsi->lppi->hProcess = 0;      // fail by default
	if (pCurProc == &ProcArray[0]) {
		if (!OpenAnExe(lppsi->lpszImageName,&pCurProc->oe,(lppsi->fdwCreate & 0x80000000) ? 0 : 1)) {
			lppsi->lppi->dwThreadId = ERROR_FILE_NOT_FOUND;
			goto cnperr;
		}
	} else {
		cbi.hProc = ProcArray[0].hProc;
		cbi.pfn = (FARPROC)OpenAnExe;
		cbi.pvArg0 = MapPtr(lppsi->lpszImageName);
		if (!PerformCallBack4Int(&cbi,MapPtr(&pCurProc->oe),(lppsi->fdwCreate & 0x80000000) ? 0 : 1)) {
			lppsi->lppi->dwThreadId = ERROR_FILE_NOT_FOUND;
			goto cnperr;
		}
	}
	if (loop = LoadE32(&pCurProc->oe,eptr,&flags,&entry,0,1,&pCurProc->bTrustLevel)) {
		lppsi->lppi->dwThreadId = loop;
		goto cnperr;
	}
	if (!eptr->e32_sect14rva && (loop = VerifyBinary(&pCurProc->oe,lppsi->lpszImageName,&pCurProc->bTrustLevel))) {
		lppsi->lppi->dwThreadId = loop;
		goto cnperr;
	}
	if (flags & IMAGE_FILE_DLL) {
		lppsi->lppi->dwThreadId = ERROR_BAD_EXE_FORMAT;
		goto cnperr;
	}
	if ((flags & IMAGE_FILE_RELOCS_STRIPPED) && ((eptr->e32_vbase < 0x10000) || (eptr->e32_vbase > 0x400000))) {
		DEBUGMSG(ZONE_LOADER1,(TEXT("No relocations - can't load/fixup!\r\n")));
		lppsi->lppi->dwThreadId = ERROR_BAD_EXE_FORMAT;
		goto cnperr;
	}
	pCurProc->BasePtr = (flags & IMAGE_FILE_RELOCS_STRIPPED) ? (LPVOID)eptr->e32_vbase : (LPVOID)0x10000;
    if (!VirtualAlloc(pCurProc->BasePtr,eptr->e32_vsize,MEM_RESERVE|MEM_IMAGE, PAGE_NOACCESS)) {
		lppsi->lppi->dwThreadId = ERROR_OUTOFMEMORY;
		goto cnperr;
	}
	if (!(pCurProc->o32_ptr = VirtualAlloc(0,eptr->e32_objcnt*sizeof(o32_lite),
		MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE))) {
		lppsi->lppi->dwThreadId = ERROR_OUTOFMEMORY;
		goto cnperr;
	}
	pCurProc->o32_ptr = MapPtr(pCurProc->o32_ptr);
	DEBUGMSG(ZONE_LOADER1,(TEXT("BasePtr is %8.8lx\r\n"),pCurProc->BasePtr));
	if (dwretval = LoadO32(&pCurProc->oe,eptr,pCurProc->o32_ptr,(ulong)pCurProc->BasePtr)) {
		lppsi->lppi->dwThreadId = dwretval;
		goto cnperr;
	}
	for (loop = 0; loop < eptr->e32_objcnt; loop++) {
		if (!(pCurProc->o32_ptr[loop].o32_flags & IMAGE_SCN_MEM_DISCARDABLE) &&
			(pCurProc->o32_ptr[loop].o32_flags & IMAGE_SCN_MEM_WRITE)) {
			DWORD base, size;
			base = pCurProc->o32_ptr[loop].o32_realaddr;
			size = pCurProc->o32_ptr[loop].o32_vsize;
			size = (size + 3) & ~3;
			if ((size % PAGE_SIZE) && ((PAGE_SIZE - (size % PAGE_SIZE)) >= eptr->e32_objcnt*sizeof(o32_lite))) {
				memcpy((LPBYTE)base+size,pCurProc->o32_ptr,eptr->e32_objcnt*sizeof(o32_lite));
				VirtualFree(pCurProc->o32_ptr,0,MEM_RELEASE);
				pCurProc->o32_ptr = (o32_lite *)MapPtrProc((base+size),pCurProc);
				pCurProc->o32_ptr[loop].o32_vsize = size + eptr->e32_objcnt*sizeof(o32_lite);
				break;
			}
		}
	}
	if (pCurProc->oe.filetype != FT_ROMIMAGE) {
		if ((pCurProc->oe.pagemode == PM_NOPAGING) && !Relocate(eptr, pCurProc->o32_ptr, (ulong)pCurProc->BasePtr,0)) {
			lppsi->lppi->dwThreadId = ERROR_OUTOFMEMORY;
			goto cnperr;
		}
		inRom = 0;
	} else
		inRom = 1;
	LoadOneLibraryW(L"coredll.dll",1,0);
	if (!InitBreadcrumbs()) {
		lppsi->lppi->dwThreadId = ERROR_OUTOFMEMORY;
		goto cnperr;
	}
	loop = DoImports(eptr, pCurProc->o32_ptr, (ulong)pCurProc->BasePtr,0);
	FinishBreadcrumbs();
	if (loop) {
		lppsi->lppi->dwThreadId = loop;
		goto cnperr;
	}
	if ((pCurProc->oe.pagemode == PM_NOPAGING) && !AdjustRegions(eptr, pCurProc->o32_ptr)) {
		lppsi->lppi->dwThreadId = ERROR_OUTOFMEMORY;
		goto cnperr;
	}
	eptr->e32_stackmax = CalcStackSize(eptr->e32_stackmax);
	if (!(lpStack = VirtualAlloc((LPVOID)pCurProc->dwVMBase,eptr->e32_stackmax,MEM_RESERVE|MEM_AUTO_COMMIT,PAGE_NOACCESS))) {
		lppsi->lppi->dwThreadId = ERROR_OUTOFMEMORY;
		goto cnperr;
	}
	if (!VirtualAlloc((LPVOID)((DWORD)lpStack+eptr->e32_stackmax-MIN_STACK_SIZE), MIN_STACK_SIZE, MEM_COMMIT, PAGE_READWRITE)) {
		lppsi->lppi->dwThreadId = ERROR_OUTOFMEMORY;
		goto cnperr;
	}
	if (eptr->e32_sect14rva) {
		HANDLE hLib;
		if (!(hLib = LoadOneLibraryW(L"mscoree.dll",1,0))) {
			lppsi->lppi->dwThreadId = ERROR_DLL_INIT_FAILED;
			goto cnperr;
		}
		if (!(startip = (DWORD)GetProcAddressA(hLib,"_CorExeMain"))) {
			lppsi->lppi->dwThreadId = ERROR_DLL_INIT_FAILED;
			goto cnperr;
		}
	} else
		startip = FindEntryPoint(entry, eptr,pCurProc->o32_ptr);
	if (inRom && SystemAPISets[SH_PATCHER] && !PatchExe(pCurProc, lppsi->lpszImageName))
	    goto cnperr;
	DEBUGMSG(ZONE_LOADER1,(TEXT("Starting up the process!!! IP = %8.8lx\r\n"),startip));

	lppsi->lppi->hProcess = hCurProc;
	lppsi->lppi->hThread = hCurThread;
	lppsi->lppi->dwProcessId = (DWORD)hCurProc;
	lppsi->lppi->dwThreadId = (DWORD)hCurThread;
	DEBUGMSG(ZONE_LOADER1,(TEXT("Calling into the kernel for threadswitch!\r\n")));
	LeaveCriticalSection(&LLcs);
    SC_CacheSync(CACHE_SYNC_INSTRUCTIONS);
	uptr = procname = lppsi->lpszImageName;
	while (*uptr) {
		if ((*uptr == (WCHAR)'\\') || (*uptr == (WCHAR)'/'))
			procname = uptr+1;
		uptr++;
	}
	lsi.startip = startip;
	lsi.flags = lppsi->fdwCreate;
	lsi.lpStack = lpStack;
	lsi.lpOldStack = (LPVOID)pCurThread->dwStackBase;
	lsi.cbStack = eptr->e32_stackmax;
	lsi.cmdlen = strlenW(lppsi->lpszCmdLine);
	lsi.namelen = strlenW(procname);
	lsi.hEvent = lppsi->he;
	lsi.hth = hCurThread;
	pOldTLS = pCurThread->tlsPtr;
	pCurProc->pcmdline = MDCreateMainThread1(pCurThread,lpStack,eptr->e32_stackmax,pCurProc->dwVMBase,(LPBYTE)lppsi->lpszCmdLine,
		(lsi.cmdlen+1)*sizeof(WCHAR),(LPBYTE)procname,(lsi.namelen+1)*sizeof(WCHAR));
	memcpy(pCurThread->tlsPtr,pOldTLS,4*TLS_MINIMUM_AVAILABLE);
	pOldTLS = pCurThread->tlsPtr;
	KDUpdateSymbols(((DWORD)pCurProc->BasePtr)+1, FALSE);
	BlockWithHelperAlloc((FARPROC)LoadSwitch,(FARPROC)FinishLoadSwitch,(LPVOID)&lsi);
	DEBUGCHK(0);
	// Never returns
cnperr:
	// No need to VirtualFree, since entire section will soon be blown away
	SC_ProcessDetachAllDLLs();
	*(__int64 *)&pCurThread->ftCreate = 0; // so we don't get an end time
	CloseExe(&pCurProc->oe);
	DEBUGMSG(ZONE_LOADER1,(TEXT("CreateNewProc failure!\r\n")));
	DEBUGMSG(1,(TEXT("CreateNewProc failure on %s!\r\n"),lppsi->lpszImageName));
	LeaveCriticalSection(&LLcs);
	SetEvent(lppsi->he);
	SC_CloseProcOE(0);
	CloseAllCrits();
	SC_CloseAllHandles();
	SC_NKTerminateThread(0);
	DEBUGCHK(0);
	// Never returns
}

// Copy DLL regions from Proc0 to pProc

BOOL CopyRegions(PMODULE pMod) {
	int loop;
	DWORD oldprot;
	struct o32_lite *optr;
	LPBYTE addr;
	long basediff;
	// Don't copy from Proc0 to Proc0 - should only be for coredll.dll
	if (pCurProc->procnum) {
		basediff = pCurProc->dwVMBase - ProcArray[0].dwVMBase;
		if (!VirtualAlloc((LPVOID)((long)pMod->BasePtr+basediff),pMod->e32.e32_vsize,MEM_RESERVE|MEM_IMAGE,PAGE_NOACCESS))
			return FALSE;
		for (loop = 0; loop < pMod->e32.e32_objcnt; loop++) {
			optr = &pMod->o32_ptr[loop];
			addr = (LPBYTE)((long)pMod->BasePtr+basediff+optr->o32_rva);
			if (!(optr->o32_flags & IMAGE_SCN_MEM_DISCARDABLE)) {
				if (pMod->oe.pagemode == PM_NOPAGING) {
					if ((optr->o32_flags & IMAGE_SCN_MEM_SHARED) ||
						!(optr->o32_flags & IMAGE_SCN_MEM_WRITE) ||
					 	(optr->o32_rva == pMod->e32.e32_unit[IMP].rva)) {
						if (!(VirtualCopy(addr,(LPVOID)optr->o32_realaddr,optr->o32_vsize,optr->o32_access)))
							goto crerr;
					} else {
						if (!(VirtualAlloc(addr,optr->o32_vsize,MEM_COMMIT,PAGE_EXECUTE_READWRITE)))
							goto crerr;
						memcpy(addr,(LPVOID)optr->o32_realaddr,optr->o32_vsize);
						VirtualProtect(addr,optr->o32_vsize,optr->o32_access,&oldprot);
					}
				} else {
					if ((pMod->oe.filetype == FT_ROMIMAGE) &&
						!(optr->o32_vsize > optr->o32_psize) &&
						!(optr->o32_flags & IMAGE_SCN_MEM_WRITE) &&
						!(((o32_rom *)(pMod->oe.tocptr->ulO32Offset+loop*sizeof(o32_rom)))->o32_dataptr & (PAGE_SIZE-1)))
						if (!(VirtualCopy(addr,(LPVOID)optr->o32_realaddr,optr->o32_vsize,optr->o32_access)))
							goto crerr;
				}
			}
		}
	}
    SC_CacheSync(CACHE_SYNC_INSTRUCTIONS);
	return TRUE;
crerr:
	DEBUGMSG(ZONE_LOADER1,(TEXT("CopyRegion: Error!!\r\n")));
	DEBUGCHK(0);
	VirtualFree((LPVOID)((long)pMod->BasePtr+basediff),pMod->e32.e32_vsize,MEM_DECOMMIT);
	VirtualFree((LPVOID)((long)pMod->BasePtr+basediff),0,MEM_RELEASE);
	return FALSE;
}

PMODULE FindModByName(LPCWSTR dllname) {
	PMODULE pMod;
	WCHAR ch = lowerW(*dllname);
	for (pMod = pModList; pMod; pMod = pMod->pMod)
		if ((ch == pMod->lpszModName[0]) && !strcmpdllnameW(pMod->lpszModName,dllname))
			break;
	return pMod;
}

// Issue all connected DLL's a DLL_THREAD_DETACH

VOID SC_ThreadDetachAllDLLs(void) {
	PMODULE pMod;
	DEBUGMSG(ZONE_ENTRY,(L"SC_ThreadDetachAllDLLs entry\r\n"));
	EnterCriticalSection(&LLcs);
	for (pMod = pModList; pMod; pMod = pMod->pMod)
		if (HasModRefProcPtr(pMod,pCurProc) && AllowThreadMessage(pMod,pCurProc))
			CallDLLEntry(pMod,DLL_THREAD_DETACH,0);
	LeaveCriticalSection(&LLcs);
	DEBUGMSG(ZONE_ENTRY,(L"SC_ThreadDetachAllDLLs exit\r\n"));
}

// Issue all connected DLL's a DLL_PROCESS_DETACH

VOID SC_ProcessDetachAllDLLs(void) {
	PMODULE pMod;
	DEBUGMSG(ZONE_ENTRY,(L"SC_ProcessDetachAllDLLs entry\r\n"));
	EnterCriticalSection(&LLcs);
	for (pMod = pModList; pMod; pMod = pMod->pMod)
		if (HasModRefProcPtr(pMod,pCurProc)) 
			__try {
				CallDLLEntry(pMod,DLL_PROCESS_DETACH,(LPVOID)1);
			} __except (EXCEPTION_EXECUTE_HANDLER) {
			}
	LeaveCriticalSection(&LLcs);
	DEBUGMSG(ZONE_ENTRY,(L"SC_ProcessDetachAllDLLs exit\r\n"));
}

// Issue all connected DLL's a DLL_THREAD_ATTACH

VOID SC_ThreadAttachAllDLLs(void) {
	PMODULE pMod;
	DEBUGMSG(ZONE_ENTRY,(L"SC_ThreadAttachAllDLLs entry\r\n"));
	EnterCriticalSection(&LLcs);
	for (pMod = pModList; pMod; pMod = pMod->pMod)
		if (HasModRefProcPtr(pMod,pCurProc) && AllowThreadMessage(pMod,pCurProc))
			CallDLLEntry(pMod,DLL_THREAD_ATTACH,0);
	LeaveCriticalSection(&LLcs);
	DEBUGMSG(ZONE_ENTRY,(L"SC_ThreadAttachAllDLLs exit\r\n"));
}

BOOL DoCopyDepends(PMODULE pMod) {
	e32_lite *eptr = &pMod->e32;
	DWORD BaseAddr = (DWORD)pMod->BasePtr;
	ulong blocksize;
	LPDWORD ltptr;
	PMODULE pMod2;
	WCHAR ucptr[MAX_DLLNAME_LEN];
	DWORD oldref;
	struct ImpHdr *blockptr;
	struct ImpProc *impptr;
	if (!(blocksize = eptr->e32_unit[IMP].size)) // No relocations
		return TRUE;
	blockptr = (struct ImpHdr *)(BaseAddr+eptr->e32_unit[IMP].rva);
	while (blockptr->imp_lookup) {
		KAsciiToUnicode(ucptr,(LPCHAR)BaseAddr+blockptr->imp_dllname,MAX_DLLNAME_LEN);
		pMod2 = FindModByName(ucptr);
		if (!pMod2)
			return FALSE;
		if (!HasBreadcrumb(pMod2)) {
			SetBreadcrumb(pMod2);
			if (!(HasModRefProcPtr(pMod2,pCurProc)) && !CopyRegions(pMod2))
				return FALSE;
			ltptr = (LPDWORD)(BaseAddr+blockptr->imp_lookup);
			while (*ltptr) {
				if (*ltptr & 0x80000000)
					ResolveImpOrd(pMod2,*ltptr&0x7fffffff);
				else {
					impptr = (struct ImpProc *)((*ltptr&0x7fffffff)+BaseAddr);
					ResolveImpHintStr(pMod2,impptr->ip_hint,(LPBYTE)impptr->ip_name);
				}
				ltptr++;
			}
			oldref = IncRefCount(pMod2);
			if (!(pMod2->wFlags & DONT_RESOLVE_DLL_REFERENCES))
				DoCopyDepends(pMod2);
			if (!oldref)
				CallDLLEntry(pMod2,DLL_PROCESS_ATTACH,0);
		}
		blockptr++;
	}
	return TRUE;
}

BOOL UnDoDepends(PMODULE pMod) {
	e32_lite *eptr = &pMod->e32;
	DWORD BaseAddr = (DWORD)pMod->BasePtr;
	ulong blocksize;
	PMODULE pMod2;
	WCHAR ucptr[MAX_DLLNAME_LEN];
	struct ImpHdr *blockptr;
	if (!(blocksize = eptr->e32_unit[IMP].size)) // No relocations
		return TRUE;
	blockptr = (struct ImpHdr *)(BaseAddr+eptr->e32_unit[IMP].rva);
	while (blockptr->imp_lookup) {
		KAsciiToUnicode(ucptr,(LPCHAR)BaseAddr+blockptr->imp_dllname,MAX_DLLNAME_LEN);
		pMod2 = FindModByName(ucptr);
		

		if (pMod2)
			FreeOneLibraryPart2(pMod2, 1);
		blockptr++;
	}
	return TRUE;
}

// Load a library (DLL)

PMODULE LoadOneLibraryPart2(LPWSTR lpszFileName, BOOL bAllowPaging, WORD wFlags) {
	PMODULE pMod = 0;
	LPWSTR dllname;
	DWORD inRom, lasterr;
	PMODULE retval;
	e32_lite *eptr = 0;
	DWORD flags, entry, loop, len;
	DWORD special;	// is the debugger
	DWORD dwretval;
	BOOL bExtended;
	CALLBACKINFO cbi;
	if (wFlags & LOAD_LIBRARY_AS_DATAFILE)
		wFlags |= DONT_RESOLVE_DLL_REFERENCES;
	if ((DWORD)lpszFileName & 0x1) {
		special = 1;
		bAllowPaging = 0;
		(DWORD)lpszFileName &= ~0x1;
	} else
		special = 0;
	dllname = lpszFileName+strlenW(lpszFileName);
	while ((dllname != lpszFileName) && (*(dllname-1) != (WCHAR)'\\') && (*(dllname-1) != (WCHAR)'/'))
		dllname--;
	DEBUGMSG(ZONE_LOADER1,(TEXT("LoadOneLibrary %s (%s)\r\n"),lpszFileName,dllname));
	pMod = FindModByName(dllname);
	if (pMod) {
		if ((pCurProc->bTrustLevel == KERN_TRUST_FULL) && (pMod->bTrustLevel == KERN_TRUST_RUN)) {
			KSetLastError(pCurThread,(DWORD)NTE_BAD_SIGNATURE);
			return 0;
		}
		if (pMod->wFlags != wFlags) {
			KSetLastError(pCurThread,ERROR_BAD_EXE_FORMAT);
			return 0;
		}
		if (HasBreadcrumb(pMod))
			return pMod;
		retval = pMod;
		SetBreadcrumb(pMod);
		if (!special) {
			DEBUGMSG(ZONE_LOADER1,(TEXT("DLL %s Loaded, copying regions for %8.8lx...\r\n"),
				pMod->lpszModName,pCurProc));
			if (!(HasModRefProcPtr(pMod,pCurProc))) {
				if (!CopyRegions(pMod)) {
					retval = 0;
					if (!pMod->inuse) {
						KSetLastError(pCurThread,ERROR_OUTOFMEMORY);
						goto lolerr;
					}
				}
			}
		}
		inRom = (pMod->oe.filetype == FT_ROMIMAGE) ? 1 : 0;
		if (!bAllowPaging && (pMod->oe.pagemode == PM_FULLPAGING))
			pMod->oe.pagemode = PM_NOPAGEOUT;
	} else {
		if (!(pMod = AllocMem(HEAP_MODULE))) {
			KSetLastError(pCurThread,ERROR_OUTOFMEMORY);
			goto lolerr;
		}
		pMod->startip = 0;
		pMod->lpSelf = pMod;
		pMod->calledfunc = 0;
		pMod->breadcrumb = 0;
		SetBreadcrumb(pMod);
		pMod->oe.filetype = 0;
		pMod->ZonePtr = 0;
		pMod->inuse = 0;
		pMod->DbgFlags = 0;
		pMod->BasePtr = 0;
		pMod->o32_ptr = 0;
		pMod->lpszModName = 0;
		pMod->dwNoNotify = 0;
		pMod->wFlags = wFlags;
		pMod->pmodResource = 0;
		pMod->bTrustLevel = KERN_TRUST_FULL;
		if (pCurProc == &ProcArray[0]) {
			if (!(bExtended = OpenADll(lpszFileName,&pMod->oe,bAllowPaging))) {
				KSetLastError(pCurThread,ERROR_MOD_NOT_FOUND);
				goto lolerr;
			}
		} else {
			cbi.hProc = ProcArray[0].hProc;
			cbi.pfn = (FARPROC)OpenADll;
			cbi.pvArg0 = MapPtr(lpszFileName);
			if (!(bExtended = PerformCallBack4Int(&cbi,MapPtr(&pMod->oe),bAllowPaging))) {
				KSetLastError(pCurThread,ERROR_MOD_NOT_FOUND);
				goto lolerr;
			}
		}
		if (bExtended == 2)
			bExtended = 0;
		memset(&pMod->refcnt[0],0,sizeof(pMod->refcnt[0])*MAX_PROCESSES);
		eptr = &pMod->e32;
		if (loop = LoadE32(&pMod->oe,eptr,&flags,&entry,(pMod->wFlags & LOAD_LIBRARY_AS_DATAFILE) ? 1 : 0, bAllowPaging,&pMod->bTrustLevel)) {
			KSetLastError(pCurThread,loop);
			goto lolerr;
		}
		if (!eptr->e32_sect14rva && (loop = VerifyBinary(&pMod->oe,lpszFileName,&pMod->bTrustLevel))) {
			KSetLastError(pCurThread,loop);
			goto lolerr;
		}
		if ((pCurProc->bTrustLevel == KERN_TRUST_FULL) && (pMod->bTrustLevel == KERN_TRUST_RUN)) {
			KSetLastError(pCurThread,(DWORD)NTE_BAD_SIGNATURE);
			goto lolerr;
		}
		if (!(pMod->wFlags & LOAD_LIBRARY_AS_DATAFILE) && !(flags & IMAGE_FILE_DLL)) {
			KSetLastError(pCurThread,ERROR_BAD_EXE_FORMAT);
			goto lolerr;
		}
		if (!(pMod->wFlags & LOAD_LIBRARY_AS_DATAFILE) && (flags & IMAGE_FILE_RELOCS_STRIPPED)) {
			KSetLastError(pCurThread,ERROR_BAD_EXE_FORMAT);
			DEBUGMSG(ZONE_LOADER1,(TEXT("No relocations - can't load/fixup!\r\n")));
			goto lolerr;
		}
		if (pMod->oe.filetype != FT_ROMIMAGE) {
		    if (!(pMod->BasePtr = VirtualAlloc((LPVOID)ProcArray[0].dwVMBase,eptr->e32_vsize,
		    	MEM_RESERVE|MEM_TOP_DOWN|MEM_IMAGE,PAGE_NOACCESS))) {
				KSetLastError(pCurThread,ERROR_OUTOFMEMORY);
				goto lolerr;
			}
			if (ZeroPtr(pMod->BasePtr) < (DWORD)DllLoadBase)
				DllLoadBase = ZeroPtr(pMod->BasePtr);
		} else
			pMod->BasePtr = (LPVOID)((DWORD)((e32_rom *)(pMod->oe.tocptr->ulE32Offset))->e32_vbase +
				ProcArray[0].dwVMBase);
		pMod->o32_ptr = VirtualAlloc((LPVOID)ProcArray[0].dwVMBase,eptr->e32_objcnt*sizeof(o32_lite),
			MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
		if (!(pMod->o32_ptr = MapPtrProc(pMod->o32_ptr,pCurProc))) {
			KSetLastError(pCurThread,ERROR_OUTOFMEMORY);
			goto lolerr;
		}
		DEBUGMSG(ZONE_LOADER1,(TEXT("BasePtr is %8.8lx\r\n"),pMod->BasePtr));

		if (dwretval = LoadO32(&pMod->oe,eptr,pMod->o32_ptr,(ulong)pMod->BasePtr)) {
			KSetLastError(pCurThread,dwretval);
			goto lolerr;
		}

		// Make pMod the first element of the list
		pMod->lpszModName = L"unknown.dll";
		EnterCriticalSection(&ModListcs);
		pMod->pMod = pModList;
		pModList = pMod;
		LeaveCriticalSection(&ModListcs);
		len = (strlenW(dllname) + 1)*sizeof(WCHAR);
		if (bExtended)
			len += 8;
		for (loop = 0; loop < eptr->e32_objcnt; loop++)
			if (!(pMod->o32_ptr[loop].o32_flags & IMAGE_SCN_MEM_DISCARDABLE) &&
				(pMod->o32_ptr[loop].o32_flags & IMAGE_SCN_MEM_WRITE)) {
				DWORD base, size;
				base = pMod->o32_ptr[loop].o32_realaddr;
				size = pMod->o32_ptr[loop].o32_vsize;
				size = (size + 3) & ~3;
				if ((size % PAGE_SIZE) && ((PAGE_SIZE - (size % PAGE_SIZE)) >= len)) {
					pMod->lpszModName = (LPWSTR)MapPtrProc(ZeroPtr(base+size),&ProcArray[0]);
					pMod->o32_ptr[loop].o32_vsize = size + len;
					break;
				}
			}
		if (loop == eptr->e32_objcnt)
			pMod->lpszModName = VirtualAlloc((LPVOID)ProcArray[0].dwVMBase,len,
				MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
		if (bExtended)
			len -= 8;
		for (loop = 0; loop < len/sizeof(WCHAR); loop++)
			pMod->lpszModName[loop] = lowerW(dllname[loop]);
		if (bExtended)
			memcpy((LPBYTE)pMod->lpszModName+len-2,L".dll",10);
		for (loop = 0; loop < eptr->e32_objcnt; loop++)
			if (!(pMod->o32_ptr[loop].o32_flags & IMAGE_SCN_MEM_DISCARDABLE) &&
				(pMod->o32_ptr[loop].o32_flags & IMAGE_SCN_MEM_WRITE)) {
				DWORD base, size;
				base = pMod->o32_ptr[loop].o32_realaddr;
				size = pMod->o32_ptr[loop].o32_vsize;
				size = (size + 3) & ~3;
				if ((size % PAGE_SIZE) && ((PAGE_SIZE - (size % PAGE_SIZE)) >= eptr->e32_objcnt*sizeof(o32_lite))) {
					memcpy(MapPtrProc(ZeroPtr((LPBYTE)base+size),&ProcArray[0]),pMod->o32_ptr,eptr->e32_objcnt*sizeof(o32_lite));
					VirtualFree(pMod->o32_ptr,0,MEM_RELEASE);
					pMod->o32_ptr = (o32_lite *)MapPtrProc(ZeroPtr(base+size),&ProcArray[0]);
					pMod->o32_ptr[loop].o32_vsize = size + eptr->e32_objcnt*sizeof(o32_lite);
					break;
				}
			}
		if (pMod->oe.filetype != FT_ROMIMAGE) {
			if ((pMod->oe.pagemode == PM_NOPAGING) && !(pMod->wFlags & LOAD_LIBRARY_AS_DATAFILE) && !Relocate(eptr, pMod->o32_ptr, (ulong)pMod->BasePtr, (special ? 0 : ProcArray[0].dwVMBase))) {
				KSetLastError(pCurThread,ERROR_OUTOFMEMORY);
				goto lolerr;
			}
			inRom = 0;
		} else
			inRom = 1;
		pMod->startip = 0xffffffff;
		if (!(pMod->wFlags & DONT_RESOLVE_DLL_REFERENCES) && (loop = DoImports(eptr, pMod->o32_ptr, (ulong)pMod->BasePtr, ProcArray[0].dwVMBase))) {
			KSetLastError(pCurThread,loop);
			goto lolerr;
		}
		if (!special && !CopyRegions(pMod)) {
			retval = 0;
			if (!pMod->inuse) {
				KSetLastError(pCurThread,ERROR_OUTOFMEMORY);
				goto lolerr;
			}
		}
		if ((pMod->oe.pagemode == PM_NOPAGING) && !AdjustRegions(eptr, pMod->o32_ptr)) {
			KSetLastError(pCurThread,ERROR_OUTOFMEMORY);
			goto lolerr;
		}
		if (entry) {
			if (special)
				pMod->startip = FindEntryPoint(entry,&pMod->e32,pMod->o32_ptr);
			else {
				if (pMod->e32.e32_sect14rva) {
					HANDLE hLib;
					if (!(hLib = LoadOneLibraryW(L"mscoree.dll",1,0))) {
						KSetLastError(pCurThread,ERROR_DLL_INIT_FAILED);
						goto lolerr;
					}
					if (!(pMod->startip = (DWORD)GetProcAddressA(hLib,"_CorDllMain"))) {
						KSetLastError(pCurThread,ERROR_DLL_INIT_FAILED);
						goto lolerr;
					}
				} else
					pMod->startip = ZeroPtr(FindEntryPoint(entry,&pMod->e32,pMod->o32_ptr));

			}
			DEBUGMSG(ZONE_LOADER1,(TEXT("DLL Startup at %8.8lx\r\n"),pMod->startip));
		} else
			pMod->startip = 0;
		retval = pMod;
		DEBUGMSG(ZONE_LOADER1,(TEXT("DLL %s Loaded, copying regions for %8.8lx...\r\n"),
			pMod->lpszModName,pCurProc));
	}
	if (retval && !IncRefCount(pMod)) {
		if (!(pMod->DbgFlags & DBG_SYMBOLS_LOADED))
			KDUpdateSymbols(((DWORD)pMod->BasePtr)+1, FALSE);
		if (special) {
			pMod->inuse |= 1;
			pMod->refcnt[0]++;
			pMod->DbgFlags |= DBG_IS_DEBUGGER;
		} else if (!(pMod->wFlags & DONT_RESOLVE_DLL_REFERENCES) && !DoCopyDepends(pMod)) {
			DEBUGMSG(1,(TEXT("LoadOneLibrary CopyDepends failure for %s!\r\n"),lpszFileName));
			FreeOneLibrary(pMod, 0);
			KSetLastError(pCurThread,ERROR_OUTOFMEMORY);
			retval = 0;
		}
       	if (retval && inRom && SystemAPISets[SH_PATCHER]
       	        && !PatchDll(pCurProc, pMod, lpszFileName)) {
			DEBUGMSG(1,(TEXT("LoadOneLibrary Patcher failure for %s!\r\n"),lpszFileName));
			FreeOneLibrary(pMod, 0);
			retval = 0;
       	}
		if (retval && !CallDLLEntry(pMod,DLL_PROCESS_ATTACH,0)) {
			DEBUGMSG(1,(TEXT("LoadOneLibrary libentry failure for %s!\r\n"),lpszFileName));
			FreeOneLibrary(pMod, 0);
			KSetLastError(pCurThread,ERROR_DLL_INIT_FAILED);
			retval = 0;
		}
		if (retval && pCurThread->pThrdDbg && ProcStarted(pCurProc) && pCurThread->pThrdDbg->hEvent) {
			pCurThread->pThrdDbg->dbginfo.dwProcessId = (DWORD)hCurProc;
			pCurThread->pThrdDbg->dbginfo.dwThreadId = (DWORD)hCurThread;
			pCurThread->pThrdDbg->dbginfo.dwDebugEventCode = LOAD_DLL_DEBUG_EVENT;
			pCurThread->pThrdDbg->dbginfo.u.LoadDll.hFile = NULL;
			pCurThread->pThrdDbg->dbginfo.u.LoadDll.lpBaseOfDll = (LPVOID)ZeroPtr(pMod->BasePtr);
			pCurThread->pThrdDbg->dbginfo.u.LoadDll.dwDebugInfoFileOffset = 0;
			pCurThread->pThrdDbg->dbginfo.u.LoadDll.nDebugInfoSize = 0;
			pCurThread->pThrdDbg->dbginfo.u.LoadDll.lpImageName = pMod->lpszModName;
			pCurThread->pThrdDbg->dbginfo.u.LoadDll.fUnicode = 1;
			SetEvent(pCurThread->pThrdDbg->hEvent);
			SC_WaitForMultiple(1,&pCurThread->pThrdDbg->hBlockEvent,FALSE,INFINITE);
		}
	} else if (retval && !(pMod->wFlags & DONT_RESOLVE_DLL_REFERENCES) && !DoCopyDepends(pMod)) {
		DEBUGMSG(1,(TEXT("LoadOneLibrary CopyDepends failure for %s!\r\n"),lpszFileName));
		FreeOneLibrary(pMod, 0);
		KSetLastError(pCurThread,ERROR_OUTOFMEMORY);
		retval = 0;
	}
   CELOG_ModuleLoad(pCurProc->hProc, (HANDLE)retval, lpszFileName, (DWORD)pMod->BasePtr);
   return retval;
lolerr:
	lasterr = pCurThread->dwLastError;
	DEBUGMSG(1,(TEXT("LoadOneLibrary failure on %s (error 0x%8.8lx)!\r\n"),lpszFileName,lasterr));
	if (pMod) {
		CloseExe(&pMod->oe);
		if (pMod->startip == 0xffffffff)
			VirtualFree((LPVOID)(ZeroPtr(pMod->BasePtr) + pCurProc->dwVMBase), pMod->e32.e32_vsize,MEM_DECOMMIT);
		VirtualFree(pMod->BasePtr,pMod->e32.e32_vsize,MEM_DECOMMIT);
		if (pMod->BasePtr && (ZeroPtr(pMod->BasePtr) < pTOC->dllfirst)) {
			if (pMod->startip == 0xffffffff)
				VirtualFree((LPVOID)(ZeroPtr(pMod->BasePtr) + pCurProc->dwVMBase), 0, MEM_RELEASE);
			VirtualFree(pMod->BasePtr,0,MEM_RELEASE);
		}
		if (pMod->o32_ptr && ((ZeroPtr(pMod->BasePtr) > ZeroPtr(pMod->o32_ptr)) ||
			(ZeroPtr(pMod->BasePtr) + pMod->e32.e32_vsize <= (DWORD)pMod->o32_ptr)))
			VirtualFree(pMod->o32_ptr,0,MEM_RELEASE);
		UnlinkModule(pMod);
		FreeMem(pMod,HEAP_MODULE);
	}
	KSetLastError(pCurThread,lasterr);
	return 0;
}

HANDLE LoadOneLibraryW(LPCWSTR lpszFileName, BOOL bAllowPaging, WORD wFlags) {
	HANDLE retval = 0;
   if (!InitBreadcrumbs())
		KSetLastError(pCurThread,ERROR_OUTOFMEMORY);
	else {
		retval = (HANDLE)LoadOneLibraryPart2((LPWSTR)lpszFileName,bAllowPaging, wFlags);
		FinishBreadcrumbs();
	}
	return retval;
}

// Win32 LoadLibrary call

HANDLE SC_LoadLibraryW(LPCWSTR lpszFileName) {
	HANDLE retval;
	DEBUGMSG(ZONE_ENTRY,(L"SC_LoadLibraryW entry: %8.8lx\r\n",lpszFileName));
	EnterCriticalSection(&LLcs);
	retval = LoadOneLibraryW(lpszFileName,1,0);
	LeaveCriticalSection(&LLcs);
	DEBUGMSG(ZONE_ENTRY,(L"SC_LoadLibraryW exit: %8.8lx\r\n",retval));
	return retval;
}

HINSTANCE SC_LoadLibraryExW(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags) {
	HANDLE retval;
	DEBUGMSG(ZONE_ENTRY,(L"SC_LoadLibraryExW entry: %8.8lx %8.8lx %8.8lx\r\n",lpLibFileName, hFile, dwFlags));
	if (hFile || (dwFlags & ~(LOAD_LIBRARY_AS_DATAFILE|DONT_RESOLVE_DLL_REFERENCES))) {
		KSetLastError(pCurThread,ERROR_INVALID_PARAMETER);
		retval = 0;
	} else {
		EnterCriticalSection(&LLcs);
		DEBUGCHK(!(dwFlags & 0xffff0000));
		retval = LoadOneLibraryW(lpLibFileName,1,(WORD)dwFlags);
		LeaveCriticalSection(&LLcs);
	}
	DEBUGMSG(ZONE_ENTRY,(L"SC_LoadLibraryExW exit: %8.8lx\r\n",retval));
	return retval;
}

HMODULE SC_GetModuleHandleW(LPCWSTR lpModuleName) {
	HMODULE retval;
	LPCWSTR dllname;
	if (!lpModuleName)
		return pCurThread->pOwnerProc->hProc;
	EnterCriticalSection(&LLcs);
	dllname = lpModuleName+strlenW(lpModuleName);
	while ((dllname != lpModuleName) && (*(dllname-1) != (WCHAR)'\\') && (*(dllname-1) != (WCHAR)'/'))
		dllname--;
	retval = (HMODULE)FindModByName(dllname);
	if (retval && !HasModRefProcPtr((PMODULE)retval,pCurProc))
		retval = 0;
	LeaveCriticalSection(&LLcs);
	return retval;
}	

HANDLE SC_LoadDriver(LPCWSTR lpszFileName) {
	HANDLE retval;
	DEBUGMSG(ZONE_ENTRY,(L"SC_LoadDriver entry: %8.8lx\r\n",lpszFileName));
	EnterCriticalSection(&LLcs);
	retval = LoadOneLibraryW(lpszFileName,0,0);
	LeaveCriticalSection(&LLcs);
	DEBUGMSG(ZONE_ENTRY,(L"SC_LoadDriver exit: %8.8lx\r\n",retval));
	return retval;
}

// Win32 FreeLibrary call

BOOL SC_FreeLibrary(HANDLE hInst) {
	BOOL retval;
	PMODULE pMod;
	DEBUGMSG(ZONE_ENTRY,(L"SC_FreeLibrary entry: %8.8lx\r\n",hInst));
	if (!(pMod = (PMODULE)hInst))
	    return FALSE;
	EnterCriticalSection(&LLcs);
	retval = FreeOneLibrary((PMODULE)hInst, 1);
	LeaveCriticalSection(&LLcs);
	DEBUGMSG(ZONE_ENTRY,(L"SC_FreeLibrary exit: %8.8lx\r\n",retval));
	return retval;
}

#define MAXIMPNAME 128

// GetProcAddress, no critical section protection

LPVOID GetOneProcAddress(HANDLE hInst, LPCVOID lpszProc, BOOL bUnicode) {
	PMODULE pMod;
	DWORD retval;
	CHAR lpszImpName[MAXIMPNAME];
	pMod = (PMODULE)hInst;
	if (!IsValidModule(pMod)) {
		KSetLastError(pCurThread,ERROR_INVALID_HANDLE);
		retval = 0;
	} else {
		if ((DWORD)lpszProc>>16) {
			if (bUnicode) {
				KUnicodeToAscii(lpszImpName,lpszProc,MAXIMPNAME);
				retval = ResolveImpStr(pMod,lpszImpName);
			} else
				retval = ResolveImpStr(pMod,lpszProc);
		} else
			retval = ResolveImpOrd(pMod,(DWORD)lpszProc);
		if (!retval)
			KSetLastError(pCurThread,ERROR_INVALID_PARAMETER);
	}
	return (LPVOID)retval;
}

BOOL DisableThreadCalls(PMODULE pMod) {
	BOOL retval;
	KCALLPROFON(6);
	if (IsValidModule(pMod)) {
		pMod->dwNoNotify |= (1 << pCurProc->procnum);
		retval = TRUE;
	} else {
		KSetLastError(pCurThread,ERROR_INVALID_HANDLE);
		retval = FALSE;
	}
	KCALLPROFOFF(6);
	return retval;
}

BOOL SC_DisableThreadLibraryCalls(PMODULE hLibModule) {
	return KCall((PKFN)DisableThreadCalls,hLibModule);
}

// Win32 GetProcAddress call

LPVOID SC_GetProcAddressA(HANDLE hInst, LPCSTR lpszProc) {
	LPVOID retval = 0;
	DEBUGMSG(ZONE_ENTRY,(L"SC_GetProcAddressA entry: %8.8lx %8.8lx\r\n",hInst,lpszProc));
	__try {
		retval = GetOneProcAddress(hInst,lpszProc, FALSE);
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		KSetLastError(pCurThread,ERROR_INVALID_HANDLE);
	}
	DEBUGMSG(ZONE_LOADER1,(TEXT("SC_GetProcAddressA %8.8lx %8.8lx/%a -> %8.8lx\r\n"),hInst,lpszProc,(DWORD)lpszProc>0xffff ? lpszProc : "<ordinal>",retval));
	DEBUGMSG(ZONE_ENTRY,(L"SC_GetProcAddressA exit: %8.8lx\r\n",retval));
	return retval;
}

LPVOID SC_GetProcAddressW(HANDLE hInst, LPWSTR lpszProc) {
	LPVOID retval = 0;
	DEBUGMSG(ZONE_ENTRY,(L"SC_GetProcAddressW entry: %8.8lx %8.8lx\r\n",hInst,lpszProc));
	__try {
		retval = GetOneProcAddress(hInst,lpszProc, TRUE);
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		KSetLastError(pCurThread,ERROR_INVALID_HANDLE);
	}
	DEBUGMSG(ZONE_LOADER1,(TEXT("SC_GetProcAddressW %8.8lx %s -> %8.8lx\r\n"),hInst,(DWORD)lpszProc>0xffff ? lpszProc : L"<ordinal>",retval));
	DEBUGMSG(ZONE_ENTRY,(L"SC_GetProcAddressW exit: %8.8lx\r\n",retval));
	return retval;
}

#ifndef x86

typedef struct {
	void *			func;
	unsigned int	prologLen	: 8;
	unsigned int	funcLen		: 22;
	unsigned int	is32Bit		: 1;
	unsigned int	hasHandler	: 1;
} pdata_t;

typedef struct _pdata_tag {
	struct _pdata_tag *pNextTable;
	int numentries;	// number of entries below
	pdata_t entry[];	// entries in-lined like in a .pdata section
} XPDATA, *LPXPDATA;

BOOL MatchesExtRange(ULONG pc, LPXPDATA pPData) {
	ULONG start, end;
	if (pPData->numentries) {
		start = (ULONG)pPData->entry[0].func;
		end = (ULONG)pPData->entry[pPData->numentries-1].func + pPData->entry[pPData->numentries-1].funcLen;
		if ((pc >= start) && (pc < end))
			return TRUE;
		pc = ZeroPtr(pc);
		if ((pc >= start) && (pc < end))
			return TRUE;
	}
	return FALSE;
}

PVOID PDataFromExt(ULONG pc, PPROCESS pProc, PULONG pSize) {
	LPXPDATA pPData;
	PVOID pResult = 0;
	if (pProc->pExtPdata) {
	    __try {
			pPData = *(LPXPDATA *)pProc->pExtPdata;
			while (pPData) {
				if (MatchesExtRange(pc,pPData)) {
					pResult = &pPData->entry[0];
					*pSize = pPData->numentries * sizeof(pPData->entry[0]);
					break;
				}
				pPData = pPData->pNextTable;
			}
		} __except (EXCEPTION_EXECUTE_HANDLER) {
    	}
	}
	return pResult;
}

PVOID PDataFromPC(ULONG pc, PPROCESS pProc, PULONG pSize) {
	ULONG mappc, unmappc;
	PMODULE pMod;
	PVOID retval = 0;
	mappc = (ULONG)MapPtrProc(pc,pProc);
	unmappc = ZeroPtr(pc);
	*pSize = 0;
	if ((mappc >= (ULONG)ProcArray[0].e32.e32_vbase) && (mappc < (ULONG)ProcArray[0].BasePtr + (ULONG)ProcArray[0].e32.e32_vsize)) {
		*pSize = ProcArray[0].e32.e32_unit[EXC].size;
		retval = (PVOID)((ULONG)ProcArray[0].BasePtr + ProcArray[0].e32.e32_unit[EXC].rva);
	} else if ((mappc >= (ULONG)MapPtrProc(pProc->BasePtr,pProc)) &&
		(mappc < (ULONG)MapPtrProc(pProc->BasePtr,pProc)+pProc->e32.e32_vsize)) {
		*pSize = pProc->e32.e32_unit[EXC].size;
		retval = MapPtrProc((ULONG)pProc->BasePtr+pProc->e32.e32_unit[EXC].rva,pProc);
	} else if (!(retval = PDataFromExt(mappc,pProc,pSize))) {
		EnterCriticalSection(&ModListcs);
		pMod = pModList;
		while (pMod) {
			if ((unmappc >= ZeroPtr(pMod->BasePtr)) &&
				(unmappc < ZeroPtr(pMod->BasePtr)+pMod->e32.e32_vsize)) {
				*pSize = pMod->e32.e32_unit[EXC].size;
				retval = (PVOID)(ProcArray[0].dwVMBase + ZeroPtr((ULONG)pMod->BasePtr+pMod->e32.e32_unit[EXC].rva));
				break;
			}
			pMod = pMod->pMod;
		}
		LeaveCriticalSection(&ModListcs);
	}
	return retval;
}

#endif

int LoaderPageIn(BOOL bWrite, DWORD addr) {
	BOOL retval = 0;
	PMODULE pMod = 0;
	DWORD zaddr, low, high;
	int loop;
	MEMORY_BASIC_INFORMATION mbi;
	addr = (DWORD)MapPtr(addr);
	zaddr = ZeroPtr(addr);
	if (zaddr >= (DWORD)DllLoadBase) {
		EnterCriticalSection(&ModListcs);
		pMod = pModList;
		while (pMod) {
			low = ZeroPtr(pMod->BasePtr);
			high = low + pMod->e32.e32_vsize;
			if ((zaddr >= low) && (zaddr < high)) {
				break;
			}
			pMod = pMod->pMod;
		}
		LeaveCriticalSection(&ModListcs);
		if (!pMod) {
			DEBUGMSG(ZONE_PAGING,(L"    DLL slot not found!\r\n"));
			return retval;
		}
	} else {
		for (loop = 0; loop < MAX_PROCESSES; loop++) {
			if ((addr >= ProcArray[loop].dwVMBase) && (addr < ProcArray[loop].dwVMBase + (1<<VA_SECTION))) {
				break;
			}
		}
		if (loop == MAX_PROCESSES) {
			DEBUGMSG(ZONE_PAGING,(L"    Proc slot not found!\r\n"));
			return retval;
		}
	}
	EnterCriticalSection(&PagerCS);
	DEBUGMSG(ZONE_PAGING,(L"LPI: %8.8lx (%d)\r\n",addr,bWrite));
	VirtualQuery((LPVOID)addr,&mbi,sizeof(mbi));
	if (mbi.State == MEM_COMMIT) {
		if ((mbi.Protect == PAGE_READWRITE) || (mbi.Protect == PAGE_EXECUTE_READWRITE) ||
			(!bWrite && ((mbi.Protect == PAGE_READONLY) || (mbi.Protect == PAGE_EXECUTE_READ))))
			retval = 1;
		DEBUGMSG(ZONE_PAGING,(L"LPI: %8.8lx (%d) -> %d (1) [%x]\r\n",addr,bWrite,retval,mbi.Protect));
		LeaveCriticalSection(&PagerCS);
		return retval;
	}
	if (pMod)
		retval = PageInModule(pMod, addr);
	else
		retval = PageInProcess(&ProcArray[loop],zaddr);
	DEBUGMSG(ZONE_PAGING,(L"LPI: %8.8lx (%d) -> %d (2)\r\n",addr,bWrite,retval));
	LeaveCriticalSection(&PagerCS);
	return retval;
}

void PageOutProc(void) {
	int loop;
	DEBUGMSG(ZONE_PAGING,(L"POP: Starting page free count: %d\r\n",PageFreeCount));
	for (loop = 0; loop < MAX_PROCESSES; loop++)
		if (ProcArray[loop].dwVMBase && ProcStarted(&ProcArray[loop])) {
            __try {
                UnloadExe(&ProcArray[loop]);
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                // Pretend it didn't happen
                RETAILMSG(1, (TEXT("Exception in UnloadExe... continuing to next process\r\n")));
            }
        }
	DEBUGMSG(ZONE_PAGING,(L"POP: Ending page free count: %d\r\n",PageFreeCount));
}

extern long PageOutNeeded;

void PageOutMod(void) {
    static int storedModNum;
    int modCount;
    PMODULE pMod;
    EnterCriticalSection(&ModListcs);
    DEBUGMSG(ZONE_PAGING,(L"POM: Starting page free count: %d\r\n",PageFreeCount));
	for (pMod = pModList, modCount = 0; pMod && (modCount < storedModNum); pMod = pMod->pMod, modCount++)
		;
    while (pMod && PageOutNeeded) {
        __try {
            UnloadMod(pMod);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            // Pretend it didn't happen
            RETAILMSG(1, (TEXT("Exception in UnloadMod... continuing to next module\r\n")));
        }
		modCount++;
		pMod = pMod->pMod;
	}
    if (PageOutNeeded)
    	for (pMod = pModList, modCount = 0; pMod && PageOutNeeded && (modCount < storedModNum); pMod = pMod->pMod, modCount++)
          UnloadMod(pMod);
    storedModNum = modCount;
    DEBUGMSG(ZONE_PAGING,(L"POM: Ending page free count: %d\r\n",PageFreeCount));
    LeaveCriticalSection(&ModListcs);
}

