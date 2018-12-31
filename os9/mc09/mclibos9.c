/* os9 driver  */ 


int errno  = 0;

typedef	struct {
    int fd;                   /*  0 */
    int fmode;                 /*  2 */
    int len;                  /*  4 */
    char *fname;               /*  6 */
    /*
    char *ptr;        /*  8 
    char buff[256];       /*  10/
    */
 } FILE ;

#define	FCBSIZE	(sizeof(FILE))

#define	NFILES	8

#define	NULL	0
#define EOF	(-1)

#define	stdin	_fcbtbl[0]
#define	stdout	_fcbtbl[1]
#define	stderr	_fcbtbl[2]

FILE *_fcbtbl[NFILES];

FILE _s0[3];

#define STDIN (&_s0[0])
#define STDOUT (&_s0[1])
#define STDERR (&_s0[2])

_main(prog,args)
char *prog;
char *args;
{int i;
 char **argv,*p,*q;
 int argc,n,quote,c;
	stdin = STDIN;  stdin->fd = 0;
	stdout = STDOUT;  stdout->fd = 1;
	stderr = STDERR;  stderr->fd = 2;
	initheap();
	for ( i = 3; i < NFILES; i++ ) _fcbtbl[i] = NULL;
        /* create argv here */
        argc = 0;
        argv = 0;
        for( i = 0; i < 2 ; i++ ) {
            q = p = args;
            if (i==1) { 
                argv = (char**)malloc(sizeof(char*)*(argc+1)); 
                argv[0] = prog; 
            }
            n = 1;
            quote = 0;
            if (i==1) argv[n] = args;
            while((c = *p) && c!='\r') {
                if (c=='\'') { 
                    if (!quote) {
                        p++; 
                        quote = 1;
                    } else {
                        p++; 
                        if (i==1) *q=0;
                        quote = 0;
                    }
                } else if (c=='\\') {
                    p++;
                } else if (c==' ') {
                    if (!quote) {
                        if (i==1) {
                            *q = 0; argv[++n] = q+1;
                        }
                    }
                }
                if (i==1) *q = *p;
                q++; p++;
            }
            if (i==1&&p!=args) { *q = 0; argv[++n] = q+1; }
            argc = n;
        }
        argv[n]=0;
	main(argc,argv);
}

FILE *fopen(name,mode)
char *name,*mode;
{FILE *fcbp;
 char *p;
 int rd,wt,cm;
	rd = wt = cm = 0;
	for ( p = mode; *p; p++ ) {
		switch ( *p ) {
		case 'r':
			rd = 1; cm |= 1; break;
		case 'w':
			wt = 1; cm |= 3; break;
		case 'c':     /* charcter mode */
			cm = 1; break;
		default:
			return NULL;
		}
	}
	if ( !(rd ^ wt) ) return NULL;
	if ( rd ) return _open(name,cm);
	else return _create(name,cm);
}

FILE *_open(name,cm)
char *name;
int cm;
{FILE *fcbp;
 int i;
	for ( i = 0; i < NFILES; i++)
		if ( _fcbtbl[i] == NULL ) break;
	if ( i >= NFILES) return NULL;
	if ( (fcbp = malloc(FCBSIZE)) == NULL ) return NULL;
	if ( _setname(name,fcbp) == 0 ) return NULL;
#asm
        pshs      x,y,u
        ldx       -2,u
        lda       7,u          mode
        ldx       6,x          name
        os9       I$Open
        bcs        _LC0001
        ldx       -2,u
        tfr       a,b
        clra
        std       ,x
        bra       _LC0002
_LC0001
        ldx       -2,u
        tfr       a,b
        clra
        std       2,x          err code
        ldd       #-1
        std       ,x
_LC0002
        puls      x,y,u
#endasm
	if (fcbp->fd < 0 ) { errno = fcbp->fmode ; return NULL; }
	/* fcbp->ptr = fcbp->buff;
	fcbp->len = 0; */
	return (_fcbtbl[i] = fcbp);
}

