*
*     GAME09 interpreter
*

         ifp1
         use   defsfile
         endc

         nam   Game09
         ttl   Game09
tylg     set   Prgrm+Objct   
atrv     set   ReEnt+rev
rev      set   $01
edition  set   1

         mod   eom,name,tylg,atrv,start,size


         org   0
dpage    rmb   $96

DP00     equ     $00
DP42     equ     $42
DP46     equ     $46
DP4A     equ     $4A
DP4B     equ     $4B
DP4E     equ     $4E
DP4F     equ     $4F
DP50     equ     $50
DP52     equ     $52
DP58     equ     $58
DP7E     equ     $7E
DP82     equ     $82
DP83     equ     $83
DP84     equ     $84
DP86     equ     $86
DP88     equ     $88
DP8A     equ     $8A
DP8C     equ     $8C
DP8D     equ     $8D
DP8E     equ     $8E
DP94     equ     $94
DPWORK   rmb     2
DPSTK    rmb     2

linebuf  rmb   256
program  rmb   10000
size     equ   .

name    fcs         "Game09"
        fcb   edition
start   * clrb
        * os9         F$Exit
LC000   BRA         LC005
LC002   NOP  
LC003   BRA         LC014
LC005   TFR         U,D
        TFR         A,DP
        STU         <DPWORK
        STS         <DPSTK
        LEAX        program,U
        STX         <$7E
        STX         <$50
        LDA         #$FF
        STA         ,X
LC014   LDU         <DPWORK
        LEAX        size,U
        STX         <$58
LC020   LDA         #$FF
        STA         <$8C
LC024   LDS         <DPSTK
        LDU         <DPWORK
        LEAU        linebuf,U
        LEAX        >LC63F,PCR
        LBSR        LC165
LC032   LBSR        LC370
        BCS         LC06D
        STX         <$42
        LBSR        LC1D3
        BEQ         LC024
LC03E   BSR         LC09C
LC040   BMI         LC024
        STX         <$42
LC044   LEAX        +$02,X
        LDA         ,X+
        CMPA        #$20
        BNE         LC05B
        LBSR        LC1D3
        BNE         LC05F
        LEAX        +$01,X
        TST         ,X
        BRA         LC040
LC057   LDS         <DPSTK
LC05B   BSR         LC0A8
        BRA         LC040
LC05F   LDX         <$42
        STX         <$84
        BSR         LC0A0
        BMI         LC024
        CMPX        <$84
        BEQ         LC03E
        BRA         LC044
LC06D   STX         <$86
        STD         <$4A
        LDB         ,X
        CMPB        #$2F
        BNE         LC0B1
LC077   BSR         LC09C
LC079   LBSR        LC397
        TST         ,X
        BMI         LC024
        BSR         LC087
        LBSR        LC173
        BRA         LC079
LC087   LDD         ,X++
        PSHS        X
        LBSR        LC443
        PULS        X
        LBRA        LC165
LC093   LDX         <$42
        LEAX        +$02,X
LC097   TST         ,X+
        BNE         LC097
        RTS  
LC09C   LDX         <$7E
LC09E   STX         <$42
LC0A0   LDD         ,X
        BMI         LC0B0
        SUBD        <$4A
        BCC         LC0AF
LC0A8   LBSR        LC173
        BSR         LC093
        BRA         LC09E
LC0AF   CLRA 
LC0B0   RTS  
LC0B1   LDX         <$4A
        BEQ         LC077
        BMI         LC117
        LDX         <$50
        LDA         ,X
        INCA 
        BNE         LC117
        BSR         LC09C
        BMI         LC0DE
        LDX         ,X
        CMPX        <$4A
        BNE         LC0DE
        BSR         LC093
        LDY         <$42
LC0CD   LDA         ,X
        STA         ,Y
        CMPX        <$50
        BEQ         LC0DB
        LEAX        +$01,X
        LEAY        +$01,Y
        BRA         LC0CD
LC0DB   STY         <$50
LC0DE   LDX         <$86
        LDB         #$03
        TST         ,X+
        BEQ         LC114
