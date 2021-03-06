/*-----------------------------------------------------------------------------+
| DLGS.C                                                                       |
|                                                                              |
| Routines to handle selection range display                                   |
|                                                                              |
| (C) Copyright Microsoft Corporation 1991.  All rights reserved.              |
|                                                                              |
| Revision History                                                             |
|    Oct-1992 MikeTri Ported to WIN32 / WIN16 common code                      |
|                                                                              |
+-----------------------------------------------------------------------------*/

//#undef NOSCROLL        // SB_* and scrolling routines
//#undef NOWINOFFSETS    // GWL_*, GCL_*, associated routines
//#undef NOCOLOR         // color stuff
//#include <string.h>
#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#ifdef WIN16
#include "port16.h"
#endif
#include "mplayer.h"
#include "stdlib.h"

extern    UINT    gwCurScale;

/*
 * FUNCTION PROTOTYPES
 */
LONG FAR PASCAL _EXPORT setselDlgProc(HWND hwnd, unsigned msg, WPARAM wParam, LPARAM lParam);
LONG FAR PASCAL _EXPORT optionsDlgProc(HWND hwnd, unsigned msg, WPARAM wParam, LPARAM lParam);
BOOL FAR PASCAL _EXPORT mciDlgProc(HWND hwnd, unsigned msg, WPARAM wParam, LPARAM lParam);

/*--------------------------------------------------------------+
| ******************* PUBLIC FUNCTIONS ************************ |
+--------------------------------------------------------------*/
/*--------------------------------------------------------------+
| setselDialog - bring up the dialog for Set Selection          |
|                                                               |
+--------------------------------------------------------------*/
BOOL FAR PASCAL setselDialog(HWND hwnd)
{
    FARPROC fpfn;

    frameboxInit(ghInst, ghInstPrev);

    fpfn = MakeProcInstance((FARPROC)setselDlgProc, ghInst);
    giHelpContext = IDM_SELECTION;
    DialogBox(ghInst, TEXT("SetSelection"), hwnd, (DLGPROC)fpfn);
    giHelpContext = 0;
#ifndef WIN32
    FreeProcInstance(fpfn);
#endif
    return TRUE;                // should we check return value?
}

