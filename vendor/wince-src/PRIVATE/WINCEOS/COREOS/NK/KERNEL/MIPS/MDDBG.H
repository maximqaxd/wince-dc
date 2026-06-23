// Copyright (c) 1999-2000 Microsoft Corporation.  All rights reserved.
extern void __asm(char[], ...);

void ThreadStopFunc(void)
{
     __asm("break 3"); 
}