LC0E6   INCB 
        TST         ,X+
        BNE         LC0E6
        CLRA 
        ADDD        <$50
        TFR         D,Y
        SUBD        <$58
        BCC         LC117
        LDX         <$50
        STY         <$50
        LEAX        +$01,X
        LEAY        +$01,Y
LC0FD   LDB         ,-X
        STB         ,-Y
        CMPX        <$42
        BNE         LC0FD
        LDY         <$4A
        STY         ,X++
        LDY         <$86
LC10E   LDB         ,Y+
        STB         ,X+
        BNE         LC10E
LC114   LBRA        LC032
LC117   LBRA        LC024
LC11A   LDX         <DPWORK
        LEAX        $8F,X
        TST         <$8C
        BNE         LC124
        LEAX        $95,X
LC124   LEAY        <LC159,PCR
LC127   PSHS        X
        LDX         ,Y++
        PSHU        X
        LBSR        LC335
        PULU        X
        PULS        X
        PSHS        A
        LDA         <$83
        ADDA        #$30
        STA         ,X+
        PULS        A
        TST         +$01,Y
        BNE         LC127
        CLR         ,X
        COM         ,-X
        PSHS        X
        LEAX        -$05,X
        CLRA 
LC14B   LEAX        +$01,X
        INCA 
        LDB         ,X
        CMPB        #$30
        BEQ         LC14B
        COM         [,S++]
        SUBA        #$06
LC158   RTS  
LC159   FDB          10000
        FDB           1000
        FDB            100
        FDB             10
        FDB              1
        FDB              0
LC165   CLRA 
LC166   STA         <$94
LC168   LDB         ,X+
        CMPB        <$94
        BEQ         LC158
        LBSR        LC412
        BRA         LC168
LC173   LBSR        LC650
        LBCS        LC024
        RTS  
LC17B   CMPA        #$20
        BEQ         LC1D1
        CLR         <$4A
        CLR         <$4B
        LDB         +$01,X
        BITB        #$DF
        BNE         LC1A6
        CMPA        #$5D
        BNE         LC195
        PULU        X,B,A
        STD         <$46
        STX         <$42
        BRA         LC1C6
LC195   CMPA        #$40
        BNE         LC1A6
        LEAX        +$01,X
        STX         <$46
        CLRA 
        CLRB 
        LDY         <$42
        PSHU        Y,X,B,A
        BRA         LC1D3
LC1A6   CMPA        #$22
        BNE         LC1CA
        LEAX        +$01,X
        BSR         LC166
        BRA         LC1D3
LC1B0   CMPA        #$5C
        BNE         LC1B9
        LBSR        LC676
        BRA         LC1D3
LC1B9   STX         <$84
        BSR         LC1EC
        LBSR        LC39F
        BSR         LC173
        LDX         <$4A
        BNE         LC1D7
LC1C6   LDX         <$46
        BRA         LC1D3
LC1CA   CMPA        #$2F
        BNE         LC1B0
        LBSR        LC397
LC1D1   LEAX        +$01,X
LC1D3   LDA         ,X
        BNE         LC17B
LC1D7   RTS  
LC1D8   PSHS        A
        LEAX        +$01,X
        BSR         LC1F6
        LDX         <$46
LC1E0   LDY         <$42
        PSHU        Y,X,B,A
        PULS        PC,B,A
LC1E7   LDA         #$3D
        LBRA        LC4F9
LC1EC   LDA         ,X+
        BITA        #$DF
        BEQ         LC1E7
        CMPA        #$3D
        BNE         LC1EC
LC1F6   LBSR        LC521
LC1F9   PSHS        B
        LDB         ,X
        BITB        #$DF
        BEQ         LC210
        CMPB        #$29
        BEQ         LC214
        CMPB        #$2C
        BEQ         LC1D8
        PULS        B
        LBSR        LC29C
        BRA         LC1F9
LC210   STX         <$46
        PULS        PC,B
LC214   LEAX        +$01,X
        PULS        PC,B
