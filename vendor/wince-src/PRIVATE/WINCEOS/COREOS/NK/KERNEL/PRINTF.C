/*
 *              NK Kernel printf code
 *
 *              Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.
 *
 * Module Name:
 *
 *              printf.c
 *
 * Abstract:
 *
 *              This file implements debug and string routines
 *
 */
 
/*
@doc    EXTERNAL KERNEL
@topic  Debug Support for Applications | 
	The kernel provides several kinds of supports for debugging
	applications. These are:
	
	<b Debug Messages>: The kernel provides API's for printing out
	of debug messages which can be turned on and off dynamically 
	using zones. Any module (DLL/Process) can register itself
	with the debug subsystem using <f DEBUGREGISTER>. This API
	registers the address of a Zonemask (which is a DWORD) with the
	kernel. Using the debug shell, a user can now dynamically turn
	bits of this zonemask on and off from the shell window.        
	The most typical way to use this is to filter debug messages
	based on these bits. A structured way to do this is to use
	the <f DEBUGZONE> macro to associate zones with bits, and then
	use the <f DEBUGMSG> function to associate each debug message
	with a zone. Type ? in the shell to see how to change zones 
	dynamically from the shell which uses the <f SetDbgZone> function
	to implement the change. See the example below for general zone
	usage.
	
	<b Asserts>: The kernel also provides for simple asserts. See
	<f DEBUGCHK> for details.
	
@ex     An example of using debug zones |
	// Declare a DBGPARAM structure & give names to all used zones
	DBGPARAM dpCurSettings = { L"foodll", {
	    L"Info",       L"Validate",  L"bar",       L"random",
		L"Undefined",  L"Undefined", L"Undefined", L"Undefined",
		L"Undefined",  L"Undefined", L"Undefined", L"Undefined",
		L"Undefined",  L"Undefined", L"Undefined", L"Undefined" },
		0x00000000 };
		
	// Associate bits with a zone 
	// these should match up with the text strings above!
	#define ZONE_INFO       DEBUGZONE(0)
	#define ZONE_VALIDATE   DEBUGZONE(1)

	// Register : Assume this is a DLL 
	// A Process would do the same in their libmain
	BOOL DllEntry (HANDLE hinstDLL, DWORD fdwReason, LPVOID lpv) {
	    if ( fdwReason == DLL_PROCESS_ATTACH ) {
		DEBUGREGISTER(hinstDLL);
	    }
	    ...
	}

	// Use the defined zone in debug messages
	DEBUGMSG (ZONE_INFO, (L"This is an illustrative messages only!"));

	// Or use a zone to turn execution of some code on & off
	if (ZONE_VALIDATE) {
	    // validate some stuff ...
	}            
			
@xref   
    <f DEBUGMSG> <tab> <f RETAILMSG> <tab> <f ERRORMSG> <tab> <f DEBUGCHK> <tab>
    <f DEBUGREGISTER> <tab> <f DEBUGZONE> <tab> <t DBGPARAM> <tab>
    <f RegisterDebugZones> <tab> <f SetDbgZone>
*/ 

#include "kernel.h"

#define out(c) if (--cchLimit) *lpOut++=(c); else goto errorout

//-------------------------- Prototype declarations ---------------------------

int NKwvsprintfW(LPWSTR lpOut, LPCWSTR lpFmt, CONST VOID * lpParms, int maxchars);
LPCWSTR SP_GetFmtValue(LPCWSTR lpch, int  *lpw, va_list *plpParms);
int SP_PutNumber(LPWSTR, ULONG, int, int, int);
int SP_PutNumber64(LPWSTR lpb, __int64 i64, int limit, int radix, int mycase);
void SP_Reverse(LPWSTR lp1, LPWSTR lp2);

extern CRITICAL_SECTION ODScs, DbgApiCS, ppfscs;

