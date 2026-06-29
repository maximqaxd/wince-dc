//
// mp3impl.cpp - minimp3 implementation unit, compiled as C++ on purpose: minimp3 is written in
// C99 style (declarations after statements), which the CE 2.12 SH-4 cl.exe rejects in C (C89)
// but accepts in C++. The decoder's public API (mp3dec_init / mp3dec_decode_frame) is declared
// extern "C" inside minimp3.h, so the definitions get C linkage and link cleanly against the
// C player (dcwplay.c). minimp3 takes the generic scalar path on SH-4 (no SSE/NEON) and has no
// libm dependency.
//
#define MINIMP3_IMPLEMENTATION
#include "minimp3.h"
