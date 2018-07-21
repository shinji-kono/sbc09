/* 6808 Simulator V092
 created 1993,1994 by L.C. Benschop. copyleft (c) 1994-2014
by the sbc09 team, see AUTHORS for more details. license: GNU
General Public License version 2, see LICENSE for more details.

   This program simulates a 6809 processor.

   System dependencies: short must be 16 bits.
   char  must be 8 bits.
   long must be more than 16 bits.
   arrays up to 65536 bytes must be supported.
   machine must be twos complement.
   Most Unix machines will work. For MSODS you need long pointers
   and you may have to malloc() the mem array of 65536 bytes.

   Define BIG_ENDIAN if you have a big-endian machine (680x0 etc)

   Special instructions:
   SWI2 writes char to stdout from register B.
   SWI3 reads char from stdout to register B, sets carry at EOF.
   (or when no key available when using term control).
   SWI retains its normal function.
   CWAI and SYNC stop simulator.

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

/*
 *   IO Map ( can be overrupped by ROM )
 *
 *   In do_input/do_output call, we cannot access 6809 registers, since it is in i*reg, 
 *     which  is a local variable of interpr
 *
 *   IOPAGE ~ IOPAGE+0x7f
 *       for OS9 level2
 *       IOPAGE 0xff80 means ioport beging 0xff80 but IOPAGE itself starts 0xff00
 *       0xfe00-0xff7f, 0xffe0-0xffff can be used as RAM in fixed area in level2
 *       and these are ROM in level1
 *       
 *
 *   IOPAGE + 0x00   ACIA  control
 *   IOPAGE + 0x01   ACIA  data
 *
 *   IOPAGE + 0x11   MMU Taskreg         0  system map, 1 user map
 *   IOPAGE + 0x20-0x27   MMU reg        system map
 *   IOPAGE + 0x28-0x2f   MMU reg        user   map
 *
 *        on reset tr==0 and only IOPAGE is valid
 *        translatation occur only on non-IOPAGE
 *        mem == phymem + 0x70000
 *        phy addr = phymem[ ( mmu[ adr >> 13 ] <<13 ) + (adr & 0x1fff ) ]
 *            tr=0  mmu=IOPAGE+0xa0
 *            tr=1  mmu=IOPAGE+0xa8
 *
 *   IOPAGE + 0x30   Timer control       0x8f start timer/0x80 stop timer/0x04 update date
 *                                       read 0x10 bit menas timer
 *   IOPAGE + 0x31-  YY/MM/DD/HH/MM/SS
 *
 *   IOPAGE + 0x40   Disk control        0x81 read/0x55 write   0 ... ok / 0xff .. error
 *                                       0xd1- VDISK command
 *   IOPAGE + 0x41   drive no           / VDISK drv
 *   IOPAGE + 0x42   LSN2               / VDISK sysmode  0 for system, 1 for user 
 *   IOPAGE + 0x43   LSN1
 *   IOPAGE + 0x44   LSN0               / VDISK Curdir pd number 
 *   IOPAGE + 0x45   ADR2               / VDISK caller stack
 *   IOPAGE + 0x46   ADR1
 *   IOPAGE + 0x47                      / VDISK path descriptor address (Y)
 *   IOPAGE + 0x48   
 *
 *
 */

#define SECSIZE 256


int timer = 1;
struct termios termsetting;
struct termios newterm;
struct itimerval timercontrol;

int tflags;
int xmstat; /* 0= no XMODEM transfer, 1=send, 2=receiver */
unsigned char xmbuf[132];
int xidx;
int acknak;
int rcvdnak;
int blocknum;
int timer_irq = 2 ; // 2 = FIRQ, 1 = IRQ

FILE *infile;
FILE *xfile;
FILE *logfile;
FILE *disk[] = {0,0};

#ifdef USE_VDISK
extern void do_vdisk(int c);
#endif


#ifdef USE_MMU
extern char *prog ;   // for disass
extern Byte * mem0(Byte *iphymem, Word adr, Byte *immu) ;
#define pmem(a)  mem0(phymem,a,mmu)
#else
#define pmem(a)  (&mem[a])
#endif


extern int bpskip ;
extern int stkskip ;
extern FILE *logfile;

void do_timer(int,int);
void do_disk(int,int);
void do_mmu(int,int);