LC218   CMPB        #$3F
        BNE         LC22E
        PSHS        X
        STB         <$8C
        LBSR        LC5D8
        BSR         LC1F6
        PULS        X
        LEAX        +$01,X
        RTS  
        LEAX        +$01,X
        BRA         LC1F6
LC22E   BSR         LC287
        CMPA        #$3A
        BEQ         LC239
        BSR         LC26E
LC236   LDD         ,Y
        RTS  
LC239   BSR         LC269
        CLRA 
LC23C   LDB         ,Y
LC23E   RTS  
LC23F   CMPB        #$22
        BCS         LC218
        CMPB        #$2D
        BHI         LC218
        SUBB        #$22
        LSLB 
        LEAY        <LC251,PCR
        LDD         B,Y
        JMP         D,Y

LC251   fdb         LC6EC-*       049b
        fdb         LC535-*       02e2
        fdb         LC5A1-*       034c
        fdb         LC545-*       02ee
        fdb         LC236-*       ffdd
        fdb         LC54E-*       02f3
        fdb         LC236-*       ffd9
        fdb         LC23C-*       ffdd
        fdb         LC23E-*       ffdd
        fdb         LC53F-*       02dc
        fdb         LC23F-*       ffdd
        fdb         LC53F-*       02d8

LC269   BSR         LC279
        LEAY        D,Y
        RTS  
LC26E   CMPA        #$28
        BNE         LC292
        BSR         LC279
        LSLB 
        ROLA 
        LEAY        D,Y
        RTS  
LC279   BSR         LC292
        LDY         ,Y
        PSHS        Y
        LEAX        +$01,X
        LBSR        LC1F6
        PULS        PC,Y
LC287   LDB         ,X+
LC289   LDA         ,X+
        CMPA        #$41
        BPL         LC289
        LEAX        -$01,X
        RTS  

LC292   ANDB        #$3F
        CLRA 
        LSLB 
        ADDD        <$04
        TFR         D,Y
        RTS  
LC29C   PSHU        B,A
        LDD         ,X+
        PSHS        B,A
        SUBB        #$3D
        BEQ         LC2A9
        DECB 
        BNE         LC2AB
LC2A9   LEAX        +$01,X
LC2AB   LBSR        LC521
        PULU        Y
        EXG         D,Y
        PSHU        Y,B,A
        PULS        B,A
        CMPA        #$3D
        BNE         LC2C4
        PULU        B,A
        SUBD        ,U++
        BNE         LC2E4
LC2C0   CLRA 
        LDB         #$01
        RTS  
LC2C4   CMPA        #$3C
        BNE         LC2E7
        CMPB        #$3D
        BEQ         LC2D7
        CMPB        #$3E
        PULU        B,A
        BNE         LC2E0
        SUBD        ,U++
        BNE         LC2C0
        RTS  
LC2D7   PULU        B,A
        SUBD        ,U++
        BLE         LC2C0
        CLRA 
        CLRB 
        RTS  
LC2E0   SUBD        ,U++
        BLT         LC2C0
LC2E4   CLRA 
        CLRB 
        RTS  
LC2E7   CMPA        #$3E
        BNE         LC2FF
        CMPB        #$3D
        PULU        B,A
        BNE         LC2F8
        SUBD        ,U++
        BGE         LC2C0
        CLRA 
        CLRB 
        RTS  
LC2F8   SUBD        ,U++
        BGT         LC2C0
        CLRA 
        CLRB 
        RTS  
LC2FF   CMPA        #$2B
        BNE         LC308
        PULU        B,A
        ADDD        ,U++
        RTS  
LC308   CMPA        #$2D
        BNE         LC311
        PULU        B,A
        SUBD        ,U++
        RTS  
LC311   CMPA        #$2A
        LBNE        LC653
        PULU        B,A
LC319   EXG         A,B
        PSHU        B,A
        LDB         +$03,U
        MUL  
        STD         <$82
        BSR         LC32B
        BSR         LC32B
        LDD         <$82
        LEAU        +$02,U
        RTS  
