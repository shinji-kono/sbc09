6809 Simulator/Emulator for OS9
=======================

sbc09 stands for Lennart Benschop 6809 Single Board Computer.
It contains a assembler and simulator for the Motorola M6809 processor.

copyleft (c) 1994-2014 by the sbc09 team, see AUTHORS for more details.
license: GNU General Public License version 2, see LICENSE for more details.


Forum thread: http://archive.worldofdragon.org/phpBB3/viewtopic.php?f=8&t=4880
Project: https://github.com/6809/sbc09


For the usage of the assembler a09 and 6809 single board system v09 
read doc/sbc09.creole!


This distribution includes 
 1. The 6809 single board system as a stand alone environment built as v09 
 1. with CoCo like MMU v09c 

How to make
---------

    make clean; make


How to run
---------

    make lv1

or

    make lv2

vrbf mount current directory on /v0, put os9 command there.

You can add os9 disk image using -0 or -1 option ( ex. https://github.com/sorenroug/osnine-java.git )

    src/v09  -rom src/os9/os9lv1.rom -0 OS9.dsk -1 WORK.dsk 

    src/v09c -rom src/os9/os9lv2.rom -0 OS9.dsk -1 WORK.dsk 

use -nt for trace debug without timmer interrupt.


Structure
---------

src/
  a09.c
      The 6809 assembler. It's fairly portable (ANSI) C. It works on Unix

      Features of the assembler:
      - os9 directives
      - Statements  MACRO, PUBLIC, EXTERN IF/ELSE/ENDIF INCLUDE not yet
        implemented. 

  v09.c
  engine.c
  io.c
  trace.c
      The 6809 single board simulator/emulator v09.
          -DUSE_MMU to use MMU
  vdisk.c
      mount current directory on /v0 using VRBF

  d09.c
      6809 disassembler with os9 feature
       
  os9/
      makerom.c    make rom for level1 and level2
      os9mod.c     check os9 module
                     -s    skip fill bytes
      crc.c        os9  crc checker
      level1       os9  level1 module 
        clock.asm
        d0.asm
        d1.asm
        v0.asm
        init.asm
        pdisk.asm
        printer.asm
        pty-dd.asm
        pty.asm
      level2       os9  level2 module 
        boot.asm
        defsfile
        init.asm
        clock.asm
        sysgo.asm
        vector.asm
        vrbf.asm   virtual rbf manager
        v0.asm

v09/v09c feature

    Usage: v09 [-rom rom-image] [-t tracefile [-tl addr] [-nt][-th addr] ] [-e escchar] 
               [-0 diskImage0] [-1 diskImage1]

    with Coco MMU
    Usage: v09c [-rom rom-image] [-t tracefile [-tl addr] [-nt][-th addr] ] [-e escchar] 
               [-0 diskImage0] [-1 diskImage1]

 -nt start with trace on
 -rom options use irq ( not firq ) timer, timer will not start until timer IO command
 vrbf default is a current directory

v09 tracing command  ( may be very slow )

    v09>h
      s [count]  one step trace (default)
      n          step over call or os9 system call
      f          finish this call (until stack pop) (unreliable)
      b [adr]    set break / watch point (on current physical address)
                 it stoped on pc==adr or value of adr was changed
      B          break / watch point list
      d [n]      delte break point list
      c  [count] continue;
      x  disassemble on pc
      x  [adr] [count]  dump
      xp page [adr] [count] mmu page dump
      xi [adr] [count]  disassemble
      0  file    disk drive 0 image
      1  file    disk drive 1 image
      L  file    start log to file
      S  file    set input file
      X  exit
      q  exit
      U  file    upload from srecord file 
      D  file    download to srecord file 
      R  do reset  (unreliable)
      h,?  print this

    to see GIME
      x 0xff90

a09 Assembler for os9
-------------

      mod   eom,name,tylg,atrv,start,size      define os9 mod with crc
      .     data pointer ( same as *, only works just after the mod )
      *     code pointer 
      emod

      os9   os9 system call
      end

      fcs   generates os9 string with 8th bit on termination

      use   use os9 sources ( subsequent use/lib follow the directories )

      accepts some more chars in names such as $ . _


os9 command
-------------
   src/os9/level1/cmds
   src/os9/level2/cmds

   sbc09    sbc09 emulator on os9
       OS9: sbc09 kernel09.s
       OS9: sbc09 basic.s

   Todo ( program load command on game09 and forth )

================

Micro C
-------------
   src/os9/mc09
      only working on level2

   OS9: mc09/mc-s -Mtestcp test/cp.c
        
   % src/a09 crtos9.asm -l c.lst -o testcp


GAME09
-------------
   game09  
   src/os9/level[12]/game09

   OS9: game09

   > \LD "game09/asm09.game"
   > #=1

TL/1
-------------
    TL/1

   OS9: tl1 tl1/test/t1.tl1


Links/References
================


Project:
  https://github.com/6809/sbc09
  Maintained by the original author and others.

Source:
  http://groups.google.com/group/alt.sources/browse_thread/thread/8bfd60536ec34387/94a7cce3fdc5df67
  Autor: Lennart Benschop  lennart@blade.stack.urc.tue.nl, 
                lennartb@xs4all.nl (Webpage, Subject must start with "Your Homepage"!)
