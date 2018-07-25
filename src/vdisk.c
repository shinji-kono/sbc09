/********************************************************************
* Virtual RBF - Random Block File Manager
*
*         Shinji KONO  (kono@ie.u-ryukyu.ac.jp)  2018/7/17
*         GPL v1 license
*/

#define engine extern
#include "v09.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <dirent.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>
#include <fcntl.h>

static int vdiskdebug = 0;  //   bit 1 trace, bit 2 filename


Byte pmmu[8];  // process dat mmu

extern char *prog ;   // for disass
#ifdef USE_MMU
extern Byte * mem0(Byte *iphymem, Word adr, Byte *immu) ;
//  smem physical address using system mmu
//  pmem physical address using caller's mmu
#define smem(a)  mem0(phymem,a,&mem[0x20+IOPAGE])
#define pmem(a)  mem0(phymem,a,pmmu)
#else
#define smem(a)  (&mem[a])
#define pmem(a)  (&mem[a])
#endif

#define MAXPDV 256

/*
 * os9 has one path descriptor for one open file or directory
 * keep coresponding information in vdisk file manager
 */

typedef struct pathDesc {
    char *name;  // path name relative to drvRoot
    FILE *fp;    // file , memfile for directory
    int mode;
    int inode ;  // lower 24 bit of unix inode, os9 lsn
    int num ;
    int sz ;     // used only for directory
    char drv ;
    char use ;
    char dir;    // is directory?
    char *fd ;   //  simulated os9 file descriptor ( not used now)
    char *dirfp; //  simulated os9 directory file
} PathDesc, *PathDescPtr;

static void 
vdisklog(Word u,PathDesc *pd, Word pdptr, int curdir,FILE *fp) ;

#define MAXVDRV 4

static char *drvRoot[] = { ".",".",".","."};
static PathDesc pdv[MAXPDV];

/*
 * byte order staff
 */
static inline Word 
getword(Byte *adr) {
    Word *padr = (Word*)adr;
    return ntohs(*padr);
}

static inline void 
setword(Byte *adr,Word value) {
    Word *padr = (Word*)adr;
    *padr = htons(value);
}

int 
setVdisk(int drv,char *name) {
    if (drv<0 || drv>=MAXVDRV) return -1;
    drvRoot[drv] = name;
    return 0;
}

static void 
closepd(PathDesc *pd) {
    if(pd->fp) fclose(pd->fp) ;
    pd->dir = 0;
    pd->use = 0;
    pd->fp = 0;
    pd->mode = 0;
    free(pd->dirfp); pd->dirfp = 0;
    free(pd->name); pd->name = 0;
}

/*
 * keep track current directory ( most recently 256 entry )
 * too easy approach
 *
 * dir command keep old directory LSN, so we have to too
 */
char *cdt[512];
static int cdtptr = 0;

Byte
setcd(char *name) {
    int len;
    for(int i=0;i<512;i++) {
        if (cdt[i] && strcmp(name,cdt[i])==0) return i;
    }
    cdtptr &= 0x1ff;
    if (cdt[cdtptr]) free(cdt[cdtptr]);
    cdt[cdtptr] = (char*)malloc(len=strlen(name)+1);
    strcpy(cdt[cdtptr],name);
    return cdtptr++;
}

#define MAXPAHTLEN 256

/*
 * print os9 string for debug
 */
static void
putOs9str(char *s,int max) {
    if (s==0) {
        printf("(null)");
        return;
    }
    while(*s && (*s&0x7f)>=' ' && ((*s&0x80)==0) && --max !=0) {
        putchar(*s); s++;
    }
    if (*s&0x80) putchar(*s&0x7f);
}

static void
err(int error) {
    printf("err %d\n",error);
}

/*
 * add current directory name to the path
 * if name starts /v0, drvRoot will be add
 */
