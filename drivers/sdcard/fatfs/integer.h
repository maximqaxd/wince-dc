/*-------------------------------------------*/
/* Integer type definitions for FatFs module */
/*-------------------------------------------*/

#ifndef _INTEGER
#define _INTEGER

#if defined(_WIN32) || defined(UNDER_CE)	/* FatFs on Windows / Windows CE: use the OS types */

#include <windows.h>
#include <tchar.h>

/* cl.exe (SH) has no GCC __attribute__; FatFs structs are in-memory (on-disk data is read
   via byte-offset macros, _WORD_ACCESS=0), so neutralising the packed attribute is safe. */
#ifndef __attribute__
#define __attribute__(x)
#endif

#else			/* Embedded platform */

/* These types must be 16-bit, 32-bit or larger integer */
typedef int				INT;
typedef unsigned int	UINT;

/* These types must be 8-bit integer */
typedef char			CHAR;
typedef unsigned char	UCHAR;
typedef unsigned char	BYTE;

/* These types must be 16-bit integer */
typedef short			SHORT;
typedef unsigned short	USHORT;
typedef unsigned short	WORD;
typedef unsigned short	WCHAR;

/* These types must be 32-bit integer */
typedef long			LONG;
typedef unsigned long	ULONG;
typedef unsigned long	DWORD;

/* Boolean type */
typedef enum { FALSE = 0, TRUE } BOOL;

#endif

#endif
