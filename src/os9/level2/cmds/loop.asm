********************************************************************
* Cmp - Binary file comparison utility
*
* $Id$
*
* Edt/Rev  YYYY/MM/DD  Modified by
* Comment
* ------------------------------------------------------------------
*   1      2003/01/20  Boisy G. Pitre
* Rewritten in assembly for size.

         nam   Cmp
         ttl   Binary file comparison utility

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
