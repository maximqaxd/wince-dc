/* Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved. */
#include <corecrt.h>

#if !defined(PPC) && !defined(x86) // in the crt on PPC and x86

/****************************************************************************
memchr

The memchr function looks for the first occurance of cCharacter in the first
iLength bytes of pBuffer and stops after finding cCharacter or after checking
iLength bytes.

Arguments:
    pBuffer: pointer to buffer
    cCharacter: character to look for
    iLength: number of characters

Returns:

If successful, memchr returns a pointer to the first location of cCharacter
in pBuffer.  Otherwise, it returns NULL.

****************************************************************************/
void * memchr(const void *pBuffer, int cCharacter, size_t iLength) {
    while ((iLength != 0) && (*((char *)pBuffer) != (char)cCharacter)) {
        ((char *) pBuffer) += 1;
        iLength -= 1;
    }
    return((iLength != 0) ? (void *)pBuffer : (void *)0);
}

#endif

int _tolower(int);
int _toupper(int);

/***
*wchar_t *wcscat(dst, src) - concatenate (append) one wchar_t string to another
*
*Purpose:
*	Concatenates src onto the end of dest.	Assumes enough
*	space in dest.
*
*Entry:
*	wchar_t *dst - wchar_t string to which "src" is to be appended
*	const wchar_t *src - wchar_t string to be appended to the end of "dst"
*
*Exit:
*	The address of "dst"
*
*Exceptions:
*
*******************************************************************************/

wchar_t * wcscat(wchar_t *dst, const wchar_t *src) {
	wchar_t * cp = dst;
	while( *cp )
		cp++;			/* find end of dst */
	while( *cp++ = *src++ )
		;	/* Copy src to end of dst */
	return( dst );			/* return dst */
}

/***
*wchar_t *wcscpy(dst, src) - copy one wchar_t string over another
*
*Purpose:
*	Copies the wchar_t string src into the spot specified by
*	dest; assumes enough room.
*
*Entry:
*	wchar_t * dst - wchar_t string over which "src" is to be copied
*	const wchar_t * src - wchar_t string to be copied over "dst"
*
*Exit:
*	The address of "dst"
*
*Exceptions:
*******************************************************************************/

wchar_t * wcscpy(wchar_t *dst, const wchar_t *src) {
	wchar_t * cp = dst;
	while( *cp++ = *src++ )
		;		/* Copy src over dst */
	return( dst );
}

/***
*wchar_t *wcschr(string, c) - search a string for a wchar_t character
*
*Purpose:
*	Searches a wchar_t string for a given wchar_t character,
*	which may be the null character L'\0'.
*
*Entry:
*	wchar_t *string - wchar_t string to search in
*	wchar_t c - wchar_t character to search for
*
*Exit:
*	returns pointer to the first occurence of c in string
*	returns NULL if c does not occur in string
*
*Exceptions:
*
*******************************************************************************/

wchar_t * wcschr (const wchar_t * string, wchar_t ch) {
	while (*string && *string != (wchar_t)ch)
		string++;
	if (*string == (wchar_t)ch)
		return((wchar_t *)string);
	return(NULL);
}

/***
*wcscmp - compare two wchar_t strings,
*	 returning less than, equal to, or greater than
*
*Purpose:
*	wcscmp compares two wide-character strings and returns an integer
*	to indicate whether the first is less than the second, the two are
*	equal, or whether the first is greater than the second.
*
*	Comparison is done wchar_t by wchar_t on an UNSIGNED basis, which is to
*	say that Null wchar_t(0) is less than any other character.
*
*Entry:
*	const wchar_t * src - string for left-hand side of comparison
*	const wchar_t * dst - string for right-hand side of comparison
*
*Exit:
*	returns -1 if src <  dst
*	returns  0 if src == dst
*	returns +1 if src >  dst
*
*Exceptions:
*
*******************************************************************************/

