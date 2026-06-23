/*
 *		Core DLL filetime / systemtime code
 *
 *		Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.
 *
 * Module Name:
 *
 *		time.c
 *
 * Abstract:
 *
 *		This file implements Core DLL system and file time functions
 *
 * Revision History:
 *
 */

#include <windows.h>
#include <memory.h>
#include <notify.h>

extern BOOL IsAPIReady(DWORD hAPI);

#define WEEKDAY_OF_1601	1

#define IsLeapYear(Y) (!((Y)%4) && (((Y)%100) || !((Y)%400)))

#define NumberOfLeapYears(Y) ((Y)/4 - (Y)/100 + (Y)/400)

#define ElapsedYearsToDays(Y) ((Y)*365 + NumberOfLeapYears(Y))

#define MaxDaysInMonth(Y,M) (IsLeapYear(Y) ? \
	LeapYearDaysBeforeMonth[(M) + 1] - LeapYearDaysBeforeMonth[M] : \
    NormalYearDaysBeforeMonth[(M) + 1] - NormalYearDaysBeforeMonth[M])

const WORD LeapYearDaysBeforeMonth[13] = {
    0,                                 // January
    31,                                // February
    31+29,                             // March
    31+29+31,                          // April
    31+29+31+30,                       // May
    31+29+31+30+31,                    // June
    31+29+31+30+31+30,                 // July
    31+29+31+30+31+30+31,              // August
    31+29+31+30+31+30+31+31,           // September
    31+29+31+30+31+30+31+31+30,        // October
    31+29+31+30+31+30+31+31+30+31,     // November
    31+29+31+30+31+30+31+31+30+31+30,  // December
    31+29+31+30+31+30+31+31+30+31+30+31};

const WORD NormalYearDaysBeforeMonth[13] = {
    0,                                 // January
    31,                                // February
    31+28,                             // March
    31+28+31,                          // April
    31+28+31+30,                       // May
    31+28+31+30+31,                    // June
    31+28+31+30+31+30,                 // July
    31+28+31+30+31+30+31,              // August
    31+28+31+30+31+30+31+31,           // September
    31+28+31+30+31+30+31+31+30,        // October
    31+28+31+30+31+30+31+31+30+31,     // November
    31+28+31+30+31+30+31+31+30+31+30,  // December
    31+28+31+30+31+30+31+31+30+31+30+31};

const BYTE LeapYearDayToMonth[366] = {
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // January
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,        // February
     2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,  // March
     3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,     // April
     4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,  // May
     5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,     // June
     6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,  // July
     7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,  // August
     8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,     // September
     9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,  // October
    10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,     // November
    11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11}; // December

const BYTE NormalYearDayToMonth[365] = {
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // January
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,           // February
     2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,  // March
     3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,     // April
     4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,  // May
     5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,     // June
     6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,  // July
     7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,  // August
     8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,     // September
     9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,  // October
    10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,     // November
    11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11}; // December

/*
	@doc BOTH EXTERNAL
	
	@func LONG | CompareFileTime | Compares two 64-bit file times.
    @parm CONST FILETIME * | lpFileTime1 | address of first file time
    @parm CONST FILETIME * | lpFileTime2 | address of second file time
 
	@devnote CHOOSE COMM TAG FOR ONLY ONE OF THE FOLLOWING:
	@comm Follows the Win32 reference description without restrictions or modifications. 

*/
LONG WINAPI CompareFileTime(const FILETIME *lpft1, const FILETIME *lpft2) {
	if (lpft1->dwHighDateTime < lpft2->dwHighDateTime)
		return -1;
	else if (lpft1->dwHighDateTime > lpft2->dwHighDateTime)
		return 1;
	else if (lpft1->dwLowDateTime < lpft2->dwLowDateTime)
		return -1;
	else if (lpft1->dwLowDateTime > lpft2->dwLowDateTime)
		return 1;
	return 0;
}

void mul64_32_64(const FILETIME *lpnum1, DWORD num2, LPFILETIME lpres) {
	__int64 num1;
	num1 = (__int64)lpnum1->dwLowDateTime * (__int64)num2;
	num1 += ((__int64)lpnum1->dwHighDateTime * (__int64)num2)<<32;
	lpres->dwHighDateTime = (DWORD)(num1>>32);
	lpres->dwLowDateTime = (DWORD)(num1&0xffffffff);
}

