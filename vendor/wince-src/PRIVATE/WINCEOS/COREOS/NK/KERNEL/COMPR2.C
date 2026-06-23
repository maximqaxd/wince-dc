/*		Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved. */
#include <kernel.h>

#define STRSIZEMASK 0x3fff
#define STRPART1RAW 0x8000
#define STRPART2RAW 0x4000

extern CRITICAL_SECTION CompCS;

DWORD Decompress(LPBYTE BufIn, DWORD InSize, LPBYTE BufOut, DWORD OutSize, DWORD skip) {
	DWORD result;
	LockPages(BufIn,InSize,0,LOCKFLAG_READ);
	if (BufOut)
		LockPages(BufOut,OutSize,0,LOCKFLAG_WRITE);
	EnterCriticalSection(&CompCS);
	DEBUGCHK(!(skip & (PAGE_SIZE-1)));
	result = CEDecompress(BufIn,InSize,BufOut, OutSize, skip, 1, PAGE_SIZE);
	LeaveCriticalSection(&CompCS);
	UnlockPages(BufIn,InSize);
	if (BufOut)
		UnlockPages(BufOut,OutSize);
	return result;
}

// Compressors - bufin and bufout must be WORD aligned, lenout must be at least as
// large as lenin, CECOMPRESS_FAILED is returned if it doesn't fit, else compressed length
// CECOMPRESS_ALL_ZEROS returned if buffer entirely zero.  If bufout is NULL, the output
// length is computed, but the results are not stored.  Lenin must be a multiple of 2.
DWORD SC_FSStringCompress(LPBYTE bufin, DWORD lenin, LPBYTE bufout, DWORD lenout) {
	DWORD result, result2, newsize;
	LPBYTE ptr1, ptr2, ptrend;
	DEBUGMSG(ZONE_ENTRY,(L"SC_FSStringCompress entry: %8.8lx %8.8lx %8.8lx %8.8lx\r\n",bufin,lenin,bufout,lenout));
	LockPages(bufin,lenin,0,LOCKFLAG_READ);
	if (bufout)
		LockPages(bufout,lenout,0,LOCKFLAG_WRITE);
	EnterCriticalSection(&CompCS);
	result = CECompress(bufin,(WORD)((lenin+1)/2),(bufout ? bufout+2 : 0),lenout - 2, 2, 1024);
	if (result == CECOMPRESS_ALLZEROS) {
		newsize = 0;
		if (bufout)
			*(LPWORD)bufout = 0;
	} else if ((result != CECOMPRESS_FAILED) && (result < (lenin+1)/2)) {
		newsize = result;
		if (bufout)	{
			DEBUGCHK(newsize <= STRSIZEMASK);
			*(LPWORD)bufout = (WORD)newsize;
		}
	} else {
		newsize = (lenin+1)/2;
		if (newsize + 2 > lenout) {
			DEBUGMSG(ZONE_ENTRY,(L"SC_FSStringCompress exit 1: %8.8lx\r\n",CECOMPRESS_FAILED));
			LeaveCriticalSection(&CompCS);
			UnlockPages(bufin,lenin);
			if (bufout)
				UnlockPages(bufout,lenout);
			return CECOMPRESS_FAILED;
		}
		if (bufout) {
			DEBUGCHK(newsize <= STRSIZEMASK);
			*(LPWORD)bufout = STRPART1RAW | (WORD)newsize;
			for (ptr1 = &bufout[2], ptr2 = bufin, ptrend = ptr1+newsize; ptr1 < ptrend; ptr2+=2)
				*ptr1++ = *ptr2;
		}
	}
	result2 = CECompress(bufin+1,(WORD)(lenin/2),(bufout ? bufout+2+newsize : 0), lenout - 2 - newsize, 2, 1024);
	LeaveCriticalSection(&CompCS);
	UnlockPages(bufin,lenin);
	if (bufout)
		UnlockPages(bufout,lenout);
	if (result2 == CECOMPRESS_ALLZEROS) {
		if (result == CECOMPRESS_ALLZEROS) {
			DEBUGMSG(ZONE_ENTRY,(L"SC_FSStringCompress exit: %8.8lx\r\n",CECOMPRESS_ALLZEROS));
			return CECOMPRESS_ALLZEROS;
		} else {
			DEBUGMSG(ZONE_ENTRY,(L"SC_FSStringCompress exit 2: %8.8lx\r\n",2+newsize >= lenin ? CECOMPRESS_FAILED : 2+newsize));
			return (2+newsize >= lenin ? CECOMPRESS_FAILED : 2+newsize);
		}
	} else if ((result2 == CECOMPRESS_FAILED) || (result2 >= (lenin/2))) {
		result2 = lenin/2;
		if (2 + newsize + result2 > lenout) {
			DEBUGMSG(ZONE_ENTRY,(L"SC_FSStringCompress exit 3: %8.8lx\r\n",CECOMPRESS_FAILED));
			return CECOMPRESS_FAILED;
		}
		if (bufout) {
			*(LPWORD)bufout |= STRPART2RAW;
			for (ptr1 = &bufout[2+newsize], ptr2 = &bufin[1], ptrend = ptr1+(lenin/2); ptr1 < ptrend; ptr2+=2)
				*ptr1++ = *ptr2;
		}
	}
	DEBUGMSG(ZONE_ENTRY,(L"SC_FSStringCompress exit 4: %8.8lx\r\n",2+newsize+result2 >= lenin ? CECOMPRESS_FAILED : 2+newsize+result2));
	return (2+newsize+result2 >= lenin ? CECOMPRESS_FAILED : 2+newsize+result2);
}