int wcscmp (const wchar_t *src, const wchar_t *dst) {
	int ret;
	while((*src == *dst) && *dst)
		++src, ++dst;
	ret = *src - *dst;
	return (ret > 0 ? 1 : ret < 0 ? -1 : 0);
}

/***
*size_t wcscspn(string, control) - search for init substring w/o control wchars
*
*Purpose:
*	returns the index of the first character in string that belongs
*	to the set of characters specified by control.	This is equivalent
*	to the length of the length of the initial substring of string
*	composed entirely of characters not in control.  Null chars not
*	considered (wide-character strings).
*
*Entry:
*	wchar_t *string - string to search
*	wchar_t *control - set of characters not allowed in init substring
*
*Exit:
*	returns the index of the first wchar_t in string
*	that is in the set of characters specified by control.
*
*Exceptions:
*
*******************************************************************************/

size_t wcscspn (const wchar_t * string, const wchar_t * control) {
    wchar_t *str = (wchar_t *) string;
    wchar_t *wcset;
    /* 1st char in control string stops search */
    while (*str) {
        for (wcset = (wchar_t *)control; *wcset; wcset++)
            if (*wcset == *str)
                return str - string;
        str++;
    }
    return str - string;
}


/***
*wchar_t *_wcsdup(string) - duplicate string into malloc'd memory
*
*Purpose:
*	Allocates enough storage via malloc() for a copy of the
*	string, copies the string into the new memory, and returns
*	a pointer to it (wide-character).
*
*Entry:
*	wchar_t *string - string to copy into new memory
*
*Exit:
*	returns a pointer to the newly allocated storage with the
*	string in it.
*
*	returns NULL if enough memory could not be allocated, or
*	string was NULL.
*
*Uses:
*
*Exceptions:
*
*******************************************************************************/

wchar_t * _wcsdup (const wchar_t * string) {
	wchar_t *memory;
	if (!string)
		return(NULL);
	if (memory = (wchar_t *) LocalAlloc(0, (wcslen(string)+1) * sizeof(wchar_t)))
		return(wcscpy(memory,string));
	return(NULL);
}

char * _strdup (const char * string) {
	char *memory;
	if (!string)
		return(NULL);
	if (memory = (char *) LocalAlloc(0, strlen(string)+1))
		return(strcpy(memory,string));
	return(NULL);
}

int _stricmp(const char *str1, const char *str2) {
	int ch1, ch2;
    for (;*str1 && *str2; str1++, str2++) {
        ch1 = _tolower(*str1); 
        ch2 = _tolower(*str2);
        if (ch1 != ch2)
            return ch1 - ch2;
    }
    // Check last character.
    return _tolower(*str1) - _tolower(*str2);
}

int _strnicmp(const char *psz1, const char *psz2, size_t cb) {
    int ch1, ch2 = 0; // Zero for case cb = 0.
    while (cb--) {
        ch1 = _tolower(*(psz1++)); 
        ch2 = _tolower(*(psz2++));
        if (!ch1 || (ch1 != ch2))
        	break;
    }
    return (ch1 - ch2);
}

/***
*wcslen - return the length of a null-terminated wide-character string
*
*Purpose:
*	Finds the length in wchar_t's of the given string, not including
*	the final null wchar_t (wide-characters).
*
*Entry:
*	const wchar_t * wcs - string whose length is to be computed
*
*Exit:
*	length of the string "wcs", exclusive of the final null wchar_t
*
*Exceptions:
*
*******************************************************************************/

size_t wcslen (const wchar_t * wcs) {
	const wchar_t *eos = wcs;
	while( *eos++ )
		;
	return( (size_t)(eos - wcs - 1) );
}

/***
*wchar_t *wcsncat(front, back, count) - append count chars of back onto front
*
*Purpose:
*	Appends at most count characters of the string back onto the
*	end of front, and ALWAYS terminates with a null character.
*	If count is greater than the length of back, the length of back
*	is used instead.  (Unlike wcsncpy, this routine does not pad out
*	to count characters).
*
*Entry:
*	wchar_t *front - string to append onto
*	wchar_t *back - string to append
*	size_t count - count of max characters to append
*
*Exit:
*	returns a pointer to string appended onto (front).
*
*Uses:
*
*Exceptions:
*
*******************************************************************************/

