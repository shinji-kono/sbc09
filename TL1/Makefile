

all : TL1.asm TL1

TL1 : TL1os9.asm
	../src/a09 TL1os9.asm -o TL1 -l TL1os9.lst
	cp TL1 ../os9/level1/cmds
	cp TL1 ../os9/level2/cmds

TL1.asm : TL1.txt
	perl toasm.pl TL1.txt > TL1.asm 

TL1.b : TL1.asm
	../src/a09 -l TL1.lst TL1.asm


