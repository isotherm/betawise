/* OS3K supplies A5 and the normal C argument frame on each dispatch. */
.section .text.Os3kEntry,"ax"
.globl Os3kEntry
.type Os3kEntry,@function
Os3kEntry:
    cmpi.l  #0x18, 4(%sp)           /* MSG_INIT */
    bne.s   .Ldispatch

    movem.l %d2/%a2-%a3,-(%sp)
    lea     Os3kEntry(%pc), %a2
    suba.l  #__os3k_entry_offset, %a2

    move.l  %a2, %a0
    adda.l  #__os3k_image_offset, %a0
    move.l  %a5, %a1
    move.l  #__os3k_image_size, %d0
.Lcopy:
    tst.l   %d0
    beq.s   .Lclear_setup
    move.b  (%a0)+, (%a1)+
    subq.l  #1, %d0
    bra.s   .Lcopy

.Lclear_setup:
    move.l  %a5, %a1
    adda.l  #__os3k_bss_offset, %a1
    move.l  #__os3k_bss_size, %d0
.Lclear:
    tst.l   %d0
    beq.s   .Lgot_setup
    clr.b   (%a1)+
    subq.l  #1, %d0
    bra.s   .Lclear

.Lgot_setup:
    move.l  %a5, %a3
    move.l  #__os3k_got_size, %d2
.Lgot:
    /* A zero GOT word can refer to the header at flash offset zero. */
    tst.l   %d2
    beq.s   .Ltable_setup
    move.l  (%a3), %d0
    bsr.s   .Lrebase
    move.l  %d0, (%a3)+
    subq.l  #4, %d2
    bra.s   .Lgot

.Ltable_setup:
    move.l  %a2, %a0
    adda.l  #__os3k_fixups_offset, %a0
.Lfixup:
    move.l  (%a0)+, %d0
    beq.s   .Linitialized
    move.l  %a5, %a1
    adda.l  %d0, %a1
    move.l  (%a1), %d0
    bsr.s   .Lrebase
    move.l  %d0, (%a1)
    bra.s   .Lfixup

.Linitialized:
    movem.l (%sp)+,%d2/%a2-%a3
.Ldispatch:
    /* A full-width PC-relative offset keeps this independent of ROM size. */
    lea     .Ldispatch_base(%pc), %a0
.Ldispatch_base:
    adda.l  #BwProcessMessage-.Ldispatch_base, %a0
    jmp     (%a0)

.Lrebase:
    tst.l   %d0
    bmi.s   .Lram
    add.l   %a2, %d0
    rts
.Lram:
    bclr    #31, %d0
    add.l   %a5, %d0
    rts
.size Os3kEntry, .-Os3kEntry

.section .note.GNU-stack,"",@progbits
