/* __divlu.c (fulllibc:__divlu.obj) - unsigned 32-bit divide. SH-4 has no divide
 * instruction; the compiler-helper ABI is standard SH (verified vs the SDK
 * __divlu @ 0x8C03F2BC). Shift-subtract: emits no recursive __divlu call. */
typedef unsigned long u32;
u32 _divlu(u32 n, u32 d)
{
    u32 q = 0, r = 0; int i;
    for (i = 31; i >= 0; i--) {
        r = (r << 1) | ((n >> i) & 1u);
        if (r >= d) { r -= d; q |= (1ul << i); }
    }
    return q;
}
