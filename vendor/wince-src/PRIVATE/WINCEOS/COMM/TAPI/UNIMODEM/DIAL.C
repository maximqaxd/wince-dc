// ******************************************************************
// **Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.
// **             
// ******************************************************************
//
//	dial.c		The dialing state machine for Unimodem
//
//	@doc	EX_TSPI
//
//	@topic	TSPI |
//
//		Tspi Stuff
//
//

#include "windows.h"
#include "types.h"
#include "memory.h"
#include "mcx.h"
#include "tspi.h"
#include "linklist.h"
#include "tspip.h"
#include "tapicomn.h"
#include "dial.h"
#include "termctrl.h"
#include "resource.h"

// Functions from modem.c
extern LONG ToggleCommMask(PTLINEDEV pLineDev);
extern void SetDCBfromDevMiniCfg(DCB * pDCB,PDEVMINICFG lpDevMiniCfg);
extern void WriteModemLog(PTLINEDEV pLineDev, UCHAR * szCommand, DWORD dwOp);

// Unimodem thread priority
extern DWORD g_dwUnimodemThreadPriority;

// Some keys we read to get modem related info from registry
const WCHAR szSettings[] =   TEXT("Settings");
const WCHAR szDialSuffix[] = TEXT("DialSuffix");
static const WCHAR szInit[] =   TEXT("Init");
#define MAXINITKEYS 5
static const PWCHAR szInitNum[MAXINITKEYS] = {
    TEXT("1"),
    TEXT("2"),
    TEXT("3"),
    TEXT("4"),
    TEXT("5") 
};

int strncmpi(char *dst, const char *src, long count)
{
    while (count) {
        if (toupper(*dst) != toupper(*src))
            return 1;
        if (*src == 0)
            return 0;
        dst++;
        src++;
        count--;
    }

    return 0;
}


/******************************************************************************

 @doc INTERNAL

 @api BOOL | ExpandMacros | Takes the string pszLine, and copies it to 
 lpszVal after expanding macros
 
 @parm char * | pszRegResponse | ptr to response string from registry.
 
 @parm char * | pszExpanded | ptr to buffer to copy string to w/ macros expanded
 
 @parm DWORD * | pdwValLen | length of pszVal w/ expanded macros.
 
 @rdesc Returns FALSE if a needed macro translation could not be found in the
 pMacroXlations table, TRUE otherwise.
 
*****************************************************************************/

BOOL
ExpandMacros(
    PWCHAR  pszRegResponse,
    PWCHAR  pszExpanded,
    DWORD * pdwValLen,
    MODEMMACRO * pMdmMacro,
    DWORD cbMacros
    )
{
    PWCHAR pszValue;
    DWORD cbTmp;
    BOOL  bFound;
    PWCHAR ptchTmp;
    DWORD i;
        
    DEBUGMSG(ZONE_DIAL, (TEXT("UNIMODEM:+ExpandMacros : Original \"%s\"\r\n"), pszRegResponse ));

    pszValue = pszExpanded;

    for ( ; *pszRegResponse; ) 
    {
        // check for a macro
        if ( *pszRegResponse == LMSCH ) 
        {
            // <cr>
            if (!wcsnicmp(pszRegResponse,CR_MACRO,CR_MACRO_LENGTH)) 
            {
                *pszValue++ = CR;
                pszRegResponse += CR_MACRO_LENGTH;
                continue;
            }
            // <lf>
            if (!wcsnicmp(pszRegResponse,LF_MACRO,LF_MACRO_LENGTH)) 
            {
                *pszValue++ = LF;
                pszRegResponse += LF_MACRO_LENGTH;
                continue;
            }
            // <hxx>
            if ((pszRegResponse[1] == 'h' || pszRegResponse[1] == 'H') &&
                     isxdigit(pszRegResponse[2]) &&
                     isxdigit(pszRegResponse[3]) &&
                     pszRegResponse[4] == RMSCH ) 
            {
                *pszValue++ = (char) ((ctox(pszRegResponse[2]) << 4) + ctox(pszRegResponse[3]));
                pszRegResponse += 5;
                continue;
            }
            // <macro>
            if (pMdmMacro) 
            {
                bFound = FALSE;
                // Check for a matching macro.
                for (i = 0; i < cbMacros; i++)
                {
                    cbTmp = wcslen(pMdmMacro[i].MacroName);
                    if (!wcsnicmp(pszRegResponse, pMdmMacro[i].MacroName, cbTmp))
                    {
                        ptchTmp = pMdmMacro[i].MacroValue;
                        while (*ptchTmp)
                        {
                            *pszValue++ = *ptchTmp++;
                        }
                        pszRegResponse += cbTmp;
                        bFound = TRUE;
                        break;
                    }
                }
                if (bFound)  // Did we get a match?
                {
                    continue;
                }
            }  // <macro>
        } // LMSCH
        
        // No matches, copy the character verbatim.
        *pszValue++ = *pszRegResponse++;
    } // for

    *pszValue = 0;
    if (pdwValLen)
    {
        *pdwValLen = pszValue - pszExpanded;
    }

    DEBUGMSG( ZONE_DIAL, (TEXT("UNIMODEM:-ExpandMacros : Expanded \"%s\"\r\n"), pszExpanded ));

    return TRUE;
}



































