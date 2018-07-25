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

tylg     set   Systm+Objct
atrv     set   ReEnt+rev
rev      set   $01
edition  set   $06
TimerPort set  $e030
TkPerSec set   60
TkPerTS  equ   TkPerSec/10 ticks per time slice


         mod   eom,name,tylg,atrv,ClkEnt,size

size     equ   .

name     fcs   /Clock/
         fcb   edition

SysTbl   fcb   F$Time
         fdb   FTime-*-2
         fcb   $80


ClockIRQ ldx   #TimerPort
         lda   ,x
         bita  #$10
         bne   L00B4
L00AE    jsr   [>D.Poll]  poll ISRs
         bcc   L00AE      keep polling until carry set
         jmp   [>D.AltIRQ] jump into an alternate IRQ if available

L00B4
         ldb   #$8f     start timer
         stb   ,x

         dec   <D.Tick 
         bne   L007F      go around if not zero
         ldb   <D.Sec     get minutes/seconds
* Seconds increment
         incb             increment seconds
         cmpb  #60        full minute?
         blo   L007F
         ldb   <D.TSec
         stb   <D.Tick
         bsr   FTime
L007F    stb   <D.Sec

         jmp   [>D.Clock]

ClkEnt   equ   *
         ldd   #59*256+$01 last second and last tick
         std   <D.Sec     will prompt RTC read at next time slice
         ldb   #TkPerSec
         stb   <D.TSec    set ticks per second
         stb   <D.Tick    set ticks per second
         ldb   #TkPerTS   get ticks per time slice
         stb   <D.TSlice  set ticks per time slice
         stb   <D.Slice   set first time slice
         pshs  cc
         orcc  #FIRQMask+IRQMask       mask ints
         leax  <ClockIRQ,pcr
         stx   <D.IRQ
* install system calls
         leay  <SysTbl,pcr
         os9   F$SSvc
         ldx   #TimerPort
         ldb   #$8f     start timer
         stb   ,x
         puls  pc,cc

* F$Time system call code
FTime    ldx   R$X,u
         ldy   #TimerPort
         ldb   #$04
         stb   ,y
         ldd   1,y
         std   ,x
         std   <D.Year
         ldd   3,y
         std   2,x
         std   <D.Day
         ldd   5,y
         std   4,x
         std   <D.Min
         clrb
         rts

         emod
eom      equ   *
         end
