/* Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved. */
// included from dbg.c, apis.c, and stubs.c

#ifdef IN_KERNEL
#undef UCurThread
#define UCurThread() hCurThread
#endif

extern void CheckTakeCritSec(LPCRITICAL_SECTION lpcs);

/*
	@doc BOTH EXTERNAL
	
	@func VOID | InitializeCriticalSection | Initializes a critical section object. 
    @parm LPCRITICAL_SECTION | lpcsCriticalSection | address of critical section object  

	@comm Follows the Win32 reference description without restrictions or modifications. 
*/
VOID WINAPI InitializeCriticalSection(LPCRITICAL_SECTION lpcs) {
	/* Init the structure */
	lpcs->LockCount = 0;
	lpcs->OwnerThread = 0;
	lpcs->needtrap = 0;
	lpcs->hCrit = CreateCrit(lpcs);
	DEBUGCHK(lpcs->hCrit);
}

/*
	@doc BOTH EXTERNAL
	
	@func VOID | DeleteCriticalSection | Releases all resources used by an unowned 
	critical section object. 
    @parm LPCRITICAL_SECTION | lpCriticalSection | address of critical section object  

	@comm Follows the Win32 reference description without restrictions or modifications. 

*/

VOID WINAPI DeleteCriticalSection(LPCRITICAL_SECTION lpcs) {
	/* Just clear out the fields */
	DEBUGCHK(!lpcs->OwnerThread);
	CreateCrit((LPCRITICAL_SECTION)((DWORD)lpcs|1));
	memset(lpcs,0,sizeof(CRITICAL_SECTION));
}

/* If fastpath, adjust critical section, else trap to kernel for rescheduling */

/*
	@doc BOTH EXTERNAL
	
	@func VOID | EnterCriticalSection | Waits for ownership of the specified critical 
	section object. The function returns when the calling thread is granted ownership. 	
	@parm LPCRITICAL_SECTION | lpCriticalSection | address of critical section object  

	@comm Follows the Win32 reference description without restrictions or modifications. 

*/


#ifdef ARM
#pragma optimize("",off)
#endif

VOID WINAPI EnterCriticalSection(LPCRITICAL_SECTION lpcs) {
	HANDLE hTh = UCurThread();
    volatile CRITICAL_SECTION *vpcs = lpcs;
#ifdef DOCHECKCS
	CheckTakeCritSec(lpcs);
#endif
	DEBUGCHK(!((DWORD)hTh & 1));
	if (vpcs->OwnerThread == hTh)
		vpcs->LockCount++; /* We are the owner - increment count */
	else if (InterlockedTestExchange((LPLONG)&lpcs->OwnerThread,0,(LONG)hTh) == 0)
		vpcs->LockCount = 1;
	else
		TakeCritSec(lpcs);
}

#ifdef COREDLL
BOOL WINAPI TryEnterCriticalSection(LPCRITICAL_SECTION lpcs) {
	BOOL bRet = TRUE;
	HANDLE hTh = UCurThread();
    volatile CRITICAL_SECTION *vpcs = lpcs;
	DEBUGCHK(!((DWORD)hTh & 1));
	if (vpcs->OwnerThread == hTh)
		vpcs->LockCount++; /* We are the owner - increment count */
	else if (InterlockedTestExchange((LPLONG)&lpcs->OwnerThread,0,(LONG)hTh) == 0)
		vpcs->LockCount = 1;
	else
		bRet = FALSE;
	return bRet;
}
#endif

/* If fastpath, adjust critical section, else trap to kernel for rescheduling */

/*
	@doc BOTH EXTERNAL
	
	@func VOID | LeaveCriticalSection | Releases ownership of the specified critical 
	section object. 
	@parm LPCRITICAL_SECTION | lpcsCriticalSection | address of critical section object  

	@comm Follows the Win32 reference description without restrictions or modifications. 
*/
VOID WINAPI LeaveCriticalSection(LPCRITICAL_SECTION lpcs) {
HANDLE hTh = UCurThread();
    volatile CRITICAL_SECTION *vpcs = lpcs;
	/* Check for leave without enter */
	DEBUGCHK(vpcs->OwnerThread == hTh);
	if (vpcs->OwnerThread != hTh)
		return;
	if (vpcs->LockCount != 1)
		vpcs->LockCount--; /* Simply decrement count if more than 1 */
	else {
		vpcs->OwnerThread = (HANDLE)((DWORD)hTh | 1);
		if (vpcs->needtrap)
			LeaveCritSec(lpcs);
		else
			InterlockedTestExchange((PLONG)&vpcs->OwnerThread, (DWORD)hTh | 1, 0L);
	}
}


#ifdef ARM
#pragma optimize("",on)
#endif