PWCHAR
CreateDialCommands(
    PTLINEDEV pLineDev,
    BOOL *fOriginate
    )
{
    DWORD  dwSize;
    PWCHAR pszTemp;
    PWCHAR pszDialPrefix;    // ex. "ATX4DT" or "ATX3DT"
    PWCHAR pszDialSuffix;    // ex. ";<cr>"
    PWCHAR pszOrigSuffix;    // ex. "<cr>"
    PWCHAR pszzDialCommands = NULL;
    PWCHAR ptchSrc, ptchDest;
    WCHAR  pszShortTemp[2];
    static const WCHAR szPrefix[] =     TEXT("Prefix");
    static const WCHAR szTerminator[] = TEXT("Terminator");
    static const WCHAR szDialPrefix[] = TEXT("DialPrefix");
    static const WCHAR szPulse[] =      TEXT("Pulse");
    static const WCHAR szTone[] =       TEXT("Tone");
    static const WCHAR *szDialType;  // szTone or szPulse
    static const WCHAR szBlindOff[] =   TEXT("Blind_Off");
    static const WCHAR szBlindOn[] =    TEXT("Blind_On");
    static const WCHAR *szBlindType;  // szBlindOn or szBlindOff

    DEBUGMSG( ZONE_DIAL, (TEXT("UNIMODEM:+CreateDialCommands - number %s\r\n"),
               pLineDev->szAddress ? pLineDev->szAddress : TEXT("") ));

     // Figure out fOriginate
    ptchSrc = pLineDev->szAddress;
    
    *fOriginate = TRUE;
    while (*ptchSrc)
    {
        if (pLineDev->chContinuation == *ptchSrc)   // usually a semicolon
        {
            *fOriginate = FALSE;
#ifdef DEBUG
            // make sure the string is correctly formed.
            if (ptchSrc[1])
            {
                DEBUGMSG(ZONE_DIAL, (TEXT("UNIMODEM: CreateDialCommands szPhoneNumber had a line continuation character not at the end.\r\n")));
            }
#endif // DEBUG
        }
        ptchSrc++;
    }

    // Trim the command continuation character off the end, now that we know this is not an origination string.
    if (!(*fOriginate))
    {
        DEBUGMSG(ZONE_DIAL, (TEXT("UNIMODEM:CreateDialCommands Non-originate string, trim trailing \"%c\"\r\n"),pLineDev->chContinuation));
#ifdef DEBUG
        if (ptchSrc[-1] != pLineDev->chContinuation)    // usually a semicolon
        {
            DEBUGMSG( ZONE_DIAL, (TEXT("UNIMODEM:CreateDialCommands made a bad assumption.\r\n")));
        }
#endif // DEBUG
        ptchSrc[-1] = 0;
    }

    // At this point, szPhoneNumber is just a string of digits to be dialed, with no semicolon at
    // the end.  Plus we know whether to originate or not.

    // make some temp space
    dwSize = ((pLineDev->dwMaxCmd + 1 +    // pszTemp
               pLineDev->dwMaxCmd + 1 +    // pszDialPrefix
               pLineDev->dwMaxCmd + 1 +    // pszDialSuffix
               pLineDev->dwMaxCmd + 1)     // pszOrigSuffix
              * SZWCHAR);

    pszTemp = (PWCHAR)TSPIAlloc( dwSize );
    if (!pszTemp)
    {
        DEBUGMSG( ZONE_DIAL, (TEXT("UNIMODEM:-CreateDialCommands : out of memory.\r\n")));
        return NULL;
    }
    DEBUGMSG(ZONE_DIAL, (TEXT("UNIMODEM:CreateDialCommands Allocated %d bytes at x%X for tmp dial strings\r\n"),
               dwSize, pszTemp));

    pszDialPrefix = pszTemp       + pLineDev->dwMaxCmd + 1;
    pszDialSuffix = pszDialPrefix + pLineDev->dwMaxCmd + 1;
    pszOrigSuffix = pszDialSuffix + pLineDev->dwMaxCmd + 1;

        // read in prefix
        dwSize = pLineDev->dwMaxCmd * SZWCHAR;
        if (ERROR_SUCCESS !=
            MdmRegGetValue( pLineDev,
                            szSettings,
                            szPrefix,
                            REG_SZ,
                            (PUCHAR)pszTemp,
                            &dwSize) )
        {
            goto Failure;
        }
        ExpandMacros(pszTemp, pszDialPrefix, NULL, NULL, 0);

        





        if ( (MDM_BLIND_DIAL & pLineDev->DevMiniCfg.dwModemOptions) ||
             (MDM_BLIND_DIAL & pLineDev->dwDialOptions) )
        {   // Turn on blind dialing
            szBlindType = szBlindOn;
            DEBUGMSG( ZONE_DIAL,
                      (TEXT("UNIMODEM:CreateDialCommands BLIND DIALING - ModemOptions 0x%X, DialOptions 0x%X\r\n"),
                       pLineDev->DevMiniCfg.dwModemOptions, pLineDev->dwDialOptions ));
        }
        else
        {   // Turn off blind dialing
            szBlindType = szBlindOff;        
        }
    
        


#ifdef BLIND_OPTION_IN_PREFIX
        // read in appropriate blind options
        dwSize = pLineDev->dwMaxCmd * SZWCHAR;
        if (ERROR_SUCCESS !=
            MdmRegGetValue( pLineDev,
                            szSettings,
                            szBlindType,
                            REG_SZ,
                            (PUCHAR)pszTemp,
                            &dwSize) )
        {
            goto Failure;
        }
        ExpandMacros(pszTemp, pszDialPrefix + wcslen(pszDialPrefix), NULL, NULL, 0);
#endif
    

    // read in dial prefix
    dwSize = pLineDev->dwMaxCmd * SZWCHAR;
    if (ERROR_SUCCESS != MdmRegGetValue( pLineDev,
                                         szSettings,
                                         szDialPrefix,
                                         REG_SZ,
                                         (PUCHAR)pszTemp,
                                         &dwSize) )
    {
        goto Failure;
    }
    
    ExpandMacros(pszTemp, pszDialPrefix + wcslen(pszDialPrefix), NULL, NULL, 0);
    
    


    dwSize = pLineDev->dwMaxCmd * SZWCHAR;
    if (MDM_TONE_DIAL & pLineDev->dwDialOptions)
    {   // We want to do tone dialing
        szDialType = szTone;
    }
    else
    {   // We want to do pulse dialing
        szDialType = szPulse;
    }
            
    if (ERROR_SUCCESS != MdmRegGetValue( pLineDev,
                                         szSettings,
                                         szDialType,
                                         REG_SZ,
                                         (PUCHAR)pszTemp,
                                         &dwSize) )
    {
        goto Failure;
    }
    
    ExpandMacros(pszTemp, pszDialPrefix + wcslen(pszDialPrefix), NULL, NULL, 0);
    
    // read in dial suffix
    dwSize = pLineDev->dwMaxCmd * SZWCHAR;
    if (ERROR_SUCCESS != MdmRegGetValue( pLineDev,
                                         szSettings,
                                         szDialSuffix,
                                         REG_SZ,
                                         (PUCHAR)pszTemp,
                                         &dwSize) )
    {
        wcscpy(pszDialSuffix, TEXT(""));
#ifdef TODO
        hPort->mi_fHaveDialSuffix = FALSE;
#endif
    }
    else
    {
#ifdef TODO
        hPort->mi_fHaveDialSuffix = TRUE;
#endif
        ExpandMacros(pszTemp, pszDialSuffix, NULL, NULL, 0);
    }

    // read in prefix terminator
    dwSize = pLineDev->dwMaxCmd * SZWCHAR;
    if (ERROR_SUCCESS == MdmRegGetValue( pLineDev,
                                         szSettings,
                                         szTerminator,
                                         REG_SZ,
                                         (PUCHAR)pszTemp,
                                         &dwSize) )
    {
        ExpandMacros(pszTemp, pszOrigSuffix, NULL, NULL, 0);
        wcscat(pszDialSuffix, pszOrigSuffix);
#ifdef DEBUG
        if (wcslen(pszOrigSuffix) > wcslen(pszDialSuffix))
        {
            DEBUGMSG( ZONE_DIAL,
                      (TEXT("UNIMODEM:CreateDialCommands OrigSuffix longer than DialSuffix!!! (contact developer)\r\n")));
        }
#endif // DEBUG
    }

#ifdef DEBUG
    if ((wcslen(pszDialPrefix) + wcslen(pszDialSuffix)) > pLineDev->dwMaxCmd)
    {
        DEBUGMSG( ZONE_DIAL,
                  (TEXT("UNIMODEM:CreateDialCommands Prefix, Terminator, BlindOn/Off, Dial Prefix, and Dial Suffix are too long!\r\n")));
    }
#endif // DEBUG

    // allocate space for the phone number lines
    {
        DWORD dwCharsAlreadyTaken = wcslen(pszDialPrefix) + wcslen(pszDialSuffix);
        DWORD dwAvailCharsPerLine = (pLineDev->dwMaxCmd - dwCharsAlreadyTaken);
        DWORD dwPhoneNumLen       = wcslen(pLineDev->szAddress);
        DWORD dwNumLines          = dwPhoneNumLen ? (dwPhoneNumLen / dwAvailCharsPerLine +
                                                     (dwPhoneNumLen % dwAvailCharsPerLine ? 1 : 0))
                                                  : 1;  // handle null string
        dwSize                    = (dwPhoneNumLen + dwNumLines * (dwCharsAlreadyTaken + 1) + 1) * SZWCHAR;

        DEBUGMSG( ZONE_DIAL,
                  (TEXT("UNIMODEM:CreateDialCommands Allocation sizes - prefix/suffix %d, Phone num %d, numLines %d\r\n"),
                   dwCharsAlreadyTaken, dwPhoneNumLen, dwNumLines));
    }

    DEBUGMSG( ZONE_DIAL,
              (TEXT("UNIMODEM:CreateDialCommands Allocate %d bytes for Dial Commands.\r\n"), dwSize));

     
    dwSize *= 4;
    
    if (!(pszzDialCommands = (PWCHAR)TSPIAlloc(dwSize)))
    {
        DEBUGMSG( ZONE_DIAL,
                  (TEXT("UNIMODEM:CreateDialCommands ran out of memory and failed a TSPIAlloc!\r\n")));
        goto Failure;
    }
    DEBUGMSG( ZONE_DIAL,
              (TEXT("UNIMODEM:CreateDialCommands Allocated %d bytes at x%X for dial string\r\n"),
               dwSize, pszzDialCommands));
    
    ptchDest = pszzDialCommands;  // point to the beginning of the commands
        
    // build dial line(s):

#ifndef BLIND_OPTION_IN_PREFIX
    // See above : we now split the blind dial out from the dial prefix.

    // read in prefix
    dwSize = pLineDev->dwMaxCmd * SZWCHAR;
    if (ERROR_SUCCESS !=
        MdmRegGetValue( pLineDev,
                        szSettings,
                        szPrefix,
                        REG_SZ,
                        (PUCHAR)pszTemp,
                        &dwSize) )
    {
        goto Failure;
    }
    ExpandMacros(pszTemp, ptchDest, NULL, NULL, 0);

    // read in appropriate blind options
    dwSize = pLineDev->dwMaxCmd * SZWCHAR;
    if (ERROR_SUCCESS !=
        MdmRegGetValue( pLineDev,
                        szSettings,
                        szBlindType,
                        REG_SZ,
                        (PUCHAR)pszTemp,
                        &dwSize) )
    {
        goto Failure;
    }
    ExpandMacros(pszTemp, ptchDest + wcslen(ptchDest), NULL, NULL, 0);
    DEBUGMSG( ZONE_DIAL,
              (TEXT("UNIMODEM:CreateDialCommands prepended Blind dial sequence = %s, len %d\r\n"),
               ptchDest, wcslen(ptchDest)));    

    //OK, be sure to add a CR to this
    ExpandMacros(CR_MACRO, ptchDest + wcslen(ptchDest), NULL, NULL, 0);
    DEBUGMSG( ZONE_DIAL,
              (TEXT("UNIMODEM:CreateDialCommands prepended Blind dial sequence = %s, len %d\r\n"),
               ptchDest, wcslen(ptchDest)));    

    // begin a new dial string
    ptchDest += wcslen(ptchDest) + 1;
#endif

    // prime the pump
    DEBUGMSG( ZONE_DIAL,
              (TEXT("UNIMODEM:CreateDialCommands Copying Dial prefix %s to x%X\r\n"),
               pszDialPrefix, ptchDest));
    wcscpy(ptchDest, pszDialPrefix);

    
    // do we have a dial suffix
    if ( /*!hPort->mi_fHaveDialSuffix*/ FALSE )
    {
        
#ifdef DEBUG
        // but, can we fit the dial string?
        if (wcslen(pszDialPrefix) + wcslen(pLineDev->szAddress) +
            wcslen(pszDialSuffix) + 1 > pLineDev->dwMaxCmd)
        {
            DEBUGMSG( ZONE_DIAL,
                      (TEXT("UNIMODEM:CreateDialCommands dial string is too long and we don't have dial suffix capability\r\n")));
        }
        // did we not want to originate?
        if (!(*fOriginate))
        {
            DEBUGMSG( ZONE_DIAL,
                      (TEXT("UNIMODEM:CreateDialCommands was told not to originate, but it has to because it doesn't have a dial suffix! (tsp screwed up!)\r\n")));
        }
#endif // DEBUG

        // build it
        DEBUGMSG( ZONE_DIAL,
                  (TEXT("UNIMODEM:CreateDialCommands Appending phone number %s\r\n"), pLineDev->szAddress));
        wcscat(ptchDest, pLineDev->szAddress);        
        DEBUGMSG( ZONE_DIAL,
                  (TEXT("UNIMODEM:CreateDialCommands Appending Dial Suffix %s\r\n"), pszDialSuffix));
        wcscat(ptchDest, pszDialSuffix);
        DEBUGMSG( ZONE_DIAL,
                  (TEXT("UNIMODEM:CreateDialCommands Final Dial String %s\n"), ptchDest));

    }
    else
    {
        // we have a dial suffix.

        // populate new pszzDialCommands with semi-colons as necessary.

        // go through and add suffixi, making sure lines don't exceed pLineDev->dwMaxCmd
        ptchSrc = pLineDev->szAddress;     // moves a character at a time.
        pszShortTemp[1] = 0;    // null terminate the 1 character temp string

         // step through the source
        while (*ptchSrc)
        {
            if (wcslen(ptchDest) + wcslen(pszDialSuffix) + 1 > pLineDev->dwMaxCmd)
            {
                // finish up this string
                DEBUGMSG( ZONE_DIAL,
                          (TEXT("UNIMODEM:CreateDialCommands Appending Dial suffix %s\r\n"), pszDialSuffix));
                wcscat(ptchDest, pszDialSuffix);

                // begin a new string
                ptchDest += wcslen(ptchDest) + 1;
                DEBUGMSG( ZONE_DIAL,
                          (TEXT("UNIMODEM:CreateDialCommands Copying Dial prefix %s for a new line at 0x%X\r\n"),
                           pszDialPrefix, ptchDest));
                wcscpy(ptchDest, pszDialPrefix);
            }
            else
            {
                // copy char
                DEBUGMSG( ZONE_DIAL,
                          (TEXT("UNIMODEM:CreateDialCommands Appending character %c to dial string %s\r\n"),
                           *ptchSrc, ptchDest));
                pszShortTemp[0] = *ptchSrc;
                wcscat(ptchDest, pszShortTemp);
                ptchSrc++;
            }
        }
        
         // conclude with the approprate Suffix.
        DEBUGMSG( ZONE_DIAL,
                  (TEXT("UNIMODEM:CreateDialCommands Appending Final Dial Suffix %s\r\n"), (*fOriginate ? pszOrigSuffix : pszDialSuffix)));
        wcscat(ptchDest, (*fOriginate ? pszOrigSuffix : pszDialSuffix));
    }

    // close keys  
Exit:
    DEBUGMSG( ZONE_DIAL, (TEXT("UNIMODEM:CreateDialCommands Trying to free pszTemp x%X\r\n"), pszTemp));
    TSPIFree(pszTemp);
    DEBUGMSG( ZONE_DIAL, (TEXT("UNIMODEM:-CreateDialCommands\r\n")));

    return pszzDialCommands;

Failure:
    if (pszzDialCommands)
    {
        DEBUGMSG( ZONE_DIAL|ZONE_ERROR, (TEXT("UNIMODEM:CreateDialCommands - Error, freeing dial string\r\n")));
        TSPIFree(pszzDialCommands);
        pszzDialCommands = NULL;
    }
    goto Exit;
}


