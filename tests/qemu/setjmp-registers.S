.text
.globl _start
_start:
    lea _GLOBAL_OFFSET_TABLE_,%a5
    jsr test
    move.l %d0,%d1
    moveq #1,%d0
    trap #0

/* Save the test caller's registers, then verify every saved integer register.
 * A successful return through longjmp also exercises SP and the saved PC. */
.globl check_registers
check_registers:
    movem.l %d2-%d7/%a2-%a6,-(%sp)
    lea -52(%sp),%sp
.irp reg,d2,d3,d4,d5,d6,d7,a2,a3,a4,a5,a6
    move.l #0x13579bdf,%\reg
.endr
    move.l %sp,-(%sp)
    jsr setjmp
    addq.l #4,%sp
    tst.l %d0
    bne.s .Lreturned
.irp reg,d2,d3,d4,d5,d6,d7,a2,a3,a4,a5,a6
    move.l #0,%\reg
.endr
    move.l #23,-(%sp)
    pea 4(%sp)
    jsr longjmp
.Lreturned:
    cmpi.l #23,%d0
    bne .Lfailed
.irp reg,d2,d3,d4,d5,d6,d7
    cmpi.l #0x13579bdf,%\reg
    bne .Lfailed
.endr
.irp reg,a2,a3,a4,a5,a6
    cmpa.l #0x13579bdf,%\reg
    bne .Lfailed
.endr
    moveq #0,%d0
    bra.s .Ldone
.Lfailed:
    moveq #1,%d0
.Ldone:
    lea 52(%sp),%sp
    movem.l (%sp)+,%d2-%d7/%a2-%a6
    rts
.section .note.GNU-stack,"",@progbits