void add64_32_64(const FILETIME *lpnum1, DWORD num2, LPFILETIME lpres) {
	DWORD bottom = lpnum1->dwLowDateTime + num2;
	lpres->dwHighDateTime = lpnum1->dwHighDateTime +
		(bottom < lpnum1->dwLowDateTime ? 1 : 0);
	lpres->dwLowDateTime = bottom;
}

#ifdef THUMB

#pragma optimize("",off)
#endif
void add64_64_64(const FILETIME *lpnum1, LPFILETIME lpnum2, LPFILETIME lpres) {
	__int64 num1, num2;
	num1 = (((__int64)lpnum1->dwHighDateTime)<<32)+(__int64)lpnum1->dwLowDateTime;
	num2 = (((__int64)lpnum2->dwHighDateTime)<<32)+(__int64)lpnum2->dwLowDateTime;
	num1 += num2;
	lpres->dwHighDateTime = (DWORD)(num1>>32);
	lpres->dwLowDateTime = (DWORD)(num1&0xffffffff);
}

void sub64_64_64(const FILETIME *lpnum1, LPFILETIME lpnum2, LPFILETIME lpres) {
	__int64 num1, num2;
	num1 = (((__int64)lpnum1->dwHighDateTime)<<32)+(__int64)lpnum1->dwLowDateTime;
	num2 = (((__int64)lpnum2->dwHighDateTime)<<32)+(__int64)lpnum2->dwLowDateTime;
	num1 -= num2;
	lpres->dwHighDateTime = (DWORD)(num1>>32);
	lpres->dwLowDateTime = (DWORD)(num1&0xffffffff);
}
#ifdef THUMB

#pragma optimize("",on)
#endif

// Unsigned divide
// Divides a 64 bit number by a *31* bit number.  Doesn't work for 32 bit divisors!

void div64_32_64(const FILETIME *lpdividend, DWORD divisor, LPFILETIME lpresult) {
	DWORD bitmask;
	DWORD top;
	FILETIME wholetop = *lpdividend;
	top = 0;
	lpresult->dwHighDateTime = 0;
	for (bitmask = 0x80000000; bitmask; bitmask >>= 1) {
		top = (top<<1) + ((wholetop.dwHighDateTime&bitmask) ? 1 : 0);
		if (top >= divisor) {
			top -= divisor;
			lpresult->dwHighDateTime |= bitmask;
		}
	}
	lpresult->dwLowDateTime = 0;
	for (bitmask = 0x80000000; bitmask; bitmask >>= 1) {
		top = (top<<1) + ((wholetop.dwLowDateTime&bitmask) ? 1 : 0);
		if (top >= divisor) {
			top -= divisor;
			lpresult->dwLowDateTime |= bitmask;
		}
	}
}

void DaysAndFractionToTime(ULONG ElapsedDays, ULONG Milliseconds, LPFILETIME lpTime) {
	lpTime->dwLowDateTime = ElapsedDays;
	lpTime->dwHighDateTime = 0;
	// Get number of milliseconds from days
	mul64_32_64(lpTime,86400000,lpTime);
	// Add in number of milliseconds
	add64_32_64(lpTime,Milliseconds,lpTime);
	// Convert to 100ns periods
	mul64_32_64(lpTime,10000,lpTime);
}

VOID TimeToDaysAndFraction(const FILETIME *lpTime, LPDWORD lpElapsedDays, LPDWORD lpMilliseconds) {
	FILETIME Temp, Temp2;
	// Convert 100ns periods to milliseconds
	Temp = *lpTime;
	div64_32_64(&Temp,10000,&Temp);
	// Get number of days
	div64_32_64(&Temp,86400000,&Temp2);
	*lpElapsedDays = Temp2.dwLowDateTime;
	mul64_32_64(&Temp2,86400000,&Temp2);
	sub64_64_64(&Temp,&Temp2,&Temp);
	*lpMilliseconds = Temp.dwLowDateTime;
}

