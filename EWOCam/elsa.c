/*****************************************************************************/
/*                                                                           */
/*  Programmname  : ELSA.C                                                   */
/*  Verwendung    : ELSA-P2VideoIn                                           */
/*  Bibliotheken  :                                                          */
/*  Autor         : ELSA, angepasst von JÅrgen Dittmer                       */
/*  Speichermodell: FLAT                                                     */
/*  Compiler      : WATCOM C/C++ v. 10.0                                     */
/*  System        : OS/2                                                     */
/*  Erstellung    :  8 Jun 1998                                              */
/*  énderung      : 24 Jun 1998 - VSD_SETVIDEORECT                           */
/*                                                                           */
/*****************************************************************************/

#define INCL_DOSMODULEMGR
#define INCL_MCIOS2
#define INCL_MMIOOS2
#include <os2.h>
#include <os2me.h>
#include <fourcc.h>
#include <vsdcmds.h>

#include <string.h>
#include <stdio.h>
#include "elsa.h"

/* Globals */
DEVICE_PARAMS device = {0, "", "" ,"" ,NULL ,0 ,0 ,0 };


/****************************************************************/
/* FUNCTION NAME: FindDevice                                    */
/* FUNCTION:      initilazes global device struct               */
/* EXIT-NORMAL:   TRUE, device was found and everything is ok   */
/* EXIT-ERROR:    FALSE, can't use device                       */
/****************************************************************/

BOOL FindDevice()
{
BOOL frc = FALSE;
MCI_SYSINFO_PARMS      Sysinfo;
MCI_SYSINFO_QUERY_NAME SysNames;
MCI_SYSINFO_LOGDEVICE  SysDrv;
ULONG   ulrc;
INT     i = 0;
HMODULE hmod;

    //init global struct
    device.hvsd            = 0;
    device.szDeviceName[0] = 0;
    device.szVSDDriver[0]  = 0;
    device.szPDDName[0]    = 0;
    device.VSDEntry        = 0;

    memset ((PCH)&Sysinfo, 0, sizeof(MCI_SYSINFO_PARMS));
    Sysinfo.ulItem       = MCI_SYSINFO_QUERY_NAMES;
    Sysinfo.pSysInfoParm = &SysNames;

    //Main loop on all Digitalvideodevices
    do {
        i++;
        memset ((PCH)&SysNames, 0, sizeof(MCI_SYSINFO_QUERY_NAME));
        sprintf (SysNames.szLogicalName, "Digitalvideo%02d",i);
        //Query the InstallName -> P2VideoIn
        ulrc = mciSendCommand (
                          (USHORT) NULL,                // device ID
                          (USHORT) MCI_SYSINFO,         // command
                          (ULONG)  MCI_SYSINFO_ITEM,    // flag
                          (PVOID)  &Sysinfo,            // command buffer
                          (USHORT) NULL);

        //It's my device?
//        printf("No: %d %s %s %s\n", i, SysNames.szInstallName, SysNames.szLogicalName, SysNames.szAliasName);
        if ((ulrc==MCIERR_SUCCESS) && (!strcmp(SysNames.szInstallName , "P2VIDEOIN"))){
            memset ((PCH)&Sysinfo, 0, sizeof(MCI_SYSINFO_PARMS));
            Sysinfo.ulItem = MCI_SYSINFO_QUERY_DRIVER;
            Sysinfo.pSysInfoParm = &SysDrv;
            memset ((PCH)&SysDrv, 0, sizeof(MCI_SYSINFO_LOGDEVICE));
            strcpy (SysDrv.szInstallName, SysNames.szInstallName);

            ulrc =  mciSendCommand (
                      (USHORT) NULL,                // device ID for all devices
                      (USHORT) MCI_SYSINFO,         // command message to return
                      (ULONG)  MCI_SYSINFO_ITEM,    // flag for device inst. names
                      (PVOID)  &Sysinfo,            // address of devices struct.
                      (USHORT) NULL);

            if (ulrc==MCIERR_SUCCESS){
                //update global device struct
                strcpy (device.szDeviceName, SysNames.szInstallName );
                strcpy (device.szVSDDriver,  SysDrv.szVSDDriver);
                strcpy (device.szPDDName,    SysDrv.szPDDName);

                //Query vsd's entry function
                if (DosLoadModule (NULL, 0, device.szVSDDriver, &hmod ) == NO_ERROR){
                    if (DosQueryProcAddr (hmod, 0, "VSD_Entry", (PFN *)&device.VSDEntry) == NO_ERROR){
                        //now we really succeed!!
                        frc = TRUE;
                    }
                }
            }//endif MCI_SYSINFO_QUERY_DRIVER was successful
        }//endif its' my device
    } while ((ulrc==MCIERR_SUCCESS) && (!frc));
    return frc;
}


