/* lshi64.c (fulllibc:lshi64.obj) - 64-bit left shift (__lshi64), via 32-bit
 * halves so the body uses no 64-bit shift operator. SH-4 little-endian. */
typedef unsigned long    u32;
typedef unsigned __int64 u64;
typedef union { u64 q; struct { u32 lo, hi; } w; } U64;
u64 _lshi64(u64 v, int n)
{
    U64 in, out; in.q = v; n &= 63;
    if (n == 0)        out = in;
    else if (n < 32) { out.w.hi = (in.w.hi << n) | (in.w.lo >> (32 - n)); out.w.lo = in.w.lo << n; }
    else             { out.w.hi = in.w.lo << (n - 32); out.w.lo = 0; }
    return out.q;
}
