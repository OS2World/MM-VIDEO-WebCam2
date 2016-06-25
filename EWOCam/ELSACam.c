/*****************************************************************************/
/*                                                                           */
/*  Programmname  : EWOCAM.C                                                 */
/*  Verwendung    : Einzelbildaufnahme mit ELSA Winner Office ViVo           */
/*  Bibliotheken  : MMPM                                                     */
/*  Autor         : Dipl.Ing. JÅrgen Dittmer                                 */
/*  Speichermodell: OS/2 PM                                                  */
/*  Compiler      : WATCOM C/C++ v. 10.0                                     */
/*  System        :                                                          */
/*  Erstellung    : 15 Jun 1998                                              */
/*  énderung      : 23 Jun 1998 - Grî·e verÑnderbar                          */
/*                                                                           */
/*****************************************************************************/

#define INCL_WIN
#define INCL_GPI
#define INCL_DOS
#define INCL_WINDIALOGS
#define INCL_GPIBITMAPS
#define INCL_DOSSEMAPHORES
#define INCL_DOSSESMGR
#include <os2.h>
#define INCL_MCIOS2
#define INCL_MMIOOS2
#include <os2me.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <process.h>
#include <time.h>
#include "elsacam.h"
#include "elsa.h"


/*********************/
/* Globale Variablen */
/*********************/

HWND  hwndClient;
HAB   hab;
HEV   hevUPDATEWIN;   /* Semaphore zum Neuzeichnen */
HEV   hevCAPTURE;     /* Semaphore zum Bild speichern */
PCHAR pBuf;           /* Bildpuffer */

PLUGIN_INITIALIZE  vin;
PPLUGIN_INITIALIZE pvIn = &vin;

PLUGIN_GETFRAME  frame;
PPLUGIN_GETFRAME pframe =&frame;

/* Configuration Parameters */
PROFIL  profil;
PPROFIL pprofil = &profil;

/* Save changed grab size until program saves it on exit */
GRABSIZE newsize;

ULONG ulYFrame, ulXFrame;   /* Pixelweiten fÅr Frame */

/*******************************************************************/
/* Initialisierung des PM, Erstellung der MSG-Queue und Semaphoren */
/*******************************************************************/


INT main (VOID)
{
HMQ  hmq;
QMSG qmsg;
HWND hwndClientFrame;

ULONG  ClientStyleFlags = FCF_TITLEBAR |
                          FCF_SYSMENU |
                          FCF_MENU |
                          FCF_ICON |
                          FCF_SIZEBORDER |
                          FCF_HIDEMAX |
                          FCF_TASKLIST;

    /*
     *  Wir wollen nur eine Instanz des Programms starten, beim zweiten Versuch wird
     *  hier abgebrochen, weil die Semaphoren nicht erzeugt werden kînnen
     */
    if (DosCreateEventSem (NULL, &hevUPDATEWIN, NULL, FALSE) |
        DosCreateEventSem ("\\SEM32\\WEBCAM\\CAPTURE", &hevCAPTURE, DC_SEM_SHARED, FALSE))
        exit (FALSE);

    ulXFrame = 2 * WinQuerySysValue (HWND_DESKTOP, SV_CXSIZEBORDER);
    ulYFrame = 2 * WinQuerySysValue (HWND_DESKTOP, SV_CYSIZEBORDER) +
                   WinQuerySysValue (HWND_DESKTOP, SV_CYTITLEBAR) +
                   WinQuerySysValue (HWND_DESKTOP, SV_CYMENU);

    profil.ulInterval   = 10;
    profil.ulBMPFile    = OVERWRITE;
    profil.swpclient.x  = 100;
    profil.swpclient.y  = 100;
    profil.swpclient.cx = 320 + ulXFrame;
  //profil.swpclient.cy = 240 + ulYFrame;
    profil.swpclient.fl = SWP_SIZE | SWP_SHOW | SWP_MOVE;
    profil.ulMonitor    = 0;
    profil.ulX_Size = 160;
    profil.ulY_Size = 120;
    ReadPrf (hab, &profil); /* Lesen der WEBCAM.INI */              
    /* AbbildungsverhÑltnis wiederherstellen */
    profil.swpclient.cy = ulYFrame + (profil.swpclient.cx - ulXFrame) *  profil.ulY_Size / profil.ulX_Size;
    newsize.ulX_Size = profil.ulX_Size;
    newsize.ulY_Size = profil.ulY_Size;

    hab = WinInitialize (0);
    hmq = WinCreateMsgQueue (hab,0);

    WinRegisterClass (hab, "Hauptfenster", ClientWndProc, CS_SIZEREDRAW, 0);

    hwndClientFrame = WinCreateStdWindow (HWND_DESKTOP,
       WS_ANIMATE,
       &ClientStyleFlags,
       "Hauptfenster",
       "EWOCam/2",
       WS_VISIBLE,
       (HMODULE)0,
       ID_ClientWindow,
       &hwndClient);                    

    WinSetWindowPos (hwndClientFrame, HWND_TOP, profil.swpclient.x, profil.swpclient.y,
        profil.swpclient.cx, profil.swpclient.cy, profil.swpclient.fl);

    while (WinGetMsg (hab,&qmsg,0L,0,0))
       WinDispatchMsg (hab,&qmsg);

    WinQueryWindowPos (hwndClientFrame, &profil.swpclient);               
    profil.ulX_Size = newsize.ulX_Size;
    profil.ulY_Size = newsize.ulY_Size;
    WritePrf (hab, &profil);  /* Sichern der EWOCAM.INI */
    WinDestroyWindow (hwndClientFrame);
    WinDestroyMsgQueue (hmq);
    WinTerminate (hab);
    return 0;
}


