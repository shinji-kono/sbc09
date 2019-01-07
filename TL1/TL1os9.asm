*******TL/1*************
         nam   tl1
         ttl   TL1 compiler

         ifp1
         use   defsfile
         endc

* Module header definitions
tylg     set   Prgrm+Objct   
atrv     set   ReEnt+rev
rev      set   $00
edition  set   1

         mod   eom,name,tylg,atrv,start,size

TXTTOP EQU $FB7E

INDN   RMB 1
OUTDN  RMB 1
LB     RMB 2
GB     RMB 2
MHIGH  RMB 1
MOD    RMB 1
WT1    RMB 1
WT2    RMB 1
RNDH   RMB 1
RNDL   RMB 1
DREG   RMB 1 
DBUF   RMB 3
XR     RMB 2
YR     RMB 2
ZR     RMB 2
PFTBEG RMB 2     prog/func table
PC     RMB 2
SREG   RMB 2
SP     RMB 2
PFMAX  RMB 2 
LSW    RMB 1 
SY     RMB 1
CH     RMB 1 
VAL    RMB 1 
GL     RMB 1     global 0xff / local 1
OPER   RMB 1 
GLL    RMB 1     left value g 0xff / local l 
AMODE  RMB 1
ACC    RMB 1
LSIZE  RMB 1
TCOUNT RMB 1     1 search reserved word only, 5 search all local/global var/array, proc
TEND   RMB 2     table end (search start from here ) include local name
WEND   RMB 2     word end
PMODE  RMB 1     0x20 main,  1 proc, 0 ?
RSW    RMB 1     0 word lookup, 0xff word register mode in tlook
GEND   RMB 2     end of global name
SSW    RMB 1

filepath rmb   2
parmptr  rmb   2
stdin    rmb   2
adr      rmb   2
work     rmb   2
bufsiz   equ   $100-1

**
* LIBRARY ADDRESSS TABLE
**
LIBR     equ   .
ioentry  rmb   $80
readbuff rmb   bufsiz+1

OBJSTART RMB 2+12


* OBJECT PG AREA

WTBLE    RMB $100+500
MSTACK   RMB $140 

OBJECT   RMB 2048        * NOP
RUB      equ 8

size   equ .

name     fcs   /TL1/
         fcb   edition

**
COMP   CLRA
       STA OUTDN
       STA LSW
       STA AMODE
       STA PMODE
       STA LSIZE
       STA RSW
       STA PFMAX
       leax MSTACK,u
       STX SP
       LEAX 2,x
       STX PFTBEG
       CLRB
C1     STA ,X+
       DECB
       BNE C1
       INCA
       STA TCOUNT   = 1
       LDA #' '
       STA CH
**  copy reserved word table
       LEAX WTABLE,PCR
       leay WTBLE,u
       ldb #WTBLEND-WTABLE
tbl1   lda ,x+
       sta ,y+
       decb
       bne tbl1
       sty TEND
       LEAX OBJECT,u
       STX PC
** 
       LBSR CRLF
       BSR REG0
       LBSR WORD
       CMPA #$30 PROC?
       BNE  *+4
       BSR REGNAM  
       BSR REG0
       CMPA #$31 FUNC?
       BNE  *+4
       BSR REGNAM
       CLR LSIZE
       BSR REG0
       LBSR PROG 
       LBSR STPOUT
       LDX TEND
       STX GEND
PLOOP  LDA  SY
       CMPA #$8F /END CODE
       LBEQ ENDL
       CMPA #$4
       BCC ERR4
       DECA
       STA  PMODE
       LDA VAL
       LBSR DEFPF
       LBSR PUTHSL
       FDB $03BD
       FDB PSHLB
       BSR REG0
       CLR LSIZE
       LBSR WORD
       CMPA #$37     (
       BNE PL1
       BSR REGNAM
       LDA #$3B       )
       LBSR CHECK
PL1    BSR PROG
       LDB PMODE
       CMPB #1
       BNE *+5
       LBSR RETP     generate return
       LDX GEND 
       STX TEND
       LDB #5
       STB TCOUNT
       BRA PLOOP
**
* REGIST ZERO
**
REG0   LDX TEND
       CLR ,X+
       STX TEND
       INC TCOUNT
       RTS
**
*  REGIST NAME
**
REGNAM COM RSW
       LBSR WORD
       LDB RSW
       BEQ *+5
ERR4   LBRA ERROR    define duplicate name 
       LDX TEND
       LDA LSIZE
       STA ,X
       INC LSIZE
       LDX WEND
       STX TEND
       BSR WORD1
       CMPA #$36    "["   array
       BNE REG2
       BSR WORD1
       BNE ERR4
       LDA VAL
       ADDA LSIZE
       BSR WORD1 
       LDA  #$3A    ":"
       LBSR CHECK
REG2   CMPA #$3C    "," get next word
       BEQ REGNAM
       RTS
**
* PROGRAM
**
PROG   CMPA #$32 VAR?
       BNE *+4
       BSR REGNAM    global variable
       BSR REG0      put mark
       CMPA #$33 ARRAY?
       BNE *+4
       BSR REGNAM
**
*  STATEMENT 
**
STAT   BSR SSTAT
       LDB SSW
       BEQ RTS1
       LBRA ERROR 
