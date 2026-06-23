/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

Module Name:  

audiddsi.h

--*/
#ifndef __AUDIDDSI_H
#define __AUDIDDSI_H

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
   iAudioPowerOff,
   iAudioStopped,
   iAudioUnderflow,
   iAudioPlaying
} AUDIO_STATE;

extern const DWORD gIntrAudio;

typedef VOID (*PFN_AUDIO_CALLBACK)(PBYTE);




// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
typedef struct _AUDIO_LINE {
   PFN_AUDIO_CALLBACK  pfnAudioCallback;
   PPCMWAVEFORMAT pFormat; 
   UINT    wFlags;
   PBYTE   pMemFile;       
   PBYTE   pData;          
   ULONG   nBufferSize;
   ULONG   nBufferPosition;
   BOOL    fLooping;

} AUDIO_LINE, *PAUDIO_LINE;



// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
AUDIO_STATE 
PDD_AudioGetInterruptType (
   VOID
   );



// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
BOOL 
PDD_AudioInitialize (
   VOID
   );



// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
VOID 
PDD_AudioDisable (
   BOOL fDetach
   );



// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
BOOL 
PDD_AudioQueryFormat (
   PPCMWAVEFORMAT lpFormat
   );



// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
BOOL 
PDD_AudioSetVolume ( 
   ULONG nVolume
   );



// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
BOOL 
PDD_AudioGetVolume (
   PULONG lpnVolume
   );



// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
VOID 
PDD_AudioStartPrimary (
   PAUDIO_LINE head
   );



// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
VOID 
PDD_AudioContinue (
   PAUDIO_LINE head
   );



// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
VOID 
PDD_AudioStopPlay (
   VOID
   );



#define ZONE_TEST       DEBUGZONE(0)
#define ZONE_1          DEBUGZONE(1)
#define ZONE_2          DEBUGZONE(2)
#define ZONE_3          DEBUGZONE(3)
#define ZONE_4          DEBUGZONE(4)
#define ZONE_5          DEBUGZONE(5)
#define ZONE_6          DEBUGZONE(6)
#define ZONE_7          DEBUGZONE(7)
#define ZONE_8          DEBUGZONE(8)
#define ZONE_MISC       DEBUGZONE(9)
#define ZONE_10         DEBUGZONE(10)
#define ZONE_11         DEBUGZONE(11)
#define ZONE_ALLOC      DEBUGZONE(12)
#define ZONE_FUNCTION   DEBUGZONE(13)
#define ZONE_WARN       DEBUGZONE(14)
#define ZONE_ERROR      DEBUGZONE(15)

//-----------------------------------------------------------------------------
// DEBUG MACROS 
//
#define FUNC(x)    DEBUGMSG(ZONE_FUNCTION, (TEXT("%a(%d) : [FUNCTION] %a()\r\n"), __FILE__, __LINE__, x))  
#define MSG1(x)    DEBUGMSG(ZONE_MISC,     (TEXT("%a(%d) : [MSG] %a()\r\n"), __FILE__, __LINE__, x))  
#define ERRMSG1(x) DEBUGMSG(ZONE_ERROR,    (TEXT("%a(%d) : [ERROR] %a()\r\n"), __FILE__, __LINE__, x))  
#define WRNMSG1(x) DEBUGMSG(ZONE_WARN,     (TEXT("%a(%d) : [WARNING] %a()\r\n"), __FILE__, __LINE__, x))  
#define TESTMSG(x) DEBUGMSG(ZONE_TEST,     (TEXT("%a(%d) : [TEST] %a()\r\n"), __FILE__, __LINE__, x))  



#ifdef __cplusplus
}
#endif

#endif
