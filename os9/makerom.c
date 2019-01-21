/* makerom.c 
   build ROM image file os9.rom from module list

         Shinji KONO    Fri Jul  6 13:31:52 JST 2018

*/

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <sys/stat.h>

// #define DEBUG 1

/*
 * Level1
 *        os9p1 should be 0xf800
 *        it searches ram from the beginning
 *        rom modules are searched from just after the end of RAM
 *
 * Level2
 *        Coco 512kb memory space
 *        last 8k is a ROM (can be switched?) ( block 0x3f )
 *        os9p1 search module on 0x0d00~0x1e00 at block 0x3f
 *
 *        8k block ( offset 0xc000 )
 *
 *        0xe000 - 0xccff   0xff
 *        0xed00 - 0xfeff   os9 modules,  os9p1 should be the last
 *                 MMU doesnot touch below
 *        0xff80 - 0xffdf   IO port  ( ACIA, clock, pdisk, MMU )
 *        0xffd0 - 0xffef   boot code
 *        0xfff0 - 0xffff   intr vector
 *         ... next few blocks as extended ROM
 *         lv2 6809 memory check routine destroys 0x200 on page 0x40
 *                  sta     >-$6000,x
 *         avoid 0x200
 *
 */

int level = 1;
int IOBASE = 0xe000;
int IOSIZE = 0x100;
char * outfile ;

#define LV2START  0xffd0     // our own small boot for mmu
#define LV2ROMEND 0xff80

// #define DEBUG

typedef struct os9module {
   int size;
   int entry;
   int location;
   int ioflag;
   unsigned char *mod;
   char *name;
   struct os9module *next;
} *MPTR ; 

unsigned short vec[8];

struct os9module *
readOS9module(char *filename) 
{
  FILE *fp = fopen(filename,"rb");
  if (fp==0) {
    fprintf(stderr,"cannot read %s\n",filename);
    exit(1);
  }
  struct stat st;
  fstat(fileno(fp),&st);
  int size = st.st_size;
  struct os9module *m = malloc(size + sizeof(struct os9module));
  m->size = size;
  m->next = 0;
  m->ioflag = 0;
  m->mod = (unsigned char*)m + sizeof(struct os9module);
  fread(m->mod , size, 1, fp);
  m->name = (char*) (m->mod + (m->mod[4]*256 + m->mod[5]) );
  m->entry = m->mod[9]*256 + m->mod[10] ;
  fclose(fp);
  return m;
}

void
fputword(unsigned short x, FILE *fp)
{
   fputc((x>>8)&0xff,fp);
   fputc(x&0xff,fp);
}

void printOs9Str(char *p)
{
   char *q = p;
   while((*p & 0x80)==0) {
      putchar(*p);
      p++;
   }
   putchar(*p & 0x7f);
   while(p<q+8) {
       putchar(' '); p++;
   }
}

unsigned short
getword(unsigned char *ptr)
{
    return (ptr[0]<<8)+ptr[1];
}

void rewrite_vector(MPTR m,int size, unsigned char *adr,int count)
{
    // vector is a offset from $F100 (possibly os9p1 module address)
    int offset = -size-0xf100;
    for(int i=0;i<count;i++) {
        int vec = getword(adr);
        vec += offset;
        adr[0] = vec>>8;
        adr[1] = vec&0xff;
        adr += 2;
    }
}

int search_vector(MPTR m) {
    unsigned char v[] = { 0x6E, 0x9F, 0x00, 0x2C, 0x6E};
    for( unsigned char *p = m->mod ; p < m->mod + m->size; p++ ) {
        int i=0;
        for(; i< sizeof(v); i++) {
            if (p[i]!=v[i]) break;
        }
        if (i==sizeof(v)) 
            return p - m->mod;
    }
    return 0;
}

