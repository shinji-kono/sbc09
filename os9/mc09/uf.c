#include "STDIO.TXT"
#include "STRING.TXT"
#include "FILEIO.TXT"
#include "ALLOC.TXT"

#define CONSTAT *0xff80
#define CONDATA *0xff81
#define REMSTAT *0xff90
#define REMDATA *0xff91

#define RXRDY   0x01
#define TXRDY   0x02

#define BREAK   0x00
#define EOT     0x04
#define ESC     0x1b

main()
{       printf("Terminal emulator\n");
        reminit();
        while (1)
        {       printf("\nT(erm , F(ile , E(xit : ");
                switch ( toupper(getchar()) )
                {       case 'T' : term(); break;
                        case 'F' : filer(); break;
                        case 'E' : exit();
                }
        }
}

term()
{char c;
        printf("\n>>> enter terminal mode <ctl-@ to exit>\n");
#asm
        ORCC    #$50    disable interrupt
#endasm
        while (1)
        {       if ( remstat() ) conwrite(remread());
                if ( constat() )
                {       if ( (c = conread()) == BREAK ) break;
                        remwrite(c);
                }
        }
        ;
#asm
        ANDCC   #$AF    restore interrupt mask
#endasm
}

filer()
{       printf("\n>>> enter file transfer mode\n");
        while (1)
        {       printf("\nDirection F(lex->unix , U(nix->flex , E(xit : ");
                switch ( toupper(getchar()) )
                {       case 'F' : flex_unix(); break;
                        case 'U' : unix_flex(); break;
                        case 'E' : return;
                }
        }
}

flex_unix()
{char   fn0[80],fn1[80],c;
 FILE *fp;
        printf("\nFLEX to UNIX file transfer\n");
        printf("FLEX file name : ");
        gets(fn0,80);
        printf("\n");
        toupstr(fn0);
        if ( (fp = fopen(fn0,"rc")) < 0 )
        {       printf("Can't open %s\n",fn0);
                return;
        }
        printf("UNIX file name : ");
        gets(fn1,80);
        printf("\n");
        tx_str("cat /dev/tty >");
        tx_str(fn1);
        tx_char('\n');
        while ( (c = getc(fp)) != EOF ) tx_char(c);
        remwrite(EOT);
        fclose(fp);
}

unix_flex()
{char   fn0[80],fn1[80],c;
 FILE *fp;
 int    i;
 char linebuf[256];
        printf("\nUNIX to FLEX file transfer\n");
        printf("UNIX file name : ");
        gets(fn0,80);
        printf("\nFLEX file name : ");
        gets(fn1,80);
        printf("\n");
        toupstr(fn1);
        if ( (fp = fopen(fn1,"wc")) < 0 )
        {       printf("Can't create %s\n",fn1);
                return;
        }
        tx_str("/mnt/sys/tezuka/unix_flex/unix_flex ");
        tx_str(fn0);
        tx_char('\n');
        while ( 1 ) {
                i = 0;
                while ( (c = remread()) != '\n' ) {
                        if ( c == ESC ) {
                                fclose(fp);
                                return;
                        }
                        linebuf[i++] = c;
                }
                linebuf[i++] = '\n';
                linebuf[i] = '\0';
                for ( i = 0; linebuf[i]; i++ ) putc(linebuf[i],fp);
                remwrite(ESC);
                putchar('.');
        }
}

toupstr(s)
char *s;
{       while ( *s )
        {       *s = toupper(*s);
                ++s;
        }
}

tx_str(s)
char *s;
{       while ( *s ) tx_char(*s++);
}

tx_char(c)
char c;
{       remwrite(c);
        while ( c != remread() );
/*    */putchar(c);
}

constat()
{       return ( CONSTAT & RXRDY );
}

conread()
{       while ( !constat() );
        return ( CONDATA & 0x7f);
}

conwrite(ch)
char ch;
{       while ( !(CONSTAT & TXRDY) );
        CONDATA = ch;
}

reminit()
{
        REMSTAT = 0x43;
        REMSTAT = 0x15;
}

remstat()
{       return ( REMSTAT & RXRDY );
}

remread()
{       while ( !remstat() );
        return ( REMDATA & 0x7f );
}

remwrite(ch)
char ch;
{       while ( !(REMSTAT & TXRDY) );
        REMDATA = ch;
}

