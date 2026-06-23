//------------------------------------------------------------------------------
//
//  Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.
//  
//------------------------------------------------------------------------------
#include <windows.h>

void CeLogThreadMigrate(HANDLE hProcess, DWORD dwReserved);

void
CELOG_ThreadMigrateARM(HANDLE hProcess)
{
    CeLogThreadMigrate(hProcess, 0);
}