VOID WINAPI OutputDebugStringW(LPCWSTR str) {
	BOOL fGetODScs;
	int len;
	if (!str)
		str = __TEXT("(NULL)");
	if (!InSysCall() && pCurThread->pThrdDbg && (DbgApiCS.OwnerThread != hCurThread)) {
		if (ProcStarted(pCurProc) && pCurThread->pThrdDbg->hEvent) {
			pCurThread->pThrdDbg->dbginfo.dwDebugEventCode = OUTPUT_DEBUG_STRING_EVENT;
			pCurThread->pThrdDbg->dbginfo.dwProcessId = (DWORD)hCurProc;
			pCurThread->pThrdDbg->dbginfo.dwThreadId = (DWORD)hCurThread;
			pCurThread->pThrdDbg->dbginfo.u.DebugString.lpDebugStringData = (LPBYTE)str;
			pCurThread->pThrdDbg->dbginfo.u.DebugString.fUnicode = TRUE;
			pCurThread->pThrdDbg->dbginfo.u.DebugString.nDebugStringLength = (strlenW(str)+1)*2;
			SetEvent(pCurThread->pThrdDbg->hEvent);
			SC_WaitForMultiple(1,&pCurThread->pThrdDbg->hBlockEvent,FALSE,INFINITE);
		}
	}
	// Don't acquire ODScs if we already own the PPFS CS (since this violates our CS ordering restrictions)
	fGetODScs = !ReadyForStrings && !InSysCall() && (hCurThread != ppfscs.OwnerThread);
	len = (strlenW(str)+1)*sizeof(WCHAR); // forces the string to be paged in if not already
	if (!InSysCall())
		LockPages((LPVOID)str,len,0,0);
	if (fGetODScs)
		EnterCriticalSection(&ODScs);
	lpWriteDebugStringFunc((unsigned short *)str);
	if (fGetODScs)
		LeaveCriticalSection(&ODScs);
	if (!InSysCall())
		UnlockPages((LPVOID)str,len);
}

int InputDebugCharW(void) {
    int retvalue;
	if (!InSysCall())
		EnterCriticalSection(&ODScs);
	retvalue = OEMReadDebugByte();
	if (!InSysCall())
		LeaveCriticalSection(&ODScs);
    return retvalue;
}

/*
@doc    EXTERNAL KERNEL
@func   VOID | NKDbgPrintf | Prints debug messages
@parm   LPWSTR | lpszFmt | Printf style formatting string
@parmvar Variable argument list
@comm   Should not be used directly - macros like <f DEBUGMSG> should
	be used to print messages. This function will format the
	debug string into a buffer and then log it according to the
	current logging paramters. If terminal logging is on it
	outputs this to the debug terminal, and if file logging
	is on it stores it to the file peg.log on the host PC.
	
	<b WARNING>: The message being output must be smaller than
	256 bytes - ie 128 unicode characters.
@xref   <f DEBUGMSG>
*/
void WINAPIV NKDbgPrintfW(LPCWSTR lpszFmt, ...)  {
    va_list arglist;
    va_start(arglist, lpszFmt);
    NKvDbgPrintfW(lpszFmt, arglist);
    va_end(arglist);
}

VOID NKvDbgPrintfW(LPCWSTR lpszFmt, va_list lpParms) {
	WCHAR rgchBuf[384];
    WORD wLen = 0;
	// Get it into a string
#ifdef DEBUG
    if (ZONE_DEBUG) {
    	rgchBuf[0] = '0';
    	rgchBuf[1] = 'x';
    	wLen += 2;
    	wLen += SP_PutNumber(rgchBuf+2,(ULONG)pCurThread,8,16,0);
        SP_Reverse(rgchBuf+2,rgchBuf+wLen-1);
    	rgchBuf[wLen++] = ':';
    	rgchBuf[wLen++] = ' ';
    }
#endif
	wLen += NKwvsprintfW(rgchBuf + wLen, lpszFmt, lpParms, sizeof(rgchBuf)/sizeof(WCHAR) - wLen);
	OutputDebugStringW(rgchBuf);
}

