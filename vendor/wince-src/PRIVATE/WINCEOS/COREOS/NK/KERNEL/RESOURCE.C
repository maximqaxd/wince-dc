/*
 *				NK Kernel resource loader code
 *
 *				Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.
 *
 * Module Name:
 *
 *				heap.c
 *
 * Abstract:
 *
 *				This file implements the NK kernel resource loader
 *
 *
 */

/* Straight-forward implementation of the resouce API's.  Since resources are
   read-only, we always return the slot 1 (kernel) copy of the resource, which
   is visable to all processes. */

#include "kernel.h"

#define DWORDUP(x) (((x)+3)&~03)

extern CRITICAL_SECTION LLcs, RFBcs, ModListcs;

typedef struct VERHEAD {
	WORD wTotLen;
	WORD wValLen;
	WORD wType;			/* always 0 */
	WCHAR szKey[(sizeof("VS_VERSION_INFO")+3)&~03];
	VS_FIXEDFILEINFO vsf;
} VERHEAD ;

typedef struct resroot_t {
	DWORD flags;
	DWORD timestamp;
	DWORD version;
	WORD  numnameents;
	WORD  numidents;
} resroot_t;

typedef struct resent_t {
	DWORD id;
	DWORD rva;
} resent_t;

typedef struct resdata_t {
	DWORD rva;
	DWORD size;
	DWORD codepage;
	DWORD unused;
} resdata_t;

DWORD atoiW(LPWSTR str) {
	DWORD retval = 0;
	while (*str) {
		retval = retval * 10 + (*str-(WCHAR)'0');
		str++;
	}
	return retval;
}

extern WCHAR lowerW(WCHAR);
HANDLE FindResourcePart2(LPBYTE BasePtr, e32_lite* eptr, LPCWSTR lpszName, LPCWSTR lpszType);

// Compare a Unicode string and a Pascal string

int StrCmpPascW(LPWSTR ustr, LPWSTR pstr) {
	int loop = (int)*pstr++;
	while (*ustr && loop--)
		if (lowerW(*ustr++) != lowerW(*pstr++))
			return 1;
	return (!*ustr && !loop ? 0 : 1);
}

// Compare an Ascii string and a Pascal string

int StrCmpAPascW(LPSTR astr, LPWSTR pstr) {
	int loop = (int)*pstr++;
	while (*astr && loop--)
		if (lowerW((WCHAR)*astr++) != lowerW(*pstr++))
			return 1;
	return (!*astr && !loop ? 0 : 1);
}

// Search for a named type

DWORD FindTypeByName(LPBYTE BasePtr, LPBYTE curr, LPWSTR str) {
	resroot_t *rootptr = (resroot_t *)curr;
	resent_t *resptr = (resent_t *)(curr + sizeof(resroot_t));
	int count = rootptr->numnameents;
	while (count--) {
		if (!str || !StrCmpPascW(str,(LPWSTR)(BasePtr+(resptr->id&0x7fffffff))))
			return resptr->rva;
		resptr++;
	}
	return 0;
}

// Search for a numbered type

DWORD FindTypeByNum(LPBYTE curr, DWORD id) {
	resroot_t *rootptr = (resroot_t *)curr;
	resent_t *resptr = (resent_t *)(curr + sizeof(resroot_t) +
		rootptr->numnameents*sizeof(resent_t));
	int count = rootptr->numidents;
	while (count--) {
		if (!id || (resptr->id == id))
			return resptr->rva;
		resptr++;
	}
	return 0;
}

// Find first entry (from curr pointer)

DWORD FindFirst(LPBYTE curr) {
	resroot_t *rootptr = (resroot_t *)curr;
	resent_t *resptr = (resent_t *)(curr + sizeof(resroot_t) +
		rootptr->numnameents*sizeof(resent_t));
	return resptr->rva;
}

// Find type entry (by name or number)

DWORD FindType(LPBYTE BasePtr, LPBYTE curr, LPWSTR str) {
	DWORD res;
	if (((DWORD)str)>>16)
		if (str[0] == (WCHAR)'#')
			return FindTypeByNum(curr, atoiW(str+1));
		else
			return FindTypeByName(BasePtr, curr, str);
	if (!(res = FindTypeByNum(curr, (DWORD)str)) && !str)
		res = FindTypeByName(BasePtr, curr, str);
	return res;
}

// global array that keeps our current UI language, and default UI language
LANGID g_rgwUILangs[6];	// at most 5 langs, one extra for array terminator
// 0 == current UI lang
// 1 == MAKELANGID(PRIMARYLANGID(g_rgwUILangs[0]), SUBLANG_DEFAULT)
// 2 == default UI lang
// 3 == MAKELANGID(PRIMARYLANGID(g_rgwUILangs[2]), SUBLANG_DEFAULT)
// 4 == 0x0409 (US English) 
// The array may be smaller since duplicates are eliminated

// Flag that enables/disables MUI globally 0==not inited, 1==inited&enabled 2==inited&disabled
DWORD g_EnableMUI;

// saved path to system MUI DLL. sizeof(\\windows\\ceosXXXX.mui)
//WCHAR g_szSystemMUI[22];

// Read language setting & add uniquely to array
void AddLangUnique(DWORD dwValue, HKEY hkey, LPCTSTR szValName)
{
	int i;
	DWORD  dwType, dwSize;
	LANGID langid1, langid2;

	if(!dwValue)	{
		dwSize = sizeof(DWORD);
		// load a langid, skip if error
		if( ERROR_SUCCESS!=RegQueryValueExW(hkey, szValName, (LPDWORD)L"MUI", &dwType, (LPBYTE)&dwValue, &dwSize) ||
			(dwType != REG_DWORD) || !dwValue ) {
				DEBUGMSG(1,(L"InitMUI: Failed to read regkey %s\r\n", szValName));
				return;
		}
	}				
	langid1 = (LANGID)dwValue;
	langid2 = MAKELANGID(PRIMARYLANGID(langid1), SUBLANG_DEFAULT);
	if(langid2 == langid1)
		langid2 = 0;
	DEBUGMSG(1,(L"InitMUI: Got Langs=%x %x\r\n", langid1, langid2));
	
	// add them uniquely to array
	for(i=0; ;i++) {
		if(!g_rgwUILangs[i]) {
			g_rgwUILangs[i] = langid1;
			g_rgwUILangs[i+1] = langid2;
			break;
		} else if(g_rgwUILangs[i]==langid1) {
			langid1 = 0;
		} else if(g_rgwUILangs[i]==langid2) {
			langid2 = 0;
		}
	}
}

