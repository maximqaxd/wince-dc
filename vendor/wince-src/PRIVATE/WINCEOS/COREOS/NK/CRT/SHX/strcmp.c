/* strcmp.c (fulllibc:strcmp.obj) */
#pragma function(strcmp)
int strcmp(const char *a, const char *b)
{
    while (*a && *a == *b) { a++; b++; }
    return (int)(unsigned char)*a - (int)(unsigned char)*b;
}