**
* STATEMENTS LIST
**
STLIST BSR SSTAT 
       LDB SSW
       BEQ STLIST
RTS1   RTS
**
* SINGLE STATEMENT
**
SSTAT  CLRA
       STA ACC 
       STA SSW 
       LDA SY
**
*  MULTIPLE STATEMENTS
**
SS1    CMPA #$34   BEGIN
       BCS SS2
       CMPA #$38   END
       BCC SS2
       ADDA #4
       PSHS A      wait for END
       BSR WORD1
       BSR STLIST
       PULS A
       CLR SSW
       LBRA CHECK
**
* STOP
**
SS2    CMPA #$50
       BNE SS3
       BSR STPOUT
WORD1  LBRA WORD
**
STPOUT LBSR PUTHSL
       FCB 3,$7E
       FDB exit
       RTS
**
* RETURN
**
SS3    CMPA #$51
       BNE SS4
       LDB PMODE
       LBEQ ERR4
       PSHS B
       BSR WORD1
       PULS B
       DECB 
       BEQ RETP
       LBSR EXPR
RETP   LBSR PUTHSL
       FDB $037E
       FDB PULLB
       RTS
**
* PROC CALL
**
SS4    CMPA #2
       LBEQ PFCALL
       CMPA #$E0
       BCS SS5
       LBRA PFCALL
**
* ASSIGNMENT STATEMENT 
**
SS5    CMPA #7
       BCS ASSIGN
       LBRA SS6
ASSIGN LDB GL 
       PSHS B
       LDB VAL
       PSHS B
       CMPA #5
       BNE ASS1
       LBSR SUBSC1
       LDB ,S
       LDA LSW 
       BNE AS0
       LDA #$8B 
       LBSR PUTAB
       LDA  #1
       BRA   AS2
AS0    ADDB OPER 
       STB ,S
       CLR LSW
       BRA   AS1
ASS1   CMPA #6
       BNE ASS2 
       LBSR DSUBSC 
       LDA #2
       BRA AS2 
ASS2    CMPA #4
       BEQ *+5 
       LBRA ERROR 
       LBSR WORD 
AS1    CLRA 
AS2    PSHS  A 
       LDA SY
       CMPA #$3C
       BNE  *+9
       LBSR  WORD 
       BSR   ASSIGN
       BRA   AS3
       LDA   #$3D 
       LBSR  CHECK
       LDA   #$27 
       LBSR  CHECK
       LBSR  EXPR
AS3    PULS D
       STB   OPER     4--
       PULS B
       STB  GLL
       TSTA 
       BNE   AS4
       LBSR  PUTX 
       LDA   #$A7 
       LDB   OPER 
       LBRA  PUTOFS
AS4      DECA 
       BNE   AS5
       LDD   #$3504          PULS B
       LBSR  PUTAB
       LBSR  PUTX            STA B,X / STA B,Y
       LDD   #$A785           
       LBRA  PUTAB
AS5    LBSR  PUTHS
       FCB   4
       FDB   $3510           PULS X
       FDB   $A700           STA ,X
       RTS
**
** NON-STATEMENT
**
SSEND  COM SSW
       RTS
**
SS6    CMPA #$58 
       BCC SSEND 
       CMPA #$52
       BCS SSEND
       SUBA #$51
       PSHS A
       LBSR WORD
       PULS B         B keyword A next token
**
*  REPEAT UNTIL 
**
       DECB
       BNE SS7
       BSR ASTOUT
       LBSR STLIST
       LDA #$60
       LBSR CHECK
       LBSR EXPR
       LBSR PUTHS
       FCB 2
       FDB $2603
       CLR SSW
       BRA   MINOUT 
**
* FOR-TO
**
SS7    DECB
       BNE SS8
       CMPA #4        should be simple var
       BEQ *+5
       LBRA ERROR
       LDB VAL
       PSHS B
       LDB GL
       PSHS B
       LBSR ASSIGN
       LDA #$61       to
       LBSR CHECK
       CLR ACC
       LBSR EXPR
       LDA #$64       do
       LBSR CHECK
       BSR ASTOUT
       LDD  #$3402     pshs a
       LBSR PUTAB
       LBSR STAT
       LDD  #$3502      puls a
       LBSR PUTAB
       PULS D
       STA GLL
       STB OPER
       LDA #$A1
       LBSR PUTABX
       LBSR PUTHS
       FCB 2
       FDB $2305
       LDA #$6C
       LDB OPER
       LBSR PUTAB
MINOUT LBRA PULJMP
ASTOUT LBRA PSHDEF 
**
* WHILE-DO
**
SS8    DECB
       BNE SS9
       BSR ASTOUT
       LBSR EXPR
       LDA #$64
       LBSR CHECK
       LBSR PUTHS
       FCB 2
       FDB $2603
       BSR SLAOUT 
       LBSR STAT
       BSR PEROUT 
       BSR MINOUT
PLUOUT LBRA PULDEF
**
** IF—THEN
**
SS9    DECB
       BNE SS10
       LBSR EXPR
       LDA #$65 
       LBSR CHECK
       LBSR PUTHS
       FCB 2
       FDB $2603
       BSR SLAOUT 
       LBSR STAT
       BRA PLUOUT 