int char_input(void) {
        int c, w, sum;
        if (!xmstat) {
                if (infile) {
                        c = getc(infile);
                        if (c == EOF) {
                                fclose(infile);
                                infile = 0;
                                return char_input();
                        }
                        if (c == '\n')
                                c = '\r';
                        return c;
                } else {
                        usleep(100);
                        return getchar();
                }
        } else if (xmstat == 1) {
                if (xidx) {
                        c = xmbuf[xidx++];
                        if (xidx == 132) {
                                xidx = 0;
                                rcvdnak = EOF;
                                acknak = 6;
                        }
                } else {
                        if ((acknak == 21 && rcvdnak == 21) || (acknak == 6 && rcvdnak == 6)) {
                                rcvdnak = 0;
                                memset(xmbuf, 0, 132);
                                w = fread(xmbuf + 3, 1, 128, xfile);
                                if (w) {
                                        printf("Block %3d transmitted, ", blocknum);
                                        xmbuf[0] = 1;
                                        xmbuf[1] = blocknum;
                                        xmbuf[2] = 255 - blocknum;
                                        blocknum = (blocknum + 1) & 255;
                                        sum = 0;
                                        for (w = 3; w < 131; w++)
                                                sum = (sum + xmbuf[w]) & 255;
                                        xmbuf[131] = sum;
                                        acknak = 6;
                                        c = 1;
                                        xidx = 1;
                                } else {
                                        printf("EOT transmitted, ");
                                        acknak = 4;
                                        c = 4;
                                }
                        } else if (rcvdnak == 21) {
                                rcvdnak = 0;
                                printf("Block %3d retransmitted, ", xmbuf[1]);
                                c = xmbuf[xidx++]; /*retransmit the same block */
                        } else
                                c = EOF;
                }
                return c;
        } else {
                if (acknak == 4) {
                        c = 6;
                        acknak = 0;
                        fclose(xfile);
                        xfile = 0;
                        xmstat = 0;
                } else if (acknak) {
                        c = acknak;
                        acknak = 0;
                } else
                        c = EOF;
                if (c == 6)
                        printf("ACK\n");
                if (c == 21)
                        printf("NAK\n");
                return c;
        }
}

int do_input(int a) {
        static int c, f = EOF;
        if (a == 0+(IOPAGE&0x1ff)) {
                if (f == EOF)
                        f = char_input();
                if (f != EOF) {
                        c = f;
                        mem[(IOPAGE&0xfe00) + a] = c;
                }
                mem[(IOPAGE&0xfe00) + a] = c = 2 + (f != EOF);
                return c;
        } else if (a == 1+(IOPAGE&0x1ff)) { /*data port*/
                if (f == EOF)
                        f = char_input();
                if (f != EOF) {
                        c = f;
                        f = EOF;
                        mem[(IOPAGE&0xfe00) + a] = c;
                }
                return c;
        }
        return mem[(IOPAGE&0xfe00) + a];
}

void do_output(int a, int c) {
        int i, sum;
        if (a == 1+(IOPAGE&0x1ff)) { /* ACIA data port,ignore address */
                if (!xmstat) {
                        if (logfile && c != 127 && (c >= ' ' || c == '\n'))
                                putc(c, logfile);
                        putchar(c);
                        fflush(stdout);
                } else if (xmstat == 1) {
                        rcvdnak = c;
                        if (c == 6 && acknak == 4) {
                                fclose(xfile);
                                xfile = 0;
                                xmstat = 0;
                        }
                        if (c == 6)
                                printf("ACK\n");
                        if (c == 21)
                                printf("NAK\n");
                        if (c == 24) {
                                printf("CAN\n");
                                fclose(xfile);
                                xmstat = 0;
                                xfile = 0;
                        }
                } else {
                        if (xidx == 0 && c == 4) {
                                acknak = 4;
                                printf("EOT received, ");
                        }
                        xmbuf[xidx++] = c;
                        if (xidx == 132) {
                                sum = 0;
                                for (i = 3; i < 131; i++)
                                        sum = (sum + xmbuf[i]) & 255;
                                if (xmbuf[0] == 1 && xmbuf[1] == 255 - xmbuf[2]
                                                && sum == xmbuf[131])
                                        acknak = 6;
                                else
                                        acknak = 21;
                                printf("Block %3d received, ", xmbuf[1]);
                                if (blocknum == xmbuf[1]) {
                                        blocknum = (blocknum + 1) & 255;
                                        fwrite(xmbuf + 3, 1, 128, xfile);
                                }
                                xidx = 0;
                        }
                }
        } else if (a >= 0x40+(IOPAGE&0x1ff)) { /* disk */
             do_disk(a,c);
        } else if (a >= 0x30+(IOPAGE&0x1ff)) { /* timer */
             do_timer(a,c);
        } else if (a >= 0x10+(IOPAGE&0x1ff)) { /* mmu */
             do_mmu(a,c);
#ifdef USE_MMU
        } else { /* fixed ram */
             mem[ a + 0xfe00 ] = c;
#endif
        }
}


