/*****************************************************************************
*
*  Copyright (c) 1993-2000 Microsoft Corporation.  All rights reserved.
*
*  @doc		EXTERNAL LINKLIST
*
*  @module	LinkList.h | 
*
*  @comm	This file defines the LinkList functions used by the
*			Communications team.
*
*  @topic	Linked Listed Functions Overview |
*
*		The Linked List functions provide a standard way to use
*		Linked Lists.
*
*		<f InitializeListHead>		Initialize a list head
*
*		<f IsListEmpty>			Check if list is empty
*
*		<f RemoveHeadList>		Remove the first entry in a list
*
*		<f RemoveHeadLockedList>	Remove the first entry from a locked list
*
*		<f RemoveEntryList>		Remove an entry from a list
*
*		<f RemoveEntryLockedList>	Remove an entry from a locked list
*
*		<f RemoveTailList>		Remove the last entry from a list
*
*		<f RemoveTailLockedList>	Remove the last entry from a locked list
*
*		<f InsertTailList>			Insert at the tail of a list
*
*		<f InsertTailLockedList>		Insert at the tail of a locked list
*
*		<f InsertHeadList>		Insert at the head of a list
*
*		<f InsertHeadLockedList>		Insert at the head of a locked list
*
*
*/

/*
    linklist.h

    Macros for linked-list manipulation.


*/


#ifndef _LINKLIST_H_
#define _LINKLIST_H_

// Since this is in multiple modules it needs to be defined outside of
// the DEBUG Zone stuff.  Otherwise each would have to have
// a zone dedicated to debugging linked lists
#ifndef DEBUG_LINK_LIST
#define DEBUG_LINK_LIST	0
#endif


/*NOINC*/

#if !defined(WIN32)


/*****************************************************************************
*
*
*	@struct	LIST_ENTRY	| The Linked List Structure
*
*	@comm	This is the List Entry structure.

*	@ex		An example of how to use this function follows |
*			No Example
*
*/
typedef struct _LIST_ENTRY {
    struct _LIST_ENTRY FAR * Flink;	//@field	The Pointer to next
    struct _LIST_ENTRY FAR * Blink;	//@field	The Pointer to the previous
} LIST_ENTRY;
typedef LIST_ENTRY FAR * PLIST_ENTRY;
 
#endif  // !WIN32

//
// Linked List Manipulation Functions - from NDIS.H
//

// Calculate the address of the base of the structure given its type, and an
// address of a field within the structure. - from NDIS.H

#ifndef CONTAINING_RECORD
#if defined(WIN32) || defined(UNDER_CE)
#define CONTAINING_RECORD(address, type, field) ((type *)( \
                          (LPBYTE)(address) - \
                          (LPBYTE)(&((type *)0)->field)))
#else   // !WIN32
#define CONTAINING_RECORD(address, type, field) ((type FAR *)( \
                          MAKELONG( \
                              ((LPBYTE)(address) - \
                               (LPBYTE)(&((type FAR *)0)->field)), \
                              SELECTOROF(address))))
#endif  // WIN32
#endif  // CONTAINING_RECORD

//  Doubly-linked list manipulation routines.  Implemented as macros

/*****************************************************************************
*
*
*	@func	void 	|	InitializeListHead | Initialize a Linked List Head
*
*	@rdesc	No return
*
*	@parm	PLIST_ENTRY	|	ListHead	| The head of the list
*
*	@comm
*			This will initialize the list head.
*
*	@ex		An example of how to use this function follows |
*			No Example
*
*/
#define InitializeListHead(ListHead) \
    ((ListHead)->Flink = (ListHead)->Blink = (ListHead) )

/*****************************************************************************
*
*
*	@func	BOOL 	|	IsListEmpty	|	Is the list Emtpy?
*
*	@rdesc	Boolean TRUE if the list is empty, FALSE if not empty.
*
*	@parm	PLIST_ENTRY	|	ListHead	| The head of the list
*
*	@comm
*			This function will test if the list is empty.
*
*	@ex		An example of how to use this function follows |
*			No Example
*
*/
#define IsListEmpty(ListHead) \
    (( ((ListHead)->Flink == (ListHead)) ? TRUE : FALSE ) )

