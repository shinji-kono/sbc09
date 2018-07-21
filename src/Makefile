#
# Makefile Sim6809
#
# created 1994 by L.C. Benschop
# 2013-10-28 - Jens Diemer: add "clean" section
# 2014-06-25 - J.E. Klasek
#
# copyleft (c) 1994-2014 by the sbc09 team, see AUTHORS for more details.
# license: GNU General Public License version 2, see LICENSE for more details.
#

# CFLAGS=-O3 -fomit-frame-pointer -DTERM_CONTROL
CFLAGS=-g  -DTERM_CONTROL

V09FLAGS= -DUSE_TERMIOS #-DBIG_ENDIAN


SIM_BIN=v09s v09st 

APPS=mon2.s

# will be installed to ".."
BIN=a09 v09 v09c d09 $(SIM_BIN) v09.rom 

TARGETS=$(BIN) $(APPS)  

OTHER=monitor.s makerom

all: $(TARGETS)

# ------------------------------------

a09 : a09.o os9crc.o
	$(CC)  $(CFLAGS) $(V09FLAGS) a09.o os9crc.o -o $@

v09: v09.o engine.o io.o d09.o trace.o vdisk.o
	$(CC) -o v09 $(CFLAGS) v09.o engine.o io.o d09.o trace.o

# with Coco MMU 
v09c: v09.c engine.c io.c d09.o trace.o vdisk.o
	$(CC) -o v09c $(CFLAGS) $(V09FLAGS) -DIOPAGE=0xff80 -DUSE_MMU=1 -DUSE_VDISK v09.c engine.c io.c d09.o trace.c vdisk.c

a09.o : a09.c 
	$(CC) -c $(CFLAGS) $(V09FLAGS) $< 

v09.o: v09.c 
	$(CC) -c $(CFLAGS) $(V09FLAGS) $<

d09 : d09.c
	$(CC) -Wno-format-security $(CFLAGS) $(V09FLAGS) $< -o $@

d09.o : d09.c
	$(CC) -c -DNO_MAIN -Wno-format-security $(CFLAGS) $(V09FLAGS) $< 

engine.o: engine.c 
	$(CC) -c $(CFLAGS) $(V09FLAGS) $<

io.o: io.c 
	$(CC) -c $(CFLAGS) $(V09FLAGS) $<

vdisk.o: vdisk.c v09.h
	$(CC) -c $(CFLAGS) $(V09FLAGS) $< 

trace.o: trace.c v09.h
	$(CC) -c $(CFLAGS) $(V09FLAGS) $< 

v09.rom: makerom monitor.s 
	./makerom <monitor.s

monitor.s: monitor.asm
	./a09 -s monitor.s -l monitor.lst monitor.asm

mon2.s: mon2.asm
	./a09 -s mon2.s -l mon2.lst mon2.asm

makerom: makerom.c
	$(CC) -o makerom makerom.c

# ------------------------------------

v09s: v09s.c
	$(CC) $(CFLAGS) -o $@ $<

v09st: v09s.c
	$(CC) $(CFLAGS) -DTRACE -o $@ $<

# ------------------------------------

install:
	-for bin in $(BIN); do \
	  [ -r $$bin ] && cp -p $$bin .. && echo "installing ../$$bin ..." ;\
	done ;\
	exit 0

# ------------------------------------

cleanall: clean
	rm -rf $(TARGETS) $(OTHER)
	(cd ..; rm -rf $(BIN) )

realclean: cleanall

clean:
	rm -rf core *.BAK *.o *.lst *.dSYM

# ------------------------------------

# DO NOT DELETE

engine.o: v09.h
io.o: v09.h
trace.o: v09.h
v09.o: v09.h
