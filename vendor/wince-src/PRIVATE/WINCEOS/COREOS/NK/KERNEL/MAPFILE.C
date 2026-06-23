/*
 *              NK Kernel loader code
 *
 *              Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.
 *
 * Module Name:
 *
 *              mapfile.c
 *
 * Abstract:
 *
 *              This file implements the NK kernel mapped file routines
 *
 */

#include "kernel.h"

DWORD rbRes;
DWORD rbSubRes[NUM_MAPPER_SECTIONS];
extern CRITICAL_SECTION VAcs, PagerCS, MapCS, MapNameCS, WriterCS;
HANDLE hMapList;

BOOL SC_MapCloseHandle(HANDLE hMap);
BOOL ValidateFile(LPFSMAP lpm);
BOOL FlushMapBuffersLogged(LPFSMAP lpm, DWORD dwOffset, DWORD dwLength, DWORD dwFlags);

const PFNVOID MapMthds[] = {
    (PFNVOID)SC_MapCloseHandle,
    (PFNVOID)0,
    (PFNVOID)SC_MapViewOfFile,
};

const CINFO cinfMap = {
    "FMAP",
    DISPATCH_KERNEL_PSL,
    HT_FSMAP,
    sizeof(MapMthds)/sizeof(MapMthds[0]),
    MapMthds
};

#define SUB_MAPPER_INCR (MAPPER_INCR/32)
#define MapBits(n) ((n) == 0x1f ? 0xffffffff : (1<<((n)+1))-1)


// Restore flags used for map buffer logged flush
#define RESTORE_FLAG_NONE      0
#define RESTORE_FLAG_UNFLUSHED 1
#define RESTORE_FLAG_FLUSHED   2



// reserves a memory region of length len
LPVOID FSMapMemReserve(DWORD len)
{
    DWORD secs;
    DWORD first, curr, trav;
    LPVOID retval = 0;
    
    DEBUGCHK(len);
    
    EnterCriticalSection(&VAcs);
    
    if (len >= MAPPER_INCR) {
        
        secs = (len + MAPPER_INCR - 1)/MAPPER_INCR;
        first = 0;
        
        for (curr = 0; curr <= NUM_MAPPER_SECTIONS - secs; curr++) {
            if (rbRes & (1<<curr))
                first = curr+1;
            else if (curr - first + 1 == secs)
                break;
        }
        
        if (curr > NUM_MAPPER_SECTIONS - secs)
            goto exit;
        
        for (trav = first; trav <= curr; trav++) {
            if (!CreateMapperSection(FIRST_MAPPER_ADDRESS+(MAPPER_INCR*trav))) {
                while (trav-- != first) {
                    DeleteMapperSection(FIRST_MAPPER_ADDRESS+(MAPPER_INCR*trav));
                    rbRes &= ~(1<<trav);
                }
                
                goto exit;
            }
            
            rbRes |= (1<<trav);
            rbSubRes[trav] = (trav == curr
                              ? MapBits(((len - 1)%MAPPER_INCR)/SUB_MAPPER_INCR)
                              : (DWORD)-1);
        }
        
        retval = (LPVOID)(FIRST_MAPPER_ADDRESS + (MAPPER_INCR*first));
    
    } else {
        
        secs = (len + SUB_MAPPER_INCR - 1) / SUB_MAPPER_INCR;
        
        for (curr = 0; curr < NUM_MAPPER_SECTIONS; curr++) {
            if (rbRes & (1<<curr)) {
                first = 0;
                
                for (trav = 0; trav <= 32-secs; trav++) {
                    if (rbSubRes[curr] & (1<<trav))
                        first = trav+1;
                    else if (trav - first == secs)
                        break;
                }
                
                if (trav <= 32-secs)
                    break;
            }
        }
        
        if (curr == NUM_MAPPER_SECTIONS) {
            for (curr = 0; curr < NUM_MAPPER_SECTIONS; curr++) {
                if (!(rbRes & (1<<curr))) {
                    if (!CreateMapperSection(FIRST_MAPPER_ADDRESS + (MAPPER_INCR*curr)))
                        goto exit;
                    
                    rbRes |= (1<<curr);
                    rbSubRes[curr] = MapBits(((len-1)/SUB_MAPPER_INCR));
                    retval = (LPVOID)(FIRST_MAPPER_ADDRESS + (MAPPER_INCR*curr));
                    
                    break;
                }
            }
        
        } else {
            rbSubRes[curr] |= (MapBits(((len-1)/SUB_MAPPER_INCR)) << first);
            retval = (LPVOID)(FIRST_MAPPER_ADDRESS + (MAPPER_INCR*curr)
                              + (SUB_MAPPER_INCR*first));
        }
    }
    
exit:
    
    LeaveCriticalSection(&VAcs);
    
    return retval;
}


CLEANEVENT *pHugeCleanList;

LPVOID HugeVirtualReserve(DWORD dwSize)
{
    LPCLEANEVENT lpce;
    LPVOID pMem;
    
    dwSize = PAGEALIGN_UP(dwSize);
    
    if (!(lpce = AllocMem(HEAP_CLEANEVENT))) {
        KSetLastError(pCurThread, ERROR_OUTOFMEMORY);
        return 0;
    }
    
    if (!(pMem = FSMapMemReserve(dwSize))) {
        FreeMem(lpce, HEAP_CLEANEVENT);
        KSetLastError(pCurThread, ERROR_OUTOFMEMORY);
        return 0;
    }
    
    lpce->base = pMem;
    lpce->op = dwSize;
    lpce->size = (DWORD)pCurProc;
    
    EnterCriticalSection(&MapNameCS);
    lpce->ceptr = pHugeCleanList;
    pHugeCleanList = lpce;
    LeaveCriticalSection(&MapNameCS);
    
    return pMem;
}


BOOL FSMapMemFree(LPBYTE pBase, DWORD len, DWORD flags);

BOOL CloseHugeMemoryAreas(LPVOID pMem)
{
    LPCLEANEVENT pce, pce2;
    
    EnterCriticalSection(&MapNameCS);
    
    pce = pHugeCleanList;
    while (pce && (pce->size == (DWORD)pCurProc) && (!pMem || (pMem == pce->base))) {
        
        pHugeCleanList = pce->ceptr;
        VirtualFree(pce->base, pce->op, MEM_DECOMMIT);
        FSMapMemFree(pce->base, pce->op, MEM_RELEASE);
        FreeMem(pce, HEAP_CLEANEVENT);
        pce = pHugeCleanList;
        
        if (pMem) {
            LeaveCriticalSection(&MapNameCS);
            return TRUE;
        }
    }
    
    if (pce) {
        while (pce->ceptr) {
            if ((pce->ceptr->size == (DWORD)pCurProc)
                && (!pMem || (pMem == pce->ceptr->base))) {
                
                pce2 = pce->ceptr;
                pce->ceptr = pce2->ceptr;
                VirtualFree(pce2->base, pce2->op, MEM_DECOMMIT);
                FSMapMemFree(pce2->base, pce2->op, MEM_RELEASE);
                FreeMem(pce2, HEAP_CLEANEVENT);
                if (pMem) {
                    LeaveCriticalSection(&MapNameCS);
                    return TRUE;
                }
            } else
                pce = pce->ceptr;
        }
    }
    
    LeaveCriticalSection(&MapNameCS);
    
    return FALSE;
}


