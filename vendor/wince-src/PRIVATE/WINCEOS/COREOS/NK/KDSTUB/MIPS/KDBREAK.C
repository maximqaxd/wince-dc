/*++

Copyright (c) 1990-2000 Microsoft Corporation.  All rights reserved.

Module Name:

    kdbreak.c

Abstract:

    This module implements machine dependent functions to add and delete
    breakpoints from the kernel debugger breakpoint table.

Revision History:

--*/

#include "kdp.h"

extern PROCESS *kdProcArray;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGEKD, KdpAddBreakpoint)
#pragma alloc_text(PAGEKD, KdpDeleteBreakpoint)
#pragma alloc_text(PAGEKD, KdpDeleteBreakpointRange)
#endif

#if defined(MIPS16SUPPORT)
ULONG KdpAddBreakpoint(IN PVOID Address)

/*++

Routine Description:

    This routine adds an entry to the breakpoint table and returns a handle
    to the breakpoint table entry.

Arguments:

    Address - Supplies the address where to set the breakpoint.

Return Value:

    A value of zero is returned if the specified address is already in the
    breakpoint table, there are no free entries in the breakpoint table, the
    specified address is not correctly aligned, or the specified address is
    not valid. Otherwise, the index of the assigned breakpoint table entry
    plus one is returned as the function value.

--*/

