SRC2  = ../../nitros9-code/level2/cmds
SRC1  = ../../nitros9-code/level1/cmds

OBJ2 = mdir mfree proc 

OBJ1 = asm attr calldbg cmp config copy cputype date debug del deldir devs dir dir_cb dirsort  dump  echo edit \
    error free help ident irqs link list load login makdir pxd pwd procs prompt shellplus sleep tee touch tsmon unlink game09

all : $(OBJ2) $(OBJ1)

A09 = ../../../src/a09 -I ../

clean :
	rm -rf *.lst $(OBJ2) $(OBJ1)

game09 :
	$(A09) -l $@.lst  -o $@ ../../../game09/$@.asm

mdir :
	$(A09) -l $@.lst  -o $@ $(SRC1)/$@.asm
mfree :
	$(A09) -l $@.lst  -o $@ $(SRC1)/$@.asm
proc :
	$(A09) -l $@.lst  -o $@ $(SRC2)/$@.asm
procs :
	$(A09) -l $@.lst  -o $@ $(SRC1)/$@.asm
reboot :
	$(A09) -l $@.lst  -o $@ $(SRC2)/$@.asm
smap :
	$(A09) -l $@.lst  -o $@ $(SRC2)/$@.asm

asm :
	$(A09) -l $@.lst  -o $@ $(SRC1)/$@.asm
attr :
	$(A09) -l $@.lst  -o $@ $(SRC1)/$@.asm
calldbg :
	$(A09) -l $@.lst  -o $@ $(SRC1)/$@.asm
cmp :
	$(A09) -l $@.lst  -o $@ $(SRC1)/$@.asm
config :
	$(A09) -l $@.lst  -o $@ $(SRC1)/$@.asm
copy :
	$(A09) -l $@.lst  -o $@ $(SRC1)/$@.asm
cputype :
	$(A09) -l $@.lst  -o $@ $(SRC1)/$@.asm
date :
	$(A09) -l $@.lst  -o $@ $(SRC1)/$@.asm
debug :
	$(A09) -l $@.lst  -o $@ $(SRC1)/$@.asm
del :
	$(A09) -l $@.lst  -o $@ $(SRC1)/$@.asm
deldir :
	$(A09) -l $@.lst  -o $@ $(SRC1)/$@.asm
devs :
	$(A09) -l $@.lst  -o $@ $(SRC1)/$@.asm
dir :
	$(A09) -l $@.lst  -o $@ $(SRC1)/$@.asm
dir_cb :
	$(A09) -l $@.lst  -o $@ $(SRC1)/$@.asm
dirsort :
	$(A09) -l $@.lst  -o $@ $(SRC1)/$@.asm
disasm :
	$(A09) -l $@.lst  -o $@ $(SRC1)/$@.asm
dump :
	$(A09) -l $@.lst  -o $@ $(SRC1)/$@.asm
dw.as :
	$(A09) -l $@.lst  -o $@ $(SRC1)/$@.asm
echo :
	$(A09) -l $@.lst  -o $@ $(SRC1)/$@.asm
edit :
	$(A09) -l $@.lst  -o $@ $(SRC1)/$@.asm
error :
	$(A09) -l $@.lst  -o $@ $(SRC1)/$@.asm
free :
	$(A09) -l $@.lst  -o $@ $(SRC1)/$@.asm
help :
	$(A09) -l $@.lst  -o $@ $(SRC1)/$@.asm
ident :
	$(A09) -l $@.lst  -o $@ $(SRC1)/$@.asm
irqs :
	$(A09) -l $@.lst  -o $@ $(SRC1)/$@.asm
link :
	$(A09) -l $@.lst  -o $@ $(SRC1)/$@.asm
list :
	$(A09) -l $@.lst  -o $@ $(SRC1)/$@.asm
load :
	$(A09) -l $@.lst  -o $@ $(SRC1)/$@.asm
login :
	$(A09) -l $@.lst  -o $@ $(SRC1)/$@.asm
makdir :
	$(A09) -l $@.lst  -o $@ $(SRC1)/$@.asm
pxd :
	$(A09) -D PXD -l $@.lst  -o $@ $(SRC1)/pd.asm
pwd :
	$(A09) -D PWD -l $@.lst  -o $@ $(SRC1)/pd.asm
prompt :
	$(A09) -l $@.lst  -o $@ $(SRC1)/$@.asm
shellplus :
	$(A09) -l $@.lst  -o $@ $(SRC1)/$@.asm
sleep :
	$(A09) -l $@.lst  -o $@ $(SRC1)/$@.asm
tee :
	$(A09) -l $@.lst  -o $@ $(SRC1)/$@.asm
touch :
	$(A09) -l $@.lst  -o $@ $(SRC1)/$@.asm
tsmon :
	$(A09) -l $@.lst  -o $@ $(SRC1)/$@.asm
unlink :
	$(A09) -l $@.lst  -o $@ $(SRC1)/$@.asm
