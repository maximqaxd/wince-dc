/* i64div.c (fulllibc:i64div.obj) - signed 64-bit divide (__divi64). The 64-bit
 * shifts inside emit __lshi64/__rshui64 (lshi64.c/rshui64.c). */
typedef unsigned __int64 u64;
typedef __int64          s64;
s64 _divi64(s64 n, s64 d)
{
    int neg = (n < 0) ^ (d < 0);
    u64 un = (n < 0) ? (u64)(-n) : (u64)n;
    u64 ud = (d < 0) ? (u64)(-d) : (u64)d;
    u64 q = 0, r = 0; int i;
    for (i = 63; i >= 0; i--) {
        r = (r << 1) | ((un >> i) & 1);
        if (r >= ud) { r -= ud; q |= ((u64)1 << i); }
    }
    return neg ? -(s64)q : (s64)q;
}