ULONG ElapsedDaysToYears(ULONG ElapsedDays) {
    ULONG NumberOf400s;
    ULONG NumberOf100s;
    ULONG NumberOf4s;

    //  A 400 year time block is 146097 days
    NumberOf400s = ElapsedDays / 146097;
    ElapsedDays -= NumberOf400s * 146097;
    //  A 100 year time block is 36524 days
    //  The computation for the number of 100 year blocks is biased by 3/4 days per
    //  100 years to account for the extra leap day thrown in on the last year
    //  of each 400 year block.
	NumberOf100s = (ElapsedDays * 100 + 75) / 3652425;
    ElapsedDays -= NumberOf100s * 36524;
	//  A 4 year time block is 1461 days
    NumberOf4s = ElapsedDays / 1461;
    ElapsedDays -= NumberOf4s * 1461;
	return (NumberOf400s * 400) + (NumberOf100s * 100) +
           (NumberOf4s * 4) + (ElapsedDays * 100 + 75) / 36525;
}

BOOL IsValidSystemTime(const SYSTEMTIME *lpst) {
	if ((lpst->wYear < 1601) ||
		(lpst->wMonth < 1) ||
		(lpst->wMonth > 12) ||
		(lpst->wDay < 1) ||
		(lpst->wDay > MaxDaysInMonth(lpst->wYear,lpst->wMonth-1)) ||
		(lpst->wHour > 23) ||
		(lpst->wMinute > 59) ||
		(lpst->wSecond > 59) ||
		(lpst->wMilliseconds > 999))
		return FALSE;
	return TRUE;
}

/*
	@doc BOTH EXTERNAL
	
	@func BOOL | SystemTimeToFileTime | Converts a system time to a file time. 
    @parm CONST SYSTEMTIME * | lpst | address of system time to convert 
    @parm LPFILETIME | lpft | address of buffer for converted file time 

	@comm Follows the Win32 reference description without restrictions or modifications. 
*/
BOOL WINAPI SystemTimeToFileTime(const SYSTEMTIME *lpst, LPFILETIME lpft) {
    ULONG ElapsedDays;
    ULONG ElapsedMilliseconds;

	if (!IsValidSystemTime(lpst))
		return FALSE;
    ElapsedDays = ElapsedYearsToDays(lpst->wYear - 1601);
	ElapsedDays += (IsLeapYear(lpst->wYear) ?
		LeapYearDaysBeforeMonth[lpst->wMonth-1] :
		NormalYearDaysBeforeMonth[lpst->wMonth-1]);
    ElapsedDays += lpst->wDay - 1;
    ElapsedMilliseconds = (((lpst->wHour*60) + lpst->wMinute)*60 +
	   lpst->wSecond)*1000 + lpst->wMilliseconds;
    DaysAndFractionToTime(ElapsedDays, ElapsedMilliseconds, lpft);
    return TRUE;
}

/*
	@doc BOTH EXTERNAL
	
	@func BOOL | FileTimeToSystemTime | Converts a 64-bit file time to system time format. 
    @parm CONST FILETIME * | lpFileTime | pointer to file time to convert 
    @parm LPSYSTEMTIME | lpSystemTime | pointer to structure to receive system time  

	@comm Follows the Win32 reference description without restrictions or modifications. 

*/
BOOL WINAPI FileTimeToSystemTime(const FILETIME *lpft, LPSYSTEMTIME lpst) {
    ULONG Days;
	ULONG Years;
	ULONG Minutes;
	ULONG Seconds;
    ULONG Milliseconds;

    TimeToDaysAndFraction(lpft, &Days, &Milliseconds );
    lpst->wDayOfWeek = (WORD)((Days + WEEKDAY_OF_1601) % 7);
	Years = ElapsedDaysToYears(Days);
    Days = Days - ElapsedYearsToDays(Years);
    if (IsLeapYear(Years + 1)) {
        lpst->wMonth = (WORD)(LeapYearDayToMonth[Days] + 1);
        Days = Days - LeapYearDaysBeforeMonth[lpst->wMonth-1];
    } else {
        lpst->wMonth = (WORD)(NormalYearDayToMonth[Days] + 1);
        Days = Days - NormalYearDaysBeforeMonth[lpst->wMonth-1];
    }
	Seconds = Milliseconds/1000;
    lpst->wMilliseconds = (WORD)(Milliseconds % 1000);
    Minutes = Seconds / 60;
    lpst->wSecond = (WORD)(Seconds % 60);
    lpst->wHour = (WORD)(Minutes / 60);
    lpst->wMinute = (WORD)(Minutes % 60);
    lpst->wYear = (WORD)(Years + 1601);
    lpst->wDay = (WORD)(Days + 1);
    return TRUE;
}

