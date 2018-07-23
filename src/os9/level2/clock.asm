********************************************************************
* Clock - OS-9 Level One V2 Clock module
*
* $Id: clock.asm,v 1.1.1.1 2001/02/21 23:30:52 boisy Exp $
*
* NOTE:  This clock is TOTALLY VALID for ALL DATES between 1900-2155
*
* Ed.    Comments                                       Who YY/MM/DD
* ------------------------------------------------------------------
* 5      Tandy/Microware original version
* 6      Modified to handle leap years properly for     BGP 99/05/03
*        1900 and 2100 A.D.

         nam   Clock
         ttl   OS-9 Level One V2 Clock module

         ifp1
         use   defsfile
         endc

usefirq  equ 0

tylg     set   Systm+Objct
atrv     set   ReEnt+rev
rev      set   $01
edition  set   $06
TimerPort set  $ffb0

         mod   eom,name,tylg,atrv,ClkEnt,size

size     equ   .

name     fcs   /Clock/
         fcb   edition

SysTbl   fcb   F$Time
         fdb   FTime-*-2
         fcb   F$STime
         fdb   FSTime-*-2
         fcb   $80


       ifeq  usefirq-1
ClockFIRQ 
         leas  -1,s
         pshs  d,dp,x,y
         lda   8,s
         ora   #$80       Entire flag
         pshs  a
         stu   8,s
         jmp   [$FFF8]
       endc
ClockIRQ 
         ldx   #TimerPort
         lda   ,x
         bita  #$10
         beq   L00AE
L00AE    leax  ClockIRQ1,pcr
         stx   <D.SvcIRQ
         jmp   [D.XIRQ]   Chain through Kernel to continue IRQ handling
ClockIRQ1
         inc   <D.Sec     go up one second
         lda   <D.Sec     grab current second
         cmpa  #60        End of minute?
         blo   VIRQend    No, skip time update and alarm check
         clr   <D.Sec     Reset second count to zero
VIRQend
         ldx   #TimerPort
         lda   #$8f
         sta   >TimerPort
         jmp   [>D.Clock]

TkPerTS  equ   2

ClkEnt   equ   *
         pshs  cc
         orcc  #FIRQMask+IRQMask       mask ints
         leax  >ClockIRQ,pcr
         stx   <D.IRQ
       ifeq usefirq-1
         leax  >ClockFIRQ,pcr
         stx   $FFF6     must be a RAM
       endc
* install system calls
         leay  >SysTbl,pcr
         os9   F$SSvc
         ldd   #59*256+TkPerTS last second and time slice in minute
         std   <D.Sec     Will prompt RTC read at next time slice
         stb   <D.TSlice  set ticks per time slice
         stb   <D.Slice   set first time slice
         lda   #TkPerSec  Reset to start of second
         sta   <D.Tick

         ldx   #TimerPort
         ldb   #$8f     start timer
         stb   ,x
         puls  pc,cc

* F$Time system call code
FTime    ldx   #TimerPort
         ldb   #$04
         stb   ,x
         leax  1,x        Address of system time packet
RetTime  ldy   <D.Proc    Get pointer to current proc descriptor
         ldb   P$Task,y   Process Task number
         lda   <D.SysTsk  From System Task
         ldu   R$X,u
STime.Mv ldy   #6         Move 6 bytes
FMove    os9   F$Move
         rts

FSTime   clrb
         rts

         emod
eom      equ   *
         end
