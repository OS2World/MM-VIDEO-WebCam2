EWOCam/2 ReadMe                                                         17 Jun 1998 


EWOCam/2 (ELSA WINNER OFFICE Camera for OS/2)

EWOCam/2 is a customized version of my WebCam/2 application.
While WebCam/2 works fine for the FAST Movie Machine, EWOCam/2 is made for
the ELSA WINNER 2000/Office graphics adapter with VideoIn driver.

EWOCam/2 takes timed camera snapshots and saves them as images to disk.
Primary I wrote this app for myself, but it came out that many OS/2 users
want to have this, too...


EWOCam/2's current developement status:

The standard image format is OS/2 bitmap (.BMP). 
Resolution is 160 x 120, 24 bit color, should be possible to enhance it.
With a user exit file ("WCAMEXIT.CMD") it is possible to start a conversion
program or manipulate the bitmap after grabbing a new one.
Native JPEG support is planned for the future.

You can download the latest EWOCam/2 or WebCam/2, the version for FAST Movie Machine II from:
ftp://wave.gkss.de/pub/os2/WebCam/

Requirements for EWOCam/2 

  1. OS/2 WARP Version 4.0
  2. ELSA WINNER 2000/Office graphics adapter with video input
  3. ELSA VideoIn OS/2 Device driver (http://www.elsa.de/DOWNLOAD/FILES/GRAPHICS/WINNER/2000OFC/INDEX.HTM)
  4. A camera or other video source


WebCam/2's current developement status: 

After an image snapshot EWOCam/2 calls a REXX file named "WCamExit.CMD".
It can be used for an external program e.g. to convert the bitmap into JPEG format.
See example file: It calls GBMREF.EXE to do this convertion.
I added the files GBMREF.EXE and GBM.DLL from Andy Key. For more information look at
http://www.interalpha.net/customer/nyangau/

In the future I want to add JPEG support directly to WebCam/2, but until today I
have not got the GBM.DLL to work with my WATCOM compiler...
(If you have a sample for me, please email)


Installation and deinstallation

Copy EWOCAM.EXE to a directory of your choice. After the first program start
a file EWOCAM.INI is created, where EWOCam/2 stores its settings. It does
not change your system ini files!
If you want to deinstall WebCam/2, just delete WEBCAM.EXE and WEBCAM.INI, thats it!


Running EWOCam/2

Set up the ELSA P2VideoIn device from the Multimedia Setup Notebook (source, brightness, etc.)


Interval:        This is the time between the video snapshots, 1 second to 24 hours.
                 Disabled, if Hours, Minutes and Seconds are set to 0.
Filename Format: WEBCAM.BMP - only one image file will be created and overwritten
                 with the next snapshot.
                 <Time>.BMP - The filename will be created from the current time.
                 Using this selection, you store the images for 24 hours until
                 they will be overwritten.
                 <Date-Time>.BMP- This selection gives every image an individual
                 filename from the actual date and time. Every image will be kept
                 until your disk runs full!


You can manually take a snapshot by either selecting "Click!" from the menu,
pressing ALT-C, or double-click with mousebutton 1 in the window.

It is also possible to "remote-control" EWOCam/2 from another program, e.g. a REXX script:
Start EWOCam/2 as usual, time interval can be set to zero. Execution of REMOTE.EXE causes
EWOCam/2 to update the image. REMOTE.EXE can be located in any directory.
Example: Imagine you want to have the video snapshot controlled by a special event;
this can be e.g. the execution of a web server cgi program or within a script file.
Just execute or call REMOTE.EXE while EWOCam/2 is running in a forground session or 
minimized.
For programmers: Post the event semaphore "\\SEM32\\WEBCAM\\CAPTURE".


Disclaimer:

There is absolutely no warranty is you use this program and I am not responsible for any
damage or loss of data.
EWOCam/2 is freeware for non-commercial use and can be distributed freely if unchanged.
Distribution on CD/Shareware collections only with permission.


Have fun!


Jurgen Dittmer
dittmer@gkss.de
http://wave.gkss.de


ELSA WINNER 2000/Office is a trade mark of ELSA AG, Aachen (Germany)
