// Copyright (c) 1999-2000 Microsoft Corporation.  All rights reserved.
#ifndef __WAVEDBG_H__
#define __WAVEDBG_H__

#ifdef __cplusplus
extern "C" {
#endif

//
// DEBUG message support
//

#ifdef DEBUG
//
// For debug builds, use the real zones.
//
#define ZONE_TEST       DEBUGZONE(0)
#define ZONE_PARAMS     DEBUGZONE(1)
#define ZONE_VERBOSE    DEBUGZONE(2)
#define ZONE_INTERRUPT  DEBUGZONE(3)
#define ZONE_WODM       DEBUGZONE(4)
#define ZONE_WIDM       DEBUGZONE(5)
#define ZONE_PDD        DEBUGZONE(6)
#define ZONE_MDD        DEBUGZONE(7)
#define ZONE_SPS        DEBUGZONE(8)
#define ZONE_MISC       DEBUGZONE(9)
#define ZONE_10         DEBUGZONE(10)
#define ZONE_IOCTL      DEBUGZONE(11)
#define ZONE_ALLOC      DEBUGZONE(12)
#define ZONE_FUNCTION   DEBUGZONE(13)
#define ZONE_WARN       DEBUGZONE(14)
#define ZONE_ERROR      DEBUGZONE(15)

#define PRINTMSG DEBUGMSG

#else
//
// For retail builds, use forced messages based on the zones turned on below.
//
#define BIG_SWITCH  0
#define ZONE_TEST       BIG_SWITCH
#define ZONE_PARAMS     BIG_SWITCH
#define ZONE_VERBOSE    0 //BIG_SWITCH
#define ZONE_INTERRUPT  0 //BIG_SWITCH
#define ZONE_WODM       BIG_SWITCH
#define ZONE_WIDM       BIG_SWITCH
#define ZONE_PDD        BIG_SWITCH
#define ZONE_MDD        BIG_SWITCH
#define ZONE_SPS        BIG_SWITCH
#define ZONE_MISC       BIG_SWITCH
#define ZONE_10         BIG_SWITCH
#define ZONE_IOCTL      BIG_SWITCH
#define ZONE_ALLOC      BIG_SWITCH
#define ZONE_FUNCTION   BIG_SWITCH
#define ZONE_WARN       BIG_SWITCH
#define ZONE_ERROR      1

#define PRINTMSG RETAILMSG

#endif

#ifdef FILELOGGING
#pragma message ("Using the PPLOG functions")
HANDLE pplog_open  (PTCHAR szFilename);
BOOL   pplog_close (HANDLE hLog);
BOOL   pplog_write (HANDLE hLog, DWORD dwCondition, PTCHAR szString);
BOOL   pplog_flush (HANDLE hLog);
#else
#pragma message ("Stubbing the PPLOG functions")
#define pplog_open(x)          0
#define pplog_close(x)         0
#define pplog_write(x,y,z)     0
#define pplog_flush(x)         0
#endif


extern HANDLE hLog;
extern HANDLE hLog2;
//-----------------------------------------------------------------------------
// DEBUG MACROS 
//
//#define FUNC(x)    PRINTMSG(ZONE_FUNCTION, (TEXT("%a(%d) : [FUNCTION] %a()\r\n"), __FILE__, __LINE__, x))  
#ifdef FILELOGGING
#define FUNC(x)       {TCHAR szTmpBuff[256]; wsprintf(szTmpBuff, (TEXT("[FUNC] ")TEXT( #x )TEXT("()\r\n"))); pplog_write(hLog, 1, szTmpBuff);}
#define FUNC_WAPI(x)  {TCHAR szTmpBuff[256]; wsprintf(szTmpBuff, (TEXT("[FUNC] ")TEXT( #x )TEXT("()\r\n"))); pplog_write(hLog2, 1, szTmpBuff);}
#define FUNC_SPS(x)   {TCHAR szTmpBuff[256]; wsprintf(szTmpBuff, (TEXT("[FUNC] ")TEXT( #x )TEXT("()\r\n"))); pplog_write(hLog, 1, szTmpBuff);}
#else
#define FUNC(x)    PRINTMSG(ZONE_FUNCTION, (TEXT("[FUNC] %a()\r\n"), x))  
#define FUNC_WAPI(x)    PRINTMSG(ZONE_FUNCTION, (TEXT("[FUNC] %a()\r\n"), x))  
#define FUNC_SPS(x)     PRINTMSG(ZONE_FUNCTION || ZONE_SPS, (TEXT("[FUNC] %a()\r\n"), x))  
#endif

#define FUNC_WMDD(x)    PRINTMSG(ZONE_FUNCTION && ZONE_MDD, (TEXT("\t[FUNC] %a()\r\n"), x))  
#define FUNC_WPDD(x)    PRINTMSG(ZONE_FUNCTION && ZONE_PDD, (TEXT("\t\t[FUNC] %a()\r\n"), x))  

#define FUNC_VERBOSE(x)  PRINTMSG(ZONE_FUNCTION && ZONE_VERBOSE, (TEXT("[FUNC] %a()\r\n"), x))  
#define MSG1(x)    PRINTMSG(ZONE_MISC,     (TEXT("%a(%d) : [MSG] %a\r\n"), __FILE__, __LINE__, x))  
//#define ERRMSG(x) PRINTMSG(ZONE_ERROR,    (TEXT("%a(%d) : [ERROR] %a()\r\n"), __FILE__, __LINE__, x))  
//#define WRNMSG1(x) PRINTMSG(ZONE_WARN,     (TEXT("%a(%d) : [WARNING] %a()\r\n"), __FILE__, __LINE__, x))  
#define WRNMSG1(x) PRINTMSG(ZONE_WARN,     (TEXT("[WARNING] %a\r\n"), x))  
//#define TESTMSG(x) PRINTMSG(ZONE_TEST,     (TEXT("%a(%d) : [TEST] %a()\r\n"), __FILE__, __LINE__, x))  
#define TESTMSG(x) PRINTMSG(ZONE_TEST,     (TEXT("[TEST] %a\r\n"), x))  

#define INTMSG(x) PRINTMSG(ZONE_INTERRUPT, (TEXT("[INTR] %a\r\n"), x))  

#define MISC1(str,x1)     PRINTMSG(ZONE_MISC, (TEXT("[MISC] ")TEXT( #str )TEXT("\r\n"), x1))
#define MISC2(str,x1, x2) PRINTMSG(ZONE_MISC, (TEXT("[MISC] ")TEXT( #str )TEXT("\r\n"), x1, x2))

#define ERRMSG(str)            PRINTMSG(ZONE_ERROR, (TEXT("[ERROR] ")TEXT( #str )TEXT("\r\n")))
#define ERRMSG1(str, x1)       PRINTMSG(ZONE_ERROR, (TEXT("[ERROR] ")TEXT( #str )TEXT("\r\n"), x1))
#define ERRMSG2(str, x1, x2)   PRINTMSG(ZONE_ERROR, (TEXT("[ERROR] ")TEXT( #str )TEXT("\r\n"), x1, x2))

#define DECVALUE(x) PRINTMSG(ZONE_MISC, (TEXT("\tVALUE : ")TEXT( #x )TEXT(" = %d\r\n"), x))
#define HEXVALUE(x) PRINTMSG(ZONE_MISC, (TEXT("\tVALUE : ")TEXT( #x )TEXT(" = 0x%X\r\n"), x))

#define DECPARAM(x) PRINTMSG(ZONE_FUNCTION && ZONE_PARAMS, (TEXT("\tPARAM : ")TEXT( #x )TEXT(" = %d\r\n"), x))
#define HEXPARAM(x) PRINTMSG(ZONE_FUNCTION && ZONE_PARAMS, (TEXT("\tPARAM : ")TEXT( #x )TEXT(" = 0x%X\r\n"), x))




#ifdef __cplusplus
}
#endif

#endif /* __WAVEDBG_H__ */