wchar_t * wcsncat(wchar_t * front, const wchar_t * back, size_t count) {
	wchar_t *start = front;
	while (*front++)
		;
	front--;
	while (count--)
		if (!(*front++ = *back++))
			return(start);
	*front = L'\0';
	return(start);
}

/***
*int wcsncmp(first, last, count) - compare first count chars of wchar_t strings
*
*Purpose:
*	Compares two strings for lexical order.  The comparison stops
*	after: (1) a difference between the strings is found, (2) the end
*	of the strings is reached, or (3) count characters have been
*	compared (wide-character strings).
*
*Entry:
*	wchar_t *first, *last - strings to compare
*	size_t count - maximum number of characters to compare
*
*Exit:
*	returns <0 if first < last
*	returns  0 if first == last
*	returns >0 if first > last
*
*Exceptions:
*
*******************************************************************************/

int wcsncmp(const wchar_t * first, const wchar_t * last, size_t count) {
	if (!count)
		return(0);
	while (--count && *first && *first == *last) {
		first++;
		last++;
	}
	return((int)(*first - *last));
}

/***
*wchar_t *wcsncpy(dest, source, count) - copy at most n wide characters
*
*Purpose:
*	Copies count characters from the source string to the
*	destination.  If count is less than the length of source,
*	NO NULL CHARACTER is put onto the end of the copied string.
*	If count is greater than the length of sources, dest is padded
*	with null characters to length count (wide-characters).
*
*
*Entry:
*	wchar_t *dest - pointer to destination
*	wchar_t *source - source string for copy
*	size_t count - max number of characters to copy
*
*Exit:
*	returns dest
*
*Exceptions:
*
*******************************************************************************/

wchar_t * wcsncpy(wchar_t * dest, const wchar_t * source, size_t count) {
	wchar_t *start = dest;
	while (count && (*dest++ = *source++))	  /* copy string */
		count--;
	if (count)				/* pad out with zeroes */
		while (--count)
			*dest++ = L'\0';
	return(start);
}

/***
*wchar_t *_wcsnset(string, val, count) - set at most count characters to val
*
*Purpose:
*	Sets the first count characters of string the character value.
*	If the length of string is less than count, the length of
*	string is used in place of n (wide-characters).
*
*Entry:
*	wchar_t *string - string to set characters in
*	wchar_t val - character to fill with
*	size_t count - count of characters to fill
*
*Exit:
*	returns string, now filled with count copies of val.
*
*Exceptions:
*
*******************************************************************************/

wchar_t * _wcsnset (wchar_t * string, wchar_t val, size_t count) {
	wchar_t *start = string;
	while (count-- && *string)
		*string++ = val;
	return(start);
}

char * _strnset (char * string, int val, size_t count) {
	char *start = string;
	while (count-- && *string)
		*string++ = val;
	return(start);
}

/***
*wchar_t *wcspbrk(string, control) - scans string for a character from control
*
*Purpose:
*	Returns pointer to the first wide-character in
*	a wide-character string in the control string.
*
*Entry:
*	wchar_t *string - string to search in
*	wchar_t *control - string containing characters to search for
*
*Exit:
*	returns a pointer to the first character from control found
*	in string.
*	returns NULL if string and control have no characters in common.
*
*Exceptions:
*
*******************************************************************************/

wchar_t * wcspbrk(const wchar_t * string, const wchar_t * control) {
    wchar_t *wcset;
    /* 1st char in control string stops search */
    while (*string) {
        for (wcset = (wchar_t *) control; *wcset; wcset++)
            if (*wcset == *string)
                return (wchar_t *) string;
        string++;
    }
    return NULL;
}