LC32B   PULU        A
        LDB         +$01,U
        MUL  
        ADDB        <$82
        STB         <$82
        RTS  
LC335   CLR         ,-S
LC337   INC         ,S
        LSL         +$01,U
        ROL         ,U
        BCC         LC337
        ROR         ,U
        ROR         +$01,U
        CLR         <$82
        CLR         <$83
LC347   SUBD        ,U
        BCC         LC351
        ADDD        ,U
        ANDCC       #$FE
        BRA         LC353
LC351   ORCC        #$01
LC353   ROR         <$83
        ROR         <$82
        DEC         ,S
        BEQ         LC361
        LSR         ,U
        ROR         +$01,U
        BRA         LC347
LC361   LEAS        +$01,S
        RTS  
LC364   LDB         ,X
        CMPB        #$30
        BCS         LC36D
        CMPB        #$3A
        RTS  
LC36D   ANDCC       #$FE
        RTS  
LC370   LBSR        LC5D8
LC373   BSR         LC364
        BCC         LC396
        CLRA 
        CLRB 
LC379   ADDB        ,X+
        ADCA        #$00
        SUBD        #$030
        PSHU        B,A
        BSR         LC364
        LDD         ,U
        BCC         LC392
        LSLB 
        ROLA 
        LSLB 
        ROLA 
        ADDD        ,U++
        LSLB 
        ROLA 
        BRA         LC379
LC392   PULU        B,A
        ORCC        #$01
LC396   RTS  
LC397   LDB         #$0D
        BSR         LC39D
LC39B   LDB         #$0A
LC39D   BRA         LC412
LC39F   PSHS        B,A
        LDA         #$01
        STA         <$8C
        LDX         <$84
        LDB         ,X
        CMPB        #$2E
        BNE         LC3BA
        PULS        A
        LDA         ,S+
LC3B1   BEQ         LC3C4
        LDB         #$20
        BSR         LC412
        DECA 
        BRA         LC3B1
LC3BA   CMPB        #$3B
        BNE         LC3C5
        LDD         ,S++
        LBEQ        LC057
LC3C4   RTS  
LC3C5   CMPB        #$40
        BNE         LC3E4
        LDB         +$02,X
        LBSR        LC292
        PULS        B,A
        STD         ,Y
        PULU        B,A
        SUBD        ,Y
        BLT         LC3E1
        PULU        X,B,A
        STD         <$46
        STX         <$42
        LEAU        -$06,U
        RTS  
LC3E1   LEAU        +$04,U
        RTS  
LC3E4   CMPB        #$26
        BNE         LC3FA
LC3E8   LDB         +$01,X
        CMPB        #$3D
        BNE         LC3FA
        LDD         ,S++
        BNE         LC3F9
        LDX         <$7E
        STX         <$50
        DECA 
        STA         ,X
LC3F9   RTS  
LC3FA   CMPB        #$21
        BNE         LC40C
        PULS        B,A
        STD         <$4A
        BEQ         LC40B
        LDY         <$42
        LDX         <$46
        PSHU        Y,X
LC40B   RTS  
LC40C   CMPB        #$24
        BNE         LC415
        PULS        B,A
LC412   LBRA        LC64A
LC415   CMPB        #$3F
        BNE         LC449
        LDB         +$01,X
        CMPB        #$28
        LBNE        LC56E
        LBSR        LC51F
LC424   STB         <$8E
        PULS        B,A
        BSR         LC433
        ADDA        <$8E
        BMI         LC430
        BSR         LC3B1
LC430   LBRA        LC165
LC433   TSTA 
        LBPL        LC11A
        LBSR        LC4F3
        LBSR        LC11A
        LDB         #$2D
        STB         ,-X
        RTS  
LC443   PSHS        B,A
        LDB         #$05
        BRA         LC424
LC449   CMPB        #$3D
        BNE         LC460
        PULS        B,A
        STD         <$7E
        LDX         #$FFFF
        STX         <$4A
        LBSR        LC09C
        STX         <$50
        LBRA        LC024