BOOL
IsDigit(
    UCHAR ch
    )
{
    switch (ch) {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        return TRUE;
    }

    return FALSE;
}

//
// Extract the connection speed from an extended CONNECT or CARRIER modem response
// and store it in pLineDev->dwCurrentBaudRate
//
void
ParseConnectSpeed(
    PTLINEDEV pLineDev,
    PUCHAR pszResponse
    )
{
    DWORD dwSpeed = 0;
    PUCHAR psz;

    DEBUGMSG(ZONE_FUNC|ZONE_CALLS, (TEXT("UNIMODEM:+ParseConnectSpeed %a\n"), pszResponse));
    //
    // CONNECT and CARRIER typically follow the form:
    // CONNECT <speed></protocol>< protocol> <compressed>
    // CARRIER <speed></protocol> < protocol> <compressed>
    // Sometimes there is a colon and/or a space before the speed.
    // Sometimes a K follows the speed indicating * 1000
    // Sometimes there are commas in the speed
    //

    //
    // Skip to the first digit
    //
    psz = pszResponse;
    while (*psz) {
        if (IsDigit(*psz)) {
            break;
        }
        psz++;
    }

    while (*psz) {
        if (IsDigit(*psz)) {
            dwSpeed = dwSpeed * 10 + (DWORD)(*psz - '0');
        } else if (*psz == 'K') {
            dwSpeed *= 1000;
        } else if (*psz != ',') {
            break;
        }
        psz++;
    }

    if (dwSpeed) {
        pLineDev->dwCurrentBaudRate = dwSpeed;
        if (pLineDev->htCall) {
            CallLineEventProc(
                pLineDev,
                pLineDev->htCall,
                LINE_CALLINFO,
                LINECALLINFOSTATE_RATE,
                0,
                0
                );
        }
        
    }

    DEBUGMSG(ZONE_FUNC|ZONE_CALLS, (TEXT("UNIMODEM:-ParseConnectSpeed %d\n"), dwSpeed));
}


const
MODEM_RESPONSE ResponseTable[] =
{
    { "OK",             MODEM_SUCCESS },
    { "RING",           MODEM_RING },
    { "RINGING",        MODEM_RING },
    { "NO CARRIER",     MODEM_HANGUP },
    { "ERROR",          MODEM_FAILURE },
    { "NO DIALTONE",    MODEM_NODIALTONE },
    { "NO DIAL TONE",   MODEM_NODIALTONE },
    { "BUSY",           MODEM_BUSY},
    { "NO ANSWER",      MODEM_NOANSWER },
    { "CONNECT",        MODEM_CONNECT },
    { "FCERROR",        MODEM_FAILURE },
    { "FCON",           MODEM_FAILURE },
    { "BLACKLIST",      MODEM_NOANSWER },
    { "CARRIER",        MODEM_CARRIER },
    { "DATA",           MODEM_FAILURE },
    { "DATE",           MODEM_PROGRESS },
    { "DELAYED",        MODEM_FAILURE },
    { "DIALING",        MODEM_PROGRESS },
    { "FAX",            MODEM_FAILURE },
    { "VCON",           MODEM_SUCCESS },
    { "VOICE",          MODEM_FAILURE },
    { "PROTOCOL",       MODEM_PROTOCOL },
    { "RRING",          MODEM_RING},
};

#ifdef DEBUG
LPWSTR
ResponseName(
    MODEMRESPCODES MdmRsp
    )
{
    switch (MdmRsp) {
    case MODEM_SUCCESS:     return TEXT("MODEM_SUCCESS");
    case MODEM_PENDING:     return TEXT("MODEM_PENDING");
    case MODEM_CONNECT:     return TEXT("MODEM_CONNECT");
    case MODEM_FAILURE:     return TEXT("MODEM_FAILURE");
    case MODEM_HANGUP:      return TEXT("MODEM_HANGUP");
    case MODEM_NODIALTONE:  return TEXT("MODEM_NODIALTONE");
    case MODEM_BUSY:        return TEXT("MODEM_BUSY");
    case MODEM_NOANSWER:    return TEXT("MODEM_NOANSWER");
    case MODEM_RING:        return TEXT("MODEM_RING");
    case MODEM_CARRIER:     return TEXT("MODEM_CARRIER");
    case MODEM_PROTOCOL:    return TEXT("MODEM_PROTOCOL");
    case MODEM_PROGRESS:    return TEXT("MODEM_PROGRESS");
    case MODEM_UNKNOWN:     return TEXT("MODEM_UNKNOWN");
    case MODEM_IGNORE:      return TEXT("MODEM_IGNORE");
    case MODEM_EXIT:        return TEXT("MODEM_EXIT");
    default:                return TEXT("UNKNOWN Modem Response Code!!!");
    }
}
#endif // DEBUG

// The only valid NULL_MODEM response is "CLIENTSERVER"
const UCHAR pchDCCResp[] = "CLIENTSERVER";
const UCHAR pchDCCCmd[] = "CLIENT";
        
#define SZ_RESPONSE_TABLE (sizeof(ResponseTable)/sizeof(MODEM_RESPONSE))

MODEMRESPCODES
ModemResponseHandler(
    PTLINEDEV pLineDev,
    PUCHAR pszResponse,
    PUCHAR pszOrigCommand
    )
{
    UINT16 wRespCode = MODEM_UNKNOWN;
    int i;
    
    DEBUGMSG(ZONE_FUNC,
             (TEXT("UNIMODEM:+ModemResponseHandler x%X, x%X, x%X\r\n"),
              pLineDev, pszResponse, pszOrigCommand));

    // Lets remove any leading CR/LF from response.
    while( (CR == *pszResponse) || (LF == *pszResponse) )
        pszResponse++;
    
    if( 0 == *pszResponse )
    {
        DEBUGMSG(ZONE_FUNC,
                 (TEXT("UNIMODEM:-ModemResponseHandler, Empty response\r\n")));
        return MODEM_IGNORE;
    }

    DEBUGMSG(ZONE_CALLS,
             (TEXT("UNIMODEM:ModemResponseHandler Modem Response '%a'\r\n"), pszResponse));

    // Lets remove any leading CR/LF from orig command.
    while( (CR == *pszOrigCommand) || (LF == *pszOrigCommand) )
        pszOrigCommand++;
    
    // First, check to see if this is just an echo of the command
    if( NULL != pszOrigCommand )
    {
        DEBUGMSG(ZONE_FUNC,
                 (TEXT("UNIMODEM:ModemResponseHandler Checking for echo of CMD %a\r\n"), pszOrigCommand));
        
        if( ! strncmpi( pszResponse, pszOrigCommand, strlen(pszResponse) ) )
        {
            // Ignore this, it was just a duplicate
            DEBUGMSG(ZONE_FUNC,
                     (TEXT("UNIMODEM:-ModemResponseHandler, Command Echo\r\n")));
            return MODEM_IGNORE; 
        }
    }
    
    DEBUGMSG(ZONE_FUNC,
             (TEXT("UNIMODEM:ModemResponseHandler response was not an echo\r\n")));
        
    
    // Regular Modems have a lookup table for responses
        
    // Lets convert the ASCII response to a numeric code
    for( i=0; i<SZ_RESPONSE_TABLE; i++ )
    {
        if( !strncmpi( pszResponse, ResponseTable[i].pszResponse,
                       (strlen(ResponseTable[i].pszResponse) - 1) ) )
        {
            
            wRespCode = ResponseTable[i].ModemRespCode;
            break;
        }
    }
    DEBUGMSG(ZONE_CALLS|ZONE_FUNC,
             (TEXT("UNIMODEM:-ModemResponseHandler, %s\n"), ResponseName(wRespCode) ));

    return wRespCode;
}


//
// Expand macros and convert command to chars
//
LPSTR
MdmConvertCommand(
    WCHAR const *pszWCommand,
    LPSTR pchCmd,
    LPDWORD lpdwSize
    )
{
    LPSTR lpstr;
    WCHAR szExpand[256];
    DWORD dwCmdLen;

    if (pchCmd) {
        lpstr = pchCmd;
    } else {
        lpstr = NULL;
    }

    if (!ExpandMacros((PWCHAR)pszWCommand, (PWCHAR)szExpand, 0, NULL, 0)) {
        return NULL;
    }

    dwCmdLen = wcslen(szExpand) + 1;

    if (lpstr == NULL) {
        lpstr = TSPIAlloc(dwCmdLen);
        if (lpstr == NULL) {
            return NULL;
        }
    }
    dwCmdLen = wcstombs( lpstr, szExpand, dwCmdLen );

    if (lpdwSize) {
        *lpdwSize = dwCmdLen;
    }

    return lpstr;
}
    

BOOL
MdmSendCommand(
    PTLINEDEV pLineDev,
    UCHAR const *pszCommand
    )
{
    DWORD   dwLen;
    BOOL bRet = FALSE;
    
    //
    // This routine sends a Modem command, and then waits for
    // a valid response from the modem (or timeout).
    //

    try
    {
        if(  (HANDLE)INVALID_DEVICE != pLineDev->hDevice )
        {    
            DEBUGMSG(ZONE_CALLS, (TEXT("UNIMODEM:MdmSendCommand '%a'\r\n"), pszCommand));
            // Purge any old responses sitting in serial buffer
            if (PurgeComm( pLineDev->hDevice, PURGE_RXCLEAR|PURGE_TXCLEAR )) {
                // Set the Comm Mask for waitcommevent
                if (SetCommMask(pLineDev->hDevice, EV_DEFAULT)) {
                    // Now, lets send this nice new string to the modem
                    bRet = WriteFile(pLineDev->hDevice, (LPVOID)pszCommand, strlen((LPVOID)pszCommand), &dwLen, 0 );
                    if ((FALSE == bRet) || (dwLen != strlen((LPVOID)pszCommand))) {
                        bRet = FALSE;
                        RETAILMSG (1,
                                   (TEXT("UNIMODEM:!!!MdmSendCommand wrote %d of %d byte dial string\r\n"),
                                    dwLen, strlen((LPVOID)pszCommand)) );
                                           
                        DEBUGMSG(ZONE_MISC,
                                 (TEXT("UNIMODEM:MdmSendCommand Wrote %d bytes of %d byte dial string\r\n"),
                                  dwLen, strlen((LPVOID)pszCommand)) );
                    }
                }
            }
        }
    }
    except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
            EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        // Just return of handle is bad
    }

    WriteModemLog(pLineDev, (PUCHAR)pszCommand, bRet ? MDMLOG_COMMAND_OK : MDMLOG_COMMAND_FAIL);
    return bRet;
}