static BOOL    sfNumLastChosen;
static BOOL    sfInUpdate = FALSE;
/*--------------------------------------------------------------+
| setselDlgProc - dialog procedure for Set Selection dialog     |
|                                                               |
+--------------------------------------------------------------*/
LONG FAR PASCAL _EXPORT setselDlgProc(HWND hwnd, unsigned msg, WPARAM wParam, LPARAM lParam)
{
    int     iItem;
    DWORD   fr, fr2, frIn, frOut, frMarkIn, frMarkOut, frCurrent;
    TCHAR   ach[80];
    LPTSTR  lpsz = (LPTSTR)ach;
    static int aKeyWordIds[] = {0, IDH_SELECT_SELECT, 0, 0};

    frMarkIn = frIn = SendMessage(ghwndTrackbar, TBM_GETSELSTART, 0, 0);
    frMarkOut = frOut = SendMessage(ghwndTrackbar, TBM_GETSELEND, 0, 0);
    frCurrent = SendMessage(ghwndTrackbar, TBM_GETPOS, 0, 0);

    switch(msg){
        case WM_INITDIALOG:
            if (gwCurScale == ID_TIME) {
                LOADSTRING(IDS_TIMEMODE, ach);
                SetWindowText(hwnd, lpsz);
            } else if (gwCurScale == ID_FRAMES) {
                LOADSTRING(IDS_FRAMEMODE, ach);
                SetWindowText(hwnd, lpsz);
            } else {
                LOADSTRING(IDS_TRACKMODE, ach);
                SetWindowText(hwnd, lpsz);
            }

        /* Always put something here - if no selection, use the cur frame */
            if (frMarkIn == -1 || frMarkOut == -1) {
                SetDlgItemInt(hwnd, IDC_EDITFROM, frCurrent, FALSE);
                SetDlgItemInt(hwnd, IDC_EDITTO, frCurrent, FALSE);
                SetDlgItemInt(hwnd, IDC_EDITNUM, 0, FALSE);
            } else {
                SetDlgItemInt(hwnd, IDC_EDITFROM, frMarkIn, FALSE);
                SetDlgItemInt(hwnd, IDC_EDITTO, frMarkOut, FALSE);
                SetDlgItemInt(hwnd, IDC_EDITNUM, frMarkOut - frMarkIn, FALSE);
            }

            if (frMarkIn == -1 || frMarkOut == -1) {
                /* turn on the NONE radio button */
                CheckRadioButton(hwnd, IDC_EDITALL, IDC_EDITNONE, IDC_EDITNONE);
            } else if(frMarkIn == gdwMediaStart &&
                frMarkOut == gdwMediaStart + gdwMediaLength){
                /* turn on the ALL button, it is all selected */
                CheckRadioButton(hwnd, IDC_EDITALL, IDC_EDITNONE, IDC_EDITALL);
            } else {
                /* turn on the From/To portion */
                CheckRadioButton(hwnd, IDC_EDITALL, IDC_EDITNONE, IDC_EDITSOME);
            }

            return TRUE;
#ifndef WIN32
        case WM_CONTEXTMENU:
                if (hwnd == (HWND)wParam)
                    break;
                aKeyWordIds[0] = GetDlgCtrlID((HWND)wParam);
                WinHelp((HWND)wParam, NULL, HELP_CONTEXTMENU, (DWORD)(LPTSTR)aKeyWordIds);
                return TRUE;

        case WM_HELP:
                if (hwnd == (HWND)((LPHELPINFO)lParam)->hItemHandle)
                    break;

                aKeyWordIds[0] = ((LPHELPINFO)lParam)->iCtrlId;
                WinHelp(((LPHELPINFO)lParam)->hItemHandle, NULL,
                        HELP_WM_HELP, (DWORD)(LPTSTR)aKeyWordIds);
                return TRUE;
#else
#pragma message("Need to implement help stuff")
#endif /* ~WIN32 */
        case WM_COMMAND:
            switch(LOWORD(wParam)){
                WORD Code;
                BOOL OK;

                case IDOK:
                    /* We hit this AFTER we press OK on the selection box */

                    /* Make sure box we're editing loses focus before we */
                    /* execute, so values will be set properly.          */
                    SetFocus(GetDlgItem(hwnd, IDOK));
                    if (IsDlgButtonChecked(hwnd, IDC_EDITALL)) {
                        /* this is the All: case */
                        frIn = gdwMediaStart;
                        frOut = gdwMediaStart + gdwMediaLength;
                    } else if (IsDlgButtonChecked(hwnd, IDC_EDITNONE)){
                        /* this is the None: case */
                        frIn = frOut = (DWORD)(-1);
                    } else {
                        /* this is the From: To: case */
                        iItem = 0;

                        frIn = GetDlgItemInt(hwnd, IDC_EDITFROM, &OK, FALSE);

                        if (!OK)
                            iItem = IDC_EDITFROM;    // we misbehaved
                        else {

                            frOut = GetDlgItemInt(hwnd, IDC_EDITTO, &OK, FALSE);

                            if (!OK)
                                iItem = IDC_EDITTO;
                        }
                        if ((!OK)
                            || (frOut < frIn)
                            || ((long)frIn < (long)gdwMediaStart)
                            || (frOut > gdwMediaStart + gdwMediaLength)) {
                            if (!iItem && (long)frIn < (long)gdwMediaStart)
                                iItem = IDC_EDITFROM; // who misbehaved?
                            else if (!iItem)
                                iItem = IDC_EDITTO;
//                   Don't beep -- Lose focus message already beeped
//                          MessageBeep(MB_ICONEXCLAMATION);
                    /* Illegal values, display msg box  */
                            ErrorResBox(hwnd, ghInst,
                                        MB_ICONEXCLAMATION | MB_OK,
                                        IDS_APPNAME, IDS_FRAMERANGE);
                    /* Prevent box from ending */
                    /* select offending value */
                            SetFocus(GetDlgItem(hwnd, iItem));
#ifdef WIN32
                            SendMessage(GetDlgItem(hwnd, iItem),
                                        EM_SETSEL, 0, (LPARAM)-1);
#else
                            SendMessage(GetDlgItem(hwnd, iItem),
                                        EM_SETSEL, 0, MAKELONG(0,32767));
#endif
                            return TRUE;
                        }
                    }
                    SendMessage(ghwndTrackbar, TBM_SETSELSTART, (WPARAM)FALSE, frIn);
                    SendMessage(ghwndTrackbar, TBM_SETSELEND, (WPARAM)TRUE, frOut);
                    DirtyObject(TRUE);
                    EndDialog(hwnd, TRUE);
                    break;

                case IDCANCEL:
                    EndDialog(hwnd, FALSE);
                    break;

                case IDC_EDITALL:
                    CheckRadioButton(hwnd, IDC_EDITALL,
                                     IDC_EDITNONE, IDC_EDITALL);
                    break;

                case IDC_EDITNONE:
                    CheckRadioButton(hwnd, IDC_EDITALL,
                                     IDC_EDITNONE, IDC_EDITNONE);
                    break;

                case IDC_EDITSOME:
                    CheckRadioButton(hwnd, IDC_EDITALL,
                                    IDC_EDITNONE, IDC_EDITSOME);

                    /* put the focus on the FROM box */
                    SetFocus(GetDlgItem(hwnd, IDC_EDITFROM));
                    break;

                case IDC_EDITNUM:
                    /* turn on the FROM box if it isn't */
                    Code = GET_WM_COMMAND_CMD(wParam, lParam);

                    if (!IsDlgButtonChecked(hwnd, IDC_EDITSOME))
                    {
                    	SetFocus(GetDlgItem(hwnd, IDC_EDITSOME));
                        CheckRadioButton(hwnd, IDC_EDITALL,
                                        IDC_EDITNONE, IDC_EDITSOME);
                    }

                    if (!sfInUpdate && Code == EN_KILLFOCUS) {
                        sfNumLastChosen = TRUE;
                        goto AdjustSomething;
                    }
                    break;

                case IDC_EDITTO:
                    /* turn on the FROM box if it isn't */
                    Code = GET_WM_COMMAND_CMD(wParam, lParam);

                    if (!IsDlgButtonChecked(hwnd, IDC_EDITSOME))
                    {
                        SetFocus(GetDlgItem(hwnd, IDC_EDITSOME));
                        CheckRadioButton(hwnd, IDC_EDITALL,
                                        IDC_EDITNONE, IDC_EDITSOME);
			
                    }

                    if (!sfInUpdate && Code == EN_KILLFOCUS) {
                        sfNumLastChosen = FALSE;
                        goto AdjustSomething;
                    }
                    break;

                case IDC_EDITFROM:
                    /* turn on the FROM box if it isn't */
                    Code = GET_WM_COMMAND_CMD(wParam, lParam);

                    if (!IsDlgButtonChecked(hwnd, IDC_EDITSOME))
                    {
                        CheckRadioButton(hwnd, IDC_EDITALL,
                                        IDC_EDITNONE, IDC_EDITSOME);
                        if (GetFocus() != GetDlgItem(hwnd, IDC_EDITSOME))
                        	SetFocus(GetDlgItem(hwnd, IDC_EDITSOME));

                    }

                    if (!sfInUpdate && Code == EN_KILLFOCUS) {
                        sfNumLastChosen = FALSE;
                        goto AdjustSomething;
                    }
                    break;

AdjustSomething:
                    sfInUpdate = TRUE;

                    fr = GetDlgItemInt(hwnd, IDC_EDITFROM, &OK, FALSE);

                    if (!OK)
                        MessageBeep(MB_ICONEXCLAMATION);
                    else {
                        if ((long)fr < (long)gdwMediaStart) {
                            MessageBeep(MB_ICONEXCLAMATION);
                            fr = gdwMediaStart;
                        }
                        if (fr > gdwMediaStart + gdwMediaLength) {
                            MessageBeep(MB_ICONEXCLAMATION);
                            fr = gdwMediaStart + gdwMediaLength;
                        }

                    // We have to do this in time format, or if fr changed

                        SetDlgItemInt(hwnd, IDC_EDITFROM, fr, FALSE);

                        if (sfNumLastChosen) {
                            /* They changed the number of frames last, */
                            /* so keep it constant.                    */
AdjustTo:
                            fr2 = GetDlgItemInt(hwnd, IDC_EDITNUM, &OK, FALSE);

                            if (!OK)
                                MessageBeep(MB_ICONEXCLAMATION);
                            else {
                                if (fr + fr2 > gdwMediaStart + gdwMediaLength) {
                                    MessageBeep(MB_ICONEXCLAMATION);
                                    fr2 = gdwMediaStart + gdwMediaLength - fr;
                                }

//                               if (fr2 < 0)
//                                   fr2 = 0;

                        // We have to do this in time format, or if fr changed

                                SetDlgItemInt(hwnd, IDC_EDITNUM, fr2, FALSE);
                                SetDlgItemInt(hwnd, IDC_EDITTO, fr + fr2, FALSE);
                            }
                        } else {
                            /* They changed a frame number last, */
                            /* so vary the number of frames      */

                            fr2 = GetDlgItemInt(hwnd, IDC_EDITTO, &OK, FALSE);

                            if (!OK)
                                MessageBeep(MB_ICONEXCLAMATION);
                            else {
                                if (fr2 < fr) {
                                /* Set TO = FROM */
                                SetDlgItemInt(hwnd, IDC_EDITNUM, 0, FALSE);
                                goto AdjustTo;
                            }

                            if (fr2 > gdwMediaStart + gdwMediaLength) {
                                MessageBeep(MB_ICONEXCLAMATION);
                                fr2 = gdwMediaStart + gdwMediaLength;
                            }

                            SetDlgItemInt(hwnd, IDC_EDITNUM, fr2 - fr, FALSE);

                            // must redraw for time mode or if fr2 changed
                            SetDlgItemInt(hwnd, IDC_EDITTO, fr2, FALSE);
                        }
                    }
                }

                sfInUpdate = FALSE;
                return TRUE;

                break;
            }
            break;

        }
        return FALSE;
}

