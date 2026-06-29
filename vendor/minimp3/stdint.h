//
// stdint.h - minimal C99 fixed-width types for the CE 2.12 SH-4 toolchain (which predates
// stdint.h). On the include path only for the music player / minimp3 build. SH-4 is ILP32:
// int = 32 bits, short = 16, char = 8, __int64 = 64.
//
#ifndef _DCWIN_STDINT_H
#define _DCWIN_STDINT_H

typedef signed char        int8_t;
typedef unsigned char      uint8_t;
typedef short              int16_t;
typedef unsigned short     uint16_t;
typedef int                int32_t;
typedef unsigned int       uint32_t;
typedef __int64            int64_t;
typedef unsigned __int64   uint64_t;
typedef int                intptr_t;
typedef unsigned int       uintptr_t;

#endif // _DCWIN_STDINT_H