#ifdef DEBUG
void
DisplayModemStatus(
    DWORD ModemStat
    )
{
    WCHAR szModemStat[256];

    szModemStat[0] = 0;

    if( ModemStat & EV_BREAK)
        wcscat(szModemStat, TEXT(" EV_BREAK"));
    if( ModemStat & EV_CTS)
        wcscat(szModemStat, TEXT(" EV_CTS"));
    if( ModemStat & EV_DSR)
        wcscat(szModemStat, TEXT(" EV_DSR"));
    if( ModemStat & EV_ERR)
        wcscat(szModemStat, TEXT(" EV_ERR"));
    if( ModemStat & EV_RING)
        wcscat(szModemStat, TEXT(" EV_RING"));
    if( ModemStat & EV_RLSD)
        wcscat(szModemStat, TEXT(" EV_RLSD"));
    if( ModemStat & EV_RXCHAR)
        wcscat(szModemStat, TEXT(" EV_RXCHAR"));
    if( ModemStat & EV_RXFLAG)
        wcscat(szModemStat, TEXT(" EV_RXFLAG"));
    if( ModemStat & EV_TXEMPTY)
        wcscat(szModemStat, TEXT(" EV_TXEMPTY"));

    DEBUGMSG (ZONE_MISC, (TEXT("UNIMODEM:DisplayModemStatus ModemStat =%s\n"), szModemStat));
}   // DisplayModemStatus

void
DisplayCommError(
    DWORD dwCommError
    )
{
    WCHAR szCommError[256];

    szCommError[0] = 0;

    if (dwCommError & CE_BREAK) {
        wcscat(szCommError, TEXT(" CE_BREAK"));
    }
    if (dwCommError & CE_FRAME) {
        wcscat(szCommError, TEXT(" CE_FRAME"));
    }
    if (dwCommError & CE_IOE) {
        wcscat(szCommError, TEXT(" CE_IOE"));
    }
    if (dwCommError & CE_MODE) {
        wcscat(szCommError, TEXT(" CE_MODE"));
    }
    if (dwCommError & CE_OOP) {
        wcscat(szCommError, TEXT(" CE_OOP"));
    }
    if (dwCommError & CE_OVERRUN) {
        wcscat(szCommError, TEXT(" CE_OVERRUN"));
    }
    if (dwCommError & CE_RXOVER) {
        wcscat(szCommError, TEXT(" CE_RXOVER"));
    }
    if (dwCommError & CE_RXPARITY) {
        wcscat(szCommError, TEXT(" CE_RXPARITY"));
    }
    if (dwCommError & CE_TXFULL) {
        wcscat(szCommError, TEXT(" CE_TXFULL"));
    }
    if (dwCommError & CE_DNS) {
        wcscat(szCommError, TEXT(" CE_DNS"));
    }
    if (dwCommError & CE_PTO) {
        wcscat(szCommError, TEXT(" CE_PTO"));
    }

    DEBUGMSG (ZONE_MISC, (TEXT("UNIMODEM:DisplayCommError CommError=%s\n"), szCommError));
}   // DisplayCommError
#endif


BOOL
LineNotDropped(
    PTLINEDEV pLineDev,
    DEVSTATES OrigDevState
    )
{
    if (pLineDev->DevState == OrigDevState) {
        return TRUE;
    }
    return FALSE;
}


MODEMRESPCODES
MdmGetResponse(
    PTLINEDEV pLineDev,
    PUCHAR    pszCommand,
    BOOL      bLeaveCS
    )
{
    MODEMRESPCODES ModemResp = MODEM_IGNORE;
    DWORD   ModemStat;
    UCHAR   InBuf[MAXSTRINGLENGTH];
    int     i;
    UCHAR   ModemResponse[MAXSTRINGLENGTH];
    UINT16  ModemResponseIndex = 0;
    DWORD   dwLen;
    DEVSTATES OrigDevState;
    PUCHAR  pchDCC; // either pchDCCCmd or pchDCCResp
    MODEMRESPCODES DCCResp;
    DWORD dwDCCLen;
    BOOL bInCS;
    DWORD OrigPendingOp;
    
    DEBUGMSG(ZONE_FUNC, (TEXT("UNIMODEM:+MdmGetResponse for %a\r\n"), pszCommand));

    OrigDevState = pLineDev->DevState;
    OrigPendingOp = pLineDev->dwPendingType;
    bInCS = bLeaveCS;

    // Wait until we get some type of meaningful response from the modem
    while ((MODEM_IGNORE == ModemResp) && LineNotDropped(pLineDev, OrigDevState))
    {
        try
        {
            ModemStat = 0;

            if (bInCS) {
                //
                // To help avoid missing signals from SignalControlThread, stay in CS as long
                // as possible before calling WaitCommEvent.
                //
                LeaveCriticalSection(&pLineDev->OpenCS);
                bInCS = FALSE;
            }
            if (!WaitCommEvent( pLineDev->hDevice, &ModemStat, NULL)) {
                dwLen = GetLastError();
                DEBUGMSG(ZONE_CALLS, (TEXT("UNIMODEM:MdmGetResponse - WaitCommEvent failed %d\n"), dwLen));
                ModemResp = MODEM_EXIT;
                goto mgr_exit;
            }

            if (pLineDev->bWatchdogTimedOut) {
                pLineDev->bWatchdogTimedOut = FALSE;
                DEBUGMSG(ZONE_CALLS, (TEXT("UNIMODEM:MdmGetResponse - Watchdog timed out\n")));
                ModemResp = MODEM_EXIT;
                goto mgr_exit;
            }

            //
            // Look for signals from SignalControlThread
            //
            if (pLineDev->dwPendingType != OrigPendingOp) {
                DEBUGMSG(ZONE_CALLS, (TEXT("UNIMODEM:MdmGetResponse - Interrupted\n")));
                ModemResp = MODEM_FAILURE;
                goto mgr_exit;
            }

            if (ModemStat) {
#ifdef DEBUG
                DisplayModemStatus(ModemStat);
#endif
                if (ModemStat & EV_ERR) {
                    if (ClearCommError(pLineDev->hDevice, &dwLen, NULL)) {
#ifdef DEBUG
                        if (dwLen) {
                            DisplayCommError(dwLen);
                        }
                    } else {
                        dwLen = GetLastError();
                        DEBUGMSG(ZONE_CALLS, (TEXT("UNIMODEM:MdmGetResponse ClearCommError failed %d\n"), dwLen));
#endif
                    }
                }

                if( ModemStat & EV_RING) {
                    ModemResp = MODEM_RING;
                }

                if ((ModemStat & EV_RXCHAR)  && LineNotDropped(pLineDev, OrigDevState))
                {
                    if (!ReadFile (pLineDev->hDevice, (LPVOID)InBuf, sizeof(InBuf), &dwLen, 0)) {
                        DEBUGMSG(ZONE_CALLS, (TEXT("UNIMODEM:MdmGetResponse - ReadFile failed %d\n"), GetLastError()));
                        ModemResp = MODEM_EXIT;
                        goto mgr_exit;
                    }

                    InBuf[dwLen] = 0;
                    WriteModemLog(pLineDev, (PUCHAR)InBuf, MDMLOG_RESPONSE);
#ifdef DEBUG_RESPONSES
                    if (dwLen) {
                        DEBUGMSG(ZONE_CALLS, (TEXT("UNIMODEM:MdmGetResponse Response = %a\n"), InBuf));
                    }
#endif
                    // Now lets process each response as they arrive
                    for( i=0; i<(int)dwLen; i++ )
                    {
                        ModemResponse[ModemResponseIndex] = InBuf[i];

                        if(  IS_NULL_MODEM(pLineDev) )
                        {
                            //
                            // Only allow one response per call to MdmGetResponse for a DCC connection
                            //
                            ModemResp = MODEM_FAILURE;

                            //
                            // Are we client or server?
                            //
                            if (pLineDev->dwPendingType == PENDING_LINEMAKECALL) {
                                pchDCC = (PUCHAR)pchDCCResp;
                                DCCResp = MODEM_CONNECT;
                            } else {
                                pchDCC = (PUCHAR)pchDCCCmd;
                                DCCResp = MODEM_RING;
                            }
                            dwDCCLen = strlen(pchDCC);

                            if( ModemResponse[ModemResponseIndex] == pchDCC[ModemResponseIndex] )
                            {
                                ModemResponseIndex++;

                                // characters match so far, see if we have a complete response.
                                if( ModemResponseIndex >= dwDCCLen )
                                {
                                    // OK, we got a full response.  Zero terminate it and exit
                                    ModemResponse[ModemResponseIndex] = 0;
                                    // No need to call response handler, we already did all the work.
                                    ModemResp = DCCResp;
                                    DEBUGMSG (ZONE_CALLS, (TEXT("UNIMODEM:MdmGetResponse Null Response detected\r\n")));
                                    break;
                                }
                                
                            }
                            else // character mismatch - restart scan sequence.
                            {
                                // If character is 'C', it might be first character of the actual
                                // response.  Any other case means that all data up to here is useless
                                // so we should toss it.  This is only true because 'C' appears only as
                                // the first character of a response, and never after that.
                                ModemResponseIndex = 0;
                                if (pchDCC[0] == InBuf[i])
                                {
                                    ModemResponse[ModemResponseIndex] = InBuf[i];
                                    ModemResponseIndex++;
                                }
                            }
                        }
                        else
                        {
                            if( (CR == InBuf[i]) ||
                                (LF == InBuf[i]) ||
                                ((DWORD)i == dwLen-1)   ||
                                ((pLineDev->dwMaxCmd - 1) <= ModemResponseIndex) ) 
                            {
                                // have a complete response (or max length).  Now we parse it.
                                ModemResponse[ModemResponseIndex] = 0;  // Lets Zero Terminate the string
                                ModemResp = ModemResponseHandler( pLineDev, ModemResponse, pszCommand );
                                
                                


                                switch (ModemResp) {
                                case MODEM_CARRIER:
                                    ParseConnectSpeed(pLineDev, ModemResponse);
                                    // fall through!!!

                                case MODEM_PROTOCOL:
                                case MODEM_PROGRESS:
                                    NewCallState(pLineDev, LINECALLSTATE_PROCEEDING, 0L);
                                    if (i < (int)(dwLen - 1)) {
                                        ModemResponseIndex = 0;  // Start a new response
                                        ModemResp = MODEM_IGNORE;
                                    }
                                    break;

                                case MODEM_UNKNOWN:
                                case MODEM_IGNORE:
                                    ModemResponseIndex = 0;  // Start a new response
                                    ModemResp = MODEM_IGNORE;
                                    break;

                                case MODEM_CONNECT:
                                    ParseConnectSpeed(pLineDev, ModemResponse);
                                    // fall through

                                default:
                                    goto mgr_exit;
                                }

                                ModemResponseIndex = 0;  // Start a new response
                            }
                            else
                            {
                                ModemResponseIndex++;  //Add another character to the string
                            }
                        }
                    }
                
                }
            }
            else
            {
                // A zero ModemStat is an error, and indicates that the
                // waitcommevent was stopped by our watchdog or linedrop.
                ModemResp = MODEM_FAILURE;
                DEBUGMSG (ZONE_CALLS, (TEXT("UNIMODEM:MdmGetResponse - Timeout\r\n")));
                goto mgr_exit;
            }
        
            DEBUGMSG(ZONE_FUNC|ZONE_CALLS, (TEXT("UNIMODEM:MdmGetResponse %s\n"), ResponseName(ModemResp)));
        }
        except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
                EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
            // Must have been a bad handle.  Give up
            ModemResp = MODEM_FAILURE;
            DEBUGMSG (ZONE_FUNC|ZONE_CALLS,
                      (TEXT("UNIMODEM:MdmGetResponse - Exception.  Handle \r\n"),
                        pLineDev->hDevice ));
        }
    }
    
