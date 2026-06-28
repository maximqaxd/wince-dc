//
// sdcard.c - DCWin "External Storage": a WDM-loaded FAT file system driver that surfaces
// the SD card as "\External Storage" in the CE namespace (browsable in Explorer).
//
// Mechanism reversed from the GD-ROM driver wsegacd.dll (see memory wince-fsd-afs-mount):
// three CreateAPISet vtables (volume / file / find) registered with RegisterAPISet, then
// RegisterAFSName + RegisterAFS to mount. The volume callbacks delegate to ChaN FatFs;
// CreateFileW/FindFirstFileW mint per-object handles with CreateAPIHandle. A single
// critical section serialises all FatFs access (static LFN buffer is not re-entrant).
//
#include <windows.h>
#include <wdm.h>
#include "fatfs/ff.h"
#include "sdblk.h"
#include "syslog.h"

// ---- CE coredll API-set / AFS primitives (exported by name, no public header) ----------
typedef int (*APIFN)(void);
extern HANDLE WINAPI CreateAPISet(char *pName, DWORD dwMethods, const APIFN *ppfn, const DWORD *pdwSig);
extern HANDLE WINAPI CreateAPIHandle(HANDLE hAPISet, void *pvData);
extern BOOL   WINAPI RegisterAPISet(HANDLE hAPISet, DWORD dwAPISet);
extern int    WINAPI RegisterAFSName(const WCHAR *pName);
extern BOOL   WINAPI RegisterAFS(int idx, HANDLE hAPISet, DWORD dwData, DWORD dwFlags);
extern BOOL   WINAPI InitWDMDriver(HANDLE hInst, PDRIVER_INITIALIZE pfnEntry);

#define APISET_FILE  0x80000007        // handle-based file API class (from the reverse)
#define APISET_FIND  0x80000008        // handle-based find API class

// ---- state -----------------------------------------------------------------------------
static FATFS            g_fs;
static int              g_afsidx = -1;
static HANDLE           g_hVol, g_hFile, g_hFind;
static CRITICAL_SECTION g_cs;
static HINSTANCE        g_hInst;

typedef struct { FIL fil; } SDFILE;
typedef struct { DIR dir; WCHAR pat[64]; } SDFIND;

#define LOCK()   EnterCriticalSection(&g_cs)
#define UNLOCK() LeaveCriticalSection(&g_cs)

// ---- helpers ---------------------------------------------------------------------------
static int IsRoot(const WCHAR *p)
{ return p == 0 || p[0] == 0 || (p[0] == L'\\' && p[1] == 0) || (p[0] == L'/' && p[1] == 0); }

// case-insensitive wildcard match ('*' and '?'); used to filter f_readdir output.
static int MatchPat(const WCHAR *pat, const WCHAR *name)
{
    while (*pat)
    {
        if (*pat == L'*')
        {
            pat++;
            if (!*pat) return 1;
            while (*name) { if (MatchPat(pat, name)) return 1; name++; }
            return MatchPat(pat, name);
        }
        if (!*name) return 0;
        if (*pat != L'?')
        {
            WCHAR a = *pat, b = *name;
            if (a >= L'a' && a <= L'z') a -= 32;
            if (b >= L'a' && b <= L'z') b -= 32;
            if (a != b) return 0;
        }
        pat++; name++;
    }
    return *name == 0;
}

static void DosToFt(WORD fdate, WORD ftime, FILETIME *ft)
{
    SYSTEMTIME st;
    memset(&st, 0, sizeof(st));
    st.wYear   = (WORD)(1980 + (fdate >> 9));
    st.wMonth  = (WORD)((fdate >> 5) & 0x0F); if (st.wMonth < 1) st.wMonth = 1;
    st.wDay    = (WORD)(fdate & 0x1F);        if (st.wDay   < 1) st.wDay   = 1;
    st.wHour   = (WORD)(ftime >> 11);
    st.wMinute = (WORD)((ftime >> 5) & 0x3F);
    st.wSecond = (WORD)((ftime & 0x1F) * 2);
    SystemTimeToFileTime(&st, ft);
}

