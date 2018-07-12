*********************************************
* Rdisk
*  A driver for a Ram disk!
* Modified for os9l1 emulator by Soren Roug 2001
*

         nam Pdisk
         ttl A Device Driver for a RAM Disk

         ifp1
         use defsfile
         endc

***********************
* Edition History

*  #   date    Comments
* -- -------- ----------------------------------------------

Revision equ 1
NumDrvs  set 2 Number of drives

         org Drvbeg
         rmb NumDrvs*DrvMem
RAMSTA   equ .

         mod RAMEND,RAMNAM,Drivr+Objct,Reent+Revision,RAMENT,RAMSTA
         fcb   $FF         mode byte

RAMNAM   fcs /Pdisk/

RAMENT   lbra INIT
         lbra READ
         lbra WRITE
         lbra GETSTA
         lbra PUTSTA
         lbra TERM

*****************************
* INIT
*  Set up the v09 disk

INIT     ldb #NumDrvs Set no drives to 2
* Setup drive tables
         stb   V.NDRV,u     save it
         lda   #$FF
         leax  DRVBEG,u     point to drive table start
L0111    sta   DD.TOT+1,x
         sta   <V.TRAK,x
         leax  <DRVMEM,x
         decb  
         bne   L0111
         clrb
INITXIT  rts

SETUPDT  lda   <PD.DRV,y      Get the drive number
         ldu   V.PORT,u
         sta   1,u    drive number
         stb   2,u    msb of lsn
         tfr   x,d
         sta   3,u
         stb   4,u
         ldd   PD.BUF,y
         sta   5,u     buffer address
         stb   6,u
         rts

*****************************
* READ
*  read a sector from disk
*  Entry: U = Static Storage
*         Y = Path Descriptor
*         B = MSB of LSN
*         X = LSB's of LSN
*  Exit: 256 byte sector in PD.BUF buffer
*
READ     bsr  SETUPDT
         lda  #$81
         sta  ,u   // perform io
         ldb  ,u   // return status
         rts

*****************************
* WRITE
*  Write a sector to disk
* Entry: U = Static Storage
*        Y = Path Descriptor
*        B = MSB of LSN
*        X = LSB's of LSN
*        PD.Buf = Sector to write
*
WRITE    bsr  SETUPDT
         lda #$55
         sta  ,u   // perform io
         ldb  ,u   // return status
WRIT99   rts

**************************
* GETSTA
*  get device status
*
GETSTA
Unknown  comb
         clrb
         rts

**************************
* PUTSTA
*  Set device Status
*
PUTSTA   
PUTSTA90 clrb
         rts

*****************************
* TERM
*  terminate Driver
*
TERM     clrb
         rts

         emod
RAMEND   equ *