LC45E   PULS        PC,B,A
LC460   CMPB        #$3E
        BNE         LC46C
        LDD         ,S
        BEQ         LC45E
        JSR         [,S++]
        PSHS        B,A
LC46C   LDX         <$84
        LBSR        LC287
        CMPA        #$3A
        BNE         LC47D
        LBSR        LC269
        PULS        B,A
        STB         ,Y
        RTS  
LC47D   LBSR        LC26E
        PULS        B,A
        STD         ,Y
        RTS  
        LDA         +$01,X
        CMPA        #$4E
        BEQ         LC48E
        LBRA        SYSTEM
LC48E   TST         +$02,X
        BNE         LC494
        CLR         +$03,X
LC494   LEAX        +$03,X
        CLR         <$8C
        LBSR        LC373
        BCS         LC4A0
        LDD         #LC3E8
LC4A0   STD         <$88
        LDB         ,X
        CMPB        #$2C
        BNE         LC4AF
        LEAX        +$01,X
        LBSR        LC373
        BCS         LC4B2
LC4AF   LDD         #$00A
LC4B2   STD         <$8A
        RTS  
LC4B5   CMPA        #$2F
        BNE         LC4F9
        CLR         <$8D
        LDD         +$02,U
        BEQ         LC4F9
        BMI         LC4D4
        CMPD        #LC002
        BNE         LC4DE
        PULU        B,A
        CLR         <$4E
        CLR         <$4F
        ASRA 
        RORB 
        ROR         <$4F
        LEAU        +$02,U
        RTS  
LC4D4   INC         <$8D
        NEG         +$03,U
        BNE         LC4DC
        DEC         +$02,U
LC4DC   COM         +$02,U
LC4DE   LDD         ,U++
        BPL         LC4E6
        DEC         <$8D
        BSR         LC4F3
LC4E6   LBSR        LC335
        LEAU        +$02,U
        STD         <$4E
        LDD         <$82
        TST         <$8D
        BEQ         LC4F8
LC4F3   NEGB 
        BNE         LC4F7
        DECA 
LC4F7   COMA 
LC4F8   RTS  
LC4F9   LBSR        LC397
        LDB         #$3F
        STB         <$8C
        LBSR        LC412
        TFR         A,B
        LBSR        LC412
        LDB         #$20
        LBSR        LC412
        LDX         <$42
        PSHS        Y,D
        LDY         <DPWORK
        LEAY        $95,Y
        STY         ,S
        CMPX        ,S
        PULS        Y,D
        BNE         LC519
        LBSR        LC165
        BRA         LC51C
LC519   LBSR        LC087
LC51C   LBRA        LC024
LC51F   LEAX        +$01,X
LC521   LBSR        LC373
        BCS         LC532
        LBRA        LC23F
        BSR         LC51F
        BRA         LC4F3
        BSR         LC51F
        TSTA 
        BMI         LC4F3
LC532   RTS  
        BSR         LC51F
LC535   PSHS        B,A
        LDD         ,S++
        LBNE        LC2E4
        INCB 
        RTS  
LC53F   BSR         LC51F
        LDD         <$4E
        RTS  
LC545
        BSR         LC51F
        PSHU        B,A
        LDD         <$52
        PSHU        B,A
LC54E
        LDD         #$3D09
        LBSR        LC319
        ADDD        #1
        STD         <$52
        TFR         A,B
        CLRA 
        LBSR        LC319
        TFR         A,B
        CLRA 
        ADDD        #1
        RTS  
LC568
        CLRA 
        LDB         +$01,X
        LEAX        +$03,X
        RTS  
LC56E   CMPB        #$3F
        BEQ         LC57E
        CMPB        #$24
        BEQ         LC582
        PULS        B,A
        LBSR        LC433
        LBRA        LC165
LC57E   LDB         ,S
        BSR         LC584
LC582   PULS        B,A
LC584   TFR         B,A
        BSR         LC58E
        TFR         A,B
        ANDB        #$0F
        BRA         LC592
LC58E   LSRB 
        LSRB 
        LSRB 
        LSRB 