/*************************/
/*   Fensterprozeduren   */
/*************************/

MRESULT EXPENTRY ClientWndProc (HWND hwnd,ULONG msg,MPARAM mp1,MPARAM mp2)
{
HWND   hwndFrame;
HPS    hps;
char   szMsg[80];
ULONG  rc;
time_t time_of_day;    /* Time structure for title */

switch (msg)
    {
    case WM_CREATE:

        DosAllocMem ((PPVOID) &pBuf,
                     (ULONG) profil.ulX_Size * profil.ulY_Size * 3 + 3,                                           
                     (ULONG) PAG_COMMIT | PAG_READ | PAG_WRITE);

        vin.cbFix   = sizeof(PLUGIN_INITIALIZE);
        vin.ulflags = 0;
        vin.cx      = profil.ulX_Size;
        vin.cy      = profil.ulY_Size;
        vin.bpp     = 24;
        vin.hini    = 0;
        rc = Initialize (pvIn, sizeof(PLUGIN_INITIALIZE));
        //SetSize ();
        if (rc != 0) {
            WinMessageBox (HWND_DESKTOP, hwndClient, (PSZ) "Cannot init P2VideoIn driver!",
                (PSZ) "EWOCam/2 Problem!", 0, MB_OK | MB_MOVEABLE | MB_ERROR);
            WinPostMsg (hwnd,WM_QUIT,(MPARAM)0,(MPARAM)0);
        } else {
            _beginthread (Capture, NULL, 8192, NULL);
            _beginthread (Paint, NULL, 4096, NULL);
            if (profil.ulInterval > 0)
                WinStartTimer (hab, hwnd, ID_TIMER, 1000 * profil.ulInterval);  /* Trigger */
            if (profil.ulMonitor) WinStartTimer (hab, hwnd, ID_TIMER, 50);
        }
        break;

    case WM_DESTROY:
        WinStopTimer (hab, hwnd, ID_TIMER);
        Terminate();
        DosFreeMem (pBuf);
        break;

    case WM_PAINT:
        hwndFrame = WinQueryWindow (hwnd, QW_PARENT);
        time_of_day = time (NULL);
        strftime (szMsg, 80, "EWOCam/2 %a %b %d, %Y  %X" , localtime (&time_of_day));
        WinSetWindowText (hwndFrame, szMsg);
        hps = WinBeginPaint (hwnd, NULLHANDLE, NULL);
        /* Wir wollen die MsgQ nicht blockieren, also lassen wir von einem andern Thread */
        /* das Fenster zeichnen und gehen gleich weiter! */
        DosPostEventSem (hevUPDATEWIN);
        WinEndPaint (hps);
        break;              

    case WM_BUTTON1CLICK:
    case WM_TIMER:
        DosPostEventSem (hevCAPTURE);
        break;

    case WM_COMMAND:                                                     
        if (SHORT1FROMMP(mp2)==CMDSRC_MENU) {
            switch (SHORT1FROMMP(mp1)) {

                case IDM_SETTINGS:
                    WinDlgBox (HWND_DESKTOP, hwnd, EinstellDlgProc, (HMODULE) 0, DLG_SETTINGS, NULL);
                    WritePrf (hab, &profil);  /* Sichern der EWOCAM.INI */
                    if (profil.ulInterval > 0) {
                        WinStartTimer (hab, hwnd, ID_TIMER, 1000 * profil.ulInterval); /* Trigger */
                    } else {
                        WinStopTimer (hab, hwnd, ID_TIMER); /* Trigger aus */
                    } /* endif */
                    if (profil.ulMonitor) WinStartTimer (hab, hwnd, ID_TIMER, 50);
                    break;

                case IDM_CLICK:
                    DosPostEventSem (hevCAPTURE);
                    break;

                case IDM_ABOUT:
                    WinDlgBox (HWND_DESKTOP, hwnd, ProdinfoDlgProc, (HMODULE) 0, DLG_PRODINFO, NULL);
                    break;
            }
        }
        break;

    case WM_CLOSE:
        WinPostMsg (hwnd, WM_QUIT, (MPARAM)0, (MPARAM)0);
        break;

        break;

    default: 
        return WinDefWindowProc (hwnd, msg, mp1, mp2);
        break;
    }
    return (MRESULT)FALSE;
}