/*--------------------------------------------------------------+
| optionsDialog - bring up the dialog for Options               |
|                                                               |
+--------------------------------------------------------------*/
BOOL FAR PASCAL optionsDialog(HWND hwnd)
{
    FARPROC fpfn;
#if 0
    DWORD   ThreadId;
    DWORD   WindowThreadId;
#endif

    fpfn = MakeProcInstance((FARPROC)optionsDlgProc, ghInst);
    giHelpContext = IDM_OPTIONS;

#if 0
    Problem:

    When in-place editing, bring up the Options (or other) dialog,
    then bring another app into the foreground.  If you now click on
    our container, you just get a beep.  You can get back using the
    Task List.

    I can't get it to work with AttachThreadInput, but I'm not even
    sure that this should be the server's responsibility.  It's the
    container that's receiving the mouse clicks.

    I haven't had any word from the OLE guys on this question.

    if (gfOle2IPEditing)
    {
        ThreadId = GetCurrentThreadId( );
        WindowThreadId = GetWindowThreadProcessId(ghwndCntr, NULL);
        AttachThreadInput(WindowThreadId, ThreadId, TRUE);
    }
#endif

    DialogBox(ghInst, TEXT("Options"), hwnd, (DLGPROC)fpfn);

#if 0
    if (gfOle2IPEditing)
        AttachThreadInput(ThreadId, WindowThreadId, FALSE);
#endif
    giHelpContext = 0;
#ifndef WIN32
    FreeProcInstance(fpfn);
#endif
    return TRUE;    // should we check return value?
}

