********************************************************************
* sbc09 emulator
*
* $Id$
*
* Edt/Rev  YYYY/MM/DD  Modified by
* Comment
* ------------------------------------------------------------------
*   1      2018/08/20  S. Kono
* Emulatoe sbc09 on os9 lv2

         nam   Sbc09
         ttl   Sbc09 emulator

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
ioentry  rmb   $80
filepath rmb   2
parmptr  rmb   2
stdin    rmb   1
chksum   rmb   1
bcount   rmb   1
adr      rmb   2
readbuff rmb   $100
         org   $400
emstart  rmb   $e000-.
size     equ   .

name     fcs   /Sbc09/
         fcb   edition

start    
         clr   <stdin
         stx   <parmptr         save parameter pointer
         lda   #READ.           read access mode
         os9   I$Open           open file
         bcs   L0049            branch if error
         sta   <filepath        else save path to file
         stx   <parmptr         and updated parm pointer
L001F    lda   <filepath        get path
         leax  readbuff,u       point X to read buffer
         ldy   #200             read up to 200 bytes
         os9   I$ReadLn         read it!
         bcs   L0035            branch if error
         bsr   srecords
         bra   L001F            else exit
L0035    cmpb  #E$EOF           did we get an EOF error?
*         bne   L0049            exit if not
         lda   <filepath        else get path
         os9   I$Close          and close it
         bcs   L0049            branch if error
*         ldx   <parmptr         get param pointer
*         lda   ,x               get char
*         cmpa  #C$CR            end of command line?
*         bne   start            branch if not

copytbl
         lda   #$17             lbra
         sta   $e400
         leax  Exit,pcr
         leax  -$e403,x
         stx   $e401
         leax  iotbl,pcr
         leay  iotblend,pcr
         pshs  x,y
         ldy   #(iotblend-iotbl)
l1       ldb   #$7e     * JMP
         stb   ,u+
         ldd   ,x++
         addb  1,s
         adca  ,s
         std   ,u++
         cmpx  2,s
         bne   l1
         puls  x,y
         jmp   $400
 
Exit     clrb
         os9   F$Exit

iotbl
         fdb   getchar-iotbl
         fdb   putchar-iotbl
         fdb   getline-iotbl
         fdb   putline-iotbl
         fdb   putcr-iotbl  
         fdb   getpoll-iotbl
         fdb   xopenin-iotbl
         fdb   xopenout-iotbl
         fdb   xabortin-iotbl
         fdb   xclosein-iotbl
         fdb   xcloseout-iotbl
         fdb   delay-iotbl
iotblend

L0049
err     ldb    #1
        bra     Exit

srecords
        leax   readbuff,u
        clr    <chksum
sline   lda    ,x+
        cmpa   #'S'
        bne    slast
        lda    ,x+
        cmpa   #'1'
        bne    slast
        bsr    gthex2
        subb   #3
        stb    <bcount
        bsr    gthex2
        stb    <adr
        bsr    gthex2
        stb    <adr+1
        lda    <bcount
        ldy    <adr
sbyte   bsr    gthex2
        stb    ,y+
        deca   
        bgt    sbyte
slast
        bsr    gthex2
        lda    <chksum
        cmpa   #$ff
        bne    err1
err1
        rts

gthex4  pshs   d
        bsr    gthex2
        stb    ,s
        bsr    gthex2
        stb    1,s
        puls   d,pc

gthex2  pshs   b
        bsr    gthex1
        aslb
        aslb
        aslb
        aslb
        stb    ,s
        bsr    gthex1
        addb   ,s
        stb    ,s
        addb   <chksum
        stb    <chksum
        puls   b,pc

gthex1  ldb    ,x+
        subb   #'0'
        blo    rgethex1 
        cmpb   #9
        bls    rgethex1
        subb   #7
rgethex1 
        rts

putchar                        * Output one character in B register.
        PSHS        X,Y
        BRA         OUTCH1
getchar                        * Input one character into B register.
        PSHS        A,B,X,Y
GETCH0
        LDA         #0
        LEAX        1,S
        LDY         #1
        OS9         I$Read
        BCS         GETCH0
        PULS        A,B,X,Y,PC
putcr                          * Output a newline.
        LDB         #C$CR
        bsr         putchar
        LDB         #C$LF
        PSHS        X,Y
OUTCH1  PSHS        A,B
        LEAX        1,S
        LDA         #1
        LDY         #1
        OS9         I$Write
        PULS        A,B,X,Y,PC
getpoll
        PSHS        X,Y,D
        LDA         #0
        LDB         #SS.Ready
        OS9         I$GetStt
        CMPB        #$F6       Not Ready
        BNE         RSENSE
        CLRB
        PULS        X,Y,D,PC
RSENSE
        ORCC        #1        set carry to indicate ready
RNSENSE
        PULS        X,Y,D,PC

getline                        * Input line at address in X, length in B.
        PSHS        A,B,X,Y
GETLN0
        CLRA
        TFR         D,Y
        LDA         <stdin
        OS9         I$ReadLn
        BCS         GETLN0
        LEAY        -1,Y
        TFR         Y,D
*        LDA         D,X
*        CMPA        #C$CR
*        BNE         GETLN1
*        LEAY        -1,Y
GETLN1  STY         ,S
        PULS        A,B,X,Y,PC
putline                        * Output string at address in X, length in B.
        PSHS        A,B,X,Y
        CLRA
        TFR         D,Y
        LDA         <stdin
        OS9         I$WritLn
        PULS        A,B,X,Y,PC
xopenin
xopenout
xabortin
xclosein
xcloseout
        RTS

delay   PSHS        D,X  * address **$21** 
                         * On input the D register contains the number of timer 
                         * ticks to wait. Each timer tick is 20ms
        TFR         D,X
        OS9         F$Sleep
        PULS        D,X,PC



         emod
eom      equ   *
         end