static char *
addCurdir(char *name, PathDesc *pd, int curdir) {
    int ns =0 ;
    char *n = name;
if(vdiskdebug&0x2) { printf("addcur \""); putOs9str(name,0); printf("\" cur \""); putOs9str( cdt[curdir],0); printf("\"\n"); }
    if (name[0]=='/') {
        name++; while(*name !='/' && *name!=0) name ++ ; // skip /d0
        while(name[ns]!=0) ns++;
    } else if (!cdt[curdir] ) return 0; // no current directory
    else ns = strlen(name);
    int ps = ns;
    char *base ; 
    if (name[0]=='/') { 
        base = drvRoot[pd->drv]; ps += strlen(drvRoot[pd->drv])+1;  name++;
    } else if (name[0]==0) { 
        base = drvRoot[pd->drv];
        char *path = (char*)malloc(strlen(base)+1);   // we'll free this, malloc it.
        int i = 0;
        for(;base[i];i++) path[i] = base[i];
        path[i]=0;
        return path;
    } else { 
        base = cdt[curdir]; ps += strlen(cdt[curdir])+2; 
    }
    char *path = (char*)malloc(ps);
    int i = 0;
    for(;base[i];i++) path[i] = base[i];
    path[i++] = '/';
    for(int j=0;i<ps;j++,i++) path[i] = name[j];
    path[i] = 0;
    if (i>ps)
        printf("overrun i=%d ps=%d\n",i,ps); // err(__LINE__);
    return path;
}

/*
 * os9 file name may contains garbage such as 8th bit on or traling space
 * fix it. and make the pointer to next path for the return value 
 *
 * pd->name will be freed, we have to malloc it
 */
static char * 
checkFileName(char *path, PathDesc *pd, int curdir) {
    char *p = path;
    char *name = path;
    int maxlen = MAXPAHTLEN;
if(vdiskdebug&2) { printf("checkf \""); putOs9str(name,0); printf("\"\n"); }
    while(*p!=0 && (*p&0x80)==0 && (*p&0x7f)>' ' && maxlen-->0) p++;
    if (maxlen==MAXPAHTLEN) return 0;
    if (*p) {  // 8th bit termination or non ascii termination
        int eighth = ((*p&0x80)!=0);
        name = (char *)malloc(p-path+1+eighth); 
        int i;
        for(i=0;i<p-path;i++) name[i] = path[i];
        if (eighth) { name[i] = path[i]&0x7f; p++ ; i++; }
        name[i] = 0;
        // skip trailing space
        while(*p==' ') p++;
    }
    char *name1 = addCurdir(name,pd,curdir);
    if (name1!=name && name1!=path) free(name);
    if (name1==0) return 0;
    pd->name = name1;
if(vdiskdebug&2) {
    printf(" remain = \"");
    char *p1 = p; int max=31;
    while(*p1 && (*p1&0x80)==0 && max-->0) { if (*p1<0x20)  printf("(0x%02x)",*p1); else  putchar(*p1); p1++; }
    if (*p1)  { if ((*p1&0x7f)<0x20)  printf("(0x%02x)",*p1); else  putchar(*p1&0x7f); }
    printf("\" checkname result \""); putOs9str(pd->name,0); printf("\"\n");
}
    return p;
}

/*
 * os9 / unix mode conversion
 */
static void 
os9setmode(Byte *os9mode,int mode) {
    char m = 0;
    if (mode&S_IFDIR) m|=0x80;
    if (mode&S_IRUSR) m|=0x01;
    if (mode&S_IWUSR) m|=0x02;
    if (mode&S_IXUSR) m|=0x04;
    if (mode&S_IROTH) m|=0x08;
    if (mode&S_IWOTH) m|=0x10;
    if (mode&S_IXOTH) m|=0x20;
    m|=0x60; // always sharable
    *os9mode = m;
}

static char * 
os9toUnixAttr(Byte mode) {
    if ((mode&0x1) && (mode&0x2)) return "r+";
    if (!(mode&0x1) && (mode&0x2)) return "w";
    if ((mode&0x1) && !(mode&0x2)) return "r";
    return "r";
}