/*--------------------------------------------------------------+
| optionsDlgProc - dialog procedure for Options dialog          |
|                                                               |
+--------------------------------------------------------------*/
LONG FAR PASCAL _EXPORT optionsDlgProc(HWND hwnd, unsigned msg, WPARAM wParam, LPARAM lParam)
{
    UINT w;
    HDC  hdc;
    static int aKeyWordIds[] =
    {
        OPT_AUTORWD,    IDH_OPT_AUTO,
        OPT_AUTOREP,    IDH_OPT_REPEAT,
        OPT_TITLE,      IDH_OPT_CAPTCONTROL,
        OPT_BORDER,     IDH_OPT_BORDER,
        OPT_PLAY,       IDH_OPT_PLAYCLIENT,
        OPT_DITHER,     IDH_OPT_DITHER,
        IDC_CAPTIONTEXT,IDH_OPT_CAPTION,
        0  , 0
    };


    switch(msg){
        case WM_INITDIALOG:
            /* Take advantage of the fact that the button IDS are the */
            /* same as the bit fields.                                */
            for (w = OPT_FIRST; w <= OPT_LAST; w <<= 1)
                CheckDlgButton(hwnd, w, gwOptions & w);

            /* Enable and Fill the Title Text */
            /* limit this box to CAPTION_LEN chars of input */
            SendMessage(GetDlgItem(hwnd, IDC_TITLETEXT), EM_LIMITTEXT,
                        (WPARAM)CAPTION_LEN, 0L);
            SendMessage(hwnd, WM_COMMAND, (WPARAM)OPT_BAR, 0L);

            hdc = GetDC(NULL);
            if (!(GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE) ||
                !(gwDeviceType & DTMCI_CANWINDOW)) {
                CheckDlgButton(hwnd, OPT_DITHER, FALSE);
                EnableWindow(GetDlgItem(hwnd, OPT_DITHER), FALSE);

#if 0
                CheckDlgButton(hwnd, OPT_USEPALETTE, FALSE);
                EnableWindow(GetDlgItem(hwnd, OPT_USEPALETTE), FALSE);
#endif
            }
            ReleaseDC(NULL, hdc);
            return TRUE;

#pragma message("Need to handle WM_CONTEXTMENU and WM_HELP")
#ifdef LATER
        case WM_CONTEXTMENU:
            {
                int i;
                for (i = 0; aKeyWordIds[i]; i+=2)
                    if (aKeyWordIds[i] == GetDlgCtrlID((HWND)wParam))
                        break;
                if (aKeyWordIds[i] == 0)
                    break;

                WinHelp((HWND)wParam, NULL, HELP_CONTEXTMENU, (DWORD)(LPTSTR)aKeyWordIds);
                return TRUE;
            }
        case WM_HELP:
            {
                int i;

                for (i = 0; aKeyWordIds[i]; i+=2)
                    if (aKeyWordIds[i] == ((LPHELPINFO)lParam)->iCtrlId)
                        break;
                if (aKeyWordIds[i] == 0)
                    break;

                WinHelp(((LPHELPINFO)lParam)->hItemHandle, NULL,
                        HELP_WM_HELP, (DWORD)(LPTSTR)aKeyWordIds);
                return TRUE;
            }
#endif /* LATER */

        case WM_COMMAND:
            switch(LOWORD(wParam)){
                BOOL f;

                case IDOK:
                    gwOptions &= OPT_SCALE;    // keep the Scale Mode

                    /* Take advantage of the fact that the button IDS are the */
                    /* same as the bit fields.                                */
                    for (w = OPT_FIRST; w <= OPT_LAST; w <<= 1)
                        if (IsDlgButtonChecked(hwnd, w))
                            gwOptions |= w;

                    if (IsDlgButtonChecked(hwnd, OPT_BAR))
                    {
                        GetWindowText(GetDlgItem(hwnd, IDC_TITLETEXT),
                                      gachCaption, CHAR_COUNT(gachCaption));

                        if (gachCaption[0])
                            gwOptions |= OPT_TITLE;
                        else
                            gwOptions &= ~OPT_TITLE;
                    }

                    DirtyObject(FALSE);
                    EndDialog(hwnd, TRUE);
                    break;

                case IDCANCEL:
                    EndDialog(hwnd, FALSE);
                    break;

                case OPT_BAR:
                    f = IsDlgButtonChecked(hwnd, OPT_BAR);
                    EnableWindow(GetDlgItem(hwnd, IDC_CAPTIONTEXT), f);
                    EnableWindow(GetDlgItem(hwnd, IDC_TITLETEXT), f);

                    if(f) {
                        SetWindowText(GetDlgItem(hwnd, IDC_TITLETEXT), gachCaption);
                    } else {
                        GetWindowText(GetDlgItem(hwnd, IDC_TITLETEXT),
                                      gachCaption, CHAR_COUNT(gachCaption));
                        SetWindowText(GetDlgItem(hwnd, IDC_TITLETEXT), TEXT(""));
                    }

                    break;
            }
    }
    return FALSE;
}


