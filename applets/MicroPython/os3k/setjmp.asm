.text
.globl setjmp
setjmp:
    movea.l 4(%sp),%a0
    movem.l %d2-%d7/%a2-%a7,(%a0)
    move.l (%sp),48(%a0)
    moveq #0,%d0
    rts

.globl longjmp
longjmp:
    movea.l 4(%sp),%a0
    move.l 8(%sp),%d0
    bne.s 1f
    moveq #1,%d0
1:
    movem.l (%a0),%d2-%d7/%a2-%a7
    move.l 48(%a0),(%sp)
    rts

.section .note.GNU-stack,"",@progbits
