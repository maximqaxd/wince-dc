/* memmove.c (fulllibc:memmove.obj) - memcpy + memmove. */
#pragma function(memcpy)
void *memcpy(void *dst, const void *src, unsigned int n)
{
    char *d = (char *)dst; const char *s = (const char *)src;
    while (n--) *d++ = *s++;
    return dst;
}
void *memmove(void *dst, const void *src, unsigned int n)
{
    char *d = (char *)dst; const char *s = (const char *)src;
    if (d <= s) { while (n--) *d++ = *s++; }
    else { d += n; s += n; while (n--) *--d = *--s; }
    return dst;
}