SLAOUT LBRA PSHJMP
PEROUT LBRA STCHG
***
* CASE—OF
SS10   DECB
       BNE SS11
       LBSR EXPR
       LDA #$66
       LBSR CHECK
       CLRB
S10A   INCB
       PSHS B
       STB ACC 
       LBSR LEXPR 
       LDA #$81 
       LBSR AOPER
       LBSR PUTHS
       FCB 2
       FDB $2703
       BSR SLAOUT
       LBSR STAT
       BSR SLAOUT
       BSR PEROUT
       BSR PLUOUT
       PULS B
       LDA SY
       CMPA #$67
       BNE S10A
       PSHS B 
       LBSR WORD
       LBSR STAT
       PULS A
       LBRA PLDFN 
**
* WRITE STATEMENT
**
SS11   LDA #$37
       LBSR CHECK
       LBSR EXPR    output channel number
       LBSR PUTHS
       FDB $0297
       FCB OUTDN
       LDA #$3D
       LBSR CHECK
WTLP   CLR ACC
       BSR WTERM
       LDA SY
       CMPA #$3C
       BNE WTEN
       LBSR WORD
       BRA WTLP
WTEN   LDA #$3B
       LBRA CHECK
**  write command argument
WTERM  CMPA #$6C      string
       BNE WR1
       LBSR PUTHSL
       FDB $03BD
       FDB PUTSTR
       LDA CH
WR01   CMPA #'"'     copy until '"'
       BEQ WR02
       LBSR PUTA
       LBSR GETCH
       BRA WR01
WR02   CLRA          put 0 at end
       LBSR PUTA
       LBSR GETCH
       LBRA WORD
**
WR1    CMPA #$6B
       BNE WR3
       LBSR WORD
       CMPA #$37
       BEQ WR2
       LBSR PUTHSL
       FDB $03BD
       FDB CRLF
       RTS
**
WR2    LBSR WEXPR
       BSR WTEN
       LBSR PUTHSL
       FDB $03BD
       FDB CRLFA
       RTS
**
WR3    CMPA #$6A
       BNE WR4
       LBSR SUBSC
       LBSR PUTHSL
       FDB $03BD
       FDB SPACEA
       RTS
**
WR4    CMPA #$A9
       BNE WR5
       LBSR SUBSC
       LBSR PUTHSL
       FDB $03BD
       FDB PUTCA
       RTS
**
WR5    CMPA #$26
       BNE WR6
       LBSR DSUBSC
       LBSR PUTPLB
       LBSR PUTHSL
       FDB $03BD
       FDB PUTDA+1
       BRA WR66
**
WR6    LBSR EXPR
       LBSR PUTHSL
       FDB $03BD
       FDB PUTDA
WR66   
RTS11  RTS
**
* PUTX & PUTB
**
PUTABX PSHS D
       BSR PUTX
       PULS D 
       BRA PUTAB
**
* use X for LB, OR use Y for GB BY 
**
PUTX   equ RTS11     * no pointer load
**
PUTOFS PSHS D,X
       LDX <PC
       STA ,X+
       CLRA
       TST  <GLL
       BMI  PUTOFSX
       LDA  #$20
PUTOFSX STA ,S
       CMPB #32
       BGT  *+6
       CMPB #-32
       BGE  PUTOFS5
       LDA  #$80
       ORA  ,S
       STA  ,X+
       STB  ,X+
       BRA  PUTOFS8
PUTOFS5
       ANDB #$1F
       ORB   ,S
       STB   ,X+
PUTOFS8
       STX  <PC
       PULS D,X,PC

***
* PUT ACC A&B
**
PUTAB  BSR PUTA
       TFR B,A
**
* PUT ACCA RS AN OBJECT
**
PUTA   PSHS X
       LBSR AOUT
       PULS X,PC
**
* PUTHS STRING
**
PUTHS  LDX ,S++
       LDB ,X+
PS1    LDA ,X+
       BSR PUTA
       DECB
       BNE PS1
       JMP ,X
**
** CHECK ACC
**
CHECK  CMPA SY
       BEQ WORD
**
* ERROR
**
ERROR  LBSR PUTSTR
       FDB $0D0A
       FCC "ERROR ",0
       LDX TEND
       LEAX 1,X
       NEG ,X
ER0    LDA ,X+
       LBSR PUTCA
       CMPX WEND
       BNE ER0
       lbra exit
**
* WORD DECORDER
**
WORD   BSR WORDS
       LDA SY
       RTS
WORDS  CLRB
       STB SY
       STB VAL
       LDA CH
** SKIP CONT  ,SPACE.;
WD1    CMPA #$21
       BCS SKIP
       CMPA #'.'
       BEQ SKIP
       CMPA #';'
       BNE WD2
SKIP   BSR GETCH
       BRA WD1
**
*  COMMENT
**
WD2    CMPA #'%'
       BNE WD3
       BSR GETCH
       CMPA #$20
       BCC *-4
       BRA WD1
**
* ASCC CONST
**
WD3    CMPA #'\''
       BNE WD4
       BSR GETCH
       STA VAL
       BSR GETCH
GETCH  PSHS X
       LBSR MEMIN
       STA CH
       PULS X,PC
