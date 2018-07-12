/* 6808 Simulator V092
 *
 * tracer

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
  struct bp *next;
} BP, *BPTR;

BPTR breakpoint = 0;
int bpskip = 0;
int trskip = 0;
int stkskip = 0;

int getarg(char *buf, char** next) {
        return strtol(buf,(char**)next,0);
}

void printhelp(void)
{
  printf( 
     "  s [count]  one step trace\n"
     "  n          step over\n"
     "  f          finish this call (until stack pop)\n"
     "  b [adr]    set break point (on current physical addreaa)\n"
     "  B          break point list\n"
     "  d [n]      delte break point list\n"
     "  c  [count] continue;\n"
     "  x  [adr] [count]  dump\n"
#ifdef USE_MMU
     "  xp page [adr]   dump physical memory\n"
#endif
     "  xi [adr] [count]  disassemble\n"
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
        if (bpskip) { // skip unbreak instruction
            bpskip--;
            int ppc = paddr(pcreg,mmu);
            BPTR *prev = &breakpoint;
            for(BPTR b = breakpoint; b ; prev=&b->next, b=b->next ) {
                if (ppc==b->address /* || pcreg==b->laddr */) {
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
           if (sreg < stkskip ) return;
        }
restart0:
        stkskip = 0;
        restore_term();
#ifdef USE_MMU
        Byte *phyadr = mem0(phymem,pcreg,mmu);
        prog = (char*)phyadr - pcreg;
#endif
        do_trace(stdout);
        if (trskip>1) { // show trace and step
            trskip--;
            set_term(escchar);
            return; 
        }
restart:
        printf("v09>");
        fgets(s, sizeof(s)-1, stdin); 
        s[strlen(s)-1] = 0; // chop
        switch (s[0]) {
        case 'n':   // step over
                if (nexti()) {
                   bpskip = -1;
                   break;
                } 
        default:
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
                    printf("%x %x %d\n", bp->laddr, bp->address, bp->count);
#else
                    printf("%x %d\n", bp->address, bp->count);
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
        case 'x':   // dump 
           {    char *next = s+1;
                if (s[1]=='i') next=s+2;
                else if (s[1]=='p') { 
                   next = s+2;
                   if (next[0]) {
                      page =  getarg(next,&next);
                   }
                }
                if (next[0]) {
                   int adr = getarg(next,&next);
                   int len = 32;
                   if (next[0]) {
                      len =  getarg(next,&next);
                   }
                   if (s[1]=='i') {
                     Word end = adr + len;
                     while(adr < end) {
#ifdef USE_MMU
                        Byte *phyadr = mem0(phymem,adr,mmu);
                        prog = (char*)phyadr - adr  ;
                        if (phyadr > phymem+memsize) goto restart;
#endif
                        int len = adr+16<end? 16 : end-adr -1 ;
                        adr = disasm(adr,adr+len);
                      }
                   } else {
#ifdef USE_MMU
                     for(int i=0; len > 0 ; i+=16, len-=16) {
                        if (s[1]=='p') {
                            int phy = page * 0x2000 + adr + i;
                            if (phy > rommemsize) goto restart;
                            hexadump(phymem+phy,len>16?16:len,adr+i,16);
                        } else {
                            Byte *phyadr = mem0(phymem,adr+i,mmu);
                            if (phyadr > phymem+rommemsize) goto restart;
                            hexadump(phyadr,len>16?16:len,adr+i,16);
                        }
                     }
#else
                     for(int i=0; len > 0 ; i+=16, len-=16) {
                        hexadump(mem+adr+i,len>16?16:len,adr+i,16);
                     }
#endif
                   }
                } else 
                   disasm(pcreg,pcreg+32);
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
                break;
        case 'S':
                if (infile)
                        fclose(infile);
                infile = 0;
                if (s[1]) {
                        int i=1; while(s[i]==' ') i++;
                        infile = fopen(s + i, "r");
                }
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
                break;
        case 'R':
                pcreg = (mem[0xfffe] << 8) + mem[0xffff];
                bpskip = 0;
#ifdef USE_MMU
                mmu = &mem[0xffa0];
                mem[0xffa7]=0x3f;
#endif
                attention = escape = 1;
                break;
        }
        if (tracing||breakpoint||trskip||bpskip||stkskip) { attention = escape = 1; }
        else attention = 0;
        set_term(escchar);
}

void setbreak(int adr, int count) {
  BPTR bp = calloc(1,sizeof(BP));
  bp->next = breakpoint;
  breakpoint = bp;
  bp->count = count;
  bp->laddr = adr;
  bp->address = paddr(adr,mmu);
}

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








