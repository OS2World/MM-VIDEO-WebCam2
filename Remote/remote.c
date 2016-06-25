#define INCL_DOSSEMAPHORES
#define INCL_DOSPROCESS
#include <os2.h>
#include <stdio.h>

int main(void)
{
HEV    hev = 0;
APIRET rc;
ULONG postcount;
int n;

    rc = DosOpenEventSem ("\\SEM32\\WEBCAM\\CAPTURE", &hev);
    if (rc != 0) {
    //    puts ("WebCam/2 not running!");
    //    printf ("rc=%d\n", rc);
        DosBeep (300, 200);
        return (rc);
    }

    DosPostEventSem (hev);
    DosBeep (4000, 50);
    for (n = 0; n < 50; n++) {  /* Max 10 Sekunden warten bis WebCam fertig */
        DosSleep (200);
        DosQueryEventSem (hev, &postcount);
//        printf ("Semaphore postcount: %d\n", postcount);
        if (postcount == 0) break;
    } /* endfor */

    DosCloseEventSem (hev);
    return (0);
}