LC592   CMPB        #$0A
        BMI         LC598
        ADDB        #$07
LC598   ADDB        #$30
        LBRA        LC412
        CLRA 
        BSR         LC5BD
LC5A1
        LBCC        LC64D
LC5A4   PSHS        B
        BSR         LC5BD
        PSHU        B
        PULS        B
        BCC         LC5BA
        LSLB 
        ROLA 
        LSLB 
        ROLA 
        LSLB 
        ROLA 
        LSLB 
        ROLA 
        ADDB        ,U+
        BRA         LC5A4
LC5BA   LEAU        +$01,U
        RTS  
LC5BD   LEAX        +$01,X
        LDB         ,X
        SUBB        #$30
        BCS         LC5D2
        CMPB        #$0A
        BCS         LC5D1
        SUBB        #$07
        CMPB        #$0A
        BCS         LC5D2
        CMPB        #$10
LC5D1   RTS  
LC5D2   ANDCC       #$FE
        RTS  
LC5D5   LBSR        LC397
LC5D8   LDB         #$3A
        LBSR         LC64A
        LDX         <DPWORK
        LEAX        $95,X 
        TST         <$8C
        BNE         LC5F2
        LDD         <$88
        LBMI        LC020
        LBSR        LC443
        LDB         #$20
        STB         -$01,X
        BSR         LC64A
LC5F2   BSR         LC64D
        CMPB        #$08
        BEQ         LC634
        CMPB        #$0D
        BCS         LC5F2
        BEQ         LC611
        CMPB        #$18
        BEQ         LC5D5
        STB         ,X+
        PSHS        Y,D
        LDD         <DPWORK
        ADDD        #$DF
        LEAY        $DF,Y
        STY         ,Y
        CMPX        ,S
        PULS        Y,D
        BNE         LC5F2
        LEAX        -$01,X
        LDB         #$08
        BSR         LC64A
        BRA         LC5F2
LC611   TST         <$8C
        BNE         LC626
        LDY         <DPWORK
        LEAY        $9B,Y  
        PSHS        Y
        CMPX        ,S++
        BEQ         LC61F
        LEAY        -1,Y  
        PSHS        Y
        CMPX        ,S++
        BCC         LC626
LC61F   COM         <$8C
        LBSR        LC39B
        BRA         LC5D8
LC626   LDD         <$88
        ADDD        <$8A
        STD         <$88
        CLR         ,X
        LDX         <DPWORK
        LEAX        $95,X
        LBRA        LC39B
LC634   LEAX        -$01,X
        LDY         <DPWORK
        LEAY        $94,Y  
        PSHS        Y
        CMPX        ,S++
        BNE         LC5F2
        STB         <$8C
        LBRA         LC5D8
LC63F
        FCB         $D,$A
        FCC         "*READY"
        FCB         $D,$A,0

LC64A   LBRA        OUTCH
LC64D   LBRA        GETCH
LC650   LBRA        SENSE

LC653   CMPA        #$2E      EXBOP
        BNE         LC65E
        PULU        B,A
        ORA         ,U+
        ORB         ,U+
        RTS  
LC65E   CMPA        #$26
        BNE         LC669
        PULU        B,A
        ANDA        ,U+
        ANDB        ,U+
        RTS  
LC669   CMPA        #$21           
        LBNE        LC4B5
        PULU        B,A
        EORA        ,U+
        EORB        ,U+
        RTS  
LC676   LDD         +$01,X      EXTEND
        LEAX        +$03,X
        PSHS        U,X,B,A
        LEAY        >OPCMD,PCR
LC680   LDX         ,Y
        BMI         LC6E7
        CMPX        ,S
        BEQ         LC68C
        LEAY        +$05,Y
        BRA         LC680
LC68C   LEAY        +$03,Y
        LEAS        +$02,S
        LDX         ,S
        LDA         -$01,Y
        BEQ         LC6C3
        PSHS        Y,A
        LDA         #$20
LC69A   CMPA        ,X+
        BEQ         LC69A
        LDA         ,-X
        STX         +$03,S