// Fill a WIN32_FIND_DATAW from a FatFs FILINFO (+ its long name when present).
static void FillFind(WIN32_FIND_DATAW *fd, FILINFO *fno, const WCHAR *lfn)
{
    const WCHAR *nm = (lfn && lfn[0]) ? lfn : fno->fname;
    int i;
    memset(fd, 0, sizeof(*fd));
    fd->dwFileAttributes = fno->fattrib;          // FatFs AM_* == Win32 FILE_ATTRIBUTE_*
    fd->nFileSizeLow     = fno->fsize;
    DosToFt(fno->fdate, fno->ftime, &fd->ftLastWriteTime);
    fd->ftCreationTime   = fd->ftLastWriteTime;
    fd->ftLastAccessTime = fd->ftLastWriteTime;
    for (i = 0; i < MAX_PATH - 1 && nm[i]; i++) fd->cFileName[i] = nm[i];
    fd->cFileName[i] = 0;
}

// ---- VOLUME callbacks (17; order per the reversed PCDF table) --------------------------
static int   V_NoSupport(void) { SetLastError(ERROR_NOT_SUPPORTED); return 0; }

static BOOL  V_CreateDirectory(void *vol, const WCHAR *path, void *sa)
{ BOOL r; (void)vol; (void)sa; LOCK(); r = (f_mkdir(path) == FR_OK); UNLOCK(); return r; }

static BOOL  V_RemoveDirectory(void *vol, const WCHAR *path)
{ BOOL r; (void)vol; LOCK(); r = (f_unlink(path) == FR_OK); UNLOCK(); return r; }

static DWORD V_GetFileAttributes(void *vol, const WCHAR *path)
{
    FILINFO fno; DWORD a; (void)vol;
    SysLog(L"sd: GetAttr '%s'", path ? path : L"(null)");
    if (IsRoot(path)) return FILE_ATTRIBUTE_DIRECTORY;
    LOCK(); a = (f_stat(path, &fno) == FR_OK) ? fno.fattrib : 0xFFFFFFFF; UNLOCK();
    return a;
}

static BOOL  V_SetFileAttributes(void *vol, const WCHAR *path, DWORD attr)
{ BOOL r; (void)vol; LOCK(); r = (f_chmod(path, (BYTE)attr, AM_RDO|AM_ARC|AM_SYS|AM_HID) == FR_OK); UNLOCK(); return r; }

static HANDLE V_CreateFile(void *vol, HANDLE hProc, const WCHAR *name, DWORD access,
                           DWORD share, void *sa, DWORD creation, DWORD flags, HANDLE htmpl)
{
    SDFILE *f; BYTE mode = 0; HANDLE h = INVALID_HANDLE_VALUE;
    (void)vol; (void)hProc; (void)share; (void)sa; (void)flags; (void)htmpl;
    if (access & GENERIC_READ)  mode |= FA_READ;
    if (access & GENERIC_WRITE) mode |= FA_WRITE;
    switch (creation)
    {
    case CREATE_NEW:        mode |= FA_CREATE_NEW;    break;
    case CREATE_ALWAYS:     mode |= FA_CREATE_ALWAYS; break;
    case OPEN_EXISTING:     mode |= FA_OPEN_EXISTING; break;
    case OPEN_ALWAYS:       mode |= FA_OPEN_ALWAYS;   break;
    case TRUNCATE_EXISTING: mode |= FA_CREATE_ALWAYS; break;
    default:                mode |= FA_OPEN_EXISTING; break;
    }
    f = (SDFILE *)LocalAlloc(LPTR, sizeof(SDFILE));
    if (!f) { SetLastError(ERROR_OUTOFMEMORY); return INVALID_HANDLE_VALUE; }
    LOCK();
    if (f_open(&f->fil, name, mode) == FR_OK)
        h = CreateAPIHandle(g_hFile, f);
    UNLOCK();
    if (h == 0 || h == INVALID_HANDLE_VALUE) { LocalFree(f); SetLastError(ERROR_FILE_NOT_FOUND); return INVALID_HANDLE_VALUE; }
    return h;
}

