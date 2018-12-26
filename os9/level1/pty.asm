*******************************************
* PTY
*  Device Driver for emulated terminal
* Emulates the 6850 UART seen in usim
* Protocol:
* V.port is the control port.
* V.port+1 is the data port. It is bidirectional.
*  bit 1: 1=data ready to read/0=no data
*  bit 2: 1=ready for output
*

         nam PTY
         ttl Pseudo Terminal

         ifp1
         use defsfile
         endc

***********************
* Edition History
*
* #   date     Comments
* - -------- -------------------------------
* 1 16.11.01  Driver first written. SMR

Revision equ 1

         org V.SCF
PST      equ .

         mod PEND,PNAM,Drivr+Objct,Reent+Revision,PENT,PST
         fcb READ.+WRITE.
PNAM     fcs /PTY/

PENT     lbra INIT
         lbra READ
         lbra WRITE
         lbra GETSTA
         lbra PUTSTA
         lbra TERM

**********************
* INIT
* Entry: U = Static storage
*   Setup the PIA
*
INIT     clrb
         rts

**********************
* READ
* Entry: U = Static Storage
*        Y = Path Descriptor
* Exit:  A = Character read
*
*   Read a byte from Port B
*
READ     ldx V.PORT,u      load port address
readlp   ldb ,x
         bitb #$01
         bne readbyte
         pshs x                sleep for a bit if not
         ldx #1
         os9 f$sleep
         puls x
         bra readlp
readbyte lda 1,x        read byte
         clrb
         rts
**********************
* WRITE
* Entry: U = Static storage
*        Y = Path Descriptor
*        A = Char to write
*
*   Write a byte to Port B
*
WRITE    ldx V.PORT,u   load port address
write1   ldb ,x
         bitb #$02
         bne wrt
         pshs x
         ldx #1
         os9 f$sleep
         puls x
         bra  write1
wrt      sta  1,x              write byte
ok       clrb
         rts

**********************
* GETSTA
*   U = Static Storage
*   Y = Path Descriptor
*
GETSTA   ldx PD.Rgs,y     X=pointer to registers
         lda R$B,x              A=contents of B reg.
         cmpa #SS.Ready         check for ready code
         bne gsta1              if not, check next
         ldx V.Port,u           use the PIA to determine
         ldb ,x                 if data is available
         bitb #$01
         bne ok
         ldb #E$NotRdy
         coma                   sets the carry flag
         rts
gsta1    cmpa #SS.EOF      check for eof code
         beq ok                 which always returns ok
         comb                   otherwise return
         ldb #E$UnkSVC          unknown code error
         rts

**********************
* PUTSTA
*  Always return error 
*
PUTSTA   ldx PD.Rgs,y     X=register stack
         ldb R$B,x              B=contents of reg B
         comb                   otherwise return error.
         ldb #E$UnkSVC
         rts

**********************
* TERM
*   Terminate Driver
*
TERM     clrb
         rts

         emod
PEND     equ *