/***********************************/
/* Prozedur fÅr den Einstelldialog */
/***********************************/

MRESULT EXPENTRY EinstellDlgProc(HWND hwnd,ULONG msg,MPARAM mp1,MPARAM mp2)
{
SHORT hours, minutes, seconds;
BOOL  bMonitor;
static ULONG ulOldX_Size;

    switch (msg)
    {
        case WM_INITDLG:
        {
            hours = (INT) profil.ulInterval / 3600;
            minutes = (INT) (profil.ulInterval - hours * 3600) / 60;
            seconds = profil.ulInterval % 60;
            WinSendDlgItemMsg (hwnd, DLG_SP_INTERVAL_HOURS,   SPBM_SETLIMITS, (MPARAM) 24, (MPARAM) 0);
            WinSendDlgItemMsg (hwnd, DLG_SP_INTERVAL_HOURS,   SPBM_SETCURRENTVALUE, MPFROMLONG (hours), NULL);
            WinSendDlgItemMsg (hwnd, DLG_SP_INTERVAL_MINUTES, SPBM_SETLIMITS, (MPARAM) 59, (MPARAM) 0);
            WinSendDlgItemMsg (hwnd, DLG_SP_INTERVAL_MINUTES, SPBM_SETCURRENTVALUE, MPFROMLONG (minutes), NULL);
            WinSendDlgItemMsg (hwnd, DLG_SP_INTERVAL_SECONDS, SPBM_SETLIMITS, (MPARAM) 59, (MPARAM) 0);
            WinSendDlgItemMsg (hwnd, DLG_SP_INTERVAL_SECONDS, SPBM_SETCURRENTVALUE, MPFROMLONG (seconds), NULL);
            WinSendDlgItemMsg (hwnd, DLG_CK_MONITOR,          BM_SETCHECK, MPFROMSHORT (profil.ulMonitor), MPVOID);
            ulOldX_Size = profil.ulX_Size;

            /* Einen Radiobutton vorbesetzen */
            switch (profil.ulBMPFile) 
            {
                case OVERWRITE:
                    WinPostMsg (WinWindowFromID (hwnd, DLG_RB_FILENAME_WEBCAM),
                        BM_CLICK, MPFROMSHORT (TRUE) ,MPVOID);
                    break;
                case DATE_TIME:
                    WinPostMsg (WinWindowFromID (hwnd, DLG_RB_FILENAME_DATETIME),
                        BM_CLICK, MPFROMSHORT (TRUE), MPVOID);
                    break;
                case TIME_NAME:
                    WinPostMsg (WinWindowFromID (hwnd, DLG_RB_FILENAME_TIME),
                        BM_CLICK, MPFROMSHORT (TRUE), MPVOID);
                    break;
            }

            /* Einen Radiobutton vorbesetzen */
            switch (newsize.ulX_Size) 
            {                                                       
                case 160:
                    WinPostMsg (WinWindowFromID (hwnd, DLG_RB_RES_160),
                        BM_CLICK, MPFROMSHORT (TRUE) ,MPVOID);
                    break;
                case 320:
                    WinPostMsg (WinWindowFromID (hwnd, DLG_RB_RES_320),
                        BM_CLICK, MPFROMSHORT (TRUE), MPVOID);
                    break;
                case 640:
                    WinPostMsg (WinWindowFromID (hwnd, DLG_RB_RES_640),
                        BM_CLICK, MPFROMSHORT (TRUE), MPVOID);
                    break;
            }
            break;
        }

        case WM_CONTROL:
            switch (SHORT2FROMMP (mp1))
            {
                case BN_CLICKED:
                    bMonitor = (BOOL) !WinSendDlgItemMsg (hwnd, DLG_CK_MONITOR, BM_QUERYCHECK, NULL, NULL);
                    WinEnableControl (hwnd, DLG_RB_FILENAME_WEBCAM,   bMonitor);
                    WinEnableControl (hwnd, DLG_RB_FILENAME_DATETIME, bMonitor);
                    WinEnableControl (hwnd, DLG_RB_FILENAME_TIME,     bMonitor);
                    WinEnableControl (hwnd, DLG_SP_INTERVAL_HOURS,    bMonitor);
                    WinEnableControl (hwnd, DLG_SP_INTERVAL_MINUTES,  bMonitor);
                    WinEnableControl (hwnd, DLG_SP_INTERVAL_SECONDS,  bMonitor);

                    switch (SHORT1FROMMP (mp1))
                    {
                        case DLG_RB_FILENAME_WEBCAM:
                            profil.ulBMPFile = OVERWRITE;
                            break;
                        case DLG_RB_FILENAME_DATETIME:
                            profil.ulBMPFile = DATE_TIME;
                            break;
                        case DLG_RB_FILENAME_TIME:
                            profil.ulBMPFile = TIME_NAME;
                            break;
                        case DLG_RB_RES_160:
                            newsize.ulX_Size = 160;
                            newsize.ulY_Size = 120;
                            break;
                        case DLG_RB_RES_320:
                            newsize.ulX_Size = 320;
                            newsize.ulY_Size = 240;
                            break;
                        case DLG_RB_RES_640:
                            newsize.ulX_Size = 640;
                            newsize.ulY_Size = 480;
                            break;
                    }
                    break;
            }
            break;

        case WM_COMMAND:
            if (SHORT1FROMMP (mp1) == DID_OK){
                WinSendDlgItemMsg (hwnd, DLG_SP_INTERVAL_HOURS,
                        SPBM_QUERYVALUE, (MPARAM) &hours,
                        MPFROM2SHORT (NULL, SPBQ_ALWAYSUPDATE));
                WinSendDlgItemMsg (hwnd, DLG_SP_INTERVAL_MINUTES,
                        SPBM_QUERYVALUE, (MPARAM) &minutes,
                        MPFROM2SHORT (NULL, SPBQ_ALWAYSUPDATE));
                WinSendDlgItemMsg (hwnd, DLG_SP_INTERVAL_SECONDS,
                        SPBM_QUERYVALUE, (MPARAM) &seconds,
                        MPFROM2SHORT (NULL, SPBQ_ALWAYSUPDATE));
                profil.ulInterval = hours * 3600 + minutes * 60 + seconds;
                profil.ulMonitor = (ULONG) WinSendDlgItemMsg (hwnd, DLG_CK_MONITOR, BM_QUERYCHECK, NULL, NULL);
                if(ulOldX_Size != profil.ulX_Size){
                    WinMessageBox (HWND_DESKTOP, hwnd, (PSZ) "Change of image size will take effect on next start",
                        (PSZ) "Information", 0, MB_OK | MB_MOVEABLE | MB_INFORMATION);
                    ulOldX_Size = profil.ulX_Size;
                }
                WinDismissDlg (hwnd, DID_OK);
            }
            break;

        case WM_CLOSE:
            WinDismissDlg (hwnd, DID_OK);
            break;

        default:
            return WinDefDlgProc (hwnd, msg, mp1, mp2);
            break;
    }
    return (MRESULT)FALSE;
}


