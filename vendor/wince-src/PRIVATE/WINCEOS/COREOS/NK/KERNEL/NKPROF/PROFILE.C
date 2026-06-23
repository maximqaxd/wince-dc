/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

Module Name:  profile.c

Abstract:  
 This file implements the NK kernel profiler support

Functions:


Notes: 

--*/
#define C_ONLY
#include "kernel.h"	   
#include "romldr.h"
#include "xtime.h"

#define NUM_SYMBOLS 500					// max number of symbols to display in report

#define PROFILEMSG(cond,printf_exp)   \
   ((cond)?(NKDbgPrintfW printf_exp),1:0)

extern BOOL bProfileBuffer;				// profiling to buffer flag

										// platform routine to disable profiler timer
extern void OEMProfileTimerDisable(void);
										// platform routine to enable profiler timer
extern void OEMProfileTimerEnable(DWORD dwUSec);

typedef struct {
    DWORD ra;                           // Interrupt program counter
    TOCentry * pte;
} PROFBUFENTRY, *PPROFBUFENTRY;

#define ENTRIES_PER_PAGE    (PAGE_SIZE / sizeof(PROFBUFENTRY))

//
// The number of bits to shift is a page shift - log2(sizeof(PROFBUFENTRY))
//
#if PAGE_SIZE == 4096
  #define PROF_PAGE_SHIFT       9
#elif PAGE_SIZE == 2048
  #define PROF_PAGE_SHIFT       8
#elif PAGE_SIZE == 1024
  #define PROF_PAGE_SHIFT       7
#else
  #error "Unsupported Page Size"
#endif

#define PROF_ENTRY_MASK       ((2 << PROF_PAGE_SHIFT) - 1)

PDWORD g_pdwProfileBufStart;            // platform profile buffer start
HANDLE g_hProfileBuf;

DWORD dwProfileCount;					// total number of profiler calls
DWORD  dwPerProcess;					// per process profiling flag
void ClearProfileHits(void);			// clear all profile counters

#define PROFILE_BUFFER_MAX   4096*1024
#define PAGES_IN_MAX_BUFFER    PROFILE_BUFFER_MAX/PAGE_SIZE

PPROFBUFENTRY g_pPBE[PAGES_IN_MAX_BUFFER];
DWORD  dwBufNumPages;              // Total number of pages
DWORD  dwBufNumEntries;            // Total number of entries
DWORD  dwBufRecDropped;            // How many hits were not recorded?

/*
 *	ProfilerHit - record profiler interrupt call address
 *
 *	Input:	ra - interrupt return address
 *			tocexeptr - pCurProf.oe->tocexeptr at time of interrupt
 *
 *	Attempts to find symbol address for the interrupt call address and increments
 *  hit counts for module and symbol if found.
 */
