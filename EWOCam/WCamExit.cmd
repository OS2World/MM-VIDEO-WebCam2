/* WCamExit.CMD */
/* This REXX script will be called from EWOCam/2 after a snapshot image is taken */
/* It can be used for post-processing the bitmap file */
/* This example converts to another JPEG using the GBM library */
/* EWOCam/2 passes the Bitmap filename without the extension ".BMP" */

PARSE ARG Filename  /* Get the Filename of the Bitmap (without ext.) */
GBMREF.EXE Filename||.BMP Filename||.JPG
DEL Filename||.BMP
EXIT