static BOOL  V_DeleteFile(void *vol, const WCHAR *path)
{ BOOL r; (void)vol; LOCK(); r = (f_unlink(path) == FR_OK); UNLOCK(); return r; }

static BOOL  V_MoveFile(void *vol, const WCHAR *oldp, const WCHAR *newp)
{ BOOL r; (void)vol; LOCK(); r = (f_rename(oldp, newp) == FR_OK); UNLOCK(); return r; }

static HANDLE V_FindFirstFile(void *vol, HANDLE hProc, const WCHAR *spec, WIN32_FIND_DATAW *pfd)
{
    SDFIND *s; WCHAR dir[MAX_PATH], lfn[256]; FILINFO fno; const WCHAR *p; int i, slash = -1;
    HANDLE h = INVALID_HANDLE_VALUE;
    (void)vol; (void)hProc;
    for (i = 0; spec[i] && i < MAX_PATH - 1; i++) if (spec[i] == L'\\' || spec[i] == L'/') slash = i;
    for (i = 0; i < slash; i++) dir[i] = spec[i];
    dir[slash > 0 ? slash : 0] = 0;                 // dir = path up to last slash ("" = root)
    p = spec + slash + 1;                           // pattern after last slash

    s = (SDFIND *)LocalAlloc(LPTR, sizeof(SDFIND));
    if (!s) { SetLastError(ERROR_OUTOFMEMORY); return INVALID_HANDLE_VALUE; }
    for (i = 0; i < 63 && p[i]; i++) s->pat[i] = p[i]; s->pat[i] = 0;
    if (!s->pat[0]) { s->pat[0] = L'*'; s->pat[1] = 0; }            // empty spec -> match all

    SysLog(L"sd: FindFirst '%s' pat='%s'", spec ? spec : L"(null)", s->pat);
    LOCK();
    {
        FRESULT fr = f_opendir(&s->dir, IsRoot(dir) ? L"\\" : dir);
        SysLog(L"sd: f_opendir('%s')=%d", IsRoot(dir) ? L"\\" : dir, fr);
        if (fr == FR_OK)
        for (;;)
        {
            fno.lfname = lfn; fno.lfsize = 256; lfn[0] = 0;
            if (f_readdir(&s->dir, &fno) != FR_OK || fno.fname[0] == 0) break;   // end
            if (MatchPat(s->pat, lfn[0] ? lfn : fno.fname)) { FillFind(pfd, &fno, lfn); h = CreateAPIHandle(g_hFind, s); break; }
        }
    }
    UNLOCK();
    if (h == 0 || h == INVALID_HANDLE_VALUE) { f_closedir(&s->dir); LocalFree(s); SetLastError(ERROR_FILE_NOT_FOUND); return INVALID_HANDLE_VALUE; }
    return h;
}

static BOOL  V_GetDiskFreeSpace(void *vol, const WCHAR *path, DWORD *spc, DWORD *bps, DWORD *freecl, DWORD *totcl)
{
    FATFS *fs; DWORD nfree; BOOL r; (void)vol; (void)path;
    LOCK(); r = (f_getfree(L"", &nfree, &fs) == FR_OK); UNLOCK();
    if (!r) return FALSE;
    if (spc)    *spc = fs->csize;
    if (bps)    *bps = 512;
    if (freecl) *freecl = nfree;
    if (totcl)  *totcl = fs->n_fatent - 2;
    return TRUE;
}

