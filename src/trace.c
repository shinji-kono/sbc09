/* 6808 Simulator V092
 *
 *          2018 Shinji KONO
 * tracer
 *
*/

#include<stdio.h>
#include<stdlib.h>
#include<ctype.h>
#include<signal.h>
#include<sys/time.h>

#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>

#ifdef USE_TERMIOS
#include <termios.h>
#endif

#define engine extern
#include "v09.h"

struct termios termsetting;

int xmstat; /* 0= no XMODEM transfer, 1=send, 2=receiver */
unsigned char xmbuf[132];
int xidx;
int acknak;
int rcvdnak;
int blocknum;

extern FILE *logfile;
extern FILE *infile;
extern FILE *xfile;
extern FILE *disk[];

extern void hexadump( unsigned char *b, int l, int loc, int w);
extern int disasm(int,int);
extern void restore_term(void) ;

#ifdef USE_MMU
extern char *prog ;   // for disass
extern Byte * mem0(Byte *iphymem, Word adr, Byte *immu) ;
extern int paddr(Word adr, Byte *immu) ;
#else
#define paddr(a,m)  (a)
#endif

void do_exit(void) {
        restore_term();
        exit(0);
}


typedef struct bp {
  int address;       // physical address
  int laddr;
  int count;
  int watch;         // watch point
  struct bp *next;
} BP, *BPTR;

BPTR breakpoint = 0;
int bpskip = 0;
int trskip = 0;
int stkskip = 0;

int getterm(char *buf, char** next) {
     int value = 0;
     while (*buf==' ') buf++;
     if (*buf=='x') { value = xreg; buf++; *next = buf ;
     } else if (*buf=='y') { value = yreg; buf++; *next = buf;
     } else if (*buf=='u') { value = ureg; buf++; *next = buf;
     } else if (*buf=='s') { value = sreg; buf++; *next = buf;
     } else if (*buf=='p') { value = pcreg; buf++; *next = buf;
     } else if (*buf=='d') { value = (*areg<<8)+*breg; buf++; *next = buf;
     } else if (*buf=='a') { value = *areg; buf++; *next = buf;
     } else if (*buf=='b') { value = *breg; buf++; *next = buf;
     } else value = strtol(buf,next,0);
     return value;
}

int getarg(char *buf, char** next) {
     int value = 0;
     char *b = buf;
     if (next==0) next = &b;
     value=getterm(*next,next);
     for(;**next;) {
         if  ( **next == '+' ) {
            value += getterm(*next+1,next);
         } else if  ( **next == '*' ) {
            value *= getterm(*next+1,next);
         } else if  ( **next == '/' ) {
            value /= getterm(*next+1,next);
         } else if  ( **next == '-' ) {
            value -= getterm(*next+1,next);
         } else if  ( **next == '&' ) {
            value &= getterm(*next+1,next);
         } else if  ( **next == '|' ) {
            value |= getterm(*next+1,next);
         } else if  ( **next == '(' ) {
            value = getarg(*next+1,next);
            if(**next==')') *next=*next+1;
         } else break;
     }
     return value;
}

void printhelp(void)
{
  printf( 
     "use 0x for hex inputs\n"
     "  s [count]  one step trace\n"
     "  n          step over\n"
     "  f          finish this call (until stack pop)\n"
     "  b [adr]    set break point (on current physical addreaa)\n"
     "  B          break point list\n"
     "  d [n]      delte break point list\n"
     "  c  [count] continue;\n"
     "  p  data    print\n"
     "  x  [adr] [count]  dump\n"
     "  xi [adr] [count]  disassemble\n"
#ifdef USE_MMU
     "  x  [p page] [offset] [count]  dump physical memory\n"
     "  xi [p page] [offset] [count]  disassemble\n"
#endif
     "  0  file    disk drive 0 image\n"
     "  1  file    disk drive 1 image\n"
     "  L  file    start log to file\n"
     "  S  file    set input file\n"
     "  X  exit\n"
     "  q  exit\n"
     "  U  file    upload from srecord file \n"
     "  D  file    download to srecord file \n"
     "  R  do reset\n"
     "  h,?  print this\n"
    );
}

               
void setbreak(int adr,int count) ;
int nexti(void);

