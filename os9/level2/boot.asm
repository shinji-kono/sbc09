********************************************************************
* Boot - V09 Boot module
*
* $Id: boot_1773.asm,v 1.1.1.1 2001/02/21 23:30:54 boisy Exp $
*
* Ed.    Comments                                       Who YY/MM/DD
* ------------------------------------------------------------------

         nam   Boot
         ttl   v09 Boot module

         ifp1
         use   defsfile
         endc

*
*  map extended rom on page 0x40-
*  first two bytes are extra rom module size 

tylg     set   Systm+Objct
atrv     set   ReEnt+rev
rev      set   $01
edition  set   1

         mod   eom,name,tylg,atrv,start,size

size     equ   .

name     fcs   /Boot/
         fcb   edition

start    
         lbra    entry
         lbra    btdebug
entry
         ldy    #$40    extended rom page no. 
         clra
         clrb
         pshs   d,x,y,u
         tfr    d,x
         leay   4,s    pointer to page no
     **  read boot rom file size
         os9    F$LDDDXY 
         bcs    last
         addb   #$ff     
         adca   #0
         clrb
         std   ,s       size return as d
     ** OS9 lv2 use $a000-$dfff as a temporary page
     ** demand at least that size ( ROM start at $ed00 )
         cmpa   #$ed-$a0
         bhi    ok
         lda    #$ed-$a0
ok
         os9    F$BtMem
         bcs    last
     **  u points the memory
         stu    2,s      return as x
         ldd    ,s
         std    6,s
         ldx    #0
     ** copy to Bt BtRAM
pagel    ldd    #$2000
         cmpd   6,s
         blo    lo
         ldd    6,s
lo       tfr    d,y
         lda    5,s
         sta    $ffa0
         tfr    y,d
loop     ldy    ,x++
         sty    ,u++
         subd   #2
         bne    loop    
         clr    $ffa0    back to system map
         ldd    6,s
         subd   #$2000
         bmi    last     all transfered
         std    6,s
     ** 2k boundary
         inc    5,s
         ldx    #0
         bra    pagel
last     clr    $ffa0
         puls   d,x,y,u,pc

      ** put lbsr btdebug on <$5e
btdebug  anda   #$7f
         sta    $ff81
         rts

         emod
eom      equ   *
         end
