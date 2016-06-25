/*
**
** MODULE:   ELSAUGI.C
**
** AUTHOR:   Hartmut Bohn (hbohn), (c) ELSA GmbH
**
** PROJECT:  Example of the VSD_USER_GETIMAGE
**           interface of the P2VideoIn device
**
** INTERF:
**
** DESCR:     This isn't valid C code and should
**            show the essential functionallity, only
**
** NOTES:
**
** REV:
**
**
**
**
*/


#define INCL_DOS
#define INCL_PM
#define INCL_DOSDEVIOCTL
#define INCL_MCIOS2
#include <os2.h>
#include <os2me.h>

#include <vsdcmds.h>

//

#define VSD_USER_GETIMAGE (11)

//commands
#define VSDUGI_INIT      0
#define VSDUGI_GET       1
#define VSDUGI_RELEASE   2
#define VSDUGI_CLOSE     3
//flags
#define VSDUGI_COLOR     0x1

typedef struct VSD_USER_GETIMAGE_PARAMS{    /* GETIMAGE */
   ULONG     ulLength;
   ULONG     ulFlags;
   ULONG     ulCommand;
   ULONG     fourcc;
   PVOID     pBuffer;
   ULONG     ulBufferLen;
   CRECT     irectDest;
   ULONG     ulActiveBuffer;  //retun value
} VSD_USER_GETIMAGE_PARAMS;
typedef VSD_USER_GETIMAGE_PARAMS FAR * PVSD_USER_GETIMAGE_PARAMS;
#define VSD_USER_GETIMAGE_Len      sizeof(VSD_USER_GETIMAGE_PARAMS)



BOOL FindDevice(VOID);
typedef struct
{
   ULONG        hvsd;                  //device ID of open device
   CHAR         szDeviceName [CCHMAXPATH];   //logical name of the device (DigitalvideoXX)
   CHAR         szVSDDriver  [CCHMAXPATH];   //vidvsd.dll
   CHAR         szPDDName    [CCHMAXPATH];   //vidvcix$
   PFNVSDENTRY  VSDEntry;                    //vsdentryfunction
   ULONG        ulFlags;
   ULONG        ulcx;
   ULONG        ulcy;
   ULONG        ulbpp;
}DEVICE_PARAMS;

DEVICE_PARAMS device={0,"","","",NULL,0,0,0};


/*****************************************************************************
 *
 *  FUNCTION NAME:    FindDevice
 *
 *
 *  DESCRIPTIVE NAME:
 *
 *  FUNCTION:         initilazes global device struct
 *
 *  ENTRY POINT:
 *
 *  INPUT:
 *
 *  OUTPUT:
 *
 *  EXIT-NORMAL:      TRUE, device was found and everything is ok
 *
 *  EXIT-ERROR:       FALSE, can't use device
 *
 *  EFFECTS:
 *
 *  NOTES:
 *
 *  INTERNAL REFERENCES:
 *    ROUTINES:
 *
 *  EXTERNAL REFERENCES:
 *    ROUTINES:
 *
 ****************************************************************************/



BOOL FindDevice()
{
  BOOL frc    = FALSE;
  MCI_SYSINFO_PARMS      Sysinfo;
  MCI_SYSINFO_QUERY_NAME SysNames;
  MCI_SYSINFO_LOGDEVICE  SysDrv;
  ULONG   ulrc;
  INT     i    = 0;
  HMODULE hmod;

#if defined (DEBUG)
  printf("ELSAPLUG >> FindDevive\n");
#endif

  //init global struct
  device.hvsd = 0;
  device.szDeviceName[0] = 0;
  device.szVSDDriver[0] = 0;
  device.szPDDName[0] = 0;
  device.VSDEntry = 0;

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
                          (USHORT) NULL,                      // device ID
                          (USHORT) MCI_SYSINFO,               // command
                          (ULONG)  MCI_SYSINFO_ITEM,         // flag
                          (PVOID)  &Sysinfo,                 // command buffer
                          (USHORT) NULL );

#if defined (DEBUG)
     printf("rc=%d szLogicalName=%s szInstallName=%s\n",ulrc,SysNames.szLogicalName,SysNames.szInstallName);
