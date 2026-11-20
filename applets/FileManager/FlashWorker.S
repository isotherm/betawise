/* Position-independent blob copied to RAM by FlashRun before each operation.
 * No flash calls or constants while CUI is non-array.
 * Request: op 0, address 4, source 8, length 12, polls 16,
 * completed 20, ID 24, status 26, waitExceeded 27.
 */
.section .text.FlashWorker,"ax",@progbits
.balign 2
.globl _FlashWorker
.type _FlashWorker,@function
_FlashWorker:
    movem.l %d2-%d7/%a2-%a4,-(%sp)
    movea.l 40(%sp),%a2
    move.w %sr,%d6
    ori.w #0x0700,%sr
    move.w 0xfffff110,%d7
    move.w %d7,%d0
    andi.w #0x7fff,%d0
    move.w %d0,0xfffff110
    movea.l #0x400000,%a3
    clr.l 20(%a2)
    clr.w 24(%a2)
    clr.w 26(%a2)
    move.b #0xff,(%a3)
    tst.l (%a2)
    bne.s .Lwrite
    move.b #0x90,(%a3)
    move.b (%a3),24(%a2)
    move.b 1(%a3),25(%a2)
    moveq #0,%d0
    bra.w .Lfinish
.Lwrite:
    movea.l 4(%a2),%a4
    movea.l 8(%a2),%a0
    move.l 12(%a2),%d4
    cmpi.l #2,(%a2)
    beq.s .Lerase
.Lprogram:
    tst.l %d4
    beq.w .Lsuccess
    move.b (%a0),%d5
    cmp.b (%a4),%d5
    beq.s .Ladvance
    move.b #0x50,(%a3)
    move.b #0x40,(%a4)
    move.b %d5,(%a4)
    bsr.s .Lpoll
    tst.l %d0
    bne.w .Lfinish
    move.b #0xff,(%a3)
    cmp.b (%a4),%d5
    bne.w .Lverify_error
.Ladvance:
    addq.l #1,%a0
    addq.l #1,%a4
    addq.l #1,20(%a2)
    subq.l #1,%d4
    bra.s .Lprogram
.Lerase:
    move.b #0x50,(%a3)
    move.b #0x20,(%a4)
    move.b #0xd0,(%a4)
    bsr.s .Lpoll
    tst.l %d0
    bne.s .Lfinish
    move.b #0xff,(%a3)
.Lverify_erase:
    cmpi.b #0xff,(%a4)+
    bne.s .Lverify_error
    subq.l #1,%d4
    bne.s .Lverify_erase
    move.l 12(%a2),20(%a2)
.Lsuccess:
    moveq #0,%d0
    bra.s .Lfinish
.Lverify_error:
    moveq #-6,%d0
    bra.s .Lfinish
.Lpoll:
    move.l 16(%a2),%d2
    move.b #0x70,(%a3)
.Lwait:
    move.b (%a3),%d1
    btst #7,%d1
    bne.s .Lready
    tst.l %d2
    beq.s .Llate
    subq.l #1,%d2
    bra.s .Lwait
.Llate:
    move.b #1,27(%a2)
    bra.s .Lwait
.Lready:
    move.b %d1,26(%a2)
    andi.b #0x7e,%d1
    bne.s .Lstatus_error
    tst.b 27(%a2)
    bne.s .Lwait_exceeded
    moveq #0,%d0
    rts
.Lstatus_error:
    moveq #-4,%d0
    rts
.Lwait_exceeded:
    moveq #-5,%d0
    rts
.Lfinish:
    move.b #0x50,(%a3)
    move.b #0xff,(%a3)
    move.w %d7,0xfffff110
    move.w %d6,%sr
    movem.l (%sp)+,%d2-%d7/%a2-%a4
    rts
.size _FlashWorker,.-_FlashWorker
.globl _FlashWorkerEnd
_FlashWorkerEnd:
.section .note.GNU-stack,"",@progbits
