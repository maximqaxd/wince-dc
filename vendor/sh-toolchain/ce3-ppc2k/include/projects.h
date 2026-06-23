/*++ 
  Copyright (c) 1998 Microsoft Corporation
  
  File: project.h
  
  Abstract:
  
  Contents:
    Project APIs defines
  
  HISTORY
   4/24/97 jgoldb    Created
  12/11/97 ?         moved to ...\sdk\inc    
   7/13/98 jparks    Added PA structure, created Ex versions of ENUM functions
  10/14/98 a-kevpau  Changed WCHAR to TCHAR in PAstruct
   3/31/99 jparks    moved to ...\sdk\inc (I guess the first time didn't stick)

    Copyright (c) 1997-1999, Microsoft Corp. All rights reserved.
--*/
#ifndef __PROJECT_H__
#define __PROJECT_H__

#ifdef __cplusplus
extern "C" {
#endif



typedef enum tagENUM {
	PRJ_ENUM_MEMORY		= 0x1,	// enumerate main memory projects only				
	PRJ_ENUM_FLASH		= 0x2,	// enumerate flash card projects only
	PRJ_ENUM_ALL_DEVICES= 0x4,	// enumerate main memory projects & Flash card,	
	PRJ_ENUM_ALL_PROJ	= 0x10,	// enumerate in all projects 
	PRJ_ENUM_HOME_PROJ	= 0x100,// add 'My Documents' home folder 
} PRJ_ENUM;


//////////////////////////////////////////////////////////////////////////////
// THE "OID" PROBLEM  (JParks)
//
// Near the Alder->Birch switch, the OS changed from returning faked OIDs for
// storage cards and other FAT file systems to returning invalid / -1.  This
// seriously messes up anything that wants to uses OIDs to identify objects
// on a storage card.  All the functions below must be rewritten.
//

typedef BOOL (CALLBACK *PFNCOLCUSTOK)(HWND, LPVOID);
typedef BOOL (CALLBACK *PROJECTS_ENUMPROC)(DWORD, LPARAM);
typedef BOOL (CALLBACK *PROJECTSFILES_ENUMPROC)(DWORD dwOID, LPARAM lParam);

/*****************************************************************************
EnumProjects	Exported API: Enumerates all projects on the specified file
				system.


PARAMETERS:
pfnEnumProc	pointer to callback function. If NULL, this function simply
			returns the number of projects, without enumerating through them.
			If the callback function ever returns FALSE, the enumeration will
			halt.  The callback has the following prototype:
			BOOL CALLBACK EnumProjectsCallback(DWORD dwOid, LPARAM lParam);

dwOidFlash	Only used for (dwFlags == PRJ_ENUM_FLASH).  This is the OID of
			the flash card to look at; the value returned by
			FindFirstFlashCard or FindNextFlashCard.

dwFlags	
			Any combo of the following (specifying the LOCATION to check):
			PRJ_ENUM_MEMORY - check all main memory projects/folders,
							  dwOidFlash is not used
			PRJ_ENUM_FLASH  - check all projects on the specified flash card,
							  where dwOidFlash specifies the flash card to
							  examine.
			PRJ_ENUM_ALL_DEVICES - check all projects/folders from main memory,
							  and from *every* flash card.  (dwOid is not used)

lParam		user defined parameter passed to pfnEnumProc 


RETURN:
The total number of projects.  NOTE: this is actually the total number of
projects in MAIN MEMORY ONLY (see below).


THIS IS PROVIDED FOR BACKWARD COMPATIBILITY.  It will silently skip over any
project that isn't on an OID-based filesystem (it enumerates everything in
main memory, but will silently skip everything on storage cards).  The modern
function is EnumProjectsEx.
******************************************************************************/
int		EnumProjects(PROJECTS_ENUMPROC lpEnumProc, DWORD  dwOid, DWORD dwFlags, LPARAM lParam);

/*****************************************************************************
EnumProjectsFiles	Exported API: Enumaretes all files in a project on the
					specified file system.

PARAMETERS:
pfnEnumProc	pointer to callback function. If NULL, this function simply
			returns the number of files without enumerating through them.
			If the callback function ever returns FALSE, the enumeration will
			halt.  The callback has the following prototype:
 (normal)	BOOL CALLBACK EnumProjectFilesCallback(DWORD dwOID, LPARAM lParam);
 (ex ver)	BOOL CALLBACK EnumProjectFilesExCallback(PAstruct* pPA, LPARAM lParam);

dwOidFlash	Only used for (dwFlags & PRJ_ENUM_FLASH).  This is the OID of
			the flash card to look at; the value returned by
			FindFirstFlashCard or FindNextFlashCard.

dwFlags	
			Any combo of the following (specifying the LOCATION to check):
			PRJ_ENUM_MEMORY - check all main memory projects/folders,
							  dwOidFlash is not used
			PRJ_ENUM_FLASH  - check all projects on the specified flash card,
							  where dwOidFlash specifies the flash card to
							  examine.
			PRJ_ENUM_ALL_DEVICES - check all projects/folders from main memory,
							  and from *every* flash card.  (dwOid is not used)

			Plus any of the following (specifying which PROJECTS to check):
			PRJ_ENUM_ALL_PROJ- enumerate in all project (szProj is not used)

szProj		Only used for when (dwFlags & PRJ_ENUM_ALL_PROJ) is NOT set.  This
			specifies which project to search; NULL specifies to look for files
			that aren't in a project/folder (i.e., they're at the top level,
			under "\My Documents").

szFileName	pointer to name of file to search for. (i.e. '*.wav' or '*.*')

lParam		user defined parameter passed to pfnEnumProc 


RETURN:
The total number of matching files found.  NOTE: in the regular version,
this is actually the total number of files in MAIN MEMORY ONLY (see below).


The regular version is provided for backward compatibility.  It will silently
skip over any file that isn't on an OID-based filesystem (it enumerates
everything in main memory, but will silently skip everything on storage cards).

The modern version, EnumProjectsFilesEx, requires the extended callback
function, too.
******************************************************************************/
int		EnumProjectsFiles(PROJECTSFILES_ENUMPROC lpEnumProc, DWORD  dwOidFlash, DWORD dwFlags,LPTSTR lpszProj, LPTSTR lpszFileName, LPARAM lParam);

/**************************************************************************************************
FindFirstFlashCard
Find the first mountable file system
	lpFindProjData -  pointer to returned information
**************************************************************************************************/
HANDLE	FindFirstFlashCard( LPWIN32_FIND_DATA  lpFindFlashData);

/**************************************************************************************************
FindNextFlashCard                                                                                
Find the next mountable file system
	lpFindProjData	-  pointer to returned information 
	hFindFlash		- Identifies a search handle returned by a previous call to the FindFirstFlashCard function. 
**************************************************************************************************/
BOOL	FindNextFlashCard(HANDLE hFindFlash, LPWIN32_FIND_DATA  lpFindFlashData);

/**************************************************************************************************
FindFirstProjectFile                                                                                
Find the first file in a project, on the desired mountable file system
	lpFileName		- pointer to name of file to search for	(i.e: '*.wav')
	lpFindFileData	- pointer to returned information 
	dwOidFlash		- Oid to the desired mountable file system (FindFirstFlashCard or FindNextFlashCard)
					  or 0 if main memory       
	lpszProj		- desired project, or NULL (or 'All') to search for files that do not 
					  have a project (under '\My Documents')
**************************************************************************************************/
HANDLE	FindFirstProjectFile(LPCTSTR lpFileName, LPWIN32_FIND_DATA  lpFindFileData,  DWORD dwOidFlash, LPTSTR szProject);

/**************************************************************************************************
FindNextProjectFile
Find the next file in a project
	lpFindProjData	- pointer to returned information 
	hFind			- Identifies a search handle returned by a previous call to the FindFirstProjectFile function. 
**************************************************************************************************/
BOOL	FindNextProjectFile(HANDLE hFindFlash, LPWIN32_FIND_DATA  lpFindFlashData);


// new code, it solves the "OID Problem"

//////////////////////////////////////////////////////////////////////////////
// PAstruct:
// This is pretty much a drop-in replacement for the spot where OIDs are
// currently used.  Instead of passing around a DWORD, we now pass a pointer
// to this struct.  IMPORTANT: callers are responsible for allocating and
// freeing the string memory for file paths.

typedef enum _EFileIDType
{
	FILE_ID_TYPE_OID	= 0,	// This file is specified through an OID
	FILE_ID_TYPE_PATH	= 1,	// This file is specified through its pathname
	FILE_ID_LAST		= 2,	// last value in the list (invalid)
} EFileIDType;

#define PA_MAX_PATHNAME 96		// including null terminator, practically speaking, might be " \storage card 3\My Documents\document folder 7\very long filename.wav" or 69 char's max

// Pure-NT machines don't necessarily define CEOID.  In the future, we'll want
// to replace this define with the actual typedef (JParks).
#ifndef CEOID
#define CEOID DWORD
#endif

typedef struct _PAstruct		// "PA" (Pathname Array) OID-NUMBER REPLACEMENT STRUCTURE
{
	EFileIDType		m_IDtype;	// is this an OID or a pathname?
	union
	{
		CEOID		m_fileOID;	// we're storing the OID
		TCHAR		m_szPathname[PA_MAX_PATHNAME];	// we're storing the full pathname\filename
	};
} PAstruct;


typedef BOOL (CALLBACK *PROJECTS_ENUMPROC_EX)(PAstruct* pPA, LPARAM lParam);
typedef BOOL (CALLBACK *PROJECTSFILES_ENUMPROC_EX)(PAstruct* pPA, LPARAM lParam);

/*****************************************************************************
EnumProjectsEx	Exported API: Enumerates all projects on the specified file
				system.

PARAMETERS:
pfnEnumProc	pointer to callback function. If NULL, this function simply
			returns the number of projects, without enumerating through them.
			If the callback function ever returns FALSE, the enumeration will
			halt.  The callback has the following prototype:
			BOOL CALLBACK EnumProjectsExCallback(PAstruct* pPA, LPARAM lParam);

dwOidFlash	Only used for (dwFlags == PRJ_ENUM_FLASH).  This is the OID of
			the flash card to look at; the value returned by
			FindFirstFlashCard or FindNextFlashCard.

dwFlags	
			Any combo of the following (specifying the LOCATION to check):
			PRJ_ENUM_MEMORY - check all main memory projects/folders,
							  dwOidFlash is not used
			PRJ_ENUM_FLASH  - check all projects on the specified flash card,
							  where dwOidFlash specifies the flash card to
							  examine.
			PRJ_ENUM_ALL_DEVICES - check all projects/folders from main memory,
							  and from *every* flash card.  (dwOid is not used)

lParam		user defined parameter passed to pfnEnumProc 


RETURN:
The total number of projects *including* storage cards.


This enhanced version can enumerate through both new (OID-based) file systems
and older (DOS-style) filesystems -- including storage cards.
******************************************************************************/
int EnumProjectsEx(PROJECTS_ENUMPROC_EX pfnEnumProc, DWORD dwOidFlash, DWORD dwFlags, LPARAM lParam);


/*****************************************************************************
EnumProjectsFiles	Exported API: Enumaretes all files in a project on the
					specified file system.

PARAMETERS:
pfnEnumProc	pointer to callback function. If NULL, this function simply
			returns the number of files without enumerating through them.
			If the callback function ever returns FALSE, the enumeration will
			halt.  The callback has the following prototype:
 (normal)	BOOL CALLBACK EnumProjectFilesCallback(DWORD dwOID, LPARAM lParam);
 (ex ver)	BOOL CALLBACK EnumProjectFilesExCallback(PAstruct* pPA, LPARAM lParam);

dwOidFlash	Only used for (dwFlags & PRJ_ENUM_FLASH).  This is the OID of
			the flash card to look at; the value returned by
			FindFirstFlashCard or FindNextFlashCard.

dwFlags	
			Any combo of the following (specifying the LOCATION to check):
			PRJ_ENUM_MEMORY - check all main memory projects/folders,
							  dwOidFlash is not used
			PRJ_ENUM_FLASH  - check all projects on the specified flash card,
							  where dwOidFlash specifies the flash card to
							  examine.
			PRJ_ENUM_ALL_DEVICES - check all projects/folders from main memory,
							  and from *every* flash card.  (dwOid is not used)

			Plus any of the following (specifying which PROJECTS to check):
			PRJ_ENUM_ALL_PROJ- enumerate in all project (szProj is not used)

szProj		Only used for when (dwFlags & PRJ_ENUM_ALL_PROJ) is NOT set.  This
			specifies which project to search; NULL specifies to look for files
			that aren't in a project/folder (i.e., they're at the top level,
			under "\My Documents").

szFileName	pointer to name of file to search for. (i.e. '*.wav' or '*.*')

lParam		user defined parameter passed to pfnEnumProc 


RETURN:
The total number of matching files found.  NOTE: in the regular version,
this is actually the total number of files in MAIN MEMORY ONLY (see below).


The regular version is provided for backward compatibility.  It will silently
skip over any file that isn't on an OID-based filesystem (it enumerates
everything in main memory, but will silently skip everything on storage cards).

The modern version, EnumProjectsFilesEx, requires the extended callback
function, too.
******************************************************************************/
int EnumProjectsFilesEx(PROJECTSFILES_ENUMPROC_EX pfnEnumProc, DWORD dwOidFlash, DWORD dwFlags,LPTSTR szProj, LPTSTR szFileName, LPARAM lParam);


#ifdef __cplusplus
}
#endif


#endif // __PROJECT_H__