/************************************************************************/
/*                                                                      */
/*  FUNCTION NAME: Initialize                                           */
/*                                                                      */
/*  INPUT:         input and output are the same pointer !!             */
/*                                                                      */
/*  OUTPUT:        typedef struct{                                      */
/*                  ULONG cbFix;                                        */
/*                  ULONG ulflags;  // should be 0 for now              */
/*                  ULONG cx;       // size of image eg 160x120         */
/*                  ULONG cy;                                           */
/*                  ULONG bpp;      // color depth                      */
/*                  HINI hini;      // handle of Initialization file    */
/*                  } PLUGIN_INITIALIZE,*PPLUGIN_INITIALIZE;            */
/*                                                                      */
/*  EXIT-NORMAL:                                                        */
/*  EXIT-ERROR:                                                         */
/************************************************************************/

ULONG Initialize(PVOID pvIN, ULONG ulIn)
{
PPLUGIN_INITIALIZE pplug = (PPLUGIN_INITIALIZE) pvIN;
ULONG rc = 87;
VSD_OPEN_PARMS           vsd_open_params;
VSD_USER_PARMS           vsd_user_params;
VSD_USER_GETIMAGE_PARAMS vsd_user_getimage_params;
VSD_VIDEORECT_PARMS      vsd_videorect_params;

    device.ulcx  = pplug->cx;
    device.ulcy  = pplug->cy;
    device.ulbpp = pplug->bpp;

    if (pvIN && ulIn >= sizeof(PLUGIN_INITIALIZE)){
        if (FindDevice()){
            vsd_open_params.ulLength   = VSD_OPEN_Len;
            vsd_open_params.hvsd       = 0;
            vsd_open_params.ulCategory = 0;
            vsd_open_params.pfEvent    = 0;
            strcpy(vsd_open_params.szPDDName, device.szPDDName);

            rc = device.VSDEntry (NULL, VSD_OPEN, VSD_NONE, &vsd_open_params);
            if (rc == VSDERR_SUCCESS){
                device.hvsd = (ULONG)vsd_open_params.hvsd;

                vsd_videorect_params.ulLength = VSD_VIDEORECT_Len; /* length of the structure */
                vsd_videorect_params.irectSource.X_left   = 0;     /* Portion of Source rectangle to use */
                vsd_videorect_params.irectSource.Y_top    = 0;
                vsd_videorect_params.irectSource.X_width  = device.ulcx;
                vsd_videorect_params.irectSource.Y_height = device.ulcy;
                vsd_videorect_params.irectDest.X_left     = 0;     /* Size of Destination rectangle */
                vsd_videorect_params.irectDest.Y_top      = 0;
                vsd_videorect_params.irectDest.X_width    = device.ulcx;
                vsd_videorect_params.irectDest.Y_height   = device.ulcy;

                rc = device.VSDEntry ((PVOID)device.hvsd,
                        VSD_SET,
                        VSD_SETVIDEORECT,
                        &vsd_videorect_params);


                vsd_user_params.ulLength      = VSD_USER_Len;
                vsd_user_params.ulLenUserInfo = VSD_USER_GETIMAGE_Len;
                vsd_user_params.pUserInfo     = &vsd_user_getimage_params;

                vsd_user_getimage_params.ulLength           = VSD_USER_GETIMAGE_Len;
                vsd_user_getimage_params.fourcc             = FOURCC_BGR3;
                vsd_user_getimage_params.ulCommand          = VSDUGI_INIT;
                vsd_user_getimage_params.ulFlags            = device.ulFlags;
                vsd_user_getimage_params.ulBufferLen        = 0;
                vsd_user_getimage_params.pBuffer            = 0;
                vsd_user_getimage_params.irectDest.X_Left   = 0;
                vsd_user_getimage_params.irectDest.Y_Top    = 0;
                vsd_user_getimage_params.irectDest.X_Width  = device.ulcx;
                vsd_user_getimage_params.irectDest.Y_Height = device.ulcy;

                rc = device.VSDEntry ((PVOID)device.hvsd,
                         VSD_USER,                   
                         VSD_USER_GETIMAGE,
                         &vsd_user_params);
            }
        }//endif (FindDevice())
    }//endif correct parameters

  return (rc);
}


