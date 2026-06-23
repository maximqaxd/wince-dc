/**********************************************************************/
/**                        Microsoft Windows                         **/
/**Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.**/
/**********************************************************************/

/*
    ccutil.h

   Declarations for some useful C library functions.

*/
#ifndef _CCLIB_H_
#define _CCLIB_H_

#ifdef UNDER_CE
#include "ndis.h"
#endif // UNDER_CE

//	C Library Functions
//	===================

#ifdef UNDER_CE

#ifdef DONT_COMPILE
// <string.h> functions
#ifdef DEBUG
// These functions are compiler intrinsic
char *strcpy(char *s, char *ct);
unsigned int strlen(char *s);
int strcmp (const char *s1, const char *s2);
#endif // DEBUG
char *strupr( char *s );
char *strchr(char *s, int c);
#endif // DONT_COMPILE

_CRTIMP char * __cdecl strncpy(char *, const char *, size_t);
_CRTIMP int __cdecl strncmp(const char *, const char *, size_t);

#ifdef UNICODE
#define _tcstol wcstol
#else
#define _tcstol strtol
#endif

PNDIS_BUFFER CopyFlatToNdis(PNDIS_BUFFER DestBuf, uchar *SrcBuf, uint Size,
							uint *StartOffset);
#endif // UNDER_CE


// Help with ARGC/ARGV 
int	CreateArgvArgc(TCHAR *pProgName, TCHAR *argv[20], TCHAR *pCmdLine);

// Debugging helpers
#ifdef DEBUG
void	DumpMem (PBYTE Ptr, ULONG Len);
#endif


#endif // _CCLIB_H_