static int 
os9mode(Byte m) {
    int mode = 0;
    if ((m&0x80)) mode|=S_IFDIR ;
    if ((m&0x01)) mode|=S_IRUSR ;
    if ((m&0x02)) mode|=S_IWUSR ;
    if ((m&0x04)) mode|=S_IXUSR ;
    if ((m&0x08)) mode|=S_IROTH ;
    if ((m&0x10)) mode|=S_IWOTH ;
    if ((m&0x20)) mode|=S_IXOTH ;
    return mode;
}


/*
 *   os9 file descriptor
 *   * File Descriptor Format
 *
 * The file descriptor is a sector that is present for every file
 * on an RBF device.  It contains attributes, modification dates,
 * and segment information on a file.
 *
 *               ORG       0
 *FD.ATT         RMB       1                   Attributes
 *FD.OWN         RMB       2                   Owner
 *FD.DAT         RMB       5                   Date last modified
 *FD.LNK         RMB       1                   Link count
 *FD.SIZ         RMB       4                   File size
 *FD.Creat       RMB       3                   File creation date (YY/MM/DD)
 *FD.SEG         EQU       .                   Beginning of segment list
 * Segment List Entry Format
               ORG       0
 * FDSL.A         RMB       3                   Segment beginning physical sector number
 * FDSL.B         RMB       2                   Segment size
 * FDSL.S         EQU       .                   Segment list entry size
 * FD.LS1         EQU       FD.SEG+((256-FD.SEG)/FDSL.S-1)*FDSL.S
 * FD.LS2         EQU       (256/FDSL.S-1)*FDSL.S
 * MINSEC         SET       16
 */

#define FD_ATT 0 
#define FD_OWN 1 
#define FD_DAT 3 
#define FD_LNK 8 
#define FD_SIZ 9 
#define FD_Creat 13 
#define FD_SEG 16 


/*
 *   os9 directory structure
 *
 *               ORG       0
 *DIR.NM         RMB       29                  File name
 *DIR.FD         RMB       3                   File descriptor physical sector number
 *DIR.SZ         EQU       .                   Directory record size
 */
#define DIR_SZ 32
#define DIR_NM 29


/* read direcotry entry 
 *
 * create simulated os9 directory structure for dir command
 * writing to the directory is not allowed
 * */
static int 
os9opendir(PathDesc *pd) {
    DIR *dir;
    struct dirent *dp;
    if (pd->dirfp) return 0; // already opened
    dir = opendir(pd->name);
    if (dir==0) return -1;
    int dircount = 0;
    while ((dp = readdir(dir)) != NULL) dircount++;   // pass 1 to determine the size
    if (dircount==0) return 0;  // should contains . and .. at least
    pd->sz = dircount*DIR_SZ;
    pd->dirfp = (char *)malloc(dircount*DIR_SZ);
    rewinddir(dir);
    int i = 0;
    while ((dp = readdir(dir)) != NULL && dircount-->=0) {
        int j = 0;
        for(j = 0; j < DIR_NM ; j++) {
            if (j< dp->d_namlen)  {
               pd->dirfp[i+j] = dp->d_name[j]&0x7f;
               if (j== dp->d_namlen-1)  
                  pd->dirfp[i+j] |= 0x80;    // os9 EOL
            } else
               pd->dirfp[i+j] = 0;
        }
        pd->dirfp[i+j] = (dp->d_ino&0xff0000)>>16;
        pd->dirfp[i+j+1] = (dp->d_ino&0xff00)>>8;
        pd->dirfp[i+j+2] = dp->d_ino&0xff;
        i += DIR_SZ;
        if (i>pd->sz) 
            return 0;
    }
    pd->fp = fmemopen(pd->dirfp,pd->sz,"r");
    return 0;
}

static void 
os9setdate(Byte *d,struct timespec * unixtime) {
    //   yymmddhhss
    struct tm r;
    localtime_r(&unixtime->tv_sec,&r);
    d[0] = r.tm_year-2048;
    d[1] = r.tm_mon + 1;
    d[2] = r.tm_mday;
    d[3] = r.tm_hour;
    d[4] = r.tm_min;
    d[5] = r.tm_sec;
}

/* read file descriptor of Path Desc 
 *    create file descriptor sector if necessary
 *    if buf!=0, copy it 
 *
 *    only dir command accesses this using undocumented getstat fdinfo command
 */