/*************************************************************/
/*                                                           */
/*  FUNCTION NAME: GetFrame                                  */
/*  INPUT:         typedef struct                            */
/*                 {                                         */
/*                     ULONG cbFix;                          */
/*                     ULONG ulflags; // should be 0 for now */
/*                     PVOID pBuffer;                        */
/*                     ULONG ulLen;                          */
/*                     HPS   hps;                            */
/*                 } PLUGIN_GETFRAME,*PPLUGIN_GETFRAME;      */
/*                                                           */
/*************************************************************/

ULONG GetFrame(PVOID pvIN, ULONG ulIn)
{
PPLUGIN_GETFRAME pplug = (PPLUGIN_GETFRAME) pvIN;
ULONG rc = 87;

VSD_USER_PARMS           vsd_user_params;
VSD_USER_GETIMAGE_PARAMS vsd_user_getimage_params;

    if (pvIN && ulIn >= sizeof(PLUGIN_GETFRAME)){
        vsd_user_params.ulLength      = VSD_USER_Len;
        vsd_user_params.ulLenUserInfo = VSD_USER_GETIMAGE_Len;
        vsd_user_params.pUserInfo     = &vsd_user_getimage_params;

        vsd_user_getimage_params.ulLength           = VSD_USER_GETIMAGE_Len;
        vsd_user_getimage_params.ulCommand          = VSDUGI_GET;
        vsd_user_getimage_params.ulFlags            = device.ulFlags;
        vsd_user_getimage_params.fourcc             = FOURCC_BGR3;
        vsd_user_getimage_params.ulBufferLen        = pplug->ulLen;
        vsd_user_getimage_params.pBuffer            = pplug->pBuffer;
        vsd_user_getimage_params.irectDest.X_Left   = 0;
        vsd_user_getimage_params.irectDest.Y_Top    = 0;
        vsd_user_getimage_params.irectDest.X_Width  = device.ulcx;
        vsd_user_getimage_params.irectDest.Y_Height = device.ulcy;

        if (device.VSDEntry) {
            rc = device.VSDEntry((PVOID)device.hvsd,
                               VSD_USER,
                               VSD_USER_GETIMAGE,
                               &vsd_user_params );
        }
    }
    return rc;
}


/********************************/
/*  FUNCTION NAME:    Terminate */
/********************************/

ULONG Terminate (void)
{
ULONG rc = 87;

VSD_USER_PARMS           vsd_user_params;
VSD_USER_GETIMAGE_PARAMS vsd_user_getimage_params;

    vsd_user_params.ulLength      = VSD_USER_Len;
    vsd_user_params.ulLenUserInfo = VSD_USER_GETIMAGE_Len;
    vsd_user_params.pUserInfo     = &vsd_user_getimage_params;

    vsd_user_getimage_params.ulLength           = VSD_USER_GETIMAGE_Len;
    vsd_user_getimage_params.ulCommand          = VSDUGI_CLOSE;
    vsd_user_getimage_params.ulFlags            = device.ulFlags;
    vsd_user_getimage_params.fourcc             = FOURCC_BGR3;
    vsd_user_getimage_params.ulBufferLen        = 0;
    vsd_user_getimage_params.pBuffer            = NULL;
    vsd_user_getimage_params.irectDest.X_Left   = 0;
    vsd_user_getimage_params.irectDest.Y_Top    = 0;
    vsd_user_getimage_params.irectDest.X_Width  = device.ulcx;
    vsd_user_getimage_params.irectDest.Y_Height = device.ulcy;

    if (device.VSDEntry){
        rc = device.VSDEntry ((PVOID)device.hvsd,
                           VSD_USER,
                           VSD_USER_GETIMAGE,
                           &vsd_user_params);

        rc = device.VSDEntry ((PVOID)device.hvsd,
                           VSD_CLOSE,
                           VSD_NONE,
                           NULL);
    }
    rc = 0;
    return rc;
}

