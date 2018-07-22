/* 6809 Simulator V09.

   Copyright 1994, L.C. Benschop, Eidnhoven The Netherlands.
   This version of the program is distributed under the terms and conditions
   of the GNU General Public License version 2. See the file COPYING.
   THERE IS NO WARRANTY ON THIS PROGRAM!!!
   
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


#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/stat.h>

#define engine extern

#include "v09.h"

FILE *tracefile;

extern FILE *disk[];
extern FILE *fp;      // for disasm
extern char *prog;    // for disasm
extern void disasm(int,int);
extern void do_mmu(Word,Byte);
extern void init_term(void) ;


void do_trace(FILE *tracefile)
{
 Word pc=pcreg;
 Byte ir;
 // fprintf(tracefile,"pc=%04x ",pc);
 // ir=mem[pc++];
 // fprintf(tracefile,"i=%02x ",ir); if((ir&0xfe)==0x10) fprintf(tracefile,"%02x ",mem[pc]);else 
 // fprintf(tracefile,"   ");
 fprintf(tracefile,"x=%04x y=%04x u=%04x s=%04x a=%02x b=%02x cc=%02x pc=",
                   xreg,yreg,ureg,sreg,*areg,*breg,ccreg);
 fp = tracefile;
 disasm(pc,pc);
} 

char *romfile = "v09.rom";
long romstart = 0x8000;

long
filesize(FILE *image)
{
    struct stat buf;
    fstat(fileno(image),&buf);
    return buf.st_size;
}


void 
read_image()
{
 FILE *image;
 if((image=fopen(romfile,"rb"))==NULL) 
  if((image=fopen("../v09.rom","rb"))==NULL) 
   if((image=fopen("..\\v09.rom","rb"))==NULL) {
    perror("v09, image file");
    exit(2);
 }
 long len = filesize(image);
 /*
  * 
  *    0x0000-0xdfff    normal mem
  *    0xxxxx-0xdfff    rom
  *    0xe000-0xe100    i/o
  *    0xe000-0xffff    rom
  *
  * discless boot
  *    rom image will be copyied from 0xed00-0x1xxxx
  *    boot copies 0x10000-0x1xxxx to os9's boot memory 
  */
#ifdef USE_MMU
 /*
  * In case of Coco, there is no ROM (switched out after boot )
  *    0x00000-0x0fdff    normal mem
  *    0x0fe00-0x0ffff    ram fixed address ram including io
  *    0x10000-0x7ffff    ram (512Kb memory current implementation)
  * it should have 2MB memory
  *    0x10000-0xfffff    ram
  *  >0x100000            lapround
  *
  * discless boot
  *    rom image will be copyied from 0xed00-0x1xxxx
  *    boot copies 0x10000-0x1xxxx to os9's boot memory 
  */
 phymem = malloc(memsize + len - 0x2000);
 rommemsize = memsize + len - 0x2000;
 mem    = phymem + memsize - 0x10000 ;
 mmu = &mem[0xffa0];
 prog = (char*)mem;
 if (romstart==0x8000) {
     // romstart = memsize - 0x10000 + 0xed00 ;
     romstart = memsize ;  // full 512kb mem
 }
 fread(mem+ 0xe000,len,1,image);
 mem[0xffa7] = 0x3f;
#else
 if (romstart==0x8000) {
     romstart = 0x10000 - len; 
 }
 fread(mem+(romstart&0xffff),len,1,image);
#endif
 fclose(image);
}

void usage(void)
{
 fprintf(stderr,"Usage: v09 [-rom rom-image] [-l romstart] [-t tracefile [-tl addr] [-nt]"
                "[-th addr] ]\n[-e escchar] \n");
 exit(1); 
}


#define CHECKARG if(i==argc)usage();else i++;

int
main(int argc,char *argv[])
{
 char *imagename=0;
 int i;
 int setterm = 1;
 memsize = 512*1024;
 escchar='\x1d'; 
 tracelo=0;tracehi=0xffff;
 for(i=1;i<argc;i++) {
    if (strcmp(argv[i],"-t")==0) {
     i++;
     if((tracefile=fopen(argv[i],"w"))==NULL) {
         perror("v09, tracefile");
         exit(2);
     }
     tracing=1;attention=1;    
   } else if (strcmp(argv[i],"-rom")==0) {
     i++;
     timer = 0;  // non standard rom image, don't start timer
     romfile = argv[i];

   } else if (strcmp(argv[i],"-0")==0) {
      i++;
      disk[0] = fopen(argv[i],"r+");
   } else if (strcmp(argv[i],"-1")==0) {
      i++;
      disk[1] = fopen(argv[i],"r+");
   } else if (strcmp(argv[i],"-tl")==0) {
     i++;
     tracelo=strtol(argv[i],(char**)0,0);
   } else if (strcmp(argv[i],"-th")==0) {
     i++;
     tracehi=strtol(argv[i],(char**)0,0);
   } else if (strcmp(argv[i],"-e")==0) {
     i++;
     escchar=strtol(argv[i],(char**)0,0);
   } else if (strcmp(argv[i],"-l")==0) {
     i++;
     romstart=strtol(argv[i],(char**)0,0);
   } else if (strcmp(argv[i],"-nt")==0) {  // start debugger at the start
     attention = escape = 1;
     timer = 0;   // no timer
   } else if (strcmp(argv[i],"-m")==0) {
     i++;
     memsize=strtol(argv[i],(char**)0,0) & ~0xffff;
     if (memsize < 512*1024) memsize = 512*1024;
   } else usage();
 }   
 #ifdef MSDOS
 if((mem=farmalloc(65535))==0) { 
   fprintf(stderr,"Not enough memory\n");
   exit(2);
 } 
 #endif
 read_image(); 
 init_term();
 if (setterm) set_term(escchar);
 pcreg=(mem[0xfffe]<<8)+mem[0xffff]; 
 prog = (char*)mem;  // for disasm
 interpr();
 return 0;
}

