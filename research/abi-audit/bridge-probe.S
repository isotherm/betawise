.text
move.l 12(%sp),-(%sp)
move.l 12(%sp),-(%sp)
move.l 12(%sp),-(%sp)
jsr 0x37f10
lea 12(%sp),%sp
move.l %d0,%a0
rts
