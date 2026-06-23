//*****************************************************************************
// WARNING: USE OF THIS SOURCE CODE IS RESTRICTED TO USE IN WINDOWS-CE BASED
//          SYSTEMS.  NO OTHER USE OF THIS CODE IS PERMITTED SUBJECT TO
//          LEGAL RESTRICTIONS. 
//*****************************************************************************

/*   
      Rev 1.0   22 Feb 1998 15:32:16
   Initial revision.
 */

/************************************************************************/
/*                                                                      */
/*		FAT-FTL Lite Software Development Kit			*/
/*              Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.  */
/*              Copyright (C) M-Systems Ltd. 1995-1998                  */
/*									*/
/************************************************************************/

#ifndef FLSTATUS_H
#define FLSTATUS_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef IFLITE_ERROR_CODES
typedef enum {                          /* Status code for operation.
					   A zero value indicates success,
					   other codes are the extended
					   DOS codes. */
             flOK                  = 0,
             flBadFunction         = 1,
             flFileNotFound        = 2,
             flPathNotFound        = 3,
             flTooManyOpenFiles    = 4,
             flNoWriteAccess       = 5,
             flBadFileHandle       = 6,
             flDriveNotAvailable   = 9,
             flNonFATformat        = 10,
             flFormatNotSupported  = 11,
             flNoMoreFiles         = 18,
             flWriteProtect        = 19,
             flBadDriveHandle      = 20,
             flDriveNotReady       = 21,
             flUnknownCmd          = 22,
             flBadFormat           = 23,
             flBadLength           = 24,
             flDataError           = 25,
             flUnknownMedia        = 26,
             flSectorNotFound      = 27,
             flOutOfPaper          = 28,
             flWriteFault          = 29,
             flReadFault           = 30,
             flGeneralFailure      = 31,
             flDiskChange          = 34,
             flVppFailure          = 50,
             flBadParameter        = 51,
             flNoSpaceInVolume     = 52,
             flInvalidFATchain     = 53,
             flRootDirectoryFull   = 54,
             flNotMounted          = 55,
	     flPathIsRootDirectory = 56,
             flNotADirectory       = 57,
             flDirectoryNotEmpty   = 58,
             flFileIsADirectory    = 59,
             flAdapterNotFound     = 60,
             flFormattingError     = 62,
             flNotEnoughMemory     = 63,
             flVolumeTooSmall      = 64,
             flBufferingError      = 65,
             flFileAlreadyExists   = 80,
             flIncomplete          = 100,
             flTimedOut            = 101,
             flTooManyComponents   = 102
#else

#include "type.h"

typedef enum {                          /* Status code for operation.
					   A zero value indicates success,
					   other codes are the extended
					   DOS codes. */
             flOK                  = ERR_NONE,
             flBadFunction         = ERR_SW_HW,
             flFileNotFound        = ERR_NOTEXISTS,
             flPathNotFound        = ERR_NOTEXISTS,
             flTooManyOpenFiles    = ERR_MAX_FILES,
             flNoWriteAccess       = ERR_WRITE,
             flBadFileHandle       = ERR_NOTOPEN,
             flDriveNotAvailable   = ERR_SW_HW,
             flNonFATformat        = ERR_PARTITION,
             flFormatNotSupported  = ERR_PARTITION,
             flNoMoreFiles         = ERR_NOTEXISTS,
             flWriteProtect        = ERR_WRITE,
             flBadDriveHandle      = ERR_SW_HW,
             flDriveNotReady       = ERR_PARTITION,
             flUnknownCmd          = ERR_PARAM,
             flBadFormat           = ERR_PARTITION,
             flBadLength           = ERR_SW_HW,
             flDataError           = ERR_READ,
             flUnknownMedia        = ERR_PARTITION,
             flSectorNotFound      = ERR_READ,
             flOutOfPaper          = ERR_SW_HW,
             flWriteFault          = ERR_WRITE,
             flReadFault           = ERR_READ,
             flGeneralFailure      = ERR_SW_HW,
             flDiskChange          = ERR_PARTITION,
             flVppFailure          = ERR_WRITE,
             flBadParameter        = ERR_PARAM,
             flNoSpaceInVolume     = ERR_SPACE,
             flInvalidFATchain     = ERR_PARTITION,
             flRootDirectoryFull   = ERR_DIRECTORY,
             flNotMounted          = ERR_PARTITION,
	     flPathIsRootDirectory = ERR_DIRECTORY,
             flNotADirectory       = ERR_DIRECTORY,
             flDirectoryNotEmpty   = ERR_NOT_EMPTY,
             flFileIsADirectory    = ERR_DIRECTORY,
             flAdapterNotFound     = ERR_DETECT,
             flFormattingError     = ERR_FORMAT,
             flNotEnoughMemory     = ERR_SW_HW,
             flVolumeTooSmall      = ERR_FORMAT,
             flBufferingError      = ERR_SW_HW,
             flFileAlreadyExists   = ERR_EXISTS,
             flIncomplete          = ERR_DETECT,
             flTimedOut            = ERR_SW_HW,
             flTooManyComponents   = ERR_PARAM
#endif
	     } FLStatus;

#ifdef __cplusplus
}
#endif

#endif /* FLSTATUS_H */