// Decompressors - bufin and bufout must be WORD aligned, lenout *must* be large
// enough to handle the entire decompressed contents.
DWORD SC_FSStringDecompress(LPBYTE bufin, DWORD lenin, LPBYTE bufout, DWORD lenout) {
	DWORD newsize1, newsize2, insize1, insize2, loop;
	DEBUGMSG(ZONE_ENTRY,(L"SC_FSStringDecompress entry: %8.8lx %8.8lx %8.8lx %8.8lx\r\n",bufin,lenin,bufout,lenout));
	LockPages(bufin,lenin,0,LOCKFLAG_READ);
	if (bufout)
		LockPages(bufout,lenout,0,LOCKFLAG_WRITE);
	EnterCriticalSection(&CompCS);
	insize1 = *(LPWORD)bufin;
	insize2 = lenin - (insize1 & STRSIZEMASK) - 2;
	if (!(insize1 & STRSIZEMASK)) {
		for (loop = 0; loop < lenout; loop+=2)
			bufout[loop] = 0;
		newsize1 = 0;
	} else if (insize1 & STRPART1RAW) {
		newsize1 = (insize1 & STRSIZEMASK);
		for (loop = 0; loop < newsize1; loop++)
			bufout[loop*2] = bufin[2+loop];
	} else {
		newsize1 = CEDecompress(bufin+2,insize1&STRSIZEMASK,bufout,(lenout+1)/2,0,2, 1024);
		if (newsize1 == CEDECOMPRESS_FAILED) {
			DEBUGMSG(ZONE_ENTRY,(L"SC_FSStringDecompress exit 1: %8.8lx\r\n",CEDECOMPRESS_FAILED));
			LeaveCriticalSection(&CompCS);
			UnlockPages(bufin,lenin);
			if (bufout)
				UnlockPages(bufout,lenout);
			return CEDECOMPRESS_FAILED;
		}
	}
	if (!insize2) {
		for (loop = 1; loop < lenout; loop+=2)
			bufout[loop] = 0;
		newsize2 = 0;
	} else if (insize1 & STRPART2RAW) {
		newsize2 = insize2;
		for (loop = 0; loop < newsize2; loop++)
			bufout[loop*2+1] = bufin[2+(insize1&STRSIZEMASK)+loop];
	} else {
		newsize2 = CEDecompress(bufin+2+(insize1&STRSIZEMASK),insize2,bufout+1,lenout/2,0,2, 1024);
		if (newsize2 == CEDECOMPRESS_FAILED) {
			DEBUGMSG(ZONE_ENTRY,(L"SC_FSStringDecompress exit 2: %8.8lx\r\n",CEDECOMPRESS_FAILED));
			LeaveCriticalSection(&CompCS);
			UnlockPages(bufin,lenin);
			if (bufout)
				UnlockPages(bufout,lenout);
			return CEDECOMPRESS_FAILED;
		}
	}
	LeaveCriticalSection(&CompCS);
	UnlockPages(bufin,lenin);
	if (bufout)
		UnlockPages(bufout,lenout);
	if (newsize1 > newsize2) {
		DEBUGMSG(ZONE_ENTRY,(L"SC_FSStringDecompress exit 3: %8.8lx\r\n",newsize1*2));
		return (newsize1*2);
	}
	DEBUGMSG(ZONE_ENTRY,(L"SC_FSStringDecompress exit: %8.8lx\r\n",newsize2*2));
	return (newsize2*2);
}