#endif
     //It's my device?
     if ( (ulrc==MCIERR_SUCCESS) && (!strcmp(SysNames.szInstallName , "P2VIDEOIN")) )
     {
         memset ((PCH)&Sysinfo, 0, sizeof(MCI_SYSINFO_PARMS));
         Sysinfo.ulItem = MCI_SYSINFO_QUERY_DRIVER;
         Sysinfo.pSysInfoParm = &SysDrv;
         memset ((PCH)&SysDrv, 0, sizeof(MCI_SYSINFO_LOGDEVICE));
         strcpy( SysDrv.szInstallName, SysNames.szInstallName );

         ulrc =  mciSendCommand (
                      (USHORT) NULL,                      // device ID for all devices
                      (USHORT) MCI_SYSINFO,               // command message to return
                      (ULONG)  MCI_SYSINFO_ITEM,         // flag for device inst. names
                      (PVOID)  &Sysinfo,                 // address of devices struct.
                      (USHORT) NULL );

         if (ulrc==MCIERR_SUCCESS)
         {
            //update global device struct
            strcpy (device.szDeviceName, SysNames.szInstallName );
            strcpy (device.szVSDDriver,  SysDrv.szVSDDriver);
            strcpy (device.szPDDName,    SysDrv.szPDDName);

            //Query vsd's entry function
            if (DosLoadModule( NULL, 0, device.szVSDDriver, &hmod ) == NO_ERROR )
            {
               if (DosQueryProcAddr ( hmod, 0, "VSD_Entry", (PFN *)&device.VSDEntry) == NO_ERROR)
               {
                  //now we really succeed!!
                  frc = TRUE;
               }
            }

         }//endif MCI_SYSINFO_QUERY_DRIVER was successful
     }//endif its' my device

  } while( (ulrc==MCIERR_SUCCESS) && (!frc) );

  //strcpy(device.szDeviceName,"Digitalvideo03");
  return frc;

}



/*****************************************************************************
 *
 *  FUNCTION NAME:    Initialize
 *
 *
 *  DESCRIPTIVE NAME:
 *
 *  FUNCTION:
 *
 *  ENTRY POINT:
 *
 *  INPUT:
 *
 *          input and output are the same pointer !!
 *
 *  OUTPUT:
 *
 *          typedef struct
 *          {
 *            ULONG cbFix;
 *            ULONG ulflags;                         // should be 0 for now
 *            ULONG cx;    // size of image eg 160x120
 *            ULONG cy;
 *            ULONG bpp;      // color depth
 *            HINI hini;      // handle of Initialization file
 *          } PLUGIN_INITIALIZE,*PPLUGIN_INITIALIZE;
 *
 *
 *  EXIT-NORMAL:
 *
 *  EXIT-ERROR:
 *
 *  EFFECTS:
 *
 *  NOTES:
 *
 *  INTERNAL REFERENCES:
 *    ROUTINES:
 *
 *  EXTERNAL REFERENCES:
 *    ROUTINES:
 *
 ****************************************************************************/


ULONG Initialize(ULONG ulCmd,PVOID pvIN,ULONG ulIn,PVOID pvOut,ULONG ulOut)

{
   PPLUGIN_INITIALIZE pplug = (PPLUGIN_INITIALIZE) pvIN;
   ULONG rc = 87;
   VSD_OPEN_PARMS           vsd_open_params;
   VSD_USER_PARMS           vsd_user_params;
   VSD_USER_GETIMAGE_PARAMS vsd_user_getimage_params;
   ULONG ulrc;


  device.ulcx=pplug->cx;
  device.ulcy=pplug->cy;
  device.ulbpp=pplug->bpp;



  if (pvIN && ulIn >= sizeof(PLUGIN_INITIALIZE))
  {
     if (FindDevice())
     {
        vsd_open_params.ulLength= VSD_OPEN_Len;
        vsd_open_params.hvsd= 0;
        vsd_open_params.ulCategory= 0;
        vsd_open_params.pfEvent= 0;
        strcpy(vsd_open_params.szPDDName, device.szPDDName);

        ulrc = device.VSDEntry( NULL, VSD_OPEN, VSD_NONE, &vsd_open_params );
        if (ulrc==VSDERR_SUCCESS)
        {
          device.hvsd = (ULONG)vsd_open_params.hvsd;

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

          ulrc = device.VSDEntry( (PVOID)device.hvsd,
                         VSD_USER,
                         VSD_USER_GETIMAGE,
                         &vsd_user_params );

          if (ulrc==VSDERR_SUCCESS)
          {
             rc = 0;
          }
        }
     }//endif (FindDevice())
  }//endif correct parameters


  return rc;
}