/*
	@doc BOTH EXTERNAL
	
	@func BOOL | FileTimeToLocalFileTime | Converts a file time based on the 
	Coordinated Universal Time (UTC) to a local file time. 
    @parm CONST FILETIME * | lpFileTime | address of UTC file time to convert  
    @parm LPFILETIME | lpLocalFileTime | address of converted file time 

	@comm Follows the Win32 reference description without restrictions or modifications. 

*/
BOOL WINAPI FileTimeToLocalFileTime(const FILETIME * lpft, LPFILETIME lpftLocal) {
	FILETIME bias;
	bias.dwHighDateTime = 0;
	if (UserKInfo[KINX_TIMEZONEBIAS] < 0)
		bias.dwLowDateTime = - UserKInfo[KINX_TIMEZONEBIAS];
	else
		bias.dwLowDateTime = UserKInfo[KINX_TIMEZONEBIAS];
	mul64_32_64(&bias,600000000L,&bias);
	if (UserKInfo[KINX_TIMEZONEBIAS] < 0)
		add64_64_64(lpft,&bias,lpftLocal);
	else
		sub64_64_64(lpft,&bias,lpftLocal);
	return TRUE;
}

/*
	@doc BOTH EXTERNAL
	
	@func BOOL | LocalFileTimeToFileTime | Converts a local file time to a file time based 
	on the Coordinated Universal Time (UTC). 
    @parm CONST FILETIME * | lpftLocal | address of local file time to convert 
    @parm LPFILETIME | lpft | address of converted file time 

	@comm Follows the Win32 reference description without restrictions or modifications. 
*/
BOOL WINAPI LocalFileTimeToFileTime(const FILETIME *lpftLocal, LPFILETIME lpft) {
	FILETIME bias;
	bias.dwHighDateTime = 0;
	if (UserKInfo[KINX_TIMEZONEBIAS] < 0)
		bias.dwLowDateTime = - UserKInfo[KINX_TIMEZONEBIAS];
	else
		bias.dwLowDateTime = UserKInfo[KINX_TIMEZONEBIAS];
	mul64_32_64(&bias,600000000L,&bias);
	if (UserKInfo[KINX_TIMEZONEBIAS] < 0)
		sub64_64_64(lpftLocal,&bias,lpft);
	else
		add64_64_64(lpftLocal,&bias,lpft);
	return TRUE;
}

/*
	@doc BOTH EXTERNAL
	
	@func VOID | GetSystemTime | Retrieves the current system date and time. 
	The system time is expressed in Coordinated Universal Time (UTC). 
    @parm LPSYSTEMTIME | lpst | address of system time structure  

	@comm Follows the Win32 reference description without restrictions or modifications. 
*/
VOID WINAPI GetSystemTime (LPSYSTEMTIME lpSystemTime) {
	FILETIME ft;
	GetRealTime(lpSystemTime);
	SystemTimeToFileTime(lpSystemTime,&ft);
	LocalFileTimeToFileTime(&ft,&ft);
	FileTimeToSystemTime(&ft,lpSystemTime);
}

/*
	@doc BOTH EXTERNAL
	
	@func BOOL | SetSystemTime | Sets the current system time and date. The system time 
	is expressed in Coordinated Universal Time (UTC). 
    @parm CONST SYSTEMTIME * | lpst | address of system time to set 

	@comm Follows the Win32 reference description without restrictions or modifications. 
*/
BOOL WINAPI SetSystemTime (const SYSTEMTIME *lpSystemTime) {
	SYSTEMTIME st;
	FILETIME ft;
	if (!SystemTimeToFileTime(lpSystemTime,&ft) || 
		!FileTimeToLocalFileTime(&ft,&ft) ||
		!FileTimeToSystemTime(&ft,&st) ||
		!SetRealTime(&st)) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}
	if (IsAPIReady(SH_WMGR))
		CeEventHasOccurred(NOTIFICATION_EVENT_TIME_CHANGE, NULL);
	RefreshKernelAlarm();
	return TRUE;
}

/*
	@doc BOTH EXTERNAL
	
	@func VOID | GetLocalTime | Retrieves the current local date and time. 
    @parm LPSYSTEMTIME |lpst | address of system time structure  

	@comm Follows the Win32 reference description without restrictions or modifications. 

*/
VOID WINAPI GetLocalTime (LPSYSTEMTIME lpSystemTime) {
	FILETIME ft;
	GetRealTime(lpSystemTime);
	SystemTimeToFileTime(lpSystemTime,&ft);
	FileTimeToSystemTime(&ft,lpSystemTime);
}

