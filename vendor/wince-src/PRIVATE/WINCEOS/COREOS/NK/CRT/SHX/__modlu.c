/* __modlu.c (fulllibc:__modlu.obj) - unsigned 32-bit remainder. */
typedef unsigned long u32;
u32 _modlu(u32 n, u32 d)
{
    u32 r = 0; int i;
    for (i = 31; i >= 0; i--) {
        r = (r << 1) | ((n >> i) & 1u);
        if (r >= d) r -= d;
    }
    return r;
}