LC6A2   BITA        #$DF
        BEQ         LC6B0
        CMPA        #$2C
        BEQ         LC6AE
        BSR         LC6C9
        BRA         LC6B3
LC6AE   LEAX        +$01,X
LC6B0   LDD         #$FFFF
LC6B3   PSHU        B,A
        LDA         ,X
        DEC         ,S
        BNE         LC6A2
        LDD         +$03,S
        STX         +$03,S
        TFR         D,X
        PULS        Y,A
LC6C3   LDD         ,Y
        JSR         D,Y
        PULS        PC,U,X
LC6C9   LBSR        LC521
LC6CC   PSHS        B
        LDB         ,X
        BITB        #$DF
        BEQ         LC6E5
        CMPB        #$2C
        BEQ         LC6E3
        CMPB        #$29
        BEQ         LC6E3
        PULS        B
        LBSR        LC29C
        BRA         LC6CC
LC6E3   LEAX        +$01,X
LC6E5   PULS        PC,B
LC6E7   PULS        U,X,B,A
        LBRA        LC4F9
LC6EC   LEAX        +$01,X
        LDB         ,X+
LC6F0   LDA         ,X+
        BEQ         LC6F8
        CMPA        #$22
        BNE         LC6F0
LC6F8   CLRA 
        RTS  

OUTCH   PSHS        X,Y
        BRA         OUTCH1
GETCH   
        PSHS        B,X,Y
GETCH0
        LDA         #0
        LEAX        ,S
        LDY         #1
        OS9         I$Read
        BCS         GETCH0
        PULS        B,X,Y,PC
OUTCH1  PSHS        B
        LEAX        ,S
        LDA         #1
        LDY         #1
        OS9         I$Write
        PULS        B,X,Y,PC
SENSE   PSHS        X,Y
        LDA         #0
        LDB         #SS.Ready
        OS9         i$GetStt
        PULS        X,Y,PC



        * COMMAND TABLE CONSIST....
        *    COMMAND NAME (2 BYTE)
        *    PARAMETER COUNT (1 BYTE)
        *    OFSET TO SUBROUTIE FROM HERE (1 WORD)
        **************************
OPCMD               FCB 'A','T',2 START LINE,STEP
        FDB CAUTO-*
        FCB 'R','N',3 RENUM first line no,inc,renum start
        FDB RENUM-*
        * FCB 'R','D',3 DISK READ address,track,sector
        * FDB GETDK-*
        * FCB 'W','R',3 DISK WRITE
        * FDB PUTDK-*
        FCB 'S','Y',0 TO SYSTEM
        FDB SYSTEM-*
        FDB -1,-1,-1,-1,-1,-1
        *************************
RENUM
        LDD         ,U++
        BPL         LC762
        CLRA 
        CLRB 
LC762   STD         <$4A
        LBSR        LC09C
        BMI         LC789
        LDD         ,U++
        BPL         LC770
        LDD         #$00A
LC770   LDY         ,U++
        BPL         LC779
        LDY         #LC3E8
LC779   STY         ,X++
        LBSR        LC173
        LEAY        D,Y
LC781   TST         ,X+
        BNE         LC781
        TST         ,X
        BPL         LC779
LC789   RTS  
CAUTO
        LDD         ,U++
        BPL         LC791
        LDD         #$00A
LC791   STD         <$8A
        LDD         ,U
        BEQ         LC79E
        BPL         LC79C
        LDD         #LC3E8
LC79C   STD         <$88
LC79E   CLR         <$8C
        RTS  
        BSR         LC7B6
        *   LBSR        LCD09
        BNE         LC7B1
        RTS  
        BSR         LC7B6
        *   LBSR        LCD0C
        BNE         LC7B1
        RTS  
LC7B1   LDA         #$44
        LBRA        LC4F9
LC7B6   LDX         +$04,U
        LDA         +$03,U
        LDB         +$01,U
        RTS  
SYSTEM  clrb
        os9         F$Exit  
        rts
        emod
eom     equ        *
        end