char * strpbrk(const char * string, const char * control) {
    char *cset;
    /* 1st char in control string stops search */
    while (*string) {
        for (cset = (char *) control; *cset; cset++)
            if (*cset == *string)
                return (char *) string;
        string++;
    }
    return NULL;
}

/***
*wchar_t *wcsrchr(string, ch) - find last occurrence of ch in wide string
*
*Purpose:
*	Finds the last occurrence of ch in string.  The terminating
*	null character is used as part of the search (wide-characters).
*
*Entry:
*	wchar_t *string - string to search in
*	wchar_t ch - character to search for
*
*Exit:
*	returns a pointer to the last occurrence of ch in the given
*	string
*	returns NULL if ch does not occurr in the string
*
*Exceptions:
*
*******************************************************************************/

wchar_t * wcsrchr(const wchar_t * string, wchar_t ch) {
	wchar_t *start = (wchar_t *)string;
	while (*string++)			/* find end of string */
		;						/* search towards front */
	while (--string != start && *string != ch)
		;
	if (*string == ch)		/* wchar_t found ? */
		return( (wchar_t *)string );
	return(NULL);
}

char * strrchr(const char * string, int ch) {
	char *start = (char *)string;
	while (*string++)			/* find end of string */
		;						/* search towards front */
	while (--string != start && *string != ch)
		;
	if (*string == ch)		/* wchar_t found ? */
		return( (char *)string );
	return(NULL);
}

/***
*wchar_t *_wcsrev(string) - reverse a wide-character string in place
*
*Purpose:
*	Reverses the order of characters in the string.  The terminating
*	null character remains in place (wide-characters).
*
*Entry:
*	wchar_t *string - string to reverse
*
*Exit:
*	returns string - now with reversed characters
*
*Exceptions:
*
*******************************************************************************/

wchar_t * _wcsrev (wchar_t * string) {
	wchar_t *start = string;
	wchar_t *left = string;
	wchar_t ch;
	while (*string++)		  /* find end of string */
		;
	string -= 2;
	while (left < string) {
		ch = *left;
		*left++ = *string;
		*string-- = ch;
	}
	return(start);
}

char * _strrev (char * string) {
	char *start = string;
	char *left = string;
	char ch;
	while (*string++)		  /* find end of string */
		;
	string -= 2;
	while (left < string) {
		ch = *left;
		*left++ = *string;
		*string-- = ch;
	}
	return(start);
}

/***
*wchar_t *_wcsset(string, val) - sets all of string to val (wide-characters)
*
*Purpose:
*	Sets all of wchar_t characters in string (except the terminating '/0'
*	character) equal to val (wide-characters).
*
*
*Entry:
*	wchar_t *string - string to modify
*	wchar_t val - value to fill string with
*
*Exit:
*	returns string -- now filled with val's
*
*Uses:
*
*Exceptions:
*
*******************************************************************************/

wchar_t * _wcsset (wchar_t * string, wchar_t val) {
	wchar_t *start = string;
	while (*string)
		*string++ = val;
	return(start);
}

#ifndef SHx
#pragma function(_strset)
#endif

char *_strset(char * string, int val) {
	char *start = string;
	while (*string)
		*string++ = val;
	return(start);
}

/***
*int wcsspn(string, control) - find init substring of control chars
*
*Purpose:
*	Finds the index of the first character in string that does belong
*	to the set of characters specified by control.	This is
*	equivalent to the length of the initial substring of string that
*	consists entirely of characters from control.  The L'\0' character
*	that terminates control is not considered in the matching process
*	(wide-character strings).
*
*Entry:
*	wchar_t *string - string to search
*	wchar_t *control - string containing characters not to search for
*
*Exit:
*	returns index of first wchar_t in string not in control
*
*Exceptions:
*
*******************************************************************************/