// Internal file system function - gets current UMT file time
VOID GetCurrentFT(LPFILETIME lpFileTime) {
	SYSTEMTIME st;
	GetRealTime(&st);
	SystemTimeToFileTime(&st,lpFileTime);
	LocalFileTimeToFileTime(lpFileTime,lpFileTime);
}

/*
	@doc BOTH EXTERNAL
	
	@func BOOL | SetLocalTime | Sets the current local time and date.
    @parm CONST SYSTEMTIME * | lpst | address of local time to set 

	@comm Follows the Win32 reference description without restrictions or modifications. 
*/
BOOL WINAPI SetLocalTime (const SYSTEMTIME *lpSystemTime) {
	SYSTEMTIME st;
	FILETIME ft;
	if (!SystemTimeToFileTime(lpSystemTime,&ft) || 
		!FileTimeToSystemTime(&ft,&st) ||
		!SetRealTime(&st)) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}
	if (IsAPIReady(SH_WMGR))
		CeEventHasOccurred(NOTIFICATION_EVENT_TIME_CHANGE, NULL);
	RefreshKernelAlarm();
	return TRUE;
}

/*
	@doc BOTH EXTERNAL
	
	@func DWORD | GetTimeZoneInformation | Retrieves the current time-zone parameters. 
	These parameters control the translations between Coordinated Universal Time (UTC) 
	and local time. 
    @parm LPTIME_ZONE_INFORMATION | lptzi | address of time-zone settings 

	@devnote CHOOSE COMM TAG FOR ONLY ONE OF THE FOLLOWING:
	@comm Follows the Win32 reference description with the following restrictions:
			Always return TIME_ZONE_ID_UNKNOWN;

*/
#define TZIStrCpyW(D,S) memcpy(D,S,sizeof(S))

#define GetKeyHKLM(valname,keyname,lptype,data,lplen)   RegQueryValueExW(HKEY_LOCAL_MACHINE,valname,(LPDWORD)keyname,lptype,(LPBYTE)data,lplen)
#define OpenKeyHKLM(keyname,pkeyret)                    RegOpenKeyExW(HKEY_LOCAL_MACHINE,keyname,0, 0,pkeyret)

#define GetKey(keyin,valname,lptype,data,lplen)         RegQueryValueExW(keyin,valname,0,lptype,(LPBYTE)data,lplen)
#define OpenKey(keyin,keyname,pkeyret)                  RegOpenKeyExW(keyin,keyname,0, 0,pkeyret)

typedef struct tagTZREG {
    LONG    Bias;
    LONG    StandardBias;
    LONG    DaylightBias;
    SYSTEMTIME StandardDate;
    SYSTEMTIME DaylightDate;
} TZREG;

const TCHAR cszTimeZones[] = L"Time Zones";

void GetDefaultTimeZoneInformation (LPTIME_ZONE_INFORMATION lpTzi) 
{
	DWORD type, size1, size2;
	HKEY hk1=0, hk2=0;
	TZREG tzr;

	memset(lpTzi, 0, sizeof(TIME_ZONE_INFORMATION));
	memset(&tzr, 0, sizeof(tzr));
	size1 = sizeof(lpTzi->StandardName);
	size2 = sizeof(tzr);

	// see if we have a value "Default" under HKLM\Time Zones
	if(	(ERROR_SUCCESS==OpenKeyHKLM(L"Time Zones", &hk1)) &&
		(ERROR_SUCCESS==GetKey(hk1, L"Default",&type,lpTzi->StandardName,&size1)) &&
		(ERROR_SUCCESS==OpenKey(hk1, lpTzi->StandardName, &hk2)) &&
    	(ERROR_SUCCESS==GetKey(hk2,L"TZI",&type,&tzr,&size2)) )
    {
		// Read the value "TZI" and "Dlt" under HKLM\Time Zones\<time zone std name>
		size1 = sizeof(lpTzi->DaylightName);
	    GetKey(hk2,L"Dlt",&type,lpTzi->DaylightName,&size1);
   		lpTzi->Bias = tzr.Bias;
        lpTzi->StandardBias = tzr.StandardBias;
        lpTzi->DaylightBias = tzr.DaylightBias;
        lpTzi->StandardDate = tzr.StandardDate;
		lpTzi->DaylightDate = tzr.DaylightDate;
	}
	else
	{
		// Default to Redmond, WA for now
		lpTzi->Bias = 480;
		TZIStrCpyW(lpTzi->StandardName,L"Pacific Standard Time");
		lpTzi->StandardDate.wYear = 0;
		lpTzi->StandardDate.wMonth = 10;
		lpTzi->StandardDate.wDayOfWeek = 0;
		lpTzi->StandardDate.wDay = 5;
		lpTzi->StandardDate.wHour = 2;
		lpTzi->StandardDate.wMinute = 0;
		lpTzi->StandardDate.wSecond = 0;
		lpTzi->StandardDate.wMilliseconds = 0;
		lpTzi->StandardBias = 0;
		TZIStrCpyW(lpTzi->DaylightName,L"Pacific Daylight Time");
		lpTzi->DaylightDate.wYear = 0;
		lpTzi->DaylightDate.wMonth = 4;
		lpTzi->DaylightDate.wDayOfWeek = 0;
		lpTzi->DaylightDate.wDay = 1;
		lpTzi->DaylightDate.wHour = 2;
		lpTzi->DaylightDate.wMinute = 0;
		lpTzi->DaylightDate.wSecond = 0;
		lpTzi->DaylightDate.wMilliseconds = 0;
		lpTzi->DaylightBias = -60;
	}
	if(hk1) RegCloseKey(hk1);
	if(hk2) RegCloseKey(hk2);
	ASSERT(lpTzi->StandardName && lpTzi->DaylightName);
}