// Initialize MUI language globals -- called only once during system startup, from RunApps
void InitMUILanguages(void)
{
	DWORD  dwType, dwSize, dwValue;
	
	DEBUGCHK(!g_EnableMUI); // this must be called only once
	if(!SystemAPISets[SH_FILESYS_APIS]) {
		g_EnableMUI = (DWORD)-1;	// if this config has no filesystem then MUI is disabled
		return;
	}
	
	// check if MUI is enabled
	dwSize = sizeof(DWORD);
	if( ERROR_SUCCESS!=RegQueryValueExW(HKEY_LOCAL_MACHINE, L"Enable",(LPDWORD)L"MUI", &dwType, (LPBYTE)&dwValue, &dwSize) ||
		(dwType != REG_DWORD) || !dwValue ) {
			DEBUGMSG(1,(L"InitMUI: DISABLED (%d)\r\n", dwValue));
			g_EnableMUI = (DWORD)-1; // disable MUI
			return;
	}

	// memset(g_rgwUILangs, 0, sizeof(g_rgwUILangs)); // not reqd
	// load Current User language, skip if error
	AddLangUnique(0, HKEY_CURRENT_USER, L"CurLang");
	// load System Default language, skip if error
	AddLangUnique(0, HKEY_LOCAL_MACHINE, L"SysLang");
	// load English
	AddLangUnique(0x0409, 0, 0);
	
	DEBUGMSG(1,(L"InitMUI: Langs=%x %x %x %x %x %x\r\n", g_rgwUILangs[0], g_rgwUILangs[1], 
		g_rgwUILangs[2], g_rgwUILangs[3], g_rgwUILangs[4], g_rgwUILangs[5]));























	g_EnableMUI = 1;	// set this as the last thing before exit
}


// Called to load MUI resource DLL. NOT in a critsec. Must be re-entrant.
HMODULE LoadMUI(HANDLE hModule, LPBYTE BasePtr2, e32_lite* eptr2)
{
	WCHAR	szPath[MAX_PATH];
	int		iLen, i;
	HMODULE hmodRes;
	DWORD dwLangID;
	HANDLE hRsrc;

	// if MUI disabled (or not yet inited, e.g. call by filesys.exe), then fail immediately
	if(g_EnableMUI != 1)
		return (HMODULE)-1;

	// See if module contains a reference to a MUI (resource type=222, id=1). If found,
	// the resource contains a basename to which we append .XXXX.MUI, where XXXX is the language code
	if( (hRsrc = FindResourcePart2(BasePtr2, eptr2, MAKEINTRESOURCE(ID_MUI), MAKEINTRESOURCE(RT_MUI))) &&
		(iLen = ((resdata_t *)hRsrc)->size) &&
		(iLen < sizeof(WCHAR)*(MAX_PATH-10)) ) {
			memcpy(szPath, (BasePtr2 + ((resdata_t *)hRsrc)->rva), iLen);
			iLen /= sizeof(WCHAR);
			szPath[iLen]=0;
			DEBUGMSG(1,(L"LoadMUI: Found indirection (%s, %d)\r\n", szPath, iLen)); 
	} 
	// otherwise search for local MUI dll, based on module path name
	else  if(!(iLen=GetModuleFileName(hModule, szPath, MAX_PATH-10))) {
		DEBUGCHK(FALSE);
		return (HMODULE)-1;
	}

	for(i=0; g_rgwUILangs[i]; i++)
	{
		// try to find local DLL in each of current/default/english languages
		dwLangID = g_rgwUILangs[i];
		NKwvsprintfW(szPath+iLen, TEXT(".%04X.MUI"), (LPVOID)&dwLangID, (MAX_PATH-iLen));
		DEBUGMSG(1,(L"LoadMUI: Trying %s\r\n",szPath));
		if(hmodRes=LoadLibraryEx(szPath, NULL, LOAD_LIBRARY_AS_DATAFILE)) { // dont call dllentry/resolve refs etc
			DEBUGMSG(1,(L"LoadMUI: Loaded %s\r\n", szPath)); 
			return hmodRes;
		}
	}
	return (HMODULE)-1;










		
}	

HANDLE FindResourcePart2(LPBYTE BasePtr, e32_lite* eptr, LPCWSTR lpszName, LPCWSTR lpszType) {
	DWORD trva;
	if (!eptr->e32_unit[RES].rva) return 0;
	BasePtr += eptr->e32_unit[RES].rva;
	if (!(trva = FindType(BasePtr, BasePtr, (LPWSTR)lpszType)) || 
		!(trva = FindType(BasePtr, BasePtr+(trva&0x7fffffff),(LPWSTR)lpszName)) ||
		!(trva = FindFirst(BasePtr+(trva&0x7fffffff)))) {
			return 0;
	}
	return (HANDLE)(BasePtr+(trva&0x7fffffff));
}

// Gets pmodResource, atomically updating it if reqd
PMODULE InterlockedUpdatePmodRes(PMODULE* ppMod, HMODULE hModule, LPBYTE BasePtr2, e32_lite* eptr2)
{
	PMODULE pmodRes;
	pmodRes = (PMODULE)LoadMUI(hModule, BasePtr2, eptr2);
	if(InterlockedTestExchange((LPLONG)ppMod, 0, (LONG)pmodRes)!=0) {
		
		if(pmodRes != (PMODULE)-1) 
			FreeLibrary(pmodRes);
		pmodRes = *ppMod;
	}
	DEBUGCHK(pmodRes);
	return pmodRes;
}


// Win32 FindResource