{

    ULONG Handle = 0;
    ULONG Index;
    KDP_BREAKPOINT_TYPE Content = 0;
    BOOLEAN Accessible = FALSE;
    KDP_BREAKPOINT_TYPE KContent = 0;
    BOOLEAN KAccessible = FALSE;
    PVOID KAddress = NULL;
    KDP_BREAKPOINT_TYPE Instruction = KDP_BREAKPOINT_32BIT_VALUE;
    ULONG Length = sizeof(KDP_BREAKPOINT_32BIT_TYPE);

    DEBUGGERMSG(KDZONE_BREAK,(L"Trying to set BP at %8.8lx\r\n", Address));
    
    //
    // The low order bit of the address is used to determine if the breakpoint
    // is in MIPS16 or MIPS32 code
    //

    if (((ULONG)Address & 1) != 0) {
        DEBUGGERMSG( KDZONE_BREAK,(L"MIPS16 breakpoint %8.8lx\r\n", Address));
        Length = sizeof(KDP_BREAKPOINT_16BIT_TYPE);
        Instruction = KDP_BREAKPOINT_16BIT_VALUE;
        Address = (PVOID) ((ULONG)Address & ~1);
    }
        
    //
    // If the specified address is not properly aligned, then return zero.
    //

    if (((ULONG)Address & (Length-1)) != 0) {
        DEBUGGERMSG(KDZONE_BREAK, (L"Address not aligned\r\n"));
        return 0;
    }

    if ( (((ulong)Address & 0x80000000) == 0) && ZeroPtr(Address) >= (ULONG)DllLoadBase) {
        DEBUGGERMSG( KDZONE_BREAK,(L"Is Dll %8.8lx ", Address));
        KAddress = (PVOID)(ZeroPtr(Address) + kdProcArray[0].dwVMBase);
        DEBUGGERMSG( KDZONE_BREAK,(L"converted to %8.8lx \r\n", KAddress));
    }

    //
    // Get the instruction to be replaced. If the instruction cannot be read,
    // then mark breakpoint as not accessible.
    //

    if (KdpMoveMemory(
            (PCHAR)&Content,
            (PCHAR)Address,
            Length ) != Length) {
        Accessible = FALSE;
    } else {
        DEBUGGERMSG(KDZONE_BREAK,(L"Successfully read %8.8lx at %8.8lx \r\n",
            Content, Address));
        Accessible = TRUE;
    }

    if (KAddress != NULL) {
        if (KdpMoveMemory(
                (PCHAR)&KContent,
                (PCHAR)KAddress,
                Length ) != Length) {
            KAccessible = FALSE;
        } else {
            DEBUGGERMSG(KDZONE_BREAK,(L"Successfully read %8.8lx at %8.8lx \r\n",
                Content, KAddress));
            KAccessible = TRUE;
        }
    }
    if (KAddress != NULL) {
        if (Content != KContent) {
            DEBUGGERMSG(KDZONE_BREAK,(L"Content %8.8lx != KContent at %8.8lx \r\n",
                Content, KContent, KAddress));
            if (!Content) {
                Content = KContent;
                DEBUGGERMSG(KDZONE_BREAK,(L"Set Content to %8.8lx \r\n", KContent));
            }
        }
    }

    //
    // Search the breakpoint table for a free entry and check if the specified
    // address is already in the breakpoint table.
    //

    if (Content == Instruction) {
        DEBUGGERMSG( KDZONE_BREAK,(L"Already found a BP %8.8lx \r\n",Address));

        for (Index = 0; Index < BREAKPOINT_TABLE_SIZE; Index += 1) {
            if (KdpBreakpointTable[Index].Address == Address ||
               (KAddress != NULL && KdpBreakpointTable[Index].KAddress == KAddress)) {
                Handle = Index + 1;
                DEBUGGERMSG( KDZONE_BREAK,(L"return Handle %d\r\n", Handle));
                return Handle;
            }
        }
    }
    for (Index = 0; Index < BREAKPOINT_TABLE_SIZE; Index += 1) {
        if (KdpBreakpointTable[Index].Flags == 0 ) {
            Handle = Index + 1;
            break;
        }
    }

    //
    // If a free entry was found, then write breakpoint and return the handle
    // value plus one. Otherwise, return zero.
    //

    if (Handle) {
        if ( Accessible || KAccessible) {
            //
            // If the specified address is not write accessible, then return zero.
            //
            if (!DbgVerify(Address, DV_SETBP)) {
                DEBUGGERMSG(KDZONE_BREAK, (L"Addresses not writable %8.8lx %8.8lx\r\n", 
                    Address, KAddress));
                Address = NULL;
            }
            if (KAddress != NULL && !DbgVerify(KAddress, DV_SETBP)) {
                DEBUGGERMSG(KDZONE_BREAK, (L"Addresses not writable %8.8lx %8.8lx\r\n", 
                    Address, KAddress));
                KAddress = NULL;
            }


            KdpBreakpointTable[Handle - 1].Address = Address;
            KdpBreakpointTable[Handle - 1].KAddress = KAddress;
            KdpBreakpointTable[Handle - 1].Content = Content;
            if (Length == sizeof(KDP_BREAKPOINT_16BIT_TYPE)) {
                KdpBreakpointTable[Handle - 1].Flags = KD_BREAKPOINT_IN_USE | KD_BREAKPOINT_16BIT;
            } else {
                KdpBreakpointTable[Handle - 1].Flags = KD_BREAKPOINT_IN_USE;
            }
                
            if (!bGlobalBreakPoint)
                KdpBreakpointTable[Handle - 1].pThrd = pCurThread;
            else
                KdpBreakpointTable[Handle - 1].pThrd = 0;

            if (Address != NULL) {
                if (KdpMoveMemory(
                        (PCHAR)Address,
                        (PCHAR)&Instruction,
                        Length) == Length
                ) {
                    DEBUGGERMSG(KDZONE_BREAK,(L"Successfully Set BP Handle %x %8.8lx\r\n", 
                        Handle - 1, Address));
                } else {
                    DEBUGGERMSG(KDZONE_BREAK,(L"Failed to Set BP Handle %x %8.8lx\r\n", 
                        Handle - 1, Address));
                }
            }
            if (KAddress != NULL) {
                if (KdpMoveMemory(
                        (PCHAR)KAddress,
                        (PCHAR)&Instruction,
                        Length) == Length
                ) {
                    DEBUGGERMSG(KDZONE_BREAK,(L"Successfully Set BP Handle %x %8.8lx\r\n", 
                        Handle - 1, KAddress));
                } else {
                    DEBUGGERMSG(KDZONE_BREAK,(L"Failed to Set BP Handle %x %8.8lx\r\n", 
                        Handle - 1, KAddress));
                }
            }
        } else {

            //
            // Breakpoint is not accessible
            //

            DEBUGGERMSG(KDZONE_BREAK, (L"Address not accessible\r\n"));
            return 0;
        }
    }
    return Handle;
}

BOOLEAN
KdpDeleteBreakpoint(IN ULONG Handle)

