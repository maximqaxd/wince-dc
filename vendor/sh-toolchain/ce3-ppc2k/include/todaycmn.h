/*****************************************************************************/
/*                                                                           */
/*  TodayCmn.h                                                               */
/*                                                                           */
/*  Today screen common header file                                          */
/*                                                                           */
/*****************************************************************************/

#ifndef _TODAYCMN_H_
#define _TODAYCMN_H_

/*****************************************************************************/


/* list item types */
typedef enum _TODAYLISTITEMTYPE
{
    tlitOwnerInfo = 0,  /* name, company */
    tlitAppointments,   /* today's appointments, events */
    tlitMail,           /* message information */
    tlitTasks,          /* overdue and upcoming tasks */
    tlitCustom,         /* other */
    tlitNil             /* sentinel */

} TODAYLISTITEMTYPE;


#define MAX_ITEMNAME    32


/* information for a single today item */
typedef struct _TODAYLISTITEM
{
    TCHAR               szName[MAX_ITEMNAME];
    TODAYLISTITEMTYPE   tlit;
    DWORD               dwOrder;
    DWORD               cyp;
    BOOL                fEnabled;
    BOOL                fOptions;
    DWORD               grfFlags;
    TCHAR               szDLLPath[MAX_PATH];
    HINSTANCE           hinstDLL;
    HWND                hwndCustom;
    BOOL                fSizeOnDraw;
    BYTE                *prgbCachedData;
    DWORD               cbCachedData;

} TODAYLISTITEM;


/* maximum number of today items */
#define k_cTodayItemsMax 12


/* custom DLL resources */

#define IDI_ICON                128
#define IDD_TODAY_CUSTOM        500

/* custom DLL functions */

#define ORDINAL_INITIALIZEITEM      240
typedef HWND (*PFNCUSTOMINITIALIZEITEM)(TODAYLISTITEM *, HWND);

#define ORDINAL_OPTIONSDIALOGPROC   241
typedef BOOL (*PFNCUSTOMOPTIONSDLGPROC)(HWND, UINT, UINT, LONG);

/* custom DLL messages */

#define WM_TODAYCUSTOM_CLEARCACHE           (WM_USER + 242)
#define WM_TODAYCUSTOM_QUERYREFRESHCACHE    (WM_USER + 243)


/*****************************************************************************/

#endif /* !_TODAYCMN_H_ */