// calcurate position from the botton
// avoid v09 IO map on 0xe000-0xe800
// os9p1 have to be last and at 0xf800
int findLocation(MPTR m, int loc) {
   if (m==0) return loc;
   int top = findLocation(m->next, loc) - m->size;
   if (m->next==0) {
       if (level == 1)
          if (m->size > 0xff80-0xf800 ) {
              top = 0x10000-(m->size+0x80);
          } else {
              top = 0xf800;  // OS9p1
          }
       else {
#if 0
          // old level2 kernel has vector at the bottom
          top = 0x10000-(m->size+0x80);
          rewrite_vector(m,m->size,m->mod+getword(m->mod+2),7);
#else
          top = 0xf000;  // level2 OS9p1 starts here
          // and theses area are RAM /REGISTER STACK/
#endif
       }
   }
   if (level==1 && !(( top+m->size < IOBASE )  || ( IOBASE+IOSIZE < top)) ) {
      top = IOBASE-m->size-1;
      m->ioflag = 1;
#ifdef DEBUG
      printf("*");
#endif 
   } else if (level==2 &&  0xed00 > top) {
      m->ioflag = 1;
   }
   m->location = top;
#ifdef DEBUG
    printf("mod ");
    printOs9Str(m->name);
    printf(" \t: 0x%x - 0x%x\n",top, top + m->size);
#endif 
   return top;
}

