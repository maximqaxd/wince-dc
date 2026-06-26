//
// arch/cc.h - lwIP compiler/architecture port for Dreamcast WinCE (SH-4, little
// endian) built with the Katana SDK SH compiler. lwIP 2.2.0.
//
#ifndef LWIP_ARCH_CC_H
#define LWIP_ARCH_CC_H

// The SDK has no stdint.h / inttypes.h - define lwIP's base types ourselves.
#define LWIP_NO_STDINT_H   1
#define LWIP_NO_INTTYPES_H 1

typedef unsigned char    u8_t;
typedef signed char      s8_t;
typedef unsigned short   u16_t;
typedef signed short     s16_t;
typedef unsigned int     u32_t;   // int is 32-bit on SH-4
typedef signed int       s32_t;
typedef u32_t            mem_ptr_t;

// The toolchain's stddef.h has size_t but not ptrdiff_t (lwIP uses it in casts).
#ifndef _PTRDIFF_T_DEFINED
typedef int ptrdiff_t;
#define _PTRDIFF_T_DEFINED
#endif

// printf-style format letters (no inttypes.h)
#define U16_F "hu"
#define S16_F "hd"
#define X16_F "hx"
#define U32_F "u"
#define S32_F "d"
#define X32_F "x"
#define SZT_F "u"

#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN 1234
#endif
#ifndef BIG_ENDIAN
#define BIG_ENDIAN    4321
#endif
#define BYTE_ORDER LITTLE_ENDIAN          // Dreamcast SH-4 runs little-endian

#define LWIP_CHKSUM_ALGORITHM 3           // portable C checksum

// Diagnostics/assert off for now (avoids C99 variadic-macro needs on this compiler)
#define LWIP_PLATFORM_DIAG(x)
#define LWIP_PLATFORM_ASSERT(x)

// Structure packing for protocol headers: use the bp/epstruct include form, which
// wraps packed structs in #pragma pack(1) ... #pragma pack().
#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_END
#define PACK_STRUCT_STRUCT
#define PACK_STRUCT_FIELD(x) x
#define PACK_STRUCT_USE_INCLUDES

#endif // LWIP_ARCH_CC_H