HANDLE SC_FindResource(HANDLE hModule, LPCWSTR lpszName, LPCWSTR lpszType) {
	LPBYTE BasePtr2;
	e32_lite *eptr2;
	PPROCESS pProc;
	PMODULE pmodRes;
	HANDLE	hRet;
	
	DEBUGMSG(ZONE_ENTRY,(L"SC_FindResource entry: %8.8lx %8.8lx %8.8lx\r\n",hModule,lpszName,lpszType));
	if (hModule == GetCurrentProcess())
		hModule = hCurProc;
	// Get Base & e32 ptr for current process or module as approp
	// also get MUI resource dll, loading it if neccesary
	if (pProc = HandleToProc(hModule)) { // a process
		BasePtr2 = (LPBYTE)MapPtrProc(pProc->BasePtr,pProc);
		eptr2 = &pProc->e32;
		if(!(pmodRes = pProc->pmodResource))
			pmodRes = InterlockedUpdatePmodRes(&(pProc->pmodResource), hModule, BasePtr2, eptr2);
	} else if (IsValidModule((LPMODULE)hModule)) { // a module
		BasePtr2 = (LPBYTE)MapPtrProc(((PMODULE)hModule)->BasePtr,&ProcArray[0]);
		eptr2 = &(((PMODULE)hModule)->e32);
		if(!(pmodRes = ((PMODULE)hModule)->pmodResource))
			pmodRes = InterlockedUpdatePmodRes(&(((PMODULE)hModule)->pmodResource), hModule, BasePtr2, eptr2);
	} else {
		KSetLastError(pCurThread,ERROR_INVALID_HANDLE);
		DEBUGMSG(ZONE_ENTRY,(L"SC_FindResource exit: %8.8lx\r\n",0));
		return 0;
	}
	// If we have a MUI dll try to load from it first
	hRet = 0;
	if(pmodRes != (PMODULE)-1) {
		//DEBUGMSG(1,(L"SC_FindResource: Trying MUI %08x\r\n",pmodRes));
		DEBUGCHK(IsValidModule(pmodRes)); // at this point pmodRes is a valid PMODULE or -1
		hRet = FindResourcePart2((LPBYTE)MapPtrProc(pmodRes->BasePtr,&ProcArray[0]), &(pmodRes->e32), lpszName, lpszType);
	}
	// if no MUI or failed to find resource, try the module itself
	if(!hRet) {
		//DEBUGMSG(1,(L"SC_FindResource: Trying self\r\n"));
		hRet = FindResourcePart2(BasePtr2, eptr2, lpszName, lpszType);
	}
	if(!hRet) 
		KSetLastError(pCurThread,ERROR_RESOURCE_NAME_NOT_FOUND);
	DEBUGMSG(ZONE_ENTRY,(L"SC_FindResource exit: %8.8lx\r\n",hRet));
	return hRet;
}

// Win32 SizeofResource

DWORD SC_SizeofResource(HANDLE hModule, HANDLE hRsrc) {
	DEBUGMSG(ZONE_ENTRY,(L"SC_SizeofResource entry: %8.8lx %8.8lx\r\n",hModule,hRsrc));
	if (!hRsrc) {
		KSetLastError(pCurThread,ERROR_INVALID_HANDLE);
		DEBUGMSG(ZONE_ENTRY,(L"SC_SizeofResource exit: %8.8lx\r\n",0));
		return 0;
	}
	DEBUGMSG(ZONE_ENTRY,(L"SC_SizeofResource exit: %8.8lx\r\n",((resdata_t *)hRsrc)->size));
	return ((resdata_t *)hRsrc)->size;
}

// Win32 LoadResource

HANDLE SC_LoadResource(HANDLE hModule, HANDLE hRsrc) {
	DWORD pnum;
	PMODULE pMod;
	LPBYTE BasePtr;
	PPROCESS pProc;
	HANDLE h;
	DEBUGMSG(ZONE_ENTRY,(L"SC_LoadResource entry: %8.8lx %8.8lx\r\n",hModule,hRsrc));
	if (hModule == GetCurrentProcess())
		hModule = hCurProc;
	if (!hRsrc) {
		KSetLastError(pCurThread,ERROR_INVALID_HANDLE);
		DEBUGMSG(ZONE_ENTRY,(L"SC_LoadResource exit: %8.8lx\r\n",0));
		return 0;
	}
	if (!hModule) {
		pnum = ((DWORD)hRsrc >> VA_SECTION);
		if (pnum-- && ProcArray[pnum].dwVMBase) {
			if (!pnum) {
				EnterCriticalSection(&ModListcs);
				pMod = pModList;
				pnum = ZeroPtr(hRsrc);
				for (pMod = pModList; pMod; pMod = pMod->pMod)
					if ((pnum >= ZeroPtr(pMod->BasePtr)) && (pnum < ZeroPtr(pMod->BasePtr)+pMod->e32.e32_vsize))
						break;
				LeaveCriticalSection(&ModListcs);
				if (!pMod) {
					KSetLastError(pCurThread,ERROR_INVALID_HANDLE);
					DEBUGMSG(ZONE_ENTRY,(L"SC_LoadResource exit: %8.8lx\r\n",0));
					return 0;
				}
				BasePtr = MapPtrProc(pMod->BasePtr,&ProcArray[0]);
			} else
				BasePtr = MapPtrProc(ProcArray[pnum].BasePtr,&ProcArray[pnum]);
		} else {
			KSetLastError(pCurThread,ERROR_INVALID_HANDLE);
			DEBUGMSG(ZONE_ENTRY,(L"SC_LoadResource exit: %8.8lx\r\n",0));
			return 0;
		}
		pMod = 0;
	} else if (pProc = HandleToProc(hModule)) { // a process
		pMod = pProc->pmodResource;
		BasePtr = (LPBYTE)MapPtrProc(pProc->BasePtr,pProc);
	} else if (IsValidModule((LPMODULE)hModule)) { // a module
		pMod = ((PMODULE)hModule)->pmodResource;
		BasePtr = (LPBYTE)MapPtrProc(((PMODULE)hModule)->BasePtr,&ProcArray[0]);
	} else {
		KSetLastError(pCurThread,ERROR_INVALID_HANDLE);
		DEBUGMSG(ZONE_ENTRY,(L"SC_LoadResource exit: %8.8lx\r\n",0));
		return 0;
	}
	// check if it came out of the MUI resource dll
	if( pMod && (pMod != (PMODULE)-1) && 
		(ZeroPtr(hRsrc) >= ZeroPtr(pMod->BasePtr)) && 
		(ZeroPtr(hRsrc) < ZeroPtr(pMod->BasePtr)+pMod->e32.e32_vsize) )
			BasePtr = (LPBYTE)MapPtrProc(pMod->BasePtr,&ProcArray[0]);
	
	DEBUGMSG(ZONE_ENTRY,(L"SC_LoadResource exit: %8.8lx\r\n",BasePtr + ((resdata_t *)hRsrc)->rva));
	__try {
		h = (HANDLE)(BasePtr + ((resdata_t *)hRsrc)->rva);
	} __except(1) {
		h = 0;
		SetLastError(ERROR_INVALID_ADDRESS);
	}
	return h;
}

DWORD Decompress(LPBYTE BufIn, DWORD InSize, LPBYTE BufOut, DWORD OutSize, DWORD skip);

