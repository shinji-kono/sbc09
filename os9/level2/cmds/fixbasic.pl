#!/usr/bin/perl
#

while(<>) {
    next if (/^ACIA/ .. /^TDRE/) ;
    next if (/^TSTBRK/ .. /^        END/) ;
    if (/^CLEAR/) {
print <<"EOFEOF"
       JSR	\$24  ;; echo off (but it is not suuported on pty.asm )
EOFEOF
    }
    if (/^GL02/) {
        print "GL02\n";      # do not echo input
        next;
    }
    print;
}


print <<"EOFEOF"
******************************
******************************
TSTBRK	bsr	BRKEEE 	
	bcc	GETC05
GETCHR	bsr 	INEEE
	CMPA	\#ETX          ; 3
	BNE	GETC05
	JMP	BREAK
INTEEE  
GETC05	RTS
PUTCHR	INC	ZONE
	JMP	OUTEEE
******************************
******************************
INEEE	PSHS    D
        JSR	0
        STB     ,S
        PULS    D,PC
OUTEEE	PSHS    D
        TFR     A,B
        JSR     3
        PULS    D,PC
BRKEEE	PSHS    D
        JSR     \$F
        PULS    D,PC
******************************
******************************
	END
EOFEOF
