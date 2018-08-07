#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>


/*************************************************************************** 
  Originally posted to comp.sys.m6809 by Didier Derny (didier@aida.remcomp.fr)

  Minor hacks by Alan DeKok

 Fixed: D_Indexed addressing used prog[2] and prog[3] when it meant
        prog[pc+2] and prog[pc+3]:  Would produce flawed disassemblies!

        changed addresses in D_Indexed to be all hex.
	added 2 instances of 'extrabyte' in D_Indexed: would not skip them..
        Added PC offsets to D_Indexed ,PCR formats
	added SWI2 print out as OS9

  To do:

 handle command-line options properly...

 Fix handling of illegal opcodes so it doesn't skip a byte
   i.e. $87 is a skip 2

 Move defines to another file

 Add 6309 support
   also add 6309 native-mode support, and listing of clock cycles for opcodes.

 Add OS-9 support

 add proper label-disassembly.  i.e. 2-pass.

****************************************************************************/

// extern int errno;
// extern char *sys_errlist[];

static unsigned char prog0[65536];
unsigned char *prog = prog0;

FILE *fp;

typedef struct {
	char *name;
	int  clock;
	int  bytes;
	int  (*display)();
	int  (*execute)();
} Opcode;

typedef struct {
  int address;
  int length;
  int width;
} String;

int    D_Illegal(Opcode *, int, int, char *);
int    D_Direct(Opcode *, int, int, char *);
int    D_Page10(Opcode *, int, int, char *);
int    D_Page11(Opcode *, int, int, char *);
int    D_Immediat(Opcode *, int, int, char *);
int    D_ImmediatL(Opcode *, int, int, char *);
int    D_Inherent(Opcode *, int, int, char *);
int    D_Indexed(Opcode *, int, int, char *);
int    D_Extended(Opcode *, int, int, char *);
int    D_Relative(Opcode *, int, int, char *);
int    D_RelativeL(Opcode *, int, int, char *);
int    D_Register0(Opcode *, int, int, char *);
int    D_Register1(Opcode *, int, int, char *);
int    D_Register2(Opcode *, int, int, char *);
int    D_Page10(Opcode *, int, int, char *);
int    D_Page11(Opcode *, int, int, char *);
int    D_OS9(Opcode *, int, int, char *);
char  *IndexRegister(int);

String stringtable[] = {
	{ 0xc321, 16,  16 },
	{ 0xc395, 258, 16 },
	{ 0xeb15,  50, 16 },
	{ 0xee6f, 128, 16 },
	{ 0xfdf4, 492, 16 },
	{ 0xfff0, 16,   2 },
};

int adoffset = 0;
int laststring = 6;