/*****************************************************************************
 *
 *  FUNCTION NAME:    Terminate
 *
 *
 *  DESCRIPTIVE NAME:
 *
 *  FUNCTION:
 *
 *  ENTRY POINT:
 *
 *  INPUT:
 *
 *         PPLUGIN_GENEREL
 *
 *  OUTPUT:
 *
 *      typedef struct
 *      {
 *         ULONG cbFix;
 *         ULONG ulflags;                         // should be 0 for now
 *
 *      } PLUGIN_TERMINATE,*PPLUGIN_TERMINATE;
 *
 *
 *  EXIT-NORMAL:
 *
 *  EXIT-ERROR:
 *
 *  EFFECTS:
 *
 *  NOTES:
 *
 *  INTERNAL REFERENCES:
 *    ROUTINES:
 *
 *  EXTERNAL REFERENCES:
 *    ROUTINES:
 *
 ****************************************************************************/


ULONG Terminate(ULONG ulCmd,PVOID pvIN,ULONG ulIn,PVOID pvOut,ULONG ulOut)

{
   PPLUGIN_TERMINATE pplug = (PPLUGIN_TERMINATE) pvIN;
   ULONG rc = 87;

   VSD_USER_PARMS           vsd_user_params;
   VSD_USER_GETIMAGE_PARAMS vsd_user_getimage_params;

   if (pvIN && ulIn >= sizeof(PLUGIN_TERMINATE))
   {


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
      //vsd_capture_params.irectDest //not used for framegrabber cards

      if (device.VSDEntry) {
         rc = device.VSDEntry( (PVOID)device.hvsd,
                               VSD_USER,
                               VSD_USER_GETIMAGE,
                               &vsd_user_params );

         rc = device.VSDEntry( (PVOID)device.hvsd,
                               VSD_CLOSE,
                               VSD_NONE,
                               NULL );
      }

      rc = 0;
  }

  return rc;
}

/*****************************************************************************
 *
 *  FUNCTION NAME:    GetFrame
 *
 *
 *  DESCRIPTIVE NAME:
 *
 *  FUNCTION:
 *
 *  ENTRY POINT:
 *
 *  INPUT:
 *
 *        typedef struct
 *        {
 *            ULONG cbFix;
 *            ULONG ulflags;                         // should be 0 for now
 *            PVOID pBuffer;
 *            ULONG ulLen;
 *            HPS   hps;
 *        } PLUGIN_GETFRAME,*PPLUGIN_GETFRAME;
 *
 *  OUTPUT:
 *
 *        Zero
 *
 *  EXIT-NORMAL:
 *
 *  EXIT-ERROR:
 *
 *  EFFECTS:
 *
 *  NOTES:
 *
 *  INTERNAL REFERENCES:
 *    ROUTINES:
 *
 *  EXTERNAL REFERENCES:
 *    ROUTINES:
 *
 ****************************************************************************/



ULONG GetFrame(ULONG ulCmd,PVOID pvIN,ULONG ulIn,PVOID pvOut,ULONG ulOut)

{

   PPLUGIN_GETFRAME pplug = (PPLUGIN_GETFRAME) pvIN;
   ULONG rc = 87;

   VSD_USER_PARMS           vsd_user_params;
   VSD_USER_GETIMAGE_PARAMS vsd_user_getimage_params;

   if (pvIN && ulIn >= sizeof(PLUGIN_GETFRAME))
   {

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
      //vsd_capture_params.irectDest //not used for framegrabber cards

      if (device.VSDEntry) {
         rc = device.VSDEntry( (PVOID)device.hvsd,
                               VSD_USER,
                               VSD_USER_GETIMAGE,
                               &vsd_user_params );
      }


  }


  return rc;

}



