/* Copyright (c) 1991-2000 Microsoft Corporation.  All rights reserved.

    ipaddr.h - TCP/IP Address custom control, global definitions

*/


// Messages sent to IPAddress controls

#define	IP_CLEARADDRESS	WM_USER+100	// no parameters
#define	IP_SETADDRESS	WM_USER+101	// lparam = TCP/IP address
#define IP_GETADDRESS	WM_USER+102	// lresult = TCP/IP address
#define IP_SETRANGE	WM_USER+103	// wparam = field, lparam = range
#define IP_SETFOCUS	WM_USER+104	// wparam = field


// IP Address style bits

#define	IP_LEFT		0x00000001L	// left justify fields
#define IP_RIGHT	0x00000002L	// right justify fields
#define	IP_CENTER	0x00000004L	// center fields
#define IP_ZERO		0x00000008L	// Zero fill the fields


// The following is a useful macro for passing the range values in the
// IP_SETRANGE message.

#define	MAKERANGE(low, high)	((LPARAM)(WORD)(((BYTE)(high) << 8) + (BYTE)(low)))

// And this is a useful macro for making the IP Address to be passed
// as a LPARAM.

#define MAKEIPADDRESS(b1,b2,b3,b4)	((LPARAM)(((DWORD)(b1)<<24)+((DWORD)(b2)<<16)+((DWORD)(b3)<<8)+((DWORD)(b4))))

/*
    defines and Typedefs.
*/

#define	MAX_IPNOMEMSTRING	30
#define MAX_IPCAPTION		30
#define MAX_IPRES		256

//#define MAX_NAME	63
#define NUM_FIELDS	4	// number of fields in an IP Address
#define MIN_FIELD1	1	// min allowed value for field 1
#define MAX_FIELD1	223	// max allowed value for field 1
#define MIN_FIELD2	0	// min for field 2
#define MAX_FIELD2	255	// max for field 2
#define MIN_FIELD3	0	// min for field 3
#define MAX_FIELD3	254	// max for field 3
#define MIN_FIELD4      1       // 0 is reserved for broadcast

#define MAX_SCOPE_ID	47	// maximum number of characters in scope id
#define MAX_SESSIONS	22	// maximum number of sessions
#define	SESSION_DIGITS   2	// maximum length of sessions as a string
#define DEFAULT_SESSIONS 6	// default number of sessions
#define DEFAULT_SOCKETS  4	// default number of sockets
//#define MAX_NAME	63	// maximum length of hostname or domain

#define MAX_MBSTRING	400	// Maximum length of string for message box.
#define MAX_KEY		32	// Maximum length of a key field in a .INI file
#define	MAX_NOMEMSTRING	128	// Maximum length of out of memory string
#define MAX_CAPTION	80	// Maximum length of caption

#define IPADDRESS_CLASS            TEXT("RNA_IPAddress")

/* The character that is displayed between address fields. */
#define	FILLER		TEXT('.')
#define	SZFILLER	TEXT(".")
#define SPACE		TEXT(' ')
#define BACK_SPACE	8

/* Min, max values */
#define NUM_FIELDS	4
#define CHARS_PER_FIELD	3
#define HEAD_ROOM	2	/* space at top of control */
#define LEAD_ROOM	2	/* space at front of control */
#define MIN_FIELD_VALUE	0	/* default minimum allowable field value */
#define MAX_FIELD_VALUE	255	/* default maximum allowable field value */


/* All the information unique to one control is stuffed in one of these
   structures in global memory and the handle to the memory is stored in the 
   Windows extra space.
 */  

typedef struct tagFIELD {
    HANDLE	hWnd;
    FARPROC	lpfnWndProc;
    BYTE    byValue; /* current value for this field. */
    BYTE	byLow;	/* lowest allowed value for this field. */
    BYTE	byHigh;	/* Highest allowed value for this field.*/
} FIELD;

typedef struct tagCONTROL {
    HWND	hwndParent;
    UINT	uiFieldWidth;
    UINT	uiFillerWidth;
    DWORD	dwStyle;	/* Style bits for the control*/
    BOOL	fEnabled;
    BOOL	fPainted;
    BOOL	fInMessageBox;	/* Set when a message box is displayed so that
                                   we don't send a EN_KILLFOCUS message when
				   we receive the EN_KILLFOCUS message for the
				   current field. */
    BOOL	fHaveFocus;	/* TRUE if we have the focus, FALSE otherwise.*/
    FIELD	Children[NUM_FIELDS];
} CONTROL;


/* The following macros extract and store the CONTROL structure for a control.*/
#define    IPADDRESS_EXTRA            sizeof(HANDLE)

#define	GET_CONTROL_HANDLE(hWnd)	((HANDLE)(GetWindowLong((hWnd), 0)))
#define SAVE_CONTROL_HANDLE(hWnd,x)	(SetWindowLong((hWnd), 0, (DWORD)x))


/* internal IPAddress function prototypes */
LONG CALLBACK IPAddrDlgProc(HWND, UINT, WPARAM, LPARAM );
LONG CALLBACK IPAddressWndFn( HWND, UINT, WPARAM, LPARAM );
LONG CALLBACK IPAddressFieldProc(HWND, UINT, WPARAM, LPARAM);
BOOL SwitchFields(CONTROL FAR *, int, int, DWORD, DWORD);
void EnterField(FIELD FAR *, DWORD, DWORD);
BOOL ExitField(CONTROL FAR *, int iField);
void PadField(CONTROL FAR *, int iField);
int GetFieldValue(FIELD FAR *);
DWORD CheckAddress(DWORD ip);