Opcode optable[] = {
  { "NEG  ",  6,  2, D_Direct,    NULL },	 /* 0x00 */
  { "?????",  0,  1, D_Illegal,   NULL },	 /* 0x01 */
  { "?????",  0,  1,	D_Illegal,   NULL },	 /* 0x02 */
  { "COM  ",  6,  2,	D_Direct,    NULL },	 /* 0x03 */
  { "LSR  ",  6,  2,	D_Direct,    NULL },	 /* 0x04 */
  { "?????",  0,  1,	D_Illegal,   NULL },	 /* 0x05 */
  { "ROR  ",  6,  2,	D_Direct,    NULL },	 /* 0x06 */
  { "ASR  ",  6,  2,	D_Direct,    NULL },	 /* 0x07 */
  { "LSL  ",  6,  2,	D_Direct,    NULL },	 /* 0x08 */
  { "ROL  ",  6,  2,	D_Direct,    NULL },	 /* 0x09 */
  { "DEC  ",  6,  2,	D_Direct,    NULL },	 /* 0x0a */
  { "?????",  0,  1,	D_Illegal,   NULL },	 /* 0x0b */
  { "INC  ",  6,  2,	D_Direct,    NULL },	 /* 0x0c */
  { "TST  ",  6,  2,	D_Direct,    NULL },	 /* 0x0d */
  { "JMP  ",  3,  2,	D_Direct,    NULL },	 /* 0x0e */
  { "CLR  ",  6,  2,	D_Direct,    NULL },     /* 0x0f */

  { "",       0,  1, D_Page10,    NULL },	 /* 0x10 */
  { "",       0,  1, D_Page11,    NULL },	 /* 0x11 */
  { "NOP  ",  2,  1, D_Inherent,  NULL },	 /* 0x12 */
  { "SYNC ",  4,  1, D_Inherent,  NULL },       /* 0x13 */
  { "?????",  0,  1, D_Illegal,   NULL },	 /* 0x14 */
  { "?????",  0,  1, D_Illegal,   NULL },  /* 0x15 */
  { "LBRA ",  5,  3, D_RelativeL, NULL },	 /* 0x16 */
  { "LBSR ",  9,  3, D_RelativeL, NULL },	 /* 0x17 */
  { "?????",  0,  1, D_Illegal,   NULL },	 /* 0x18 */
  { "DAA  ",  2,  1, D_Inherent,  NULL },	 /* 0x19 */
  { "ORCC ",  3,  2, D_Immediat,  NULL },	 /* 0x1a */
  { "?????",  0,  1, D_Illegal,   NULL },	 /* 0x1b */
  { "ANDCC",  3,  2, D_Immediat,  NULL },  /* 0x1c */
  { "SEX  ",  2,  1, D_Inherent,  NULL },	 /* 0x1d */
  { "EXG  ",  8,  2, D_Register0, NULL },	 /* 0x1e */
  { "TFR  ",  6,  2,   D_Register0, NULL },	 /* 0x1f */

  { "BRA  ",  3,  2,   D_Relative,  NULL },	 /* 0x20 */
  { "BRN  ",  3,  2,   D_Relative,  NULL },	 /* 0x21 */
  { "BHI  ",  3,  2,   D_Relative,  NULL },	 /* 0x22 */
  { "BLS  ",  3,  2,   D_Relative,  NULL },	 /* 0x23 */
  { "BCC  ",  3,  2,   D_Relative,  NULL },	 /* 0x24 */
  { "BCS  ",  3,  2,   D_Relative,  NULL },	 /* 0x25 */
  { "BNE  ",  3,  2,   D_Relative,  NULL },	 /* 0x26 */
  { "BEQ  ",  3,  2,   D_Relative,  NULL },	 /* 0x27 */
  { "BVC  ",  3,  2,   D_Relative,  NULL },	 /* 0x28 */
  { "BVS  ",  3,  2,   D_Relative,  NULL },	 /* 0x29 */
  { "BPL  ",  3,  2,   D_Relative,  NULL },	 /* 0x2a */
  { "BMI  ",  3,  2,   D_Relative,  NULL },	 /* 0x2b */
  { "BGE  ",  3,  2,   D_Relative,  NULL },	 /* 0x2c */
  { "BLT  ",  3,  2,   D_Relative,  NULL },	 /* 0x2d */
  { "BGT  ",  3,  2,   D_Relative,  NULL },	 /* 0x2e */
  { "BLE  ",  3,  2,   D_Relative,  NULL },	 /* 0x2f */

  { "LEAX ",  4,  2,   D_Indexed,   NULL },  /* 0x30 */
  { "LEAY ",  4,  2,   D_Indexed,   NULL },  /* 0x31 */
  { "LEAS ",  4,  2,   D_Indexed,   NULL },  /* 0x32 */
  { "LEAU ",  4,  2,   D_Indexed,   NULL },  /* 0x33 */
  { "PSHS ",  5,  2,   D_Register1, NULL },  /* 0x34 */
  { "PULS ",  5,  2,   D_Register1, NULL },  /* 0x35 */
  { "PSHU ",  5,  2,   D_Register2, NULL },  /* 0x36 */
  { "PULU ",  5,  2,   D_Register2, NULL },  /* 0x37 */
  { "?????",  0,  1,   D_Illegal,   NULL },  /* 0x38 */
  { "RTS  ",  5,  1,   D_Inherent,  NULL },  /* 0x39 */
  { "ABX  ",  3,  1,   D_Inherent,  NULL },  /* 0x3a */
  { "RTI  ",  6,  1,   D_Inherent,  NULL },  /* 0x3b */
  { "CWAI ",  20, 2,   D_Inherent,  NULL },  /* 0x3c */
  { "MUL  ",  11, 1,   D_Inherent,  NULL },  /* 0x3d */
  { "?????",  0,  1,   D_Illegal,   NULL },  /* 0x3e */
  { "SWI  ",  19, 1,   D_Inherent,  NULL },  /* 0x3f */

  { "NEGA ",   2,  1,   D_Inherent,  NULL },	 /* 0x40 */
  {  "?????",  0, 1,   D_Illegal,   NULL },	 /* 0x41 */
  {  "?????",  0, 1,   D_Illegal,   NULL },	 /* 0x42 */
  {  "COMA ",  2, 1,   D_Inherent,  NULL },	 /* 0x43 */
  {  "LSRA ",  2, 1,   D_Inherent,  NULL },	 /* 0x44 */
  {  "?????",  0, 1,   D_Illegal,   NULL },	 /* 0x45 */
  {  "RORA ",  2, 1,   D_Inherent,  NULL },	 /* 0x46 */
  {  "ASRA ",  2, 1,   D_Inherent,  NULL },	 /* 0x47 */
  {  "LSLA ",  2, 1,   D_Inherent,  NULL },	 /* 0x48 */
  {  "ROLA ",  2, 1,   D_Inherent,  NULL },	 /* 0x49 */
  {  "DECA ",  2, 1,   D_Inherent,  NULL },	 /* 0x4a */
  {  "?????",  0, 1,   D_Illegal,   NULL },	 /* 0x4b */
  {  "INCA ",  2, 1,   D_Inherent,  NULL },	 /* 0x4c */
  {  "TSTA ",  2, 1,   D_Inherent,  NULL },  /* 0x4d */
  {  "?????",  0, 1,   D_Illegal,   NULL },	 /* 0x4e */
  {  "CLRA ",  2, 1,   D_Inherent,  NULL },	 /* 0x4f */

  {  "NEGB ",  2, 1,   D_Inherent,  NULL },	 /* 0x50 */
  { "?????",   0, 1,   D_Illegal,   NULL },	 /* 0x51 */
  { "?????",   0, 1,   D_Illegal,   NULL },	 /* 0x52 */
  { "COMB ",   2, 1,   D_Inherent,  NULL },	 /* 0x53 */
  { "LSRB ",   2, 1,   D_Inherent,  NULL },	 /* 0x54 */
  { "?????",   0, 1,   D_Illegal,   NULL },	 /* 0x55 */
  { "RORB ",   2, 1,   D_Inherent,  NULL },	 /* 0x56 */
  { "ASRB ",   2, 1,   D_Inherent,  NULL },	 /* 0x57 */
  { "LSLB ",   2, 1,   D_Inherent,  NULL },	 /* 0x58 */
  { "ROLB ",   2, 1,   D_Inherent,  NULL },	 /* 0x59 */
  { "DECB ",   2, 1,   D_Inherent,  NULL },	 /* 0x5a */
  { "?????",   0, 1,   D_Illegal,   NULL },	 /* 0x5b */
  { "INCB ",   2, 1,   D_Inherent,  NULL },	 /* 0x5c */
  { "TSTB ",   2, 1,   D_Inherent,  NULL },	 /* 0x5d */
  { "?????",   0, 1,   D_Illegal,   NULL },	 /* 0x5e */
  { "CLRB ",   2, 1,   D_Inherent,  NULL },	 /* 0x5f */

  { "NEG  ",   6, 2,   D_Indexed,   NULL },	 /* 0x60 */
  { "?????",   0, 2,   D_Illegal,   NULL },	 /* 0x61 */
  { "?????",   0, 2,   D_Illegal,   NULL },	 /* 0x62 */
  { "COM  ",   6, 2,   D_Indexed,   NULL },	 /* 0x63 */
  { "LSR  ",   6, 2,   D_Indexed,   NULL },	 /* 0x64 */
  { "?????",   0, 2,   D_Indexed,   NULL },	 /* 0x65 */
  { "ROR  ",   6, 2,   D_Indexed,   NULL },	 /* 0x66 */
  { "ASR  ",   6, 2,   D_Indexed,   NULL },	 /* 0x67 */
  { "LSL  ",   6, 2,   D_Indexed,   NULL },	 /* 0x68 */
  { "ROL  ",   6, 2,   D_Indexed,   NULL },	 /* 0x69 */
  { "DEC  ",   6, 2,   D_Indexed,   NULL },	 /* 0x6a */
  { "?????",   0, 2,   D_Illegal,   NULL },	 /* 0x6b */
  { "INC  ",   6, 2,   D_Indexed,   NULL },	 /* 0x6c */
  { "TST  ",   6, 2,   D_Indexed,   NULL },	 /* 0x6d */
  { "JMP  ",   3, 2,   D_Indexed,   NULL },	 /* 0x6e */
  { "CLR  ",   6, 2,   D_Indexed,   NULL },	 /* 0x6f */

  { "NEG  ",   7, 3,   D_Extended,  NULL },      /* 0x70 */
  { "?????",   0, 1,   D_Illegal,   NULL },      /* 0x71 */
  { "?????",   0, 1,   D_Illegal,   NULL },	 /* 0x72 */
  { "COM  ",   7, 3,   D_Extended,  NULL },	 /* 0x73 */
  { "LSR  ",   7, 3,   D_Extended,  NULL },	 /* 0x74 */
  { "?????",   0, 1,   D_Illegal,   NULL },	 /* 0x75 */
  { "ROR  ",   7, 3,   D_Extended,  NULL },	 /* 0x76 */
  { "ASR  ",   7, 3,   D_Extended,  NULL },	 /* 0x77 */
  { "LSL  ",   7, 3,   D_Extended,  NULL },	 /* 0x78 */
  { "ROL  ",   7, 3,   D_Extended,  NULL },	 /* 0x79 */
  { "DEC  ",   7, 3,   D_Extended,  NULL },	 /* 0x7a */
  { "?????",   0, 1,   D_Illegal,   NULL },	 /* 0x7b */
  { "INC  ",   7, 3,   D_Extended,  NULL },	 /* 0x7c */
  { "TST  ",   7, 3,   D_Extended,  NULL },	 /* 0x7d */
  { "JMP  ",   4, 3,   D_Extended,  NULL },	 /* 0x7e */
  { "CLR  ",   7, 3,   D_Extended,  NULL },	 /* 0x7f */

  { "SUBA ",  2,  2,   D_Immediat,  NULL },	/* 0x80 */
  { "CMPA ",  2,  2,   D_Immediat,  NULL },	/* 0x81 */
  { "SBCA ",  2,  2,   D_Immediat,  NULL },	/* 0x82 */
  { "SUBD ",  4,  3,   D_ImmediatL, NULL },	/* 0x83 */
  { "ANDA ",  2,  2,   D_Immediat,  NULL },	/* 0x84 */
  { "BITA ",  2,  2,   D_Immediat,  NULL },	/* 0x85 */
  { "LDA  ",  2,  2,   D_Immediat,  NULL },	/* 0x86 */
  { "?????",  0,  2,   D_Illegal,   NULL },	/* 0x87 */
  { "EORA ",  2,  2,   D_Immediat,  NULL },	/* 0x88 */
  { "ADCA ",  2,  2,   D_Immediat,  NULL },	/* 0x89 */
  { "ORA  ",  2,  2,   D_Immediat,  NULL },	/* 0x8a */
  { "ADDA ",  2,  2,   D_Immediat,  NULL },	/* 0x8b */
  { "CMPX ",  4,  3,   D_ImmediatL, NULL },	/* 0x8c */
  { "BSR  ",  7,  2,   D_Relative,  NULL },	/* 0x8d */
  { "LDX  ",  3,  3,   D_ImmediatL, NULL },	/* 0x8e */
  { "?????",  0,  2,   D_Illegal,   NULL },	/* 0x8f */

  { "SUBA ",  4,  2,   D_Direct,    NULL },	/* 0x90 */
  { "CMPA ",  4,  2,   D_Direct,    NULL },	/* 0x91 */
  { "SBCA ",  4,  2,   D_Direct,    NULL },	/* 0x92 */
  { "SUBD ",  6,  2,   D_Direct,    NULL },	/* 0x93 */
  { "ANDA ",  4,  2,   D_Direct,    NULL },	/* 0x94 */
  { "BITA ",  4,  2,   D_Direct,    NULL },	/* 0x95 */
  { "LDA  ",  4,  2,   D_Direct,    NULL },	/* 0x96 */
  { "STA  ",  4,  2,   D_Direct,    NULL },	/* 0x97 */
  { "EORA ",  4,  2,   D_Direct,    NULL },	/* 0x98 */
  { "ADCA ",  4,  2,   D_Direct,    NULL },	/* 0x99 */
  { "ORA  ",  4,  2,   D_Direct,    NULL },	/* 0x9a */
  { "ADDA ",  4,  2,   D_Direct,    NULL },	/* 0x9b */
  { "CMPX ",  6,  2,   D_Direct,    NULL },	/* 0x9c */
  { "JSR  ",  7,  2,   D_Direct,    NULL },	/* 0x9d */
  { "LDX  ",  5,  2,   D_Direct,    NULL },	/* 0x9e */
  { "STX  ",  5,  2,   D_Direct,    NULL },	/* 0x9f */

  { "SUBA ",  4,  2,   D_Indexed,   NULL },	/* 0xa0 */
  { "CMPA ",  4,  2,   D_Indexed,   NULL },	/* 0xa1 */
  { "SBCA ",  4,  2,   D_Indexed,   NULL },	/* 0xa2 */
  { "SUBD ",  6,  2,   D_Indexed,   NULL },	/* 0xa3 */
  { "ANDA ",  4,  2,   D_Indexed,   NULL },	/* 0xa4 */
  { "BITA ",  4,  2,   D_Indexed,   NULL },	/* 0xa5 */
  { "LDA  ",  4,  2,   D_Indexed,   NULL },	/* 0xa6 */
  { "STA  ",  4,  2,   D_Indexed,   NULL },	/* 0xa7 */
  { "EORA ",  4,  2,   D_Indexed,   NULL },	/* 0xa8 */
  { "ADCA ",  4,  2,   D_Indexed,   NULL },	/* 0xa9 */
  { "ORA  ",  4,  2,   D_Indexed,   NULL },	/* 0xaa */
  { "ADDA ",  4,  2,   D_Indexed,   NULL },	/* 0xab */
  { "CMPX ",  6,  2,   D_Indexed,   NULL },	/* 0xac */
  { "JSR  ",  7,  2,   D_Indexed,   NULL },	/* 0xad */
  { "LDX  ",  5,  2,   D_Indexed,   NULL },	/* 0xae */
  { "STX  ",  5,  2,   D_Indexed,   NULL },	/* 0xaf */

  { "SUBA ",  5,  3,   D_Extended,  NULL },	/* 0xb0 */
  { "CMPA ",  5,  3,   D_Extended,  NULL },	/* 0xb1 */
  { "SBCA ",  5,  3,   D_Extended,  NULL },	/* 0xb2 */
  { "SUBD ",  7,  3,   D_Extended,  NULL },	/* 0xb3 */
  { "ANDA ",  5,  3,   D_Extended,  NULL },	/* 0xb4 */
  { "BITA ",  5,  3,   D_Extended,  NULL },	/* 0xb5 */
  { "LDA  ",  5,  3,   D_Extended,  NULL },	/* 0xb6 */
  { "STA  ",  5,  3,   D_Extended,  NULL },	/* 0xb7 */
  { "EORA ",  5,  3,   D_Extended,  NULL },	/* 0xb8 */
  { "ADCA ",  5,  3,   D_Extended,  NULL },	/* 0xb9 */
  { "ORA  ",  5,  3,   D_Extended,  NULL },	/* 0xba */
  { "ADDA ",  5,  3,   D_Extended,  NULL },	/* 0xbb */
  { "CMPX ",  7,  3,   D_Extended,  NULL },	/* 0xbc */
  { "JSR  ",  8,  3,   D_Extended,  NULL },	/* 0xbd */
  { "LDX  ",  6,  3,   D_Extended,  NULL },	/* 0xbe */
  { "STX  ",  6,  3,   D_Extended,  NULL },	/* 0xbf */

  { "SUBB ",  2,  2,   D_Immediat,  NULL },	/* 0xc0 */
  { "CMPB ",  2,  2,   D_Immediat,  NULL },	/* 0xc1 */
  { "SBCB ",  2,  2,   D_Immediat,  NULL },	/* 0xc2 */
  { "ADDD ",  4,  3,   D_ImmediatL, NULL },	/* 0xc3 */
  { "ANDB ",  2,  2,   D_Immediat,  NULL },	/* 0xc4 */
  { "BITB ",  2,  2,   D_Immediat,  NULL },	/* 0xc5 */
  { "LDB  ",  2,  2,   D_Immediat,  NULL },	/* 0xc6 */
  { "?????",  0,  1,   D_Illegal,   NULL },	/* 0xc7 */
  { "EORB ",  2,  2,   D_Immediat,  NULL },	/* 0xc8 */
  { "ADCB ",  2,  2,   D_Immediat,  NULL },	/* 0xc9 */
  { "ORB  ",  2,  2,   D_Immediat,  NULL },	/* 0xca */
  { "ADDB ",  2,  2,   D_Immediat,  NULL },	/* 0xcb */
  { "LDD  ",  3,  3,   D_ImmediatL, NULL },	/* 0xcc */
  { "?????",  0,  1,   D_Illegal,   NULL },	/* 0xcd */
  { "LDU  ",  3,  3,   D_ImmediatL, NULL },	/* 0xce */
  { "?????",  0,  1,   D_Illegal,   NULL },	/* 0xcf */

  { "SUBB ",  4,  2,   D_Direct,    NULL },	/* 0xd0 */
  { "CMPB ",  4,  2,   D_Direct,    NULL },	/* 0xd1 */
  { "SBCB ",  4,  2,   D_Direct,    NULL },	/* 0xd2 */
  { "ADDD ",  6,  2,   D_Direct,    NULL },	/* 0xd3 */
  { "ANDB ",  4,  2,   D_Direct,    NULL },	/* 0xd4 */
  { "BITB ",  4,  2,   D_Direct,    NULL },	/* 0xd5 */
  { "LDB  ",  4,  2,   D_Direct,    NULL },	/* 0xd6 */
  { "STB  ",  4,  2,   D_Direct,    NULL },	/* 0xd7 */
  { "EORB ",  4,  2,   D_Direct,    NULL },	/* 0xd8 */
  { "ADCB ",  4,  2,   D_Direct,    NULL },	/* 0xd9 */
  { "ORB  ",  4,  2,   D_Direct,    NULL },	/* 0xda */
  { "ADDB ",  4,  2,   D_Direct,    NULL },	/* 0xdb */
  { "LDD  ",  5,  2,   D_Direct,    NULL },	/* 0xdc */
  { "STD  ",  5,  2,   D_Direct,    NULL },	/* 0xdd */
  { "LDU  ",  5,  2,   D_Direct,    NULL },	/* 0xde */
  { "STU  ",  5,  2,   D_Direct,    NULL },	/* 0xdf */

  { "SUBB ",  4,  2,   D_Indexed,   NULL },	   /* 0xe0 */
  { "CMPB ",  4,  2,   D_Indexed,   NULL },	   /* 0xe1 */
  { "SBCB ",  4,  2,   D_Indexed,   NULL },	   /* 0xe2 */
  { "ADDD ",  6,  2,   D_Indexed,   NULL },  	   /* 0xe3 */
  { "ANDB ",  4,  2,   D_Indexed,   NULL },	   /* 0xe4 */
  { "BITB ",  4,  2,   D_Indexed,   NULL },	   /* 0xe5 */
  { "LDB  ",  4,  2,   D_Indexed,   NULL },	   /* 0xe6 */
  { "STB  ",  4,  2,   D_Indexed,   NULL },	   /* 0xe7 */
  { "EORB ",  4,  2,   D_Indexed,   NULL },	   /* 0xe8 */
  { "ADCB ",  4,  2,   D_Indexed,   NULL },	   /* 0xe9 */
  { "ORB  ",  4,  2,   D_Indexed,   NULL },	   /* 0xea */
  { "ADDB ",  4,  2,   D_Indexed,   NULL },	   /* 0xeb */
  { "LDD  ",  5,  2,   D_Indexed,   NULL },	   /* 0xec */
  { "STD  ",  5,  2,   D_Indexed,   NULL },	   /* 0xed */
  { "LDU  ",  5,  2,   D_Indexed,   NULL },	   /* 0xee */
  { "STU  ",  5,  2,   D_Indexed,   NULL },	   /* 0xef */

  { "SUBB ",  5,  3,   D_Extended,  NULL },	   /* 0xf0 */
  { "CMPB ",  5,  3,   D_Extended,  NULL },	   /* 0xf1 */
  { "SBCB ",  5,  3,   D_Extended,  NULL },	   /* 0xf2 */
  { "ADDD ",  7,  3,   D_Extended,  NULL },	   /* 0xf3 */
  { "ANDB ",  5,  3,   D_Extended,  NULL },	   /* 0xf4 */
  { "BITB ",  5,  3,   D_Extended,  NULL },	   /* 0xf5 */
  { "LDB  ",  5,  3,   D_Extended,  NULL },	   /* 0xf6 */
  { "STB  ",  5,  3,   D_Extended,  NULL },	   /* 0xf7 */
  { "EORB ",  5,  3,   D_Extended,  NULL },	   /* 0xf8 */
  { "ADCB ",  5,  3,   D_Extended,  NULL },	   /* 0xf9 */
  { "ORB  ",  5,  3,   D_Extended,  NULL },	   /* 0xfa */
  { "ADDB ",  5,  3,   D_Extended,  NULL },	   /* 0xfb */
  { "LDD  ",  6,  3,   D_Extended,  NULL },	   /* 0xfc */
  { "STD  ",  6,  3,   D_Extended,  NULL },	   /* 0xfd */
  { "LDU  ",  6,  3,   D_Extended,  NULL },	   /* 0xfe */
  { "STU  ",  6,  3,   D_Extended,  NULL },   	   /* 0xff */
};

