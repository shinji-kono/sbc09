#
# Makefile Sim6809
#
# created 1994 by L.C. Benschop
# 2013-10-28 - Jens Diemer: add "clean" section
# 2014-06-25 - J.E. Klasek
# 2018-07-12 - S. Kono
#
# copyleft (c) 1994-2014 by the sbc09 team, see AUTHORS for more details.
# license: GNU General Public License version 2, see LICENSE for more details.
#

all :
	cd src ; make
	cd os9 ; make 

lv1 : all
	src/v09  -rom os9/os9v1.rom -v os9/level1 -0 os9/OS9.dsk -1 os9/WORK.dsk
lv2 : all
	src/v09c -rom os9/os9v2.rom -v os9/level2 -0 os9/OS9.dsk -1 os9/WORK.dsk

clean :
	cd src ; make realclean
	cd os9 ; make clean
	cd os9/level1 ; make clean
	cd os9/level2 ; make clean