/*****************************************************************************
*
*
*	@func	PLIST_ENTRY	|	RemoveHeadList	|
*			Remove the head of the list
*
*	@rdesc	Returns the entry from the head of the list.
*
*	@parm	PLIST_ENTRY	|	ListHead	| The head of the list
*
*	@comm
*			If this is called on an empty list it will return a pointer to
*			the list head (probably not what you wanted).
*
*	@ex		An example of how to use this function follows |
*			No Example
*
*/

_inline
PLIST_ENTRY
RemoveHeadList(PLIST_ENTRY ListHead)
{
	PLIST_ENTRY	FirstEntry;

	DEBUGMSG(DEBUG_LINK_LIST,
			 (TEXT("+RemoveHeadList: Head=%x(%x,%x)\r\n"),
			  ListHead, ListHead->Flink, ListHead->Blink));

	FirstEntry               = ListHead->Flink;
	FirstEntry->Flink->Blink = (ListHead);
	ListHead->Flink          = FirstEntry->Flink;

	DEBUGMSG(DEBUG_LINK_LIST,
			 (TEXT("-RemoveHeadList: %x Head=%x(%x,%x)\r\n"),
			  FirstEntry, ListHead, ListHead->Flink, ListHead->Blink));

	return FirstEntry;
}

/*****************************************************************************
*
*
*	@func	PLIST_ENTRY	|	RemoveHeadLockedList	|
*			Remove the first entry of a locked list
*
*	@rdesc	Returns the entry from the head of the list.
*
*	@parm	PLIST_ENTRY	|	ListHead	| The head of the list
*	@parm	LPCRITICAL_SECTION	|	CS	| The critical section
*
*	@comm
*			If this is called on an empty list it will return a pointer to
*			the list head (probably not what you wanted).
*
*	@ex		An example of how to use this function follows |
*			No Example
*
*/

_inline
PLIST_ENTRY
RemoveHeadLockedList(PLIST_ENTRY ListHead, LPCRITICAL_SECTION CS)
{
	PLIST_ENTRY	FirstEntry;

	DEBUGMSG(DEBUG_LINK_LIST,
			 (TEXT("+RemoveHeadLockedList: Head=%x(%x,%x) CS=%x\r\n"),
			  ListHead, ListHead->Flink, ListHead->Blink, CS));

	EnterCriticalSection(CS);

	FirstEntry               = ListHead->Flink;
	FirstEntry->Flink->Blink = (ListHead);
	ListHead->Flink          = FirstEntry->Flink;

	DEBUGMSG(DEBUG_LINK_LIST,
			 (TEXT("-RemoveHeadLockedList: %x Head=%x(%x,%x)\r\n"),
			  FirstEntry, ListHead, ListHead->Flink, ListHead->Blink));

	LeaveCriticalSection(CS);

	return FirstEntry;
}

/*****************************************************************************
*
*
*	@func	void	|	RemoveEntryList | Removes an entry from the list
*
*	@rdesc	No return
*
*	@parm	PLIST_ENTRY	|	Entry	| The entry to remove
*
*	@comm
*			This function will remove the specified entry from a list.
*
*	@ex		An example of how to use this function follows |
*			No Example
*
*/

_inline
void
RemoveEntryList(PLIST_ENTRY Entry)
{
	PLIST_ENTRY EX_Entry;
	
	DEBUGMSG(DEBUG_LINK_LIST, (TEXT("+RemoveEntryList: Entry=%x(%x,%x)\r\n"),
                               Entry, Entry->Flink, Entry->Blink));

    EX_Entry = (Entry);
    EX_Entry->Blink->Flink = EX_Entry->Flink;
    EX_Entry->Flink->Blink = EX_Entry->Blink;

	DEBUGMSG(DEBUG_LINK_LIST, (TEXT("-RemoveEntryList: \r\n")));
}

/*****************************************************************************
*
*
*	@func	void	|	RemoveEntryLockedList | Removes an entry from the list
*
*	@rdesc	No return
*
*	@parm	PLIST_ENTRY			|	Entry	| The entry to remove
*	@parm	LPCRITICAL_SECTION	|	CS		| The critical section to lock
*
*	@comm
*			This function will remove the specified entry from a list.
*
*	@ex		An example of how to use this function follows |
*			No Example
*
*/

