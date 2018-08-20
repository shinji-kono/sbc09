********************************************************************
* loop dummy loader
*
* $Id$
*
* Edt/Rev  YYYY/MM/DD  Modified by
* Comment
* ------------------------------------------------------------------
*   1      2018/07/30  S. Kono

         nam   Loop
         ttl   Dummy loop

         ifp1
         use   defsfile
         endc

* Module header definitions
tylg     set   Prgrm+Objct   
atrv     set   ReEnt+rev
rev      set   $00
edition  set   1

         mod   eom,name,tylg,atrv,start,size

         org   0
count    rmb   2
size     equ   .

name     fcs   /Loop/
         fcb   edition

start    ldy   #4000
l1       ldx   #0
l0       leax  -1,x
         bne   l0
         leay  -1,y
         bne   l1
Exit     clrb
         os9   F$Exit


         emod
eom      equ   *
         end
