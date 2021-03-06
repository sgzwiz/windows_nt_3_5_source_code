HANDLE hInst;

HANDLE hAccTable;	      /* handle to accelerator table */

HWND   hwnd;		      /* handle to main window */

HWND  APIENTRY ViewScreen2(LPSTR,LPSTR,INT,LPSTR,LPSTR,INT,HWND);
LONG  APIENTRY ViewWndProc2(HWND, UINT, WPARAM, LPARAM);
BOOL  APIENTRY FileInfo(HWND, UINT, WPARAM, LPARAM);
VOID  APIENTRY DisplayErrMessage(HWND,INT);
VOID  APIENTRY DisplayMessage(HWND,INT,INT);
INT   APIENTRY DisplayMessageRet(HWND,INT,INT);
BOOL  APIENTRY GetInfo(HWND,INT,INT,INT,INT,INT);
BOOL  APIENTRY DisplayErrSetFocus(HWND,INT,INT);
VOID  APIENTRY AdjustScreenNumber(HWND,INT,WORD,INT FAR *);

LPSTR  APIENTRY instr (LPSTR,LPSTR);

// DUMP.DLL Routines

INT  APIENTRY fCompFiles     (LPSTR, INT, LPSTR, INT, INT);
INT  APIENTRY fCompScreen    (LPSTR, RECT FAR *, INT, INT, INT);
INT  APIENTRY fDelScreen     (LPSTR, INT);
INT  APIENTRY fDumpScreen    (LPSTR, RECT FAR *, INT, INT, INT);
INT  APIENTRY fViewScreen    (LPSTR, HWND, RECT FAR *,INT, INT,INT) ;
INT  APIENTRY fFileInfo      (LPSTR, RECT FAR *, INT FAR *, INT FAR *);
INT  APIENTRY fGetOS         (LPSTR);
INT  APIENTRY fGetMaxScreen  (LPSTR);
INT  APIENTRY fGetDLLVersion (LPSTR);
BYTE  APIENTRY DetermineMode(INT FAR *, INT FAR *);
INT  APIENTRY fDumpSrnToClip    (RECT FAR *, INT) ;
INT  APIENTRY fDumpWndToClip    (HWND, INT) ;
INT  APIENTRY fDumpFileToClip   (LPSTR, INT) ;
INT  APIENTRY fSaveSrnToDIB     (LPSTR, RECT FAR *, INT) ;
INT  APIENTRY fSaveWndToDIB     (LPSTR, HWND, INT) ;
INT  APIENTRY fSaveFileToDIB    (LPSTR, INT, LPSTR) ;
LONG  APIENTRY DetermineColours (VOID);


#define LEN_EDITBOX    128
#define LEN_SCRBOX       3
#define ALL_SCROLLBAR  200
#define MAX_SCREEN     100
