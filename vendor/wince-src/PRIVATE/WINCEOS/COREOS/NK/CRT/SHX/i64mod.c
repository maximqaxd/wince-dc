/* i64mod.c (fulllibc:i64mod.obj) - signed 64-bit remainder (__modi64). */
typedef unsigned __int64 u64;
typedef __int64          s64;
s64 _modi64(s64 n, s64 d)
{
    u64 un = (n < 0) ? (u64)(-n) : (u64)n;
    u64 ud = (d < 0) ? (u64)(-d) : (u64)d;
    u64 r = 0; int i;
    for (i = 63; i >= 0; i--) {
        r = (r << 1) | ((un >> i) & 1);
        if (r >= ud) r -= ud;
    }
    return (n < 0) ? -(s64)r : (s64)r;
}
