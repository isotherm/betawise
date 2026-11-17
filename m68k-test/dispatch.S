/* Emulate the firmware with the production flat binary in an RX segment. */
.text
.globl _start
_start:
    lea     image, %a4
    lea     applet_ram, %a5
    move.l  8(%a4), %d0
    beq.s   .Lfail
    cmpi.l  #4094, %d0
    bhi.s   .Lfail
    move.l  %a5, %a0
.Ldirty:
    move.b  #0xa5, (%a0)+
    subq.l  #1, %d0
    bne.s   .Ldirty
    move.b  #0x5a, (%a0)

    move.l  %a4, %a3
    adda.l  0x84(%a4), %a3

    move.l  %a4, %d1
    moveq   #0x18, %d0
    bsr.s   .Ldispatch

    moveq   #1, %d1
    moveq   #0x19, %d0
    bsr.s   .Ldispatch
    moveq   #2, %d1
    moveq   #0x19, %d0
    bsr.s   .Ldispatch

    /* A second INIT must restore the pristine image and reset BSS. */
    move.l  %a4, %d1
    moveq   #0x18, %d0
    bsr.s   .Ldispatch
    moveq   #1, %d1
    moveq   #0x19, %d0
    bsr.s   .Ldispatch

    move.l  %a5, %a0
    adda.l  8(%a4), %a0
    cmpi.b  #0x5a, (%a0)
    bne.s   .Lfail
    moveq   #0, %d1
    bra.s   .Lexit
.Lfail:
    moveq   #1, %d1
.Lexit:
    moveq   #1, %d0
    trap    #0

.Ldispatch:
    move.l  #-1, -(%sp)
    pea     (%sp)
    move.l  %d1, -(%sp)
    move.l  %d0, -(%sp)
    jsr     (%a3)
    lea     12(%sp), %sp
    tst.l   (%sp)+
    bne.s   .Lfail
    lea     applet_ram, %a0
    cmpa.l  %a0, %a5
    bne.s   .Lfail
    rts

.balign 2
image:
    .incbin "smoke.OS3KApp"

.bss
.balign 4
applet_ram:
    .skip 4096

.section .note.GNU-stack,"",@progbits