static int 
filedescriptor(Byte *buf, int len, Byte *name,int curdir) {
    int err = 0x255;
    PathDesc pd;  
    if (len<13) return -1;
    checkFileName((char*)name,&pd,curdir);
    struct stat st;
    if (stat(pd.name,&st)!=0) goto err1;
    os9setmode(buf+FD_ATT,st.st_mode);
    buf[FD_OWN]=(st.st_uid&0xff00)>>8;
    buf[FD_OWN+1]=st.st_uid&0xff;
    os9setdate(buf+ FD_DAT,&st.st_mtimespec);
    buf[FD_LNK]=st.st_nlink&0xff;
    buf[FD_SIZ+0]=(st.st_size&0xff000000)>>24;
    buf[FD_SIZ+1]=(st.st_size&0xff0000)>>16;
    buf[FD_SIZ+2]=(st.st_size&0xff00)>>8;
    buf[FD_SIZ+3]=st.st_size&0xff;
    os9setdate(buf+FD_Creat,&st.st_ctimespec);
    err = 0;
err1:
    free(pd.name);
    return err;
}

/* 
 * undocumented getstat command
 *
 * read direcotry entry for *any* file in the directory 
 *     we only returns a file descriptor only in the current opened directory
 *
 *     inode==0 should return disk id section
 *     inode==bitmap should return disk sector map for os9 free command
 */
static int 
fdinfo(Byte *buf,int len, int inode, PathDesc *pd,int curdir) {
    int i;
    for(i=0;i<MAXPDV;i++) {
        PathDesc *pd = pdv+i;
        if (!pd->use || !pd->dir) continue;
        //  find inode in directory
        Byte *dir = (Byte*)pd->dirfp;
        Byte *end = (Byte*)pd->dirfp + pd->sz;
        while( dir < end ) {
            Byte *p = (dir + DIR_NM);
            int dinode = (p[0]<<16)+(p[1]<<8)+p[2];
            if (inode == dinode) {
                return filedescriptor(buf,len,dir,curdir);
            }
            dir += 0x20;
        }
    }
    return 255;
}

/*
 *  on os9 level 2, user process may on different memory map
 *  get DAT table on process descriptor on 0x50 in system page
 */
void
getDAT() {
#ifdef USE_MMU
    Word ps = getword(smem(0x50));      // process structure
    Byte *dat = smem(ps+0x40);          // process dat (dynamic address translation)
    for(int i=0; i<8; i++) {
        pmmu[i] = dat[i*2+1];
    }
#endif
}

/*
 *   vdisk command processing
 *
 *   vrbf.asm will write a command on 0x40+IOPAGE ( 0xffc0 or 0xe040)
 *
 *   U contains caller's stack (call and return value)  on 0x45+IOPAGE
 *   current directory (cwd or cxd)                     on 0x44+IOPAGE
 *   Path dcriptor number                               on 0x47+IOPAGE
 *   drive number                                       on 0x41+IOPAGE
 *   caller's process descriptor                        on <0x50
 *
 *   each command should have preallocated os9 path descriptor on Y
 *
 *   name or buffer, can be in a user map, check that drive number ( mem[0x41+IOPAGE]  0 sys 1 user )
 *   current directory path number                                        mem[0x42+IOPAGE]  
 *   yreg has pd number
 */
