/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

Module Name:  

coblist.h

Abstract:  

Notes: 


--*/
#ifndef __COBLIST_H__
#define __COBLIST_H__

#ifdef __cplusplus
extern "C" {
#endif

/* possible states for enumeration
 */
typedef enum {
  ENUM_NONE,
  ENUM_FORWARD,
  ENUM_BACKWARD
} ENUM_STATES;

typedef enum {
  GETOBJECT_CURRENT,
  GETOBJECT_FIRST,
  GETOBJECT_LAST,
  GETOBJECT_NEXT,
  GETOBJECT_PREV,
} GETOBJECT_CMD;

/* forward declarations
 */
class CObject;  
class CObList;

typedef CObject* POBJECT;
typedef CObList* POBLIST;

typedef CObject* LPOBJECT;
typedef CObList* LPOBLIST;


/*---------------------------------------------------------------------------*\
 * CProtect: class definition
\*---------------------------------------------------------------------------*/
class CProtect
{
public:
	CProtect(LPCRITICAL_SECTION lpcs) : m_lpcs(lpcs)
		{ EnterCriticalSection(m_lpcs); }
	~CProtect() { LeaveCriticalSection(m_lpcs); }

private:
	LPCRITICAL_SECTION m_lpcs;
	
}; /* CProtect */

typedef CProtect PROTECT;


/*---------------------------------------------------------------------------*\
 * CObject: class definition
\*---------------------------------------------------------------------------*/
class CObject
{
public:
	CObject();
	virtual ~CObject();

	VOID Initialize(VOID) { m_pprev = m_pnext = NULL; m_powner = NULL; }
    
	VOID SetOwner(CObList *powner) { m_powner = powner; }
	LPOBLIST GetOwner(void) { return m_powner; }

	LPOBJECT GetPrev(VOID) { return m_pprev; }
	LPOBJECT GetNext(VOID) { return m_pnext; }

	VOID SetPrev(LPOBJECT pobject)  { m_pprev = pobject; };
	VOID SetNext(LPOBJECT pobject)  { m_pnext = pobject; };

private:
	CObject *m_pprev, *m_pnext;
	CObList *m_powner;
	
}; /* CObject */


/*---------------------------------------------------------------------------*\
 * CObList: class definition
\*---------------------------------------------------------------------------*/
class CObList
{
private:
    LPOBJECT m_phead, m_ptail, m_pcurr;
    ENUM_STATES m_enumstate;
    DWORD m_dwcount;
	CRITICAL_SECTION m_cs;
	
public:
	CObList();
    virtual ~CObList();

	LPCRITICAL_SECTION GetCriticalSection() { return &m_cs; }
	
	BOOL IsEmpty(VOID) { return (0 == m_dwcount); }
	LPOBJECT IsValid(LPOBJECT pobject);
    
	LPOBJECT AddHead(LPOBJECT pobject);
	LPOBJECT AddTail(LPOBJECT pobject);
	LPOBJECT MoveToHead(LPOBJECT pobject);
	LPOBJECT MoveToTail(LPOBJECT pobject);
	LPOBJECT Disconnect(LPOBJECT pobject);
	LPOBJECT Get_Object(GETOBJECT_CMD goCmd);
	LPOBJECT SetCurrent(LPOBJECT pobject);
	LPOBJECT Reset(VOID) { return SetCurrent(First()); }

	VOID DisconnectAll(VOID);
	DWORD GetCount(VOID) { return m_dwcount; }

	LONG GetObjectIndex(LPOBJECT pobject);
	LPOBJECT GetAtIndex(LONG cbIndex);
	LPOBJECT DisconnectAtIndex(LONG cbIndex);
	LPOBJECT InsertBeforeIndex(LPOBJECT pobject, LONG cbIndex);
	LPOBJECT InsertAfterIndex(LPOBJECT pobject, LONG cbIndex);

	LPOBJECT InsertAfterObject(LPOBJECT pobject, LPOBJECT pInsertAfter);
       
	BOOL Swap(LPOBJECT pobject1, LPOBJECT pobject2);
    
	LPOBJECT Head(VOID) { return m_phead; }
	LPOBJECT Tail(VOID) { return m_ptail; }
	LPOBJECT Current(VOID) { return m_pcurr; }

	LPOBJECT First(VOID) { return Get_Object(GETOBJECT_FIRST); }
	LPOBJECT Next(VOID)  { return Get_Object(GETOBJECT_NEXT ); } 
	LPOBJECT Last(VOID)  { return Get_Object(GETOBJECT_LAST ); } 
	LPOBJECT Prev(VOID)  { return Get_Object(GETOBJECT_PREV ); }

}; /* CObList */

#ifdef __cplusplus
}
#endif

#endif // __COBLIST_H__