/*++

Routine Description:

    This routine deletes an entry from the breakpoint table.

Arguments:

    Handle - Supplies the index plus one of the breakpoint table entry
        which is to be deleted.

Return Value:

    A value of FALSE is returned if the specified handle is not a valid
    value or the breakpoint cannot be deleted because the old instruction
    cannot be replaced. Otherwise, a value of TRUE is returned.

--*/

{
    KDP_BREAKPOINT_TYPE Instruction = KDP_BREAKPOINT_32BIT_VALUE;
    ULONG Length = sizeof(KDP_BREAKPOINT_32BIT_TYPE);

    //
    // If the specified handle is not value, then return FALSE.
    //
    if ((Handle == 0) || (Handle > BREAKPOINT_TABLE_SIZE)) {
        DEBUGGERMSG(KDZONE_BREAK,(L"BP not a valid value %d\r\n", Handle));
        return FALSE;
    }
    Handle -= 1;

    DEBUGGERMSG(KDZONE_BREAK,(L"Removing BP Handle %d\r\n", Handle));
    //
    // If the specified breakpoint table is not valid, the return FALSE.
    //

    if (KdpBreakpointTable[Handle].Flags == 0) {
        DEBUGGERMSG(KDZONE_BREAK,(L"BP Handle already removed\r\n"));
        return FALSE;
    }

    //
    // Determine the breakpoint instruction and size
    //

    if (KdpBreakpointTable[Handle].Flags & KD_BREAKPOINT_16BIT) {
        Instruction = KDP_BREAKPOINT_16BIT_VALUE;
        Length = sizeof(KDP_BREAKPOINT_16BIT_TYPE);
    }
        

    //
    // If the breakpoint is suspended, just delete from the table
    //

    if (KdpBreakpointTable[Handle].Flags & KD_BREAKPOINT_SUSPENDED) {
        KdpBreakpointTable[Handle].Flags = 0;
        DEBUGGERMSG(KDZONE_BREAK,(L"BP Suspended, this should never happen\r\n"));
        return TRUE;
    }

    //
    // Replace the instruction contents.
    //

    if ((KdpBreakpointTable[Handle].Flags & KD_BREAKPOINT_IN_USE) &&
        (KdpBreakpointTable[Handle].Content == Instruction)) {
        DEBUGGERMSG(KDZONE_BREAK, (L"BP in USE and replacing BP %8.8lx\r\n", 
            KdpBreakpointTable[Handle].Address));
    }

    if (KdpBreakpointTable[Handle].Address != NULL) {
        if (KdpMoveMemory(
                (PCHAR)KdpBreakpointTable[Handle].Address,
                (PCHAR)&KdpBreakpointTable[Handle].Content,
                Length) == Length) {
            DEBUGGERMSG(KDZONE_BREAK, (L"Successfully restored BP %8.8lx\r\n", 
                KdpBreakpointTable[Handle].Address));
        } else {
            DEBUGGERMSG(KDZONE_BREAK, (L"Failed to restore BP %8.8lx\r\n", 
                KdpBreakpointTable[Handle].Address));
        }
    }
    if (KdpBreakpointTable[Handle].KAddress != NULL) {
        if (KdpMoveMemory(
                (PCHAR)KdpBreakpointTable[Handle].KAddress,
                (PCHAR)&KdpBreakpointTable[Handle].Content,
                Length) == Length) {
            DEBUGGERMSG(KDZONE_BREAK, (L"Successfully restored BP %8.8lx %8.8lx\r\n", 
                (PCHAR)&KdpBreakpointTable[Handle].Content, 
                KdpBreakpointTable[Handle].KAddress));
        } else {
            DEBUGGERMSG(KDZONE_BREAK, (L"Failed to restore BP %8.8lx\r\n", KdpBreakpointTable[Handle].KAddress));
        }
    }
    // Release lock on memory.
    if (KdpBreakpointTable[Handle].Address != NULL)
        DbgVerify((PCHAR)KdpBreakpointTable[Handle].Address, DV_CLEARBP);
    if (KdpBreakpointTable[Handle].KAddress != NULL)
        DbgVerify((PCHAR)KdpBreakpointTable[Handle].KAddress, DV_CLEARBP);

    //
    // Delete breakpoint table entry and return TRUE.
    //

    KdpBreakpointTable[Handle].Flags = 0;
    KdpBreakpointTable[Handle].Content = 0;
    KdpBreakpointTable[Handle].Address = NULL;
    KdpBreakpointTable[Handle].KAddress = NULL;

    return TRUE;
}