size_t wcsspn(const wchar_t * string, const wchar_t * control) {
    wchar_t *str = (wchar_t *) string;
    wchar_t *ctl;
    /* 1st char not in control string stops search */
    while (*str) {
        for (ctl = (wchar_t *)control; *ctl != *str; ctl++)
            if (*ctl == 0) /* reached end of control string without finding a match */
                return str - string;
        str++;
    }
    /* The whole string consisted of characters from control */
    return str - string;
}

size_t strspn(const char * string, const char * control) {
    char *str = (char *) string;
    char *ctl;
    /* 1st char not in control string stops search */
    while (*str) {
        for (ctl = (char *)control; *ctl != *str; ctl++)
            if (*ctl == 0) /* reached end of control string without finding a match */
                return str - string;
        str++;
    }
    /* The whole string consisted of characters from control */
    return str - string;
}

/******************************************************************************
*wchar_t *wcsstr(string1, string2) - search for string2 in string1 
*	(wide strings)
*
*Purpose:
*	finds the first occurrence of string2 in string1 (wide strings)
*
*Entry:
*	wchar_t *string1 - string to search in
*	wchar_t *string2 - string to search for
*
*Exit:
*	returns a pointer to the first occurrence of string2 in
*	string1, or NULL if string2 does not occur in string1
*
*Uses:
*
*Exceptions:
*
********************************************************************************/

wchar_t * wcsstr (const wchar_t * wcs1, const wchar_t * wcs2) {
	wchar_t *cp = (wchar_t *) wcs1;
	wchar_t *s1, *s2;
	while (*cp) {
		s1 = cp;
		s2 = (wchar_t *) wcs2;
		while ( *s1 && *s2 && !(*s1-*s2) )
			s1++, s2++;
		if (!*s2)
			return(cp);
		cp++;
	}
	return(NULL);
}

/***
*wchar_t *wcstok(string, control) - tokenize string with delimiter in control
*	(wide-characters)
*
*Purpose:
*	wcstok considers the string to consist of a sequence of zero or more
*	text tokens separated by spans of one or more control chars. the first
*	call, with string specified, returns a pointer to the first wchar_t of
*	the first token, and will write a null wchar_t into string immediately
*	following the returned token. subsequent calls with zero for the first
*	argument (string) will work thru the string until no tokens remain. the
*	control string may be different from call to call. when no tokens remain
*	in string a NULL pointer is returned. remember the control chars with a
*	bit map, one bit per wchar_t. the null wchar_t is always a control char
*	(wide-characters).
*
*Entry:
*	wchar_t *string - wchar_t string to tokenize, or NULL to get next token
*	wchar_t *control - wchar_t string of characters to use as delimiters
*
*Exit:
*	returns pointer to first token in string, or if string
*	was NULL, to next token
*	returns NULL when no more tokens remain.
*
*Uses:
*
*Exceptions:
*
*******************************************************************************/

wchar_t * wcstok(wchar_t * string, const wchar_t * control) {
	wchar_t *token;
	const wchar_t *ctl;
	crtGlob_t *pcrt;
	if (!(pcrt = GetCRTStorage()))
		return 0;

	/* If string==NULL, continue with previous string */
	if (!string)
		string = pcrt->wnexttoken;

	/* Find beginning of token (skip over leading delimiters). Note that
	 * there is no token iff this loop sets string to point to the terminal
	 * null (*string == '\0') */

	while (*string) {
		for (ctl=control; *ctl && *ctl != *string; ctl++)
			;
		if (!*ctl) break;
		string++;
	}

	token = string;

	/* Find the end of the token. If it is not the end of the string,
	 * put a null there. */
	for ( ; *string ; string++ ) {
		for (ctl=control; *ctl && *ctl != *string; ctl++)
			;
		if (*ctl) {
			*string++ = '\0';
			break;
		}
	}

	pcrt->wnexttoken = string;
	/* Determine if a token has been found. */
	if (token == string)
		return NULL;
	else
		return token;
}