mgr_exit:    
    if (bInCS) {
        LeaveCriticalSection(&pLineDev->OpenCS);
    }
    DEBUGMSG(ZONE_FUNC, (TEXT("UNIMODEM:-MdmGetResponse %s\r\n"), ResponseName(ModemResp)));
    return ModemResp;
}


//
// There were cases where the ModemWatchdog thread would not make it to the WaitForSingleObject
// before the next SetWatchdog happened. This would leave two or more watchdogs runnning at the
// same time which leads to all kinds of problems. Now SetWatchdog passes a UNIMODEM_WATCHDOG_INFO
// structure to the thread and the watchdog checks that it is the current one before aborting
// any pending operations.
//
typedef struct _UNIMODEM_WATCHDOG_INFO {
    PTLINEDEV pLineDev;
    DWORD dwTimeout;
    DWORD dwThisWatchdog;
} UNIMODEM_WATCHDOG_INFO, * PUNIMODEM_WATCHDOG_INFO;


// **********************************************************************
//
// This is the dialer/answer watchdog thread.  It is created by the dialer thread.
// Its purpose is to wait for a specified interval, or until signalled by
// the dialer.  If the wait times out, then the watchdog calls into the
// COM port driver, causing the waitcommevent in the dialer thread to
// return.
// 
// **********************************************************************
void
ModemWatchdog(
    PUNIMODEM_WATCHDOG_INFO pWDInfo
    )
{
    DWORD RetCode;
    BOOL bAbort;
    PTLINEDEV pLineDev;
    DWORD dwTimeout;
    DWORD dwThisWatchdog;

    pLineDev        = pWDInfo->pLineDev;
    dwTimeout       = pWDInfo->dwTimeout;
    dwThisWatchdog  = pWDInfo->dwThisWatchdog;
    TSPIFree(pWDInfo);

    DEBUGMSG(ZONE_CALLS, (TEXT("UNIMODEM:+ModemWatchdog(%d) - timeout set to %d seconds\r\n"),
        dwThisWatchdog, dwTimeout/1000));
    
    
    // Wait for the specified interval.
    RetCode = WaitForSingleObject( pLineDev->hTimeoutEvent, dwTimeout );

    if ((dwThisWatchdog == pLineDev->dwCurWatchdog) && ( WAIT_TIMEOUT == RetCode )) {
        pLineDev->bWatchdogTimedOut = TRUE;

        DEBUGMSG(ZONE_CALLS, (TEXT("UNIMODEM:ModemWatchdog(%d)  *** %s TIMEOUT!\n"),
            dwThisWatchdog, GetPendingName(pLineDev->dwPendingType)));

        // We need to stop the waitcommevent.  Note that we set the
        // mask to 0, which will cause any subsequent waitcommevent
        // calls to also return immediately.
        ToggleCommMask(pLineDev);

        switch (pLineDev->dwPendingType) {
        case PENDING_LINEMAKECALL:
            //
            // Allow an in progress DCC lineMakeCall to retry
            //
            bAbort = !(IS_NULL_MODEM(pLineDev));
            break;

        case PENDING_LINEANSWER:
            NewCallState(pLineDev, LINECALLSTATE_DISCONNECTED, LINEDISCONNECTMODE_UNKNOWN);
            NewCallState(pLineDev, LINECALLSTATE_IDLE, 0L);
            bAbort = TRUE;
            break;

        case PENDING_LINEDIAL:
        case PENDING_LINEDROP:
            bAbort = TRUE;
            break;

        default:
            bAbort = FALSE;
            break;
        }

        // We set an event when we are done so that DevLineDrop can
        // continue with his cleanup.
        if (bAbort) {
            SetEvent(pLineDev->hCallComplete);
            SetAsyncStatus(pLineDev, LINEERR_OPERATIONFAILED);
        }
    }
#ifdef DEBUG
    else {
        DEBUGMSG(ZONE_CALLS, (TEXT("UNIMODEM:ModemWatchdog(%d) cancelled\r\n"), dwThisWatchdog));
    }
#endif

    // This is a one shot timeout.  The thread should go away as
    // soon as the timer expires or is signalled/cancelled.
}

// **********************************************************************
// Wrapper function to start the watchdog.
// Note that a timeout of 0 cancels the current watchdog.
// **********************************************************************
DWORD
SetWatchdog(
    PTLINEDEV pLineDev,
    DWORD dwTimeout
    )
{
    HANDLE hThread;
    PUNIMODEM_WATCHDOG_INFO pWDInfo;

    pLineDev->dwCurWatchdog++;
    pLineDev->bWatchdogTimedOut = FALSE;

    // First, lets cancel any current outstanding watchdogs for this line
    PulseEvent(pLineDev->hTimeoutEvent);  // Stop the watchdog

    // And now, start a new watchdog if they want us to 
    if( dwTimeout ) {
        pWDInfo = TSPIAlloc(sizeof(UNIMODEM_WATCHDOG_INFO));
        if (NULL == pWDInfo) {
            NKDbgPrintfW( TEXT("UNIMODEM:SetWatchDog Unable to alloc Watchdog info\n") );
            return (DWORD)-1;
        }

        pWDInfo->pLineDev       = pLineDev;
        pWDInfo->dwTimeout      = dwTimeout;
        pWDInfo->dwThisWatchdog = pLineDev->dwCurWatchdog;

        hThread = CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE)ModemWatchdog,
                                pWDInfo, 0, NULL );
        if ( ! hThread ) {
            TSPIFree(pWDInfo);
            NKDbgPrintfW( TEXT("UNIMODEM:SetWatchDog Unable to Create Watchdog Thread\n") );
            return (DWORD)-1;
        }
        CeSetThreadPriority(hThread, g_dwUnimodemThreadPriority);   
        CloseHandle( hThread );
    }
    return STATUS_SUCCESS;
}


