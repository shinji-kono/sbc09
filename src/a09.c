/* A09, 6809 Assembler2

   created 1993,1994 by L.C. Benschop.
   copyleft (c) 1994-2014 by the sbc09 team, see AUTHORS for more details.
   license: GNU General Public License version 2, see LICENSE for more details.
   THERE IS NO WARRANTY ON THIS PROGRAM. 

   Generates binary image file from the lowest to
   the highest address with actually assembled data.

   Machin edependencies:
                  char is 8 bits.
                  short is 16 bits.
                  integer arithmetic is twos complement.

   syntax a09 [-o filename] [-l filename] sourcefile.

   Options
   -o filename name of the output file (default name minus a09 suffix)
   -s filename name of the s-record output file (default its a binary file)
   -l filename list file name (default no listing)
   -d enable debugging

   recognized pseudoops:
    extern public
    macro endm if else endif
    org equ set setdp
    fcb fcw fdb fcc rmb
    end include title

   Not all of these are actually IMPLEMENTED!!!!!! 

   Revisions:
        1993-11-03 v0.1 
                Initial version.
        1994/03/21 v0.2
                Fixed PC relative addressing bug
                Added SET, SETDP, INCLUDE. IF/ELSE/ENDIF
                No macros yet, and no separate linkable modules.
        2012-06-04 j at klasek at
                New: debugging parameter/option.
                Fixed additional possible issue PC relative addressing.
                Compatibility: Octal number prefix "&".
        2014-07-15 j at klasek at
                Fixed usage message.
        2018-07-11
                leax  $ED00/256,x kernel offset in map
                should be positive offset expr should be int(32bit)
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define NLABELS (2048*2)
#define MAXIDLEN 16
#define MAXLISTBYTES 8
#define FNLEN 30
#define LINELEN 128

static int debug=0;
static struct incl {
    char *name;
    struct incl *next;
} *incls = 0;

static struct longer {
    int gline;
    int change;
     struct longer *next;
} *lglist = 0;


struct oprecord{char * name;
                unsigned char cat;
                unsigned short code;};

#define EXPERR 1
#define ILLAERR 2
#define UDEFLABELERR 4
#define MULTLABELERR 8
#define RBRACHERR 16
#define MSSINGLBLERR 32
#define ILLNMERR 0x8000

/* Instruction categories:
   0 one byte oprcodes   NOP
   1 two byte opcodes    SWI2
   2 opcodes w. imm byte ANDCC
   3 LEAX etc.
   4 short branches. BGE
   5 long branches 2byte opc LBGE
   6 long branches 1byte opc LBRA
   7 accumulator instr.      ADDA
   8 double reg instr 1byte opc LDX
   9 double reg instr 2 byte opc LDY
   10 single address instrs NEG
   11 TFR, EXG
   12 push,pull
   13 pseudoops
*/

struct oprecord optable[]={
  {"ABX",0,0x3a},{"ADCA",7,0x89},{"ADCB",7,0xc9},
  {"ADDA",7,0x8b},{"ADDB",7,0xcb},{"ADDD",8,0xc3},
  {"ANDA",7,0X84},{"ANDB",7,0xc4},{"ANDCC",2,0x1c},
  {"ASL",10,0x08},{"ASLA",0,0x48},{"ASLB",0,0x58},
  {"ASR",10,0x07},{"ASRA",0,0x47},{"ASRB",0,0x57},
  {"BCC",4,0x24},{"BCS",4,0x25},{"BEQ",4,0x27},
  {"BGE",4,0x2c},{"BGT",4,0x2e},{"BHI",4,0x22},
  {"BHS",4,0x24},{"BITA",7,0x85},{"BITB",7,0xc5},
  {"BLE",4,0x2f},{"BLO",4,0x25},{"BLS",4,0x23},
  {"BLT",4,0x2d},{"BMI",4,0x2b},{"BNE",4,0x26},
  {"BPL",4,0x2a},{"BRA",4,0x20},{"BRN",4,0x21},
  {"BSR",4,0x8d},
  {"BVC",4,0x28},{"BVS",4,0x29},
  {"CLC",1,0x1cfe},{"CLF",1,0x1cbf},{"CLI",1,0x1cef},
  {"CLIF",1,0x1caf},
  {"CLR",10,0x0f},{"CLRA",0,0x4f},{"CLRB",0,0x5f},
  {"CLV",1,0x1cfd},
  {"CMPA",7,0x81},{"CMPB",7,0xc1},{"CMPD",9,0x1083},
  {"CMPS",9,0x118c},{"CMPU",9,0x1183},{"CMPX",8,0x8c},
  {"CMPY",9,0x108c},
  {"COM",10,0x03},{"COMA",0,0x43},{"COMB",0,0x53},
  {"CWAI",2,0x3c},{"DAA",0,0x19},
  {"DEC",10,0x0a},{"DECA",0,0x4a},{"DECB",0,0x5a},
  {"DES",1,0x327f},{"DEU",1,0x335f},{"DEX",1,0x301f},
  {"DEY",1,0x313f},
  {"ELSE",13,1},
  {"EMOD",13,25},
  {"END",13,2},
  {"ENDC",13,3},
  {"ENDIF",13,3},
  {"ENDM",13,4},
  {"EORA",7,0x88},{"EORB",7,0xc8},
  {"EQU",13,5},{"EXG",11,0x1e},{"EXTERN",13,6},
  {"FCB",13,7},{"FCC",13,8},
  {"FCS",13,23},
  {"FCW",13,9},
  {"FDB",13,9},
  {"IF",13,10},
  {"IFEQ",13,30},
  {"IFGT",13,29},
  {"IFNDEF",13,33},
  {"IFNE",13,28},
  {"IFP1",13,21},
  {"INC",10,0x0c},{"INCA",0,0x4c},{"INCB",0,0x5c},
  {"INCLUDE",13,16},
  {"INS",1,0x3261},{"INU",1,0x3341},{"INX",1,0x3001},
  {"INY",1,0x3121},{"JMP",10,0x0e},{"JSR",8,0x8d},
  {"LBCC",5,0x1024},{"LBCS",5,0x1025},{"LBEQ",5,0x1027},
  {"LBGE",5,0x102c},{"LBGT",5,0x102e},{"LBHI",5,0x1022},
  {"LBHS",5,0x1024},
  {"LBLE",5,0x102f},{"LBLO",5,0x1025},{"LBLS",5,0x1023},
  {"LBLT",5,0x102d},{"LBMI",5,0x102b},{"LBNE",5,0x1026},
  {"LBPL",5,0x102a},{"LBRA",6,0x16},{"LBRN",5,0x1021},
  {"LBSR",6,0x17},
  {"LBVC",5,0x1028},{"LBVS",5,0x1029},
  {"LDA",7,0x86},{"LDB",7,0xc6},{"LDD",8,0xcc},
  {"LDS",9,0x10ce},{"LDU",8,0xce},{"LDX",8,0x8e},
  {"LDY",9,0x108e},{"LEAS",3,0x32},
  {"LEAU",3,0x33},{"LEAX",3,0x30},{"LEAY",3,0x31},
  {"LSL",10,0x08},{"LSLA",0,0x48},{"LSLB",0,0x58},
  {"LSR",10,0x04},{"LSRA",0,0x44},{"LSRB",0,0x54},
  {"MACRO",13,11},
  {"MOD",13,24},
  {"MUL",0,0x3d},
  {"NAM",13,26},
  {"NEG",10,0x00},{"NEGA",0,0x40},{"NEGB",0,0x50},
  {"NOP",0,0x12},
  {"OPT",13,19},
  {"ORA",7,0x8a},{"ORB",7,0xca},{"ORCC",2,0x1a},
  {"ORG",13,12},
  {"OS9",13,32},
  {"PAG",13,20}, {"PAGE",13,20},
  {"PSHS",12,0x34},{"PSHU",12,0x36},{"PUBLIC",13,13},
  {"PULS",12,0x35},{"PULU",12,0x37},{"RMB",13,0},
  {"ROL",10,0x09},{"ROLA",0,0x49},{"ROLB",0,0x59},
  {"ROR",10,0x06},{"RORA",0,0x46},{"RORB",0,0x56},
  {"RTI",0,0x3b},{"RTS",0,0x39},
  {"SBCA",7,0x82},{"SBCB",7,0xc2},
  {"SEC",1,0x1a01},{"SEF",1,0x1a40},{"SEI",1,0x1a10},
  {"SEIF",1,0x1a50},{"SET",13,15},
  {"SETDP",13,14},{"SEV",1,0x1a02},{"SEX",0,0x1d},{"SPC",13,20},
  {"STA",7,0x87},{"STB",7,0xc7},{"STD",8,0xcd},
  {"STS",9,0x10cf},{"STU",8,0xcf},{"STX",8,0x8f},
  {"STY",9,0x108f},
  {"SUBA",7,0x80},{"SUBB",7,0xc0},{"SUBD",8,0x83},
  {"SWI",0,0x3f},{"SWI2",1,0x103f},{"SWI3",1,0x113f},
  {"SYNC",0,0x13},{"TFR",11,0x1f},
  {"TITLE",13,18},
  {"TST",10,0x0d},{"TSTA",0,0x4d},{"TSTB",0,0x5d},
  {"TTL",13,18},
  {"USE",13,27},
};