Opcode optable10[] = {
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x00 */
  { "?????",  0,  1,   D_Illegal,   NULL },	   /* 0x01 */
  { "?????",  0,  1,   D_Illegal,   NULL },	   /* 0x02 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x03 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x04 */
  { "?????",  0,  1,   D_Illegal,   NULL },	   /* 0x05 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x06 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x07 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x08 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x09 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x0a */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x0b */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x0c */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x0d */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x0e */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x0f */

  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x10 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x11 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x12 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x13 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x14 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x15 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x16 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x17 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x18 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x19 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x1a */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x1b */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x1c */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x1d */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x1e */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x1f */

  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x20 */
  { "LBRN ",  5,  4,   D_RelativeL, NULL },        /* 0x21 */
  { "LBHI ",  5,  4,   D_RelativeL, NULL },        /* 0x22 */
  { "LBLS ",  5,  4,   D_RelativeL, NULL },        /* 0x23 */
  { "LBCC ",  5,  4,   D_RelativeL, NULL },        /* 0x24 */
  { "LBCS ",  5,  4,   D_RelativeL, NULL },        /* 0x25 */
  { "LBNE ",  5,  4,   D_RelativeL, NULL },        /* 0x26 */
  { "LBEQ ",  5,  4,   D_RelativeL, NULL },        /* 0x27 */
  { "LBVC ",  5,  4,   D_RelativeL, NULL },        /* 0x28 */
  { "LBVS ",  5,  4,   D_RelativeL, NULL },        /* 0x29 */
  { "LBPL ",  5,  4,   D_RelativeL, NULL },        /* 0x2a */
  { "LBMI ",  5,  4,   D_RelativeL, NULL },        /* 0x2b */
  { "LBGE ",  5,  4,   D_RelativeL, NULL },        /* 0x2c */
  { "LBLT ",  5,  4,   D_RelativeL, NULL },        /* 0x2d */
  { "LBGT ",  5,  4,   D_RelativeL, NULL },        /* 0x2e */
  { "LBLE ",  5,  4,   D_RelativeL, NULL },        /* 0x2f */

  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x30 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x31 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x32 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x33 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x34 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x35 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x36 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x37 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x38 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x39 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x3a */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x3b */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x3c */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x3d */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x3e */
/* Fake SWI2 as an OS9 F$xxx system call */
  { "OS9  ",  20, 3,   D_OS9,  NULL },        /* 0x3f */

  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x40 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x41 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x42 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x43 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x44 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x45 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x46 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x47 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x48 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x49 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x4a */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x4b */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x4c */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x4d */
  { "?????",  0,  1,   D_Illegal,   NULL },	   /* 0x4e */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x4f */

  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x50 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x51 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x52 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x53 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x54 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x55 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x56 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x57 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x58 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x59 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x5a */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x5b */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x5c */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x5d */
  { "?????",  0,  1,   D_Illegal,   NULL },	   /* 0x5e */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x5f */

  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x60 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x61 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x62 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x63 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x64 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x65 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x66 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x67 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x68 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x69 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x6a */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x6b */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x6c */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x6d */
  { "?????",  0,  1,   D_Illegal,   NULL },	   /* 0x6e */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x6f */

  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x70 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x71 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x72 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x73 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x74 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x75 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x76 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x77 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x78 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x79 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x7a */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x7b */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x7c */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x7d */
  { "?????",  0,  1,   D_Illegal,   NULL },	   /* 0x7e */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x7f */

  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x80 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x81 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x82 */
  { "CMPD ",  5,  4,   D_ImmediatL, NULL },        /* 0x83 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x84 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x85 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x86 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x87 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x88 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x89 */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x8a */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x8b */
  { "CMPY ",  5,  4,   D_ImmediatL, NULL },        /* 0x8c */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x8d */
  { "LDY  ",  4,  4,   D_ImmediatL, NULL },	   /* 0x8e */
  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x8f */

             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x90 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x91 */
 	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x92 */
             { "CMPD ",  7,  3,   D_Direct,    NULL },        /* 0x93 */
			  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x94 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x95 */
			  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x96 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x97 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x98 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x99 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x9a */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x9b */
             { "CMPY ",  7,  3,   D_Direct,    NULL },        /* 0x9c */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x9d */
				 { "LDY  ",  6,  3,   D_Direct,    NULL },	 /* 0x9e */
             { "STY  ",  6,  3,   D_Direct,    NULL },        /* 0x9f */

	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xa0 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xa1 */
 	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xa2 */
             { "CMPD ",  7,  3,   D_Indexed,   NULL },        /* 0xa3 */
   	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xa4 */
				 { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xa5 */
   	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xa6 */
				 { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xa7 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xa8 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xa9 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xaa */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xab */
             { "CMPY ",  7,  3,   D_Indexed,   NULL },        /* 0xac */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xad */
             { "LDY  ",  6,  3,   D_Indexed,   NULL },	 /* 0xae */
				 { "STY  ",  6,  3,   D_Indexed,   NULL },        /* 0xaf */

	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xb0 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xb1 */
 	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xb2 */
             { "CMPD ",  8,  4,   D_Extended,  NULL },        /* 0xb3 */
   	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xb4 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xb5 */
			  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xb6 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xb7 */
				 { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xb8 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xb9 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xba */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xbb */
             { "CMPY ",  8,  4,   D_Extended,  NULL },        /* 0xbc */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xbd */
             { "LDY  ",  7,  4,   D_Extended,  NULL },	 /* 0xbe */
             { "STY  ",  7,  4,   D_Extended,  NULL },        /* 0xbf */

	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xc0 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xc1 */
 	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xc2 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xc3 */
   	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xc4 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xc5 */
   	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xc6 */
				 { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xc7 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xc8 */
				 { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xc9 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xca */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xcb */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xcc */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xcd */
             { "LDS  ",  4,  4,   D_ImmediatL, NULL },	 /* 0xce */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xcf */

		  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xd0 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xd1 */
 	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xd2 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xd3 */
   	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xd4 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xd5 */
   	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xd6 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xd7 */
				 { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xd8 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xd9 */
				 { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xda */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xdb */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xdc */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xdd */
             { "LDS  ",  6,  3,   D_Direct,    NULL },	 /* 0xde */
             { "STS  ",  6,  3,   D_Direct,    NULL },        /* 0xdf */

	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xe0 */
				 { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xe1 */
 	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xe2 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xe3 */
   	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xe4 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xe5 */
   	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xe6 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xe7 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xe8 */
				 { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xe9 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xea */
				 { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xeb */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xec */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xed */
             { "LDS  ",  6,  3,   D_Indexed,   NULL },	 /* 0xee */
             { "STS  ",  6,  3,   D_Indexed,   NULL },        /* 0xef */

	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xf0 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xf1 */
		  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xf2 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xf3 */
   	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xf4 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xf5 */
   	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xf6 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xf7 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xf8 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xf9 */
				 { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xfa */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xfb */
				 { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xfc */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xfd */
             { "LDS  ",  7,  4,   D_Extended,  NULL },	 /* 0xfe */
             { "STS  ",  7,  4,   D_Extended,  NULL },        /* 0xff */

};


Opcode optable11[] = {
	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x00 */
             { "?????",  0,  1, 	D_Illegal,   NULL },	 /* 0x01 */
             { "?????",  0,  1,	D_Illegal,   NULL },	 /* 0x02 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x03 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x04 */
             { "?????",  0,  1,	D_Illegal,   NULL },	 /* 0x05 */
	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x06 */
		  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x07 */
	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x08 */
		  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x09 */
	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x0a */
	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x0b */
	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x0c */
	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x0d */
	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x0e */
	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x0f */

		  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x10 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x11 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x12 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x13 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x14 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x15 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x16 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x17 */
		  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x18 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x19 */
				 { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x1a */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x1b */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x1c */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x1d */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x1e */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x1f */
		     
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x20 */
				 { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x21 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x22 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x23 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x24 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x25 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x26 */
	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x27 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x28 */
				 { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x29 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x2a */
				 { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x2b */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x2c */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x2d */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x2e */
	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x2f */

             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x30 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x31 */
				 { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x32 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x33 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x34 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x35 */
	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x36 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x37 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x38 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x39 */
				 { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x3a */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x3b */
				 { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x3c */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x3d */
	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x3e */
             { "SWI3 ",  20, 2,   D_Inherent,  NULL },        /* 0x3f */

	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x40 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x41 */
 	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x42 */
				 { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x43 */
   	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x44 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x45 */
   	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x46 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x47 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x48 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x49 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x4a */
				 { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x4b */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x4c */
				 { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x4d */
             { "?????",  0,  1,   D_Illegal,   NULL },	 /* 0x4e */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x4f */

	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x50 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x51 */
 	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x52 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x53 */
			  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x54 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x55 */
   	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x56 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x57 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x58 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x59 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x5a */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x5b */
				 { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x5c */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x5d */
				 { "?????",  0,  1,   D_Illegal,   NULL },	 /* 0x5e */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x5f */

	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x60 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x61 */
 	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x62 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x63 */
   	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x64 */
				 { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x65 */
   	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x66 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x67 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x68 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x69 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x6a */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x6b */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x6c */
				 { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x6d */
             { "?????",  0,  1,   D_Illegal,   NULL },	 /* 0x6e */
				 { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x6f */

	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x70 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x71 */
 	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x72 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x73 */
   	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x74 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x75 */
			  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x76 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x77 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x78 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x79 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x7a */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x7b */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x7c */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x7d */
				 { "?????",  0,  1,   D_Illegal,   NULL },	 /* 0x7e */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x7f */

	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x80 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x81 */
 	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x82 */
             { "CMPU ",  5,  4,   D_ImmediatL, NULL },        /* 0x83 */
   	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x84 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x85 */
   	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x86 */
				 { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x87 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x88 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x89 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x8a */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x8b */
             { "CMPS ",  5,  4,   D_ImmediatL, NULL },        /* 0x8c */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x8d */
             { "?????",  0,  1,   D_Illegal,   NULL },	 /* 0x8e */
				 { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x8f */

		  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x90 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x91 */
 	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x92 */
             { "CMPU ",  7,  3,   D_Direct,    NULL },        /* 0x93 */
   	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x94 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x95 */
   	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x96 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x97 */
				 { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x98 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x99 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x9a */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x9b */
             { "CMPS ",  7,  3,   D_Direct,    NULL },        /* 0x9c */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x9d */
             { "?????",  0,  1,   D_Illegal,   NULL },	 /* 0x9e */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0x9f */

	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xa0 */
				 { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xa1 */
 	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xa2 */
             { "CMPU ",  7,  3,   D_Indexed,   NULL },        /* 0xa3 */
   	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xa4 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xa5 */
   	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xa6 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xa7 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xa8 */
				 { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xa9 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xaa */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xab */
             { "CMPS ",  7,  3,   D_Indexed,   NULL },        /* 0xac */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xad */
             { "?????",  0,  1,   D_Illegal,   NULL },	 /* 0xae */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xaf */

		  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xb0 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xb1 */
		  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xb2 */
             { "CMPU ",  8,  4,   D_Extended,  NULL },        /* 0xb3 */
   	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xb4 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xb5 */
   	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xb6 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xb7 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xb8 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xb9 */
				 { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xba */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xbb */
             { "CMPS ",  8,  4,   D_Extended,  NULL },        /* 0xbc */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xbd */
             { "?????",  0,  1,   D_Illegal,   NULL },	 /* 0xbe */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xbf */

	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xc0 */
				 { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xc1 */
 	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xc2 */
				 { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xc3 */
   	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xc4 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xc5 */
   	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xc6 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xc7 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xc8 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xc9 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xca */
				 { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xcb */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xcc */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xcd */
             { "?????",  0,  1,   D_Illegal,   NULL },	 /* 0xce */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xcf */

	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xd0 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xd1 */
		  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xd2 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xd3 */
			  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xd4 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xd5 */
   	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xd6 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xd7 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xd8 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xd9 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xda */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xdb */
				 { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xdc */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xdd */
             { "?????",  0,  1,   D_Illegal,   NULL },	 /* 0xde */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xdf */

	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xe0 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xe1 */
 	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xe2 */
				 { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xe3 */
   	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xe4 */
				 { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xe5 */
   	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xe6 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xe7 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xe8 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xe9 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xea */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xeb */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xec */
				 { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xed */
             { "?????",  0,  1,   D_Illegal,   NULL },	 /* 0xee */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xef */

	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xf0 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xf1 */
 	     { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xf2 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xf3 */
			  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xf4 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xf5 */
			  { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xf6 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xf7 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xf8 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xf9 */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xfa */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xfb */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xfc */
             { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xfd */
				 { "?????",  0,  1,   D_Illegal,   NULL },	 /* 0xfe */
				 { "?????",  0,  1,   D_Illegal,   NULL },        /* 0xff */
};


struct os9syscall { int code; char *name; } os9sys[] = {
    {0x0000,"F$LINK"},
    {0x0001,"F$LOAD"},
    {0x0002,"F$UNLINK"},
    {0x0003,"F$FORK"},
    {0x0004,"F$WAIT"},
    {0x0005,"F$CHAIN"},
    {0x0006,"F$EXIT"},
    {0x0007,"F$MEM"},
    {0x0008,"F$SEND"},
    {0x0009,"F$ICPT"},
    {0x000a,"F$SLEEP"},
    {0x000b,"F$SSPD"},
    {0x000c,"F$ID"},
    {0x000d,"F$SPRIOR"},
    {0x000e,"F$SSWI"},
    {0x000f,"F$PERR"},
    {0x0010,"F$PRSNAM"},
    {0x0011,"F$CMPNAM"},
    {0x0012,"F$SCHBIT"},
    {0x0013,"F$ALLBIT"},
    {0x0014,"F$DELBIT"},
    {0x0015,"F$TIME"},
    {0x0016,"F$STIME"},
    {0x0017,"F$CRC"},
    {0x0018,"F$GPRDSC"},
    {0x0019,"F$GBLKMP"},
    {0x001a,"F$GMODDR"},
    {0x001b,"F$CPYMEM"},
    {0x001c,"F$SUSER"},
    {0x001d,"F$UNLOAD"},
    {0x001e,"F$ALARM"},
    {0x0021,"F$NMLINK"},
    {0x0022,"F$NMLOAD"},
    {0x0023,"F$DEBUG"},
    {0x0025,"F$TPS"},
    {0x0026,"F$TIMALM"},
    {0x0027,"F$VIRQ"},
    {0x0028,"F$SRQMEM"},
    {0x0029,"F$SRTMEM"},
    {0x002a,"F$IRQ"},
    {0x002b,"F$IOQU"},
    {0x002c,"F$APROC"},
    {0x002d,"F$NPROC"},
    {0x002e,"F$VMODUL"},
    {0x002f,"F$FIND64"},
    {0x0030,"F$ALL64"},
    {0x0031,"F$RET64"},
    {0x0032,"F$SSVC"},
    {0x0033,"F$IODEL"},
    {0x0034,"F$SLINK"},
    {0x0035,"F$BOOT"},
    {0x0036,"F$BTMEM"},
    {0x0037,"F$GPROCP"},
    {0x0038,"F$MOVE"},
    {0x0039,"F$ALLRAM"},
    {0x003a,"F$ALLIMG"},
    {0x003b,"F$DELIMG"},
    {0x003c,"F$SETIMG"},
    {0x003d,"F$FREELB"},
    {0x003e,"F$FREEHB"},
    {0x003f,"F$ALLTSK"},
    {0x0040,"F$DELTSK"},
    {0x0041,"F$SETTSK"},
    {0x0042,"F$RESTSK"},
    {0x0043,"F$RELTSK"},
    {0x0044,"F$DATLOG"},
    {0x0045,"F$DATTMP"},
    {0x0046,"F$LDAXY"},
    {0x0047,"F$LDAXYP"},
    {0x0048,"F$LDDDXY"},
    {0x0049,"F$LDABX"},
    {0x004a,"F$STABX"},
    {0x004b,"F$ALLPRC"},
    {0x004c,"F$DELPRC"},
    {0x004d,"F$ELINK"},
    {0x004e,"F$FMODUL"},
    {0x004f,"F$MAPBLK"},
    {0x0050,"F$CLRBLK"},
    {0x0051,"F$DELRAM"},
    {0x0052,"F$GCMDIR"},
    {0x0053,"F$ALHRAM"},
    {0x0054,"F$REBOOT"},
    {0x0055,"F$CRCMOD"},
    {0x0056,"F$XTIME"},
    {0x0057,"F$VBLOCK"},
    {0x0070,"F$REGDMP"},
    {0x0071,"F$NVRAM"},
    {0x0080,"I$ATTACH"},
    {0x0081,"I$DETACH"},
    {0x0082,"I$DUP"},
    {0x0083,"I$CREATE"},
    {0x0084,"I$OPEN"},
    {0x0085,"I$MAKDIR"},
    {0x0086,"I$CHGDIR"},
    {0x0087,"I$DELETE"},
    {0x0088,"I$SEEK"},
    {0x0089,"I$READ"},
    {0x008a,"I$WRITE"},
    {0x008b,"I$READLN"},
    {0x008c,"I$WRITLN"},
    {0x008d,"I$GETSTT"},
    {0x008e,"I$SETSTT"},
    {0x008f,"I$CLOSE"},
    {0x0090,"I$DELETX"},
  } ;


int iotable[32] = {
	0x0000,
	0x0001,
	0x0002,
	0x0003,
	0x0008,
	0x0009,
	0x000a,
	0x000b,
	0x000c,
	0x000d,
	0x000e,
	0x0010,
	0x0011,
	0x0012,
	0x0013,
	0x0014,
	0x8000,
	0x8001,
	0x8002,
	0x8003,
	0x8004,
	0x8005,
	0x8006,
	0x8007,
	0x8008,
	0x8009,
	0x800a,
	0x800b,
	0x800c,
	0x800d,
	0x800e,
	0x800f,
};

char *iocomment[32] = {
	"Data direction register port 1",
	"Data direction register port 2",
	"I/O register port 1",
	"I/O register port 2",
	"Timer control and status",
	"Counter high byte",
	"Counter low byte",
	"Output compare high byte",
	"Output compare low byte",
	"Input capture high byte",
	"Input capture low byte",
	"Serial rate and mode register",
	"Serial control and status register",
	"Serial receiver data register",
	"Serial transmit data register",
	"Ram control register",
	"Modem port 0",
	"Modem port 1",
	"Modem port 2",
	"Modem port 3",
	"Modem port 4",
	"Modem port 5",
	"Modem port 6",
	"Modem port 7",
	"Modem port 8",
	"Modem port 9",
	"Modem port 10",
	"Modem port 11",
	"Modem port 12",
	"Modem port 13",
	"Modem port 14",
	"Modem port 15",
};

char *Inter_Register[16]={"D","X","Y","U","S","PC","??","??","A","B","CC","DP","??","??","??","??"};

char *Indexed_Register[4]={"X","Y","U","S"};

int  lastio = 32;

#pragma argsused
int D_Illegal(op, code, pc, suffix)
Opcode *op;
int code;
int pc;
char *suffix;
{
  fprintf(fp,"%0.2X          %s%s", code, suffix, op->name);
  return op->bytes;
}

#pragma argsused
int D_Direct(op, code, pc, suffix)
Opcode *op;
int code;
int pc;
char *suffix;
{
  int offset;

  offset = prog[pc+1];
  fprintf(fp,"%0.2X %0.2X       %s%s       <$%0.2X",
	code, offset, suffix, op->name, offset);
  return op->bytes;
}

#pragma argsused
int D_Page10(op, code, pc, suffix)
Opcode *op;
int code;
int pc;
char *suffix;
{
  fprintf(fp,"10 ");
  code = prog[pc+1];
  return (*optable10[code].display)(&optable10[code], code, pc+1, "");
}

#pragma argsused
int D_Page11(op, code, pc, suffix)
Opcode *op;
int code;
int pc;
char *suffix;
{
  fprintf(fp,"11 ");
  code = prog[pc+1];
  return (*optable11[code].display)(&optable11[code], code, pc+1, "");
}

#pragma argsused
int D_Immediat(op, code, pc, suffix)
Opcode *op;
int code;
int pc;
char *suffix;
{
  int offset;

  offset = prog[pc+1];
  fprintf(fp,"%0.2X %0.2X       %s%s       #$%0.2X",
	code, offset, suffix, op->name, offset);
  return op->bytes;
}

#pragma argsused
int D_ImmediatL(op, code, pc, suffix)
Opcode *op;
int code;
int pc;
char *suffix;
{
  int offset;

  offset = prog[pc+1] * 256 + prog[pc+2];
  fprintf(fp,"%0.2X %0.2X %0.2X    %s%s       #$%0.4X",
	code, prog[pc+1], prog[pc+2], suffix, op->name, offset);
  return op->bytes;
}

#pragma argsused
int D_Inherent(op, code, pc, suffix)
Opcode *op;
int code;
int pc;
char *suffix;
{
  fprintf(fp,"%0.2X          %s%s", code, suffix, op->name);
  return op->bytes;
}

#pragma argsused
int D_OS9(op, code, pc, suffix)
Opcode *op;
int code;
int pc;
char *suffix;
{
  int offset;

  offset = prog[pc+1];
  for(int i =0, j = sizeof(os9sys)/sizeof(struct os9syscall), m = (i+j)/2 ;i<=j; m=(i+j)/2  ) {
     if (os9sys[m].code > offset) {
         j=m-1; 
     } else if (os9sys[m].code < offset) {
         i=m+1; 
     } else if (os9sys[m].code == offset) {
          fprintf(fp,"%0.2X %0.2X       %s%s       %s",
                code, offset, suffix, op->name, os9sys[m].name);
          return op->bytes;
     } 
  } 
  fprintf(fp,"%0.2X %0.2X       %s%s       $%0.2X",
	code, offset, suffix, op->name, prog[pc+1]);
  return op->bytes;
}

#pragma argsused
char *IndexRegister(postbyte)
int postbyte;
{
  return Indexed_Register[ (postbyte>>5) & 0x03];
}

#pragma argsused
int D_Indexed(op, code, pc, suffix)
Opcode *op;
int code;
int pc;
char *suffix;
{
  int postbyte;
  char *s;
  int extrabytes;
  int disp;
  int address;
  int offset;

  extrabytes = 0;
  postbyte = prog[pc+1];
  if ((postbyte & 0x80) == 0x00) {
	 disp = postbyte & 0x1f;
	 if ((postbyte & 0x10) == 0x10) {
		s = "-";
		disp=0x20-disp;
	      }
	 else
		s = "+";
	 fprintf(fp,"%0.2X %0.2X       %s%s       %s$%0.2X,%s",
			code, postbyte, suffix, op->name, s,disp,IndexRegister(postbyte));
  } else {
	 switch(postbyte & 0x1f) {
	 case 0x00 :
		fprintf(fp,"%0.2X %0.2X       %s%s       ,%s+",
				  code, postbyte, suffix, op->name, IndexRegister(postbyte));
		break;
	 case 0x01 :
		fprintf(fp,"%0.2X %0.2X       %s%s       ,%s++",
				  code, postbyte, suffix, op->name, IndexRegister(postbyte));
		break;
	 case 0x02 :
		fprintf(fp,"%0.2X %0.2X       %s%s       ,-%s",
				  code, postbyte, suffix, op->name, IndexRegister(postbyte));
		break;
	 case 0x03 :
		fprintf(fp,"%0.2X %0.2X       %s%s       ,--%s",
				  code, postbyte, suffix, op->name, IndexRegister(postbyte));
		break;
	 case 0x04 :
		fprintf(fp,"%0.2X %0.2X       %s%s       ,%s",
				  code, postbyte, suffix, op->name, IndexRegister(postbyte));
		break;
	 case 0x05 :
		fprintf(fp,"%0.2X %0.2X       %s%s       B,%s",
				  code, postbyte, suffix, op->name, IndexRegister(postbyte));
		break;
	 case 0x06 :
		fprintf(fp,"%0.2X %0.2X       %s%s       A,%s",
				  code, postbyte, suffix, op->name, IndexRegister(postbyte));
		break;
	 case 0x07 :
		break;
	 case 0x08 :
		offset = prog[pc+2];
		if (offset < 128)
		  s = "+";
		else {
		  s = "-";
		  offset=0x0100-offset;
		}
		fprintf(fp,"%0.2X %0.2X %0.2X    %s%s       %s$%0.2X,%s",
			code, postbyte, prog[pc+2], suffix, op->name, s, offset,
		IndexRegister(postbyte));
		extrabytes=1;
		break;
	 case 0x09 :
		offset = prog[pc+2] * 256 + prog[pc+3];
		if (offset < 32768)
			s = "+";
		else {
		  s = "-";
		  offset=0xffff-offset+1;
		}
		fprintf(fp,"%0.2X %0.2X %0.2X %0.2X %s%s       %s$%0.4X,%s",
			code, postbyte, prog[pc+2], prog[pc+3], suffix, op->name, s, offset,
		IndexRegister(postbyte));
		extrabytes=2;
		break;
	 case 0x0a :
		break;
	 case 0x0b :
		fprintf(fp,"%0.2X %0.2X       %s%s       D,%s",
				  code, postbyte, suffix, op->name, IndexRegister(postbyte));
		break;
	 case 0x0c :
		offset = (*(char *)(prog+pc+2)+pc+3) & 0xFFFF;
		s = "<";
		fprintf(fp,"%0.2X %0.2X %0.2X    %s%s       %s$%0.2X,PCR",
				  code, postbyte, prog[pc+2], suffix, op->name, s, offset+adoffset);
		extrabytes = 1;
		break;
	 case 0x0d :
		offset = prog[pc+2] * 256 + prog[pc+3];
		offset = ((offset>0x7fff? offset-0x10000 : offset )+pc+4) & 0xFFFF;
		s = ">";
		fprintf(fp,"%0.2X %0.2X %0.2X %0.2X %s%s       %s$%0.4X,PCR",
				  code, postbyte, prog[pc+2], prog[pc+3], suffix, op->name, s, offset+adoffset);
		extrabytes = 2;
		break;
	 case 0x0e :
		break;
	 case 0x0f :
		fprintf(fp,"%0.2X %0.2X       %s?????",
				  code, postbyte, suffix);
		break;
	 case 0x10 :
		fprintf(fp,"%0.2X %0.2X       %s?????",
				  code, postbyte, suffix);
		break;
	 case 0x11 :
		fprintf(fp,"%0.2X %0.2X       %s%s       [,%s++]",
				  code, postbyte, suffix, op->name, IndexRegister(postbyte));
		break;
	 case 0x12 :
		fprintf(fp,"%0.2X %0.2X       %s?????",
				  code, postbyte, suffix);
		break;
	 case 0x13 :
		fprintf(fp,"%0.2X %0.2X       %s%s       [,--%s]",
				  code, postbyte, suffix, op->name, IndexRegister(postbyte));
		break;
	 case 0x14 :
		fprintf(fp,"%0.2X %0.2X       %s%s       [,%s]",
				  code, postbyte, suffix, op->name, IndexRegister(postbyte));
		break;
	 case 0x15 :
		fprintf(fp,"%0.2X %0.2X       %s%s       [B,%s]",
				  code, postbyte, suffix, op->name, IndexRegister(postbyte));
		break;
	 case 0x16 :
		fprintf(fp,"%0.2X %0.2X       %s%s       [A,%s]",
				  code, postbyte, suffix, op->name, IndexRegister(postbyte));
		break;
	 case 0x17 :
		break;
	 case 0x18 :
		offset = prog[pc+2];
		if (offset < 128)
		  s = "+";
		else {
		  s = "-";
		  offset=0x0100-offset;
		}
		fprintf(fp,"%0.2X %0.2X %0.2X    %s%s       [%s$%0.2X,%s]",
			code, postbyte, prog[pc+2], suffix, op->name, s, offset,
		IndexRegister(postbyte));
		extrabytes = 1;
		break;
	 case 0x19 :
		offset = prog[pc+2] * 256 + prog[pc+3];
		if (offset < 32768)
		  s = "+";
		else {
		  s = "-";
		  offset=0xffff-offset+1;
		}
		fprintf(fp,"%0.2X %0.2X %0.2X %0.2X %s%s       [%s$%0.4X,%s]",
			code, postbyte, prog[pc+2], prog[pc+3], suffix, op->name, s, offset,
		IndexRegister(postbyte));
		extrabytes = 2;
		break;
	 case 0x1a :
		break;
	 case 0x1b :
		fprintf(fp,"%0.2X %0.2X       %s%s       [D,%s]",
				  code, postbyte, suffix, op->name, IndexRegister(postbyte));
		break;
	 case 0x1c :
		offset = (*((char*)prog+pc+2)+pc+3) & 0xFFFF;
		s = "<";
		fprintf(fp,"%0.2X %0.2X %0.2X    %s%s       [%s$%0.2X,PCR]",
				  code, postbyte, prog[pc+2], suffix, op->name, s, offset+adoffset);
		extrabytes = 1;
		break;
	 case 0x1d :
		offset = prog[pc+2] * 256 + prog[pc+3];
		offset = ((offset>0x7fff?offset-0x8001 : offset )+pc+4) & 0xFFFF;
		s = ">";
		fprintf(fp,"%0.2X %0.2X %0.2X %0.2X %s%s       [%s$%0.4X,PCR]",
				  code, postbyte, prog[pc+2], prog[pc+3], suffix, op->name, s, offset+adoffset);
		extrabytes = 2;
		break;
	 case 0x1e :
		break;
	 case 0x1f :
		address = prog[pc+2] * 256 + prog[pc+3];
		extrabytes = 2;
		fprintf(fp,"%0.2X %0.2X %0.2X %0.2X %s%s       [$%4X]",
				  code, postbyte, prog[pc+2], prog[pc+3], suffix, op->name, address);
		break;
	 }
  }
  return op->bytes + extrabytes;
}

#pragma argsused
int D_Extended(op, code, pc, suffix)
Opcode *op;
int code;
int pc;
char *suffix;
{
  int offset;

  offset = prog[pc+1] * 256 + prog[pc+2];
  fprintf(fp,"%0.2X %0.2X %0.2X    %s%s       $%0.4X",
	code, prog[pc+1], prog[pc+2], suffix, op->name, offset);
  return op->bytes;
}

#pragma argsused
int D_Relative(op, code, pc, suffix)
Opcode *op;
int code;
int pc;
char *suffix;
{
  int offset;
  int disp;

  offset = prog[pc+1];
  if (offset < 127 )
	 disp   = pc + 2 + offset + adoffset;
  else
	 disp   = pc + 2 - (256 - offset + adoffset);
  fprintf(fp,"%0.2X %0.2X       %s%s       $%0.4X",
	code, offset, suffix, op->name, disp);
  return op->bytes;
}

#pragma argsused
int D_RelativeL(op, code, pc, suffix)
Opcode *op;
int code;
int pc;
char *suffix;
{
  int offset;
  int disp;

  offset = prog[pc+1] * 256 + prog[pc+2];
  if (offset < 32767 )
	 disp   = pc + 3 + offset + adoffset;
  else
	 disp   = pc + 3 - (65536 - offset) + adoffset;
  fprintf(fp,"%0.2X %0.2X %0.2X    %s%s       $%0.4X",
	code, prog[pc+1], prog[pc+2], suffix, op->name, disp);
  return op->bytes;
}

#pragma argsused
int D_Register0(op, code, pc, suffix)
Opcode *op;
int code;
int pc;
char *suffix;
{
  int postbyte;

  postbyte = prog[pc+1];

  fprintf(fp,"%0.2X %0.2X       %s%s       %s,%s",
	code, postbyte, suffix, op->name, Inter_Register[postbyte>>4], Inter_Register[postbyte & 0x0F]);


  return op->bytes;
}

#pragma argsused
int D_Register1(op, code, pc, suffix)
Opcode *op;
int code;
int pc;
char *suffix;
{
  int postbyte;
  int i;
  int flag=0;
  static char *s_stack[8]={"PC","U","Y","X","DP","B","A","CC"};
  static int bits[8]={0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01};

  postbyte = prog[pc+1];	

  fprintf(fp,"%0.2X %0.2X       %s%s       ", 
  	code, postbyte, suffix, op->name);   	

  for(i=0;i<8;i++) {
    if ((postbyte & bits[i]) !=0) {
      if (flag !=0) {
	fprintf(fp,",");
      } else {
	flag=1;
      }
      fprintf(fp,s_stack[i]);
    }
  }
  return op->bytes;
}

#pragma argsused
int D_Register2(op, code, pc, suffix)
Opcode *op;
int code;
int pc;
char *suffix;
{
  int postbyte;
  int i;
  int flag=0;
  static char *u_stack[8]={"PC","S","Y","X","DP","B","A","CC"};
  static int bits[8]={0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01};

  postbyte = prog[pc+1];	
  fprintf(fp,"%0.2X %0.2X       %s%s       ", 
  	code, postbyte, suffix, op->name);   	

  for(i=0;i<8;i++) {
    if ((postbyte & bits[i]) !=0) {
      if (flag !=0) {
	fprintf(fp,",");
      } else {
	flag=1;
      }
      fprintf(fp,u_stack[i]);
    }
  }
  return op->bytes;
}


void hexadump(b, l, loc, w)
unsigned char *b;
int l;
int loc;
int w;
{
  int i;
  int j;
  int end;
  // char b[4096];

  // memset(b, '\0', 4096);
  // memcpy(b, s, l);
  //fprintf(fp,"\n");
  end = ((l%w)>0)?(l/w)+1:(l/w);
  for (j=0;j<end;j++) {
    fprintf(fp,"%04X: ", loc+j*w+adoffset);
    for (i=0;i<w;i++) {
		fprintf(fp,"%02X ", b[j*w+i]);
    }
    fprintf(fp,"|");
    for (i=0;i<w;i++) {
      if ((b[j*w+i] >= 0x20) && (b[j*w+i] < 0x7f)) {
	fprintf(fp,"%c", b[j*w+i]);
      } else {
	fprintf(fp,".");
      }
    }
    fprintf(fp,"|\n");
  }
  //fprintf(fp,"\n");
}

char *comment(arg)
int arg;
{
  int i;

  for (i=0;i<lastio;i++) {
    if (arg == iotable[i]) {
      return iocomment[i];
    }
  }
  return "";
}

int disasm(start, end)
int start;
int end;
{
  int pc;	
  int code;
  int currstring;

  currstring = 0;
  for (pc=start; pc <= end;) {
    code = prog[pc];
    fprintf(fp,"%0.4X: ", pc + adoffset);
    if (currstring < laststring) {
      if (pc == stringtable[currstring].address) {
	  hexadump(&prog[pc], stringtable[currstring].length, pc,
	           stringtable[currstring].width);
	pc += stringtable[currstring].length;
	currstring++;
	continue;
      }
    }
    pc += (*optable[code].display)(&optable[code], code, pc, "   ");
    fprintf(fp,"\n");
  }
  return pc;
}

#ifndef NO_MAIN

int main(int argc, char **argv )
{
  int fd;
  int start,end;
  int size;

  if ( argc > 2 && *argv[1] == '-') {
     if (argv[1][1]=='o') {
        adoffset=strtol(argv[2],(char**)0,0);
        argc-=2;
        argv += 2;
     }
  }
  if ( argc != 4 ) {
    fprintf(stderr, "usage: disasm [-o offset] <file> <start> <end>\n");
    fprintf(stderr, "       where start and end are in hex.\n");
    exit(1);
  }

  sscanf(argv[2],"%x",&start); start -= adoffset;
  sscanf(argv[3],"%x",&end); end     -= adoffset;
  printf("disass %x - %x\n",start,end);

  fp = stdout;

  fd = open(argv[1], O_RDONLY, S_IREAD|S_IWRITE);
  size = read(fd, &prog[0x0000], 0xffff);

  if (end > size) end=size;

  disasm(start, end);
  close(fd);
  return 0;
}

#endif // NO_MAIN