BOOL
ModemInit(
    PTLINEDEV pLineDev
    )
{
    MODEMRESPCODES MdmResp = MODEM_FAILURE;
    DWORD dwSize, dwCmdLen;
    int   RetryCount = 0;
    PWCHAR pszTemp = NULL;
    PWCHAR pszExpand = NULL;
    PCHAR pchCmd = NULL;
    MODEMMACRO ModemMacro;
    DWORD dwCallSetupFailTimer;
    DEVSTATES OrigDevState;

    static const WCHAR szReset[] =   TEXT("Reset");
    static const WCHAR szFlowHard[] =  TEXT("FlowHard");
    static const WCHAR szFlowSoft[] =  TEXT("FlowSoft");
    static const WCHAR szFlowNone[] =  TEXT("FlowOff");
    static const WCHAR *szFlowType;  // Hard, Soft, None
    static const WCHAR szCallFail[] =   TEXT("CallSetupFailTimeout");

    static const UCHAR szEscapeCmd[] = "+++";
    

    OrigDevState = pLineDev->DevState;

    try
    {
        DEBUGMSG(ZONE_FUNC, (TEXT("UNIMODEM:+ModemInit\r\n")));

        // Allocate temp buffers for registry results
        pszTemp = (PWCHAR)TSPIAlloc( (pLineDev->dwMaxCmd + 1) * SZWCHAR );
        pszExpand = (PWCHAR)TSPIAlloc( (pLineDev->dwMaxCmd + 1) * SZWCHAR );
        pchCmd =  (PCHAR)TSPIAlloc( pLineDev->dwMaxCmd + 1 );
    
        // First, read the reset command from the registry
        dwSize = pLineDev->dwMaxCmd * SZWCHAR;
        if (ERROR_SUCCESS !=
            MdmRegGetValue( pLineDev,
                            szSettings,
                            szReset,
                            REG_SZ,
                            (PUCHAR)pszTemp,
                            &dwSize) )
        {
            goto init_error;
        }

        MdmConvertCommand(pszTemp, pchCmd, &dwCmdLen);
        DEBUGMSG(ZONE_CALLS, (TEXT("UNIMODEM:ModemInit Reset string is \"%s\" (%d)\r\n"),
                              pszTemp, dwCmdLen));
    
        // ***** reset modem by clearing DTR    
        if(LineNotDropped(pLineDev, OrigDevState))
        {
            DEBUGMSG(ZONE_CALLS, (TEXT("UNIMODEM:ModemInit SetCommState - DTR off\r\n")));
            EscapeCommFunction ( pLineDev->hDevice, CLRDTR);
            Sleep( 400 );
        }
    
    
        if(LineNotDropped(pLineDev, OrigDevState))
        {
            DEBUGMSG(ZONE_CALLS, (TEXT("UNIMODEM:ModemInit SetCommState - DTR on\r\n")));
            EscapeCommFunction ( pLineDev->hDevice, SETDTR);
            Sleep( 200 );
        }
    
        // No matter what the state was, we are now initializing
        pLineDev->MdmState = MDMST_INITIALIZING;
    
        while( (LineNotDropped(pLineDev, OrigDevState)) &&
                 (RetryCount < MAX_COMMAND_TRIES) )
        {
            // Set short timeout for init strings
            if( SetWatchdog( pLineDev, 8000 ) )
            {
                pLineDev->MdmState = MDMST_UNKNOWN;
                goto init_error;
            }
        
            if( LineNotDropped(pLineDev, OrigDevState) )
            {
                DEBUGMSG(ZONE_CALLS, (TEXT("UNIMODEM:ModemInit Send Init\r\n")));
                MdmResp = MODEM_FAILURE;
                if (MdmSendCommand( pLineDev, pchCmd )) {
                    MdmResp = MdmGetResponse( pLineDev, pchCmd, NOCS );
                }
            }
        
            // See if we managed to get initialized
            if( MODEM_SUCCESS == MdmResp )
            {
                pLineDev->MdmState = MDMST_DISCONNECTED;  // It worked
                break;
            }
            else
            {
                if( LineNotDropped(pLineDev, OrigDevState) )
                {
                    // the modem didn't reset
                    pLineDev->MdmState = MDMST_UNKNOWN;

                    // If szReset didn't work, we likely need a +++
                    MdmSendCommand( pLineDev, szEscapeCmd );
                    Sleep( 200 );
                }
            }
            RetryCount++;
        }

        if( (MDMST_DISCONNECTED == pLineDev->MdmState) &&
            (LineNotDropped(pLineDev, OrigDevState)) )
        {
            DWORD mdmRegRslt = ERROR_SUCCESS;
            WORD wInitKey;

            // read a sequence of additional init strings from the
            // registry and send them down now. Can't enumerate the 
            // keys since that wouldn't allow us to have modem specific
			// values mixed with default values.
            wInitKey = 0;
            while( (wInitKey < MAXINITKEYS) &&
                     (LineNotDropped(pLineDev, OrigDevState)) &&
                     (ERROR_SUCCESS == mdmRegRslt) )
            {
                dwSize = pLineDev->dwMaxCmd * SZWCHAR;
                mdmRegRslt = MdmRegGetValue( pLineDev,
                                             szInit,
                                             szInitNum[wInitKey],
                                             REG_SZ,
                                             (PUCHAR)pszTemp,
                                             &dwSize);

                if (ERROR_SUCCESS == mdmRegRslt)
                {
                    MdmConvertCommand(pszTemp, pchCmd, &dwCmdLen);
                    DEBUGMSG(ZONE_CALLS, (TEXT("UNIMODEM:ModemInit Init string %d - %s\r\n"),
                                          wInitKey, pszTemp));

                    // Set short timeout for init strings
                    if( SetWatchdog( pLineDev, 8000 ) )
                    {
                        pLineDev->MdmState = MDMST_UNKNOWN;
                        goto init_error;
                    }
                
                    if (LineNotDropped(pLineDev, OrigDevState)) {
                        // We got a value, write it out to modem
                        MdmResp = MODEM_FAILURE;
                        if (MdmSendCommand( pLineDev, pchCmd )) {
                            if (LineNotDropped(pLineDev, OrigDevState)) {
                                MdmResp = MdmGetResponse( pLineDev, pchCmd, NOCS );
                            }
                        }
                    }
                    if( MODEM_SUCCESS != MdmResp )
                    {
                        // bad init string.  We can't recover from this so exit
                        pLineDev->MdmState = MDMST_UNKNOWN;
                        goto init_error;
                    }
                }
                else
                {
                    DEBUGMSG(ZONE_ERROR, (TEXT("UNIMODEM:ModemInit Error x%X reading Init string %d\r\n"),
                                          mdmRegRslt, wInitKey));
                }
            
                wInitKey++;
            }

            // And now that we have the basic init in place, we still need to
            // make sure the flow control settings match what we are doing in
            // the UART.
            if( MDM_FLOWCONTROL_HARD & pLineDev->DevMiniCfg.dwModemOptions )
            {  // Set Modem for hardware flow control
                szFlowType = szFlowHard;
            }
            else  if( MDM_FLOWCONTROL_SOFT & pLineDev->DevMiniCfg.dwModemOptions )
            {  // Set Modem for software flow control
                szFlowType = szFlowSoft;            
            }
            else
            {  // Set modem for no flow control
                szFlowType = szFlowNone;            
            }
            dwSize = pLineDev->dwMaxCmd * SZWCHAR;
            if( ERROR_SUCCESS == MdmRegGetValue( pLineDev,
                                                 szSettings,
                                                 szFlowType,
                                                 REG_SZ,
                                                 (PUCHAR)pszTemp,
                                                 &dwSize) )
            {
                // OK, we got the flow control command, send it.
                MdmConvertCommand(pszTemp, pchCmd, &dwCmdLen);
                DEBUGMSG(ZONE_CALLS, (TEXT("UNIMODEM:ModemInit Flow Control string %s\r\n"),
                                      pszTemp));
            
                if (LineNotDropped(pLineDev, OrigDevState))
                {
                    // We got a value, write it out to modem
                    MdmResp = MODEM_FAILURE;
                    if (MdmSendCommand( pLineDev, pchCmd )) {
                        if (LineNotDropped(pLineDev, OrigDevState)) {
                            MdmResp = MdmGetResponse( pLineDev, pchCmd, NOCS );
                        }
                    }
                }
                if( MODEM_SUCCESS != MdmResp )
                {
                    // bad flow control string.  We can't recover from this.
                    pLineDev->MdmState = MDMST_UNKNOWN;
                    goto init_error;
                }
            }

            // Now we set up the call fail timer if it exists.
            dwSize = pLineDev->dwMaxCmd * SZWCHAR;
            if( ERROR_SUCCESS == MdmRegGetValue( pLineDev,
                                                 szSettings,
                                                 szCallFail,
                                                 REG_SZ,
                                                 (PUCHAR)pszTemp,
                                                 &dwSize) )
            {
                // OK, we got the Call Fail setup command, send it.

                // Most modems can't handle a value > 255 for call fail time
                // And a time of 0 usually really means wait the maximum
                dwCallSetupFailTimer = pLineDev->DevMiniCfg.dwCallSetupFailTimer;
                if( (dwCallSetupFailTimer > 255) ||
                    (dwCallSetupFailTimer == 0) )
                    dwCallSetupFailTimer = 255;
                
                // Expand macros, including the fail time <#>
                wcscpy(ModemMacro.MacroName, INTEGER_MACRO);
                wsprintf(ModemMacro.MacroValue, TEXT("%d"), dwCallSetupFailTimer );
                ExpandMacros(pszTemp, pszExpand, NULL, &ModemMacro, 1);
            
                // We need to convert the UniCode to ASCII
                dwCmdLen = wcslen(pszExpand) + 1;
                dwCmdLen = wcstombs( pchCmd, pszExpand, dwCmdLen );
                DEBUGMSG(ZONE_CALLS, (TEXT("UNIMODEM:ModemInit Call fail timer string %s\r\n"),
                                      pszTemp));
            
                if (LineNotDropped(pLineDev, OrigDevState))
                {
                    // We got a value, write it out to modem
                    MdmResp = MODEM_FAILURE;
                    if (MdmSendCommand( pLineDev, pchCmd )) {
                        if (LineNotDropped(pLineDev, OrigDevState)) {
                            MdmResp = MdmGetResponse( pLineDev, pchCmd, NOCS );
                        }
                    }
                }
                if( MODEM_SUCCESS != MdmResp )
                {
                    // Hmm, we could probably ignore this failure, but the call might
                    // never time out.  Lets be safe and abort now.
                    pLineDev->MdmState = MDMST_UNKNOWN;
                    goto init_error;
                }
            }

            // And finally, see if user requested any special modifier string
            if( dwCmdLen = wcslen(pLineDev->DevMiniCfg.szDialModifier) )
            {
                DEBUGMSG(ZONE_CALLS, (TEXT("UNIMODEM:ModemInit Dial Modifier string %s\r\n"),
                                      pLineDev->DevMiniCfg.szDialModifier));

                // Convert the UniCode to ASCII, and insert 'AT' in command
                pchCmd[0] = 'A';
                pchCmd[1] = 'T';
                
                dwCmdLen += 1;
                dwCmdLen = wcstombs( pchCmd+2, pLineDev->DevMiniCfg.szDialModifier, dwCmdLen );

                // OK, now don't forget the CR at end.
                strcat( pchCmd, "\r" );
                DEBUGMSG(ZONE_CALLS, (TEXT("UNIMODEM:ModemInit Generated Modifier string %a\r\n"),
                                      pchCmd));

                if (LineNotDropped(pLineDev, OrigDevState))
                {
                    // And write this out to modem
                    MdmResp = MODEM_FAILURE;
                    if (MdmSendCommand( pLineDev, pchCmd )) {
                        if (LineNotDropped(pLineDev, OrigDevState)) {
                            MdmResp = MdmGetResponse( pLineDev, pchCmd, NOCS );
                        }
                    }
                }
                if( MODEM_SUCCESS != MdmResp )
                {
                    // bad modifier string.  We can't recover from this.
                    pLineDev->MdmState = MDMST_UNKNOWN;
                    DEBUGMSG(ZONE_CALLS | ZONE_ERROR,
                             (TEXT("UNIMODEM:ModemInit Bad user supplied dial modifier %s\r\n"),
                              pLineDev->DevMiniCfg.szDialModifier));
                    goto init_error;
                }
            }
            
        }
    }
    except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
            EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        // Most likely a bad handle.  Just give up
        pLineDev->MdmState = MDMST_UNKNOWN;
    }
    
init_error:
    // Cancel watchdog if he's still around.
    SetWatchdog( pLineDev, 0 );
    if( pszTemp )
        TSPIFree( pszTemp );        
    if( pchCmd )
        TSPIFree( pchCmd );
    if( pszExpand )
        TSPIFree( pszExpand );

    DEBUGMSG(ZONE_FUNC, (TEXT("UNIMODEM: -ModemInit %s\n"),
        (pLineDev->MdmState == MDMST_UNKNOWN) ? TEXT("FAILED"): TEXT("OK")));
    return (pLineDev->MdmState == MDMST_UNKNOWN) ? FALSE : TRUE;
}   // ModemInit


#define NUM_BAUD_RATES 9