struct symrecord{char name[MAXIDLEN+1];
                 char cat;
                 unsigned short value;
                 struct symrecord *next;
                };

int symcounter=0;
int os9 = 0;       // os9 flag
int rmbmode = 0;   // in os9 work area 
struct symrecord * prevlp = 0;

/* expression categories...
   ECORD   all zeros is ordinary constant.
   ECADR   bit 1 indicates address within module.
   ECEXT   bit 2 indicates external address.
   ECLBL   bit 3 public label
   ECABS   bit 4 indicates this can't be relocated if it's an address.
   ECNEG   bit 5 indicates address (if any) is negative.
*/


/* Symbol categories.                               exprcat ( symcat & 0xe )
   0 SCC      Constant value (from equ).                     ECORD
   1 SCV      Variable value (from set)                      ECORD
   2 SCC__ADR Address within program module (label).         ECADR
   3 SCV__ADR Variable containing address.                   ECADR
   4 SC_E_ADR Adress in other program module (extern)        ECEXT
   5 SCVE_ADR Variable containing external address.          ECEXT
   6 SCU _ADR Unresolved address.                            ECEXT+ECADR
   7 SCV_UADR Variable containing unresolved address.        ECEXT+ECADR
   8 SC___LBL Public label.                                  ECLBL
   9 SCMACRO  Macro definition.                              xxx
  10 SCU__LBL Public label (yet undefined).                  ECADR+ECLBL
  11 SCPARAM  parameter name.                                ECADR+ECLBL
  12 SCLOCAL  local label.                                   ECEXT+ECLBL
  13 SCEMPTY  empty.                                         xxx
*/

struct symrecord symtable[NLABELS];

void processfile(char *name);

struct oprecord * findop(char * nm)
/* Find operation (mnemonic) in table using binary search */
{
 int lo,hi,i,s;
 lo=0;hi=sizeof(optable)/sizeof(optable[0])-1;
 do {
  i=(lo+hi)/2;
  s=strcmp(optable[i].name,nm);
  if(s<0) lo=i+1;
  else if(s>0) hi=i-1;
  else break;
 } while(hi>=lo);
 if (s) return NULL;
 return optable+i;
}

struct symrecord * findsym(char * nm) {
/* finds symbol table record; inserts if not found
   uses binary search, maintains sorted table */
 int lo,hi,i,j,s;
 lo=0;hi=symcounter-1;
 s=1;i=0;
 while (hi>=lo) {
  i=(lo+hi)/2;
  s=strcmp(symtable[i].name,nm);
  if(s<0) lo=i+1;
  else if(s>0) hi=i-1;
  else break;
 }
 if(s) {
  i=(s<0?i+1:i);
  if(symcounter==NLABELS) {
   fprintf(stderr,"Sorry, no storage for symbols!!!");
   exit(4);
  }
  for(j=symcounter;j>i;j--) {
      struct symrecord *from = &symtable[j-1];
      if (prevlp == from)  prevlp++; 
      if (from->next && from->next - symtable > i) from->next ++;
      symtable[j]=symtable[j-1];
  }
  symcounter++;
  strcpy(symtable[i].name,nm);
  symtable[i].cat=13;
 }
 return symtable+i;
}

FILE *listfile,*objfile;
char *listname,*objname,*srcname,*curname;
int lineno,glineno;

void
outsymtable()
{
 int i,j=0;
 fprintf(listfile,"\nSYMBOL TABLE");
 for(i=0;i<symcounter;i++)
 if(symtable[i].cat!=13) {
  if(j%4==0)fprintf(listfile,"\n");
  fprintf(listfile,"%10s %02d %04x",symtable[i].name,symtable[i].cat,
                       symtable[i].value);
  j++;
 }
 fprintf(listfile,"\n");
}