#pragma pack(2)
typedef struct {
	WORD idReserved;
	WORD idType;
	WORD idCount;
} ICONHEADER;

typedef struct ResourceDirectory {
	BYTE bWidth;
	BYTE bHeight;
	BYTE bColorCount;
	BYTE bReserved;
	WORD wPlanes;
	WORD wBitCount;
	DWORD dwBytesInRes;
	WORD wOrdinal;	   /* points to an RT_ICON resource */
} RESDIR;

typedef struct {
	ICONHEADER ih;
	RESDIR rgdir[1];  /* may really be > 1 */
} HEADER_AND_DIR, *PHEADER_AND_DIR;

LPBYTE OpenResourceFile(LPCWSTR lpszFile, BOOL *pInRom, DWORD *o32rva) {
	openexe_t oe;
	e32_lite e32l;
	o32_obj o32;
	o32_rom *o32rp;
	LPBYTE lpres;
	DWORD bytesread, loop;
	BOOL bRet;
	CALLBACKINFO cbi;
	OEinfo_t OEinfo;
	*pInRom = 0;
	__try {
		cbi.hProc = ProcArray[0].hProc;
		cbi.pfn = (FARPROC)SafeOpenExe;
		cbi.pvArg0 = MapPtr(lpszFile);
		bRet = PerformCallBack(&cbi,MapPtr(&oe),0,0,&OEinfo);
	} __except(EXCEPTION_EXECUTE_HANDLER) {
		bRet = FALSE;
	}
	if (!bRet) {
		KSetLastError(pCurThread,ERROR_RESOURCE_NAME_NOT_FOUND);
		DEBUGMSG(ZONE_ENTRY,(L"SC_ExtractResource exit: %8.8lx\r\n",0));
		return 0;
	}
	if (LoadE32(&oe,&e32l,0,0,1,1,0)) {
		CloseExe(&oe);
		KSetLastError(pCurThread,ERROR_RESOURCE_NAME_NOT_FOUND);
		DEBUGMSG(ZONE_ENTRY,(L"SC_ExtractResource exit: %8.8lx\r\n",0));
		return 0;
	}
	if (!e32l.e32_unit[RES].rva) {
		CloseExe(&oe);
		KSetLastError(pCurThread,ERROR_RESOURCE_NAME_NOT_FOUND);
		DEBUGMSG(ZONE_ENTRY,(L"SC_ExtractResource exit: %8.8lx\r\n",0));
		return 0;
	}
	if (oe.filetype == FT_PPFSFILE) {
		for (loop = 0; loop < e32l.e32_objcnt; loop++) {
			rlseek(oe.hppfs,oe.offset+sizeof(e32_exe)+loop*sizeof(o32_obj),0);
			rread(oe.hppfs,(LPBYTE)&o32,sizeof(o32_obj));
			if (o32.o32_rva == e32l.e32_unit[RES].rva)
				break;
		}
		if (loop == e32l.e32_objcnt) {
			CloseExe(&oe);
			KSetLastError(pCurThread,ERROR_RESOURCE_NAME_NOT_FOUND);
			DEBUGMSG(ZONE_ENTRY,(L"SC_ExtractResource exit: %8.8lx\r\n",0));
			return 0;
		}
		if (!(lpres = (LPBYTE)LocalAlloc(LMEM_FIXED,o32.o32_vsize))) {
			CloseExe(&oe);
			KSetLastError(pCurThread,ERROR_RESOURCE_NAME_NOT_FOUND);
			DEBUGMSG(ZONE_ENTRY,(L"SC_ExtractResource exit: %8.8lx\r\n",0));
			return 0;
		}
		rlseek(oe.hppfs,o32.o32_dataptr,0);
		rread(oe.hppfs,lpres,min(o32.o32_psize,o32.o32_vsize));
		*o32rva = o32.o32_rva;
	} else if (oe.filetype == FT_OBJSTORE) {
		for (loop = 0; loop < e32l.e32_objcnt; loop++) {
			SetFilePointer(oe.hf,oe.offset+sizeof(e32_exe)+loop*sizeof(o32_obj),0,FILE_BEGIN);
			ReadFile(oe.hf,(LPBYTE)&o32,sizeof(o32_obj),&bytesread,0);
			if (o32.o32_rva == e32l.e32_unit[RES].rva)
				break;
		}
		if (loop == e32l.e32_objcnt) {
			CloseExe(&oe);
			KSetLastError(pCurThread,ERROR_RESOURCE_NAME_NOT_FOUND);
			DEBUGMSG(ZONE_ENTRY,(L"SC_ExtractResource exit: %8.8lx\r\n",0));
			return 0;
		}
		if (!(lpres = (LPBYTE)LocalAlloc(LMEM_FIXED,o32.o32_vsize))) {
			CloseExe(&oe);
			KSetLastError(pCurThread,ERROR_RESOURCE_NAME_NOT_FOUND);
			DEBUGMSG(ZONE_ENTRY,(L"SC_ExtractResource exit: %8.8lx\r\n",0));
			return 0;
		}
		SetFilePointer(oe.hf,o32.o32_dataptr,0,FILE_BEGIN);
		ReadFile(oe.hf,lpres,min(o32.o32_psize,o32.o32_vsize),&bytesread,0);
		*o32rva = o32.o32_rva;
	} else {
		for (loop = 0; loop < e32l.e32_objcnt; loop++) {
			o32rp = (o32_rom *)(oe.tocptr->ulO32Offset+loop*sizeof(o32_rom));
			if (o32rp->o32_rva == e32l.e32_unit[RES].rva)
				break;
		}
		if (loop == e32l.e32_objcnt) {
			CloseExe(&oe);
			KSetLastError(pCurThread,ERROR_RESOURCE_NAME_NOT_FOUND);
			DEBUGMSG(ZONE_ENTRY,(L"SC_ExtractResource exit: %8.8lx\r\n",0));
			return 0;
		}
		if (o32rp->o32_psize) {
			if (o32rp->o32_flags & IMAGE_SCN_COMPRESSED) {
				if (!(lpres = (LPBYTE)LocalAlloc(LMEM_FIXED,o32rp->o32_vsize))) {
					CloseExe(&oe);
					KSetLastError(pCurThread,ERROR_RESOURCE_NAME_NOT_FOUND);
					DEBUGMSG(ZONE_ENTRY,(L"SC_ExtractResource exit: %8.8lx\r\n",0));
					return 0;
				}
				bytesread = Decompress((LPVOID)(o32rp->o32_dataptr),o32rp->o32_psize,lpres,o32rp->o32_vsize,0);
				if (bytesread < o32rp->o32_vsize)
					memset(lpres+bytesread,0,o32rp->o32_vsize-bytesread);
			} else {
				lpres = (LPVOID)(o32rp->o32_dataptr);
				*pInRom = 1;
			}
		}
		*o32rva = o32rp->o32_rva;
	}
	CloseExe(&oe);
	return lpres;
}

