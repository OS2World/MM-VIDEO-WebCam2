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
#include "webcam.h"


int ReadPrf (HAB hab, PPROFIL pprofil)
{
HINI  hini; /* ini file handle */
ULONG ulSWP    = sizeof (SWP);
ULONG ulULONG  = sizeof (ULONG);
//ULONG ulUSHORT = sizeof (USHORT);                                                             

    hini = PrfOpenProfile (hab, "WEBCAM.INI");
    if (PrfQueryProfileData (hini, "WEBCAM", "CLIENTSWP",    &pprofil->swpclient,        &ulSWP) &
        PrfQueryProfileData (hini, "WEBCAM", "INTERVAL",     &pprofil->ulInterval,       &ulULONG) &
        PrfQueryProfileData (hini, "WEBCAM", "REGION",       &pprofil->ulRegionIndex,    &ulULONG) &
        PrfQueryProfileData (hini, "WEBCAM", "CONNECTOR",    &pprofil->ulConnectorIndex, &ulULONG) &
        PrfQueryProfileData (hini, "WEBCAM", "CHANNEL",      &pprofil->ulTVChannel,      &ulULONG) &
        PrfQueryProfileData (hini, "WEBCAM", "BMPFILE",      &pprofil->ulBMPFile,        &ulULONG) &
        PrfQueryProfileData (hini, "WEBCAM", "DIGITALVIDEO", &pprofil->ulDigitalVideo,   &ulULONG) &
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

    hini = PrfOpenProfile (hab, "WEBCAM.INI");
    PrfWriteProfileData (hini, "WEBCAM", "CLIENTSWP",    &pprofil->swpclient,        ulSWP);
    PrfWriteProfileData (hini, "WEBCAM", "INTERVAL",     &pprofil->ulInterval,       ulULONG);
    PrfWriteProfileData (hini, "WEBCAM", "REGION",       &pprofil->ulRegionIndex,    ulULONG);
    PrfWriteProfileData (hini, "WEBCAM", "CONNECTOR",    &pprofil->ulConnectorIndex, ulULONG);
    PrfWriteProfileData (hini, "WEBCAM", "CHANNEL",      &pprofil->ulTVChannel,      ulULONG);
    PrfWriteProfileData (hini, "WEBCAM", "BMPFILE",      &pprofil->ulBMPFile,        ulULONG);
    PrfWriteProfileData (hini, "WEBCAM", "DIGITALVIDEO", &pprofil->ulDigitalVideo,   ulULONG);
    PrfCloseProfile (hini);
    return (0);
}