/*--------------------------------------------------------------+
| mciDialog - bring up the dialog for MCI Send Command          |
|                                                               |
+--------------------------------------------------------------*/
BOOL FAR PASCAL mciDialog(HWND hwnd)
{
    FARPROC fpfn;

    fpfn = MakeProcInstance((FARPROC)mciDlgProc, ghInst);
    DialogBox(ghInst, MAKEINTATOM(DLG_MCICOMMAND), hwnd, (DLGPROC)fpfn);
#ifndef WIN32
    FreeProcInstance(fpfn);
#endif
    return TRUE;    // should we check return value?
}

BOOL FAR PASCAL _EXPORT mciDlgProc(HWND hwnd, unsigned msg, WPARAM wParam, LPARAM lParam)
{
    TCHAR   ach[255];
    UINT    w;
    DWORD   dw;

    switch (msg)
    {
        case WM_INITDIALOG:
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
#ifdef WIN32
                    SendDlgItemMessage(hwnd, IDC_MCICOMMAND, EM_SETSEL, 0, (LPARAM)-1);
#else
                    SendDlgItemMessage(hwnd, IDC_MCICOMMAND, EM_SETSEL, 0, MAKELONG(0, -1));
#endif
                    w = GetDlgItemText(hwnd, IDC_MCICOMMAND, ach, CHAR_COUNT(ach));

                    dw = SendStringMCI(ach, ach, CHAR_COUNT(ach));

                    if (dw != 0)
                    {
                        mciGetErrorString(dw, ach, CHAR_COUNT(ach));
//                        Error1(hwnd, IDS_DEVICEERROR, (LPTSTR)ach);
                    }

                    SetDlgItemText(hwnd, IDC_RESULT, ach);

                    break;

                case IDCANCEL:
                    EndDialog(hwnd, FALSE);
                    break;
            }
            break;
    }

    return FALSE;
}