DWORD
NextBaudRate(
    DWORD dwBaudRate
    )
{
    switch (dwBaudRate) {
    case CBR_19200:  return CBR_57600;
    case CBR_57600:  return CBR_115200;
    case CBR_115200: return CBR_38400;
    case CBR_38400:  return CBR_56000;
    case CBR_56000:  return CBR_128000;
    case CBR_128000: return CBR_256000;
    case CBR_256000: return CBR_9600;
    case CBR_9600:   return CBR_14400;
    case CBR_14400:  return CBR_19200;
    }
    return CBR_19200;
}

//
// Called to set the baud rate for a NULL MODEM device.
//
BOOL
AutoBaudRate(
    PTLINEDEV pLineDev,
    DWORD dwBaudRate
    )
{
    DCB commDCB;
    DWORD status;
    DWORD Retry;
    DWORD dwCurBaud;

    Sleep(200);
    if (!EscapeCommFunction ( pLineDev->hDevice, CLRDTR)) {
        status = GetLastError();
        DEBUGMSG(ZONE_CALLS, (TEXT("UNIMODEM:AutoBaudRate EscapeCommFunction(CLRDTR) failed %d\n"), status));
        return FALSE;
    }
    Sleep(400);

    if (!GetCommState(pLineDev->hDevice, &commDCB)) {
        status = GetLastError();
        DEBUGMSG(ZONE_CALLS, (TEXT("UNIMODEM:AutoBaudRate GetCommState failed %d\n"), status));
        return FALSE;
    }

    Retry = 0;

    if (dwBaudRate) {
        dwCurBaud = dwBaudRate;
    } else {
abr_next:
        dwCurBaud = NextBaudRate(pLineDev->dwCurrentBaudRate);
    }

    pLineDev->dwCurrentBaudRate = commDCB.BaudRate = dwCurBaud;

    if (!PurgeComm(pLineDev->hDevice, PURGE_TXCLEAR|PURGE_RXCLEAR)) {
        status = GetLastError();
        DEBUGMSG(ZONE_CALLS, (TEXT("UNIMODEM:AutoBaudRate PurgeComm failed %d\r\n"), status));
    }
    if (!SetCommState(pLineDev->hDevice, &commDCB)) {
        status = GetLastError();
        DEBUGMSG(ZONE_CALLS, (TEXT("UNIMODEM:AutoBaudRate SetCommState(baud = %d) failed %d\n"),
            commDCB.BaudRate, status));
        //
        // If SetCommState fails with inval parm, assume the error was an unsupported baud rate
        // and try the next one.
        //
        if (status == ERROR_INVALID_PARAMETER) {
            Retry++;
            if (Retry < NUM_BAUD_RATES) {
                goto abr_next;
            }
        }
        DEBUGMSG(ZONE_CALLS, (TEXT("UNIMODEM:AutoBaudRate failed.\n")));
        return FALSE;
    }

    if (!EscapeCommFunction ( pLineDev->hDevice, SETDTR)) {
        status = GetLastError();
        DEBUGMSG(ZONE_CALLS, (TEXT("UNIMODEM:AutoBaudRate EscapeCommFunction(SETDTR) failed %d\n"), status));
        return FALSE;
    }
    Sleep(200);

    DEBUGMSG(ZONE_CALLS, (TEXT("UNIMODEM:AutoBaudRate trying %d\n"), commDCB.BaudRate));
    return TRUE;
}   // AutoBaudRate


BOOL
CallNotDropped(
    PTLINEDEV pLineDev
    )
{
    switch (pLineDev->dwCallState) {
    case LINECALLSTATE_CONNECTED:
    case LINECALLSTATE_DIALING:
    case LINECALLSTATE_DIALTONE:
    case LINECALLSTATE_PROCEEDING:
    case LINECALLSTATE_UNKNOWN:
            return TRUE;
    }
    DEBUGMSG(ZONE_CALLS, (TEXT("UNIMODEM:CallNotDropped call state = %x\n"), pLineDev->dwCallState));
    return FALSE;
}


