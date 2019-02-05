#define	FILE	char
#define	FCBSIZE	320
#define	NFILES	8

#define	NULL	0
#define EOF	(-1)

#define	stdin	_fcbtbl[0]
#define	stdout	_fcbtbl[1]
#define	stderr	_fcbtbl[2]

#define	STDIN	0xffff
#define	STDOUT	0xfffe
#define	STDERR	0xfffd

FILE *_fcbtbl[NFILES];

_main(argc,argv)
int argc;
char **argv;
{int i;
	stdin = STDIN;
	stdout = STDOUT;
	stderr = STDERR;
	initheap();
	for ( i = 3; i < NFILES; i++ ) _fcbtbl[i] = NULL;
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
			rd = 1; break;
		case 'w':
			wt = 1; break;
		case 'c':
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
	if ( FMS(fcbp,1) < 0 ) return NULL;
	fcbp[59] = cm ? 0 : 0xff;
	fcbp[60] = 0;
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
	if ( FMS(fcbp,2) < 0 )
	{	if ( (fcbp[1] != 3) || (FMS(fcbp,12) < 0) ) return NULL;
		_setname(name,fcbp);
		if (FMS(fcbp,2) < 0) return NULL;
	}
	fcbp[15] = 0;
	fcbp[59] = cm ? 0 : 0xff;
	fcbp[60] = 0;
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
	if ( FMS(fcbp,4) < 0 ) return EOF;
	mfree(fcbp);
	return 0;
}

_setname(name,fcbp)
char *name,*fcbp;
{int i;
	while(isspace(*name)) ++name;
	if (isdigit(*name))
	{	fcbp[3] = *name++ - '0';
		if (*name++ != '.') return 0;
	}
	else fcbp[3] = 0xff;
	for (i = 4; i < 15; ++i) fcbp[i] = 0;
	if (!isalpha(*name)) return -1;
	for (i = 4; i < 12; ++i)
	{	if (!*name || (*name == '.')) break;
		fcbp[i] = *name++;
	}
	while (*name && (*name != '.')) ++name;
	if (*name == '.')
	{	++name;
		for (i = 12; i < 15; ++i)
		{	if (!*name) break;
			fcbp[i] = *name++;
		}
	}
	return 1;
}


getc(fcbp)
char *fcbp;
{
	switch (fcbp)
	{case STDIN:
		return GETCH();
	case STDOUT:
	case STDERR:
		return EOF;
	default:
		if (fcbp[2] != 1) return EOF;
		return FMS(fcbp,0);
	}
}

putc(c,fcbp)
char c,*fcbp;
{	if ( c == '\t' ) c = ' ';
	switch (fcbp)
	{case STDIN:
		return EOF;
	case STDOUT:
		return PUTCH(c);
	case STDERR:
		return PUTCH2(c);
	default:
		if (fcbp[2] != 2) return EOF;
		if (FMS(fcbp,0,c) < 0) return EOF;
		return c;
	}
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
	
	LDD	heapp,Y
	BNE	_mc0
	BSR	initheap
_mc0	PSHS	D
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

initheap
	PSHS	U
	LEAU	,S
	TFR	Y,D
	ADDD	#_GLOBALS
	STD	heapp,Y
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