void do_escape(void) {
        char s[80];
        int adr,page;
        int ppc = paddr(pcreg,mmu);
        if (bpskip) { // skip unbreak instruction
            bpskip--;
            BPTR *prev = &breakpoint;
            for(BPTR b = breakpoint; b ; prev=&b->next, b=b->next ) {
#ifdef USE_MMU
                int watch = phymem[b->address];
#else
                int watch = mem[b->address];
#endif
                if (ppc==b->address || b->watch != watch  ) {
                    b->watch = watch;
                    if (b->count==-1) {  // temporaly break point
                        BPTR next = b->next;
                        free(b);
                        *prev = next;
                        goto restart0;
                    }
                    if (b->count) b->count--;
                    if (b->count==0) {
                        goto restart0;
                    }
                }
            }
            return;
        }
        if (stkskip) { // skip until return
#ifdef USE_MMU
           if (phymem[ppc]==0x3b||(phymem[ppc]==0x10&&phymem[ppc+1]==0x3f)) 
               goto restart0;
#else
           if (mem[ppc]==0x3b||(mem[ppc]==0x10&&mem[ppc+1]==0x3f)) 
               goto restart0;
#endif
           if (sreg < stkskip ) return;
        }
restart0:
        stkskip = 0;
        restore_term();
#ifdef USE_MMU
        Byte *phyadr = phymem + ppc;
        prog = (char*)phyadr - pcreg;
#endif
        do_trace(stdout);
        if (trskip>1) { // show trace and step
            trskip--;
            int watch;         // watch point
            set_term(escchar);
            return; 
        }
restart:
        printf("v09>");
        fgets(s, sizeof(s)-1, stdin); 
        s[strlen(s)-1] = 0; // chop
        switch (s[0]) {
        case 'p':  {
                int d = getarg(s+1,0);
                printf("0x%x %d '%c'\n",d,d,(d<' '||d>0x7f)?' ':d);
                goto restart;
           } 
        case 'n':   // step over
                if (nexti()) {
                   bpskip = -1;
                   break;
                } 
        case 's':   // one step trace
                trskip = 1;
                if (s[1]) {
                   trskip = getarg(s+1,0);
                }
                bpskip = 0;
                attention = escape = 1;
                break;
        case 'f':   // finish this call (until stack pop)
                stkskip = sreg + 2;
                attention = escape = 1;
                break;
        case 'b':   // set break point
                if (s[1]) {
                   char *next;
                   int count = 0;
                   int adr = getarg(s+1,&next);
                   if (next[0]) {
                      count = getarg(next,&next);
                   }
                   setbreak(adr,count);
                } else {
                   setbreak(pcreg,0);
                }
                bpskip = -1;
                goto restart;
        case 'B':   // break point list
                for(BPTR bp = breakpoint; bp ; bp = bp->next) {
#ifdef USE_MMU
                    printf("0x%x p=0x%x c=%d w=0x%x\n", bp->laddr, bp->address, bp->count, bp->watch);
#else
                    printf("0x%x c=%d w=0x%x\n", bp->address, bp->count,bp->watch);
#endif
                }
                goto restart;
        case 'd':   // delte break point list
                if (s[1]) {
                   int trskip = getarg(s+1,0);
                   BPTR *prev = &breakpoint;
                   for(BPTR bp = breakpoint; bp ; prev=&bp->next, bp = bp->next) {
                       if (trskip-- == 0) {
                          BPTR next = bp->next;
                          free(bp);
                          *prev = next;
                          break;
                       }
                       prev = &bp->next;
                   }
                }
                goto restart;
        case 'c':   // continue;
                bpskip = -1;
                attention = escape = 1;
                if (s[1]) {
                   bpskip = getarg(s+1,0);
                }
                break;
                /*
                 * we should have disassembler for a mmu page
                 */
        case 'x':   // dump 
           {    char d = 0;
                char p = 0;
                char *next = s+1;
                int len = 32;
                int adr = pcreg;
                if (*next=='i') { next++; d='i';
                } 
                if (*next=='p') { 
                   p = 'p';
                   next++;
                   if (next[0]) {
                      page =  getarg(next,&next);
                   }
                }
                if (next[0]) {
                   adr = getarg(next,&next);
#ifdef USE_MMU
                   adr -= adr &0xf;
                   // if (p=='p') adr -= adr&0x1fff;
#endif
                   if (next[0]) {
                       len = getarg(next,&next);
                   }
                }
                for(; len > 0 ; len-=16,adr+=16) {
                    Byte *phyadr = 0;
#ifdef USE_MMU
                    if (p=='p') {
                        phyadr  = phymem + (page * 0x2000 + adr);
                        prog = (char*)phyadr - adr  ;
                    } else {
                        phyadr = mem0(phymem,adr,mmu);
                        prog = (char*)phyadr - adr  ;
                    }
                    if (phyadr > phymem+memsize) goto restart;
#else
                    phyadr = mem+adr;
                    if (phyadr > mem+0xffff) goto restart;
#endif
                    if (d=='i') {
                        adr = disasm(adr,adr+(len>16?16:len));
                    } else {
                        hexadump(phyadr,len>16?16:len,adr,16);
                    }
                }
                goto restart;
            }
        case 'L':
                if (logfile)
                        fclose(logfile);
                logfile = 0;
                if (s[1]) {
                        int i=1; while(s[i]==' ') i++;
                        logfile = fopen(s + i, "w");
                }
                goto restart;
                break;
        case 'S':
                if (infile)
                        fclose(infile);
                infile = 0;
                if (s[1]) {
                        int i=1; while(s[i]==' ') i++;
                        infile = fopen(s + i, "r");
                }
                goto restart;
                break;
        case 'h':
        case '?':
                printhelp();
                goto restart;
        case 'X':
        case 'q':
                if (!xmstat)
                        do_exit();
                else {
                        xmstat = 0;
                        fclose(xfile);
                        xfile = 0;
                }
                goto restart;
                break;
        case '0':
        case '1':
                {   FILE **drv = &disk[ s[0]-'0'] ;
                if (*drv)
                        fclose(*drv);
                *drv = 0;
                if (s[1]) {
                        int i=1; while(s[i]==' ') i++;
                        *drv = fopen(s + i, "r+b");
                        if ( *drv == 0 ) { printf("can't open %s\n", &s[i]); }
                }
                }
                goto restart;
                break;
        case 'U':
                if (xfile)
                        fclose(xfile);
                xfile = 0;
                if (s[1]) {
                        int i=1; while(s[i]==' ') i++;
                        xfile = fopen(s + i, "rb");
                        if ( xfile == 0 ) { printf("can't open %s\n", &s[i]); }
                }
                if (xfile)
                        xmstat = 1;
                else
                        xmstat = 0;
                xidx = 0;
                acknak = 21;
                rcvdnak = EOF;
                blocknum = 1;
                goto restart;
                break;
        case 'D':
                if (xfile)
                        fclose(xfile);
                xfile = 0;
                if (s[1]) {
                        int i=1; while(s[i]==' ') i++;
                        xfile = fopen(s + i, "wb");
                        if ( xfile == 0 ) { printf("can't open %s\n", &s[i]); }
                }
                if (xfile)
                        xmstat = 2;
                else
                        xmstat = 0;
                xidx = 0;
                acknak = 21;
                blocknum = 1;
                goto restart;
                break;
        case 'R':
                pcreg = (mem[0xfffe] << 8) + mem[0xffff];
                bpskip = 0;
#ifdef USE_MMU
                mmu = &mem[0xffa0];
                mem[0xffa7]=0x3f;
#endif
                attention = escape = 1;
                // we have to reload romfile
                // readimage();
                goto restart;
                break;
        default:  // one step trace
                trskip = 1;
                bpskip = 0;
                attention = escape = 1;
        }
        if (tracing||breakpoint||trskip||bpskip||stkskip) { attention = escape = 1; }
        else attention = 0;
        set_term(escchar);
}