struct regrecord{char *name;unsigned char tfr,psh;};
struct regrecord regtable[]=
                 {{"D",0x00,0x06},{"X",0x01,0x10},{"Y",0x02,0x20},
                  {"U",0x03,0x40},{"S",0x04,0x40},{"PC",0x05,0x80},
                  {"A",0x08,0x02},{"B",0x09,0x04},{"CC",0x0a,0x01},
                  {"CCR",0x0a,0x01},{"DP",0x0b,0x08},{"DPR",0x0b,0x08}};

struct regrecord * findreg(char *nm)
{
 int i;
 for(i=0;i<12;i++) {
  if(strcmp(regtable[i].name,nm)==0) return regtable+i;
 }
 return 0;
}


char pass;             /* Assembler pass=1 or 2 */
char listing;          /* flag to indicate listing */
char relocatable;      /* flag to indicate relocatable object. */
char terminate;        /* flag to indicate termination. */
char generating;       /* flag to indicate that we generate code */
unsigned short loccounter,oldlc,prevloc,rmbcounter;  /* Location counter */

char inpline[128];     /* Current input line (not expanded)*/
char srcline[128];     /* Current source line */
char * srcptr;         /* Pointer to line being parsed */

char unknown;          /* flag to indicate value unknown */
char certain;          /* flag to indicate value is certain at pass 1*/
int error;             /* flags indicating errors in current line. */
int errors;            /* number of errors */
char exprcat;          /* category of expression being parsed, eg.
                          label or constant, this is important when
                          generating relocatable object code. */

void seterror(int e) {
    error |= e;
}

void makelonger(int gl) {
    if (pass==1) return;
    for(struct longer *p=lglist;p;p=p->next) {
        if (p->gline==gl) { // already fixed
            p->change = 1;
            return;
        }
    }
    struct longer *p = (struct longer *)calloc(sizeof(struct longer *),1);
    p->gline=gl;
    p->next = lglist;
    lglist = p;
}

int longer() {
    for(struct longer *p=lglist;p;p=p->next) {
        if (p->change == 0) return 1;
    }
    return 0;
}
void generate()
{
    generating = 1;
    if (rmbmode) {
        rmbcounter = loccounter;
        oldlc = loccounter = prevloc;
        rmbmode = 0;
    }
}


char namebuf[MAXIDLEN+1];

void
err(int er) {
    error |= er ;
}

void
scanname()
{
 int i=0;
 char c;
 while(1) {
   c=*srcptr++;
   if(c>='a'&&c<='z')c-=32;
   if(c!='_'&&c!='@'&&c!='.'&&c!='$'&&(c<'0'||c>'9')&&(c<'A'||c>'Z'))break;
   if(i<MAXIDLEN)namebuf[i++]=c;
 }
 namebuf[i]=0;
 srcptr--;
}

void
skipspace()
{
 char c;
 do {
  c=*srcptr++;
 } while(c==' '||c=='\t');
 srcptr--;
}

int scanexpr(int);

int scandecimal()
{
 char c;
 int t=0;
 c=*srcptr++;
 while(isdigit(c)) {
  t=t*10+c-'0';
  c=*srcptr++;
 }
 srcptr--;
 return t;
}

int scanhex()
{
 int t=0,i=0;
 srcptr++;
 scanname();
 while(namebuf[i]>='0'&&namebuf[i]<='F') {
  t=t*16+namebuf[i]-'0';
  if(namebuf[i]>'9')t-=7;
  i++;
 }
 if(i==0)seterror(1);
 return t;
}

int scanchar()
{
 int t;
 srcptr++;
 if (*srcptr=='\\')srcptr++;
 t=*srcptr;
 if(t)srcptr++;
 if (*srcptr=='\'')srcptr++;
 return t;
}

int scanbin()
{
 char c;
 int t=0;
 srcptr++;
 c=*srcptr++;
 while(c=='0'||c=='1') {
  t=t*2+c-'0';
  c=*srcptr++;
 }
 srcptr--;
 return t;
}

int scanoct()
{
 char c;
 int t=0;
 srcptr++;
 c=*srcptr++;
 while(c>='0'&&c<='7') {
  t=t*8+c-'0';
  c=*srcptr++;
 }
 srcptr--;
 return t;
}


int scanlabel()
{
 struct symrecord * p;
 scanname();
 p=findsym(namebuf);
 if(p->cat==13) {
   p->cat=6;
   p->value=0;
 }
 if(p->cat==9||p->cat==11)seterror(1);
 exprcat=p->cat&14;
 if(exprcat==6||exprcat==10)unknown=1;
 if(((exprcat==2||exprcat==8)
     && (unsigned short)(p->value)>(unsigned short)loccounter)||
     exprcat==4)
   certain=0;
 if(exprcat==8||exprcat==6||exprcat==10)exprcat=2;
 return p->value;
}


int scanfactor()
{
 char c;
 int t;
 skipspace();
 c=*srcptr;
 if(isalpha(c)||c=='_')return scanlabel();
 else if(isdigit(c))return scandecimal();
 else switch(c) {
  case '*' : srcptr++;exprcat|=2; if(rmbmode) return prevloc; else return loccounter;
  case '.' : srcptr++;exprcat|=2; if(os9&&!rmbmode) return rmbcounter; else return loccounter;
  case '$' : return scanhex();
  case '%' : return scanbin();
  case '&' : /* compatibility */
  case '@' : return scanoct();
  case '\'' : return scanchar();
  case '(' : srcptr++;t=scanexpr(0);skipspace();
             if(*srcptr==')')srcptr++;else seterror(1);
             return t;
  case '-' : srcptr++;exprcat^=32;return -scanfactor();
  case '+' : srcptr++;return scanfactor();
  case '!' : srcptr++;exprcat|=16;return !scanfactor();
  case '^' : 
  case '~' : srcptr++;exprcat|=16;return ~scanfactor();
 }
 seterror(1);
 return 0;
}

#define EXITEVAL {srcptr--;return t;}

#define RESOLVECAT if((oldcat&15)==0)oldcat=0;\
           if((exprcat&15)==0)exprcat=0;\
           if((exprcat==2&&oldcat==34)||(exprcat==34&&oldcat==2)) {\
             exprcat=0;\
             oldcat=0;}\
           exprcat|=oldcat;\
/* resolve such cases as constant added to address or difference between
   two addresses in same module */