BOOLEAN
KdpDeleteBreakpointRange (
    IN PVOID Lower,
    IN PVOID Upper
    )

/*++

Routine Description:

    This routine deletes all breakpoints falling in a given range
    from the breakpoint table.

Arguments:

    Lower - inclusive lower address of range from which to remove BPs.

    Upper - include upper address of range from which to remove BPs.

Return Value:

    TRUE if any breakpoints removed, FALSE otherwise.

--*/

{
    KDP_BREAKPOINT_TYPE Instruction;
    ULONG Length;
    ULONG   Index;
    BOOLEAN ReturnStatus = FALSE;

    //
    // Examine each entry in the table in turn
    //

    for (Index = 0; Index < BREAKPOINT_TABLE_SIZE; Index++) {

        if ( ((KdpBreakpointTable[Index].Flags & KD_BREAKPOINT_IN_USE) &&
             ((KdpBreakpointTable[Index].Address >= Lower) &&
              (KdpBreakpointTable[Index].Address <= Upper))) ||
            ( KdpBreakpointTable[Index].KAddress != NULL &&
            ((KdpBreakpointTable[Index].Flags & KD_BREAKPOINT_IN_USE) &&
            ((KdpBreakpointTable[Index].KAddress >= Lower) &&
             (KdpBreakpointTable[Index].KAddress <= Upper))))
           ) {

            //
            // Breakpoint is in use and falls in range, clear it.
            //

            if (KdpBreakpointTable[Index].Flags & KD_BREAKPOINT_16BIT) {
                Instruction = KDP_BREAKPOINT_16BIT_VALUE;
                Length = sizeof(KDP_BREAKPOINT_16BIT_TYPE);
            } else {
                Instruction = KDP_BREAKPOINT_32BIT_VALUE;
                Length = sizeof(KDP_BREAKPOINT_32BIT_TYPE);
            }

            if (KdpBreakpointTable[Index].Content != Instruction) {

                if (KdpBreakpointTable[Index].Address != NULL)
                    KdpMoveMemory(
                        (PCHAR)KdpBreakpointTable[Index].Address,
                        (PCHAR)&KdpBreakpointTable[Index].Content,
                        Length
                        );
                if (KdpBreakpointTable[Index].KAddress != NULL)
                    KdpMoveMemory(
                        (PCHAR)KdpBreakpointTable[Index].KAddress,
                        (PCHAR)&KdpBreakpointTable[Index].Content,
                        Length
                        );
                // Release lock on memory.
                if (KdpBreakpointTable[Index].Address != NULL)
                    DbgVerify((PCHAR)KdpBreakpointTable[Index].Address, DV_CLEARBP);
                if (KdpBreakpointTable[Index].KAddress != NULL)
                    DbgVerify((PCHAR)KdpBreakpointTable[Index].KAddress, DV_CLEARBP);
            }

            KdpBreakpointTable[Index].Flags = 0;
            ReturnStatus = TRUE;
        }
    }
    return ReturnStatus;
}

#else
KDP_BREAKPOINT_TYPE Content;
KDP_BREAKPOINT_TYPE KContent;

ULONG KdpAddBreakpoint(IN PVOID Address)

/*++

Routine Description:

    This routine adds an entry to the breakpoint table and returns a handle
    to the breakpoint table entry.

Arguments:

    Address - Supplies the address where to set the breakpoint.

Return Value:

    A value of zero is returned if the specified address is already in the
    breakpoint table, there are no free entries in the breakpoint table, the
    specified address is not correctly aligned, or the specified address is
    not valid. Otherwise, the index of the assigned breakpoint table entry
    plus one is returned as the function value.

--*/