// **********************************************************************
//
// This is the dialer thread.  It is created whenever we decide to
// dial a modem, and exists until the call completes or is terminated.
// 
// **********************************************************************
void
Dialer(
    PTLINEDEV pLineDev
    )
{
    DCB  commDCB;
    MODEMRESPCODES MdmResp;
    BOOL fOriginate;
    BOOL bFirstResp;
    PWCHAR pszzDialCommands = NULL;  // strings of dial strings returned from CreateDialCommand
    PWCHAR ptchDialCmd;
    PCHAR  pchDialCmd = NULL;
    DWORD  dwDialLen;
    DWORD  LineCallState;
    DWORD  LineCallStateParm2;
    DWORD  LineError;   // Indicates asynchronous return code for LineMakeCall completion
    DWORD  dwCallFailTimer;
    BOOL bKeepLooping;
    
    DEBUGMSG(ZONE_FUNC|ZONE_CALLS,
             (TEXT("UNIMODEM:Dialer - dial options x%X (x%X), modem options x%X\r\n"),
              pLineDev->dwDialOptions, pLineDev->DevMiniCfg.fwOptions, pLineDev->DevMiniCfg.dwModemOptions));

    dwCallFailTimer = pLineDev->DevMiniCfg.dwCallSetupFailTimer;
    LineCallStateParm2 = LINEDISCONNECTMODE_NORMAL;
    LineError = LINEERR_OPERATIONFAILED;
    bFirstResp = TRUE;

    


    if( (dwCallFailTimer == 0) ||
        (dwCallFailTimer > 512) )
        dwCallFailTimer = 512;
    
    // Clear any old CallComplete events before starting a new call
    WaitForSingleObject(pLineDev->hCallComplete, 0);
    
    pLineDev->dwCallState = LineCallState = LINECALLSTATE_UNKNOWN;

    try
    {
        // Disable HW flow control while we try to connect.
        // Otherwise, our xmit may get stuck in a flowed-off state and
        // then we cannot send.
        GetCommState( pLineDev->hDevice, &commDCB );
        commDCB.fRtsControl = RTS_CONTROL_ENABLE;
        commDCB.fOutxCtsFlow = 0;
        SetCommState( pLineDev->hDevice, &commDCB );
    
        // Our job is to dial the modem, and then sit here waiting for
        // a response.  All the helper routines in this file are responsible
        // for driving the state machine.
        LineCallState = LINECALLSTATE_DIALING;
        NewCallState(pLineDev, LineCallState, 0L);

        //
        // If we are here because of a lineDial call, then skip the NULL modem
        // and manual dial stuff
        //
        if (pLineDev->dwPendingType == PENDING_LINEDIAL) {
            goto dial_continue;
        }

#define NULL_MODEM_RETRY	3
	
        if( IS_NULL_MODEM(pLineDev) )
        {
            DWORD Retry;
            DWORD BaudRates;
            DWORD NumBaudRates;        
        
            DEBUGMSG(ZONE_CALLS,
                     (TEXT("UNIMODEM:Dialer - Attempting DCC connection\r\n")));

            //
            // This is a DCC connection.  Dial strings are simple.
            // Send 'CLIENT', expect back 'CLIENTSERVER'. Try several
            // times, unless we are dropping the line
            //

            if (pLineDev->DevMiniCfg.dwModemOptions & MDM_SPEED_ADJUST) {
                NumBaudRates = NUM_BAUD_RATES;
            } else {
                //
                // Automatic baud rate detection is off, just use the supplied baud rate
                //
                NumBaudRates = 1;
                pLineDev->dwLastBaudRate = 0;
            }

            if (!AutoBaudRate(
                    pLineDev, 
                    (pLineDev->dwLastBaudRate) ?
                            pLineDev->dwLastBaudRate : pLineDev->DevMiniCfg.dwBaudRate)) {
                pLineDev->dwLastBaudRate = 0;
                LineCallStateParm2 = LINEDISCONNECTMODE_UNKNOWN;
                goto dial_done;
            }

            for (BaudRates=0; BaudRates < NumBaudRates; BaudRates++) {
                for( Retry=0;
                    (Retry <= NULL_MODEM_RETRY) && (CallNotDropped(pLineDev) );
                    Retry++ )
                {
                    // Start the watchdog thread before we block in a read
                    if( SetWatchdog( pLineDev, dwCallFailTimer * 1000 ) )
                    {
                        LineCallStateParm2 = LINEDISCONNECTMODE_UNKNOWN;
                        goto dial_done;
                    }
                
                    // Send out CLIENT
                    if (MdmSendCommand( pLineDev, pchDCCCmd )) {
                        // Now look for CLIENTSERVER
                        do
                        {
                            MdmResp = MdmGetResponse( pLineDev, (PUCHAR)pchDCCCmd, NOCS);
                        } while ( (MODEM_FAILURE != MdmResp) &&
                                  (MODEM_EXIT != MdmResp) &&
                                  (MODEM_CONNECT != MdmResp) &&
                                  CallNotDropped(pLineDev) );
        
                    } else {
                        MdmResp = MODEM_FAILURE;
                    }
    
                    if( MODEM_CONNECT == MdmResp )
                    {
                        // Cool, we are connected
                        LineError = 0;
                        pLineDev->dwLastBaudRate = pLineDev->dwCurrentBaudRate;
                        pLineDev->dwCallFlags |= CALL_ACTIVE;
                        goto dial_connected;
                    }
    
                    // We timed out on a connect.  Reset the watchdog and try again
                    SetWatchdog( pLineDev, 0 );

                    //
                    // Check if the operation got cancelled.
                    //
                    if (pLineDev->dwPendingType != PENDING_LINEMAKECALL) {
                        LineCallStateParm2 = LINEDISCONNECTMODE_UNKNOWN;
                        goto dial_done;
                    }
    
                    // Tell the application that we are retrying connect
    				if (Retry < NULL_MODEM_RETRY) {
    					NewCallState(pLineDev, LineCallState, 0L);
    				}
    
                }

                if (BaudRates == 0) {
                    //
                    // Make sure the baud rate is within autobaud's range
                    //
                    pLineDev->dwCurrentBaudRate = NextBaudRate(0);
                }

                if (!AutoBaudRate(pLineDev, 0)) {
                    pLineDev->dwLastBaudRate = 0;
                    LineCallStateParm2 = LINEDISCONNECTMODE_UNKNOWN;
                    goto dial_done;
                }
            }

            pLineDev->dwLastBaudRate = 0;
            LineCallStateParm2 = LINEDISCONNECTMODE_UNKNOWN;
            goto dial_done;
        }
        else if (pLineDev->DevMiniCfg.fwOptions & MANUAL_DIAL)
        {
            if (DisplayTerminalWindow(pLineDev, IDS_MANUAL_DIAL_TERMINAL)) {
                // The user pressed OK, so he must be connected.
                LineError = 0;
                pLineDev->dwCallFlags |= CALL_ACTIVE;
            }
            goto dial_done;
        }
        else  
        {
            // Its not a NULL modem, so send actual dial strings
            DEBUGMSG(ZONE_CALLS, (TEXT("UNIMODEM:Dialer - Attempting Modem connection\r\n")));
         
            EnterCriticalSection(&pLineDev->OpenCS);
            ModemInit(pLineDev);
            LeaveCriticalSection(&pLineDev->OpenCS);
            
            // If modeminit failed, we sure won't be able to dial
            if( MDMST_UNKNOWN == pLineDev->MdmState )
            {
                DEBUGMSG(ZONE_CALLS,
                         (TEXT("UNIMODEM:Dialer - Modem Reset failed, aborting call\r\n")));
                LineCallStateParm2 = LINEDISCONNECTMODE_UNKNOWN;
                goto dial_done;
            }
            
            // Are we supposed to do a pre-dial terminal???
            if ( (pLineDev->DevMiniCfg.fwOptions & TERMINAL_PRE) &&
                 CallNotDropped(pLineDev) )
            {
                DEBUGMSG(ZONE_CALLS,
                         (TEXT("UNIMODEM:Dialer - PreDial - Modem State x%X\r\n"),
                          pLineDev->MdmState ));
                if (!DisplayTerminalWindow(pLineDev, IDS_PRE_DIAL_TERMINAL))
                {
                    // Either the user cancelled or something went wrong.
                    goto dial_done;
                }    
            }

dial_continue:       
            // Convert the dial string to a modem format.
            DEBUGMSG(ZONE_MISC, (TEXT("UNIMODEM:Dialer - Ready to create Dial Strings\r\n")));
            pszzDialCommands = CreateDialCommands( pLineDev, &fOriginate );
            if (!pszzDialCommands) {
                goto dial_done;
            }

            ptchDialCmd = pszzDialCommands;

            // Dial strings get truncated to pLineDev->dwMaxCmd so
            // allocate a buffer large enough to store that.
            pchDialCmd = TSPIAlloc( pLineDev->dwMaxCmd + 4 );
            if (!pchDialCmd) {
                goto dial_done;
            }
    
            // Start the watchdog thread before we block in a read
            if( SetWatchdog( pLineDev, dwCallFailTimer * 1000 ) )
            {
                LineCallStateParm2 = LINEDISCONNECTMODE_UNKNOWN;
                goto dial_done;
            }

            // Remember, this is a string of dial strings.  We need to iterate
            // through each of them and handle the responses.
            while (CallNotDropped(pLineDev) && *ptchDialCmd)
            {
                // We need to convert the UniCode to ASCII
                dwDialLen = wcslen(ptchDialCmd) + 1;
                DEBUGMSG(ZONE_MISC,
                         (TEXT("UNIMODEM:Dialer - Sending Dial string of len %d - %s\r\n"),
                          dwDialLen, ptchDialCmd));
                
                dwDialLen = wcstombs( pchDialCmd, ptchDialCmd, dwDialLen );

                DEBUGMSG(ZONE_CALLS,
                         (TEXT("UNIMODEM:Dialer - Advancing ptchDialCmd to x%X\r\n"),
                          ptchDialCmd));
                ptchDialCmd += wcslen(ptchDialCmd) + 1;
                        
                // Send the command and wait for a response
                MdmSendCommand( pLineDev, pchDialCmd );

                // Loop looking for a failure or success response
                bKeepLooping = TRUE;
                while ((CallNotDropped(pLineDev)) && (bKeepLooping)) {
                    MdmResp = MdmGetResponse( pLineDev, pchDialCmd, NOCS );
                    bKeepLooping = FALSE;

                    if (bFirstResp) {
                        bFirstResp = FALSE;
                        switch (MdmResp) {
                        case MODEM_CARRIER:
                        case MODEM_CONNECT:
                        case MODEM_PROGRESS:
                        case MODEM_PROTOCOL:
                        case MODEM_SUCCESS:
                            




                            NewCallState(pLineDev, LINECALLSTATE_DIALTONE, LINEDIALTONEMODE_UNAVAIL);

                            break;
                        }
                    }

                    LineError = LINEERR_OPERATIONFAILED;

                    switch (MdmResp) {
                    case MODEM_CONNECT:
                        DEBUGMSG(ZONE_CALLS, (TEXT("UNIMODEM:Dialer - Dial succeeded\n")));
                        pLineDev->dwCallFlags |= CALL_ACTIVE;
                        LineError = 0;
                        break;

                    case MODEM_BUSY:
                        // If it was busy, let application know about it
                        NewCallState(pLineDev, LINECALLSTATE_BUSY, 0L);
                        LineCallStateParm2 = LINEDISCONNECTMODE_BUSY;
                        DEBUGMSG(ZONE_CALLS,(TEXT("UNIMODEM:Dialer - BUSY\r\n")));
                        goto dial_done;

                    case MODEM_NODIALTONE:
                        LineCallStateParm2 = LINEDISCONNECTMODE_NODIALTONE;
                        DEBUGMSG(ZONE_CALLS,(TEXT("UNIMODEM:Dialer - No Dial Tone\r\n")));
                        goto dial_done;

                    case MODEM_NOANSWER:
                        LineCallStateParm2 = LINEDISCONNECTMODE_NOANSWER;
                        DEBUGMSG(ZONE_CALLS,(TEXT("UNIMODEM:Dialer - No Answer\r\n")));
                        goto dial_done;
                    
                    case MODEM_HANGUP:
                        LineCallStateParm2 = LINEDISCONNECTMODE_NORMAL;
                        DEBUGMSG(ZONE_CALLS,(TEXT("UNIMODEM:Dialer - HANGUP\r\n")));
                        goto dial_done;
                    
                    case MODEM_FAILURE:
                    case MODEM_EXIT:
                        LineCallStateParm2 = LINEDISCONNECTMODE_UNKNOWN;
                        DEBUGMSG(ZONE_CALLS,(TEXT("UNIMODEM:Dialer - UNKNOWN Disconnect mode\r\n")));
                        goto dial_done;
                    
                    case MODEM_SUCCESS:
                    case MODEM_PROGRESS:
                    case MODEM_CARRIER:
                        DEBUGMSG(ZONE_CALLS,(TEXT("UNIMODEM:Dialer - Continuing Dial Sequence\r\n")));
                        LineError = 1;
                        break;

                    default:
                        DEBUGMSG(ZONE_CALLS,
                                 (TEXT("UNIMODEM:Dialer - Unknown response x%X\r\n"),
                                  MdmResp));
                        // Don't fail on unknown responses. Just look for a known good, a fail, or a timeout
                        bKeepLooping = TRUE;
                        break;
                    }
                }
            }   // while more dial strings
        }
    
dial_connected:
        SetWatchdog( pLineDev, 0 );  // Stop the watchdog

        // Are we supposed to do a post-dial terminal???
        // LAM - Why isn't this inside of the NOT MANUAL DIAL section
        if ( (pLineDev->DevMiniCfg.fwOptions & TERMINAL_POST) &&
             CallNotDropped(pLineDev) )

        {
            DEBUGMSG(ZONE_CALLS,
                     (TEXT("UNIMODEM:Dialer - PostDial - Modem State x%X\r\n"),
                      pLineDev->MdmState ));
            if (!DisplayTerminalWindow(pLineDev, IDS_POST_DIAL_TERMINAL)) {
                // Either the user cancelled or something went wrong.
                goto dial_done;
            }
        }
    }
    except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
            EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        // Somebody must have hit a bad handle.  Fail this dial sequence
        goto dial_done;        
    }
    
        
dial_done:
    switch (LineError) {
    case 0:
        pLineDev->DevState = DEVST_CONNECTED;
        LineCallState = LINECALLSTATE_CONNECTED;
        LineCallStateParm2 = 0;
        break;

    case 1:
        pLineDev->DevState = DEVST_PORTCONNECTWAITFORLINEDIAL;
        LineCallState = LINECALLSTATE_DIALING;
        LineCallStateParm2 = 0;
        break;

    default:
        pLineDev->DevState = DEVST_DISCONNECTED;
        LineCallState = LINECALLSTATE_DISCONNECTED;
        break;
    }

    SetWatchdog( pLineDev, 0 );  // Stop the watchdog
    
    if ((pLineDev->dwPendingType == PENDING_LINEMAKECALL) ||
        (pLineDev->dwPendingType == PENDING_LINEDIAL)) {
        DEBUGMSG(ZONE_CALLS,
                 (TEXT("UNIMODEM:Dialer - tCall Completion for ReqID x%X, Status x%X\r\n"),
                  pLineDev->dwPendingID, (LineError == 1) ? 0 : LineError));
        SetAsyncStatus(pLineDev, (LineError == 1) ? 0 : LineError);
    }

    NewCallState(pLineDev, LineCallState, LineCallStateParm2);

    if (CallNotDropped(pLineDev)) {
        if (LINECALLSTATE_CONNECTED == LineCallState) {
            try {
                // Don't forget to restore the dev state if we connected.  The init
                // sequence took care of telling the modem what we thought the
                // flow control should be.
                SetDCBfromDevMiniCfg(&commDCB, &(pLineDev->DevMiniCfg));
                if (IS_NULL_MODEM(pLineDev)) {
                    commDCB.BaudRate = pLineDev->dwLastBaudRate;
                }
                SetCommState( pLineDev->hDevice, &commDCB );
                DEBUGMSG(ZONE_FUNCTION|ZONE_CALLS,
                         (TEXT("UNIMODEM:Dialer - RTS Control %d, CTS Out Flow %d, XON/XOFF out/in %d/%d\r\n\r\n"),
                          commDCB.fRtsControl, commDCB.fOutxCtsFlow, commDCB.fOutX, commDCB.fInX));
            }
            except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
                    EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
                // Nothing special needs done if the handle is gone.
            }
            
            DEBUGMSG(ZONE_CALLS, (TEXT("UNIMODEM:Dialer **** Call Connected\r\n")));
        }
    } else {
        DEBUGMSG(ZONE_CALLS, (TEXT("UNIMODEM:Dialer **** Call State x%X, closing port\r\n"), LineCallState));
        DevlineClose(pLineDev, FALSE);
    }

    // Free up anything we may have allocated in here.
    if( pchDialCmd )
        TSPIFree( pchDialCmd );
    if( pszzDialCommands )
        TSPIFree( pszzDialCommands );
    
    // We set an event when we are done so that DevLineDrop can
    // continue with his cleanup.
    SetEvent(pLineDev->hCallComplete);
    
    DEBUGMSG(ZONE_FUNC|ZONE_CALLS, (TEXT("UNIMODEM:-Dialer\r\n")));
}