LPVOID ExtractOneResource(LPBYTE lpres, LPCWSTR lpszName, LPCWSTR lpszType, DWORD o32rva, LPDWORD pdwSize) {
	LPBYTE lpres2;
	DWORD trva;
	if (!(trva = FindType(lpres, lpres, (LPWSTR)lpszType)) ||
	   !(trva = FindType(lpres, lpres+(trva&0x7fffffff),(LPWSTR)lpszName)) ||
	   !(trva = FindFirst(lpres+(trva&0x7fffffff))) ||
	   (trva & 0x80000000)) {
		KSetLastError(pCurThread,ERROR_RESOURCE_NAME_NOT_FOUND);
		return 0;
	}
	trva = (ULONG)(lpres + trva);
	if (!(lpres2 = (LPBYTE)LocalAlloc(LMEM_FIXED,((resdata_t *)trva)->size))) {
		KSetLastError(pCurThread,ERROR_RESOURCE_NAME_NOT_FOUND);
		return 0;
	}
	

	if (pdwSize)
		*pdwSize = ((resdata_t *)trva)->size;
	memcpy(lpres2,lpres+((resdata_t *)trva)->rva-o32rva,((resdata_t *)trva)->size);
	return lpres2;
}

LPVOID SC_ExtractResource(LPCWSTR lpszFile, LPCWSTR lpszName, LPCWSTR lpszType) {
	LPBYTE lpres, lpres2;
	BOOL inRom;
	DWORD o32rva;
	DEBUGMSG(ZONE_ENTRY,(L"SC_ExtractResource entry: %8.8lx %8.8lx %8.8lx\r\n",lpszFile,lpszName,lpszType));
	if (!(lpres = OpenResourceFile(lpszFile, &inRom, &o32rva))) {
		DEBUGMSG(ZONE_ENTRY,(L"SC_ExtractResource exit: %8.8lx\r\n",0));
		return 0;
	}
	lpres2 = ExtractOneResource(lpres,lpszName,lpszType,o32rva,0);
	if (!inRom)
		LocalFree(lpres);
	DEBUGMSG(ZONE_ENTRY,(L"SC_ExtractResource exit: %8.8lx\r\n",lpres2));
	return lpres2;
}

typedef void (* KEICB_t)(LPVOID, PHEADER_AND_DIR,PWORD);

UINT SC_KernExtractIcons(LPCWSTR lpszFile, int nIconIndex, LPBYTE *pIconLarge, LPBYTE *pIconSmall, CALLBACKINFO *pcbi) {
	LPBYTE lpres;
	BOOL inRom;
	UINT nRet = 0;
	PHEADER_AND_DIR phd;
	DWORD o32rva;
	WORD index[2];
	if (!(lpres = OpenResourceFile(lpszFile, &inRom, &o32rva)))
		return 0;
	if (phd = (PHEADER_AND_DIR)ExtractOneResource(lpres, MAKEINTRESOURCE(nIconIndex), RT_GROUP_ICON, o32rva,0)) {
		if (phd->ih.idCount) {
			((KEICB_t)pcbi->pfn)(pcbi->pvArg0,phd,index);
			if (pIconLarge && (*pIconLarge = ExtractOneResource(lpres, MAKEINTRESOURCE(phd->rgdir[index[0]].wOrdinal), RT_ICON, o32rva,0)))
				nRet++;
			if (pIconSmall && (*pIconSmall = ExtractOneResource(lpres, MAKEINTRESOURCE(phd->rgdir[index[1]].wOrdinal), RT_ICON, o32rva,0)))
				nRet++;
		}
		LocalFree(phd);
	}
	if (!inRom)
		LocalFree(lpres);
	return nRet;
}

BOOL SC_VerQueryValueW(VERBLOCK *pBlock, LPWSTR lpSubBlock, LPVOID *lplpBuffer, PUINT puLen) {
	LPWSTR lpSubBlockOrg, lpEndBlock, lpEndSubBlock, lpStart;
	WCHAR cEndBlock, cTemp;
	BOOL bString;
	DWORD dwTotBlockLen, dwHeadLen;
	int nCmp;
	*puLen = 0;
	if ((int)pBlock->wTotLen < sizeof(VERBLOCK)) {
		KSetLastError(pCurThread,ERROR_INVALID_DATA);
		return FALSE;
	}
	if (!(lpSubBlockOrg = (LPWSTR)LocalAlloc(LPTR,(strlenW(lpSubBlock)+1)*sizeof(WCHAR)))) {
		KSetLastError(pCurThread,ERROR_NOT_ENOUGH_MEMORY);
		return FALSE;
	}
	kstrcpyW(lpSubBlockOrg,lpSubBlock);
	lpSubBlock = lpSubBlockOrg;
	lpEndBlock = (LPWSTR)((LPBYTE)pBlock + pBlock->wTotLen - sizeof(WCHAR));
	cEndBlock = *lpEndBlock;
	*lpEndBlock = 0;
	bString = FALSE;
	while (*lpSubBlock) {
		while (*lpSubBlock == L'\\')
			++lpSubBlock;
		if (*lpSubBlock) {
			dwTotBlockLen = (DWORD)((LPSTR)lpEndBlock - (LPSTR)pBlock + sizeof(WCHAR));
			if (((int)dwTotBlockLen < sizeof(VERBLOCK)) || (pBlock->wTotLen > (WORD)dwTotBlockLen))
				goto NotFound;
			dwHeadLen = DWORDUP(sizeof(VERBLOCK) - sizeof(WCHAR) + (strlenW(pBlock->szKey) + 1) * sizeof(WCHAR)) + DWORDUP(pBlock->wValLen);
			if (dwHeadLen > pBlock->wTotLen)
				goto NotFound;
			lpEndSubBlock = (LPWSTR)((LPBYTE)pBlock + pBlock->wTotLen);
			pBlock = (VERBLOCK*)((LPBYTE)pBlock+dwHeadLen);
			for (lpStart=lpSubBlock; *lpSubBlock && (*lpSubBlock!=L'\\'); lpSubBlock++)
				;
			cTemp = *lpSubBlock;
			*lpSubBlock = 0;
			nCmp = 1;
			while (((int)pBlock->wTotLen > sizeof(VERBLOCK)) && ((int)pBlock->wTotLen <= (LPBYTE)lpEndSubBlock-(LPBYTE)pBlock)) {
				if (!(nCmp=kstrcmpi(lpStart, pBlock->szKey)))
					break;
				pBlock=(VERBLOCK*)((LPBYTE)pBlock+DWORDUP(pBlock->wTotLen));
			}
			*lpSubBlock = cTemp;
			if (nCmp)
				goto NotFound;
		}
	}
	*puLen = pBlock->wValLen;
	lpStart = (LPWSTR)((LPBYTE)pBlock+DWORDUP((sizeof(VERBLOCK)-sizeof(WCHAR))+(strlenW(pBlock->szKey)+1)*sizeof(WCHAR)));
	*lplpBuffer = lpStart < (LPWSTR)((LPBYTE)pBlock+pBlock->wTotLen) ? lpStart : (LPWSTR)(pBlock->szKey+strlenW(pBlock->szKey));
	bString = pBlock->wType;
	*lpEndBlock = cEndBlock;
	LocalFree(lpSubBlockOrg);
	return (TRUE);
NotFound:
	KSetLastError(pCurThread,ERROR_RESOURCE_NAME_NOT_FOUND);
	*lpEndBlock = cEndBlock;
	LocalFree(lpSubBlockOrg);
	return (FALSE);
}

