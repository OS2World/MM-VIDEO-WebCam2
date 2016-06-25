/*****************************************************************************/
/*                                                                           */
/*  Programmname  : PMWEBCAM.C                                               */
/*  Verwendung    : Einzelbildaufnahme mit MM2                               */
/*  Bibliotheken  : MMPM                                                     */
/*  Autor         : Dipl.Ing. JÅrgen Dittmer                                 */
/*  Speichermodell: OS/2 PM                                                  */
/*  Compiler      : WATCOM C/C++ v. 10.0                                     */
/*  System        :                                                          */
/*  Erstellung    : 28 Jul 1997                                              */
/*  énderung      :                                                          */
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
#include "webcam.h"


/*********************/
/* Globale Variablen */
/*********************/

HWND            hwndClient;
HAB             hab;
HEV             hevUPDATEWIN;   /* Semaphore zum Neuzeichnen */
HEV             hevCAPTURE;     /* Semaphore zum Bild speichern */
MCI_IMAGE_PARMS mciImageParms;  /* Imageparameter von der Videohardware */
PCHAR           pBuf;           /* Bildpuffer */
USHORT          usDeviceID;     /* Assigned by MCI_OPEN */
USHORT          usVideoDevices; /* Number of total digitalvideos installed */
PSZ             pszRegion[] = {"AUS",
                               "CCIR",
                               "CCIRCATV",
                               "JAPAN",
                               "JPNCATV",
                               "UK",
                               "USA",
                               "USACATV"};
PSZ       pszDigitalVideo[] = {"Digitalvideo01",
                               "Digitalvideo02",
                               "Digitalvideo03",
                               "Digitalvideo04",
                               "Digitalvideo05",
                               "Digitalvideo06",
                               "Digitalvideo07",
                               "Digitalvideo08",
                               "Digitalvideo09"};
/* Configuration Parameters */
PROFIL  profil;
PPROFIL pprofil = &profil;


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
                          FCF_DLGBORDER |
                          FCF_HIDEBUTTON |
                          FCF_TASKLIST;
    /*
     *  Wir wollen nur eine Instanz des Programms starten, beim zweiten Versuch wird
     *  hier abgebrochen, weil die Semaphoren nicht erzeugt werden kînnen
     */
    if (DosCreateEventSem (NULL, &hevUPDATEWIN, NULL, FALSE) |
        DosCreateEventSem ("\\SEM32\\WEBCAM\\CAPTURE", &hevCAPTURE, DC_SEM_SHARED, FALSE))
        exit (FALSE);

    profil.ulRegionIndex = 2;
    profil.ulConnectorIndex = 2;
    profil.ulTVChannel = 9;
    profil.ulDigitalVideo = 3;
    profil.ulInterval = 10;
    profil.ulBMPFile = OVERWRITE;
    profil.swpclient.x = 100;
    profil.swpclient.y = 100;
    profil.swpclient.cx = 300;
    profil.swpclient.cy = 200;
    profil.swpclient.fl = SWP_SIZE | SWP_SHOW | SWP_MOVE;
    ReadPrf (hab, &profil); /* Lesen der WEBCAM.INI */              

    hab = WinInitialize (0);
    hmq = WinCreateMsgQueue (hab,0);

    WinRegisterClass (hab, "Hauptfenster", ClientWndProc, CS_SIZEREDRAW, 0);

    hwndClientFrame = WinCreateStdWindow (HWND_DESKTOP,
       WS_ANIMATE,
       &ClientStyleFlags,
       "Hauptfenster",
       "PMWebCam",
       WS_VISIBLE,
       (HMODULE)0,
       ID_ClientWindow,
       &hwndClient);                    

    WinSetWindowPos (hwndClientFrame, HWND_TOP, profil.swpclient.x, profil.swpclient.y,
        profil.swpclient.cx, profil.swpclient.cy, profil.swpclient.fl);

    while (WinGetMsg (hab,&qmsg,0L,0,0))
       WinDispatchMsg (hab,&qmsg);

    WinQueryWindowPos (hwndClientFrame, &profil.swpclient);               
    WritePrf (hab, &profil);  /* Sichern der PMWATCH.INI */
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
        rc = InitTV ();
        //if(rc != 0) ShowMMErrorBox (rc);                            
        rc = SetInput ();
        if(rc != 0){
            ShowMMErrorBox (rc);
            WinDlgBox (HWND_DESKTOP, hwnd, EinstellDlgProc, (HMODULE) 0, DLG_SETTINGS, NULL);
            WinPostMsg (hwnd,WM_QUIT,(MPARAM)0,(MPARAM)0);
        } else {
            _beginthread (Capture, NULL, 8192, NULL);
            _beginthread (Paint, NULL, 4096, NULL);
            if (profil.ulInterval > 0)
                WinStartTimer (hab, hwnd, ID_TIMER, 1000 * profil.ulInterval);  /* Trigger */
        }
        break;

    case WM_DESTROY:
        DosFreeMem (pBuf);
        CloseTv ();
        break;

    case WM_PAINT:
        hwndFrame = WinQueryWindow (hwnd, QW_PARENT);
        WinSetWindowPos (hwndFrame, 0, 0, 0,
                         mciImageParms.ulPelBufferWidth,
                         mciImageParms.ulPelBufferHeight +
                         WinQuerySysValue (HWND_DESKTOP, SV_CYTITLEBAR) +
                         WinQuerySysValue (HWND_DESKTOP, SV_CYMENU),
                         SWP_SIZE);
        time_of_day = time (NULL);
        strftime (szMsg, 80, "WebCam/2 %a %b %d, %Y  %X" , localtime (&time_of_day));
        WinSetWindowText (hwndFrame, szMsg);
        hps = WinBeginPaint (hwnd, NULLHANDLE, NULL);
        /*
         * Wir wollen die MsgQ nicht blockieren, also lassen wir von einem andern Thread
         * das Fenster zeichnen und gehen gleich weiter!
         */
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
                case IDM_EXIT:
                    if (WinMessageBox (HWND_DESKTOP, hwnd, "Do you want to exit WebCam?", "Question:", 0,
                        MB_QUERY | MB_YESNO) == MBID_YES) {
                            WinPostMsg (hwnd, WM_CLOSE, (MPARAM) 0,(MPARAM) 0);
                            return (MRESULT) 0;
                    }
                    break;

                case IDM_SETTINGS:
                    WinDlgBox (HWND_DESKTOP, hwnd, EinstellDlgProc, (HMODULE) 0, DLG_SETTINGS, NULL);
                    if (profil.ulInterval > 0) {
                        WinStartTimer (hab, hwnd, ID_TIMER, 1000 * profil.ulInterval);  /* Trigger */
                    } else {
                        WinStopTimer (hab, hwnd, ID_TIMER);  /* Trigger aus */
                    } /* endif */
                    SetInput ();
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
        WinPostMsg (hwnd,WM_QUIT,(MPARAM)0,(MPARAM)0);
        break;

    default: 
        return WinDefWindowProc (hwnd,msg,mp1,mp2);
        break;
    }
    return (MRESULT)FALSE;
}