/*
 *     keep break point / watch point in a list
 */
void setbreak(int adr, int count) {
  BPTR bp = calloc(1,sizeof(BP));
  bp->count = count;
  bp->laddr = adr;
  bp->address = paddr(adr,mmu);
#ifdef USE_MMU
  if (bp->address >= memsize) { free(bp); return; }
  bp->watch = *mem0(phymem,adr,mmu);
#else
  bp->watch = mem[adr];
#endif
  bp->next = breakpoint;
  breakpoint = bp;
}

/*
 * length of call instruction
 *
 * if next instruction is call or swi, put temporary break after the call instruction
 *   (ignoring page boundary, sorry)
 */
int nexti(void) {
#ifdef USE_MMU
    int op1 = *mem0(phymem,pcreg,mmu);
    int op2 = *mem0(phymem,pcreg+1,mmu);
#else
    int op1 = mem[pcreg];
    int op2 = mem[pcreg+1];
#endif
    int ofs = 0;
    switch(op1) {
        case 0x17: // LBSR
        case 0xbd: // JSR extended
            ofs=3; break;
        case 0x10: // page2
            {
                if (op2==0x3f) { // os9 system call
                    ofs=3; break;
                }
            }
            break;
        case 0x11: // page3
            {
                if (op2==0x3f) { // SWI3
                    ofs=2; break;
                }
            }
            break;
        case 0x3f: // SWI
            ofs=1; break;
        case 0x3c: // CWAI
        case 0x8d: // BSR
        case 0x9d: // JSR direct
            ofs=2; break;
        case 0xad: // JSR index
            {
                if (op2<0x80) ofs = 2;   // 5bit ofs
                else switch (op2&0xf) { 
                    case 8: case 0xc:
                        ofs = 3; break;
                    case 9: case 0xd: case 0xf:
                        ofs = 4; break;
                    default:
                        ofs = 2; break;
                }
            }
            break;
    }
    if (ofs) setbreak(pcreg+ofs,-1);
    return ofs;
}

/* end */