DWORD SC_GetFileVersionInfoSizeW(LPWSTR lpFilename, LPDWORD lpdwHandle) {
	LPBYTE lpres;
	BOOL inRom;
	DWORD o32rva, dwRet = 0;
	VERHEAD *pVerHead;
	if (lpdwHandle)
		*lpdwHandle = 0;
	DEBUGMSG(ZONE_ENTRY,(L"SC_GetFileVersionInfoSizeW entry: %8.8lx %8.8lx\r\n",lpFilename, lpdwHandle));
	if (lpres = OpenResourceFile(lpFilename, &inRom, &o32rva)) {
		if (pVerHead = (VERHEAD *)ExtractOneResource(lpres, MAKEINTRESOURCE(VS_VERSION_INFO), VS_FILE_INFO, o32rva, &dwRet)) {
			if (((DWORD)pVerHead->wTotLen > dwRet) || (pVerHead->vsf.dwSignature != VS_FFI_SIGNATURE))
				KSetLastError(pCurThread,ERROR_INVALID_DATA);
			else
				dwRet = pVerHead->wTotLen;
			LocalFree(pVerHead);
		}
		if (!inRom)
			LocalFree(lpres);
	}
	DEBUGMSG(ZONE_ENTRY,(L"SC_GetFileVersionInfoSizeW exit: %8.8lx\r\n",dwRet));
	return dwRet;
}

BOOL SC_GetFileVersionInfoW(LPWSTR lpFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData) {
	LPBYTE lpres;
	BOOL inRom, bRet = FALSE;
	DWORD o32rva, dwTemp;
	VERHEAD *pVerHead;
	DEBUGMSG(ZONE_ENTRY,(L"SC_GetFileVersionInfoW entry: %8.8lx %8.8lx %8.8lx %8.8lx\r\n",lpFilename, dwHandle, dwLen, lpData));
	if (dwLen < sizeof(((VERHEAD*)lpData)->wTotLen))
		KSetLastError(pCurThread,ERROR_INSUFFICIENT_BUFFER);
	else if (lpres = OpenResourceFile(lpFilename, &inRom, &o32rva)) {
		if (pVerHead = (VERHEAD *)ExtractOneResource(lpres, MAKEINTRESOURCE(VS_VERSION_INFO), VS_FILE_INFO, o32rva, &dwTemp)) {
			if (((DWORD)pVerHead->wTotLen > dwTemp) || (pVerHead->vsf.dwSignature != VS_FFI_SIGNATURE))
				KSetLastError(pCurThread,ERROR_INVALID_DATA);
			else {
				dwTemp = (DWORD)pVerHead->wTotLen;
				if (dwTemp > dwLen)
					dwTemp = dwLen;
				memcpy(lpData,pVerHead,dwTemp);
				((VERHEAD*)lpData)->wTotLen = (WORD)dwTemp;
				bRet = TRUE;
			}
			LocalFree(pVerHead);
		}
		if (!inRom)
			LocalFree(lpres);
	}
	DEBUGMSG(ZONE_ENTRY,(L"SC_GetFileVersionInfoSizeW exit: %d\r\n",bRet));
	return bRet;;
}

void AToUCopy(WCHAR *wptr, LPBYTE aptr, int maxlen) {
	int len;
	for (len = 1; (len<maxlen) && *aptr; len++)
		*wptr++ = (WCHAR)*aptr++;
	*wptr = 0;
}

#define COMP_BLOCK_SIZE PAGE_SIZE

BYTE lpDecBuf[COMP_BLOCK_SIZE];
DWORD cfile=0xffffffff,cblock=0xffffffff;

