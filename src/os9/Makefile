all: os9mod makerom os9d.rom os9lv2.rom

CC=clang
CFLAGS = -g

os9mod : crc.c os9.h os9mod.c os9.h
	$(CC) $(CFLAGS) -o os9mod crc.c  os9mod.c

makerom : makerom.c
	$(CC) $(CFLAGS) -o makerom  makerom.c

clean :
	rm -f makerom *.rom os9mod 

os9d.rom : makerom level1/init
	./makerom -o os9d.rom  level1/shell level1/init level1/mdir level1/dir level1/SysGo level1/IOMan level1/term level1/pty level1/pdisk level1/d0 level1/d1 level1/clock level1/scf level1/rbf level1/os9p2 level1/OS9
   
os9lv2.rom : makerom level2/init
	./makerom -o os9lv2.rom -2  level2/Shell  level2/dir level2/d1 level2/ioman  level2/os9p3_perr level2/os9p4_regdump  level2/pipe level2/piper level2/pipeman  level2/scf level2/rbf level2/os9p2 level2/sysgo level2/pdisk level2/d0 level2/pty level2/term level2/init level2/boot level2/os9p1 


level1/init :
	cd level1 ; make

level2/init :
	cd level2 ; make
