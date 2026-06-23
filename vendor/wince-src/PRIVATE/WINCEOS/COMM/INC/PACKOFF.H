/*++

Copyright (c) 1990-2000 Microsoft Corporation.  All rights reserved.

Module Name:

    packoff.h

Abstract:

    This file turns packing of structures off.  (That is, it enables
    automatic alignment of structure fields.)  An include file is needed
    because various compilers do this in different ways.

    packoff.h is the complement to packon.h.  An inclusion of packoff.h
    MUST ALWAYS be preceded by an inclusion of packon.h, in one-to-one
    correspondence.

--*/

#if ! (defined(lint) || defined(_lint))

#ifndef VXD
#if i386 || defined(UNDER_CE)
#pragma warning(disable:4103)
#endif
#endif
#pragma pack()                  // x86, MS compiler; MIPS, MIPS compiler
#endif // ! (defined(lint) || defined(_lint))