int scanexpr(int level) /* This is what you call _recursive_ descent!!!*/
{
 int t,u;
 char oldcat,c;
 exprcat=0;
 if(level==10)return scanfactor();
 t=scanexpr(level+1);
 while(1) {
  // skipspace();
  c=*srcptr++;
  switch(c) {
  case '*':oldcat=exprcat;
           t*=scanexpr(10);
           exprcat|=oldcat|16;
           break;
  case '/':oldcat=exprcat;
           u=scanexpr(10);
           if(u)t/=u;else seterror(1);
           exprcat|=oldcat|16;
           break;
  case '%':oldcat=exprcat;
           u=scanexpr(10);
           if(u)t%=u;else seterror(1);
           exprcat|=oldcat|16;
           break;
  case '+':if(level==9)EXITEVAL
           oldcat=exprcat;
           t+=scanexpr(9);
           RESOLVECAT
           break;
  case '-':if(level==9)EXITEVAL
           oldcat=exprcat;
           t-=scanexpr(9);
           exprcat^=32;
           RESOLVECAT
           break;
  case '<':if(*(srcptr)=='<') {
            if(level>=8)EXITEVAL
            srcptr++;
            oldcat=exprcat;
            t<<=scanexpr(8);
            exprcat|=oldcat|16;
            break;
           } else if(*(srcptr)=='=') {
            if(level>=7)EXITEVAL
            srcptr++;
            oldcat=exprcat;
            t=t<=scanexpr(7);
            exprcat|=oldcat|16;
            break;
           } else {
            if(level>=7)EXITEVAL
            oldcat=exprcat;
            t=t<scanexpr(7);
            exprcat|=oldcat|16;
            break;
           }
  case '>':if(*(srcptr)=='>') {
            if(level>=8)EXITEVAL
            srcptr++;
            oldcat=exprcat;
            t>>=scanexpr(8);
            exprcat|=oldcat|16;
            break;
           } else if(*(srcptr)=='=') {
            if(level>=7)EXITEVAL
            srcptr++;
            oldcat=exprcat;
            t=t>=scanexpr(7);
            exprcat|=oldcat|16;
            break;
           } else {
            if(level>=7)EXITEVAL
            oldcat=exprcat;
            t=t>scanexpr(7);
            exprcat|=oldcat|16;
            break;
           }
  case '!':if(level>=6) {
               if (*srcptr=='=') {
                   srcptr++;
                   oldcat=exprcat;
                   t=t!=scanexpr(6);
                   exprcat|=oldcat|16;
               } else {
                   oldcat=exprcat;
                   t|=scanexpr(6);
                   exprcat|=oldcat|16;
               }
           }
           break;
  case '=':if(level>=6)EXITEVAL
           if(*srcptr=='=')srcptr++;
           oldcat=exprcat;
           t=t==scanexpr(6);
           exprcat|=oldcat|16;
           break;
  case '&':if(level>=5)EXITEVAL
           oldcat=exprcat;
           t&=scanexpr(5);
           exprcat|=oldcat|16;
           break;
  case '^':if(level>=4)EXITEVAL
           oldcat=exprcat;
           t^=scanexpr(4);
           exprcat|=oldcat|16;
           break;
  case '|':if(level>=3)EXITEVAL
           oldcat=exprcat;
           t|=scanexpr(3);
           exprcat|=oldcat|16;
  default: EXITEVAL
  }
 }
}

char mode; /* addressing mode 0=immediate,1=direct,2=extended,3=postbyte
               4=pcrelative(with postbyte) 5=indirect 6=pcrel&indirect*/
char opsize; /*desired operand size 0=dunno,1=5,2=8,3=16*/
short operand;
unsigned char postbyte;

int dpsetting = 0;


int scanindexreg()
{
 char c;
 c=*srcptr;
 if(islower(c))c-=32;
 if (debug) fprintf(stderr,"DEBUG: scanindexreg: indexreg=%d, mode=%d, opsize=%d, error=%d, postbyte=%02X\n",c,mode,opsize,error,postbyte);
 switch(c) {
  case 'X':return 1;
  case 'Y':postbyte|=0x20;return 1;
  case 'U':postbyte|=0x40;return 1;
  case 'S':postbyte|=0x60;return 1;
  default: return 0;
 }
}

void
set3()
{
 if(mode<3)mode=3;
}

void
scanspecial()
{
 set3();
 skipspace();
 if(*srcptr=='-') {
  srcptr++;
  if(*srcptr=='-') {
   srcptr++;
   postbyte=0x83;
  } else postbyte=0x82;
  if(!scanindexreg())seterror(2);else srcptr++;
 } else {
  postbyte=0x80;
  if(!scanindexreg())seterror(2);else srcptr++;
  if(*srcptr=='+') {
   srcptr++;
   if(*srcptr=='+') {
    srcptr++;
    postbyte+=1;
   }
  } else postbyte+=4;
 }
}

void
scanindexed()
{
 set3();
 postbyte=0;
 if(scanindexreg()) {
   srcptr++;
   if(opsize==0) { 
                if(unknown||!certain)opsize=3;
                else if(operand>=-16&&operand<16&&mode==3)opsize=1;
                else if(operand>=-128&&operand<128)opsize=2;
                else opsize=3;
         }
   switch(opsize) {
   case 1:postbyte+=(operand&31);opsize=0;break;
   case 2:postbyte+=0x88;break;
   case 3:postbyte+=0x89;break;
   }
 } else { /*pc relative*/
  if(toupper(*srcptr)!='P')seterror(2);
  else {
    srcptr++;
    if(toupper(*srcptr)!='C')seterror(2);
    else {
     srcptr++;
     if(toupper(*srcptr)=='R')srcptr++;
    }
  }
  mode++;postbyte+=0x8c;
  if(opsize==1)opsize=2;
 }
}

#define RESTORE {srcptr=oldsrcptr;c=*srcptr;goto dodefault;}

