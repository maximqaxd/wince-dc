/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

Module Name:  

fatui.h

Abstract:  

This header file defines the functions, data structures, and string IDs
associated with the 'fatui' component of the Windows CE operating system.

Notes: 

Neither the 'fatui' component nor any of the following interfaces existed
in versions 1.0 or 2.0 of the Windows CE OS.  The 'fatui' was added after
those releases, to address the needs of a growing number of Windows CE-based
platforms that have UI capabilities and/or design requirements that vary
significantly from the earlier Windows CE-based systems.


--*/
#ifndef __FATUI_H__
#define __FATUI_H__

#ifdef __cplusplus
extern "C" {
#endif


// IDs for idsCaption

#define IDS_FATUI_ERROR                 101
#define IDS_FATUI_WARNING               102

// ID of string to be used by FATUIEvent in place of pszVol if pszVol is NULL

#define IDS_FATUI_VOLUME_NAME           103

// IDs for idsMessage;  new IDs may be added over time, and existing IDs may
// stop being issued over time, but no exising IDs will ever be reused, nor will
// the definitions of any of the accompanying UIPARAMS documented below be changed.

#define IDS_FATUI_COMPRESSED_WARNING    201
#define IDS_FATUI_UNCERTAIN_WARNING     202
#define IDS_FATUI_UNSUPPORTED_WARNING   203
#define IDS_FATUI_FORMATSTART_WARNING   204
#define IDS_FATUI_FORMATDONE_WARNING    205
#define IDS_FATUI_FORMATERROR_WARNING   206
#define IDS_FATUI_DIRTYVOL_WARNING      207
#define IDS_FATUI_RETRY_WARNING         208
#define IDS_FATUI_WRITEVERIFY_WARNING   209
#define IDS_FATUI_SCANSTART_WARNING     210
#define IDS_FATUI_SCANDONE_WARNING      211
#define IDS_FATUI_SCANERR_UNKNOWN       212
#define IDS_FATUI_SCANERR_DIR_BADDATA   213
#define IDS_FATUI_SCANERR_DIR_BADCLUS   214
#define IDS_FATUI_SCANERR_DIR_BADSIZE   215
#define IDS_FATUI_SCANERR_FAT_BADINDEX  216
#define IDS_FATUI_SCANERR_FAT_CROSSLINK 217
#define IDS_FATUI_SCANERR_FAT_WASTED    218

// IDs of strings that can appear in any auiParams entry marked UIPARAM_STRINGID

#define IDS_FATUI_HIDDEN_STRING         301
#define IDS_FATUI_SCANERR_REPAIR        302
#define IDS_FATUI_SCANERR_ADVISE        303


/*
    Examples of English text for each the above string IDs; the actual messages
    for the English FATUI component are subject to change.

    IDS_FATUI_ERROR:
        %s Error

    IDS_FATUI_WARNING:
        %s Warning

    IDS_FATUI_VOLUME_NAME:
        Storage Card

    IDS_FATUI_HIDDEN_STRING:
        hidden

    IDS_FATUI_COMPRESSED_WARNING:
        The %1!s! folder contains a %2!s!compressed volume file named %3!s!.
        The system cannot access files inside compressed volumes.
        If you do not need the compressed volume, select the file in the %1!s! folder and delete it.

        The first 3 UIPARAM entries supplied with this idsMessage are:
            1. Registered name of volume
            2. IDS_FATUI_HIDDEN_STRING or -1 (other string IDs may be added in the future)
            3. Name of compressed volume file

    IDS_FATUI_UNCERTAIN_WARNING:
        An error occurred while reading the %1!s! folder.
        If you are unable to use this storage card, try reinserting it, or try a different card.

        The first UIPARAM entry supplied with this idsMessage is:
            1. Registered name of volume

    IDS_FATUI_UNSUPPORTED_WARNING:
        The %1!s! folder contains data in an unsupported format.
        The system cannot access any information currently stored on the storage card.

        The first UIPARAM entry supplied with this idsMessage is:
            1. Registered name of volume

    IDS_FATUI_FORMATSTART_WARNING:
        Would you like the system to format the %1!s! folder so that the system can use it?
        WARNING: If you select Yes, any files in the %1!s! folder will be erased!

        The first UIPARAM entry supplied with this idsMessage is:
            1. Registered name of volume

    IDS_FATUI_FORMATDONE_WARNING:
        The %1!s! folder has been successfully formatted.
        It contains approximately %2!d!Mb of usable space.

        The first 2 UIPARAM entries supplied with this idsMessage are:
            1. Registered name of volume
            2. Number of megabytes available for use

    IDS_FATUI_FORMATERROR_WARNING:
        An error occurred while formatting the %1!s! folder (error code %2!d!).

        The first 2 UIPARAM entries supplied with this idsMessage are:
            1. Registered name of volume
            2. Error code from format operation

    IDS_FATUI_DIRTYVOL_WARNING:
        The system has not finished updating the files in the %1!s! folder.
        Please reinsert the storage card now.
        WARNING: Failure to do so may result in lost data on the storage card.

        The first UIPARAM entry supplied with this idsMessage is:
            1. Registered name of volume

    IDS_FATUI_RETRY_WARNING:
        The system encountered a storage card error (error code %1!d!).
        If a storage card was removed, please reinsert it,
        and then select Yes to retry or No to fail.
        WARNING: Failing this error may result in lost data.

        The first UIPARAM entry supplied with this idsMessage is:
            1. Error code from last operation

    IDS_FATUI_WRITEVERIFY_WARNING:
        There was an error verifying a write operation to sector %1!d!.
        The storage card may be faulty or unsupported (error code %2!d!).
        Do you want to ignore this error?"

        The first 2 UIPARAM entries supplied with this idsMessage are:
            1. First sector number of last write operation
            2. Error code from write operation

    IDS_FATUI_SCANSTART_WARNING:
        [No text displayed]

    IDS_FATUI_SCANDONE_WARNING:
        [No text displayed]

    IDS_FATUI_SCANERR_UNKNOWN:
        This storage card may contain errors.
        You should insert the card in a Windows desktop or laptop system and run SCANDISK to confirm and repair any problems.

    IDS_FATUI_SCANERR_DIR_BADDATA:
        The directory entry for '%2!s!' is not valid. %1!s!

        The first 3 UIPARAM entries supplied with this idsMessage are:
            1. A string ID (eg, IDS_FATUI_SCANERR_REPAIR or IDS_FATUI_SCANERR_ADVISE)
            2. Name of file
            3. Full path of directory containing file

    IDS_FATUI_SCANERR_DIR_BADCLUS:
        The directory entry for '%2!s!' contains an invalid cluster number. %1!s!

        The first 3 UIPARAM entries supplied with this idsMessage are:
            1. A string ID (eg, IDS_FATUI_SCANERR_REPAIR or IDS_FATUI_SCANERR_ADVISE)
            2. Name of file
            3. Full path of directory containing file

    IDS_FATUI_SCANERR_DIR_BADSIZE:
        The directory entry for '%2!s!' specifies an incorrect file size. %1!s!

        The first 3 UIPARAM entries supplied with this idsMessage are:
            1. A string ID (eg, IDS_FATUI_SCANERR_REPAIR or IDS_FATUI_SCANERR_ADVISE)
            2. Name of file
            3. Full path of directory containing file

    IDS_FATUI_SCANERR_FAT_BADINDEX:
        The cluster chain for '%2!s!' contains an invalid cluster number. %1!s!

        The first 3 UIPARAM entries supplied with this idsMessage are:
            1. A string ID (eg, IDS_FATUI_SCANERR_REPAIR or IDS_FATUI_SCANERR_ADVISE)
            2. Name of file
            3. Full path of directory containing file

    IDS_FATUI_SCANERR_FAT_CROSSLINK:
        The cluster chain for '%2!s!' refers to a cluster that is being used by another file. %1!s!

        The first 3 UIPARAM entries supplied with this idsMessage are:
            1. A string ID (eg, IDS_FATUI_SCANERR_REPAIR or IDS_FATUI_SCANERR_ADVISE)
            2. Name of file
            3. Full path of directory containing file

    IDS_FATUI_SCANERR_FAT_WASTED:
        There are %2!d! lost clusters in %3!d! chains.
        They will be saved in one or more FILEnnnn.CHK files in the %4!s! folder. %1!s!

        The first 4 UIPARAM entries supplied with this idsMessage are:
            1. A string ID (eg, IDS_FATUI_SCANERR_REPAIR or IDS_FATUI_SCANERR_ADVISE)
            2. Number of lost clusters
            3. Number of lost cluster chains
            4. Registered name of volume

    IDS_FATUI_SCANERR_REPAIR:
        Do you want this problem automatically repaired now?

    IDS_FATUI_SCANERR_ADVISE:
        The system is unable to repair this error.
        Select OK to continue checking for errors on this volume, or Cancel to stop.
 */


// Data structures for FATUIEvent()

#define UIPARAM_VALUE           0
#define UIPARAM_STRINGID        1

struct _UIPARAM {
    DWORD       dwType;         // one of UIPARAM_VALUE or UIPARAM_STRINGID
    DWORD       dwValue;        // value or string ID, as specified by dwType
};
typedef struct _UIPARAM UIPARAM, *PUIPARAM;

#define FATUI_NONE              0x00000000      // no action
#define FATUI_YES               0x00000001      // perform the specified action, if any
#define FATUI_NO                0x00000002      // do NOT perform the specified action, if any
#define FATUI_DELETE            0x00000004      // delete the specified file, if any
#define FATUI_CANCEL            0x00000008      // cancel the current operation, if any
#define FATUI_DEBUGMESSAGE      0x80000000

struct _FATUIDATA {
    DWORD       dwSize;         // set to sizeof(FATUIDATA)
    DWORD       dwFlags;        // zero or more FATUI_* flags
    DWORD       idsEvent;       // IDS_FATUI_* string resource ID of the event message
    DWORD       idsEventCaption;// IDS_FATUI_* string resource ID of the event message caption
    DWORD       cuiParams;      // count of UIPARAM entries
    UIPARAM     auiParams[4];   // zero or more UIPARAM entries
};
typedef struct _FATUIDATA FATUIDATA, *PFATUIDATA;

int FATUIEvent(
    HANDLE hModule,             // handle to module containing string resources
    PCWSTR pszVol,              // pointer to FAT volume name, NULL if none/unknown
    PFATUIDATA pfui             // pointer to FATUIDATA structure
);                              // returns exactly one of the FATUI response flags defined above

#ifdef __cplusplus
}
#endif

#endif  // __FATUI_H__