DWORD SC_GetRomFileBytes(DWORD type, DWORD count, DWORD pos, LPVOID buffer, DWORD nBytesToRead) {
	TOCentry *tocptr;
	FILESentry *filesptr;
	DWORD dwBlock,bRead,csize, fullsize;
	LPVOID buffercopy;
	ROMChain_t *pROM = ROMChain;
//	DEBUGMSG(ZONE_ENTRY,(L"SC_GetRomFileBytes entry: %8.8lx %8.8lx %8.8lx %8.8lx %8.8lx\r\n",
//		type,count,pos,buffer,nBytesToRead));
	if (type == 2) {
		while (pROM && (count >= pROM->pTOC->numfiles)) {
			count -= pROM->pTOC->numfiles;
			pROM = pROM->pNext;
		}
		if (!pROM) {
//			DEBUGMSG(ZONE_ENTRY,(L"SC_GetRomFileBytes exit: %8.8lx\r\n",0));
			return 0;
		}
		tocptr = &(((TOCentry *)&pROM->pTOC[1])[pROM->pTOC->nummods]);
		filesptr = &(((FILESentry *)tocptr)[count]);
		if (pos > filesptr->nRealFileSize) {
//			DEBUGMSG(ZONE_ENTRY,(L"SC_GetRomFileBytes exit: %8.8lx\r\n",0));
			return 0;
		}
		if (pos + nBytesToRead > filesptr->nRealFileSize)
			nBytesToRead = filesptr->nRealFileSize - pos;
		if (!nBytesToRead)
			fullsize = 0;
		else {
			fullsize = nBytesToRead;
			buffercopy = buffer;
			LockPages(buffercopy,fullsize,0,LOCKFLAG_WRITE);
			EnterCriticalSection(&RFBcs);
			if (filesptr->nRealFileSize == filesptr->nCompFileSize)
				memcpy(buffer,(LPBYTE)filesptr->ulLoadOffset+pos,nBytesToRead);
			else if (!filesptr->nCompFileSize)
				memset(buffer,0,nBytesToRead);
			else while (nBytesToRead) {
				dwBlock = pos/COMP_BLOCK_SIZE;
				bRead = nBytesToRead;
				if ((pos + bRead-1)/COMP_BLOCK_SIZE != dwBlock)
					bRead = COMP_BLOCK_SIZE - (pos&(COMP_BLOCK_SIZE-1));
				if ((count != cfile) || (dwBlock != cblock)) {
					if (dwBlock == filesptr->nRealFileSize/COMP_BLOCK_SIZE)
						csize = filesptr->nRealFileSize - (COMP_BLOCK_SIZE*dwBlock);
					else
						csize = COMP_BLOCK_SIZE;
					if (bRead == csize)
						Decompress((LPBYTE)filesptr->ulLoadOffset,filesptr->nCompFileSize,buffer,csize,dwBlock*COMP_BLOCK_SIZE);
					else {
						Decompress((LPBYTE)filesptr->ulLoadOffset,filesptr->nCompFileSize,lpDecBuf,csize,dwBlock*COMP_BLOCK_SIZE);
						cfile = count;
						cblock = dwBlock;
						memcpy(buffer,lpDecBuf+(pos&(COMP_BLOCK_SIZE-1)),bRead);
					}
				} else
					memcpy(buffer,lpDecBuf+(pos&(COMP_BLOCK_SIZE-1)),bRead);
				buffer = (LPVOID)((LPBYTE)buffer+bRead);
				nBytesToRead -= bRead;
				pos += bRead;
			}
			LeaveCriticalSection(&RFBcs);
			UnlockPages(buffercopy,fullsize);
		}
//		DEBUGMSG(ZONE_ENTRY,(L"SC_GetRomFileBytes exit: %8.8lx\r\n",fullsize));
		return fullsize;
	}
//	DEBUGMSG(ZONE_ENTRY,(L"SC_GetRomFileBytes exit: %8.8lx\r\n",0));
	return 0;
}

LPVOID SC_MapUncompressedFileW(LPCWSTR pFileName, LPDWORD pLen) {
	DWORD loop;
	TOCentry *tocptr;
	FILESentry *filesptr;
	BYTE *bTrav;
	const WCHAR *wTrav;
	ROMChain_t *pROM = ROMChain;
	for (pROM = ROMChain; pROM; pROM = pROM->pNext) {
		tocptr = &(((TOCentry *)&pROM->pTOC[1])[pROM->pTOC->nummods]);
		for (loop = 0; loop < pROM->pTOC->numfiles; loop++) {
			filesptr = &(((FILESentry *)tocptr)[loop]);
			for (bTrav = filesptr->lpszFileName, wTrav = pFileName; *wTrav && (*wTrav == *bTrav); wTrav++, bTrav++)
				;
			if (!*wTrav && !*bTrav) {
				if (filesptr->nRealFileSize != filesptr->nCompFileSize) {
					KSetLastError(pCurThread,ERROR_INVALID_FLAGS);
					return 0;
				}
				if (pLen)
					*pLen = filesptr->nRealFileSize;
				return (LPVOID)filesptr->ulLoadOffset;
			}
		}
	}
	KSetLastError(pCurThread,ERROR_FILE_NOT_FOUND);
	return 0;
}

const PFNVOID ExtraMethods[] = {
	(LPVOID)SC_EventModify,			// 000
	(LPVOID)SC_ReleaseMutex,		// 001
	(LPVOID)SC_CreateAPIHandle,		// 002
	(LPVOID)SC_MapViewOfFile,		// 003
	(LPVOID)SC_ThreadGetPrio,		// 004
	(LPVOID)SC_ThreadSetPrio,		// 005
	(LPVOID)SC_SelectObject,		// 006
	(LPVOID)SC_PatBlt,				// 007
	(LPVOID)SC_GetTextExtentExPointW,// 008
	(LPVOID)SC_GetSysColorBrush,	// 009
	(LPVOID)SC_SetBkColor,			// 010
	(LPVOID)SC_GetParent,			// 011
	(LPVOID)SC_InvalidateRect,		// 012
	(LPVOID)SC_RegOpenKeyExW,		// 013
	(LPVOID)SC_RegQueryValueExW,	// 014
	(LPVOID)SC_CreateFileW,			// 015
	(LPVOID)SC_ReadFile,			// 016
	(LPVOID)SC_OpenDatabaseEx,		// 017
	(LPVOID)SC_SeekDatabase,		// 018
	(LPVOID)SC_ReadRecordPropsEx,	// 019
	(LPVOID)SC_RegCreateKeyExW,		// 020
	(LPVOID)SC_DeviceIoControl,		// 021
	(LPVOID)SC_CloseHandle,			// 022
	(LPVOID)SC_RegCloseKey,			// 023
	(LPVOID)SC_ClientToScreen,		// 024
	(LPVOID)SC_DefWindowProcW,		// 025
	(LPVOID)SC_GetClipCursor,		// 026
	(LPVOID)SC_GetDC,				// 027
	(LPVOID)SC_GetFocus,			// 028
	(LPVOID)SC_GetMessageW,			// 029
	(LPVOID)SC_GetWindow,			// 030
	(LPVOID)SC_PeekMessageW,		// 031
	(LPVOID)SC_ReleaseDC,			// 032
	(LPVOID)SC_SendMessageW,		// 033
	(LPVOID)SC_SetScrollInfo,		// 034
	(LPVOID)SC_SetWindowLongW,		// 035
	(LPVOID)SC_SetWindowPos,		// 036
	(LPVOID)SC_CreateSolidBrush,	// 037
	(LPVOID)SC_DeleteMenu,			// 038
	(LPVOID)SC_DeleteObject,		// 039
	(LPVOID)SC_DrawTextW,			// 040
	(LPVOID)SC_ExtTextOutW,			// 041
	(LPVOID)SC_FillRect,			// 042
	(LPVOID)SC_GetAsyncKeyState,	// 043
	(LPVOID)SC_GetDlgCtrlID,		// 044
	(LPVOID)SC_GetStockObject,		// 045
	(LPVOID)SC_GetSystemMetrics,	// 046
	(LPVOID)SC_RegisterClassWStub,	// 047
	(LPVOID)SC_RegisterClipboardFormatW, // 048
	(LPVOID)SC_SetBkMode,			// 049
	(LPVOID)SC_SetTextColor,		// 050
	(LPVOID)SC_TransparentImage,	// 051
	(LPVOID)SC_IsDialogMessageW,	// 052
	(LPVOID)SC_PostMessageW,		// 053
	(LPVOID)SC_IsWindowVisible,		// 054
	(LPVOID)SC_GetKeyState,			// 055
	(LPVOID)SC_BeginPaint,			// 056
	(LPVOID)SC_EndPaint,			// 057
	(LPVOID)SC_PerformCallBack4,	// 058
	(LPVOID)SC_CeWriteRecordProps,	// 059
	(LPVOID)SC_ReadFileWithSeek,	// 060
	(LPVOID)SC_WriteFileWithSeek,	// 061
};

