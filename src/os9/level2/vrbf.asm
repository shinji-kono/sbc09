*******************************************************************2
* Virtual RBF - Random Block File Manager * 

         nam   VRBF
         ttl   VRandom Block File Manager

         ifp1  
         use   defsfile
         endc  

rev      set   $00
ty       set   FlMgr
         IFNE  H6309
lg       set   Obj6309
         ELSE
lg       set   Objct
         ENDC
tylg     set   ty+lg
atrv     set   ReEnt+rev
edition  set   37

         org   $00
size     equ   .

         mod   eom,name,tylg,atrv,start,size

name     fcs   /VRBF/
         fcb   edition

*L0012    fcb   DRVMEM


****************************
*
* Main entry point for RBF
*
* Entry: Y = Path descriptor pointer
*        U = Register stack pointer

start    lbra  Create
         lbra  Open
         lbra  MakDir
         lbra  ChgDir
         lbra  Delete
         lbra  Seek
         lbra  Read
         lbra  Write
         lbra  ReadLn
         lbra  WriteLn
         lbra  GetStat
         lbra  SetStat
         lbra  Close

* 
*            
*


*
* I$Create Entry Point
*
* Entry: A = access mode desired (0 read, 1 write, 2 update, bit 4 for exex)
*        B = file attributes
*        X = address of the pathlist
*
* Exit:  A = pathnum
*        X = last byte of pathlist address
*
* Error: CC Carry set
*        B = errcode
*
Create   pshs  y,u,cc		Preserve path desc ptr
         orcc  #IntMasks
         bsr   setuppd
         stb   4,u              put file attribute
         ldb   #$d1
         stb   ,u               do IO   b,x will be rewrited
         ldb   ,u
         beq   ok00
         bra   er00

*
* I$Open Entry Point
*
* Entry: A = access mode desired
*        X = address of the pathlist
*
* Exit:  A = pathnum
*        X = last byte of pathlist address
*
* Error: CC Carry set
*        B = errcode
*
Open     pshs  y,u,cc
         orcc  #IntMasks
         bsr   setuppd
         ldb   #$d2
         stb   ,x
         ldb   ,x
         cmpb  #0
         beq   ok00
         bra   er00

*        u    user stack
*        y    path descriptor
*         PD.PD.y     path number
*         PD.PD.MOD.y mode
*         PD.RGS,y    caller's rega  = u
*         PD.DEV,y    device table
*         PD.DRV,y    drive number

setuppd  ldx   #$FFc0           vdisk port
         sty   7,x              path descriptor
         stu   5,x              caller stack
         lda   <PD.DRV,y
         sta   1,x
         ldy   <D.Proc          get process pointer
         lda   R$A,u
         bita  #EXEC.
         bne   usechx
         ldb   P$DIO+3,y        get curwdir #pdnumber
         bra   s1
usechx   ldb   P$DIO+9,y        get curxdir #pdnumber
s1       stb   4,x
         rts

er00     puls  y,u,cc
         ldb   R$b,u
         lda   R$Cc,u
         ora   #Carry
         sta   R$Cc,u
         orcc  #Carry
         rts
ok00     puls  y,u,cc,pc

*
* I$MakDir Entry Point
*
* Entry: X = address of the pathlist
*
* Exit:  X = last byte of pathlist address
*
* Error: CC Carry set
*        B = errcode
*
MakDir   pshs  y,u,cc
         orcc  #IntMasks
         bsr   setuppd
         ldb   #$d3
         stb   ,x
         ldb   ,x
         cmpb  #0
         beq   ok00
         bra   er00

*
* I$Close Entry Point
*
* Entry: A = path number
*
* Exit:
*
* Error: CC Carry set
*        B = errcode
*
Close    pshs  y,u,cc
         orcc  #IntMasks
         bsr   setuppd
         ldb   #$db
         stb   ,x
         ldb   ,x
         cmpb  #0
         beq   ok00
         bra   er00

