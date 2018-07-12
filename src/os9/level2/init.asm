* OS9 standard init modul.
         nam   Init
         ttl   os9 system module

         ifp1
         use   defsfile
         endc
null     set   $0000
tylg     set   Systm+$00
atrv     set   ReEnt+rev
rev      set   $01
         mod   eom,initnam,tylg,atrv
         fcb   7
         fdb   $c000
         fcb   $0C
         fcb   $0C
         fdb   sysgo
         fdb   sysdev      system device (sysdev)
         fdb   systerm
         fdb   bootst      bootstrap module (bootst)
initnam  fcs   "Init"
sysgo    fcs   "SysGo"
sysdev   fcs   "/D0"
systerm  fcs   "/Term"
bootst   fcs   "Boot"
         emod
eom      equ   *