void
scanoperands()
{
 char c,d,*oldsrcptr;
 unknown=0;
 opsize=0;
 certain=1;
 skipspace();
 c=*srcptr;
 mode=0;
 if(c=='[') {
  srcptr++;
  c=*srcptr;
  mode=5;
 }
 if (debug) fprintf(stderr,"DEBUG: scanoperands: c=%c (%02X)\n",c,c);
 switch(c) {
 case 'D': case 'd':
  oldsrcptr=srcptr;
  srcptr++;
  skipspace();
  if(*srcptr!=',')RESTORE else {
     postbyte=0x8b;
     srcptr++;
     if(!scanindexreg())RESTORE else {srcptr++;set3();}
  }
  break;
 case 'A': case 'a':
  oldsrcptr=srcptr;
  srcptr++;
  skipspace();
  if(*srcptr!=',')RESTORE else {
     postbyte=0x86;
     srcptr++;
     if(!scanindexreg())RESTORE else {srcptr++;set3();}
  }
  break;
 case 'B': case 'b':
  oldsrcptr=srcptr;
  srcptr++;
  skipspace();
  if(*srcptr!=',')RESTORE else {
     postbyte=0x85;
     srcptr++;
     if (debug) fprintf(stderr,"DEBUG: scanoperands: breg preindex: c=%c (%02X)\n",*srcptr,*srcptr);
     if(!scanindexreg())RESTORE else {srcptr++;set3();}
     if (debug) fprintf(stderr,"DEBUG: scanoperands: breg: postindex c=%c (%02X)\n",*srcptr,*srcptr);
  }
  break;
 case ',':
  srcptr++;
  scanspecial();
  break;
 case '#':
  if(mode==5)seterror(2);else mode=0;
  srcptr++;
  if (*srcptr=='"') { // ??
      operand = (srcptr[1]<<8) + srcptr[2] ;
      srcptr += 3;
      break;
  }
  operand=scanexpr(0);
  break;
 case '<':
  srcptr++;
  if(*srcptr=='<') {
   srcptr++;
   opsize=1;
  } else opsize=2;
  goto dodefault;
 case '>':
  srcptr++;
  opsize=3;
 default: dodefault:
  operand=scanexpr(0);
  skipspace();
  if(*srcptr==',') {
   srcptr++;
   scanindexed();
  } else {
   if(opsize==0) {
    if(unknown||dpsetting==-1||    // omit !certain
         ((((operand&0xff00)>>8))!=dpsetting))
    opsize=3; else opsize=2;
   }
   if(opsize==1)opsize=2;
   if(mode==5){
    postbyte=0x8f;
    opsize=3;
   } else mode=opsize-1;
  }
 }
 if (debug) fprintf(stderr,"DEBUG: scanoperands: mode=%d, error=%d, postbyte=%02X\n",mode,error,postbyte);
 if(mode>=5) {
  skipspace();
  postbyte|=0x10;
  if(*srcptr!=']')seterror(2);else srcptr++;
 }
 if(pass==2&&unknown)seterror(4);
}

unsigned char codebuf[128];
int codeptr; /* byte offset within instruction */
int suppress; /* 0=no suppress 1=until ENDIF 2=until ELSE 3=until ENDM */
int ifcount;  /* count of nested IFs within suppressed text */

unsigned char outmode; /* 0 is binary, 1 is s-records */

unsigned short hexaddr;
int hexcount;
unsigned char hexbuffer[16];
unsigned int chksum;

extern int os9crc(unsigned char c, int crcp);
int crc;

void
reset_crc() 
{
  crc = -1;
}


void
flushhex()
{
 int i;
 if(hexcount){
  fprintf(objfile,"S1%02X%04X",(hexcount+3)&0xff,hexaddr&0xffff);
  for(i=0;i<hexcount;i++)fprintf(objfile,"%02X",hexbuffer[i]);
  chksum+=(hexaddr&0xff)+((hexaddr>>8)&0xff)+hexcount+3;
  fprintf(objfile,"%02X\n",0xff-(chksum&0xff));
  hexaddr+=hexcount;
  hexcount=0;
  chksum=0;
 }
}

void
outhex(unsigned char x) 
{
 if(hexcount==16)flushhex();
 hexbuffer[hexcount++]=x;
 chksum+=x;
}

void
outbuffer()
{
 int i;
 for(i=0;i<codeptr;i++) {
   crc = os9crc(codebuf[i],crc);   
   if(!outmode)fputc(codebuf[i],objfile);else outhex(codebuf[i]);
 }
}

char *errormsg[]={"Error in expression",
                "Illegal addressing mode",
                "Undefined label",
                "Multiple definitions of label",
                "Relative branch out of range",
                "Missing label",
                "","","","","","","","","",
                "Illegal mnemonic"
               };

void
report()
{
 int i;
 fprintf(stderr,"File %s, line %d:%s\n",curname,lineno,srcline);
 for(i=0;i<16;i++) {
  if(error&1) {
   fprintf(stderr,"%s\n",errormsg[i]);
   if(pass==2&&listing)fprintf(listfile,"**** %s\n",errormsg[i]);
  }
  error>>=1;
 }
 error = 0;
 errors++;
}

void
outlist()
{
 int i;
 fprintf(listfile,"%04X: ",oldlc);
 for(i=0;i<codeptr&&i<MAXLISTBYTES;i++)
  fprintf(listfile,"%02X",codebuf[i]);
 for(;i<=MAXLISTBYTES;i++)
  fprintf(listfile,"  ");
 fprintf(listfile,"%s\n",srcline);
 while(i<codeptr) {
   fprintf(listfile,"%04X: ",oldlc + i);
   for(int j=0;i<codeptr&&j<MAXLISTBYTES;j++) {
    fprintf(listfile,"%02X",codebuf[i]); i++; 
   }
   fprintf(listfile,"\n"); 
 }
}

void
setlabel(struct symrecord * lp)
{
 while (prevlp) {
     struct symrecord *l = prevlp;
     prevlp = prevlp->next;
     l->next = 0;
     setlabel(l);
 }
 if(lp) {
  if(lp->cat!=13&&lp->cat!=6) {
   if(lp->cat!=2||lp->value!=loccounter)
     lp->value=loccounter; 
     if (pass==1) seterror(8);
  } else {
   lp->cat=2;
   lp->value=loccounter;
  }
 }
}

void
putbyte(unsigned char b)
{
 codebuf[codeptr++]=b;
}

void
putword(unsigned short w)
{
 codebuf[codeptr++]=w>>8;
 codebuf[codeptr++]=w&0x0ff;
}