// Compressors - bufin and bufout must be WORD aligned, lenout must be at least as
// large as lenin, CECOMPRESS_FAILED is returned if it doesn't fit, else compressed length
// CECOMPRESS_ALL_ZEROS returned if buffer entirely zero.  If bufout is NULL, the output
// length is computed, but the results are not stored.  Lenin must be a multiple of 2.
DWORD SC_FSBinaryCompress(LPBYTE bufin, DWORD lenin, LPBYTE bufout, DWORD lenout) {
	DWORD retval;
	DEBUGMSG(ZONE_ENTRY,(L"SC_FSBinaryCompress entry: %8.8lx %8.8lx %8.8lx %8.8lx\r\n",bufin,lenin,bufout,lenout));
	LockPages(bufin,lenin,0,LOCKFLAG_READ);
	if (bufout)
		LockPages(bufout,lenout,0,LOCKFLAG_WRITE);
	EnterCriticalSection(&CompCS);
	retval = CECompress(bufin,lenin,bufout,lenout,1, 1024);
	LeaveCriticalSection(&CompCS);
	UnlockPages(bufin,lenin);
	if (bufout)
		UnlockPages(bufout,lenout);
	DEBUGMSG(ZONE_ENTRY,(L"SC_FSBinaryCompress exit: %8.8lx\r\n",retval));
	return retval;
}

DWORD BufferedDecompress(LPBYTE bufin, DWORD lenin, LPBYTE bufout, DWORD lenout, DWORD skip) {
	BYTE buf[4096];	// file system blocking factor
	DWORD retval;
	LockPages(buf,4096,0,LOCKFLAG_WRITE);
	EnterCriticalSection(&CompCS);
	retval = CEDecompress(bufin,lenin,buf,lenout + (skip&1023),skip & ~1023,1, 1024);
	LeaveCriticalSection(&CompCS);
	if (retval != CEDECOMPRESS_FAILED)
		memcpy(bufout,buf+(skip&1023),lenout);
	UnlockPages(buf,4096);
	return retval;		
}

// Decompressors - bufin and bufout must be WORD aligned, lenout *must* be large
// enough to handle the entire decompressed contents.
DWORD SC_FSBinaryDecompress(LPBYTE bufin, DWORD lenin, LPBYTE bufout, DWORD lenout, DWORD skip) {
	DWORD retval;
	DEBUGMSG(ZONE_ENTRY,(L"SC_FSBinaryDecompress entry: %8.8lx %8.8lx %8.8lx %8.8lx\r\n",bufin,lenin,bufout,lenout));
	LockPages(bufin,lenin,0,LOCKFLAG_READ);
	if (bufout)
		LockPages(bufout,lenout,0,LOCKFLAG_WRITE);
	if (skip & 1023)
		retval = BufferedDecompress(bufin,lenin,bufout,lenout,skip);
	else {
		EnterCriticalSection(&CompCS);
		retval = CEDecompress(bufin,lenin,bufout,lenout,skip,1, 1024);
		LeaveCriticalSection(&CompCS);
	}
	UnlockPages(bufin,lenin);
	if (bufout)
		UnlockPages(bufout,lenout);
	DEBUGMSG(ZONE_ENTRY,(L"SC_FSBinaryDecompress exit: %8.8lx\r\n",retval));
	return retval;
}