*
* I$ChgDir Entry Point
*
* Entry:
*
* Exit:
*
* Error: CC Carry set
*        B = errcode
*
ChgDir   pshs  y,u,cc
         orcc  #IntMasks
         bsr   setuppd
         ldb   #$d4
         stb   ,x
         ldb   ,x
         ldy   1,s
         ldx   <D.Proc          get process pointer
         ldu   PD.RGS,y
         lda   R$A,u
         ldb   PD.MOD,y		get current file mode
         bitb  #UPDAT.		read or write mode?
         beq   CD30D		no, skip ahead
* Change current data dir
         sta   P$DIO+3,x
CD30D    bitb  #EXEC.		is it execution dir?
         beq   ok01		no, skip ahead
* Change current execution directory
         sta   P$DIO+9,x
         bra   ok01

*
* I$Delete Entry Point
*
* Entry:
*
* Exit:
*
* Error: CC Carry set
*        B = errcode
*
*
Delete   pshs  y,u,cc
         orcc  #IntMasks
         lbsr   setuppd
         ldb   #$d5
         stb   ,x
         ldb   ,x
         cmpb  #0
         beq   ok01
         bra   er01

*
* I$Seek Entry Point
*
* Entry:
*
* Exit:
*
* Error: CC Carry set
*        B = errcode
*
Seek     pshs  y,u,cc
         orcc  #IntMasks
         lbsr   setuppd
         ldb   #$d6
         stb   ,x
         ldb   ,x
         cmpb  #0
         beq   ok01
         bra   er01

*
* I$ReadLn Entry Point
*
* Entry:
*
* Exit:
*
* Error: CC Carry set
*        B = errcode
*
ReadLn   pshs  y,u,cc
         orcc  #IntMasks
         lbsr   setuppd
         ldb   #$d7
         stb   ,x
         ldb   ,x
         beq   ok01
         bra   er01

*
* I$Read Entry Point
*
* Entry:
*
* Exit:
*
* Error: CC Carry set
*        B = errcode
*
Read     pshs  y,u,cc
         orcc  #IntMasks
         lbsr   setuppd
         ldb   #$d8
         stb   ,x
         ldb   ,x
         beq   ok01
         bra   er01


*
* I$WritLn Entry Point
*
* Entry:
*
* Exit:
*
* Error: CC Carry set
*        B = errcode
*
WriteLn  pshs  y,u,cc
         orcc  #IntMasks
         lbsr   setuppd
         ldb   #$d9
         stb   ,x
         ldb   ,x
         beq   ok01
         bra   er01

*
* I$Write Entry Point
*
* Entry:
*
* Exit:
*
* Error: CC Carry set
*        B = errcode
*
Write    pshs  y,u,cc
         orcc  #IntMasks
         lbsr   setuppd
         ldb   #$da
         stb   ,x
         ldb   ,x
         beq   ok01
er01     puls  y,u,cc
         ldb   R$b,u
         lda   R$Cc,u
         ora   #Carry
         sta   R$Cc,u
         orcc  #Carry
         rts
ok01     puls  y,u,cc,pc


*
* I$GetStat Entry Point
*
* Entry:
*
* Exit:
*
* Error: CC Carry set
*        B = errcode
*
GetStat  pshs  y,u,cc
         orcc  #IntMasks
         ldb   R$B,u		get function code
         lbsr   setuppd
         ldb   #$dc
         stb   ,x
         ldb   ,x
         beq   ok01
         bra   er01


*
* I$SetStat Entry Point
*
* Entry:
*
* Exit:
*
* Error: CC Carry set
*        B = errcode
*
SetStat  pshs  y,u,cc
         orcc  #IntMasks
         ldb   R$B,u		get function code
         lbsr   setuppd
         ldb   #$dd
         stb   ,x
         ldb   ,x
         beq   ok01
         bra   er01


         emod  
eom      equ   *
         end

