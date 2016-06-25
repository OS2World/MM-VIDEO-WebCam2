/********************************************/
/* File Name    : convert.c                 */
/*                                          */
/*                Converts to JPEG          */
/*                                          */
/********************************************/

#define  INCL_WIN                   /* required to use Win APIs.           */
#define  INCL_PM                    /* required to use PM APIs.            */
#define  INCL_DOSFILEMGR            /* File System values */
#define  INCL_OS2MM                 /* required for MCI and MMIO headers   */
#define  INCL_MMIOOS2               /* required for MMVIDEOHEADER          */
#define  INCL_MMIO_CODEC            /* required for circular, secondary    */

#include <os2.h>
#include <os2me.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Prototypes */
INT ConvertToJPEG (VOID);

/*************************************************************/
/* Data Translation                                          */
/*************************************************************/
INT ConvertToJPEG (VOID)
{
   ULONG            ulRc = 0;
   LONG             lRc = 0;
   HMMIO            hmmioFrom = 0;
   HMMIO            hmmioTo = 0;
   MMIOINFO         mmioinfo;
   PCHAR            pBuffer = 0;
   PMMIMAGEHEADER   pImageHdr = 0;
   PBYTE            pImage = 0;
   LONG             lLen = 0;
   LONG             lNumBytesRead = 0;

  /* Open the FROM file and read its headers */

  /* Open the FROM file with header translation  */
   memset(&mmioinfo, '\0', sizeof(MMIOINFO));
   mmioinfo.ulTranslate = MMIO_TRANSLATEHEADER+MMIO_TRANSLATEDATA;
   mmioinfo.aulInfo[3] = MMIO_MEDIATYPE_IMAGE;
   hmmioFrom = mmioOpen("output.bmp", &mmioinfo, MMIO_READ);
   /* Check for error */
   if (!hmmioFrom) {
      /* error */
      goto end;
   } 

   /* Query the length of the header */
   ulRc = mmioQueryHeaderLength(hmmioFrom, &lLen, 0,0);
   if (ulRc) {
      /* error */
      goto end;
   }

   /* Allocate size of header */
   pBuffer = (PCHAR) malloc (lLen);
   pImageHdr = (PMMIMAGEHEADER)pBuffer;

   /* Get the movie header */
   ulRc = mmioGetHeader(hmmioFrom, pBuffer, lLen, &lNumBytesRead, 0,0);
   /* Check for error */
   if (ulRc) {
      /* error */
      goto end;
   } 



  /* Create the TO file and write its headers */

  /* Create the TO file with header translation  */
   memset(&mmioinfo, '\0', sizeof(MMIOINFO));
   mmioinfo.ulTranslate = MMIO_TRANSLATEHEADER+MMIO_TRANSLATEDATA;
   mmioinfo.aulInfo[3] = MMIO_MEDIATYPE_IMAGE;
   /* Make sure the MM viewer is installed from the OS/2 Warp Bonus Pack to get the GIF IOProc */
   mmioinfo.fccIOProc = mmioFOURCC('J','P','E','G');
   hmmioTo = mmioOpen("output.jpg", &mmioinfo, MMIO_CREATE+MMIO_WRITE);

   /* Check for error */
   if (!hmmioTo) {
      /* error */
      goto end;
   } 

   /* Set the movie header */
   ulRc = mmioSetHeader(hmmioTo, pBuffer, sizeof(MMIMAGEHEADER), &lNumBytesRead, 0,0);
   /* Check for error */
   if (ulRc) {
      /* error */
      goto end;
   } 

   /* Allocate image buffer */
   lLen = (LONG) pImageHdr->mmXDIBHeader.BMPInfoHeader2.cbImage;
   pImage = (PBYTE) malloc (lLen);

   /* Get the image from the FROM file */
   lRc = mmioRead(hmmioFrom, pImage, lLen);
   
   /* Check for error */
   if (lRc == MMIO_ERROR) {
      /* error */
      goto end;
   } 

   /*Anything to write ? */
   if (lRc > 0) {
      /* Save the image into the TO file */
      lLen = lRc;
      lRc = mmioWrite(hmmioTo, pImage, lLen);
   } 

end:
   if (pBuffer) free((PVOID)pBuffer);
   if (pImage)  free((PVOID)pImage);
   if (hmmioFrom) ulRc = mmioClose(hmmioFrom,0);
   if (hmmioTo)   ulRc = mmioClose(hmmioTo,0);
   return( ulRc);
} /* End of convert */
