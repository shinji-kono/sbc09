        org      $ffd0
entry   equ      $f100

reset   clra
        tfr      a,dp
        ldx      #$ff90
        sta      1,x       use system mmu
        sta      $10,x     set 0 page
        lda      #$3f
        sta      $17,x     set IO/os9p1 page
        jmp      [<vector,pcr]

        org      $fff0
vector  
        fdb      $ff1f     os9entry
        fdb      $ff03     SWI3
        fdb      $ff06     SWI2
        fdb      $ff09     FIRQ
        fdb      $ff0c     IRQ
        fdb      $ff0f     SWI
        fdb      $ff1f     NMI
vreset
        fdb      reset