* HEX CONSTANT
WD4    CMPA #'$'
       BNE WD5
WD40   BSR GETCH
       BSR TSTNA
       BEQ WD41
       BCC RTS2
       CMPA #'F'+1
       BCC RTS2
       SUBA #7
WD41   SUBA #'0'
       LDB VAL
       ASLB
       ASLB
       ASLB
       ASLB
       PSHS B
       ADDA ,S+
       STA VAL
       BRA WD40
**
* TEST ALPHA NUMERIC   Z=0 C=0 Not Number/Not Alpha
TSTNA  CMPA #'0'       Z=1 C=1 Number
       BCS NAF         Z=0 C=1 Not Number/Alpha
       CMPA #'9'+1
       BCS NT
       CMPA #'A'
       BCS NAF
       CMPA #'Z'+1
       BCS AT
NAF    CLRB CLEAR C
AT     LDB #-1
RTS2   RTS
NT     CLRB
       RTS
** DECIMAL CONSTANT
WD5    BSR TSTNA
       BNE WD6
WD50   SUBA #'0'
       PSHS A
       LDA VAL
       LDB #10
       MUL
       ADDB ,S+
       STB VAL
       BSR GETCH
       BSR TSTNA
       BEQ WD50
       RTS
* THE OTHER WORDS
WD6    LDX TEND
       PSHS A
       NEGA
       LEAX 1,X
       BSR STAONE
       PULS A
       BSR TSTNA   first word must alpha
       BCC TLOOK1
WD61   LDA CH
       BSR TSTNA   alpha numeric?
       BCS *+4
       BNE TLOOK1
       BSR STAONE
       BRA  WD61
STAONE STA ,X+     store to the table
       STX WEND
       BRA GETCH 
TLOOK1 LDA RSW     word end
       BEQ TLOOK   let's search
       COM RSW
       RTS
**
* WORD TABLE SEARCH
*
*  if not find then error
*  on return    X point last of word (VAL)
*          SY    7 larray 6 lvar 5 garray 4 gvar 3 func or proc 0 reserved word
*          VAL   word id or size
*          GL    1 local 0xff global
**
TLOOK  PSHS U
       LDA TCOUNT 
       STA SY
       LDX TEND 
S01    LDU WEND 
S02    LDA ,-U
       CMPA ,-X 
       BEQ S06
S03    TST ,X
       BEQ S05 
       BMI S04 
       LEAX -1,X
       BRA  S03 
S04    LEAX -1,X 
       BRA S01 
S05    DEC SY
       BNE S01
       LBRA ERROR
S06    TSTA 
       BPL S02
       LDB ,-X
       STB VAL
       LDA SY
       CMPA #1
       BNE *+6
       TFR B,A 
       BRA S07 
       LDB  #1
       CMPA #4 
       BCS  RTSS
       TST PMODE
       BEQ *+6
       CMPA #6
       BCS *+3 
       NEGB 
       STB GL
       ANDA #$FD
S07    STA SY
RTSS   PULS U,PC
**
* ARITHMATIC EXPRESSION
**
WEXPR  LBSR WORD
EXPR   BSR LEXPR
       BSR OLOAD 
RTE    RTS
** WORD * LEXPR
WLEXPR LBSR WORD
**
* LOGICAL EXPRESSION
**
LEXPR  BSR REXPR
LE1    LDY SY
       CMPA #$82
       BCS RTE
       CMPA #$8A+1
       BCC RTE
       PSHS A
       LBSR WORD
       BSR REXPR
       LDB LSW
       BNE LE2
       LBSR PUTHS
       FDB $0397 
       FCB WT1
       FCB $32
       PULS A
       ADDA #$10
       LDB #WT1
       LBSR PUTAB 
       BRA LE1
LE2    PULS A
       BSR OCORD 
       BRA LE1
**
* RELATIONAL EXPRESS 
**
REXPR  LBSR AEXPR
RE1    LDA SY
       CMPA #$21 
       BCS      RTE 
       CMPA #$30 
       BCC RTE
       PSHS A
       LBSR WORD 
       ASR AEXPR 
       LDA #$80 
       BSR AOPER 
       PULS A
       LBSR PUTHS
       FCB 6
       FCB $3,$4F,$20,$02,$86,$FF
       BRA RE1
** ADDING OPERATFR
AOPER  LDB LSW
       BNE OCORD 
       PSHS A
       CMPA #$80 
       BEQ  *+5 
       BSR PUTPUL
       FCB $8C 
       BSR PUTPLB
       PULS A
       SUBA #$70
PUTA1  LBRA PUTA
** OUTPUT SAVED L-CC 
OLOAD  LDA LSW 
       BEQ RTE 
       LDA ACC
       BEQ OL1
       LDD #$3402  pshs a
       LBSR PUTAB 
OL1    LDA #$86
OCORD  PSHS A
       CLRA
       STA LSW
       COMA
       STA ACC
       LDA AMODE
       CMPA #$20
       BNE *+5
       LBSR PUTX
       CLR LSW
       PULS A
       ADDA AMODE
       CMPA #$A6
       BEQ OCOFS
       CMPA #$E6
       BEQ OCOFS
       LDB OPER
       LBRA PUTAB
OCOFS  LDB OPER
       LBRA PUTOFS