/***
*long _wtol(char *nptr) - Convert string to long
*
*Purpose:
*	Converts ASCII string pointed to by nptr to binary.
*	Overflow is not detected.
*
*Entry:
*	nptr = ptr to string to convert
*
*Exit:
*	return long int value of the string
*
*Exceptions:
*	None - overflow is not detected.
*
*******************************************************************************/

long _wtol(const WCHAR *nptr) {
	WCHAR c;			/* current char */
	long total=0;		/* current total */
	int sign, digit;		/* if '-', then negative, otherwise positive */
	/* skip whitespace */
	while (iswspace(*nptr))
		++nptr;
    // check for hex "0x"
    if (TEXT('0') == *nptr)
		nptr++;
    if (TEXT('x') == *nptr || TEXT('X') == *nptr) {
        nptr++; // skip the x
        while (c=*nptr++) {
            if (c >= TEXT('0') && c <= TEXT('9'))
                digit = c - TEXT('0');
            else if (c >= TEXT('a') && c <= TEXT('f'))
                digit = (c - TEXT('a')) + 10;
            else if (c >= TEXT('A') && c <= TEXT('F'))
                digit = (c - TEXT('A')) + 10;
            else // invalid character
                break;
            total = 16 * total + digit;
        }
        return total;
    }
    // We are in decimal.    
	c = *nptr++;
	sign = c;		/* save sign indication */
	if (c == TEXT('-') || c == TEXT('+'))
		c = *nptr++;	/* skip sign */
	while (c >= TEXT('0') && c <= TEXT('9')) {
		total = 10 * total + (c - TEXT('0')); 	/* accumulate digit */
		c = *nptr++;	/* get next char */
	}
	if (sign == TEXT('-'))
		return -total;
	else
		return total;	/* return result, negated if necessary */
}

/***
*long _wtoll(char *nptr) - Convert string to 64 bit integer
*
*Purpose:
*	Converts ASCII string pointed to by nptr to binary.
*	Overflow is not detected.
*
*Entry:
*	nptr = ptr to string to convert
*
*Exit:
*	return __int64 value of the string
*
*Exceptions:
*	None - overflow is not detected.
*
*******************************************************************************/

__int64 _wtoll(const WCHAR *nptr) {
	WCHAR c;			/* current char */
	__int64 total;		/* current total */
	int sign;		/* if '-', then negative, otherwise positive */
	/* skip whitespace */
	while (iswspace(*nptr))
		++nptr;
	c = *nptr++;
	sign = c;		/* save sign indication */
	if (c == TEXT('-') || c == TEXT('+'))
		c = *nptr++;	/* skip sign */
	total = 0;
	while (c >= TEXT('0') && c <= TEXT('9')) {
		total = 10 * total + (c - TEXT('0')); 	/* accumulate digit */
		c = *nptr++;	/* get next char */
	}
	if (sign == TEXT('-'))
		return -total;
	else
		return total;	/* return result, negated if necessary */
}

__int64 _atoi64(const char *nptr) {
	char c;			/* current char */
	__int64 total;		/* current total */
	int sign;		/* if '-', then negative, otherwise positive */
	/* skip whitespace */
	while (isspace(*nptr))
		++nptr;
	c = *nptr++;
	sign = c;		/* save sign indication */
	if (c == '-' || c == '+')
		c = *nptr++;	/* skip sign */
	total = 0;
	while (c >= '0' && c <= '9') {
		total = 10 * total + (c - '0'); 	/* accumulate digit */
		c = *nptr++;	/* get next char */
	}
	if (sign == '-')
		return -total;
	else
		return total;	/* return result, negated if necessary */
}

char * _strlwr(char *pstr) {
	char *pTrav;
	for (pTrav = pstr; *pTrav; pTrav++)
		*pTrav = _tolower(*pTrav);
	return pstr;
}

char * _strupr(char *pstr) {
	char *pTrav;
	for (pTrav = pstr; *pTrav; pTrav++)
		*pTrav = _toupper(*pTrav);
	return pstr;
}

