/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1997-2000 Microsoft Corporation.  All rights reserved.

Component Name:

gweeadd.h

Abstract:  

This header file declares the function Initialize_AdditionalComponents().
If an additional components (beyond those that come with Windows CE) is added
to GWE via the cesysgen.bat file, and if that component needs to do some
initialization at boot time, then that components can implement this function.
GWE will call this function at boot time.
++*/

#ifdef __cplusplus
extern "C"
#endif
void Initialize_AdditionalComponents (void);