void ProfilerSymbolHit(unsigned int ra, TOCentry *tocexeptr) {
	TOCentry 	*tocptr;				// table of contents entry pointer
	PROFentry	*profptr=NULL;			// profile entry pointer
	SYMentry	*symptr;				// profiler symbol entry pointer
	unsigned int iMod;					// module enty in TOC
	unsigned int iSym;					// symbol counter
    SYMentry    *pClosestSym;           // nearest symbol entry found

	if (ra & 0x80000000) {				// if high bit set, this is NK
		ra&=0xdfffffff;					// mask off uncached bit
										// profptr-> first entry= NK
		profptr= (PROFentry *)pTOC->ulProfileOffset;
										// if ra >= pTOC->dllfirst, the hit
										// is in a dll in the ROM
	}else if ((unsigned int)ra >= (unsigned int)pTOC->dllfirst) {
		profptr= (PROFentry *)pTOC->ulProfileOffset+1;
										// skip exe's which start at 0x10000
										// dll's are stored in descending addresses
		while (profptr->ulStartAddr == 0 || profptr->ulStartAddr == 0x10000 || (unsigned int)ra<(unsigned int)profptr->ulStartAddr)
			profptr++;
	} else {							// this is an exe in the ROM, something 
										// loaded in RAM or unknown
		tocptr=(TOCentry *)(pTOC+1);	// tocptr-> first entry in ROM
		if (tocexeptr < tocptr) {		// if tocexeptr at interrupt time < first entry
			return;						// we don't know what this is
		}
										// compute module number
		iMod= ((DWORD)tocexeptr-(DWORD)tocptr)/sizeof(TOCentry);
		if (!iMod) {
			return;
		}
		if (iMod > pTOC->nummods) {		// if module > number of modules in ROM
			return;						// this was loaded in RAM
		}
										// make profptr point to entry for this module
		profptr= (PROFentry *)pTOC->ulProfileOffset;
		profptr+=iMod;
	}

	profptr->ulHits++;					// increment hits for this module
	if (profptr->ulNumSym) {
		// look up the symbol if module found
		iSym= profptr->ulNumSym-1;
		symptr=(SYMentry*)profptr->ulHitAddress;
        pClosestSym = symptr;
            
        // Scan the whole list of symbols, looking for the closest match.
        while (iSym) {
            // Keep track of the closest symbol found
            if (((unsigned int)symptr->ulFuncAddress <= ra)
                && (symptr->ulFuncAddress > pClosestSym->ulFuncAddress)) {
                pClosestSym = symptr;
            }
            
            iSym--;
            symptr++;
        }
        
        pClosestSym->ulFuncHits++;		// inc hit count for this symbol
	}
	return;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
BOOL
ProfilerFreeBuffer(void)
{
    DWORD dwMemSize;
    BOOL fRet;

    dwMemSize = dwBufNumPages * PAGE_SIZE;

    fRet = SC_UnlockPages(g_pdwProfileBufStart, dwMemSize);
    ASSERT(fRet);
    fRet = UnmapViewOfFile(g_pdwProfileBufStart);
    ASSERT(fRet);
    fRet = CloseHandle(g_hProfileBuf);
    ASSERT(fRet);
    
    return TRUE;
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
BOOL
ProfilerAllocBuffer(void)
{
    DWORD dwMemRequest;
    BOOL fRet;
    DWORD i;

    //
    // Determine half the free RAM.
    //
    dwMemRequest = (UserKInfo[KINX_PAGEFREE] * PAGE_SIZE) / 2;
    //
    // Make an upper limit.
    //
    if (dwMemRequest > PROFILE_BUFFER_MAX) {
        dwMemRequest = PROFILE_BUFFER_MAX;
    }
    dwBufNumPages   = dwMemRequest / PAGE_SIZE;
    dwBufNumEntries = dwBufNumPages * ENTRIES_PER_PAGE;

    //
    // Create a shared memory-mapped object
    //
    g_hProfileBuf = CreateFileMapping((HANDLE) -1, NULL, 
                                     PAGE_READWRITE | SEC_COMMIT, 0,
                                     dwMemRequest, NULL);
    if (!g_hProfileBuf) {
        goto EXIT_FALSE;
    }
    
    // Map in the object
    g_pdwProfileBufStart = (PDWORD) MapViewOfFile(g_hProfileBuf, FILE_MAP_ALL_ACCESS, 0, 0, 0);

    if (g_pdwProfileBufStart == NULL) {
        CloseHandle(g_hProfileBuf);
        goto EXIT_FALSE;
    }

    //
    // Lock the pages and get the physical addresses.
    //
    fRet = SC_LockPages(g_pdwProfileBufStart, dwMemRequest, (PDWORD) g_pPBE, LOCKFLAG_READ | LOCKFLAG_WRITE);

    //
    // Convert the physical addresses to statically-mapped virtual addresses
    //
    for (i = 0; i < dwBufNumPages; i++) {
        g_pPBE[i] = Phys2Virt((DWORD) g_pPBE[i]);
    }

    if (fRet == FALSE) {
        UnmapViewOfFile(g_pdwProfileBufStart);
        CloseHandle(g_hProfileBuf);
        goto EXIT_FALSE;
    }

    PROFILEMSG(1, (TEXT("ProfileStart() : Allocated %d kB for Profiler Buffer (0x%08X)\r\n"), dwMemRequest >> 10, g_pdwProfileBufStart));
    return TRUE;
    
EXIT_FALSE:
    PROFILEMSG(1, (TEXT("ProfileStart() : Error allocating buffer... defaulting to symbol lookup in ISR.\r\n")));
    return FALSE;
}



//------------------------------------------------------------------------------
//
// Takes a raw index and looks up the tiered entry pointer.
//
//------------------------------------------------------------------------------
PPROFBUFENTRY
GetEntryPointer(
    DWORD dwIndex
    )
{
    DWORD dwPage, dwEntry;

    dwPage  = dwIndex >> PROF_PAGE_SHIFT;
    dwEntry = dwIndex & PROF_ENTRY_MASK;

    return (&(g_pPBE[dwPage][dwEntry]));
}



/*
 *	ProfilerHit - OEMProfilerISR calls this routine to record profile hit
 *
 *	Input:	ra - interrupt return address
 *
 *	if buffer profiling, save ra and pCurProc->oe.tocptr in buffer
 *	else do symbol lookup in ISR
 */
void ProfilerHit(unsigned int ra) {
    PPROFBUFENTRY pEntry;

    if (bProfileBuffer) {           
        //
        // If profiling to buffer and these is still room in the buffer
        //
        if (dwProfileCount < dwBufNumEntries) {
            
            if (ra & 0x80000000)    // if high bit set, this is NK
                ra &= 0xdfffffff;     // mask off uncached bit
            
            pEntry = GetEntryPointer(dwProfileCount);
            dwProfileCount++;       // inc profile count
            //
            // Record an entry point
            //
            pEntry->ra = ra;
            pEntry->pte = pCurProc->oe.tocptr;
        } else {
            //
            // No place to record this hit. Let's remember how many we dropped.
            //
            dwBufRecDropped++;
        }
    } else {
        //
        // No buffer. Just lookup the symbol now.
        //
        dwProfileCount++;
        ProfilerSymbolHit(ra,pCurProc->oe.tocptr);
    }
}

/*
 *	ClearProfileHits - zero all profiler counters
 *
 */
void ClearProfileHits(void) {
	unsigned int iMod=0;
	PROFentry	*profptr;
	SYMentry	*symptr;
	unsigned int iSym;
	if (!pTOC->ulProfileLen)			// if no profile section
		return;							// just return
										// profptr-> first profile entry
	profptr= (PROFentry *)pTOC->ulProfileOffset;
	while (iMod++  < pTOC->nummods) {
		profptr->ulHits=0;				// clear module hit count
		if (profptr->ulNumSym) {		// if there are symbols for this module
			iSym=profptr->ulNumSym;		// clear symbol hits
			symptr=(SYMentry*)profptr->ulHitAddress;
			while (iSym) {
				symptr->ulFuncHits=0;
				iSym--;
				symptr++;
			}
		}
		profptr++;
	}
	dwBufRecDropped = 0;
}


//---------------------------------------------------------------------------
//	GetSymbol - searches symbol table for passed symbol number
//
//	Input:	lpSym - pointer to start of symbol buffer
//			dwSymNum - symbol number to lookup
//
//	Output:	returns pointer to symbol
//---------------------------------------------------------------------------
static LPBYTE GetSymbol(LPBYTE lpSym, DWORD dwSymNum)
{
	while (dwSymNum > 0) {
		while (*lpSym++);
		dwSymNum--;
	}
	return lpSym;
}

//---------------------------------------------------------------------------
//	ProfilerReport -display hit report
//
//
//---------------------------------------------------------------------------
void ProfilerReport(void)
{
	DWORD	loop, loop2, loop3;			// index
	PROFentry *pProf;					// profile section pointer
	DWORD	dwSymCount=0;				// number of symbols hit
	ULONG	ulPercent;					// hit percentage
	DWORD	dwNumModules;				// number of modules
	SYMentry *pSym;						// symbol address/hit pointer
	SYMentry *pHits=NULL;				// hit sorting structure pointer
	SYMentry symTemp;					// sorting temp
	LPBYTE	lpSymData=0;				// symbol data pointer
	ULONG 	ulTotalPercent;				// total percent of modules
	ULONG	ulTotalHits;
	TOCentry *tocptr;
	DWORD	dwHits[200],dwMods[200],dwNumHits=0,dwTemp;
	PPROFBUFENTRY pEntry;
	
	if (dwProfileCount)					// if profile size==0, profiling disabled
	{
		if (bProfileBuffer) {			// if profiling to buffer, lookup symbols now
			PROFILEMSG(1,(TEXT("ProfileReport: Looking up symbols for %u hits.\r\n"),dwProfileCount));
			for (loop=0; loop<dwProfileCount; loop++) {
				if (loop%10000 == 0)	// display a . every 10000 hits, so user knows we are working
					PROFILEMSG(1,(TEXT(".")));

                pEntry = GetEntryPointer(loop);
				ProfilerSymbolHit(pEntry->ra, pEntry->pte);
			}
			PROFILEMSG(1,(TEXT("\r\n")));
		}

		pProf= (PROFentry *)pTOC->ulProfileOffset;
		lpSymData= (LPBYTE)pProf->ulSymAddress;
		dwNumModules= pTOC->nummods;
										// display hits by module and count number of symbols hit
		PROFILEMSG(1,(TEXT("Total samples recorded = %lu\r\n"),dwProfileCount));
		if (dwBufRecDropped) {
            PROFILEMSG(1,(TEXT("Total samples dropped  = %lu\r\n"),dwBufRecDropped));
        }

		PROFILEMSG(1,(TEXT("Module        Hits        Percent\r\n")));
		PROFILEMSG(1,(TEXT("------------  ----------  -------\r\n")));
		ulTotalPercent=0;
		ulTotalHits=0;
		if (dwProfileCount) {   				// if any samples were collected
			// sort the samples
			pProf= (PROFentry *)pTOC->ulProfileOffset;
			for (loop=0; loop<dwNumModules; loop++) {
				if (pProf->ulHits){ 		// if there were any hits
					dwHits[dwNumHits]= pProf->ulHits;
					dwMods[dwNumHits]=loop;
					dwNumHits++;
				}
				pProf++;
			}
			for (loop=1; loop<dwNumHits; loop++) {
				for (loop2=dwNumHits-1; loop2 >= loop; loop2--) {
					if (dwHits[loop2-1] < dwHits[loop2]) {
						dwTemp= dwHits[loop2-1];
						dwHits[loop2-1]= dwHits[loop2];
						dwHits[loop2]= dwTemp;
						dwTemp= dwMods[loop2-1];
						dwMods[loop2-1]= dwMods[loop2];
						dwMods[loop2]= dwTemp;
					}
				}
			}

										// for each profile entry
			for (loop=0; loop<dwNumHits; loop++)	{
				tocptr = (TOCentry *)(pTOC+1);
				tocptr+= dwMods[loop];
				pProf= (PROFentry *)pTOC->ulProfileOffset;
				pProf+= dwMods[loop];
				if (pProf->ulHits) {	// if there were any hits
										// display module name, hits, percentage
					ulPercent= pProf->ulHits;
					ulPercent*=1000;
					ulPercent/=dwProfileCount;
					ulTotalPercent+= ulPercent;
					ulTotalHits+= pProf->ulHits;
					PROFILEMSG(1,(TEXT("%-12a  %10lu  %5lu.%1d\r\n"),
						tocptr->lpszFileName,pProf->ulHits, ulPercent / 10,ulPercent % 10));
										// inc total number of symbols
					pSym= (SYMentry *)pProf->ulHitAddress;
										// for each symbol hit						
					for (loop2=0; loop2<pProf->ulNumSym; loop2++) {
						if (pSym->ulFuncHits) {
							ulPercent= pSym->ulFuncHits;
							ulPercent*=100;
							ulPercent/=dwProfileCount;
							dwSymCount++;
						}
						pSym++;
					}
				}
			}
			ulTotalPercent= 1000-ulTotalPercent;
			ulTotalHits= dwProfileCount-ulTotalHits;
			if (ulTotalHits)
				PROFILEMSG(1,(TEXT("%-12a  %10lu  %5lu.%1d\r\n"),
				"UNKNOWN",ulTotalHits,ulTotalPercent/10,ulPercent%10));
			if (!dwSymCount) {
				PROFILEMSG(1,(TEXT("No symbols found.\r\n")));
				goto profile_exit;
			}

										// allocate memory for sorting
			pHits=(SYMentry *)VirtualAlloc(NULL,dwSymCount*sizeof(SYMentry),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
			if (pHits == NULL) {
				PROFILEMSG(1,(TEXT("ProfileStop: Sort memory allocation size %u failed.\r\n"),dwSymCount*sizeof(SYMentry)));
				goto profile_exit;
			}
										// build a table of offsets for each symbol
										// so it can be sorted
			dwSymCount=0;
			pProf= (PROFentry *)pTOC->ulProfileOffset;
			for (loop=0; loop<dwNumModules; loop++) {
				if (pProf->ulHits) {
					pSym= (SYMentry *)pProf->ulHitAddress;
										// for each symbol hit						
					for (loop2=0; loop2<pProf->ulNumSym; loop2++) {
						if (pSym->ulFuncHits) {
							ulPercent= pSym->ulFuncHits;
							ulPercent*=100;
							ulPercent/=dwProfileCount;
							pHits[dwSymCount].ulFuncAddress=pSym->ulFuncAddress;
							pHits[dwSymCount++].ulFuncHits=pSym->ulFuncHits;
						}
						pSym++;
					}
				}
				pProf++;
			}
	
										// sort the symbols (decreasing hits)
			for (loop=1; loop<dwSymCount; loop++) {
				for (loop2=dwSymCount-1; loop2 >= loop; loop2--) {
					if ((unsigned int)pHits[loop2-1].ulFuncHits < (unsigned int)pHits[loop2].ulFuncHits) {
						symTemp.ulFuncHits=pHits[loop2-1].ulFuncHits;
						symTemp.ulFuncAddress=pHits[loop2-1].ulFuncAddress;
						pHits[loop2-1].ulFuncHits=pHits[loop2].ulFuncHits;
						pHits[loop2-1].ulFuncAddress=pHits[loop2].ulFuncAddress;
						pHits[loop2].ulFuncHits=symTemp.ulFuncHits;
						pHits[loop2].ulFuncAddress=symTemp.ulFuncAddress;
					}
				}
			}

										// display hit list
			PROFILEMSG(1,(TEXT("Hits       Percent Address  Module       Routine\r\n")));
			PROFILEMSG(1,(TEXT("---------- ------- -------- ------------:---------------------\r\n")));
										// for each symbol
			for (loop=0; loop<dwSymCount && loop < NUM_SYMBOLS; loop++) {
										// find profile entry for this symbol
				pProf= (PROFentry *)pTOC->ulProfileOffset;
				tocptr = (TOCentry *)(pTOC+1);
				for (loop2=0; loop2<dwNumModules; loop2++) {
					pSym= (SYMentry *)pProf->ulHitAddress;
										// for each symbol hit						
					for (loop3=0; loop3<pProf->ulNumSym; loop3++) {
						if (pSym->ulFuncHits == pHits[loop].ulFuncHits &&
							pSym->ulFuncAddress == pHits[loop].ulFuncAddress) {
							ulPercent= pHits[loop].ulFuncHits;
							ulPercent*=100;
							ulPercent/=dwProfileCount;
							PROFILEMSG(1,(TEXT("%10d %7d %8.8lx %-12a:%a\r\n"),pHits[loop].ulFuncHits,
								ulPercent ,pHits[loop].ulFuncAddress, tocptr->lpszFileName,
								GetSymbol((LPBYTE)pProf->ulSymAddress,loop3)));
							goto next_sym;
						}
						pSym++;
					}
					pProf++;
					tocptr++;
				}
				next_sym:;
			}
		}

		profile_exit:
		if (pHits)
			VirtualFree(pHits,0,MEM_DECOMMIT|MEM_FREE);
	}
	return;
}

//---------------------------------------------------------------------------
//	GetEPC - Get exception program counter
//
//	Output:	returns EPC, zero if cpu not supported
//---------------------------------------------------------------------------
DWORD GetEPC(void) {
    return( GetThreadIP(pCurThread) );
}

// enable profiler syscall
//#define NKPROF
#include "..\kernel\profiler.c"
