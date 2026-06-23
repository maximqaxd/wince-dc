/* memcmp.c - not in stock fulllibc; our addition (the kernel references it). */
#pragma function(memcmp)
int memcmp(const void *a, const void *b, unsigned int n)
{
    const unsigned char *x = (const unsigned char *)a, *y = (const unsigned char *)b;
    while (n--) { if (*x != *y) return (int)*x - (int)*y; x++; y++; }
    return 0;
}