* PUT 'TAB:PULS A'
PUTPUL LBSR PUTHS
       FCB 4
       FCB $1f,$89,$35,2     tfr a,b ; puls a
RTE1   RTS 
* PUT 'PULS B'
PUTPLB LDA #$3504 puls b
       LBRA PUTAB
**
*  ADDING EXPRESSION
**
AEXPR  BSR MEXPR
AE1    LDA SY
       CMPA #$80
       BEQ  AE2
       CMPA #$8B 
       BNE RTE1
AE2    PSHS A
       LBSR WORD
       BSR MEXPR
       PULS A
       LBSR AOPER
       BRA AE1
**
*  MUTIPLYING EXPRESSION
**
MEXPR  BSR TERM 
ME1    LDA SY
       CMPA #$8E
       BEQ ME2
       CMPA #$8F
       BNE RTE1
ME2    PSHS A
       LBSR WORD
       BSR TERM
       LDB LSW 
       BEQ ME3
       LDA #$C6
       BSR OCORD
       FCB $8C
ME3    BSR PUTPUL 
       PULS A
       CMPA #$8E 
       BHS ME4
       LBSR PUTHSL
       FCB 3
       FCB $BD
       FDB MULT
       BRA ME1
ME4    LBSR PUTHSL
       FCB 3
       FCB $BD
       FDB DIV
       BRA ME1
**
* TERM
**
TERM   LDA SY
       BNE TM1
* SAVE L-COMMAND 
SLOAD  PSHS A
       LBSR OLOAD
       LDA VAL 
       STA OPER 
       LDA GL
       STA GLL
       PULS A
       STA AMODE 
       COM LSW 
       LBRA WORD 
* CONST TRUE & FALSE
TM1     CMPA #$A0  
        BEQ TM01
        CMPA #$A1
        BNE TM2
TM01    SUBA #$A1
        STA VAL
        CLRA
        BRA SLOAD
* SYSTEM VAR MHIGH & MOD
TM2       CMPA #$16
        BEQ *+6
        CMPA #$17
        BNE TM3
        STA VAL
        LDA #$10
        BRA SLOAD
* SIMPLE VARIABLE 
TM3       CMPA #4
        BNE TM4
        LDA #$20
        BRA SLOAD
* ( EXPTRSSION ) 
TM4       CMPA #$35
        BCS TM5
        CMPA #$38
        BCC TM5
        PSHS A
        LBSR WLEXPR
        PULS A
        ADDA #4
        LBRA CHECK
** FUNCTION CALL
TM5       CMPA #3
        BMI *+6
        CMPA #$E0
        BCS TM6
        BSR OLP 
PFCALL LDA VAL
        PSHS A 
        LBSR WORD
        CMPA #$37 
        BNE PFC1 
        LDA LSIZE
        PSHS A 
        INC LSIZE 
PFC2       INC LSIZE 
        LBSR WEXPR
        CLRB
        STB ACC 
        DECB
        STB    GLL
        LDA #$A7 
        LDB LSIZE 
        LBSR PUTABX 
        LDA SY 
        CMPA #$3C 
        BEQ PFC2 
        PULS A
        STA LSIZE 
        LDA #$3B 
        LBSR CHECK 
PFC1   LDB #-1 
        STB ACC 
        LDA #$86 
        LDB LSIZE
       LBSR PUTAB
       PULS A
       CMPA #$C0
       BCC *+5
       LBRA CALPF
       LDX #LIBR
       SUBA #$C0
PFC3   BEQ PFC4
       LEAX 2,X
       DECA
       BRA PFC3
PFC4   LDA #$BD
       LBSR PUTA
       LDD ,X
       LBRA PUTAB
**
OLP    LBSR OLOAD
       LDB ACC 
       BEQ RTS4
       LDD #$3402     pshs a
       LBSR PUTAB
       CLR ACC 
RTS4   RTS
** FUNCTION RND
TM6    CMPA #$70 
       BNE TM61 
       BSR SUBSC
       LBSR PUTHS
       FCB $03BD
       FDB RND
       RTS
* FUNTION GET
TM61   CMPA #$71
       BNE TM62 
       BSR SUBSC
       LBSR PUTHS
       FDB $0297
       FCB INDN
       LBSR PUTHSL
       FCB $03BD
       FDB GETCH
       RTS
* FUNCTION READ 
TM62   CMPA #$72 
       BNE TM7
       BSR SUBSC
       LBSR PUTHS
       FDB $0297
       FDB INDN
       LBSR PUTHSL
       FCB $038D
       FDB GETDA
       RTS
* FUNCTION NOTASL ET AL
TM7    CMPA #$40
       BCS TM8
       CMPA #$49+1
       BCC TM8
       PSHS A
       BSR SUBSC
       PULS A
       LBRA PUTA
* ARRAY
TM8    CMPA #$5
       BNE TM9
       LDB VAL
       PSHS B
       LDB GL
       PSHS B
       BSR SUBSC1
       PULS B
       STB GLL
       PULS B
       LDA LSW
       BEQ ARY1
       ADDB OPER 
       STB OPER
       LDA #$20 
       STA AMODE
       RTS 
ARY1   LDA #$8B
       LBSR PUTABX
       LBRA LDAAX
