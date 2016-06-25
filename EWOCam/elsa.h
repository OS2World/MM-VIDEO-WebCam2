#include <os2.h>
#include <vsdcmds.h>

BOOL  FindDevice (VOID);
ULONG Initialize (PVOID pvIN, ULONG ulIn);
ULONG GetFrame   (PVOID pvIN, ULONG ulIn);
ULONG Terminate  (void);

#define VSD_USER_GETIMAGE (11)

//commands
#define VSDUGI_INIT      0
#define VSDUGI_GET       1
#define VSDUGI_RELEASE   2
#define VSDUGI_CLOSE     3
//flags
#define VSDUGI_COLOR     0x1

typedef struct
{
    ULONG        hvsd;                      //device ID of open device
    CHAR         szDeviceName [CCHMAXPATH]; //logical name of the device (DigitalvideoXX)
    CHAR         szVSDDriver  [CCHMAXPATH]; //vidvsd.dll
    CHAR         szPDDName    [CCHMAXPATH]; //vidvcix$
    PFNVSDENTRY  VSDEntry;                  //vsdentryfunction
    ULONG        ulFlags;
    ULONG        ulcx;
    ULONG        ulcy;
    ULONG        ulbpp;
} DEVICE_PARAMS;

typedef struct
{
    ULONG cbFix;
    ULONG ulflags; // should be 0 for now
    ULONG cx;      // size of image eg 160x120
    ULONG cy;
    ULONG bpp;     // color depth
    HINI  hini;    // handle of Initialization file
} PLUGIN_INITIALIZE,*PPLUGIN_INITIALIZE;

/* Common Cature Rectangle Define */
typedef struct CRECT {     /* CR */
   ULONG X_Left;
   ULONG Y_Top;
   ULONG Y_Height;
   ULONG X_Width;
} CRECT;

typedef struct
{
    ULONG ulLength;
    ULONG ulFlags;
    ULONG ulCommand;
    ULONG fourcc;
    PVOID pBuffer;
    ULONG ulBufferLen;
    CRECT irectDest;
    ULONG ulActiveBuffer;  //return value
} VSD_USER_GETIMAGE_PARAMS;
#define VSD_USER_GETIMAGE_Len  sizeof(VSD_USER_GETIMAGE_PARAMS)

typedef struct
{
    ULONG cbFix;
    ULONG ulflags;   // should be 0 for now
    PVOID pBuffer;
    ULONG ulLen;
    HPS   hps;
} PLUGIN_GETFRAME,*PPLUGIN_GETFRAME;