int
main(int ac, char *av[])
{
 int vectable = 0;
 struct os9module *m = 0, root ;
 root.size = 0;
 root.mod = 0;
 m = &root;

 for(int i = 1 ; i<ac ; i++ ) {
    if (*av[i]=='-') {
        if (av[i][1] =='2') {  // for level 2
            level = 2;
            IOBASE = LV2ROMEND;
        } else if (av[i][1] =='o') {
            outfile = av[i+1];
            i += 1;
        } else {
            return 1;
        }
        continue;
    }
    struct os9module *cur;
    cur = readOS9module(av[i]);
    m->next = cur;
    m = cur;
 }

 FILE *romfile;
 unsigned pos;
 if (outfile==0) return 1;

 romfile=fopen(outfile,"wb");
 if(!romfile) {
  fprintf(stderr,"Cannot create file %s\n",av[1]);
  exit(1);
 }

 
 int start = findLocation(root.next,0);
 start = start&0xf800;
 printf("\n\n");

 if (level==2) {
     for(int i=0; i<0xd00; i++) fputc(0xff,romfile);
     pos = 0xed00;
 } else {
     pos = start;
 }
 int ofs = 0;
 struct os9module *os9p1 = 0;
 for(struct os9module *cur = root.next; cur ; cur = cur->next ) {
    if ( level==2 && cur->ioflag ==1) continue; 
    // last module have to os9p1
    if ( cur->next == 0 ) {
        os9p1 = cur;
        if ( level==1 ) { 
           if (os9p1->size > 0x07f0) {
               ofs = (os9p1->size+0xf)&0xfff0;
               ofs -= 0x07f0;
           }
           for(; pos < 0xf800-ofs ; pos++) {   // os9p1 begins at 0xf800
              fputc(0xff,romfile);
           }
        } else {
#if 0
           int pend = 0x10000-( cur->size +0x80);
           for(; pos < pend ; pos++) {      // os9p1 ends 0xff7f
              fputc(0xff,romfile);
           }
#endif
           for(; pos < 0xf000 ; pos++) {   // level2 os9p1 start from 0xf000
              fputc(0xff,romfile);
           }
        }
    }
    printf("mod ");
    printOs9Str(cur->name);
    cur->location = pos;
    fwrite(cur->mod, cur->size, 1, romfile);
    printf(" \t: 0x%x - 0x%x size 0x%04x entry 0x%x\n",pos, pos + cur->size-1,cur->size,cur->entry+cur->location);
#ifdef DEBUG
    printf(" \t: 0x%x \n",cur->location);
    printf(" \t: 0x%x - 0x%x : 0x%lx \n",pos, pos + cur->size, ftell(romfile)+start);
#endif 
    pos = pos+cur->size;
    if (level==1 && cur->ioflag) {
       if (level==1) {
           for(; pos < IOBASE + IOSIZE; pos++) {
              fputc(0xff,romfile);
           }
           printf("*");
       } 
    } 
 }
 printf("os9 end %x\n",pos);
 if (level==1) {
     vectable  = 0x10000 - 2*7;
     for( ; pos<vectable; pos++) fputc(0xff,romfile);
     printf("vectbl %x\n",pos);
     if (1) {
         int vecofs = search_vector(os9p1);
         if (vecofs==0) {
            printf("can't find vector\n");
         }
         static int perm[] = {0,1,5,4,2,3};
         for(int i=0;i<6;i++) {
            fputword(os9p1->location +vecofs+perm[i]*4,romfile);
         }  
         int entry_ofs = (m->mod[9]<<8) + m->mod[10];  
         fputword( os9p1->location + entry_ofs ,romfile);  
         // printf("os9p1 location ofs %0x\n", os9p1->location);
         // printf("vector ofs %0x\n", vecofs);
         // printf("reset ofs %0x\n", entry_ofs);
     } else {  
        fputword(0xF82d-ofs,romfile);
        fputword(0xF831-ofs,romfile);
        fputword(0xF835-ofs,romfile);
        fputword(0xF839-ofs,romfile);
        fputword(0xF83d-ofs,romfile);
        fputword(0xF841-ofs,romfile);
        fputword(0xF876-ofs,romfile);
     }
 } else {
     char vector[] = "level2/vector";
     FILE *fp = fopen(vector,"rb");
     if (fp==0) {
        fprintf(stderr,"cannot read %s\n",vector);
        exit(1);
     }
     for( ; pos<LV2START; pos++) fputc(0xff,romfile);
     printf("vectbl %x\n",pos);
     for( ; pos<0xfff0; pos++) fputc(fgetc(fp),romfile);
#ifdef DEBUG
     printf("os9entry %x\n",os9p1->location);
#endif 
     printf("os9entry %x\n",os9p1->location+getword(os9p1->mod+9));

     fputword(os9p1->location+getword(os9p1->mod+9),romfile);     // os9p1 entry point 
     unsigned short vec = os9p1->location+os9p1->size - 18;
     fputword(vec,romfile);
     fputword(vec+3,romfile);
     fputword(vec+6,romfile);

     fputword(vec+9,romfile);
     fputword(vec+12,romfile);
     fputword(vec+15,romfile);
     fputword(LV2START,romfile);

     pos = 0x10000;
     int bootsize = 2;
     for(struct os9module *cur = root.next; cur ; cur = cur->next ) {
        if ( cur->ioflag ==0) continue; 
        bootsize += cur->size;
     }
     bootsize += 0x300-2;    // to avoid 0x200 bombing
     fputc(bootsize>>8,romfile);
     fputc(bootsize&0xff,romfile);
     pos += 2;
     for( int i = 0; i<0x300-2; i++) fputc(0xff,romfile);
     for(struct os9module *cur = root.next; cur ; cur = cur->next ) {
        if ( cur->ioflag ==0) continue; 
        cur->location = pos;
        printf("mod ");
        printOs9Str(cur->name);
        fwrite(cur->mod, cur->size, 1, romfile);
        printf(" \t: 0x%x - 0x%x size 0x%04x entry 0x%x\n",pos, pos + cur->size-1, cur->size, cur->entry+cur->location);
#ifdef DEBUG
        printf(" \t: 0x%x \n",cur->location);
        printf(" \t: 0x%x - 0x%x : 0x%lx \n",pos, pos + cur->size, ftell(romfile)+start);
#endif 
        pos += cur->size;
     }
     while(pos++ & 0xff) fputc(0xff,romfile);
 }
 if (level==1) 
     printf("boot rom from 0x%lx\n",0x10000-ftell(romfile)); 
 else {
     long size;
     printf("boot rom from 0xc000 size 0x%lx\n",(size=ftell(romfile))); 
     if (size > 0x4d00 + 0x2000) {
         printf(" was too big. make it less than 0x6d00\n");
     } 
 }
 fclose(romfile);
 return 0;
}