/************************************************/
/* Neuzeichnen des Fensters im Separatem Thread */
/************************************************/

void Paint (void *arg)                                      
{                
HPS               hps;
ULONG             postcount;
HBITMAP           hbmBmp;
BITMAPINFOHEADER2 bmih;
RECTL             dest;

    arg = arg;
    memset ((PVOID)&bmih, 0, sizeof (BITMAPINFOHEADER2));
    bmih.cbFix     = sizeof (BITMAPINFOHEADER2);
    bmih.cx        = profil.ulX_Size;                        
    bmih.cy        = profil.ulY_Size;
    bmih.cBitCount = 24;
    bmih.cPlanes   = 1;

    for (;;) {
        DosWaitEventSem (hevUPDATEWIN, SEM_INDEFINITE_WAIT);  /* Warten bis wir was tun sollen */
        WinQueryWindowRect (WinQueryWindow (hwndClient, QW_PARENT), &dest);
        dest.xRight -= ulXFrame;
        dest.yTop   -= ulYFrame;
        //printf ("X: %d  Y: %d\n", dest.xRight, dest.yTop);
        hps = WinGetPS (hwndClient);
        hbmBmp = GpiCreateBitmap (hps, &bmih, CBM_INIT, pBuf, (BITMAPINFO2 *) &bmih);
        WinDrawBitmap (hps, hbmBmp, NULL, (PPOINTL)&dest, 0L, 0L, DBM_STRETCH);
//        WinDrawBitmap (hps, hbmBmp, NULL, (PPOINTL)&dest, 0L, 0L, DBM_NORMAL);
        GpiDeleteBitmap (hbmBmp);
        WinReleasePS (hps);
        DosResetEventSem (hevUPDATEWIN, &postcount);          /* Zwischenzeitlich eingetroffene WM_PAINTs ignorieren */
    } /* forever */
}


