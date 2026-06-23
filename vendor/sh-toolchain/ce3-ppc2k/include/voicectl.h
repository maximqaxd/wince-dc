/*++ 
  Copyright (c) 1995 Microsoft Corporation
  
  File: VoiceCtl.c
  
  Abstract:
  
  Contents:
	Voice Control related defines
  
  History:
      10/26/97   jgoldb:   Created    
--*/

#ifndef __VOICE_CTL_H__
#define __VOICE_CTL_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tag_NM_VOICE_RECORDER 
{
	 NMHDR	    hdr;                     
	 DWORD		dwExtra;            
} NM_VOICE_RECORDER, *LPNM_VOICE_RECORDER;


typedef struct tag_CM_VOICE_RECORDER 
{
	 WORD	    cb;                     
	 DWORD		dwStyle;
	 int		xPos, yPos;
	 HWND		hwndParent;
	 int		id;
	 LPTSTR		lpszRecordFileName;
} CM_VOICE_RECORDER, *LPCM_VOICE_RECORDER;



#define VRM_FIRST               0x1900      // must be uniqe
							   
typedef enum tagVR_MSG {
	VRM_RECORD = (VRM_FIRST + 0),				
	VRM_PLAY,				
	VRM_STOP,				
	VRM_CANCEL,				
	VRM_OK,				
} VR_MSG;


#define VRN_FIRST              (0U-860U)    
#define VRN_LAST               (0U-890U)

#define VRN_RECORD_START	(VRN_FIRST-1)
#define VRN_RECORD_STOP		(VRN_FIRST-2)
#define VRN_PLAY_START		(VRN_FIRST-3)
#define VRN_PLAY_STOP		(VRN_FIRST-4)
#define VRN_CANCEL			(VRN_FIRST-5)
#define VRN_OK				(VRN_FIRST-6)
#define VRN_ERROR			(VRN_FIRST-7)


typedef enum tagVR_STYLE {
	VRS_NO_OKCANCEL = 0x0001,		// No OK/CANCLE dispalyed
	VRS_NO_NOTIFY   = 0x0002,		// No parent Notifcation
	VRS_MODAL	    = 0x0004,		// Control is Modal     
	VRS_NO_OK	    = 0x0008,		// No OK displayed
	VRS_NO_RECORD   = 0x0010,		// No REOCRD button displayed
	VRS_PLAY_MODE   = 0x0020,		// Immediatly play supplied file when launched
	VRS_NO_MOVE		= 0x0040,		// Grip is removed and the control cannot be moved around by the user
} VR_STYLE;


typedef enum tagVR_ErrorCodes {	 
	// must be the save as in VoiceDll.h
	ER_SUCCESS,         // No error 
    ER_FAIL,            // Unknown error
    ER_OUTOFMEMORY,     // Out of memory
    ER_UNUSED1,         //
    ER_INVALIDPARAM,    // Invalid param
    ER_UNUSED2,         //
    ER_WRITE_FILE_FAIL, // Write to file error
    ER_UNUSED3,         //
	ER_OOM_STORAGE,     // Out of storage
	ER_MAX_FILE_SIZE,	// Maximum file size reached during recording.
	ER_BUSY,            // Control is busy recording or playing
} VR_ECODE;


HWND	VoiceRecorder_Create(LPCM_VOICE_RECORDER lpVR);

#ifdef __cplusplus
}
#endif


#endif // __VOICE_CTL_H__