**
DSUBSC LDA #$3C
       BSR SUBS1
       LDA #$3B
       PSHS A
       BRA SUBS2
SUBSC  LDA #$3E
SUBS1  PSHS A
       LBSR WORD
       LDA #$37
       LBSR CHECK
SUBS2  LBSR EXPR
       PULS A 
       LBRA CHECK
SUBSC1 LBSR WORD
       LDA #$36
       LBSR CHECK
       LBSR LEXPR
       LDB LSW
       BEQ SBS5
       LDB AMODE
       BEQ SBS5
       LBSR OLOAD
SBS5   LDA #$3A
       LBRA CHECK
* MEM FUNCTION
TM9    CMPA #6
       BNE TM10
       BSR DSUBSC
       LBSR PUTHS
       FDB $0997
       FCB WT2
       FDB $3297
       FCB WT1,$9E,WT1
       FDB $A600
       RTS
** FOR EXPANTION
TM10   LBRA ERROR
**
* ADDRESS DEPENDENT CODE
* GENARATION
**
* SET PRC—FUNC TABLE
**
SETPFT PSHS A 
       LDB #3
       MUL
       ADDD PFTBEG
       STD XR 
       PULS A,PC
**
TWICE  LBSR PUTSTR
       FCB $0D,$0A
       FCC "TWICE!",0
       lbra exit 
**
* DEF PROC-FUNC
**
DEFPF  BSR SETPFT
       LDX XR
       TST ,X
       BNE TWICE
       COM ,X
       LDX 1,X
       STX YR 
       LDX XR 
       LEAX 1,X
       BSR PCST 
       LDX YR
DP1    BEQ RT10
       LDX ,X
       STX ZR
       LDX YR
       BSR PCST 
       LDX ZR
       STX YR
       BRA DP1
**
* CALL PORC-FUNC
**
CALPF  BSR SETPFT
       INCA
       CMPA PFMAX
       BCS *+4
       STA PFMAX
       LDA #$BD
       BSR AOUT
       LDX XR
       LDD 1,X
       LDX PC
       BSR STAABX 
       LDX XR
       TST ,X
       BNE PC2ADD
       LEAX 1,X
       BSR PCST
       BRA PC2ADD
**
* PUL-DEF N TIMES
**
PLDFN  PSHS A
       BSR PULDEF
       DEC ,S
       BNE *-4
       PULS A,PC
**
* PULL AND DEFINE
**
PULDEF BSR PULSTK
PCST   LDD PC
STAABX STD ,X
RT10   RTS
**
JMPOUT LDA #$7E
AOUT   LDX PC
       STA ,X
       BRA INCPC1
**
PULSTK LDX SP
       LEAX 2,x
       STX SP
       LDD ,X
       LDX ,X
       RTS
**
*  PUL STACK & Jump
**
PULJMP BSR JMPOUT
       BSR PULSTK
PCST2  LDX PC
       BSR STAABX
INCPC  LEAX 1,X
INCPC1 LEAX 1,X
SETPC  STX PC
       RTS
**
* PUSH STACK & JUMP
**
PSHJMP BSR JMPOUT
       BSR PSHDEF
PC2ADD LDX PC
       BRA INCPC
**
* PUSH STRCK & DEFINE
**
PSHDEF LDX SP
       BSR PCST
       LEAX -2,X
       STX SP
       RTS
**
OUTPC3 BSR AOUT 
       LDD PC
       ADDD #3
       BRA PCST2
**
* LDA R,X
LDAAX  LDA #$B7
       BSR OUTPC3
       LDA #$A6
LA1    BSR AOUT 
       BRA INCPC1
**
* STA B,X
STABX  LDA #$F7
       BSR OUTPC3
       LDA #$A7
       BRA LA1
**
* STACK TOP CHANGE
**
STCHG  LDD 2,S
       LDX 4,S
       STD 4,S
       STX 2,S
       RTS
**
* PUTHSL output with address calculation
*   only working on 3 byte 7E/BD (JMP/JSR)
**
PUTHSL  LDX ,S++
       LDB ,X+
       LDA ,X+
       LBSR PUTA
       LDD ,X++
       leay 0,pcr
       leay d,y
       exg  d,y
       ldy  pc
       std  ,y++
       sty  pc
       JMP ,X
**
**
* END OF LOAD
**
ENDL   LDX PFTBEG
       LDA PFMAX
EL1    BEQ EL
       TST ,X
       BEQ UDERR
EL2    LEAX 3,X
        DECA
       BRA EL1
UDERR  PSHS A
       STA ZR
       SUBA PFMAX
       NEGA
       PSHS A
       LBSR PUTSTR
       FDB $0D0A
       FCC "UNDEF",0
       PULS A
       LBSR PUTCA
       PULS A
       LDX ZR
       BRA EL2
EL     LDX PC
       LBRA C