/******************************************/
/* Name : BMPCaptureBitmap                */
/*                                        */
/* Function: Capture bitmap from hardware */
/*                                        */
/******************************************/

VOID Capture (VOID *arg)
{   
HFILE             hBMP;
ULONG             ulAction;
ULONG             cBytes;    
ULONG             ulError;
ULONG             postcount;
CHAR              szCommand[CCHMAXPATH];
CHAR              szFilename[CCHMAXPATH];
//CHAR              szJPGFile[CCHMAXPATH];
time_t            time_of_day;    /* Time structure for filename */
PPROGDETAILS      pDetails;
BITMAPFILEHEADER2 bmph;

    arg=arg;
    pDetails = (PPROGDETAILS) malloc( sizeof(PROGDETAILS) );
    pDetails->Length                      = sizeof(PROGDETAILS);
    pDetails->progt.progc                 = PROG_WINDOWABLEVIO;
    pDetails->progt.fbVisible             = SHE_INVISIBLE;
    pDetails->pszTitle                    = "WCAMEXIT";
    pDetails->pszExecutable               = "WCAMEXIT.CMD";
    pDetails->pszParameters               = szCommand;
    pDetails->pszStartupDir               = NULL;
    pDetails->pszIcon                     = NULL;
    pDetails->pszEnvironment              = "\0\0";
    pDetails->swpInitial.fl               = SWP_HIDE; /* Window positioning   */
    pDetails->swpInitial.cy               = 0;        /* Width of window      */
    pDetails->swpInitial.cx               = 0;        /* Height of window     */
    pDetails->swpInitial.y                = 0;        /* Lower edge of window */
    pDetails->swpInitial.x                = 0;        /* Left edge of window  */
    pDetails->swpInitial.hwndInsertBehind = HWND_TOP;
    pDetails->swpInitial.ulReserved1      = 0;
    pDetails->swpInitial.ulReserved2      = 0;

    frame.cbFix = sizeof(PLUGIN_GETFRAME);
    frame.ulflags = 0;
    frame.pBuffer = (PVOID)pBuf;
    frame.ulLen = profil.ulX_Size * profil.ulY_Size * 3;
    frame.hps = 0;

    memset (&bmph, 0, sizeof(BITMAPFILEHEADER2));
    bmph.usType  = 0x4D42;    /* BM */
    bmph.cbSize  = sizeof(BITMAPFILEHEADER2);
    bmph.offBits = sizeof(BITMAPFILEHEADER2);

    bmph.bmp2.cbFix           = sizeof(BITMAPINFOHEADER2);
    bmph.bmp2.cPlanes         = 1;
    bmph.bmp2.cBitCount       = 24;
    bmph.bmp2.cx              = profil.ulX_Size;
    bmph.bmp2.cy              = profil.ulY_Size;
    bmph.bmp2.ulCompression   = BCA_UNCOMP;
    bmph.bmp2.usRecording     = BRA_BOTTOMUP;
    bmph.bmp2.usRendering     = BRH_NOTHALFTONED;
    bmph.bmp2.ulColorEncoding = BCE_RGB;

    DosPostEventSem (hevCAPTURE);
    for (;;) {
        DosWaitEventSem (hevCAPTURE, SEM_INDEFINITE_WAIT);  /* Warten bis wir was tun sollen */
        time_of_day = time (NULL);
        ulError = GetFrame (pframe, sizeof(PLUGIN_GETFRAME));

        if (!ulError) {

            /* getimage buffer is successful open file and write out bitmap */
            switch (profil.ulBMPFile) {
            case OVERWRITE:
               sprintf (szFilename, "WEBCAM.BMP");
               //sprintf (szJPGFile,  "WEBCAM.JPG");
               sprintf (szCommand, " WEBCAM");
               break;
            case DATE_TIME:
               strftime (szFilename, 20, "%Y%m%d-%H%M%S.BMP", localtime (&time_of_day));
               //strftime (szJPGFile,  20, "%Y%m%d-%H%M%S.JPG", localtime (&time_of_day));
               strftime (szCommand, 20, " %Y%m%d-%H%M%S", localtime (&time_of_day));
               break;
            case TIME_NAME:
               strftime (szFilename, 20, "%H%M%S.BMP", localtime (&time_of_day));
               //strftime (szJPGFile,  20, "%H%M%S.JPG", localtime (&time_of_day));
               strftime (szCommand, 20, " %H%M%S", localtime (&time_of_day));
               break;
            default:
              break;
            } /* endswitch */

            if (!profil.ulMonitor){ /* Nicht wenn Monitor an */
                DosOpen (szFilename, &hBMP, &ulAction, 0, FILE_NORMAL,
                   FILE_CREATE | OPEN_ACTION_REPLACE_IF_EXISTS,
                   OPEN_ACCESS_WRITEONLY | OPEN_SHARE_DENYWRITE,
                   0L);
                DosWrite (hBMP, &bmph, sizeof(BITMAPFILEHEADER2), &cBytes);
                DosWrite (hBMP, (PVOID)pBuf, profil.ulX_Size * profil.ulY_Size * 3, &cBytes);
                DosClose (hBMP);

            //ulError = bmp2jpg (szFilename, szJPGFile);
            //if(!ulError) remove (szFilename);

                WinStartApp(NULL,
                            pDetails,
                            NULL,
                            NULL,
                            SAF_STARTCHILDAPP |
                            SAF_INSTALLEDCMDLINE);
            } /* End If Monitor */
        }
        WinInvalidateRect (hwndClient, NULL, TRUE); /* Fenster neu zeichnen, WM_PAINT */
        DosResetEventSem (hevCAPTURE, &postcount); /* Zwischenzeitlich eingetroffene Semaphoren ignorieren */
    } /* forever */
//    return (ulError);
}


/************************************/
/* Prozedur fÅr die Produktinfo-Box */
/************************************/

MRESULT EXPENTRY ProdinfoDlgProc(HWND hwnd,ULONG msg,MPARAM mp1,MPARAM mp2)          
{                
                 
    switch (msg) 
    {            
        case WM_COMMAND:                     
           if (SHORT1FROMMP (mp1) == DID_OK) 
               WinDismissDlg (hwnd, DID_OK); 
           break;                            
                                             
        case WM_CLOSE:                       
           WinDismissDlg (hwnd, DID_OK);     
           break;
                 
        default: 
           return WinDefDlgProc (hwnd, msg, mp1, mp2);                                  
           break; 
    }             
    return (MRESULT)FALSE;                                                           
} 

