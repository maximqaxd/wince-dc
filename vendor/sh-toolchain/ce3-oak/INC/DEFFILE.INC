// Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.
//
// Forced include file for preprocessing dll def files.
//

#if defined(MIPS) || defined(SH3)
#define STDAPI(sz,c) sz
#define METHOD(x,y) x##A##y
#else
#if UNDER_CE
#define STDAPI(sz,c) sz
#else
#define STDAPI(sz,c) sz##@##c
#endif
#define METHOD(x,y) x##E##y
#endif

#ifdef UNDER_CE
#include <bldver.h>
#endif