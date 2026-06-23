#ifndef _MATH_H_
#define _MATH_H_
//
// definition of _exception struct - this struct is passed to the matherr
// routine when a floating point exception is detected
struct _exception {
    int type;        /* exception type - see below */
    char *name;        /* name of function where error occured */
    double arg1;        /* first argument to function */
    double arg2;        /* second argument (if any) to function */
    double retval;        /* value to be returned by function */
    } ;

// Non-ANSI name for compatibility
#define exception _exception

#include "stdlib.h"

#endif // _MATH_H_