ERRFALSE(sizeof(ExtraMethods) == sizeof(ExtraMethods[0])*62);

BOOL SC_GetRomFileInfo(DWORD type, LPWIN32_FIND_DATA lpfd, DWORD count) {
	TOCentry *tocptr;
	FILESentry *filesptr;
	ROMChain_t *pROM = ROMChain;
	extern const PFNVOID Win32Methods[];
	DEBUGMSG(ZONE_ENTRY,(L"SC_GetRomFileInfo entry: %8.8lx %8.8lx %8.8lx\r\n",type,lpfd,count));
	switch (type) {
		case 1:
			while (pROM && (count >= pROM->pTOC->nummods)) {
				count -= pROM->pTOC->nummods;
				pROM = pROM->pNext;
			}
			if (!pROM) {
				DEBUGMSG(ZONE_ENTRY,(L"SC_GetRomFileInfo exit: %8.8lx\r\n",FALSE));
				return FALSE;
			}
			tocptr = &(((TOCentry *)&pROM->pTOC[1])[count]);
			lpfd->dwFileAttributes = (tocptr->dwFileAttributes | FILE_ATTRIBUTE_INROM | FILE_ATTRIBUTE_ROMMODULE) & ~FILE_ATTRIBUTE_ROMSTATICREF;
			lpfd->ftCreationTime = tocptr->ftTime;
			lpfd->ftLastAccessTime = tocptr->ftTime;
			lpfd->ftLastWriteTime = tocptr->ftTime;
			lpfd->nFileSizeHigh = 0;
			lpfd->nFileSizeLow = tocptr->nFileSize;
			AToUCopy(lpfd->cFileName,tocptr->lpszFileName,MAX_PATH);
			DEBUGMSG(ZONE_ENTRY,(L"SC_GetRomFileInfo exit: %8.8lx\r\n",TRUE));
			return TRUE;
		case 2:
			while (pROM && (count >= pROM->pTOC->numfiles)) {
				count -= pROM->pTOC->numfiles;
				pROM = pROM->pNext;
			}
			if (!pROM) {
				DEBUGMSG(ZONE_ENTRY,(L"SC_GetRomFileInfo exit: %8.8lx\r\n",FALSE));
				return FALSE;
			}
			tocptr = &(((TOCentry *)&pROM->pTOC[1])[pROM->pTOC->nummods]);
			filesptr = &(((FILESentry *)tocptr)[count]);
			lpfd->dwFileAttributes = filesptr->dwFileAttributes | FILE_ATTRIBUTE_INROM;
			lpfd->ftCreationTime = filesptr->ftTime;
			lpfd->ftLastAccessTime = filesptr->ftTime;
			lpfd->ftLastWriteTime = filesptr->ftTime;
			lpfd->nFileSizeHigh = 0;
			lpfd->nFileSizeLow = filesptr->nRealFileSize;
			AToUCopy(lpfd->cFileName,filesptr->lpszFileName,MAX_PATH);
			DEBUGMSG(ZONE_ENTRY,(L"SC_GetRomFileInfo exit: %8.8lx\r\n",TRUE));
			return TRUE;
		case 3:
			*(LPVOID *)lpfd = Win32Methods;
			
			*(BOOL UNALIGNED *)count = bAllKMode;
			return TRUE;
		case 4:
			*(LPVOID *)lpfd = ExtraMethods;
			return TRUE;
		case 5:
#ifdef CELOG			
			if (pTOC->ulKernelFlags & KFLAG_CELOGENABLE) {
				// Tell COREDLL that CeLogData is present.
				*((LPBOOL)lpfd) = TRUE;
			} else {
				*((LPBOOL)lpfd) = FALSE;
			}
#else
			*((LPBOOL)lpfd) = FALSE;
#endif
			return TRUE;
		case 0xffffffff:
			for (count = 0; count < pROM->pTOC->nummods; count++) {
				tocptr = &(((TOCentry *)&pROM->pTOC[1])[count]);
				if (!StrCmpAPascW(tocptr->lpszFileName,lpfd->cFileName)) {
					DEBUGMSG(ZONE_ENTRY,(L"SC_GetRomFileInfo exit: %8.8lx\r\n",TRUE));
					return TRUE;
				}
			}
			for (count = 0; count < pROM->pTOC->numfiles; count++) {
				tocptr = &(((TOCentry *)&pROM->pTOC[1])[pROM->pTOC->nummods]);
				filesptr = &(((FILESentry *)tocptr)[count]);
				if (!StrCmpAPascW(filesptr->lpszFileName,lpfd->cFileName)) {
					DEBUGMSG(ZONE_ENTRY,(L"SC_GetRomFileInfo exit: %8.8lx\r\n",TRUE));
					return TRUE;
				}
			}
			DEBUGMSG(ZONE_ENTRY,(L"SC_GetRomFileInfo exit: %8.8lx\r\n",FALSE));
			return FALSE;
	}		
	DEBUGCHK(0);
	DEBUGMSG(ZONE_ENTRY,(L"SC_GetRomFileInfo exit: %8.8lx\r\n",FALSE));
	return FALSE;
}