_inline
void
RemoveEntryLockedList(PLIST_ENTRY Entry, LPCRITICAL_SECTION CS)
{
	PLIST_ENTRY EX_Entry;
	
	DEBUGMSG(DEBUG_LINK_LIST,
			 (TEXT("+RemoveEntryLockedList: Entry=%x(%x,%x) CS=%x\r\n"),
			  Entry, Entry->Flink, Entry->Blink, CS));

	EnterCriticalSection(CS);

    EX_Entry               = (Entry);
    EX_Entry->Blink->Flink = EX_Entry->Flink;
    EX_Entry->Flink->Blink = EX_Entry->Blink;

	DEBUGMSG(DEBUG_LINK_LIST, (TEXT("-RemoveEntryLockedList: \r\n")));

	LeaveCriticalSection(CS);
}

/*****************************************************************************
*
*
*	@func	PLIST_ENTRY	|	RemoveTailList	|
*			This function will remove the entry from the end of a list.
*
*	@rdesc	Returns a pointer to the tail entry.
*
*	@parm	PLIST_ENTRY	|	ListHead	| The List Head
*
*	@comm
*			Don't call this with a null list.
*
*	@ex		An example of how to use this function follows |
*			No Example
*
*/

_inline
PLIST_ENTRY
RemoveTailList(PLIST_ENTRY ListHead)
{
	PLIST_ENTRY _EX_Entry;

	DEBUGMSG(DEBUG_LINK_LIST, (TEXT("+RemoveTailList: ListHead=%x(%x,%x)\r\n"),
                               ListHead, ListHead->Flink, ListHead->Blink));

	_EX_Entry = ListHead->Blink;
	RemoveEntryList(_EX_Entry);

	DEBUGMSG(DEBUG_LINK_LIST, (TEXT("-RemoveTailList: ListHead=%x(%x,%x)\r\n"),
				 ListHead, ListHead->Flink, ListHead->Blink));

	return _EX_Entry;
}

/*****************************************************************************
*
*
*	@func	PLIST_ENTRY	|	RemoveTailLockedList	|
*			This function will remove the entry from the end of a list.
*
*	@rdesc	Returns a pointer to the tail entry.
*
*	@parm	PLIST_ENTRY	|	ListHead	| The List Head
*	@parm	LPCRITICAL_SECTION	|	CS			| Pointer to a Critical Section
*
*	@comm
*			Don't call this with a null list.
*
*	@ex		An example of how to use this function follows |
*			No Example
*
*/

_inline
PLIST_ENTRY
RemoveTailLockedList(PLIST_ENTRY ListHead, LPCRITICAL_SECTION CS)
{
	PLIST_ENTRY _EX_Entry;

	DEBUGMSG(DEBUG_LINK_LIST, (TEXT("+RemoveTailList: ListHead=%x(%x,%x)\r\n"),
				 ListHead, ListHead->Flink, ListHead->Blink));

	EnterCriticalSection(CS);

	_EX_Entry = ListHead->Blink;
	RemoveEntryList(_EX_Entry);

	DEBUGMSG(DEBUG_LINK_LIST, (TEXT("-RemoveTailList: ListHead=%x(%x,%x)\r\n"),
				 ListHead, ListHead->Flink, ListHead->Blink));

	LeaveCriticalSection(CS);

	return _EX_Entry;
}

/*****************************************************************************
*
*
*	@func	void	|	InsertTailList	|
*			Insert at the end of a list
*
*	@rdesc	No return
*
*	@parm	PLIST_ENTRY	|	ListHead	| The head of the list
*	@parm	PLIST_ENTRY	|	Entry		| The Entry to add to the list
*
*	@comm
*			Inserts an entry at the tail of a list
*
*	@ex		An example of how to use this function follows |
*			No Example
*
*/

_inline
void
InsertTailList(PLIST_ENTRY ListHead, PLIST_ENTRY Entry)
{
	DEBUGMSG(DEBUG_LINK_LIST,
			 (TEXT("+InsertTailList: Head=%x(%x,%x) Entry=%x\r\n"),
			  ListHead, ListHead->Flink, ListHead->Blink, Entry));

    Entry->Flink           = ListHead;
    Entry->Blink           = ListHead->Blink;
    ListHead->Blink->Flink = Entry;
    ListHead->Blink        = Entry;

	DEBUGMSG(DEBUG_LINK_LIST,
			 (TEXT("-InsertTailList: Head=%x(%x,%x) Entry=%x(%x,%x)\r\n"),
			  ListHead, ListHead->Flink, ListHead->Blink, Entry,
			  Entry->Flink, Entry->Blink));

	return;
}