**********************
* ADVANCE WORD
**
WTABLE FCB 0 END MARK 
       FCB $30,-'P'
       FCC "ROC" 
       FCB $31,-'F'
       FCC "UNC" 
       FCB $32,-'V'
       FCC "AR"
       FCB $33,-'A'
       FCC "RRAY"
       FCB $34,-'B'
       FCC "EGIN"
       FCB $35,-';'
       FCB $36,-'['
       FCB $37,-'('
       FCB $38,-'E'
       FCC "ND"
       FCB $39,-'=' 
       FCB $3A,-']'
       FCB $3B,-')'
       FCB $3C,-','
       FCB $3D,-':'
       FCB $50,-'S'
       FCC "TOP"  
       FCB $51,-'R'
       FCC "ETURN"
       FCB $55,-'I'
       FCC "F"
       FCB $65,-'T'  
       FCC "HEN"
       FCB $53,-'F'  
       FCC "OR"  
       FCB $61,-'T'  
       FCC "O"
       FCB $52,-'R'
       FCC "EPEAT"
       FCB $60,-'U'
       FCC "NTIL"
       FCB $54,-'W'  
       FCC "HILE"
       FCB $64,-'D'  
       FCC "O"
       FCB $56,-'C'  
       FCC "ASE"
       FCB $66,-'O'  
       FCC "F"  
       FCB $67,-'E'  
       FCC "LSE"
       FCB $57,-'W'
       FCC "RITE"
       FCB $69,-'A'
       FCC "SCII"  
       FCB $6A,-'S'
       FCC "PACE"
       FCB $6B,-'C'  
       FCC "RLF"  
       FCB $6C,-'"'
       FCB $8B,-'+'
       FCB $80,-'-'
       FCB $8E,-'*'  
       FCB $8F,-'/'
       FCB $82,-'S'
       FCC "BC"
       FCB $84,-'A'
       FCC "ND"
       FCB $88,-'E'
       FCC "OR"
       FCB $89,-'A'  
       FCC "DC"
       FCB $8A,-'O'  
       FCC "R"
       FCB $22,-'>'  
       FCB $25,-'<'
       FCB $26,-'#'
       FCB $27,-'=' 
       FCB $2D,-'L','T'
       FCB $2E,-'G','T'
       FCB $40,-'N','E','G'
       FCB $43,-'N','O','T'
       FCB,$43,-'C','O','M'
       FCB $44,-'L','S','R'
       FCB $46,-'R','O','R'
       FCB $47,-'A','S','R'
       FCB $48,-'A','S','L'
       FCB $49,-'R','O','L'
       FCB $06,-'M','E','M'
       FCB $A0,-'T'
       FCC "RUE"
       FCB $A1,-'F'
       FCC "ALSE"
       FCB $16,-'M'
       FCC "HIGH"
       FCB $17,-'M'
       FCC "OD"
       FCB $70,-'R','N','D'
       FCB $71,-'G','E','T'
       FCB $72,-'R'
       FCC "EAD"
WTBLEND 

******
* SUPORTING ROUTINES
* & I/0 CONTROL
**
** OBJECT START
******
C      leas OBJECT,u
VARPTR LDX <PC
       STX GB
       STX LB
       lda INDN
       lbsr close
       clra       os9 stdin
       sta INDN
       inca
       sta OUTDN
OBJMP  JMP OBJECT,u

**
* PUSH LB & SET NEW LB
**
PSHLB  pshs y
       leay ,x
       leax a,x
       sty ,x++
       puls y,pc
**
* PULL LB
**
*  
PULLB  LDX ,--X
       TSTA
       RTS
**
* RND FUNCTION
**
RND    PSHS A
       LDA RNDL
       LDB #125
       MUL
       ADDD #1
       STA RNDL
       PSHS A
       LDA RNDH
       LDB #125
       MUL
       ADDA ,S+
       STA RNDH
       PULS B
       MUL
       INCA
       RTS
**
* DVISITION SET MOD 
**
DIV    STB WT1
       BEQ ERDIV 
       TFR A,B
       CLRA
       STA WT2
DV0    INC WT2
       ASL WT1
       BCC DV0
DV1    ROR WT1
       CMPB WT1
       BCS DV2
       SUBB WT1
       ORCC #1
       BRA *+4
DV2    ANDCC #$FE CLR C
       ROLA
       DEC WT2
       BNE DV1
       STB MOD
       TSTA
       RTS
**
ERDIV  STB OUTDN
       LBSR CRLF
       LBSR PUTSTR
       FCC "ERR DIV 0",0
MONIT  lbra exit
**
MULT   MUL
       STA MHIGH
       TFR B,A
       RTS
**
* PUT A IN DECIMAL
**
PUTDA  CLRB
PUTDR  STB DREG
       PSHS X
       LEAX -2,S
       LEAS -6,S
       CLR 1,X
       LDB #3
P0     PSHS B
       LDB #10
       BSR DIV
       ADDB #$30
       STB ,X
       LEAX -1,X
       PULS B
       DECB
       BNE P0
       COM 3,X
       LDA #'0'
       LDB #4
P1     LEAX 1,X
       DECB
       CMPA ,X
       BEQ P1
       COM 4,S
       LDA DREG
       PSHS B
       SUBA ,S+
       BCS PRX
       BSR SPACEA
PRX    LDA ,X
       BEQ P4
       LBSR PUTCA 
       LEAX 1,X 
       BRA PRX 
P4     LEAS 6,S
CL1    PULS X,PC
** 
* GET IN A DECIMAL
** 
GETDA  CLRA
       PSHS A
       LBSR GETCA 
       CMPA #RUB 
       BNE GD1 
       PULS A
       LDB #10 
       LBSR DIV
       BRA GETDA+1 
