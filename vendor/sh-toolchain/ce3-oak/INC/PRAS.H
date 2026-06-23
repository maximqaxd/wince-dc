/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

Module Name:  

pras.h

Abstract:

Contains the private portions of ras.h

Notes: 


--*/
    
#ifndef _PRIV_RAS_H_
#define _PRIV_RAS_H_

#ifdef __cplusplus
extern "C" {
#endif



#ifdef UNDER_CE
#define RNA_GETINFO		3

// Our main dialog has DWL_USER set to the following value.
#define RNAAPP_MAGIC_NUM 0x006A6D6D

#endif // UNDER_CE



#ifdef __cplusplus
}
#endif


#endif // _PRIV_RAS_H_
