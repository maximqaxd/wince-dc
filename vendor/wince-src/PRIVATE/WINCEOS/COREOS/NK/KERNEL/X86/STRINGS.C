/* Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved. */

typedef unsigned short wchar_t;

unsigned int strlenW(const wchar_t *wcs)
{
	const wchar_t *eos = wcs;

	while (*eos)
	    ++eos;

	return eos-wcs;
}


int strcmpW(const wchar_t *pwc1, const wchar_t *pwc2)
{
	int ret = 0;

	while ( !(ret = *pwc1 - *pwc2) && *pwc2)
		++pwc1, ++pwc2;
	return ret;
}