FILE *_create(name,cm)
char *name;
int cm;
{FILE *fcbp;
 int i;
	for ( i = 0; i < NFILES; i++)
		if ( _fcbtbl[i] == NULL ) break;
	if ( i >= NFILES) return NULL;
	if ( (fcbp = malloc(FCBSIZE)) == NULL ) return NULL;
	if ( _setname(name,fcbp) == 0 ) return NULL;
#asm
        pshs      x,y,u
        ldx       -2,u
        lda       7,u          mode
        ldb       #3
        ldx       6,x          name
        os9       I$Create
        bcs        _LC0003
        ldx       -2,u
        tfr       a,b
        clra
        std       ,x
        bra       _LC0004
_LC0003
        ldx       -2,u
        tfr       a,b
        clra
        stD       2,x          err code
        ldd       #-1
        std       ,x
_LC0004
        puls      x,y,u
#endasm
	if (fcbp->fd < 0 ) { errno = fcbp->fmode ; return NULL; }
	/* fcbp->ptr = fcbp->buff;
	fcbp->len = 0; */
	return (_fcbtbl[i] = fcbp);
}

fclose(fcbp)
FILE *fcbp;
{int i;
	for ( i = 0; i < NFILES; i++ )
		if ( fcbp == _fcbtbl[i] ) break;
	if ( i >= NFILES ) return EOF;
	_fcbtbl[i] = NULL;
	if ( (fcbp == STDIN) || (fcbp == STDOUT) || (fcbp == STDERR) ) return 0;
#asm
        pshs      x,y,u
        ldx       4,u
        lda       1,x
        os9       I$Close
        puls      x,y,u
#endasm
	mfree(fcbp);
	return 0;
}

_setname(name,fcbp)
char *name; FILE *fcbp;
{
        fcbp->fname = name;
	return 1;
}


getc(fcbp)
char *fcbp;
{
    int c;
#asm
        pshs      y
        ldx       4,u         
        lda       1,x       file descriptor
        leax      -1,u
        clr       -2,u
        ldy       #1
        os9       I$Read
        bcc       _LC0005
        ldd       #-1
        std       -2,u
_LC0005
        puls      y
#endasm
}

putc(c,fcbp)
char c,*fcbp;
{	
    int ret;
#asm
        pshs      y
        ldx       6,u         
        lda       1,x       file descriptor
        leax      5,u
        ldy       #1
        os9       I$Write
        puls      y
#endasm
     return c;
}

getchar()
{	return getc(stdin);
}

putchar(c)
char c;
{	return putc(c,stdout);
}

printf(s)
char *s;
{	_fprintf(stdout,s,(int *)&s+1);
}

fprintf(f,s)
char *f,*s;
{	_fprintf(f,s,(int *)&s+1);
}

_fprintf(f,s,p)
char *f,*s;
int *p;
{int l,m,n;
 char c,buf[8];
	while(c = *s++)
		if (c != '%') putc(c,f);
		else
		{	if (l=(*s == '-')) ++s;
			if (isdigit(*s)) s += _getint(&m,s);
			else m = 0;
			if (*s == '.') ++s;
			if (isdigit(*s)) s += _getint(&n,s);
			else n = 32767;
			switch(*s++)
			{case 'd':
				itoa(*p++,buf);
				break;
			case 'o':
				itooa(*p++,buf);
				break;
			case 'x':
				itoxa(*p++,buf);
				break;
			case 'u':
				itoua(*p++,buf);
				break;
			case 'c':
				ctos(*p++,buf);
				break;
			case 's':
				_putstr(f,*p++,l,m,n);
				continue;
			case '\0':
				return;
			default:
				ctos(c,buf);
				break;
			}
			_putstr(f,buf,l,m,n);
		}
}

_getint(p,s)
int *p;
char *s;
{int i;
	for(*p=i=0; isdigit(*s); ++i) *p = *p * 10 + *s++ - '0';
	return i;
}

_putstr(f,s,l,m,n)
char *f,*s;
int l,m,n;
{int k;
	k = (strlen(s) < n ? strlen(s) : n);
	m = (k < m ? m-k : 0);
	if (l)
	{	_putsn(f,s,n);
		_putspc(f,m);
	}
	else
	{	_putspc(f,m);
		_putsn(f,s,n);
	}
}
	
_putsn(f,s,n)
char *f,*s;
int n;
{	while(*s)
		if (--n >= 0) putc(*s++,f);
		else break;
}

_putspc(f,n)
char *f;
int n;
{	while(--n >= 0) putc(' ',f);
}

puts(s)
char *s;
{	while(*s) putchar(*s++);
}

itoa(n,s)
int n;
char *s;
{	if (n < 0)
	{	*s++ = '-';
		return (itoua(-n,s)+1);
	}
	return itoua(n,s);
}

itoua(n,s)
int n;
char *s;
{	return _itoda(n,s,10);
}