/*****************************************************************************
*
*
*	@func	void	|	InsertTailLockedList	|
*			Insert at the end of a list
*
*	@rdesc	No return
*
*	@parm	PLIST_ENTRY			|	ListHead	| The head of the list
*	@parm	PLIST_ENTRY			|	Entry		| The Entry to add to the list
*	@parm	LPCRITICAL_SECTION	|	CS			| Pointer to a Critical Section
*
*	@comm
*			Inserts an entry at the tail of a list
*
*	@ex		An example of how to use this function follows |
*			No Example
*
*/

_inline
void
InsertTailLockedList(PLIST_ENTRY ListHead, PLIST_ENTRY Entry,
					 LPCRITICAL_SECTION CS)
{
	DEBUGMSG(DEBUG_LINK_LIST,
			 (TEXT("+InsertTailList: Head=%x(%x,%x) CS=%x Entry=%x\r\n"),
			  ListHead, ListHead->Flink, ListHead->Blink, CS, Entry));

	EnterCriticalSection(CS);

    Entry->Flink           = ListHead;
    Entry->Blink           = ListHead->Blink;
    ListHead->Blink->Flink = Entry;
    ListHead->Blink        = Entry;

	DEBUGMSG(DEBUG_LINK_LIST,
			 (TEXT("-InsertTailList: Head=%x(%x,%x) Entry=%x(%x,%x)\r\n"),
			  ListHead, ListHead->Flink, ListHead->Blink, Entry,
			  Entry->Flink, Entry->Blink));

	LeaveCriticalSection(CS);

	return;
}

/*****************************************************************************
*
*
*	@func	void	|	InsertHeadList	|
*			Insert an entry at the head of a list
*
*	@rdesc	No return.
*
*	@parm	PLIST_ENTRY	|	ListHead	| The head of the list
*	@parm	PLIST_ENTRY	|	Entry		| The Entry to add to the list
*
*	@comm
*			This will add an entry to the head of a list.
*
*	@ex		An example of how to use this function follows |
*			No Example
*
*/

_inline
void
InsertHeadList(PLIST_ENTRY ListHead, PLIST_ENTRY Entry)
{
	DEBUGMSG(DEBUG_LINK_LIST,
			 (TEXT("+InsertHeadList: Head=%x(%x,%x) Entry=%x\r\n"),
			  ListHead, ListHead->Flink, ListHead->Blink, Entry));

    Entry->Flink           = ListHead->Flink;
    Entry->Blink           = ListHead;
    ListHead->Flink->Blink = Entry;
    ListHead->Flink        = Entry;

	DEBUGMSG(DEBUG_LINK_LIST,
			 (TEXT("-InsertHeadList: Head=%x(%x,%x) Entry=%x(%x,%x)\r\n"),
			  ListHead, ListHead->Flink, ListHead->Blink,
			  Entry, Entry->Flink, Entry->Blink));
}

/*****************************************************************************
*
*
*	@func	void	|	InsertHeadLockedList	|
*			Insert an entry at the head of a critcal section locked list
*
*	@rdesc	No return.
*
*	@parm	PLIST_ENTRY			|	ListHead	| The head of the list
*	@parm	PLIST_ENTRY			|	Entry		| The Entry to add to the list
*	@parm	LPCRITICAL_SECTION	|	CS			| Pointer to a Critical Section
*
*	@comm
*			This will add an entry to the head of a list.
*
*	@ex		An example of how to use this function follows |
*			No Example
*
*/

_inline
void
InsertHeadLockedList(PLIST_ENTRY ListHead, PLIST_ENTRY Entry,
					 LPCRITICAL_SECTION CS)
{
	DEBUGMSG(DEBUG_LINK_LIST,
			 (TEXT("+InsertHeadLockedList: Head=%x(%x,%x) CS=%x Entry=%x\r\n"),
			  ListHead, ListHead->Flink, ListHead->Blink, CS, Entry));

	EnterCriticalSection (CS);

    Entry->Flink           = ListHead->Flink;
    Entry->Blink           = ListHead;
    ListHead->Flink->Blink = Entry;
    ListHead->Flink        = Entry;

	DEBUGMSG(DEBUG_LINK_LIST,
			 (TEXT("-InsertHeadLockedList: Head=%x(%x,%x) Entry=%x(%x,%x)\r\n"),
			  ListHead, ListHead->Flink, ListHead->Blink,
			  Entry, Entry->Flink, Entry->Blink));

	LeaveCriticalSection (CS);
}

/*INC*/

#endif  // _LINKLIST_H_