{

    ULONG Handle = 0;
    ULONG Index;
    BOOLEAN Accessible = FALSE;
    BOOLEAN KAccessible = FALSE;
    PVOID KAddress = NULL;

    Content = 0;
    KContent = 0;
    //
    // If the specified address is not properly aligned, then return zero.
    //

    DEBUGGERMSG(KDZONE_BREAK,(L"Trying to set BP at %8.8lx\r\n", Address));

    if (((ULONG)Address & KDP_BREAKPOINT_ALIGN) != 0) {
        DEBUGGERMSG(KDZONE_BREAK, (L"Address not aligned\r\n"));
        return 0;
    }

#if !defined(x86)
    if ( (((ulong)Address & 0x80000000) == 0) && ZeroPtr(Address) >= (ULONG)DllLoadBase) {
        DEBUGGERMSG( KDZONE_BREAK,(L"Is Dll %8.8lx ", Address));
        KAddress = (PVOID)(ZeroPtr(Address) + kdProcArray[0].dwVMBase);
        DEBUGGERMSG( KDZONE_BREAK,(L"converted to %8.8lx \r\n", KAddress));
    }
#endif
    //
    // Get the instruction to be replaced. If the instruction cannot be read,
    // then mark breakpoint as not accessible.
    //

    if (KdpMoveMemory(
            (PCHAR)&Content,
            (PCHAR)Address,
            sizeof(KDP_BREAKPOINT_TYPE) ) != sizeof(KDP_BREAKPOINT_TYPE)) {
        Accessible = FALSE;
    } else {
        DEBUGGERMSG(KDZONE_BREAK,(L"Successfully read %8.8lx at %8.8lx \r\n",
            Content, Address));
        Accessible = TRUE;
    }

    if (KAddress != NULL) {
        if (KdpMoveMemory(
                (PCHAR)&KContent,
                (PCHAR)KAddress,
                sizeof(KDP_BREAKPOINT_TYPE) ) != sizeof(KDP_BREAKPOINT_TYPE)) {
            KAccessible = FALSE;
        } else {
            DEBUGGERMSG(KDZONE_BREAK,(L"Successfully read %8.8lx at %8.8lx \r\n",
                Content, KAddress));
            KAccessible = TRUE;
        }
    }
    if (KAddress != NULL) {
        if (Content != KContent) {
            // assert(FALSE);
            DEBUGGERMSG(KDZONE_BREAK,(L"Content %8.8lx != KContent at %8.8lx \r\n",
                Content, KContent, KAddress));
            if (!Content) {
                Content = KContent;
                DEBUGGERMSG(KDZONE_BREAK,(L"Set Content to %8.8lx \r\n", KContent));
            }
        }
    }

    //
    // Search the breakpoint table for a free entry and check if the specified
    // address is already in the breakpoint table.
    //

    if (Content == KdpBreakpointInstruction) {
        DEBUGGERMSG( KDZONE_BREAK,(L"Already found a BP %8.8lx \r\n",Address));

        for (Index = 0; Index < BREAKPOINT_TABLE_SIZE; Index += 1) {
            if (KdpBreakpointTable[Index].Address == Address ||
               (KAddress != NULL && KdpBreakpointTable[Index].KAddress == KAddress)) {
                Handle = Index + 1;
                DEBUGGERMSG( KDZONE_BREAK,(L"return Handle %d\r\n", Handle));
                return Handle;
            }
        }
    }
    for (Index = 0; Index < BREAKPOINT_TABLE_SIZE; Index += 1) {
        if (KdpBreakpointTable[Index].Flags == 0 ) {
            Handle = Index + 1;
            break;
        }
    }

    //
    // If a free entry was found, then write breakpoint and return the handle
    // value plus one. Otherwise, return zero.
    //

    if (Handle) {
        if ( Accessible || KAccessible) {
            //
            // If the specified address is not write accessible, then return zero.
            //
            if (!DbgVerify(Address, DV_SETBP)) {
                DEBUGGERMSG(KDZONE_BREAK, (L"Addresses not writable %8.8lx %8.8lx\r\n", 
                    Address, KAddress));
                Address = NULL;
            }
            if (KAddress != NULL && !DbgVerify(KAddress, DV_SETBP)) {
                DEBUGGERMSG(KDZONE_BREAK, (L"Addresses not writable %8.8lx %8.8lx\r\n", 
                    Address, KAddress));
                KAddress = NULL;
            }


            KdpBreakpointTable[Handle - 1].Address = Address;
            KdpBreakpointTable[Handle - 1].KAddress = KAddress;
            KdpBreakpointTable[Handle - 1].Content = Content;
            KdpBreakpointTable[Handle - 1].Flags = KD_BREAKPOINT_IN_USE;
            if (!bGlobalBreakPoint)
                KdpBreakpointTable[Handle - 1].pThrd = pCurThread;
            else
                KdpBreakpointTable[Handle - 1].pThrd = 0;

            if (Address != NULL) {
                if (KdpMoveMemory(
                        (PCHAR)Address,
                        (PCHAR)&KdpBreakpointInstruction,
                        sizeof(KDP_BREAKPOINT_TYPE)
                        ) == sizeof(KDP_BREAKPOINT_TYPE)) {
                    DEBUGGERMSG(KDZONE_BREAK,(L"Successfully Set BP Handle %x %8.8lx\r\n", 
                        Handle - 1, Address));
                } else {
                    DEBUGGERMSG(KDZONE_BREAK,(L"Failed to Set BP Handle %x %8.8lx\r\n", 
                        Handle - 1, Address));
                }
            }
            if (KAddress != NULL) {
                if (KdpMoveMemory(
                        (PCHAR)KAddress,
                        (PCHAR)&KdpBreakpointInstruction,
                        sizeof(KDP_BREAKPOINT_TYPE)
                        ) == sizeof(KDP_BREAKPOINT_TYPE)
                ) {
                    DEBUGGERMSG(KDZONE_BREAK,(L"Successfully Set BP Handle %x %8.8lx\r\n", 
                        Handle - 1, KAddress));
                } else {
                    DEBUGGERMSG(KDZONE_BREAK,(L"Failed to Set BP Handle %x %8.8lx\r\n", 
                        Handle - 1, KAddress));
                }
            }
        } else {

            //
            // Breakpoint is not accessible
            //

            DEBUGGERMSG(KDZONE_BREAK, (L"Address not accessible\r\n"));
            return 0;
        }
    }
    return Handle;
}