int NKwvsprintfW(LPWSTR lpOut, LPCWSTR lpFmt, va_list lpParms, int maxchars) {
    int left, width, prec, size, sign, radix, upper, cch, cchLimit;
    WCHAR prefix, fillch;
    LPWSTR lpT;
	LPCHAR lpC;
    union {
		long l;
		unsigned long ul;
		__int64 i64;
		WCHAR sz[sizeof(long)];
	} val;

	cchLimit = maxchars;
    while (*lpFmt) {
		if (*lpFmt==(WCHAR)'%') {
		    /* read the flags.  These can be in any order */
		    left=0;
		    prefix=0;
		    while (*++lpFmt) {
				if (*lpFmt==(WCHAR)'-')
				    left++;
				else if (*lpFmt==(WCHAR)'#')
				    prefix++;
				else
				    break;
			}
		    /* find fill character */
		    if (*lpFmt==(WCHAR)'0') {
				fillch=(WCHAR)'0';
				lpFmt++;
			} else
				fillch=(WCHAR)' ';
		    /* read the width specification */
		    lpFmt=SP_GetFmtValue(lpFmt,&cch, &lpParms);
		    width=cch;
		    /* read the precision */
		    if (*lpFmt==(WCHAR)'.') {
				lpFmt=SP_GetFmtValue(++lpFmt,&cch, &lpParms);
				prec=cch;
			} else
				prec=-1;
		    /* get the operand size */
			size=1;
		    if (*lpFmt=='l') {
				lpFmt++;
			} else if (*lpFmt=='h') {
				size=0;
				lpFmt++;
			} else if ((*lpFmt == 'I') && (*(lpFmt+1) == '6') && (*(lpFmt+2) == '4')) {
	    		lpFmt+=3;
	    		size = 2;
	    	}
		    upper=0;
		    sign=0;
		    radix=10;
		    switch (*lpFmt) {
			    case 0:
					goto errorout;
			    case (WCHAR)'i':
			    case (WCHAR)'d':
					sign++;
			    case (WCHAR)'u':
					/* turn off prefix if decimal */
					prefix=0;
donumeric:
					/* special cases to act like MSC v5.10 */
					if (left || prec>=0)
					    fillch=(WCHAR)' ';
					if (size == 1)
					    val.l=va_arg(lpParms, long);
					else if (size == 2)
						val.i64 = va_arg(lpParms, __int64);
					else if (sign)
						val.l=va_arg(lpParms, short);
				    else
						val.ul=va_arg(lpParms, unsigned);
					if (sign && val.l<0L)
					    val.l=-val.l;
					else
					    sign=0;
					lpT=lpOut;
					/* blast the number backwards into the user buffer */
					if (size == 2)
						cch=SP_PutNumber64(lpOut,val.i64,cchLimit,radix,upper);
					else
						cch=SP_PutNumber(lpOut,val.l,cchLimit,radix,upper);
					if (!(cchLimit-=cch))
					    goto errorout;
					lpOut+=cch;
					width-=cch;
					prec-=cch;
					if (prec>0)
					width-=prec;
					/* fill to the field precision */
					while (prec-->0)
					    out((WCHAR)'0');
					if (width>0 && !left) {
					    /* if we're filling with spaces, put sign first */
					    if (fillch!=(WCHAR)'0') {
							if (sign) {
							    sign=0;
							    out((WCHAR)'-');
							width--;
						    }
							if (prefix) {
							    out(prefix);
							    out((WCHAR)'0');
							    prefix=0;
						    }
						}
					    if (sign)
							width--;
					    /* fill to the field width */
					    while (width-->0)
							out(fillch);
					    /* still have a sign? */
					    if (sign)
							out((WCHAR)'-');
					    if (prefix) {
							out(prefix);
							out((WCHAR)'0');
						}
					    /* now reverse the string in place */
					    SP_Reverse(lpT,lpOut-1);
				} else {
					    /* add the sign character */
					    if (sign) {
							out((WCHAR)'-');
							width--;
						}
					    if (prefix) {
							out(prefix);
							out((WCHAR)'0');
						}
					    /* reverse the string in place */
					SP_Reverse(lpT,lpOut-1);
					    /* pad to the right of the string in case left aligned */
					while (width-->0)
							out(fillch);
				}
					break;
				case (WCHAR) 'p' :
					// Fall into case below, since NT is going to 64 bit
					// they are starting to use p to indicate a pointer
					// value.  They only seem to support lower case 'p'
			    case (WCHAR)'X':
					upper++;
			    case (WCHAR)'x':
					radix=16;
					if (prefix)
					if (upper)
							prefix=(WCHAR)'X';
					    else
							prefix=(WCHAR)'x';
					goto donumeric;
			    case (WCHAR)'c':
					val.sz[0] = va_arg(lpParms, WCHAR);
					val.sz[1]=0;
					lpT=val.sz;
					cch = 1;  // Length is one character. 
							 /* stack aligned to larger size */
					goto putstring;
			    case 'a':   // ascii string!
PrtAscii:
					if (!(lpC=va_arg(lpParms, LPCHAR)))
						lpC = "(NULL)";
					cch=strlen(lpC);
					if (prec>=0 && cch>prec)
					    cch=prec;
					width -= cch;
					if (left) {
					    while (cch--)
							out((WCHAR)*lpC++);
					    while (width-->0)
							out(fillch);
				    } else {
					    while (width-->0)
							out(fillch);
					    while (cch--)
							out((WCHAR)*lpC++);
				    }
					break;
			    case 's':
					if (!size)
						goto PrtAscii;
					if (!(lpT=va_arg(lpParms,LPWSTR)))
						lpT = L"(NULL)";
					cch=strlenW(lpT);
putstring:
					if (prec>=0 && cch>prec)
					    cch=prec;
					width -= cch;
					if (left) {
					    while (cch--)
							out(*lpT++);
					    while (width-->0)
							out(fillch);
				    } else {
					    while (width-->0)
							out(fillch);
					    while (cch--)
							out(*lpT++);
				    }
					break;
			    default:
					out(*lpFmt);    /* Output the invalid char and continue */
					break;
			}
	    } else /* character not a '%', just do it */
		    out(*lpFmt);
		/* advance to next format string character */
		lpFmt++;
	}
errorout:
    *lpOut=0;
    return maxchars-cchLimit;
}

