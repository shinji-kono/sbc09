MCLIBS=alloc.txt c.txt fileio.txt fileio2.txt mclib.c scanf.txt\
	stdio.txt stdio2.txt string.txt

MCUTILS=uf.c

DESTDIR=../level2

# CFLAGS = -m32 -O0 -g -Wno-return-type -Wno-implicit-int -Wno-implicit-function-declaration -Wno-parentheses
CFLAGS = -O0 -g -Wno-return-type -Wno-implicit-int -Wno-implicit-function-declaration -Wno-parentheses -Wno-format

AS09 = ../../src/a09

all:	mc2 testcp

mc: mc.c
	cc $(CFLAGS) mc.c -o mc

mc2:	mc mc2.c mclib.c
	./mc -s -Mmc mc2.c
	$(AS09) crtos9.asm -l mc2.lst -o mc2

# mc2.o:	c.out
#	$(AS09) c.txt -l c.lst -o mc2.o 

lint: mc.c
	lint mc.c >lint

shar: diff_to_mc2
	shar COPYRIGHT README MANIFEST makefile $(MCLIBS) $(MCUTILS) diff_to_mc2 >mc.shar.0
	shar mc.c >mc.shar.1

diff_to_mc2: mc.c mc2.c
	-diff -c mc.c mc2.c >diff_to_mc2

mc2.c:
	patch <diff_to_mc2 -o mc2.c

testcp : mc test/cp.c
	./mc -s -Mtestcp test/cp.c
	$(AS09) crtos9.asm -l c.lst -o testcp

clean:
	rm -f mc c.out mc2.o lint testcp

install: mc2
	cp mc2 $(DESTDIR)/cmds
	cp crtos9.asm mclibos9.c test/cp.c $(DESTDIR)/