BOOLEAN
KdpDeleteBreakpoint(IN ULONG Handle)

/*++

Routine Description:

    This routine deletes an entry from the breakpoint table.

Arguments:

    Handle - Supplies the index plus one of the breakpoint table entry
        which is to be deleted.

Return Value:

    A value of FALSE is returned if the specified handle is not a valid
    value or the breakpoint cannot be deleted because the old instruction
    cannot be replaced. Otherwise, a value of TRUE is returned.

--*/

{

    //
    // If the specified handle is not value, then return FALSE.
    //
    if ((Handle == 0) || (Handle > BREAKPOINT_TABLE_SIZE)) {
        DEBUGGERMSG(KDZONE_BREAK,(L"BP not a valid value %d\r\n", Handle));
        return FALSE;
    }
    Handle -= 1;

    DEBUGGERMSG(KDZONE_BREAK,(L"Removing BP Handle %d\r\n", Handle));
    //
    // If the specified breakpoint table is not valid, the return FALSE.
    //

    if (KdpBreakpointTable[Handle].Flags == 0) {
        DEBUGGERMSG(KDZONE_BREAK,(L"BP Handle already removed\r\n"));
        return FALSE;
    }

    //
    // If the breakpoint is suspended, just delete from the table
    //

    if (KdpBreakpointTable[Handle].Flags & KD_BREAKPOINT_SUSPENDED) {
        KdpBreakpointTable[Handle].Flags = 0;
        DEBUGGERMSG(KDZONE_BREAK,(L"BP Suspended, this should never happen\r\n"));
        return TRUE;
    }

    //
    // Replace the instruction contents.
    //

    if ((KdpBreakpointTable[Handle].Flags & KD_BREAKPOINT_IN_USE) &&
        (KdpBreakpointTable[Handle].Content == KdpBreakpointInstruction)) {
        DEBUGGERMSG(KDZONE_BREAK, (L"BP in USE and replacing BP %8.8lx\r\n", 
            KdpBreakpointTable[Handle].Address));
    }

    if (KdpBreakpointTable[Handle].Address != NULL) {
        if (KdpMoveMemory(
                (PCHAR)KdpBreakpointTable[Handle].Address,
                (PCHAR)&KdpBreakpointTable[Handle].Content,
                sizeof(KDP_BREAKPOINT_TYPE)) == sizeof(KDP_BREAKPOINT_TYPE)) {
            DEBUGGERMSG(KDZONE_BREAK, (L"Successfully restored BP %8.8lx\r\n", 
                KdpBreakpointTable[Handle].Address));
        } else {
            DEBUGGERMSG(KDZONE_BREAK, (L"Failed to restore BP %8.8lx\r\n", 
                KdpBreakpointTable[Handle].Address));
        }
    }
    if (KdpBreakpointTable[Handle].KAddress != NULL) {
        if (KdpMoveMemory(
                (PCHAR)KdpBreakpointTable[Handle].KAddress,
                (PCHAR)&KdpBreakpointTable[Handle].Content,
                sizeof(KDP_BREAKPOINT_TYPE)) == sizeof(KDP_BREAKPOINT_TYPE)) {
            DEBUGGERMSG(KDZONE_BREAK, (L"Successfully restored BP %8.8lx %8.8lx\r\n", 
                (PCHAR)&KdpBreakpointTable[Handle].Content, 
                KdpBreakpointTable[Handle].KAddress));
        } else {
            DEBUGGERMSG(KDZONE_BREAK, (L"Failed to restore BP %8.8lx\r\n", KdpBreakpointTable[Handle].KAddress));
        }
    }
    // Release lock on memory.
//    if (KdpBreakpointTable[Handle].Address != NULL)
//        DbgVerify((PCHAR)KdpBreakpointTable[Handle].Address, DV_CLEARBP);
    if (KdpBreakpointTable[Handle].KAddress != NULL)
        DbgVerify((PCHAR)KdpBreakpointTable[Handle].KAddress, DV_CLEARBP);

    //
    // Delete breakpoint table entry and return TRUE.
    //

    KdpBreakpointTable[Handle].Flags = 0;
    KdpBreakpointTable[Handle].Content = 0;
    KdpBreakpointTable[Handle].Address = NULL;
    KdpBreakpointTable[Handle].KAddress = NULL;

    return TRUE;
}