void do_timer(int a, int c) {
   struct itimerval timercontrol;
   if (a==0x30+(IOPAGE&0x1ff) && c==0x8f) {
        timercontrol.it_interval.tv_sec = 0;
        timercontrol.it_interval.tv_usec = 20000;
        timercontrol.it_value.tv_sec = 0;
        timercontrol.it_value.tv_usec = 20000;
        timer_irq = 1;
        setitimer(ITIMER_REAL, &timercontrol, NULL);
        mem[(IOPAGE&0xfe00)+a]=c;
   } else if (a==0x30+(IOPAGE&0x1ff) && c==0x80) {
        timercontrol.it_interval.tv_sec = 0;
        timercontrol.it_interval.tv_usec = 0;
        setitimer(ITIMER_REAL, &timercontrol, NULL);
        mem[(IOPAGE&0xfe00)+a]=c;
   } else if (a==0x30+(IOPAGE&0x1ff) && c==0x04) {
      time_t tm = time(0);
      struct tm *t = localtime(&tm);
      mem[IOPAGE+0x31] = t->tm_year;
      mem[IOPAGE+0x32] = t->tm_mon;
      mem[IOPAGE+0x33] = t->tm_mday;
      mem[IOPAGE+0x34] = t->tm_hour;
      mem[IOPAGE+0x35] = t->tm_min;
      mem[IOPAGE+0x36] = t->tm_sec;
   } else {
      mem[(IOPAGE&0xfe00)+a]=c;
   }
}


void do_disk(int a, int c) {
   if (a!=0x40+(IOPAGE&0x1ff)) {
      mem[(IOPAGE&0xfe00)+a]=c;
      return;
   }
   int drv = mem[IOPAGE+0x41];
   int lsn = (mem[IOPAGE+0x42]<<16) + (mem[IOPAGE+0x43]<<8) + mem[IOPAGE+0x44];
   int buf = (mem[IOPAGE+0x45]<<8) + mem[IOPAGE+0x46];
   if (drv > 1 || disk[drv]==0) goto error;
   Byte *phy = pmem(buf);
   if (c==0x81) {
      if (lseek(fileno(disk[drv]),lsn*SECSIZE,SEEK_SET)==-1) goto error;
      if (read(fileno(disk[drv]),phy,SECSIZE)==-1) goto error;
   } else if (c==0x55) {
      if (lseek(fileno(disk[drv]),lsn*SECSIZE,SEEK_SET)==-1) goto error;
      if (write(fileno(disk[drv]),phy,SECSIZE)==-1) goto error;
#ifdef USE_VDISK
   } else  {
       do_vdisk(c);
       return;
#endif
   }
   mem[IOPAGE+0x40] = 0;
   return;
error :
   mem[IOPAGE+0x40] = 0xff;
}

void do_mmu(int a, int c)
{
#ifdef USE_MMU

   if (a==0x11+(IOPAGE&0x1ff)) {
       if (c&1) {
           mmu = &mem[0xffa8];
       } else {
           mmu = &mem[0xffa0];
       }
   }
   mem[(IOPAGE&0xfe00)+a] = c;   // other register such as 0xffa0-0xffaf
#endif 
}

void timehandler(int sig) {
        attention = 1;
        irq = 2;
        mem[(IOPAGE&0xfe00)+0x30] |= 0x10  ;
        // signal(SIGALRM, timehandler);
}

void handler(int sig) {
        escape = 1;
        attention = 1;
        bpskip = 0;
        stkskip = 0;
}

void init_term(void) {
        tcgetattr(0, &termsetting);
        tflags = fcntl(0, F_GETFL, 0);
}

void set_term(char c) {
        signal(SIGQUIT, SIG_IGN);
        signal(SIGTSTP, SIG_IGN);
        signal(SIGINT, handler);
        signal(SIGUSR1, handler);
        newterm = termsetting;
        newterm.c_iflag = newterm.c_iflag & ~INLCR & ~ICRNL;
        newterm.c_lflag = newterm.c_lflag & ~ECHO & ~ICANON;
        newterm.c_cc[VTIME] = 0;
        newterm.c_cc[VMIN] = 1;
        newterm.c_cc[VINTR] = escchar;
        tcsetattr(0, TCSAFLUSH, &newterm);
        fcntl(0, F_SETFL, tflags | O_NDELAY); /* Make input from stdin non-blocking */
        signal(SIGALRM, timehandler);
        timercontrol.it_interval.tv_sec = 0;
        timercontrol.it_interval.tv_usec = 20000;
        timercontrol.it_value.tv_sec = 0;
        timercontrol.it_value.tv_usec = 20000;
        if (timer)
            setitimer(ITIMER_REAL, &timercontrol, NULL);
}

void restore_term(void) {
        termsetting.c_iflag = termsetting.c_iflag | INLCR | ICRNL;
        tcsetattr(0, TCSAFLUSH, &termsetting);
        fcntl(0, F_SETFL, tflags);
        signal(SIGALRM, SIG_IGN);
}




