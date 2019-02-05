
* * micro-C driver under FLEX * * 12-Dec-81 M.Ohta,H.Tezuka
* 12-Dec-2018 for OS-9 S.Kono * 

*
* micro-C user program
*
_tylg     set   Prgrm+Objct   
_atrv     set   ReEnt+rev
_rev      set   $00
_edition  set   5
        ifp1
            use   defsfile
        endc


*	OPT	LIST
	INCLUDE	"c.out"		include compilers output
*	OPT	NOL

*       x    arguments pointer
*       u    global variable area
_start
_00000
	LEAS	-256,S		ALLOCATE WORK AREA
        leay    _name,pcr
        pshs    x,y             POINT TO CONTENT OF ARGUMENT VECTOR
        leay    ,u 
	PSHS	Y

*       allocate memory and change stack
*       try to allocate maximum memory if not specified
    ifndef  __MEMSIZ
        LDD     #(1024*48) 
    else
        LDD     #__MEMSIZ
    endif
        pshs     d
__0C004
        os9      F$Mem
        bcc     __0C005
        ldd      ,s
        subd     #$1000
        lblo      exit           can't get any memroy
        std      ,s
        bra     __0C004
__0C005
*       y is heap upper bound
* copy arg string
        ldx     4,s
__0C007 tst     ,x+
        bne     __0C007
        clr     ,-y
__0C008 lda     ,-x
        sta     ,-y
        cmpx    4,s
        bne     __0C008
        sty     4,s
        leax    ,y
*  copy saved arg into new stack
*  and change the stack
        ldy     2,s
        ldd     4,s
        std     ,--x
        ldd     6,s
        std     ,--x
        leas    ,x
*                         	clear globals on Y
	LDD	#_GLOBALS
_0C002	CLR	D,Y
        subd     #1
	BNE	_0C002
_0C003	
        tfr     y,d
        addd    #_GLOBALS
        std     heapp,y
	LBSR	_INITIALIZE	call initializer
	LBSR	_main
* exit	clrb
	os9     F$Exit


*
* run time support
*

*
_00001	PSHS	D,X,Y		multiply
	
	LDA	,S
	LDB	3,S
	MUL
	STB	4,S
	
	LDD	1,S
	MUL
	STB	5,S
	
	LDA	1,S
	LDB	3,S
	MUL
	ADDA	4,S
	ADDA	5,S
	
	LEAS	6,S
initheap
	RTS
*
_00002	CLR	,-S		signed divide
	
	CMPX	#0
	BPL	_02000
	
	COM	,S
	
	EXG	D,X
	LBSR	_00020
	EXG	D,X

_02000	TSTA
	BPL	_02001
	
	COM	,S
	
	LBSR	_00020
	
_02001	LBSR	_00010
	TFR	X,D
	TST	,S+
	BPL	_02002
	
	LBSR	_00020
	
_02002	RTS
*
_00003	LBSR	_00010		unsigned divide
	TFR	X,D
	RTS
*
_00004	CLR	,-S		signed modulous
	
	CMPX	#0
	BPL	_04000
	
	EXG	D,X
	BSR	_00020
	EXG	D,X

_04000	TSTA
	BPL	_04001
	
	COM	,S
	BSR	_00020
	
_04001	BSR	_00010
	
	TST	,S+
	BPL	_04002
	
	BSR	_00020
	
_04002	RTS
*
_00005	BSR	_00010		unsigned modulous

	RTS
*
_00006	CMPX	#0		signed left shift
	BMI	_06001
 
_06000	BEQ	_06009
	LSLB
	ROLA
	LEAX	-1,X
	BRA	_06000
	
_06001	BEQ	_06009
	ASRA
	RORB
	LEAX	1,X
	BRA	_06001
	
_06009	RTS
*
_00007	CMPX	#0		unsined left shift
	BMI	_07001
	
_07000	BEQ	_07009
	LSLB
	ROLA
	LEAX	-1,X
	BRA	_07000
	
_07001	BEQ	_07009
	LSRA
	RORB
	LEAX	1,X
	BRA	_07001
	
_07009	RTS
*
_00008	CMPX	#0		sined right shift
	BMI	_08001
	
_08000	BEQ	_08009
	ASRA
	RORB
	LEAX	-1,X
	BRA	_08000
	
_08001	BEQ	_08009
	LSLB
	ROLA
	LEAX	1,X
	BRA	_08001
	
_08009	RTS
*
_00009	CMPX	#0		unsined right shift
	BMI	_09001
	
_09000	BEQ	_09009
	LSRA
	RORB
	LEAX	-1,X
	BRA	_09000
	
_09001	BEQ	_09009
	LSLB
	ROLA
	LEAX	1,X
	BRA	_09001
	
_09009	RTS
*
_00020	NEGA			negate D reg
	NEGB
	SBCA	#0
	RTS
*
_00010	PSHS	D,X		divide subroutine
	
	CLRA
	CLRB
	
	LDX	#17
	
_00011	SUBD	2,S
	BCC	_00012
	
	ADDD	2,S
	
_00012	ROL	1,S
	ROL	,S
	ROLB
	ROLA
	
	LEAX	-1,X
	BNE	_00011
	
	RORA
	RORB
	
	COM	1,S
	COM	,S
	PULS	X
	
	LEAS	2,S
	RTS
*
*
*
*
        emod
_eom