/*
 *  GetFmtValue
 *
 *  reads a width or precision value from the format string
 */

LPCWSTR SP_GetFmtValue(LPCWSTR lpch, int *lpw, va_list *plpParms) {
    int i=0;
	if (*lpch == TEXT('*')) {
		*lpw = va_arg(*plpParms, int);
		lpch++;
	} else {
		while (*lpch>=(WCHAR)'0' && *lpch<=(WCHAR)'9') {
			i = i*10 + (*lpch-(WCHAR)'0');
			lpch++;
		}
    	*lpw=i;
    }
    /* return the address of the first non-digit character */
    return lpch;
}

void SP_Reverse(LPWSTR lpFirst, LPWSTR lpLast) {
	int swaps;
	WCHAR tmp;
	swaps = ((((DWORD)lpLast - (DWORD)lpFirst)/sizeof(WCHAR)) + 1)/2;
	while (swaps--) {
		tmp = *lpFirst;
		*lpFirst++ = *lpLast;
		*lpLast-- = tmp;
	}
}

const WCHAR lowerval[] = TEXT("0123456789abcdef");
const WCHAR upperval[] = TEXT("0123456789ABCDEF");

int SP_PutNumber(LPWSTR lpb, ULONG n, int limit, int radix, int mycase) {
	int used = 0;
	while (limit--) {
		*lpb++ = (mycase ? upperval[(n % radix)] : lowerval[(n % radix)]);
		used++;
		if (!(n /= radix))
			break;
	}
	return used;
}

int SP_PutNumber64(LPWSTR lpb, __int64 i64, int limit, int radix, int mycase) {
	int used = 0;
	while (limit--) {
		*lpb++ = (mycase ? upperval[(i64 % radix)] : lowerval[(i64 % radix)]);
		used++;
		if (!(i64 /= radix))
			break;
	}
	return used;
}

