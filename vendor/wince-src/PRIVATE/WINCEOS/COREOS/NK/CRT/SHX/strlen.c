/* strlen.c (fulllibc:strlen.obj) */
#pragma function(strlen)
unsigned int strlen(const char *s)
{
    const char *p = s;
    while (*p) p++;
    return (unsigned int)(p - s);
}