// ---- FILE callbacks (14; order per the reversed HCDF table) ----------------------------
static BOOL  F_Close(SDFILE *f)
{ BOOL r; if (!f) return FALSE; LOCK(); r = (f_close(&f->fil) == FR_OK); UNLOCK(); LocalFree(f); return r; }

static BOOL  F_Read(SDFILE *f, void *buf, DWORD cb, DWORD *pcb, void *ovl)
{ UINT br = 0; BOOL r; (void)ovl; if (!f) return FALSE; LOCK(); r = (f_read(&f->fil, buf, cb, &br) == FR_OK); UNLOCK(); if (pcb) *pcb = br; return r; }

static BOOL  F_Write(SDFILE *f, const void *buf, DWORD cb, DWORD *pcb, void *ovl)
{ UINT bw = 0; BOOL r; (void)ovl; if (!f) return FALSE; LOCK(); r = (f_write(&f->fil, buf, cb, &bw) == FR_OK); UNLOCK(); if (pcb) *pcb = bw; return r; }

static DWORD F_GetSize(SDFILE *f, DWORD *high)
{ DWORD sz; if (!f) return 0; LOCK(); sz = f_size(&f->fil); UNLOCK(); if (high) *high = 0; return sz; }

static DWORD F_SetPointer(SDFILE *f, LONG lo, LONG *high, DWORD method)
{
    DWORD pos; (void)high; if (!f) return 0xFFFFFFFF;
    LOCK();
    if (method == FILE_CURRENT)    lo += (LONG)f_tell(&f->fil);
    else if (method == FILE_END)   lo += (LONG)f_size(&f->fil);
    f_lseek(&f->fil, (DWORD)lo);
    pos = f_tell(&f->fil);
    UNLOCK();
    return pos;
}

static BOOL  F_Flush(SDFILE *f)
{ BOOL r; if (!f) return FALSE; LOCK(); r = (f_sync(&f->fil) == FR_OK); UNLOCK(); return r; }

static BOOL  F_SetEnd(SDFILE *f)
{ BOOL r; if (!f) return FALSE; LOCK(); r = (f_truncate(&f->fil) == FR_OK); UNLOCK(); return r; }

static BOOL  F_ReadSeek(SDFILE *f, void *buf, DWORD cb, DWORD *pcb, void *ovl, DWORD lo, DWORD hi)
{ UINT br = 0; BOOL r; (void)ovl; (void)hi; if (!f) return FALSE; LOCK(); f_lseek(&f->fil, lo); r = (f_read(&f->fil, buf, cb, &br) == FR_OK); UNLOCK(); if (pcb) *pcb = br; return r; }

static BOOL  F_WriteSeek(SDFILE *f, const void *buf, DWORD cb, DWORD *pcb, void *ovl, DWORD lo, DWORD hi)
{ UINT bw = 0; BOOL r; (void)ovl; (void)hi; if (!f) return FALSE; LOCK(); f_lseek(&f->fil, lo); r = (f_write(&f->fil, buf, cb, &bw) == FR_OK); UNLOCK(); if (pcb) *pcb = bw; return r; }

// ---- FIND callbacks (3; order per the reversed FCDF table) -----------------------------
static BOOL  S_Close(SDFIND *s)
{ if (!s) return FALSE; LOCK(); f_closedir(&s->dir); UNLOCK(); LocalFree(s); return TRUE; }

static BOOL  S_Next(SDFIND *s, WIN32_FIND_DATAW *pfd)
{
    WCHAR lfn[256]; FILINFO fno; BOOL r = FALSE;
    if (!s) return FALSE;
    LOCK();
    for (;;)
    {
        fno.lfname = lfn; fno.lfsize = 256; lfn[0] = 0;
        if (f_readdir(&s->dir, &fno) != FR_OK || fno.fname[0] == 0) break;
        if (MatchPat(s->pat, lfn[0] ? lfn : fno.fname)) { FillFind(pfd, &fno, lfn); r = TRUE; break; }
    }
    UNLOCK();
    if (!r) SetLastError(ERROR_NO_MORE_FILES);
    return r;
}

