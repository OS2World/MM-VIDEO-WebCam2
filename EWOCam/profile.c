/*****************************************************************************/
/*                                                                           */
/*  Programmname  : PROFILE.C                                                */
/*  Verwendung    : Lesen und Schreiben der Profil-Datei (WEBCAM.INI)        */
/*  Bibliotheken  :                                                          */
/*  Autor         : Dipl.Ing. JÅrgen Dittmer                                 */
/*  Speichermodell: OS/2 PM                                                  */
/*  Compiler      : WATCOM C/C++ v. 10.0                                     */
/*  System        :                                                          */
/*  Erstellung    : 8 Aug 1997                                               */
/*  énderung      :                                                          */
/*                                                                           */
/*****************************************************************************/

#define INCL_WINSHELLDATA
#include <os2.h>
#include "elsacam.h"


int ReadPrf (HAB hab, PPROFIL pprofil)
{
HINI  hini; /* ini file handle */
ULONG ulSWP    = sizeof (SWP);
ULONG ulULONG  = sizeof (ULONG);
//ULONG ulUSHORT = sizeof (USHORT);                                                             

    hini = PrfOpenProfile (hab, "EWOCAM.INI");
    if (PrfQueryProfileData (hini, "EWOCAM", "CLIENTSWP", &pprofil->swpclient,  &ulSWP)   &
        PrfQueryProfileData (hini, "EWOCAM", "INTERVAL",  &pprofil->ulInterval, &ulULONG) &
        PrfQueryProfileData (hini, "EWOCAM", "BMPFILE",   &pprofil->ulBMPFile,  &ulULONG) &
        PrfQueryProfileData (hini, "EWOCAM", "MONITOR",   &pprofil->ulMonitor,  &ulULONG) &
        PrfQueryProfileData (hini, "EWOCAM", "X_SIZE",    &pprofil->ulX_Size,   &ulULONG) &
        PrfQueryProfileData (hini, "EWOCAM", "Y_SIZE",    &pprofil->ulY_Size,   &ulULONG) &
        PrfCloseProfile (hini))
        return (0);
    else
        return (1);
}

int WritePrf (HAB hab, PPROFIL pprofil)
{
HINI  hini; /* ini file handle */
ULONG ulSWP    = sizeof (SWP);
ULONG ulULONG  = sizeof (ULONG);
//ULONG ulUSHORT = sizeof (USHORT);

    hini = PrfOpenProfile (hab, "EWOCAM.INI");
    PrfWriteProfileData (hini, "EWOCAM", "CLIENTSWP", &pprofil->swpclient,  ulSWP);
    PrfWriteProfileData (hini, "EWOCAM", "INTERVAL",  &pprofil->ulInterval, ulULONG);
    PrfWriteProfileData (hini, "EWOCAM", "BMPFILE",   &pprofil->ulBMPFile,  ulULONG);
    PrfWriteProfileData (hini, "EWOCAM", "MONITOR",   &pprofil->ulMonitor,  ulULONG);
    PrfWriteProfileData (hini, "EWOCAM", "X_SIZE",    &pprofil->ulX_Size,   ulULONG);
    PrfWriteProfileData (hini, "EWOCAM", "Y_SIZE",    &pprofil->ulY_Size,   ulULONG);
    PrfCloseProfile (hini);
    return (0);
}