VOID ShowMMErrorBox (ULONG mmrc)
{
CHAR szErrorText[128];

    mciGetErrorString (mmrc, szErrorText, sizeof (szErrorText));
    WinMessageBox (HWND_DESKTOP, hwndClient, (PSZ) szErrorText,
        (PSZ) "Multimediasystem error! Please Change Settings and Restart WebCam/2!", 0, MB_OK | MB_MOVEABLE);
}


/***********************************/
/* Prozedur fÅr den Einstelldialog */
/***********************************/

MRESULT EXPENTRY EinstellDlgProc(HWND hwnd,ULONG msg,MPARAM mp1,MPARAM mp2)
{
SHORT hours, minutes, seconds;

    switch (msg)
    {
        case WM_INITDLG:
        {
           hours = (INT) profil.ulInterval / 3600;
           minutes = (INT) (profil.ulInterval - hours * 3600) / 60;
           seconds = profil.ulInterval % 60;
           WinSendDlgItemMsg (hwnd, DLG_SP_INTERVAL_HOURS, SPBM_SETLIMITS, (MPARAM) 24, (MPARAM) 0);
           WinSendDlgItemMsg (hwnd, DLG_SP_INTERVAL_HOURS, SPBM_SETCURRENTVALUE, MPFROMLONG (hours), NULL);
           WinSendDlgItemMsg (hwnd, DLG_SP_INTERVAL_MINUTES, SPBM_SETLIMITS, (MPARAM) 59, (MPARAM) 0);
           WinSendDlgItemMsg (hwnd, DLG_SP_INTERVAL_MINUTES, SPBM_SETCURRENTVALUE, MPFROMLONG (minutes), NULL);
           WinSendDlgItemMsg (hwnd, DLG_SP_INTERVAL_SECONDS, SPBM_SETLIMITS, (MPARAM) 59, (MPARAM) 0);
           WinSendDlgItemMsg (hwnd, DLG_SP_INTERVAL_SECONDS, SPBM_SETCURRENTVALUE, MPFROMLONG (seconds), NULL);
           WinSendDlgItemMsg (hwnd, DLG_SP_CHANNEL, SPBM_SETLIMITS, (MPARAM) 99, (MPARAM) 0);
           WinSendDlgItemMsg (hwnd, DLG_SP_CHANNEL, SPBM_SETCURRENTVALUE, MPFROMLONG (profil.ulTVChannel), NULL);
           WinSendDlgItemMsg (hwnd, DLG_SP_CONNECTOR, SPBM_SETLIMITS, (MPARAM) 4, (MPARAM) 0);
           WinSendDlgItemMsg (hwnd, DLG_SP_CONNECTOR, SPBM_SETCURRENTVALUE, MPFROMLONG (profil.ulConnectorIndex), NULL);
           WinSendDlgItemMsg (hwnd, DLG_SP_REGION, SPBM_SETARRAY, &pszRegion, MPFROMSHORT (8));
           WinSendDlgItemMsg (hwnd, DLG_SP_REGION, SPBM_SETCURRENTVALUE, MPFROMLONG (profil.ulRegionIndex), NULL);
           WinSendDlgItemMsg (hwnd, DLG_SP_DIGITALVIDEO, SPBM_SETARRAY, &pszDigitalVideo, MPFROMSHORT (usVideoDevices));
           WinSendDlgItemMsg (hwnd, DLG_SP_DIGITALVIDEO, SPBM_SETCURRENTVALUE, MPFROMLONG (profil.ulDigitalVideo), NULL);

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
            break;
        }

        case WM_CONTROL:
            switch (SHORT2FROMMP (mp1))
            {
                case BN_CLICKED:
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
                WinSendDlgItemMsg (hwnd, DLG_SP_CHANNEL,
                        SPBM_QUERYVALUE, (MPARAM) &pprofil->ulTVChannel,
                        MPFROM2SHORT (NULL, SPBQ_ALWAYSUPDATE));
                WinSendDlgItemMsg (hwnd, DLG_SP_CONNECTOR,
                        SPBM_QUERYVALUE, (MPARAM) &pprofil->ulConnectorIndex,
                        MPFROM2SHORT (NULL, SPBQ_ALWAYSUPDATE));
                WinSendDlgItemMsg (hwnd, DLG_SP_REGION,
                        SPBM_QUERYVALUE, (MPARAM) &pprofil->ulRegionIndex,
                        MPFROM2SHORT (NULL, SPBQ_ALWAYSUPDATE));
                WinSendDlgItemMsg (hwnd, DLG_SP_DIGITALVIDEO,
                        SPBM_QUERYVALUE, (MPARAM) &pprofil->ulDigitalVideo,
                        MPFROM2SHORT (NULL, SPBQ_ALWAYSUPDATE));
                WinDismissDlg (hwnd, DID_OK);
            }
            break;

        case WM_CLOSE:
            WinDismissDlg (hwnd,DID_OK);
            break;

        default:
            return WinDefDlgProc (hwnd,msg,mp1,mp2);
            break;
    }
    return (MRESULT)FALSE;
}


