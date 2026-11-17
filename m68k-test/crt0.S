/* QEMU Linux-user entry point. Model flash and RAM at two load addresses. */
#ifdef QEMU_TEST
.section .text.startup.qemu,"ax"
#endif
.global _start

.extern main

_start:
    | Determine which A5
    lea     (%pc), %a0
    move.l  %a0, %d0
    andi.l  #0x10000000, %d0
    lsl.l   #1, %d0
    ori.l   #0x80000000, %d0
    move.l  %d0, %a5

    | Apply the applet fixups while flash is mapped read-only.

    lea     (__code_start__, %pc), %a6

    move.l  %a5, %a0
.L_got_loop:
    move.l  (%a0), %d0
    beq     .L_got_done
    bsr     .L_do_fixup
    move.l  %d0, (%a0)+
    bne     .L_got_loop
.L_got_done:
#ifdef QEMU_TEST
    move.l  #__reloc_table_offset, %a0
    adda.l  %a6, %a0
#else
    addq    #4, %a0
#endif

.L_rela_loop:
    move.l  (%a0)+, %d0
    bsr     .L_do_fixup
    beq     .L_rela_done
    move.l  %d0, %a1
    move.l  (%a1), %d0
    bsr     .L_do_fixup
    move.l  %d0, (%a1)
    bra     .L_rela_loop
.L_rela_done:

    jsr      (main, %pc)

    | Linux exit status only preserves eight bits. Fold the full mask.
    tst.l    %d0
    sne      %d1
    andi.l   #1, %d1
    moveq    #1, %d0
    trap     #0

    rts

.L_do_fixup:
    tst.l   %d0
    beq     .L_do_fixup_done
    bmi     .L_do_fixup_ram
    add.l   %a6, %d0
    bra     .L_do_fixup_done
.L_do_fixup_ram:
    bclr.l  #31, %d0
    add.l   %a5, %d0
.L_do_fixup_done:
    rts

.section .note.GNU-stack,"",@progbits
