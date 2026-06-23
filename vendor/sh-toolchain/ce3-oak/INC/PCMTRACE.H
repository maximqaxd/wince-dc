/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

Module Name:  

    pcmtrace.h

Abstract:  

    This file implements a non-intrusive debug trace facility which logs debug
    output to the PCMCIA common memory area.
    This is provided as a sample to platform writers and is
    expected to be able to be used without modification on most (if not
    all) hardware platforms.

Functions:


Notes:

    The PCMCIA Tracing Utilities defines/macros can be used to do non-intrusive
    tracing by using an analyzer to monitor an SRAM card inserted in the
    PCMCIA slot.

    This file should be conditionally included in your sources.

    You have two options for how this is implemented.  The default
    relies on SetKMode calls, which add an overhead of 120 uS for
    each debug.  Alternatively, if you #define FAST_PCM_DBG then
    the macro will use a byte pointer __PCMDBG_PTR which must be
    initialized by calling PCM_DBG_INIT somewhere in you code.

--*/


#ifndef _PCMTRACE_H
#define _PCMTRACE_H 1

#ifdef __cplusplus
extern "C" {
#endif


//================ First, define the macros we need to do the tracing =====================
#ifdef USE_PCMCIA_DEBUGS

#include <pcm_dbg.h>  // Get platform dependent defines.

#ifdef FAST_PCM_DBG  //------------------------------------------------------------------
extern BYTE * __PCMDBG_PTR;

# define PCM_DBG_INIT( ) \
  { \
    void * pvAdd; \
    \
    __PCMDBG_PTR = NULL; \
	pvAdd = VirtualAlloc(0, __PCMDBG_MEMORY_SIZE, MEM_RESERVE, PAGE_NOACCESS); \
	if( NULL != pvAdd ) \
		if( VirtualCopy( pvAdd, (void*)__PCMDBG_BASE, \
                         __PCMDBG_MEMORY_SIZE, PAGE_READWRITE | PAGE_NOCACHE ) ) \
           __PCMDBG_PTR = (BYTE*)pvAdd; \
  }

# define PCM_DBG_DEINIT( ) \
  { \
        if (__PCMDBG_PTR != NULL) \
           VirtualFree(__PCMDBG_PTR,__PCMDBG_MEMORY_SIZE,MEM_RELEASE);\
  }

# define PCM_DBG( Key, Value ) \
  if( NULL != __PCMDBG_PTR ) \
  { \
    *(__PCMDBG_PTR + Key) = Value ; \
  }

# define PCM_DBG_WORD( Key, Value ) \
  if( NULL != __PCMDBG_PTR ) \
  { \
    *((UINT16 *)(__PCMDBG_PTR + Key)) = Value ; \
  }

#else // FAST_PCM_DBG ------------------------------------------------------------------

#define PCM_DBG_INIT()
#define PCM_DBG_DEINIT()

# define PCM_DBG( Key, Value ) \
  { \
    BOOL fMode; \
    fMode = SetKMode( TRUE ); \
    *((BYTE *)__PCMDBG_BASE + Key) = Value ; \
    SetKMode( fMode ); \
  }

# define PCM_DBG_WORD( Key, Value ) \
  { \
    BOOL fMode; \
    fMode = SetKMode( TRUE ); \
    *((UINT16 *)(__PCMDBG_BASE + Key)) = Value ; \
    SetKMode( fMode ); \
  }
#endif  // FAST_PCM_DBG ----------------------------------------------------------------

#else  // USE_PCMCIA_DEBUGS
#define PCM_DBG( Key, Value )
#define PCM_DBG_WORD( Key, Value)
#define PCM_DBG_INIT()
#define PCM_DBG_DEINIT()
#endif // USE_PCMCIA_DEBUGS

//========= And now, define some 'keys' for the various subsystems =====================
#define PCM_KEY_SERIAL 0x1000

#define PCM_KEY_TOUCH  0x2000
#define PCM_KEY_TOUCH_RAWX_HI     (PCM_KEY_TOUCH +1)   /* Raw X sample */
#define PCM_KEY_TOUCH_RAWX_LO     (PCM_KEY_TOUCH +2)   /* Raw X sample */
#define PCM_KEY_TOUCH_RAWY_HI     (PCM_KEY_TOUCH +3)   /* Raw Y Sample */
#define PCM_KEY_TOUCH_RAWY_LO     (PCM_KEY_TOUCH +4)   /* Raw Y Sample */
#define PCM_KEY_TOUCH_FLTX_HI     (PCM_KEY_TOUCH +5)   /* Filtered X Sample */
#define PCM_KEY_TOUCH_FLTX_LO     (PCM_KEY_TOUCH +6)   /* Filtered X Sample */
#define PCM_KEY_TOUCH_FLTY_HI     (PCM_KEY_TOUCH +7)   /* Filtered Y Sample */
#define PCM_KEY_TOUCH_FLTY_LO     (PCM_KEY_TOUCH +8)   /* Filtered Y Sample */
#define PCM_KEY_TOUCH_PENUP       (PCM_KEY_TOUCH +9)   /* Pen up sample */
#define PCM_KEY_TOUCH_NEWSAMP     (PCM_KEY_TOUCH +10)  /* New sample arrived */


#define PCM_KEY_INTS   0x3000

#define PCM_KEY_AUDIO  0x4000

#define PCM_KEY_KEYBD  0x5000

#ifdef __cplusplus
}
#endif


#endif _PCMTRACE_H