// ---- API-set method + signature tables (exact slot order + sigs from the reverse) ------
static const APIFN g_volMethods[17] = {
    (APIFN)V_NoSupport, (APIFN)V_NoSupport, (APIFN)V_CreateDirectory, (APIFN)V_RemoveDirectory,
    (APIFN)V_GetFileAttributes, (APIFN)V_SetFileAttributes, (APIFN)V_CreateFile, (APIFN)V_DeleteFile,
    (APIFN)V_MoveFile, (APIFN)V_FindFirstFile, (APIFN)V_NoSupport /*RegFSNotify*/, (APIFN)V_NoSupport /*OidGetInfo*/,
    (APIFN)V_MoveFile /*DeleteAndRename*/, (APIFN)V_NoSupport /*CloseAllHandles*/, (APIFN)V_GetDiskFreeSpace,
    (APIFN)V_NoSupport /*Notify*/, (APIFN)V_NoSupport /*RegFSFunc*/ };
static const DWORD g_volSigs[17] = {
    0,0,0x14,4,4,4,0x410,4,0x14,0x50,0,0x10,0x14,0,0x554,0,0 };

static const APIFN g_fileMethods[14] = {
    (APIFN)F_Close, (APIFN)V_NoSupport, (APIFN)F_Read, (APIFN)F_Write, (APIFN)F_GetSize,
    (APIFN)F_SetPointer, (APIFN)V_NoSupport /*GetFileInfo*/, (APIFN)F_Flush, (APIFN)V_NoSupport /*GetFileTime*/,
    (APIFN)V_NoSupport /*SetFileTime*/, (APIFN)F_SetEnd, (APIFN)V_NoSupport /*DeviceIoControl*/,
    (APIFN)F_ReadSeek, (APIFN)F_WriteSeek };
static const DWORD g_fileSigs[14] = {
    0,0,0x144,0x144,4,0x10,4,0,0x54,0x54,0,0x5110,0x144,0x144 };

static const APIFN g_findMethods[3] = { (APIFN)S_Close, (APIFN)V_NoSupport, (APIFN)S_Next };
static const DWORD   g_findSigs[3]    = { 0, 0, 4 };

// ---- mount + WDM entry -----------------------------------------------------------------
static void FsdInit(void)
{
    InitializeCriticalSection(&g_cs);
    g_hVol  = CreateAPISet("FATV", 17, g_volMethods,  g_volSigs);
    g_hFile = CreateAPISet("FATH", 14, g_fileMethods, g_fileSigs);
    g_hFind = CreateAPISet("FATF", 3,  g_findMethods, g_findSigs);
    if (!g_hVol || !g_hFile || !g_hFind) return;
    RegisterAPISet(g_hFile, APISET_FILE);
    RegisterAPISet(g_hFind, APISET_FIND);

    f_mount(&g_fs, L"", 0);                          // lazy: the disk is initialised on first access

    g_afsidx = RegisterAFSName(L"External Storage");
    if (g_afsidx >= 0)
    {
        BOOL r = RegisterAFS(g_afsidx, g_hVol, 1, 4);   // dwData unused (single volume)
        SysLog(L"sd: FsdInit afs=%d RegisterAFS=%d", g_afsidx, r);
    }
    else SysLog(L"sd: RegisterAFSName failed");
}

static NTSTATUS DriverEntry(PDRIVER_OBJECT drv, PUNICODE_STRING reg)
{
    (void)drv; (void)reg;
    FsdInit();
    return 0;                                        // STATUS_SUCCESS
}

BOOL WINAPI DllMain(HANDLE hInst, DWORD reason, LPVOID reserved)
{
    (void)reserved;
    if (reason == DLL_PROCESS_ATTACH)
    {
        g_hInst = (HINSTANCE)hInst;
        return InitWDMDriver(hInst, DriverEntry);
    }
    return TRUE;
}