/************************************************/
/* Neuzeichnen des Fensters im Separatem Thread */
/************************************************/

void Paint (void *arg)                                      
{                
HPS     hps;
ULONG   postcount;
HBITMAP hbmBmp;
BITMAPINFOHEADER2 bmih;
POINTL  ptl;

    ptl.x = 0;
    ptl.y = 0;
    arg = arg;
    for (;;) {
        DosWaitEventSem (hevUPDATEWIN, SEM_INDEFINITE_WAIT);  /* Warten bis wir was tun sollen */
        memset ((PVOID)&bmih, 0, sizeof (BITMAPINFOHEADER2));
        bmih.cbFix = sizeof (BITMAPINFOHEADER2);
        bmih.cx = mciImageParms.ulPelBufferWidth;
        bmih.cy = mciImageParms.ulPelBufferHeight;
        bmih.cBitCount = 24;
        bmih.cPlanes = 1;
        hps = WinGetPS (hwndClient);
        hbmBmp = GpiCreateBitmap (hps, &bmih, CBM_INIT, pBuf+78, (BITMAPINFO2 *) &bmih);
        WinDrawBitmap (hps, hbmBmp, NULL, &ptl, 0L, 0L, DBM_NORMAL);
        GpiDeleteBitmap (hbmBmp);
        WinReleasePS (hps);
        DosResetEventSem (hevUPDATEWIN, &postcount);          /* Zwischenzeitlich eingetroffene WM_PAINTs ignorieren */
    } /* forever */
}