BOOL HugeVirtualRelease(LPVOID pMem)
{
    if (!CloseHugeMemoryAreas(pMem)) {
        KSetLastError(pCurThread, ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    return TRUE;
}


void DecommitROPages(LPBYTE pBase, DWORD len)
{
    MEMORY_BASIC_INFORMATION mbi;

    EnterCriticalSection(&VAcs);

    while (len) {
        if (!VirtualQuery(pBase, &mbi, sizeof(mbi))) {
            DEBUGCHK(0);
            break;
        }

        if ((mbi.State == MEM_COMMIT) && (mbi.Protect == PAGE_READONLY))
            VirtualFree(pBase, mbi.RegionSize, MEM_DECOMMIT);

        pBase += mbi.RegionSize;
        if (len < mbi.RegionSize)
            break;

        len -= mbi.RegionSize;
    }

    LeaveCriticalSection(&VAcs);
}


// Decommits or releases a MapMemReserve'd chunk of memory.  Length must be passed in.
// Flags must be MEM_DECOMMIT *or* MEM_RELEASE, and to RELEASE the range must be
// decommitted already.  pBase and len must be page aligned for DECOMMIT.  pBase must be
// SUB_MAPPER_INCR aligned and len must be page aligned for RELEASE
BOOL FSMapMemFree(LPBYTE pBase, DWORD len, DWORD flags)
{
    BOOL res, retval = FALSE;
    LPBYTE pCurr = pBase;
    DWORD curr, trav, currbytes, bytesleft = len;

    DEBUGCHK((DWORD)pCurr == PAGEALIGN_UP((DWORD)pCurr));
    DEBUGCHK(bytesleft == PAGEALIGN_UP(bytesleft));

    EnterCriticalSection(&VAcs);

    if (flags == MEM_DECOMMIT) {

        while (bytesleft) {
            currbytes = ((DWORD)(pCurr + MAPPER_INCR) & ~(MAPPER_INCR-1)) - (DWORD)pCurr;
            if (currbytes > bytesleft)
                currbytes = bytesleft;

            DEBUGCHK((DWORD)pCurr == ((DWORD)pCurr & ~(PAGE_SIZE-1)));
            DEBUGCHK(currbytes == PAGEALIGN_UP(currbytes));

            res = VirtualFree(pCurr, currbytes, MEM_DECOMMIT);
            DEBUGCHK(res);

            pCurr += currbytes;
            bytesleft -= currbytes;
        }

        retval = TRUE;

    } else if (flags == MEM_RELEASE) {

        

        do {
            curr = ((DWORD)pCurr - FIRST_MAPPER_ADDRESS)/MAPPER_INCR;
            trav = ((DWORD)pCurr/SUB_MAPPER_INCR)%32;
            DEBUGCHK(rbSubRes[curr] & (1<<trav));

            if (!(rbSubRes[curr] &= ~(1<<trav))) {
                rbRes &= ~(1<<curr);
                DeleteMapperSection(FIRST_MAPPER_ADDRESS+(MAPPER_INCR*curr));
            }

            pCurr += SUB_MAPPER_INCR;

        } while (pCurr < pBase + len);

        retval = TRUE;
    }

    LeaveCriticalSection(&VAcs);

    return retval;
}


// pBase and len must be page-aligned
BOOL FSMapMemCommit(LPBYTE pBase, DWORD len, DWORD access)
{
    BOOL retval = FALSE;
    LPBYTE pFree, pCurr = pBase;
    DWORD currbytes, bytesleft = len;

    DEBUGCHK((DWORD)pCurr == PAGEALIGN_UP((DWORD)pCurr));
    DEBUGCHK(bytesleft == PAGEALIGN_UP(bytesleft));

    EnterCriticalSection(&VAcs);

    while (bytesleft) {

        currbytes = (((DWORD)pCurr + MAPPER_INCR) & ~(MAPPER_INCR-1)) - (DWORD)pCurr;
        if (currbytes > bytesleft)
            currbytes = bytesleft;
        DEBUGCHK(currbytes == PAGEALIGN_UP(currbytes));

        if (!VirtualAlloc(pCurr, currbytes, MEM_COMMIT, access)) {
            pFree = pBase;
            bytesleft = len;

            while (pFree != pCurr) {
                currbytes = (((DWORD)pCurr + MAPPER_INCR) & ~(MAPPER_INCR-1)) - (DWORD)pCurr;
                if (currbytes > bytesleft)
                    currbytes = bytesleft;

                VirtualFree(pFree, currbytes,MEM_DECOMMIT);

                pFree += currbytes;
                bytesleft -= currbytes;
            }

            goto exit;
        }

        pCurr += currbytes;
        bytesleft -= currbytes;
    }

    retval = TRUE;

exit:
    LeaveCriticalSection(&VAcs);

    return retval;
}


// Provides r/w view if request r/o view of r/w map
LPVOID SC_MapViewOfFile(HANDLE hMap, DWORD fdwAccess, DWORD dwOffsetHigh,
                        DWORD dwOffsetLow, DWORD cbMap)
{
    LPFSMAP lpm;
    LPVOID lpret = 0;
    DWORD length;
    CLEANEVENT *lpce;

    DEBUGMSG(ZONE_ENTRY,(L"SC_MapViewOfFile entry: %8.8lx %8.8lx %8.8lx %8.8lx %8.8lx\r\n",
                         hMap, fdwAccess, dwOffsetHigh, dwOffsetLow, cbMap));

    EnterCriticalSection(&MapNameCS);


    //
    // Check args
    //

    if (!(lpm = (bAllKMode ? HandleToMapPerm(hMap) : HandleToMap(hMap)))) {
        KSetLastError(pCurThread, ERROR_INVALID_HANDLE);
        goto exit;
    }

    length = (cbMap ? cbMap : lpm->length - dwOffsetLow);

    if ((fdwAccess == FILE_MAP_ALL_ACCESS)
        || (fdwAccess == (FILE_MAP_WRITE | FILE_MAP_READ)))
        fdwAccess = FILE_MAP_WRITE;

    if (dwOffsetHigh
        || ((fdwAccess == FILE_MAP_WRITE)
            && (lpm->hFile != INVALID_HANDLE_VALUE)
            && !lpm->pDirty)
        || (fdwAccess & ~(FILE_MAP_READ|FILE_MAP_WRITE))
        || ((dwOffsetLow + length) > lpm->length)
        || ((dwOffsetLow + length) < dwOffsetLow)
        || ((dwOffsetLow + length) < length)) {
        KSetLastError(pCurThread,ERROR_INVALID_PARAMETER);
        goto exit;
    }

    
    //
    // Prepare CleanEvent
    //

    if (!(lpce = AllocMem(HEAP_CLEANEVENT))) {
        KSetLastError(pCurThread,ERROR_OUTOFMEMORY);
        goto exit;
    }

    IncRef(hMap,pCurProc);
    lpret = lpm->pBase + dwOffsetLow;
    lpce->base = lpret;
    lpce->size = (DWORD)pCurProc;
    lpce->ceptr = lpm->lpmlist;
    lpm->lpmlist = lpce;

exit:
    LeaveCriticalSection(&MapNameCS);

    DEBUGMSG(ZONE_ENTRY,(L"SC_MapViewOfFile exit: %8.8lx %8.8lx %8.8lx %8.8lx %8.8lx -> %8.8lx\r\n",
                         hMap, fdwAccess, dwOffsetHigh, dwOffsetLow, cbMap, lpret));

    return lpret;
}


#define FMB_LEAVENAMECS 	0x1
#define FMB_DOFULLDISCARD 	0x2
#define FMB_NOWRITEOUT 		0x4


BOOL FlushMapBuffers(LPFSMAP lpm, DWORD dwOffset, DWORD dwLength, DWORD dwFlags)
{
    DWORD remain, page, end, dw1;
    BOOL res, retval = FALSE;
    HANDLE h;
    LPBYTE pb;
    ACCESSKEY ulOldKey;

    SWITCHKEY(ulOldKey,0xffffffff);


    //
    // Figure out where we're starting and stopping, page-aligned
    //

    if (!dwLength)
        dwLength = lpm->length;
    end = dwOffset+dwLength;
    dwOffset = PAGEALIGN_DOWN(dwOffset);
    end = PAGEALIGN_UP(end);
    if (end > lpm->length)
        end = lpm->length;
    if (end < dwOffset) {
        if (dwFlags & FMB_LEAVENAMECS)
            LeaveCriticalSection(&MapNameCS);
        return FALSE;
    }


    if (lpm->pDirty == (LPBYTE)1) {
        //
        // Old-style (non-transactionable) file system
        //

        h = lpm->hFile;
        pb = lpm->pBase;
        if (dwFlags & FMB_LEAVENAMECS)
            LeaveCriticalSection(&MapNameCS);
        SetFilePointer(h,dwOffset,0,FILE_BEGIN);
        if (!WriteFile(h,pb,end-dwOffset,&remain,0) || (remain != end-dwOffset)) {
            DEBUGMSG(ZONE_PAGING,(L"FMB 5\r\n"));
            DEBUGCHK(0);
            goto exit;
        }
        FlushFileBuffers(h);
        retval = TRUE;

    } else if (lpm->pDirty) {
        //
        // We have dirty memory to flush
        //

        EnterCriticalSection(&WriterCS);

        if (!(dwFlags & FMB_NOWRITEOUT)) {

            DEBUGMSG(ZONE_PAGING,(L"FMB 1\r\n"));
            EnterCriticalSection(&PagerCS);

            //
            // Now flush the pages in memory back to the file
            //

            for (page = dwOffset/PAGE_SIZE; (page+1)*PAGE_SIZE <= end; page++) {

                // Flush the page only if it is dirty
                if (lpm->pDirty[page/8] & (1<<(page%8))) {

                    lpm->bRestart = 1;
                    lpm->pDirty[page/8] &= ~(1<<(page%8));
                    res = VirtualProtect(lpm->pBase+page*PAGE_SIZE, PAGE_SIZE,
                                         PAGE_READONLY, &dw1);
                    DEBUGCHK(res);
                    lpm->dwDirty--;
                    DEBUGCHK(!(lpm->dwDirty & 0x80000000));

                    LeaveCriticalSection(&PagerCS);

                    if (!WriteFileWithSeek(lpm->hFile, lpm->pBase+page*PAGE_SIZE, PAGE_SIZE,
                                           &dw1, 0, page*PAGE_SIZE, 0) ||
                        (dw1 != PAGE_SIZE)) {
                        DEBUGMSG(ZONE_PAGING,(L"FMB 2\r\n"));
                        LeaveCriticalSection(&WriterCS);
                        if (dwFlags & FMB_LEAVENAMECS)
                            LeaveCriticalSection(&MapNameCS);
                        DEBUGCHK(0);
                        goto exit;
                    }

                    EnterCriticalSection(&PagerCS);
                }
            }

            // Flush the incomplete page at the end, if present
            remain = end - (page*PAGE_SIZE);
            if (remain && (lpm->pDirty[page/8] & (1<<(page%8)))) {

                DEBUGCHK(remain<=PAGE_SIZE);
                lpm->bRestart = 1;
                lpm->pDirty[page/8] &= ~(1<<(page%8));
                res = VirtualProtect(lpm->pBase+page*PAGE_SIZE, PAGE_SIZE,
                                     PAGE_READONLY, &dw1);
                DEBUGCHK(res);
                lpm->dwDirty--;
                DEBUGCHK(!(lpm->dwDirty & 0x80000000));

                LeaveCriticalSection(&PagerCS);

                if (!WriteFileWithSeek(lpm->hFile, lpm->pBase+page*PAGE_SIZE, remain,
                                       &dw1, 0, page*PAGE_SIZE, 0)
                    || (dw1 != remain)) {
                    DEBUGMSG(ZONE_PAGING,(L"FMB 3\r\n"));
                    LeaveCriticalSection(&WriterCS);
                    if (dwFlags & FMB_LEAVENAMECS)
                        LeaveCriticalSection(&MapNameCS);
                    DEBUGCHK(0);
                    goto exit;
                }

            } else
                LeaveCriticalSection(&PagerCS);

            DEBUGMSG(ZONE_PAGING,(L"FMB 4\r\n"));

            // Flush the dirty blocks in the cache back to the file
            FlushFileBuffers(lpm->hFile);
        }

        if (dwFlags & FMB_DOFULLDISCARD)
            DecommitROPages(lpm->pBase,lpm->reslen);
        LeaveCriticalSection(&WriterCS);
        if (dwFlags & FMB_LEAVENAMECS)
            LeaveCriticalSection(&MapNameCS);

    } else {
        //
        // No dirty memory
        //

        if ((lpm->hFile != INVALID_HANDLE_VALUE) && (dwFlags & FMB_DOFULLDISCARD))
            FSMapMemFree(lpm->pBase,lpm->reslen,MEM_DECOMMIT);
        if (dwFlags & FMB_LEAVENAMECS)
            LeaveCriticalSection(&MapNameCS);
    }

    retval = TRUE;

exit:
    SETCURKEY(ulOldKey);
    return retval;
}


BOOL KernelCloseHandle(HANDLE hFile)
{
    CALLBACKINFO cbi;

    cbi.hProc = ProcArray[0].hProc;
    cbi.pfn = (FARPROC)CloseHandle;
    cbi.pvArg0 = hFile;

    return (BOOL)PerformCallBack4Int(&cbi);
}


void FreeMap(HANDLE hMap)
{
    CLEANEVENT *lpce;
    LPFSMAP lpm, lpm2;
    HANDLE hm;

    lpm = HandleToMap(hMap);
    DEBUGCHK(lpm);

    EnterCriticalSection(&MapCS);

    // Remove from the list of maps
    if (hMapList == hMap)
        hMapList = HandleToMap(hMap)->hNext;
    else {
        for (hm = hMapList; hm; hm = lpm2->hNext) {
            lpm2 = HandleToMap(hm);
            DEBUGCHK(lpm2);
            if (lpm2->hNext == hMap) {
                lpm2->hNext = lpm->hNext;
                break;
            }
        }
        DEBUGCHK(hm);
    }

    LeaveCriticalSection(&MapCS);

    // Flush all dirty pages back to the file
    if (lpm->bNoAutoFlush)
        FlushMapBuffersLogged(lpm, 0, lpm->length, FMB_LEAVENAMECS);
    else
        FlushMapBuffers(lpm, 0, lpm->length, FMB_LEAVENAMECS);

    if ((lpm->hFile != INVALID_HANDLE_VALUE) && !lpm->bNoAutoFlush)
        KernelCloseHandle(lpm->hFile);

    if (lpm->name)
        FreeName(lpm->name);

    while (lpm->lpmlist) {
        lpce = lpm->lpmlist->ceptr;
        FreeMem(lpm->lpmlist, HEAP_CLEANEVENT);
        lpm->lpmlist = lpce;
    }

    FSMapMemFree(lpm->pBase, lpm->reslen, MEM_DECOMMIT);
    FSMapMemFree(lpm->pBase, lpm->reslen, MEM_RELEASE);
    FreeMem(lpm, HEAP_FSMAP);
    FreeHandle(hMap);
}


HANDLE FindMap(LPBYTE pMem)
{
    HANDLE hm;
    LPFSMAP lpm;

    EnterCriticalSection(&MapCS);

    for (hm = hMapList; hm; hm = lpm->hNext) {
        lpm = HandleToMap(hm);
        DEBUGCHK(lpm);
        if ((lpm->pBase <= pMem) && (pMem < lpm->pBase + lpm->length))
            break;
    }

    LeaveCriticalSection(&MapCS);

    return hm;
}


BOOL SC_UnmapViewOfFile(LPVOID lpvAddr)
{
    BOOL retval = FALSE;
    HANDLE hm;
    BOOL bLeft = 0;
    LPFSMAP lpm;
    CLEANEVENT *lpce, *lpce2;

    DEBUGMSG(ZONE_ENTRY,(L"SC_UnmapViewOfFile entry: %8.8lx\r\n", lpvAddr));

    EnterCriticalSection(&MapNameCS);

    if (hm = FindMap(lpvAddr)) {

        lpm = HandleToMap(hm);
        lpce2 = NULL;

        if (lpce = lpm->lpmlist) {
            if ((lpce->base == lpvAddr) && (lpce->size == (DWORD)pCurProc))
                lpm->lpmlist = lpce->ceptr;
            else {
                do {
                    lpce2 = lpce;
                    lpce = lpce->ceptr;
                } while (lpce && ((lpce->base != lpvAddr)
                                  || (lpce->size != (DWORD)pCurProc)));

                if (lpce)
                    lpce2->ceptr = lpce->ceptr;
            }
        }

        if (lpce) {
            FreeMem(lpce, HEAP_CLEANEVENT);
            if (DecRef(hm, pCurProc, FALSE)) {
                FreeMap(hm);
                bLeft = 1;
            }
            retval = TRUE;
        } else
            KSetLastError(pCurThread, ERROR_INVALID_ADDRESS);

    } else
        KSetLastError(pCurThread, ERROR_INVALID_ADDRESS);

    if (!bLeft)
        LeaveCriticalSection(&MapNameCS);

    DEBUGMSG(ZONE_ENTRY,(L"SC_UnmapViewOfFile exit: %8.8lx -> %8.8lx\r\n",
                         lpvAddr, retval));

    return retval;
}


BOOL SC_FlushViewOfFile(LPCVOID lpBaseAddress, DWORD dwNumberOfBytesToFlush)
{
    BOOL retval = FALSE;
    LPFSMAP lpm;
    HANDLE hm;

    DEBUGMSG(ZONE_ENTRY,(L"SC_FlushViewOfFile entry: %8.8lx %8.8lx\r\n",
                         lpBaseAddress, dwNumberOfBytesToFlush));

    EnterCriticalSection(&MapNameCS);

    if (hm = FindMap((LPBYTE)lpBaseAddress)) {
        lpm = HandleToMap(hm);
        DEBUGCHK(lpm);
        if (lpm->bNoAutoFlush) {
            retval = FlushMapBuffersLogged(lpm, (LPBYTE)lpBaseAddress-lpm->pBase,
                                           dwNumberOfBytesToFlush, FMB_LEAVENAMECS);
        } else
            retval = FlushMapBuffers(lpm, (LPBYTE)lpBaseAddress-lpm->pBase,
                                     dwNumberOfBytesToFlush, FMB_LEAVENAMECS);
    } else
        LeaveCriticalSection(&MapNameCS);
    DEBUGMSG(ZONE_ENTRY,(L"SC_FlushViewOfFile exit: %8.8lx %8.8lx -> %8.8lx\r\n",
                         lpBaseAddress, dwNumberOfBytesToFlush, retval));
    return retval;
}

#if (PAGE_SIZE == 1024)
    #define MAYBE_LIMIT_MEMORY 32
    #define MAYBE_LIMIT_DIRTY 72
#else
    #define MAYBE_LIMIT_MEMORY 10
    #define MAYBE_LIMIT_DIRTY 18
#endif


BOOL SC_FlushViewOfFileMaybe(LPCVOID lpBaseAddress, DWORD dwNumberOfBytesToFlush)
{
    extern long PageOutTrigger;
    BOOL retval = FALSE;
    LPFSMAP lpm;
    HANDLE hm;

    DEBUGMSG(ZONE_ENTRY,(L"SC_FlushViewOfFileMaybe entry: %8.8lx %8.8lx\r\n",
                         lpBaseAddress, dwNumberOfBytesToFlush));

    EnterCriticalSection(&MapNameCS);

    if (hm = FindMap((LPBYTE)lpBaseAddress)) {

        lpm = HandleToMap(hm);
        DEBUGCHK(lpm);

        if ((PageFreeCount >= MAYBE_LIMIT_MEMORY + PageOutTrigger)
            && (lpm->dwDirty < MAYBE_LIMIT_DIRTY)) {
            LeaveCriticalSection(&MapNameCS);
            retval = TRUE;

        } else if (lpm->bNoAutoFlush) {
            retval = FlushMapBuffersLogged(lpm, (LPBYTE)lpBaseAddress-lpm->pBase,
                                           dwNumberOfBytesToFlush, FMB_LEAVENAMECS);

        } else {
            retval = FlushMapBuffers(lpm, (LPBYTE)lpBaseAddress-lpm->pBase,
                                     dwNumberOfBytesToFlush, FMB_LEAVENAMECS);
        }

    } else
        LeaveCriticalSection(&MapNameCS);

    DEBUGMSG(ZONE_ENTRY,(L"SC_FlushViewOfFileMaybe exit: %8.8lx %8.8lx -> %8.8lx\r\n",
                         lpBaseAddress, dwNumberOfBytesToFlush, retval));

    return retval;
}


int MappedPageIn(BOOL bWrite, DWORD addr)
{
    HANDLE hm;
    LPFSMAP lpm;
    DWORD bread, len, len2, page;
    LPVOID pMem = 0, pMem2 = 0;
    BOOL bWritable, retval = 2;
    MEMORY_BASIC_INFORMATION mbi;
    ACCESSKEY ulOldKey;

    SWITCHKEY(ulOldKey,0xffffffff);
    addr = PAGEALIGN_DOWN(addr);

    EnterCriticalSection(&PagerCS);

    DEBUGMSG(ZONE_PAGING,(L"Taking Mapped PI fault: %8.8lx (%d)\r\n", addr, bWrite));

    VirtualQuery((LPVOID)addr, &mbi, sizeof(mbi));
    if (mbi.State == MEM_COMMIT) {
        if ((mbi.Protect == PAGE_READWRITE) || (mbi.Protect == PAGE_EXECUTE_READWRITE)
            || (!bWrite && ((mbi.Protect == PAGE_READONLY)
                            || (mbi.Protect == PAGE_EXECUTE_READ)))) {
            DEBUGMSG(ZONE_PAGING,(L"LPI: %8.8lx (%d) -> %d (1)\r\n", addr, bWrite, retval));
            LeaveCriticalSection(&PagerCS);
            return 1;
        }
    }

    if (!(hm = FindMap((LPBYTE)addr))) {
        DEBUGMSG(ZONE_PAGING,(L"LPI: %8.8lx (%d) -> 0 (4)\r\n",addr,bWrite));
        LeaveCriticalSection(&PagerCS);
        return 0;
    }

    lpm = HandleToMap(hm);
    DEBUGCHK(lpm);
    bWritable = ((lpm->hFile == INVALID_HANDLE_VALUE) || lpm->pDirty);
    if (lpm->hFile == INVALID_HANDLE_VALUE) {

        if (!(pMem = VirtualAlloc((LPVOID)addr, PAGE_SIZE, MEM_COMMIT, PAGE_READWRITE))) {
            DEBUGMSG(ZONE_PAGING, (L"    MPI: VA0 Failed!\r\n"));
            goto exit;
        }
        PagedInCount++;
        retval = 1;

    } else if (bWritable || !bWrite) {

        if (mbi.State == MEM_COMMIT) {
            if (!VirtualProtect((LPVOID)addr, PAGE_SIZE, PAGE_READWRITE, &bread)) {
                DEBUGMSG(ZONE_PAGING, (L"    MPI: VP1 Failed!\r\n"));
                goto exit;
            }

        } else {

            if (!(pMem = VirtualAlloc((LPVOID)ProcArray[0].dwVMBase, 0x10000,
                                      MEM_RESERVE, PAGE_NOACCESS))) {
                DEBUGMSG(ZONE_PAGING, (L"    MPI: VA1 Failed!\r\n"));
                goto exit;
            }

            pMem2 = (LPVOID)((DWORD)pMem + (addr&0xffff));
            if (!VirtualAlloc(pMem2, PAGE_SIZE, MEM_COMMIT, PAGE_READWRITE)) {
                DEBUGMSG(ZONE_PAGING, (L"    MPI: VA2 Failed!\r\n"));
                goto exit;
            }

            len = addr - (DWORD)lpm->pBase;
            lpm->bRestart = 0;

            LeaveCriticalSection(&PagerCS);

            // flush any logged mapfiles if too many pages are dirty
            if (bWrite && lpm->bNoAutoFlush && (lpm->dwDirty >= MAYBE_LIMIT_DIRTY - 1)) {
                FlushMapBuffersLogged(lpm, 0, 0, 0);
                retval = 3;
                goto exitnoleave;
            }

            len2 = ((DWORD)lpm->pBase + lpm->filelen - addr < PAGE_SIZE
                    ? (DWORD)lpm->pBase + lpm->filelen - addr
                    : PAGE_SIZE);
            if (!ReadFileWithSeek(lpm->hFile, pMem2, len2, &bread, 0, len, 0)
                || (bread != len2)) {
                DEBUGMSG(ZONE_PAGING,(L"    MPI: SFP Failed!\r\n"));
                retval = 3;
                goto exitnoleave;
            }

            EnterCriticalSection(&PagerCS);

            if (lpm->bRestart) {
                retval = 1;
                DEBUGMSG(ZONE_PAGING,(L"LPI: %8.8lx (%d) -> %d (restart)\r\n",
                                      addr, bWrite, retval));
                goto exit;
            }

            VirtualQuery((LPVOID)addr, &mbi, sizeof(mbi));
            if (mbi.State == MEM_COMMIT) {
                if ((mbi.Protect == PAGE_READWRITE) || (mbi.Protect == PAGE_EXECUTE_READWRITE)
                    || (!bWrite && ((mbi.Protect == PAGE_READONLY)
                                    || (mbi.Protect == PAGE_EXECUTE_READ))))
                    retval = 1;
                else
                    retval = 0;
                DEBUGMSG(ZONE_PAGING,(L"LPI: %8.8lx (%d) -> %d (1)\r\n",
                                      addr, bWrite, retval));
                goto exit;
            }

            if (!VirtualCopy((LPVOID)addr, pMem2, PAGE_SIZE,
                             bWrite ? PAGE_READWRITE : PAGE_READONLY)) {
                DEBUGMSG(ZONE_PAGING,(L"    MPI: VC Failed!\r\n"));
                goto exit;
            }

            PagedInCount++;
        }

        if (bWrite) {
            page = (addr - (DWORD)lpm->pBase + PAGE_SIZE - 1)/PAGE_SIZE;
            lpm->pDirty[page/8] |= (1<<(page%8));
            lpm->dwDirty++;
        }

        retval = 1;

    } else
        retval = 0;

exit:
    LeaveCriticalSection(&PagerCS);

exitnoleave:
    DEBUGMSG(ZONE_PAGING,(L"MappedPageIn exit: %8.8lx %8.8lx -> %8.8lx\r\n",
                          bWrite, addr, retval));

    if (pMem2)
        if (!VirtualFree(pMem2,PAGE_SIZE,MEM_DECOMMIT))
            DEBUGMSG(ZONE_PAGING,(L"  MapPage in failure in VF1!\r\n"));

    if (pMem)
        if (!VirtualFree(pMem,0,MEM_RELEASE))
            DEBUGMSG(ZONE_PAGING,(L"  MapPage in failure in VF2!\r\n"));

    SETCURKEY(ulOldKey);

    return retval;
}


void PageOutFile(void)
{
    HANDLE hMap;
    LPFSMAP lpm;
    EnterCriticalSection(&MapNameCS);
    DEBUGMSG(ZONE_PAGING,(L"MPO: Starting free count: %d\r\n",PageFreeCount));
    for (hMap = hMapList; hMap; hMap = lpm->hNext) {
        lpm = HandleToMap(hMap);
        DEBUGCHK(lpm);
        if (lpm->pDirty != (LPBYTE)1)
            FlushMapBuffers(lpm, 0, lpm->length,
                            FMB_DOFULLDISCARD | (lpm->bNoAutoFlush ? FMB_NOWRITEOUT : 0));
    }
    DEBUGMSG(ZONE_PAGING,(L"MPO: Ending page free count: %d\r\n",PageFreeCount));
    LeaveCriticalSection(&MapNameCS);
}


CLEANEVENT *pCFMCleanup;


BOOL TryCloseMappedHandle(HANDLE h)
{
    BOOL retval;
    CLEANEVENT *pce, *pce2;

    EnterCriticalSection(&MapNameCS);

    pce = pCFMCleanup;
    if (pce) {
        if ((pce->size == (DWORD)pCurProc) && (pce->base == h)) {
            pCFMCleanup = pce->ceptr;
            FreeMem(pce, HEAP_CLEANEVENT);
            retval = KernelCloseHandle(h);
            LeaveCriticalSection(&MapNameCS);
            return retval;
        }

        while (pce->ceptr) {
            if ((pce->ceptr->size == (DWORD)pCurProc) && (pce->ceptr->base == h)) {
                pce2 = pce->ceptr;
                pce->ceptr = pce2->ceptr;
                FreeMem(pce2, HEAP_CLEANEVENT);
                retval = KernelCloseHandle(h);
                LeaveCriticalSection(&MapNameCS);
                return retval;
            }

            pce = pce->ceptr;
        }
    }

    LeaveCriticalSection(&MapNameCS);

    KSetLastError(pCurThread, ERROR_INVALID_HANDLE);

    return FALSE;
}


void CloseMappedFileHandles()
{
    HANDLE hm;
    LPFSMAP lpm;
    CLEANEVENT *pce, *pce2;

    CloseHugeMemoryAreas(0);

    EnterCriticalSection(&MapNameCS);
    pce = pCFMCleanup;
    while (pce && (pce->size == (DWORD)pCurProc)) {
        pCFMCleanup = pce->ceptr;
        KernelCloseHandle(pce->base);
        FreeMem(pce,HEAP_CLEANEVENT);
        pce = pCFMCleanup;
    }

    if (pce) {
        while (pce->ceptr) {
            if (pce->ceptr->size == (DWORD)pCurProc) {
                pce2 = pce->ceptr;
                pce->ceptr = pce2->ceptr;
                KernelCloseHandle(pce2->base);
                FreeMem(pce2,HEAP_CLEANEVENT);
            } else
                pce = pce->ceptr;
        }
    }

    EnterCriticalSection(&MapCS);
    for (hm = hMapList; hm; hm = lpm->hNext) {

        lpm = HandleToMap(hm);
        DEBUGCHK(lpm);
        while ((pce = lpm->lpmlist) && (pce->size == (DWORD)pCurProc)) {
            lpm->lpmlist = pce->ceptr;
            FreeMem(pce, HEAP_CLEANEVENT);
        }

        if (lpm->lpmlist) {
            pce2 = lpm->lpmlist;
            pce = pce2->ceptr;
            while (pce) {
                if (pce->size == (DWORD)pCurProc) {
                    pce2->ceptr = pce->ceptr;
                    FreeMem(pce, HEAP_CLEANEVENT);
                    pce = pce2->ceptr;
                } else {
                    pce2 = pce;
                    pce = pce->ceptr;
                }           
            }
        }
    }

    LeaveCriticalSection(&MapCS);
    LeaveCriticalSection(&MapNameCS);
}   


HANDLE SC_CreateFileForMapping(LPCTSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
                               LPSECURITY_ATTRIBUTES lpSecurityAttributes, 
                               DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes,
                               HANDLE hTemplateFile)
{
    HANDLE hFile;
    CALLBACKINFO cbi;
    CLEANEVENT *pce;
    DWORD dwFlags;

    pce = AllocMem(HEAP_CLEANEVENT);
    if (!pce) {
        KSetLastError(pCurThread, ERROR_NOT_ENOUGH_MEMORY);
        return INVALID_HANDLE_VALUE;
    }

    // Prepare to create the file
    cbi.hProc = ProcArray[0].hProc;
    cbi.pfn = (FARPROC)CreateFile;
    cbi.pvArg0 = MapPtr(lpFileName);

    if (dwShareMode & FILE_SHARE_WRITE_OVERRIDE) {
        dwFlags = FILE_SHARE_WRITE | FILE_SHARE_READ;
    } else {
        dwFlags = (dwDesiredAccess == GENERIC_READ) ? FILE_SHARE_READ : 0;
    }

    // Create the file
    hFile = (HANDLE)PerformCallBack(&cbi, dwDesiredAccess, dwFlags, lpSecurityAttributes,
                                    dwCreationDisposition,
                                    dwFlagsAndAttributes | FILE_FLAG_WRITE_THROUGH,
                                    hTemplateFile);
    if (hFile != INVALID_HANDLE_VALUE) {
        // Add to mapper cleanup list
        EnterCriticalSection(&MapNameCS);
        pce->base = hFile;
        pce->size = (DWORD)pCurProc;
        pce->ceptr = pCFMCleanup;
        pCFMCleanup = pce;
        LeaveCriticalSection(&MapNameCS);
    } else
        FreeMem(pce, HEAP_CLEANEVENT);

    return hFile;
}


// flProtect can only combine PAGE_READONLY, PAGE_READWRITE, and SEC_COMMIT
// lpsa ignored

HANDLE SC_CreateFileMapping(HANDLE hFile, LPSECURITY_ATTRIBUTES lpsa,
                            DWORD flProtect, DWORD dwMaxSizeHigh,
                            DWORD dwMaxSizeLow, LPCTSTR lpName)
{
    HANDLE hMap = 0;
    LPFSMAP lpm = 0;
    BYTE testbyte;
    BOOL bPage = 1;
    DWORD len, realfilelen, reallen;
    PHDATA phd;
    LPBYTE pData = 0;
    BOOL bNoAutoFlush;

    DEBUGMSG(ZONE_ENTRY,(L"SC_CreateFileMapping entry: %8.8lx %8.8lx %8.8lx %8.8lx %8.8lx %8.8lx\r\n",
                         hFile, lpsa, flProtect, dwMaxSizeHigh, dwMaxSizeLow, lpName));

    bNoAutoFlush = (flProtect & PAGE_INTERNALDBMAPPING) ? 1 : 0;
    flProtect &= ~(SEC_COMMIT|PAGE_INTERNALDBMAPPING);

    // Remove from mapper cleanup list, if present
    if ((hFile != INVALID_HANDLE_VALUE) && !bNoAutoFlush) {
        CLEANEVENT *pce, *pce2;

        EnterCriticalSection(&MapNameCS);

        if (pce = pCFMCleanup) {
            if (pce->base == hFile) {
                pCFMCleanup = pce->ceptr;
                FreeMem(pce, HEAP_CLEANEVENT);
            } else {
                while (pce->ceptr && (pce->ceptr->base != hFile))
                    pce = pce->ceptr;
                if (pce2 = pce->ceptr) {
                    pce->ceptr = pce2->ceptr;
                    FreeMem(pce2, HEAP_CLEANEVENT);
                }
            }
        }

        LeaveCriticalSection(&MapNameCS);
    }

    // Reject if the file is empty (??)
    if ((hFile != INVALID_HANDLE_VALUE)
        && ((realfilelen = SetFilePointer(hFile, 0, 0, FILE_END)) == 0xffffffff)) {
        KernelCloseHandle(hFile);
        KSetLastError(pCurThread, ERROR_INVALID_PARAMETER);
        return 0;
    }

    // Determine the size the map should have
    if ((hFile == INVALID_HANDLE_VALUE) || dwMaxSizeLow)
        reallen = dwMaxSizeLow;
    else
        reallen = realfilelen;
    if (reallen & 0x80000000) {
        KernelCloseHandle(hFile);
        KSetLastError(pCurThread, ERROR_INVALID_PARAMETER);
        return 0;
    }

    // Clip the file to the new size, if necessary
    if ((hFile != INVALID_HANDLE_VALUE) && (realfilelen < reallen)) {
        if (!bNoAutoFlush && ((SetFilePointer(hFile, reallen, 0, FILE_BEGIN) != reallen)
                              || !SetEndOfFile(hFile))) {
            KernelCloseHandle(hFile);
            KSetLastError(pCurThread, ERROR_ACCESS_DENIED);
            return 0;
        }

        realfilelen = reallen;
    }

    //
    // Make sure the mapping doesn't already exist
    //

    EnterCriticalSection(&MapNameCS);

    if (lpName) {
        // Check name
        if ((len = strlenW(lpName)) > MAX_PATH - 1) {
            KSetLastError(pCurThread, ERROR_INVALID_PARAMETER);
            goto errexit;
        }

        EnterCriticalSection(&MapCS);

        for (hMap = hMapList; hMap; hMap = lpm->hNext) {
            lpm = HandleToMap(hMap);
            DEBUGCHK(lpm);
            if (lpm->name && !strcmpW(lpm->name->name, lpName)) {
                // Found an existing map
                IncRef(hMap, pCurProc);
                if (hFile != INVALID_HANDLE_VALUE)
                    KernelCloseHandle(hFile);
                KSetLastError(pCurThread, ERROR_ALREADY_EXISTS);
                LeaveCriticalSection(&MapCS);
                goto exit;
            }
        }
        KSetLastError(pCurThread, 0);

        LeaveCriticalSection(&MapCS);
    }

    LeaveCriticalSection(&MapNameCS);


    //
    // Not found, validate params outside critsecs to avoid deadlocks
    //
    if (hFile != INVALID_HANDLE_VALUE) {

        // Verify that we can read from the file (??)
        // As far as I can tell, ReadFileWithSeek(*,0,0,0,0,0,0) will always
        // return true!!!
        if (!ReadFileWithSeek(hFile, 0, 0, 0, 0, 0, 0)) {
            SetFilePointer(hFile, 0, 0, 0);
            if (realfilelen && (!ReadFile(hFile, &testbyte, 1, &len, 0)
                                || (len != 1))) {
                KernelCloseHandle(hFile);
                KSetLastError(pCurThread, ERROR_INVALID_HANDLE);
                return 0;
            }
            
            bPage = 0;
        }

        // Verify that we can write to the file (??)
        if (realfilelen && (flProtect == PAGE_READWRITE)) {
            if ((bPage && (!ReadFileWithSeek(hFile, &testbyte, 1, &len, 0, 0, 0)
                           || (len != 1)
                           || !WriteFileWithSeek(hFile, &testbyte, 1, &len, 0, 0, 0)
                           || (len != 1)))
                || (!bPage && (SetFilePointer(hFile, 0, 0, FILE_BEGIN)
                               || !WriteFile(hFile, &testbyte, 1, &len, 0)
                               || (len != 1)))) {
                KernelCloseHandle(hFile);
                KSetLastError(pCurThread, ERROR_ACCESS_DENIED);
                return 0;
            }
        }

        if (!bPage) {
            // Reserve memory for mapping
            if (!(pData = FSMapMemReserve(PAGEALIGN_UP(reallen)))) {
                if (hFile != INVALID_HANDLE_VALUE)
                    KernelCloseHandle(hFile);
                KSetLastError(pCurThread, ERROR_NOT_ENOUGH_MEMORY);
                return 0;
            }

            // Commit
            if (!FSMapMemCommit(pData, PAGEALIGN_UP(reallen), PAGE_READWRITE)) {
                FSMapMemFree(pData, PAGEALIGN_UP(reallen), MEM_RELEASE);
                if (hFile != INVALID_HANDLE_VALUE)
                    KernelCloseHandle(hFile);
                KSetLastError(pCurThread, ERROR_NOT_ENOUGH_MEMORY);
                return 0;
            }

            // Copy the file data into the mapped memory
            SetFilePointer(hFile, 0, 0, FILE_BEGIN);
            if (!ReadFile(hFile, pData, reallen, &len, 0) || (len != reallen)) {
                // Free up the memory we've grabbed
                FSMapMemFree(pData, PAGEALIGN_UP(reallen), MEM_DECOMMIT);
                FSMapMemFree(pData, PAGEALIGN_UP(reallen), MEM_RELEASE);
                if (hFile != INVALID_HANDLE_VALUE)
                    KernelCloseHandle(hFile);
                KSetLastError(pCurThread, ERROR_INVALID_HANDLE);
                return 0;
            }

            // Mark the memory as R/O, if necessary
            if (flProtect != PAGE_READWRITE)
                VirtualProtect(pData, PAGEALIGN_UP(reallen), PAGE_READONLY,&len);
        }
    }

    //
    // Check again to make sure the mapping doesn't already exist,
    // since not holding critsecs above
    //
    
    EnterCriticalSection(&MapNameCS);
    
    if (lpName) {
        // Is this check necessary??
        if ((len = strlenW(lpName)) > MAX_PATH - 1) {
            KSetLastError(pCurThread,ERROR_INVALID_PARAMETER);
            goto errexit;
        }

        EnterCriticalSection(&MapCS);

        for (hMap = hMapList; hMap; hMap = lpm->hNext) {
            lpm = HandleToMap(hMap);
            DEBUGCHK(lpm);
            if (lpm->name && !strcmpW(lpm->name->name, lpName)) {
                // Found an existing map
                IncRef(hMap, pCurProc);
                if (hFile != INVALID_HANDLE_VALUE)
                    KernelCloseHandle(hFile);
                KSetLastError(pCurThread, ERROR_ALREADY_EXISTS);
                LeaveCriticalSection(&MapCS);
                
                if (!bPage && pData) {
                    // Free up the memory we've grabbed
                    FSMapMemFree(pData, PAGEALIGN_UP(reallen), MEM_DECOMMIT);
                    FSMapMemFree(pData, PAGEALIGN_UP(reallen), MEM_RELEASE);
                }
                
                goto exit;
            }
        }
        KSetLastError(pCurThread,0);

        LeaveCriticalSection(&MapCS);
    }

    
    //
    // Prepare the map control struct
    //
    
    lpm = 0;
    hMap = 0;

    // Validate args (??)
    if (((flProtect != PAGE_READONLY) && (flProtect != PAGE_READWRITE))
        || dwMaxSizeHigh
        || ((hFile == INVALID_HANDLE_VALUE) && !dwMaxSizeLow)
        || ((flProtect == PAGE_READONLY) && (hFile == INVALID_HANDLE_VALUE))) {
        KSetLastError(pCurThread,ERROR_INVALID_PARAMETER);
        goto errexit;
    }

    // Allocate space for the map struct
    if (!(lpm = (LPFSMAP)AllocMem(HEAP_FSMAP))) {
        KSetLastError(pCurThread,ERROR_NOT_ENOUGH_MEMORY);
        goto errexit;
    }

    lpm->lpmlist = 0;
    lpm->bNoAutoFlush = bNoAutoFlush;
    lpm->dwDirty = 0;
    lpm->pBase = NULL;

    // Copy the name
    if (lpName) {
        // Is this check necessary??
        if (strlenW(lpName) > MAX_PATH - 1) {
            KSetLastError(pCurThread, ERROR_INVALID_PARAMETER);
            goto errexit;
        }
        
        if (!(lpm->name = (Name *)AllocName((strlenW(lpName) + 1) * 2))) {
            KSetLastError(pCurThread, ERROR_NOT_ENOUGH_MEMORY);
            goto errexit;
        }
        kstrcpyW(lpm->name->name, lpName);
    } else
        lpm->name = 0;

    if (!(hMap = AllocHandle(&cinfMap, lpm, pCurProc))) {
        KSetLastError(pCurThread, ERROR_NOT_ENOUGH_MEMORY);
        goto errexit;
    }

    lpm->length = reallen;
    lpm->reslen = PAGEALIGN_UP(lpm->length);

    // Leave space for dirty bits
    if ((flProtect == PAGE_READWRITE) && (hFile != INVALID_HANDLE_VALUE))
        lpm->reslen += PAGEALIGN_UP(((lpm->reslen + PAGE_SIZE - 1) / PAGE_SIZE + 7) / 8); // one bit per page
    if (!lpm->reslen)
        lpm->reslen = PAGE_SIZE;

    // If we haven't already reserved memory for mapping, do it now
    if (bPage) {
        // Reserve memory for mapping
        if (!(lpm->pBase = FSMapMemReserve(lpm->reslen))) {
            KSetLastError(pCurThread, ERROR_NOT_ENOUGH_MEMORY);
            goto errexit;
        }

        // Commit
        if ((flProtect == PAGE_READWRITE) && (hFile != INVALID_HANDLE_VALUE)) {
            lpm->pDirty = lpm->pBase + PAGEALIGN_UP(lpm->length);
            lpm->bRestart = 0;
            if (!FSMapMemCommit(lpm->pDirty, lpm->reslen - PAGEALIGN_UP(lpm->length),
                                PAGE_READWRITE)) {
                KSetLastError(pCurThread, ERROR_NOT_ENOUGH_MEMORY);
                goto errexit;
            }
        } else
            lpm->pDirty = 0;
    
    } else {
        // We already reserved memory, so fill in the struct
        lpm->pBase = pData;
        lpm->bRestart = 0;
        lpm->pDirty = (flProtect == PAGE_READWRITE) ? (LPBYTE)1 : 0;
    }

    // Final sanity checks on the file
    if ((lpm->hFile = hFile) != INVALID_HANDLE_VALUE) {
        phd = HandleToPointer(hFile);
        if ((phd->lock != 1) || (phd->ref.count != 1)) {
            RETAILMSG(1,(L"CreateFileMapping called with handle not created with " \
                         L"CreateFileForMapping!\r\n"));
            KSetLastError(pCurThread, ERROR_INVALID_PARAMETER);
            goto errexit;
        }
        
        if ((lpm->filelen = realfilelen) == 0xFFFFFFFF) {
            KSetLastError(pCurThread, ERROR_INVALID_PARAMETER);
            goto errexit;
        }
    }

    // Add to the list of mapped files
    EnterCriticalSection(&MapCS);
    lpm->hNext = hMapList;
    hMapList = hMap;
    LeaveCriticalSection(&MapCS);

exit:
    LeaveCriticalSection(&MapNameCS);
    
    if (bNoAutoFlush) {
        if (!ValidateFile(lpm)) {
            // We cannot commit the file.  Fail.
            goto errexitnocs;
        }
    }
    DEBUGMSG(ZONE_ENTRY,(L"SC_CreateFileMapping exit: %8.8lx\r\n", hMap));
    
    return hMap;

errexit:
    LeaveCriticalSection(&MapNameCS);

errexitnocs:

    // Remove from the list of maps, if it's there
    if (hMap && lpm) {
        EnterCriticalSection(&MapNameCS);
        EnterCriticalSection(&MapCS);
        
        if (hMapList == hMap)
            hMapList = HandleToMap(hMap)->hNext;
        else {
            HANDLE hmTemp = 0;
            LPFSMAP lpmTemp = 0;
            
            for (hmTemp = hMapList; hmTemp; hmTemp = lpmTemp->hNext) {
                lpmTemp = HandleToMap(hmTemp);
                DEBUGCHK(lpmTemp);
                if (lpmTemp->hNext == hMap) {
                    lpmTemp->hNext = lpm->hNext;
                    break;
                }
            }
        }
        
        LeaveCriticalSection(&MapCS);
        LeaveCriticalSection(&MapNameCS);
    }


    // Free up allocated memory
    if (lpm) {
        if (lpm->name)
            FreeName(lpm->name);
        if (hMap) {
            FreeHandle(hMap);
        }
        if (lpm->pBase) {
            FSMapMemFree(lpm->pBase, lpm->reslen, MEM_DECOMMIT);
            FSMapMemFree(lpm->pBase, lpm->reslen, MEM_RELEASE);
        }
        FreeMem(lpm,HEAP_FSMAP);
    }

    if (!bPage && pData) {
        FSMapMemFree(pData, PAGEALIGN_UP(reallen), MEM_DECOMMIT);
        FSMapMemFree(pData, PAGEALIGN_UP(reallen), MEM_RELEASE);
    }

    if (hFile != INVALID_HANDLE_VALUE)
        KernelCloseHandle(hFile);

    DEBUGMSG(ZONE_ENTRY,(L"SC_CreateFileMapping exit: %8.8lx\r\n", 0));

    return 0;
}


BOOL SC_MapCloseHandle(HANDLE hMap)
{
    DEBUGMSG(ZONE_ENTRY,(L"SC_MapCloseHandle entry: %8.8lx\r\n", hMap));

    EnterCriticalSection(&MapNameCS);
    if (DecRef(hMap, pCurProc, FALSE))
        FreeMap(hMap);
    else
        LeaveCriticalSection(&MapNameCS);

    DEBUGMSG(ZONE_ENTRY,(L"SC_MapCloseHandle exit: %8.8lx -> %8.8lx\r\n", hMap, TRUE));

    return TRUE;
}


ERRFALSE(offsetof(fslog_t,dwRestoreStart) == offsetof(fslog_t,dwRestoreFlags) + 4);
ERRFALSE(offsetof(fslog_t,dwRestoreSize) == offsetof(fslog_t,dwRestoreFlags) + 8);


// This function is only run on non-auto-flush maps, ie. mapped database files.
BOOL ValidateFile(LPFSMAP lpm)
{
    // This struct contains the state for restoring the file
    struct {
        DWORD dwRestoreFlags;
        DWORD dwRestoreStart;
        DWORD dwRestoreSize;
    } FlushStruct;

    // This struct is used to copy pages of data
    struct {
        DWORD dataoffset;
        BYTE restorepage[4096]; // must be last!
    } RestStruct;

    DWORD bread, dwToWrite, offset, size, count;

    // Read restore data
    if (!ReadFileWithSeek(lpm->hFile, &FlushStruct, sizeof(FlushStruct), &bread, 0, 
                         offsetof(fslog_t,dwRestoreFlags), 0)
        || (bread != sizeof(FlushStruct))) {
        // This shouldn't happen.
        DEBUGCHK(0);
        return FALSE;
    }

    // Now act on the restore state
    switch (FlushStruct.dwRestoreFlags) {
    case RESTORE_FLAG_FLUSHED:

        // The whole file has been flushed, but the dirty pages were flushed to
        // the end of the file for safety.  Now we must move all of those pages
        // into their proper places within the file.

        offset = FlushStruct.dwRestoreStart;
        count = FlushStruct.dwRestoreSize>>16;
        FlushStruct.dwRestoreSize &= 0xffff;
        size = (DWORD)&RestStruct.restorepage - (DWORD)&RestStruct + FlushStruct.dwRestoreSize;

        // Move each dirty page from the end of the file to its proper place.
        while (count--
               && ReadFileWithSeek(lpm->hFile, &RestStruct, size, &bread, 0, offset, 0)
               && (bread == size)) {

            dwToWrite = ((RestStruct.dataoffset + FlushStruct.dwRestoreSize <= FlushStruct.dwRestoreStart)
                         ? FlushStruct.dwRestoreSize
                         : ((RestStruct.dataoffset < FlushStruct.dwRestoreStart)
                            ? (FlushStruct.dwRestoreStart - RestStruct.dataoffset)
                            : 0));

            if (!WriteFileWithSeek(lpm->hFile, &RestStruct.restorepage[0],
                                   dwToWrite, &bread, 0, RestStruct.dataoffset, 0)
                || (bread != dwToWrite)) {
                ERRORMSG(1,(L"Failed to commit page on ValidateFile!\r\n"));
                
                if (count + 1 == FlushStruct.dwRestoreSize >> 16) {
                    // We haven't actually flushed anything yet, so the state
                    // is still consistent.
                    return FALSE;
                }

                




                DEBUGCHK(0);
                return FALSE;
            }

            offset += size;
        }

        FlushFileBuffers(lpm->hFile);
        
        // intentionally fall through

    case RESTORE_FLAG_UNFLUSHED:

        // Clear the restore flag
        FlushStruct.dwRestoreFlags = RESTORE_FLAG_NONE;
        if (!WriteFileWithSeek(lpm->hFile, &FlushStruct.dwRestoreFlags,
                               sizeof(DWORD), &bread, 0,
                               offsetof(fslog_t,dwRestoreFlags), 0)
            || (bread != sizeof(DWORD))) {
            ERRORMSG(1,(L"Failed to clear flags on ValidateFile!\r\n"));                    
            
            

            return FALSE;
        }

        FlushFileBuffers(lpm->hFile);
    }

    return TRUE;
}











#define FMBL_FLUSH_SKIP_LIMIT   (PAGES_PER_BLOCK*2)


BOOL FlushMapBuffersLogged(LPFSMAP lpm, DWORD dwOffset, DWORD dwLength, DWORD dwFlags)
{
    DWORD remain, page, end, dw1, count;
    BOOL retval = FALSE;
    HANDLE h;
    LPBYTE pb;
    ACCESSKEY ulOldKey;

    SWITCHKEY(ulOldKey,0xffffffff);

    //
    // Figure out where we're starting and stopping, page-aligned
    //

    if (!dwLength)
        dwLength = lpm->length;
    end = dwOffset+dwLength;
    dwOffset = PAGEALIGN_DOWN(dwOffset);
    end = PAGEALIGN_UP(end);
    if (end > lpm->length)
        end = lpm->length;
    if (end < dwOffset) {
        if (dwFlags & FMB_LEAVENAMECS)
            LeaveCriticalSection(&MapNameCS);
        return FALSE;
    }


    if (lpm->pDirty == (LPBYTE)1) {
        //
        // Old-style (non-transactionable) file system
        //

        ERRORMSG(1,(L"FlushMapBuffersLogged being used on old-style " \
                    L"(non-transactionable) file system!\r\n"));
        h = lpm->hFile;
        pb = lpm->pBase;
        if (dwFlags & FMB_LEAVENAMECS)
            LeaveCriticalSection(&MapNameCS);
        SetFilePointer(h, dwOffset, 0, FILE_BEGIN);
        if (!WriteFile(h, pb, end-dwOffset, &remain, 0) || (remain != end-dwOffset)) {
            DEBUGMSG(ZONE_PAGING,(L"FMB 5\r\n"));
            DEBUGCHK(0);
            goto exit;
        }
        FlushFileBuffers(h);
        retval = TRUE;

    } else if (lpm->pDirty) {
        //
        // The file is read/write -- we may have dirty pages to flush
        //

        EnterCriticalSection(&WriterCS);

        if (!(dwFlags & FMB_NOWRITEOUT)) {

            DWORD dwFilePos, dwMemPos;
            
            // This struct is used to communicate with ValidateFile.
            // Only dwRestoreStart (which we do not modify) is used by any
            // other part of the file system.
            struct {
                DWORD dwRestoreFlags;
                DWORD dwRestoreStart;
                DWORD dwRestoreSize;
            } FlushStruct;

            // Track pages that are skipped
            DWORD dwSkippedPage[FMBL_FLUSH_SKIP_LIMIT];  // List of page nums
            WORD wSkippedPages = 0;                      // Count of pages
            WORD wNextSkip;                              // Used to iterate list

            count = 0;


            //
            // Read and update restore data
            //

            DEBUGMSG(ZONE_PAGING, (L"FMB 1\r\n"));
            if (!ReadFileWithSeek(lpm->hFile,&FlushStruct.dwRestoreStart, sizeof(DWORD),
                                  &dw1, 0, offsetof(fslog_t,dwRestoreStart), 0)
                || (dw1 != sizeof(DWORD))) {
                // We can exit here because nothing has been changed.
                RETAILMSG(1,(L"Read 0 failed on attempt at logged flush!\r\n"));
                LeaveCriticalSection(&WriterCS);
                if (dwFlags & FMB_LEAVENAMECS)
                    LeaveCriticalSection(&MapNameCS);
                
                // debugchk because this shouldn't happen
                DEBUGCHK(0);
                
                goto exit;
            }
            dwFilePos = FlushStruct.dwRestoreStart;
            FlushStruct.dwRestoreFlags = RESTORE_FLAG_UNFLUSHED;
            FlushStruct.dwRestoreSize = PAGE_SIZE;
            if (!WriteFileWithSeek(lpm->hFile, &FlushStruct, sizeof(FlushStruct),
                                   &dw1, 0, offsetof(fslog_t,dwRestoreFlags), 0)
                || (dw1 != sizeof(FlushStruct))) {
                // We can exit here because nothing has been changed.
                RETAILMSG(1,(L"Write 0 failed on attempt at logged flush!\r\n"));
                LeaveCriticalSection(&WriterCS);
                if (dwFlags & FMB_LEAVENAMECS)
                    LeaveCriticalSection(&MapNameCS);
                goto exit;
            }


            //
            // Now flush the pages in memory back to the file.  We write the
            // new pages to the end of the file for safety, and only copy them
            // into their rightful places when we are done, inside ValidateFile.
            // We cannot clear the dirty flags until all pages are successfully
            // flushed, so we only update their dirty flags at the end.
            //

            // Flush the dirty blocks in the cache back to the file
            FlushFileBuffers(lpm->hFile);

            EnterCriticalSection(&PagerCS);
            
            for (page = dwOffset/PAGE_SIZE; (page+1)*PAGE_SIZE <= end; page++) {

                // Flush the page only if it is dirty
                if (lpm->pDirty[page/8] & (1<<(page%8))) {

                    // If it's the first page, make sure the restore data is correct.
                    if (!page)
                        memcpy(lpm->pBase + offsetof(fslog_t,dwRestoreFlags),
                               &FlushStruct, sizeof(FlushStruct));
                    
                    if (VirtualProtect(lpm->pBase+page*PAGE_SIZE, PAGE_SIZE,
                                       PAGE_READONLY, &dw1)) {

                        LeaveCriticalSection(&PagerCS);

                        // Write the page's location to the start of the page
                        dwMemPos = page*PAGE_SIZE;
                        if (!WriteFileWithSeek(lpm->hFile, &dwMemPos, sizeof(DWORD),
                                               &dw1, 0, dwFilePos, 0)
                            || (dw1 != sizeof(DWORD))) {
                            // We can exit here because nothing has really changed.
                            RETAILMSG(1, (L"Write 1 failed on attempt at logged flush!\r\n"));
                            goto exitUnlock;
                        }

                        // Now write the page of data
                        dwFilePos += sizeof(DWORD);
                        if (!WriteFileWithSeek(lpm->hFile, lpm->pBase+page*PAGE_SIZE, PAGE_SIZE,
                                               &dw1, 0, dwFilePos, 0)
                            || (dw1 != PAGE_SIZE)) {
                            // We can exit here because nothing has really changed.
                            RETAILMSG(1,(L"Write 2 failed on attempt at logged flush!\r\n"));
                            goto exitUnlock;
                        }

                        dwFilePos += PAGE_SIZE;
                        count++;

                        EnterCriticalSection(&PagerCS);
                    
                    } else {
                        // Failed the VirtualProtect - skip the page
                        if (wSkippedPages < FMBL_FLUSH_SKIP_LIMIT) {
                            dwSkippedPage[wSkippedPages] = page;
                            wSkippedPages++;
                        } else {
                            // Reached our limit; this shouldn't happen
                            RETAILMSG(1, (TEXT("Logged flush failures exceeded limit! (file 0x%08x)\r\n"),
                                          lpm->hFile));
                            DEBUGCHK(0);
                        }
                    }
                }
            }

            // Flush the incomplete page at the end, if present
            remain = end - (page*PAGE_SIZE);
            if (remain && (lpm->pDirty[page/8] & (1<<(page%8)))) {

                DEBUGCHK(remain <= PAGE_SIZE);

                // If it's the first page, make sure the restore data is correct.
                if (!page)
                    memcpy(lpm->pBase + offsetof(fslog_t,dwRestoreFlags), 
                           &FlushStruct, sizeof(FlushStruct));

                if (VirtualProtect(lpm->pBase+page*PAGE_SIZE, PAGE_SIZE, 
                                   PAGE_READONLY, &dw1)) {

                    LeaveCriticalSection(&PagerCS);

                    // Write the page's location to the start of the page
                    dwMemPos = page*PAGE_SIZE;
                    if (!WriteFileWithSeek(lpm->hFile, &dwMemPos, sizeof(DWORD),
                                           &dw1, 0, dwFilePos, 0)
                        || (dw1 != sizeof(DWORD))) {
                        // We can exit here because nothing has really changed.
                        RETAILMSG(1, (L"Write 3 failed on attempt at logged flush!\r\n"));
                        goto exitUnlock;
                    }

                    // Now write the page of data
                    dwFilePos += sizeof(DWORD);
                    if (!WriteFileWithSeek(lpm->hFile, lpm->pBase+page*PAGE_SIZE, PAGE_SIZE,
                                           &dw1, 0, dwFilePos, 0)
                        || (dw1 != PAGE_SIZE)) {
                        // We can exit here because nothing has really changed.
                        RETAILMSG(1, (L"Write 4 failed on attempt at logged flush!\r\n"));
                        goto exitUnlock;
                    }

                    dwFilePos += PAGE_SIZE;
                    count++;
                
                } else {
                    // Failed the VirtualProtect - skip the page
                    if (wSkippedPages < FMBL_FLUSH_SKIP_LIMIT) {
                        dwSkippedPage[wSkippedPages] = page;
                        wSkippedPages++;
                    } else {
                        // Reached our limit; this shouldn't happen
                        RETAILMSG(1, (TEXT("Logged flush failures exceeded limit! (file 0x%08x)\r\n"),
                                      lpm->hFile));
                        DEBUGCHK(0);
                    }
                    
                    LeaveCriticalSection(&PagerCS);
                }

            } else
                LeaveCriticalSection(&PagerCS);

            // Flush the dirty blocks in the cache back to the file
            FlushFileBuffers(lpm->hFile);


            //
            // Update restore data again
            //

            FlushStruct.dwRestoreFlags = RESTORE_FLAG_FLUSHED;
            FlushStruct.dwRestoreSize |= count<<16;
            if (!WriteFileWithSeek(lpm->hFile, &FlushStruct, sizeof(FlushStruct),
                                   &dw1, 0, offsetof(fslog_t,dwRestoreFlags), 0)
                || (dw1 != sizeof(FlushStruct))) {
                // We can exit here because nothing has really changed.
                RETAILMSG(1,(L"Write 5 failed on attempt at logged flush!\r\n"));
                goto exitUnlock;
            }


            // Flush the dirty blocks in the cache back to the file
            FlushFileBuffers(lpm->hFile);

            DEBUGMSG(ZONE_PAGING, (L"FMB 4\r\n"));

            
            // Commit the flush.
            if (!ValidateFile(lpm)) {
                // We cannot validate the file, so we must fail the flush!
                goto exitUnlock;
            }
        
        
            //
            // Now that everything is totally done, we can mark the blocks
            // as clean.
            //
            
            DEBUGMSG(wSkippedPages, (TEXT("FlushMapBuffersLogged: skipped %u pages\r\n"), wSkippedPages));
            if (wSkippedPages > 0) {
                wNextSkip = 0;
            } else {
                wNextSkip = FMBL_FLUSH_SKIP_LIMIT;
            }

            EnterCriticalSection(&PagerCS);
            for (page = dwOffset/PAGE_SIZE; (page+1)*PAGE_SIZE <= end; page++) {
                if (lpm->pDirty[page/8] & (1<<(page%8))) {
                    if ((wNextSkip < wSkippedPages)
                        && (dwSkippedPage[wNextSkip] == page)) {
                        // The page was skipped -- leave it marked dirty
                        wNextSkip++;
                    } else {
                        // Mark as clean
                        lpm->bRestart = 1;
                        lpm->pDirty[page/8] &= ~(1<<(page%8));
                        lpm->dwDirty--;
                        DEBUGCHK(!(lpm->dwDirty & 0x80000000));
                    }
                }
            }
            remain = end - (page*PAGE_SIZE);
            if (remain && (lpm->pDirty[page/8] & (1<<(page%8)))) {
                if ((wNextSkip < wSkippedPages)
                    && (dwSkippedPage[wNextSkip] == page)) {
                    // The page was skipped -- leave it marked dirty
                    wNextSkip++;
                } else {
                    // Mark as clean
                    lpm->bRestart = 1;
                    lpm->pDirty[page/8] &= ~(1<<(page%8));
                    lpm->dwDirty--;
                    DEBUGCHK(!(lpm->dwDirty & 0x80000000));
                }
            }
            DEBUGCHK((wNextSkip == wSkippedPages) || (wNextSkip == FMBL_FLUSH_SKIP_LIMIT));
            DEBUGCHK(!(lpm->dwDirty) || (wSkippedPages > 0));
            LeaveCriticalSection(&PagerCS);
        }

        if (dwFlags & FMB_DOFULLDISCARD)
            DecommitROPages(lpm->pBase, lpm->reslen);
        LeaveCriticalSection(&WriterCS);
        if (dwFlags & FMB_LEAVENAMECS)
            LeaveCriticalSection(&MapNameCS);

    } else {
        //
        // The file is read-only (no dirty pages to flush)
        //

        if ((lpm->hFile != INVALID_HANDLE_VALUE) && (dwFlags & FMB_DOFULLDISCARD))
            FSMapMemFree(lpm->pBase, lpm->reslen, MEM_DECOMMIT);
        if (dwFlags & FMB_LEAVENAMECS)
            LeaveCriticalSection(&MapNameCS);
    }

    // Successful return
    retval = TRUE;
    goto exit;

exitUnlock:
    // If an error occurred after some pages were VirtualProtected, we must
    // go back and unlock them.
    
    // File must be read/write
    DEBUGCHK(lpm->pDirty > (LPBYTE)1);
        
    RETAILMSG(1, (TEXT("FlushMapBuffersLogged: Unlocking protected pages\r\n")));
    
    EnterCriticalSection(&PagerCS);

    // Here we assume that all dirty pages should be read/write...
    for (page = dwOffset/PAGE_SIZE; (page+1)*PAGE_SIZE <= end; page++) {
        if ((lpm->pDirty[page/8] & (1<<(page%8)))
            && !VirtualProtect(lpm->pBase + page*PAGE_SIZE, PAGE_SIZE,
                               PAGE_READWRITE, &dw1)) {
            // Is it possible to fail a call to VirtualProtect?
            DEBUGCHK(0);
        }
    }
    remain = end - (page*PAGE_SIZE);
    if (remain && (lpm->pDirty[page/8] & (1<<(page%8)))
        && !VirtualProtect(lpm->pBase + page*PAGE_SIZE, PAGE_SIZE,
                           PAGE_READWRITE, &dw1)) {
        // Is it possible to fail a call to VirtualProtect?
        DEBUGCHK(0);
    }
    
    LeaveCriticalSection(&PagerCS);
    
    // Flush the dirty blocks in the cache back to the file
    FlushFileBuffers(lpm->hFile);

    LeaveCriticalSection(&WriterCS);
    if (dwFlags & FMB_LEAVENAMECS)
        LeaveCriticalSection(&MapNameCS);

exit:
    SETCURKEY(ulOldKey);
    return retval;
}