DWORD WINAPI GetTimeZoneInformation (LPTIME_ZONE_INFORMATION lpTzi) 
{
	DWORD type;
	DWORD size = sizeof(TIME_ZONE_INFORMATION);
	// check for existing time-zone struct
	if (GetKeyHKLM(L"TimeZoneInformation",L"Time",&type,(LPBYTE)lpTzi,&size)) 
	{
		// fallback to default
		GetDefaultTimeZoneInformation(lpTzi);
	}
	if (lpTzi->Bias + lpTzi->StandardBias == UserKInfo[KINX_TIMEZONEBIAS])
		return TIME_ZONE_ID_STANDARD;
	else if (lpTzi->Bias + lpTzi->DaylightBias == UserKInfo[KINX_TIMEZONEBIAS])
		return TIME_ZONE_ID_DAYLIGHT;
	return TIME_ZONE_ID_UNKNOWN;
}

BOOL IsValidTZISystemTime(const SYSTEMTIME *lpst) {
	if (IsValidSystemTime(lpst))
		return TRUE;
	if (!lpst->wYear && (lpst->wDayOfWeek <= 6) && (lpst->wDay >= 1) && (lpst->wDay <= 5))
		return TRUE;
	if (!lpst->wMonth)
		return TRUE;
	return FALSE;
}

/*
	@doc BOTH EXTERNAL
	
	@func BOOL | SetTimeZoneInformation | Sets the current time-zone parameters. 
	These parameters control translations from Coordinated Universal Time (UTC) to local time.
    @parm CONST TIME_ZONE_INFORMATION * | lptzi | address of time-zone settings  

	@comm Follows the Win32 reference description without restrictions or modifications. 
*/
BOOL WINAPI SetTimeZoneInformation (const TIME_ZONE_INFORMATION *lpTzi) {
	DWORD dwDisp;
	HKEY  hKey=NULL;
	
	if (!IsValidTZISystemTime(&lpTzi->StandardDate) ||
		!IsValidTZISystemTime(&lpTzi->DaylightDate)) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}
	SetTimeZoneBias(lpTzi->Bias+lpTzi->StandardBias,lpTzi->Bias+lpTzi->DaylightBias);

	if(ERROR_SUCCESS==RegCreateKeyExW(HKEY_LOCAL_MACHINE, L"Time", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dwDisp) && hKey) {
		if(ERROR_SUCCESS==RegSetValueExW(hKey, L"TimeZoneInformation", 0, REG_BINARY, (LPBYTE)lpTzi,sizeof(TIME_ZONE_INFORMATION))) {
			if (IsAPIReady(SH_WMGR))
				CeEventHasOccurred(NOTIFICATION_EVENT_TZ_CHANGE, NULL);
			RegCloseKey(hKey);
			return TRUE;
		}
		RegCloseKey(hKey);
	}
	SetLastError(ERROR_OUTOFMEMORY);
	return FALSE;
}