/****************************************************************************/
/*                                                                          */
/* Name          : InitTv                                                   */
/*                                                                          */
/* Description   : Opens the appropriate digital video device.              */
/*                                                                          */
/* Return        : ULONG   - 0 = success, 1 = fail                          */
/*                                                                          */
/****************************************************************************/

ULONG InitTv (VOID)
{
ULONG             ulError;
MCI_OPEN_PARMS    mciOpenParms;
MCI_SYSINFO_PARMS SysInfo;
#define           RETBUFSIZE 10
CHAR              SysInfoRet[RETBUFSIZE];

    /* Query numbers of Digitalvideo devices */
    memset( &SysInfo, 0, sizeof (SysInfo));
    SysInfo.usDeviceType = MCI_DEVTYPE_DIGITAL_VIDEO;
    SysInfo.pszReturn = (PSZ) &SysInfoRet;                                                
    SysInfo.ulRetSize = RETBUFSIZE;
    mciSendCommand ((USHORT) 0,
                     MCI_SYSINFO,
                     MCI_SYSINFO_QUANTITY |
                     MCI_WAIT,
                     (PVOID) &SysInfo, 0);
    usVideoDevices = atoi (SysInfoRet);

    memset( &mciOpenParms, 0, sizeof (mciOpenParms));
    mciOpenParms.pszDeviceType = pszDigitalVideo[profil.ulDigitalVideo];

    /* Open the required DigitalVideo Device */
    ulError = mciSendCommand ((USHORT) 0,
                                MCI_OPEN,
                                MCI_WAIT |
                                MCI_OPEN_SHAREABLE,
                                (PVOID) &mciOpenParms,
                                0);

    usDeviceID = mciOpenParms.usDeviceID;

    /* Set up capture struct and allocate BMP memory */
    memset ((PVOID)&mciImageParms, 0, sizeof (MCI_IMAGE_PARMS));
    /* prepare structures */
    //mciImageParms.pPelBuffer   = 0L;
    //mciImageParms.ulBufLen     = 0L;
    //mciImageParms.rect.xLeft   = 100;
    //mciImageParms.rect.yBottom = 100;
    //mciImageParms.rect.xRight  = 400;
    //mciImageParms.rect.yTop    = 300;
    //mciImageParms.ulPelBufferWidth  = 200;
    //mciImageParms.ulPelBufferHeight = 100;
    mciSendCommand (usDeviceID,
                    MCI_GETIMAGEBUFFER,
                    MCI_WAIT | MCI_USE_HW_BUFFER | MCI_CONVERT,
                    (PVOID) &mciImageParms,
                    0);
    DosAllocMem ((PPVOID) &pBuf,
                 (ULONG) mciImageParms.ulBufLen,
                 (ULONG) PAG_COMMIT | PAG_READ | PAG_WRITE);
    mciImageParms.pPelBuffer=(PVOID)pBuf;
    return (ulError);
} /* endfunc InitTv */


/****************************************************************************/
/*                                                                          */
/* Name          : SetInput                                                 */
/*                                                                          */
/* Description   : Opens the appropriate digital video device.              */
/*                                                                          */
/* Return        : ULONG   - 0 = success, 1 = fail                          */
/*                                                                          */
/****************************************************************************/

ULONG SetInput (VOID)
{                    
ULONG               ulError;
MCI_CONNECTOR_PARMS mciConnectorParms;
MCI_DGV_TUNER_PARMS settuner;

    memset( &mciConnectorParms, 0, sizeof (mciConnectorParms));
    mciConnectorParms.ulConnectorType = MCI_VIDEO_IN_CONNECTOR;
    mciConnectorParms.ulConnectorIndex  = profil.ulConnectorIndex;

    ulError = mciSendCommand (usDeviceID,
                                MCI_CONNECTOR,
                                MCI_WAIT |
                                MCI_ENABLE_CONNECTOR |
                                MCI_CONNECTOR_TYPE |
                                MCI_CONNECTOR_INDEX,
                                (PVOID) &mciConnectorParms,
                                0);

    memset( &settuner, 0, sizeof (settuner));
    settuner.ulFrequency = 0;
    settuner.pszRegion = pszRegion[profil.ulRegionIndex];
    settuner.ulTVChannel = profil.ulTVChannel;
    settuner.lFineTune = 0;

    /* ulError = */
    mciSendCommand (usDeviceID,
                                MCI_SETTUNER,
                                MCI_WAIT |
                                MCI_DGV_REGION |
                                MCI_DGV_TV_CHANNEL,
                                (PVOID) &settuner,
                                0);

    DosSleep (700);
    return (ulError);
} /* endfunc InitTv */

