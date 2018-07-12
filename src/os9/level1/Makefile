
A09 = ../../a09

OS9SRC=../../../CoCoOS9
SRCDIR=$(OS9SRC)/level1/MODULES
SRCCMD=$(OS9SRC)/level1/CMDS

all : ioman pdisk init os9p1 os9p2 pipe pipeman pipeman_named piper rbf scf term pty d0 d1 clock vector shell dir mdir sysgo

clean : 
	rm -f ioman pdisk init os9p1 os9p2 pipe pipeman pipeman_named piper rbf scf term pty d0 d1 clock vector boot shell dir mdir sysgo *.lst

LST = -l $@.lst

# our own module for sbc09

pdisk : 
	$(A09) pdisk.asm -o pdisk $(LST) $(LST)

# full rom system does not requre the boot
# boot : boot.asm
# 	$(A09) boot.asm -o boot $(LST)

sysgo : sysgo.asm
	$(A09) sysgo.asm -o sysgo $(LST)

init : init.asm
	$(A09) init.asm -o init $(LST)

vector : vector.asm
	$(A09) vector.asm -o vector $(LST)

pty : pty.asm
	$(A09) pty.asm -o pty $(LST)

term : pty-dd.asm
	$(A09) pty-dd.asm -o term $(LST)

d0 : d0.asm
	$(A09) d0.asm -o d0 $(LST)

d1 : d1.asm
	$(A09) d1.asm -o d1 $(LST)

clock :  clock.asm
	$(A09) clock.asm -o clock $(LST)

# os9 level1 moduels

shell :
	cp ${SRCCMD}/shell .

mdir :
	cp ${SRCCMD}/mdir .

dir :
	$(A09) ${SRCCMD}/dir.asm -o dir $(LST)

os9p1 :
	$(A09) ${SRCDIR}/os9.asm -o os9p1 $(LST)

os9p2 :
	$(A09) ${SRCDIR}/os9p2.asm -o os9p2 $(LST)

ioman :
	$(A09) ${SRCDIR}/ioman.asm -o ioman $(LST)

pipe :
	$(A09) ${SRCDIR}/pipe.asm -o pipe $(LST)

pipeman :
	$(A09) ${SRCDIR}/pipeman.asm -o pipeman $(LST)

pipeman_named :
	$(A09) ${SRCDIR}/pipeman_named.asm -o pipeman_named $(LST)

piper :
	$(A09) ${SRCDIR}/piper.asm -o piper $(LST)

rbf :
	$(A09) ${SRCDIR}/rbf.asm -o rbf $(LST)

scf :
	$(A09) ${SRCDIR}/scf.asm -o scf $(LST)

