#define ID_ClientWindow 1001
#define ID_TIMER        1002
#define IDM_SETTINGS    1003
#define IDM_CLICK       1005
#define IDM_ABOUT       1006
#define OVERWRITE       0
#define DATE_TIME       1
#define TIME_NAME       2

VOID  Paint         (VOID*);
VOID  Capture       (VOID*);
INT   bmp2jpg       (PSZ infile, PSZ outfile);

MRESULT EXPENTRY ClientWndProc   (HWND hwnd,ULONG msg,MPARAM mp1,MPARAM mp2);
MRESULT EXPENTRY EinstellDlgProc (HWND hwnd,ULONG msg,MPARAM mp1,MPARAM mp2);
MRESULT EXPENTRY ProdinfoDlgProc (HWND hwnd,ULONG msg,MPARAM mp1,MPARAM mp2);

typedef struct{
    SWP   swpclient;
    ULONG ulInterval;
    ULONG ulBMPFile;
    ULONG ulMonitor;
    ULONG ulX_Size;
    ULONG ulY_Size;
    } PROFIL, *PPROFIL;

typedef struct{
    ULONG ulX_Size;
    ULONG ulY_Size;
    } GRABSIZE;

int ReadPrf  (HAB, PPROFIL); 
int WritePrf (HAB, PPROFIL); 

#define DLG_SP_INTERVAL_HOURS       105
#define DLG_RB_FILENAME_WEBCAM      106
#define DLG_SP_INTERVAL_MINUTES     108
#define DLG_SP_INTERVAL_SECONDS     109
#define DLG_SETTINGS                100
#define DLG_RB_FILENAME_DATETIME    107
#define DLG_RB_FILENAME_TIME        110
#define DLG_PRODINFO                200
#define DLG_PRODINFO_ICO            201

#define DLG_CK_MONITOR              101
#define DLG_RB_RES_160              102
#define DLG_RB_RES_320              103
#define DLG_RB_RES_640              104