GD1    SUBA #'0'
       BCS GD2
       CMPA #10
       BCC GD2 
       STA DBUF 
       PULS A
       LDB #10 
       MUL
       ADDB DBUF 
       TFR B,A
       BRA GETDA+1 
GD2    PULS A,PC
**
* SPACE A TIMES 
**
SPACEA BEQ CL1
       PSHS A
       LDA #' '
       LBSR PUTCA
       PULS A
       DECA
       BRA SPACEA
**
* STR OUT
**
PUTSTR PSHS X
       LDX 2,S
STR1   LDA ,X+
       BEQ STR2
       LBSR PUTCA
       BRA STR1
STR2   STX 2,S
       PULS X,PC
**
* CRLF
**
CRLF   LDA #$0D
       BSR PUTCA 
       LDA #$0A
       BRA PUTCA
**
* CRLF A TIMES
**
CRLFA  BEQ CL1
       PSHS A
       BSR CRLF 
       PULS A
       DECA
       BRA CRLFA


start    clr   <stdin
         stx   <parmptr         save parameter pointer
         stu   <work            save parameter pointer
         lda   #READ.           read access mode
         os9   I$Open           open file
         lbcs   L0049            branch if error
         sta   <INDN            else save path to file
         stx   <parmptr         and updated parm pointer
         leax  readbuff,u       buffer 
         clr   ,x               buffer empty
         stx   <adr
         lbra  comp

copytbl
         pshs  y,x,u
         leau  LIBR,y
         leax  iotbl,pcr
         leay  iotblend,pcr
         ldy   #(iotblend-iotbl)
l1       ldb   #$7e     * JMP
         stb   ,u+
         ldd   ,x++
         addb  1,s
         adca  ,s
         std   ,u++
         cmpx  2,s
         ble   l1
         puls  x,y,u
 
Exit     lbsr        setecho
*        ldx         <work
*        leax        readbuff,x
*        ldb         #1
*        lbsr         getline
*        lbsr         getpoll
*        lda        <stdin        
*        os9        I$Close      

        clrb
        os9        F$Exit
*       no return


iotbl
         fdb   getchar-iotbl            ; 0
         fdb   putchar-iotbl            ; 3
         fdb   getline-iotbl            ; 6
         fdb   putline-iotbl            ; 9
         fdb   putcr-iotbl              ; $C
         fdb   getpoll-iotbl            ; $F
         fdb   xopenin-iotbl            ; $12
         fdb   xopenout-iotbl           ; $15
         fdb   xabortin-iotbl           ; $18
         fdb   xclosein-iotbl           ; $1B
         fdb   xcloseout-iotbl          ; $1E
         fdb   delay-iotbl              ; $21
         fdb   noecho-iotbl             ; $24
         fdb   setecho-iotbl            ; $27
         fdb   exit-iotbl               ; $2a
iotblend

err     ldb    #1
L0049
        bra     Exit


PUTCA   tfr         a,b
putchar                        * Output one character in B register.
        PSHS        X,Y
        BRA         OUTCH1

close
         lda   <INDN        else get path
         os9   I$Close          and close it
         bcs   L0049            branch if error
         rts

MEMIN
        PSHS        A,B,X,Y
        ldx         <adr
        lda         ,x+
        bne         GETCA1
        LDA         INDN
        LEAX        readbuff,u
        LDY         #bufsiz
        OS9         I$Read
        BCC         GETCA0
        lda         #'/'
        ldx         <adr
        bra         GETCA1
GETCA0  LEAX        readbuff,u
        tfr         y,d
        clr         d,x      eof
        lda         ,x+
GETCA1  stx         <adr
        sta         ,s
        PULS        A,B,X,Y,PC

GETCA   bsr         getchar
        tfr         b,a
        rts

getchar                        * Input one character into B register.
        PSHS        A,B,X,Y
GETCH0
        LDA         INDN
        LEAX        ,S
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
        LDA         OUTDN
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
        clr         ,s
GETLN0
        ldy         ,s
        lda         INDN   
        OS9         I$ReadLn
        BCS         GETLN0
        LEAY        -1,Y
GETLN1  STY         ,S
        PULS        A,B,X,Y,PC
putline                        * Output string at address in X, length in B.
        PSHS        A,B,X,Y
        CLRA
        TFR         D,Y
        lda         OUTDN
        OS9         I$WritLn
        PULS        A,B,X,Y,PC
xopenin
xopenout
xabortin
xclosein
xcloseout
        RTS

setecho lda          #1
        bra          sss
noecho  clra
sss     leas         -128,s
        leax        ,s
        pshs         a
        clra  
        ldb          #SS.Opt
        OS9         I$GetStt
        bcs         err2
        LDA         ,s
        STA         PD.EKO-PD.OPT,X
setopts
        ldb         #SS.Opt         
        clra        
        OS9         I$SetStt
err2
        puls        a
        leas        128,s
        rts


delay   PSHS        D,X  * address **$21** 
                         * On input the D register contains the number of timer 
                         * ticks to wait. Each timer tick is 20ms
        TFR         D,X
        OS9         F$Sleep
        PULS        D,X,PC


         emod
eom      equ   *
         end