itooa(n,s)
int n;
char *s;
{	return _itoda(n,s,8);
}

itoxa(n,s)
int n;
char *s;
{	return _itoda(n,s,16);
}

_itoac(n)
int n;
{	return (n + ((n < 10) ? '0' : ('A'-10)));
}

_itoda(n,s,r)
unsigned n;
int r;
char *s;
{int i;
 char t[8],*u;
	u = t;
	*u++ = '\0';
	do *u++ = _itoac(n % r); while(n /= r);
	for (i=0; *s++ = *--u; ++i);
	return i;
}

char *ctos(c,s)
char c,*s;
{	s[0] = c;
	s[1] = '\0';
	return s;
}

strlen(s)
char *s;
{int i;
	for(i = 0; *s++; ++i);
	return i;
}

isdigit(c)
char c;
{	return '0' <= c && c <= '9';
}

isspace(c)
char c;
{	return (c == ' ' || c == '\t' || c == '\n');
}

isalpha(c)
char c;
{	return (isupper(c) || islower(c) || c == '_');
}

isupper(c)
char c;
{	return ('A' <= c && c <= 'Z');
}

islower(c)
char c;
{	return ('a' <= c && c <= 'z');
}

toupper(c)
char c;
{	return (islower(c) ? c + ('A'-'a') : c);
}

tolower(c)
char c;
{	return (isupper(c) ? c + ('a'-'A') : c);
}

atoi(s)
char *s;
{int i;
	while (isspace(*s)) ++s;
	for (i = 0; isdigit(*s);) i = i * 10 + *s++ - '0';
	return i;
}

typedef struct header
		{	struct header *bptr;
			unsigned bsize;
		} HEADER;

HEADER base,*allocp,*heapp;

char *malloc(s)
unsigned s;
{HEADER *p,*q;
 int nunits;
	nunits = 1 + (s + sizeof(HEADER) - 1) / sizeof(HEADER);
	if ((q = allocp) == NULL)
	{	base.bptr = allocp = q = &base;
		base.bsize = 0;
	}
	for (p = q->bptr; ; q = p,p = p->bptr)
	{	if (p->bsize >= nunits)
		{	if (p->bsize == nunits)
				q->bptr = p->bptr;
			else
			{	p->bsize -= nunits;
				p += p->bsize;
				p->bsize = nunits;
			}
			allocp = q;
			clearblock(p);
			return ((char *)(p + 1));
		}
		if (p == allocp)
			if ((p = morecore(nunits)) == NULL)
				return(NULL);
	}
}

clearblock(p)
HEADER *p;
{char *s,*t;
	s = (char *)(p + 1);
	t = (char *)(p + p->bsize);
	while (s < t) *s++ = 0;
}

#define NALLOC 128

HEADER *morecore(nu)
unsigned nu;
{char *cp;
 HEADER *up;
 int rnu;
	rnu = NALLOC * ((nu + NALLOC - 1) / NALLOC);
	cp = sbrk(rnu * sizeof(HEADER));
	if ((int)cp == -1) return NULL;
	up = (HEADER *) cp;
	up->bsize = rnu;
	mfree((char *)(up+1));
	return allocp;
}

#asm
sbrk	PSHS	U
	LEAU	,S
        LDD     heapp,Y
    	PSHS	D
	TFR	S,D
	SUBD	,S++
	CMPD	4,U
	BCC	_mc1
	LDD	#-1
	LEAS	,U
	PULS	U,PC
	
_mc1	LDD	4,U
	LDX	heapp,Y
	LEAX	D,X
	LDD	heapp,Y
	STX	heapp,Y
	LEAS	,U
	PULS	U,PC

#endasm

mfree(ap)
char *ap;
{HEADER *p,*q;
	p = (HEADER *)ap - 1;
	for (q = allocp; !(p > q && p < q->bptr); q = q->bptr)
		if (q >= q->bptr && (p > q || p < q->bptr)) break;
	if (p + p->bsize == q->bptr)
	{	p->bsize += q->bptr->bsize;
		p->bptr = q->bptr->bptr;
	}
	else p->bptr = q->bptr;
	if (q + q->bsize == p)
	{	q->bsize += p->bsize;
		q->bptr = p->bptr;
	}
	else q->bptr = p;
	allocp = q;
}

unsigned freesize()
{int i;
	if (!heapp) initheap();
	return ((char *)&i - (char *)heapp);
}
