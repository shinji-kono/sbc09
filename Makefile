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
	cd src/os9 ; make os9v1.rom
	cd src/os9 ; make os9v2.rom

lv1 : all
	src/v09  -rom src/os9/os9v1.rom -v src/os9/level1
lv2 : all
	src/v09c -rom src/os9/os9v2.rom -v src/os9/level2

clean :
	cd src ; make realclean
	cd src/os9 ; make clean
	cd src/os9/level1 ; make clean
	cd src/os9/level2 ; make clean
