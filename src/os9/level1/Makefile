
OS9SRC = ../nitros9-code
SRCDIR = $(OS9SRC)/level1/modules
SRCCMD = $(OS9SRC)/level1/cmds
SRCDIR2 = $(OS9SRC)/level2/modules

A09 = ../../a09 -I $(OS9SRC)/defs/

MOD =  pdisk init os9p1 os9p2 ioman pipe piper rbf scf term pty d0 d1 clock shell dir mdir sysgo shell

all : ${MOD}

clean : 
	rm -f $(MOD) *.lst krn.asm 

LST = -l $@.lst

# our own module for sbc09
# CoCoOS9's level1/os9p1.asm has wrong vector value, use our own

pdisk : 
	$(A09) pdisk.asm -o pdisk $(LST) 

#boot : boot.asm
#	$(A09) boot.asm -o boot $(LST)

init : init.asm
	$(A09) ${SRCDIR}/init.asm -o $@ $(LST)
#	$(A09) init.asm -o init $(LST)

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

shell : $(SRCCMD)/shell_21.asm 
	$(A09) $< -o $@ $(LST)

mdir : ${SRCCMD}/mdir.asm
	$(A09) $< -o $@ $(LST)

dir : ${SRCCMD}/dir.asm
	$(A09) ${SRCCMD}/dir.asm -o dir $(LST)

os9p1 :
	cat  ${OS9SRC}//defs/coco.d ${SRCDIR}/kernel/krn.asm > krn.asm
	$(A09) -I ../nitros9-code/level1/modules/kernel/ krn.asm  -o os9p1 $(LST)

os9p2 :
	$(A09) ${SRCDIR}/kernel/krnp2.asm -o os9p2 $(LST)

sysgo :
	$(A09) sysgo.asm -o sysgo $(LST)

ioman :
	$(A09) ${SRCDIR}/ioman.asm -o ioman $(LST)

pipe :
	$(A09) ${SRCDIR}/pipe.asm -o pipe $(LST)

piper :
	$(A09) ${SRCDIR}/piper.asm -o piper $(LST)

rbf :
	$(A09) ${SRCDIR}/rbf.asm -o rbf $(LST)

scf :
	$(A09) ${SRCDIR}/scf.asm -o scf $(LST)

