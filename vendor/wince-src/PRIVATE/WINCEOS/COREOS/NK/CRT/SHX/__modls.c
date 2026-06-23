/* __modls.c (fulllibc:__modls.obj) - signed 32-bit remainder (sign of dividend). */
typedef unsigned long u32;
typedef long          s32;
s32 _modls(s32 n, s32 d)
{
    u32 un = (n < 0) ? (u32)(-n) : (u32)n;
    u32 ud = (d < 0) ? (u32)(-d) : (u32)d;
    u32 r = 0; int i;
    for (i = 31; i >= 0; i--) {
        r = (r << 1) | ((un >> i) & 1u);
        if (r >= ud) r -= ud;
    }
    return (n < 0) ? -(s32)r : (s32)r;
}