/****************************************************************************
 *                                                                          *
 * Name          : CloseTv                                                  *
 *                                                                          *
 * Description   : Closes the digital video device.                         *
 *                                                                          *
 *                                                                          *
 * Parameters    : hwnd    -  Client window handle                          *
 *                 pad     -  Application data                              *
 *                                                                          *
 * Return        : None.                                                    *
 *                                                                          *
 ****************************************************************************/

VOID CloseTv (VOID)
{
ULONG             ulError;
MCI_GENERIC_PARMS mciGenericParms;

   mciGenericParms.hwndCallback = (HWND) NULL;

   ulError = mciSendCommand (usDeviceID,
                             MCI_CLOSE,
                             MCI_WAIT,
                             (PVOID) &mciGenericParms,
                             NULL);

} /* endfunc CloseTv */


/******************************************/
/* Name : BMPCaptureBitmap                */
/*                                        */
/* Function: Capture bitmap from hardware */
/*                                        */
/******************************************/

VOID Capture (VOID *arg)
{   
HFILE  hBMP;
ULONG  ulAction;
ULONG  cBytes;
ULONG  ulError;
ULONG  postcount;
CHAR   szCommand[20];
CHAR   szFilename[20];
time_t time_of_day;    /* Time structure for filename */
PPROGDETAILS pDetails;

    arg=arg;
    pDetails = (PPROGDETAILS) malloc( sizeof(PROGDETAILS) );   /* Allocate structure */
    pDetails->Length                      = sizeof(PROGDETAILS);
    pDetails->progt.progc                 = PROG_WINDOWABLEVIO;
    pDetails->progt.fbVisible             = SHE_INVISIBLE;
    pDetails->pszTitle                    = "WCAMEXIT";
    pDetails->pszExecutable               = "WCAMEXIT.CMD";
    pDetails->pszParameters               = szCommand;
    pDetails->pszStartupDir               = NULL;
    pDetails->pszIcon                     = NULL;
    pDetails->pszEnvironment              = "\0\0";
    pDetails->swpInitial.fl               = SWP_HIDE;   /* Window positioning   */
    pDetails->swpInitial.cy               = 0;          /* Width of window      */
    pDetails->swpInitial.cx               = 0;          /* Height of window     */
    pDetails->swpInitial.y                = 0;          /* Lower edge of window */
    pDetails->swpInitial.x                = 0;          /* Left edge of window  */
    pDetails->swpInitial.hwndInsertBehind = HWND_TOP;
    pDetails->swpInitial.ulReserved1      = 0;
    pDetails->swpInitial.ulReserved2      = 0;

    DosPostEventSem (hevCAPTURE);
    for (;;) {
        DosWaitEventSem (hevCAPTURE, SEM_INDEFINITE_WAIT);  /* Warten bis wir was tun sollen */
        time_of_day = time (NULL);
        ulError = mciSendCommand (usDeviceID,              
                            MCI_GETIMAGEBUFFER,
                            MCI_WAIT | MCI_USE_HW_BUFFER | MCI_CONVERT,
                            (PVOID) &mciImageParms,
                            0);

        if (!ulError) {

            /* getimage buffer is successful open file and write out bitmap */
            switch (profil.ulBMPFile) {
            case OVERWRITE:
               sprintf (szFilename, "WEBCAM.BMP");
               sprintf (szCommand, " WEBCAM");
               break;
            case DATE_TIME:
               strftime (szFilename, 20, "%Y%m%d-%H%M%S.BMP", localtime (&time_of_day));
               strftime (szCommand, 20, " %Y%m%d-%H%M%S", localtime (&time_of_day));
               break;
            case TIME_NAME:
               strftime (szFilename, 20, "%H%M%S.BMP", localtime (&time_of_day));
               strftime (szCommand, 20, " %H%M%S", localtime (&time_of_day));
               break;
            default:
              break;
            } /* endswitch */

            DosOpen (szFilename, &hBMP, &ulAction, 0, FILE_NORMAL,
               FILE_CREATE | OPEN_ACTION_REPLACE_IF_EXISTS,
               OPEN_ACCESS_WRITEONLY | OPEN_SHARE_DENYWRITE,
               0L);
            DosWrite (hBMP, (PVOID)pBuf,
               mciImageParms.ulBufLen,
               &cBytes);
            DosClose (hBMP);

            WinStartApp(NULL,
                        pDetails,
                        NULL,
                        NULL,
                        SAF_STARTCHILDAPP |
                        SAF_INSTALLEDCMDLINE);

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
           return WinDefDlgProc (hwnd,msg,mp1,mp2);                                  
           break; 
    }             
    return (MRESULT)FALSE;                                                           
} 