void
doaddress() /* assemble the right addressing bytes for an instruction */
{
 int offs;
 switch(mode) {
 case 0: if(opsize==2)putbyte(operand);else putword(operand);break;
 case 1: putbyte(operand);break;
 case 2: putword(operand);break;
 case 3: case 5: putbyte(postbyte);
    switch(opsize) {
     case 2: putbyte(operand);break;
     case 3: putword(operand);
    }
    break;
 case 4: case 6: offs=(unsigned short)operand-loccounter-codeptr-2;
                if(offs<-128||offs>=128||opsize==3||unknown||!certain) {
                  if((!unknown)&&opsize==2&&(offs<-128||offs>=128) ) {
                     seterror(16); makelonger(glineno);
                  }
                  offs--;
                  opsize=3;
                  postbyte++;
                }
                putbyte(postbyte);
                if (debug) fprintf(stderr,"DEBUG: doaddress: mode=%d, opsize=%d, error=%d, postbyte=%02X, operand=%04X offs=%d\n",mode,opsize,error,postbyte,operand,offs);
                if(opsize==3)putword(offs);
                else putbyte(offs);
 }
}

void
onebyte(int co)
{
 putbyte(co);
}

void
twobyte(int co)
{
 putword(co);
}

void
oneimm(int co)
{
 scanoperands();
 if(mode>=3)
      seterror(2);
 putbyte(co);
 putbyte(operand);
}

void
lea(int co)
{
 putbyte(co);
 scanoperands();
 if(mode==0) seterror(2);
 if(mode<3) {
   opsize=3;
   postbyte=0x8f;
   mode=3;
 }
 if (debug) fprintf(stderr,"DEBUG: lea: mode=%d, opsize=%d, error=%d, postbyte=%02X, *src=%c\n",mode,opsize,error,postbyte,*srcptr);
 doaddress();
}

void
sbranch(int co)
{
 int offs;
 scanoperands();
 if(mode!=1&&mode!=2)seterror(2);
 offs=(unsigned short)operand-loccounter-2;
 if(!unknown&&(offs<-128||offs>=128)) {
     seterror(16);makelonger(glineno);
     if (co==0x20) {
         if(mode!=1&&mode!=2)seterror(2);
         putbyte(0x16);
         putword(operand-loccounter-3);
     } else {
         if(mode!=1&&mode!=2)seterror(2);
         putbyte(0x10);
         putbyte(co);
         putword(operand-loccounter-4);
     }
     return;
 }
 if(pass==2&&unknown)seterror(4);
 putbyte(co);
 putbyte(offs);
}

void
lbra(int co)
{
 scanoperands();
 if(mode!=1&&mode!=2)seterror(2);
 putbyte(co);
 putword(operand-loccounter-3);
}

void
lbranch(int co)
{
 scanoperands();
 if(mode!=1&&mode!=2)seterror(2);
 putword(co);
 putword(operand-loccounter-4);
}

void
arith(int co)
{
 scanoperands();
 switch(mode) {
 case 0:opsize=2;putbyte(co);break;
 case 1:putbyte(co+0x010);break;
 case 2:putbyte(co+0x030);break;
 default:putbyte(co+0x020);
 }
 doaddress();
}

void
darith(int co)
{
 scanoperands();
 switch(mode) {
 case 0:opsize=3;putbyte(co);break;
 case 1:putbyte(co+0x010);break;
 case 2:putbyte(co+0x030);break;
 default:putbyte(co+0x020);
 }
 doaddress();
}

void
d2arith(int co)
{
 scanoperands();
 switch(mode) {
 case 0:opsize=3;putword(co);break;
 case 1:putword(co+0x010);break;
 case 2:putword(co+0x030);break;
 default:putword(co+0x020);
 }
 doaddress();
}

void
oneaddr(int co)
{
 scanoperands();
 switch(mode) {
 case 0: seterror(2);break;
 case 1: putbyte(co);break;
 case 2: putbyte(co+0x70);break;
 default: putbyte(co+0x60);break;
 }
 doaddress();
}

void
tfrexg(int co)
{
 struct regrecord * p;
 putbyte(co);
 skipspace();
 scanname();
 if((p=findreg(namebuf))==0)seterror(2);
 else postbyte=(p->tfr)<<4;
 skipspace();
 if(*srcptr==',')srcptr++;else seterror(2);
 skipspace();
 scanname();
 if((p=findreg(namebuf))==0)seterror(2);
 else postbyte|=p->tfr;
 putbyte(postbyte);
}

void
pshpul(int co)
{
 struct regrecord *p;
 putbyte(co);
 postbyte=0;
 do {
  if(*srcptr==',')srcptr++;
  skipspace();
  scanname();
  if((p=findreg(namebuf))==0)seterror(2);
  else postbyte|=p->psh;
  skipspace();
 }while (*srcptr==',');
 putbyte(postbyte);
}

void
skipComma()
{
 while(*srcptr && *srcptr!='\n' && *srcptr!=',')srcptr++;
 if (*srcptr==',') {
   srcptr++;
 } else {
   seterror(1);  
 }
}

void os9begin()
{
 generate();
 os9=1;   // contiguous code generation ( seprate rmb and code )
 oldlc = loccounter = rmbcounter = rmbmode = 0;
 reset_crc();
 putword(0x87cd);
 putword(scanexpr(0)-loccounter);  // module size
 if(unknown&&pass==2)seterror(4);
 skipComma();
 putword(scanexpr(0)-loccounter);  // offset to module name
 if(unknown&&pass==2)seterror(4);
 skipComma();
 putbyte(scanexpr(0));             // type / language
 if(unknown&&pass==2)seterror(4);
 skipComma();
 putbyte(scanexpr(0));             // attribute
 if(unknown&&pass==2)seterror(4);
 int parity=0;
 for(int i=0; i< 8; i++) parity^=codebuf[i];
 putbyte(parity^0xff);              // header parity
 skipspace();
 while (*srcptr==',') {             // there are some more
   srcptr++;
   putword(scanexpr(0));   
   if(unknown&&pass==2)seterror(4);
   skipspace();
 }
 prevloc = codeptr;
 rmbmode = 1;                   // next org works on rmb
 rmbcounter=0;
 loccounter = 0x10000-codeptr;  // should start at 0
}

void os9end()
{
 crc = crc ^ 0xffffff;

 putbyte((crc>>16)&0xff);
 putbyte((crc>>8)&0xff);
 putbyte(crc&0xff);
 os9 = 0;
}


