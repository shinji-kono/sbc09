#!/usr/bin/perl
# 
# 
#  the basic source include direct ACIA access 
#   which is not allowed in OS-9 
#   sbc09 emulator on OS-9, echos input in default 
#   the basic assumes input has no echo, so call echo
#   off at initialization 

while(<>) {
    next if (/^ACIA/ .. /^TDRE/) ;
    next if (/^TSTBRK/ .. /^        END/) ;
    if (/^CLEAR/) {
print <<"EOFEOF";
       JSR	\$24  ;; echo off 
EOFEOF
    }
    #    if (/^GL02/) {
    #    print "GL02\n";      # do not echo input
    #    next;
    #    }
    if (/FDB\s+CLEAR/) {
        print;
print <<"EOFEOF";
        FCC     /BYE/
        FCB     EOL
        FDB     EXIT
EOFEOF
       next;
    }
    print;
}

print <<"EOFEOF";
EXIT    
        JSR	\$24  ;; echo off 
        JSR	\$27  ;; echo on 
        JMP    \$2a   ;; exit
        
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

