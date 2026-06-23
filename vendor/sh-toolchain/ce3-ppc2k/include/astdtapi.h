// The  Telephony  API  is jointly copyrighted by Intel and Microsoft.  You are
// granted  a royalty free worldwide, unlimited license to make copies, and use
// the   API/SPI  for  making  applications/drivers  that  interface  with  the
// specification provided that this paragraph and the Intel/Microsoft copyright
// statement is maintained as is in the text and source code files.
//
// Copyright (c) 1992-1998 Intel/Microsoft, all rights reserved.

#ifndef __ASTDTAPI_H__
#define __ASTDTAPI_H__

#include <tapi.h>

#ifndef ASTDTAPIAPI
#define ASTDTAPIAPI __declspec(dllimport)
#endif // !ASTDTAPIAPI


LONG
ASTDTAPIAPI
tapiRequestMakeCallW(
    LPCWSTR              lpszDestAddress,
    LPCWSTR              lpszAppName,
    LPCWSTR              lpszCalledParty,
    LPCWSTR              lpszComment
    );

#define tapiRequestMakeCall tapiRequestMakeCallW

LONG
ASTDTAPIAPI
tapiGetLocationInfoW(
    LPWSTR               lpszCountryCodeW,
    LPWSTR               lpszCityCodeW
    );

#define tapiGetLocationInfo tapiGetLocationInfoW


#define TAPIERR_NOREQUESTRECIPIENT              -2L
#define TAPIERR_REQUESTQUEUEFULL                -3L
#define TAPIERR_INVALDESTADDRESS                -4L
#define TAPIERR_REQUESTFAILED                   -16L
#define TAPIERR_REQUESTCANCELLED                -17L
#define TAPIERR_INVALPOINTER                    -18L

#define TAPIMAXAPPNAMESIZE                      40L
#define TAPIMAXCALLEDPARTYSIZE                  40L
#define TAPIMAXCOMMENTSIZE                      80L

#endif //!__ASTDTAPI_H__