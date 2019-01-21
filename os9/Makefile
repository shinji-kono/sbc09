all: os9mod make9rom  os9v2.rom os9v1.rom level1/init level2/init

CFLAGS = -g # -std=c99 -D_POSIX_C_SOURCE=1 -D_XOPEN_SOURCE=1 -D_POSIX_SOURCE=200809L -D_GNU_SOURCE=1

os9mod : crc.c os9.h os9mod.c os9.h
	$(CC) $(CFLAGS) -o os9mod crc.c  os9mod.c

make9rom : makerom.c
	$(CC) $(CFLAGS) -o make9rom  makerom.c

clean :
	rm -rf makerom *.rom os9mod *.dSYM

os9v1.rom : make9rom level1/init 
	./make9rom -o os9v1.rom  level1/shell level1/sysgo level1/ioman level1/term level1/pty level1/pdisk level1/d0 level1/d1 level1/vrbf level1/v0 level1/clock level1/scf level1/rbf level1/init level1/os9p2 level1/os9p1
   
os9v2.rom : make9rom level2/init
	./make9rom -o os9v2.rom -2  level2/shell  level2/dir level2/d1 level2/ioman  level2/os9p3_perr level2/os9p4_regdump  level2/pipe level2/piper level2/pipeman  level2/v0 level2/vrbf level2/scf level2/rbf level2/os9p2 level2/sysgo level2/clock level2/pdisk level2/d0 level2/pty level2/term level2/init level2/boot level2/os9p1 

level1/init : nitros9-code
	cd level1 ; make

level2/init : nitros9-code
	cd level2 ; make

nitros9-code :
	hg clone http://hg.code.sf.net/p/nitros9/code nitros9-code

runv2 :
	../src/v09c -rom os9v2.rom -0 OS9.dsk -1 WORK.dsk

runv1 :
	../src/v09 -rom os9v1.rom -0 OS9.dsk -1 WORK.dsk

