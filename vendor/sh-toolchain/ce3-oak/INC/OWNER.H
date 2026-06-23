/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

Module Name:

owner.h

Abstract:

Notes:


--*/


#ifdef __cplusplus
extern "C" {
#endif


// owner/notes/password include file
#define TYPE_OWNER			0
#define TYPE_NOTES			1
#define TYPE_PASSWORD 		2
		

#define PASSWORD_LENGTH		40
#define MAX_NOTES_STRING 	(6*32)
#define MAX_NAME_STRING 	32
#define MAX_COMPANY_STRING 	32
#define MAX_ADD_STRING 		(4*32)
#define MAX_CC_STRING	 	4
#define MAX_AC_STRING	 	10
#define MAX_PHONE_STRING	24

typedef struct tagOWNER_PROFILE {
	// Owner ID
	TCHAR   szName[MAX_NAME_STRING+1];
	TCHAR   szCompany[MAX_COMPANY_STRING+1];
	TCHAR   szAdd[MAX_ADD_STRING+1];

	TCHAR   szWCC[MAX_CC_STRING+1];
	TCHAR   szWAC[MAX_AC_STRING+1];
	TCHAR   szWPhone[MAX_PHONE_STRING+1];

	TCHAR   szHCC[MAX_CC_STRING+1];
	TCHAR   szHAC[MAX_AC_STRING+1];
	TCHAR   szHPhone[MAX_PHONE_STRING+1];
	BYTE    IDPowerUp;
	} OWNER_PROFILE;

typedef struct tagOWNER_NOTES {
	// Notes
	TCHAR   Notes[MAX_NOTES_STRING+1];
	BYTE    NotesPowerUp;
	} OWNER_NOTES ;


typedef struct tagOWNER_PASSWRD {
	// Passwrd
	BYTE    PasswordOnPowerUp;

#ifdef UNDER_NT			
	TCHAR	Password[PASSWORD_LENGTH+1];
#endif
	} OWNER_PASSWRD;



// Registry Key defines
// Owner
#define	szOwner				TEXT("Owner")
// Notes
#define	szNotes				TEXT("Notes")
// Password
#define szPasswordPowerUp	TEXT("PowrPass")


#ifdef UNDER_NT
#define szRegistryKey		TEXT("WinceControlPanel\\Owner")
#else
#define szRegistryKey		TEXT("ControlPanel\\Owner")
#endif

#ifdef UNDER_NT
#define szPassword 			TEXT("Pass")
#endif


#ifdef __cplusplus
}
#endif


