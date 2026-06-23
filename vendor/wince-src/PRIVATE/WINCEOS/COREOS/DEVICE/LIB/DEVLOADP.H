/* File:    devloadp.h
 *
 * Purpose: Device Loader internal definitions
 *
 * Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.
 */

#define MAX_LOAD_ORDER 255

#define REG_PATH_LEN 256    // Length of registry path string

extern const TCHAR s_DllName_ValName[];
extern const TCHAR s_ActiveKey[];
extern const TCHAR s_BuiltInKey[];
extern const TCHAR s_PcmciaKey[];

//
// typedefs of pcmcia.dll functions device.exe uses
//
typedef DWORD (*REGISTERCLIENT)(CLIENT_CALLBACK, PCARD_REGISTER_PARMS);
typedef DWORD (*GETFIRSTTUPLE)(PCARD_TUPLE_PARMS);
typedef DWORD (*GETNEXTTUPLE)(PCARD_TUPLE_PARMS);
typedef DWORD (*GETTUPLEDATA)(PCARD_DATA_PARMS);
typedef DWORD (*GETPARSEDTUPLE)(CARD_SOCKET_HANDLE, UINT8, PVOID, PUINT32);
typedef DWORD (*GETSTATUS)(PCARD_STATUS);
typedef DWORD (*SYSTEMPOWER)(DWORD);
typedef DWORD (*SYSTEMINIT)(VOID);

#ifdef DEBUG
//
// Debug zones
//
#define ZONE_ERROR      DEBUGZONE(0)
#define ZONE_WARNING    DEBUGZONE(1)
#define ZONE_FUNCTION   DEBUGZONE(2)
#define ZONE_INIT       DEBUGZONE(3)
#define ZONE_BUILTIN    DEBUGZONE(4)
#define ZONE_PCMCIA     DEBUGZONE(5)
#define ZONE_ACTIVE     DEBUGZONE(6)
#define ZONE_TAPI       DEBUGZONE(7)
#define ZONE_FSD        DEBUGZONE(8)
#define ZONE_DYING      DEBUGZONE(9)

#endif  // DEBUG