void 
do_vdisk(Byte cmd) {
    int err;
    int curdir = mem[0x44+IOPAGE];  // garbage until set
    Byte attr ;
    Word u = getword(&mem[0x45+IOPAGE]);   // caller's stack in system segment
    Byte *frame = smem(u);
    xreg = getword(frame+4);
    yreg = getword(frame+6);
    *areg = *smem(u+1);
    *breg = *smem(u+2);
    Byte mode = 0;
    Byte *os9pd = smem(getword(&mem[0x47+IOPAGE]));
    PathDesc *pd = pdv+*os9pd;

    getDAT();
    pd->num = *os9pd;
    pd->drv = mem[0x41+IOPAGE];
    char *path,*next,*buf;
    if (vdiskdebug&1) vdisklog(u,pd,getword(&mem[0x47+IOPAGE]),curdir,stdout);

    switch(cmd) {
        /*
        * I$Create Entry Point
        *
        * Entry: A = access mode desired
        *        B = file attributes
        *        X = address of the pathlist
        *
        * Exit:  A = pathnum
        *        X = last byte of pathlist address
        *
        * Error: CC Carry set
        *        B = errcode
        */
        case 0xd1:
            mode = *areg;
            attr = *breg;
            pd->fp = 0;
            path = (char *)pmem(xreg);
            next = checkFileName(path,pd,curdir);
            *breg = 0xff;
            int fd = open(pd->name, O_RDWR+O_CREAT,os9mode(attr) );
            if (fd>0)
                pd->fp = fdopen(fd, os9toUnixAttr(mode));
            if (next!=0 && pd->fp ) {
                *breg = 0;
                *areg = pd->num;
                *smem(u+1) = *areg ;
                xreg += ( next - path );
                pd->use = 1;
            } else  {
                pd->use = 0;
            }
            break;

        /*
        * I$Open Entry Point
        *
        * Entry: A = access mode desired
        *        X = address of the pathlist
        *
        * Exit:  A = pathnum
        *        X = last byte of pathlist address
        *
        * Error: CC Carry set
        *        B = errcode
        */
        case 0xd2:
            mode = *areg;
            attr = *breg;
            pd->fp = 0;
            path = (char*)pmem(xreg);
            next = checkFileName(path,pd,curdir);
            *breg = 0xff;
            if (next!=0) {
                struct stat buf;
                if (stat(pd->name,&buf)!=0) break;
                if ((buf.st_mode & S_IFMT) == S_IFDIR) {
                    pd->dir = 1;
                    os9opendir(pd);
                } else {
                    pd->dir = 0;
                    pd->fp = fopen( pd->name,os9toUnixAttr(mode));
                }
                pd->use = 1;
            }
            if (next!=0 && pd->fp !=0) {
                *breg = 0;
                *areg = pd->num;
                *smem(u+1) = *areg ;
                xreg += ( next - path );
                pd->use = 1;
            } else {
                pd->use = 0;
            }
            break;

        /*
        * I$MakDir Entry Point
        *
        * Entry: X = address of the pathlist
        *        B = directory attributes
        *
        * Exit:  X = last byte of pathlist address
        *
        * Error: CC Carry set
        *        B = errcode
        */

        case 0xd3:
            *breg = 0xff;
            mode = *areg;
            attr = *breg;
            path = (char*)pmem(xreg);
            next =  checkFileName(path,pd,curdir);
            if (next!=0 &&  mkdir(pd->name,os9mode(attr))== 0 ) {
                xreg += ( next - path );
                *breg = 0;
            } 
            closepd(pd);
            break;

        /*
        * I$ChgDir Entry Point
        *
        *    data dir   P$DIO 3-5     contains open dir Path number 
        *    exec dir   P$DIO 9-11    contains open dir Path number 
        *
        * Entry:
        *
        * *areg = access mode
        *    0 = Use any special device capabilities
        *    1 = Read only
        *    2 = Write only
        *    3 = Update (read and write)
        * Entry: X = address of the pathlist
        *
        * Exit:  X = last byte of pathlist address
        *        A = open directory Path Number
        *
        * Error: CC Carry set
        *        B = errcode
        *
        *
        * we keep track a cwd and a cxd for a process using 8bit id
        * don't use path descriptor on y 
        */
        case 0xd4: {
            PathDesc dm = *pd;
            path = (char*)pmem(xreg);
            next = checkFileName(path,&dm,curdir);
            if (next!=0) { 
                struct stat buf;
                if (stat(dm.name,&buf)!=0) break;
                if ((buf.st_mode & S_IFMT) != S_IFDIR) break;
                xreg += ( next - path );
                *areg = setcd(dm.name);
                *smem(u+1) = *areg ;
                *breg = 0;
                break;
            } 
            *breg = 0xff;
         }
            break;

        /*
        * I$Delete Entry Point
        *
        * Entry:
        *
        * Exit:
        *
        * Error: CC Carry set
        *        B = errcode
        */
        case 0xd5: {
            *breg = 0xff;
            if (pd==0) break;
            struct stat st;
            path = (char*)pmem(xreg);
            next = checkFileName(path,pd,curdir);
            pd->use = 0;
            if (next!=0 && stat(pd->name,&st)!=0) break;
            if (next!=0 && ((st.st_mode&S_IFDIR)?rmdir(pd->name):unlink(pd->name)) == 0) {
                xreg += ( next - path );
                *breg = 0;
            } 
         }
            break;

        /*
        * I$Seek Entry Point
        *
        * Entry Conditions
        * A path number
        * X MS 16 bits of the desired file position
        * U LS 16 bits of the desired file position
        * 
        * Exit:
        *
        * Error: CC Carry set
        *        B = errcode
        */
        case 0xd6: {
            *breg = 0xff;
            ureg = (*smem(u+8)<<8)+*smem(u+9);
            off_t seek = (xreg<<16)+ureg;
            *breg = fseek(pd->fp,(off_t)seek,SEEK_SET);
            break;
            }
        /*
        * I$ReadLn Entry Point
        *
        * Entry:
        * Entry Conditions     in correct mmu map
        * A path number
        * X address at which to store data
        * Y maximum number of bytes to read
        *
        * Exit:
        *
        * Y number of bytes read
        *
        * Error: CC Carry set
        *        B = errcode
        *
        *
        */
        case 0xd7:
            *breg = 0xff;
            buf = (char*)pmem(xreg);
            char *b;
            if ((b=fgets(buf,yreg,pd->fp))) {
                if (b==0) {
                    *breg = 0xd3;
                    break;
                }
                int i;
                for(i=0;i<yreg && buf[i];i++);
                if (i>0 && buf[i-1]=='\n') {
                    buf[i-1] = '\r';
                    yreg = i;
                }
                // set y 
                setword(smem(u+6),yreg);
                *breg = 0;
            } 
            break;

        /*
        * I$Read Entry Point
        *
        * Entry:
        *
        * Exit:
        *
        * Error: CC Carry set
        *        B = errcode
        */
        case 0xd8:
            *breg = 0xff;
            buf = (char*)pmem(xreg);
            int i =  fread(buf,1,yreg,pd->fp);
            // set y 
            setword(smem(u+6),i);
            *breg = (i==0?0xd3:0) ;
            break;

        /*
        * I$WritLn Entry Point
        *
        * Entry:
        * A path number
        * X address of the data to write
        * Y maximum number of bytes to read

        *
        * Exit:
        * Y number of bytes written
        *
        * Error: CC Carry set
        *        B = errcode
        */
        case 0xd9: {
            *breg = 0xff;
            if (pd->dir) break;
            int len = yreg;
            int i = 0;
            Byte *buf = pmem(xreg);
            while(len>0 && buf[i] !='\r') {
                fputc(buf[i++],pd->fp);
                len--;
            }
            if (buf[i]=='\r') {
                fputc('\n',pd->fp);
                i++;
            }
            *breg = 0;
            // set y 
            setword(smem(u+6),i);
            break; 
         }

        /*
        * I$Write Entry Point
        *
        * Entry:
        *
        * Exit:
        *
        * Error: CC Carry set
        *        B = errcode
        */
        case 0xda :
            *breg = 0xff;
            if (!pd->dir) {
                Byte *buf = pmem(xreg);
                int len = yreg;
                int err = fwrite(buf,1,len,pd->fp);
                *breg = err?0:0xff;
                // set y 
                setword(smem(u+6),err);
            }
            break;

        /* I$Close Entry Point
        *
        * Entry: A = path number
        *
        * Exit:
        *
        * Error: CC Carry set
        *        B = errcode
        *
        */
        case 0xdb:
            *breg = 0xff;
            if (pd==0) break;
            closepd(pd);
            *breg = 0;
            break;

        /*
        * I$GetStat Entry Point
        *
        * Entry:
        *
        * Exit:
        *
        * Error: CC Carry set
        *        B = errcode
        */
        case 0xdc: {
            struct stat st;
            off_t pos;
            switch (*breg) {
                case 01: // SS.Ready
                    *breg = 0xff;
                    if (pd==0) break;
                    fstat(fileno(pd->fp),&st);
                    if ((pos = ftell(pd->fp))) {
                        xreg =  st.st_size - pos;
                        *breg = 0;
                    }
                    break;
                case 02: // SS.SIZ
                    *breg = 0xff;
                    if (pd==0) break;
                    fstat(fileno(pd->fp),&st);
                    xreg =  st.st_size ;
                    *breg = 0;
                    break;
                case 05: // SS.Pos
                    *breg = 0xff;
                    if (pd==0) break;
                    xreg =  ftell(pd->fp);
                    *breg = 0;
                    break;
                case 15: // SS.FD
                /*         SS.FD ($0F) - Returns a file descriptor
                 *                          Entry: R$A=Path #
                 *                                 R$B=SS.FD ($0F)
                 *                                 R$X=Pointer to a 256 byte buffer
                 *                                 R$Y=# bytes of FD required
                 */
                    *breg = 0xff;
                    if (pd==0) break;
                    *breg = filedescriptor(pmem(xreg), yreg,(Byte*)pd->name,curdir) ;
                    break;
                case 0x20: // Pos.FDInf    mandatry for dir command (undocumented, use the source)
                /*         SS.FDInf ($20) - Directly reads a file descriptor from anywhere
                 *                          on drive.
                 *                          Entry: R$A=Path #
                 *                                 R$B=SS.FDInf ($20)
                 *                                 R$X=Pointer to a 256 byte buffer
                 *                                 R$Y= MSB - Length of read
                 *                                      LSB - MSB of logical sector #
                 *                                 R$U= LSW of logical sector #
                 */
                    *breg = 0xff;
                    if (pd==0) break;
                    ureg = getword(smem(u+8));
                    *breg  = fdinfo(pmem(xreg),(yreg&0xff),((yreg&0xff00)>>8)*0x10000+ureg,pd,curdir);
                    break;
                default:
                *breg = 0xff;
            }
            break;
          }

        /*
        * I$SetStat Entry Point
        *
        * Entry:
        *
        * Exit:
        *
        *
        * Error: CC Carry set
        *        B = errcode
        */
        case 0xdd:
            switch (*breg) {
                case 0: // SS.Opt
                case 02: // SS.SIZ
                case 15: // SS.FD
                case 0x11: // SS.Lock
                case 0x10: // SS.Ticks
                case 0x20: // SS.RsBit
                case 0x1c: // SS.Attr
                default: 
                    *breg = 0xff;
            }
            break;
    }
    if (vdiskdebug && *breg) printf("  vdisk call error %x\n",*breg);
    // return value
    mem[0x40+IOPAGE] = *breg;
    *smem(u+2) = *breg ;
    setword(smem(u+4),xreg);
}

static void 
vdisklog(Word u,PathDesc *pd, Word pdptr, int curdir, FILE *fp) {
    char *cd = cdt[curdir]?cdt[curdir]:"(null)";
    fprintf(fp,"pd %d 0x%x cd[%d]=%s ",pd->num, pdptr, curdir,cd);
    Byte *frame = smem(u);
    sreg  = u;
    ccreg = frame[0];
    *areg = frame[1];
    *breg = frame[2];
    xreg  = getword(frame+4);
    yreg  = getword(frame+6);
    ureg  = getword(frame+8);
    pcreg  = getword(frame+10)-3;            // point os9 swi2
    prog = (char*)(pmem(pcreg) - pcreg);
    if (*pmem(pcreg)==0 && *pmem(pcreg+1)==0) {
        // may be we are called from system state
        // of coursel, this may wrong. but in system state, <$50 process has wrong DAT for pc
        // and we can't know wether we are called from system or user
        prog = (char*)(smem(pcreg) - pcreg);
    }
    do_trace(fp);
}


/* end */