void
pseudoop(int co,struct symrecord * lp)
{
 int i;
 char c;
 char *fname;
 int locsave;

 switch(co) {
 case 0:/* RMB */
        //   in OS9 mode, this generates no data
        //   loccounter will be reset after any code to the current code generation
        if (os9 && !rmbmode) {
            prevloc = loccounter;
            oldlc = loccounter  = rmbcounter;
            rmbmode = 1;
        }
        setlabel(lp);
        oldlc = loccounter;
        operand=scanexpr(0);
        if(unknown)seterror(4);
        loccounter+=operand;
        if(generating&&pass==2) {
           if(!outmode && !os9 ) {
               for(i=0;i<operand;i++) { fputc(0,objfile); } 
           } else 
               flushhex();  
        }   
        hexaddr=loccounter;
        break;
 case 5:/* EQU */
        operand=scanexpr(0);
        if(!lp)seterror(32);
        else {
         if(lp->cat==13||lp->cat==6||
            (lp->value==(unsigned short)operand&&pass==2)) {
          if(exprcat==2)lp->cat=2;
          else lp->cat=0;
          lp->value=oldlc=operand;
         } else // else seterror(8);
          lp->value=oldlc=operand;
        }
        break;
 case 7:/* FCB */
        generate();
        setlabel(lp);
        skipspace();
fcb:
        do {
        if(*srcptr==',')srcptr++;
        if(*srcptr=='\"') {
         srcptr++;
         while(*srcptr!='\"'&&*srcptr)
          putbyte(*srcptr++);
         if(*srcptr=='\"')srcptr++;
        } else {
          putbyte(scanexpr(0));
          if(unknown&&pass==2)seterror(4);
        }
        } while(*srcptr==',');
        break;
 case 8:/* FCC */
        generate();
        setlabel(lp);
        skipspace();
        c=*srcptr++;
        while(*srcptr!=c&&*srcptr)
          putbyte(*srcptr++);
        if(*srcptr==c)srcptr++;
        if(*srcptr==',') { srcptr++; goto fcb; }
        break;
 case 9:/* FDB */
        generate();
        setlabel(lp);
        do {
         if(*srcptr==',')srcptr++;
         skipspace();
         putword(scanexpr(0));
         if(unknown&&pass==2)seterror(4);
         skipspace();
        } while(*srcptr==',');
        break;
 case 23 :/* FCS */
        generate();
        setlabel(lp);
        skipspace();
        int sep = *srcptr;
        if(sep=='\"' || sep=='/' || sep=='\'') {
         srcptr++;
         while(*srcptr!=sep&&*srcptr)
          putbyte(*srcptr++);
         if(*srcptr==sep)srcptr++; 
         codebuf[codeptr-1] |= 0x80;  // os9 string termination
        }
        break;
 case 1: /* ELSE */
        suppress=1;
        break;
 case 21: /* IFP1 */
        if(pass==2)suppress=2;
        break;                
 case 29: /* IFGT */
        operand=scanexpr(0);
        if(operand<=0)suppress=2;
        break;                
 case 31: /* IFLT */
        operand=scanexpr(0);
        if(operand>=0)suppress=2;
        break;                
 case 30: /* IFEQ */
        operand=scanexpr(0);
        if(operand!=0)suppress=2;
        break;                
 case 28: /* IFNE */
 case 10: /* IF */
        operand=scanexpr(0);
        if(operand==0)suppress=2;
        break;                
 case 33: /* IFNDEF */
        operand=scanexpr(0);
        if(!unknown)suppress=2;
        break;                
 case 12: /* ORG */
         operand=scanexpr(0);
         if(unknown)seterror(4);
         if(generating&&pass==2&&!outmode&&!os9) {
           for(i=0;i<(unsigned short)operand-loccounter;i++)
                fputc(0,objfile); 
         } else flushhex();
         loccounter=operand;
         hexaddr=loccounter;
         break;
  case 14: /* SETDP */
         operand=scanexpr(0);
         if(unknown)seterror(4);
         if(!(operand&255))operand=(unsigned short)operand>>8;
         if((unsigned)operand>255)operand=-1;
         dpsetting=operand;              
         break;
  case 15: /* SET */
        operand=scanexpr(0);
        if(!lp)seterror(32);
        else {
         if(lp->cat&1||lp->cat==6) {
          if(exprcat==2)lp->cat=3;
          else lp->cat=1;
          lp->value=oldlc=operand;
         } else // else seterror(8);
          lp->value=oldlc=operand;
        }
        break;
   case 2: /* END */
        terminate=1;
        break;     
   case 27: /* USE */     
   case 16: /* INCLUDE */     
        skipspace();
        if(*srcptr=='"')srcptr++;
        i = 0;
        for(i=0; !(srcptr[i]==0||srcptr[i]=='"'); i++);
        int len = i;
        fname = calloc(1,len);
        for(i=0;i<len;i++) {
          if(*srcptr==0||*srcptr=='"')break;
          fname[i]=*srcptr++;
        }
        fname[i]=0;
        processfile(fname);
        codeptr=0;
        srcline[0]=0;
        break; 
   case 24: /* MOD */     
        oldlc = loccounter = 0;
        setlabel(lp);
        os9begin();
        break; 
   case 25: /* EMOD */     
        os9end();
        break; 
   case 32: /* OS9 */     
        generate();
        setlabel(lp);
        putword(0x103f); // SWI2
        putbyte(scanexpr(0));
        if(unknown&&pass==2)seterror(4);
        break; 
   case 18: /* TTL */     
        break;
   case 19: /* OPT */     
   case 26: /* NAM */     
   case 20: /* PAG */     
   case 3:  /* ENDIF/ENDC */     
        break; 
 }
}


void
processline()
{
 struct symrecord * lp;
 struct oprecord * op;
 int co;
 char c;
 srcptr=srcline;
 oldlc=loccounter;
 //  error=0;
 unknown=0;certain=1;
 lp=0;
 codeptr=0;
 if(*srcptr=='_'||isalnum(*srcptr)) {
  scanname();lp=findsym(namebuf);
  if(*srcptr==':') srcptr++;
  if(lp && pass==2) {
    oldlc = lp->value;
  }
 }
 skipspace();
 if(isalnum(*srcptr)) {
  scanname();
  op=findop(namebuf);
  if(op) {
   if(op->cat!=13){
     generate();
     setlabel(lp);
   }
   co=op->code;
   switch(op->cat) {
   case 0:onebyte(co);break;
   case 1:twobyte(co);break;
   case 2:oneimm(co);break;
   case 3:lea(co);break;
   case 4:sbranch(co);break;
   case 5:lbranch(co);break;
   case 6:lbra(co);break;
   case 7:arith(co);break;
   case 8:darith(co);break;
   case 9:d2arith(co);break;
   case 10:oneaddr(co);break;
   case 11:tfrexg(co);break;
   case 12:pshpul(co);break;
   case 13:pseudoop(co,lp);
   }
   c=*srcptr;
   if (debug) fprintf(stderr,"DEBUG: processline: mode=%d, opsize=%d, error=%d, postbyte=%02X c=%c\n",mode,opsize,error,postbyte,c);
   if(c!=' '&&*(srcptr-1)!=' '&&c!=0&&c!=';')seterror(2);
  }
  else seterror(0x8000);
 } else {
     if (lp) {
         lp->next = prevlp;
         prevlp = lp;    // os9 mode label can be data or code 
     }
 }
 if(pass==2) {
  outbuffer();
  if(listing)outlist();
 }
 if(error)report();
 loccounter+=codeptr;
}