BOOLEAN
KdpDeleteBreakpointRange (
    IN PVOID Lower,
    IN PVOID Upper
    )

/*++

Routine Description:

    This routine deletes all breakpoints falling in a given range
    from the breakpoint table.

Arguments:

    Lower - inclusive lower address of range from which to remove BPs.

    Upper - include upper address of range from which to remove BPs.

Return Value:

    TRUE if any breakpoints removed, FALSE otherwise.

--*/

{
    ULONG   Index;
    BOOLEAN ReturnStatus = FALSE;

    //
    // Examine each entry in the table in turn
    //

    for (Index = 0; Index < BREAKPOINT_TABLE_SIZE; Index++) {

        if ( ((KdpBreakpointTable[Index].Flags & KD_BREAKPOINT_IN_USE) &&
             ((KdpBreakpointTable[Index].Address >= Lower) &&
              (KdpBreakpointTable[Index].Address <= Upper))) ||
            ( KdpBreakpointTable[Index].KAddress != NULL &&
            ((KdpBreakpointTable[Index].Flags & KD_BREAKPOINT_IN_USE) &&
            ((KdpBreakpointTable[Index].KAddress >= Lower) &&
             (KdpBreakpointTable[Index].KAddress <= Upper))))
           ) {

            //
            // Breakpiont is in use and falls in range, clear it.
            //

            if (KdpBreakpointTable[Index].Content != KdpBreakpointInstruction) {

                if (KdpBreakpointTable[Index].Address != NULL)
                    KdpMoveMemory(
                        (PCHAR)KdpBreakpointTable[Index].Address,
                        (PCHAR)&KdpBreakpointTable[Index].Content,
                        sizeof(KDP_BREAKPOINT_TYPE)
                        );
                if (KdpBreakpointTable[Index].KAddress != NULL)
                    KdpMoveMemory(
                        (PCHAR)KdpBreakpointTable[Index].KAddress,
                        (PCHAR)&KdpBreakpointTable[Index].Content,
                        sizeof(KDP_BREAKPOINT_TYPE)
                        );
                // Release lock on memory.
//                if (KdpBreakpointTable[Index].Address != NULL)
//                    DbgVerify((PCHAR)KdpBreakpointTable[Index].Address, DV_CLEARBP);
                if (KdpBreakpointTable[Index].KAddress != NULL)
                    DbgVerify((PCHAR)KdpBreakpointTable[Index].KAddress, DV_CLEARBP);
            }

            KdpBreakpointTable[Index].Flags = 0;
            ReturnStatus = TRUE;
        }
    }
    return ReturnStatus;
}
#endif
