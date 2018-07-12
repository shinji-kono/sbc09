********************************************************************
* SysGo - Kickstart program module
*
* $Id: sysgo.asm,v 1.1.1.1 2001/02/21 23:30:54 boisy Exp $ 
*
* Ed.    Comments                                       Who YY/MM/DD
* ------------------------------------------------------------------

         nam   Sysgo
         ttl   Kickstart program module

         ifp1
         use   defsfile
         endc

tylg     set   Prgrm+Objct
atrv     set   ReEnt+rev
rev      set   $01
edition  set   $01

         mod   eom,name,tylg,atrv,start,size
u0000    rmb   32
u0020    rmb   42
u004A    rmb   33
u006B    rmb   6
u0071    rmb   655

size     equ   .

name     fcs   /Sysgo/
         fcb  edition

Banner   fcc   / OS-9 LEVEL TWO VR. 0/
         fcb   48+OS9Vrsn
         fcc   /.0/
         fcb   48+OS9Major
         fcc   /.0/
         fcb   48+OS9Minor
         fcb   C$CR,C$LF
         fcc   /     COPYRIGHT 1988 BY/
         fcb   C$CR,C$LF
         fcc   /   MICROWARE SYSTEMS CORP./
         fcb   C$CR,C$LF
         fcc   /   LICENSED TO TANDY CORP./
         fcb   C$CR,C$LF
         fcc   /    ALL RIGHTS RESERVED./
         fcb   C$CR,C$LF
         fcb   C$LF
BannLen  equ   *-Banner
DefDev   fcc   "/D0"
         fcb   C$CR
HDDev    fcc   "/D0/"
ExecDir  fcc   "Cmds"
         fcb   C$CR
         fcc   ",,,,,"
Shell    fcc   "Shell"
         fcb   C$CR
         fcc   ",,,,,"
ShellPrm fcc   "i=/1"
CRtn     fcb   C$CR
         fcc   ",,,,,"
ShellPL  equ   *-ShellPrm


start    leax  >IcptRtn,pcr
         os9   F$Icpt
         os9   F$ID
         ldb   #$80
         os9   F$SPrior
         leax  >Banner,pcr
         ldy   #BannLen
         lda   #$01                    standard output
         os9   I$Write                 write out banner
*         leax  >DefTime,pcr
*         os9   F$STime                 set time to default
         leax  >ExecDir,pcr
         lda   #EXEC.
         os9   I$ChgDir                change exec. dir
         leax  >DefDev,pcr
         lda   #READ.+WRITE.
         os9   I$ChgDir                change data dir.
         bcs   L0125
         leax  >HDDev,pcr
         lda   #EXEC.
         os9   I$ChgDir                change exec. dir to HD
L0125    pshs  u,y
         os9   F$ID
         bcs   L01A9
         leax  ,u
         os9   F$GPrDsc
         bcs   L01A9
         leay  ,u
         ldx   #$0000
         ldb   #$01
         os9   F$MapBlk
         bcs   L01A9
* Copy our default I/O ptrs to the system process
         ldd   <D.SysPrc,u
         leau  d,u
         leau  <P$DIO,u
         leay  <P$DIO,y
         ldb   #DefIOSiz-1
L0151    lda   b,y
         sta   b,u
         decb
         bpl   L0151
L0186    puls  u,y
         leax  >ShellPrm,pcr
         leay  ,u
         ldb   #ShellPL
L0190    lda   ,x+
         sta   ,y+
         decb
         bne   L0190
* Fork final shell here
         leax  >Shell,pcr
         ldd   #$0100
         ldy   #ShellPL
         os9   F$Chain
L01A5    ldb   #$06
         bra   Crash
L01A9    ldb   #$04
Crash    jmp   <D.Crash

IcptRtn  rti

         emod
eom      equ   *
         end
