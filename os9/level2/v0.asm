********************************************************************
* progname - program module
*
* $Id: d0.asm,v 1.1 2002/06/14 12:35:43 roug Exp $
*
* Ed.    Comments                                       Who YY/MM/DD
* ------------------------------------------------------------------
*  -     Original Dragon Data distribution version
*
* $Log: d0.asm,v $
* Revision 1.1  2002/06/14 12:35:43  roug
* Add work done on ideal devices
*
*

         nam   V0
         ttl   40-track floppy disk device descriptor

         ifp1
         use   defsfile
         endc
tylg     set   Devic+Objct   
atrv     set   ReEnt+rev
rev      set   $02
         mod   eom,name,tylg,atrv,mgrnam,drvnam
         fcb   $FF mode byte
         fcb   $00  extended controller address
         fdb   $ffc0  physical controller address
         fcb   initsize-*-1  initilization table size
         fcb   $01 device type:0=scf,1=rbf,2=pipe,3=scf
         fcb   $00 drive number
         fcb   $00 step rate
         fcb   $20 drive device type
         fcb   $01 media density:0=single,1=double
         fdb   $0100 number of cylinders (tracks)
         fcb   $01 number of sides
         fcb   $00 verify disk writes:0=on
         fdb   $0012 # of sectors per track
         fdb   $0012 # of sectors per track (track 0)
         fcb   $01 sector interleave factor
         fcb   $08 minimum size of sector allocation
initsize equ   *
name     equ   *
         fcs   /V0/
mgrnam   equ   *
         fcs   /VRBF/
drvnam   equ   *
         fcs   /PDisk/
         emod
eom      equ   *