void
suppressline()
{
 struct oprecord * op;
 srcptr=srcline;
 oldlc=loccounter;
 struct symrecord * lp = 0;
 codeptr=0;
 if(isalnum(*srcptr)) {
  scanname();lp=findsym(namebuf);
  if (lp) oldlc = lp->value;
  if(*srcptr==':')srcptr++;
 }
 skipspace();
 scanname();op=findop(namebuf); 
 if(op && op->cat==13) {
  if(op->code==10||op->code==13||op->code==29||op->code==28||op->code==21||op->code==30||op->code==31||op->code==33) ifcount++;
  else if(op->code==3) {
   if(ifcount>0)ifcount--;else if(suppress==1|suppress==2)suppress=0;
  } else if(op->code==1) {
   if(ifcount==0 && suppress==2)suppress=0;
  }
 }  
 if(pass==2&&listing)outlist(); } 

void
usage(char*nm)
{
  fprintf(stderr,"Usage: %s [-o objname] [-l listname] [-s srecord-file] srcname\n",nm);
  exit(2);
}

char *
strconcat(char *s,int spos,char *d)
{
  int slen = strlen(s);
  int dlen = strlen(d);
  if ( spos == 0) spos = slen;
  char *out = calloc(1,spos+dlen+1);
  int i = 0;
  for(; i< spos; i++ ) out[i] = s[i];
  for(; i< spos+dlen+1; i++ ) out[i] = *d++;
  return out;
} 


void
getoptions(int c,char*v[])
{
 int i=1;
 if(c==1)usage(v[0]);
 while(v[i]) {
     if(strcmp(v[i],"-d")==0) {
       debug=1;
       i++;
     } else if(strcmp(v[i],"-o")==0) {
       objname = v[i+1];
       i+=2;
     } else if(strcmp(v[i],"-s")==0) {
       objname=v[i+1];
       outmode=1;
       i+=2;
     } else if(strcmp(v[i],"-l")==0) {
       listname=v[i+1];
       i+=2;
     } else if(strcmp(v[i],"-D")==0) {
       struct symrecord * p;
       p=findsym(v[i+1]);
       p->value = 1;
       p->cat = 0;
       i+=2;
     } else if(strcmp(v[i],"-I")==0) {
       struct incl *j = (struct incl *)malloc(sizeof(struct incl));
       j->name = v[i+1];
       j->next  = 0;
       if (!incls) incls = j;
       else { 
           struct incl *k=incls ;
           for(; k->next ; k = k->next ) ;
           k->next = j;
       }
       i+=2;
     } else if(*v[i]=='-') {
         usage(v[0]);
     } else {
        if (srcname) usage(v[0]);
        srcname=v[i];
        i++;
     }
 }
 if(objname==0) {
   for(i=0;srcname[i]!='.' && srcname[i]!=0 ;i++) ;
   objname = strconcat(srcname,i,".b");
 }
 listing=(listname!=0);
}

void
expandline()
{
 int i=0,j=0,k,j1;
 for(i=0;i<128&&j<128;i++)
 {
  if(inpline[i]=='\n') {
    srcline[j]=0;break;
  }
  if(inpline[i]=='\t') {
    j1=j;
    for(k=0;k<8-j1%8 && j<128;k++)srcline[j++]=' ';
  }else srcline[j++]=inpline[i];
 }
 srcline[127]=0;
}


void
processfile(char *name)
{
 char *oldname;
 int oldno;
 FILE *srcfile;
 oldname=curname;
 curname=name;
 oldno=lineno;
 lineno=0;
 if((srcfile=fopen(name,"r"))==0) {
   int i = 0;
   if (oldname) {
       i = strlen(oldname);
       while(i>0 && oldname[i]!='/') i--;
   }
   if (i>0) {
       char *next = strconcat(oldname,i+1,name);
       if((srcfile=fopen(next,"r"))!=0) {
          curname = next;
       }
   } 
   if (!srcfile) {
     for( struct incl *d = incls; d ; d = d->next) {
          char *next = strconcat(d->name,0,name);
          if((srcfile=fopen(next,"r"))!=0) {
             curname = next;
             break;
          }
     }
   }
 }
 if (!srcfile) {
     fprintf(stderr,"Cannot open source file %s\n",name);
     exit(4);
 }
 while(!terminate&&fgets(inpline,128,srcfile)) {
   expandline();
   lineno++; glineno++;
   srcptr=srcline;
   if(suppress)
       suppressline(); 
   else 
       processline();
 }
 setlabel(0);   // process prevlp
 fclose(srcfile);
 if(suppress) {
   fprintf(stderr,"improperly nested IF statements in %s",curname);
   errors++;
   suppress=0;
 }
 lineno=oldno;
 curname=oldname;
}

int
main(int argc,char *argv[])
{
 char c;
 getoptions(argc,argv);
 pass=1;
 errors=0;
 generating=0;
 terminate=0;
 processfile(srcname);
 if(errors) {
  fprintf(stderr,"%d Pass 1 Errors, Continue?",errors);
  c=getchar();
  if(c=='n'||c=='N') exit(3);
 }
 do {
 pass=2;
 prevloc = 0;
 loccounter=0;
 rmbcounter=0;
 errors=0;
 generating=0;
 terminate=0;
 glineno=0;
 if(listing&&((listfile=fopen(listname,"w"))==0)) {
  fprintf(stderr,"Cannot open list file");
  exit(4);
 }
 if((objfile=fopen(objname,outmode?"w":"wb"))==0) {
  fprintf(stderr,"Cannot write object file\n");
  exit(4);
 }
 processfile(srcname);
 fprintf(stderr,"%d Pass 2 errors.\n",errors);
 if(listing) {
  fprintf(listfile,"%d Pass 2 errors.\n",errors);
  outsymtable();
  fclose(listfile);
 }
 if(outmode){
  flushhex();
  fprintf(objfile,"S9030000FC\n");
 } 
 fclose(objfile);
 } while (longer());
 return 0;
}

