/* rshui64.c (fulllibc:rshui64.obj) - 64-bit unsigned right shift (__rshui64). */
typedef unsigned long    u32;
typedef unsigned __int64 u64;
typedef union { u64 q; struct { u32 lo, hi; } w; } U64;
u64 _rshui64(u64 v, int n)
{
    U64 in, out; in.q = v; n &= 63;
    if (n == 0)        out = in;
    else if (n < 32) { out.w.lo = (in.w.lo >> n) | (in.w.hi << (32 - n)); out.w.hi = in.w.hi >> n; }
    else             { out.w.lo = in.w.hi >> (n - 32); out.w.hi = 0; }
    return out.q;
}
