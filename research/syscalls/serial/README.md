# Legacy UART services and callback context

## Scope

Slots 79–87 (A13C–A15C).

Caller contracts remain in [`os3k_serial.h`](../../../os3k/os3k_serial.h). This page records
**evidence and confidence**, not a duplicate normative API reference. No new
reverse-engineering claims are inferred from the documentation reorganization.

## Findings and open questions

| Finding | Status and evidence |
|---|---|
| Nine prototypes and stack widths | Ghidra plus typed C/MAME calls. No syscall-entry pointer bridge needed. |
| Read/write options and errors | Controlled UART samples/writes verify binary bytes, terminators, partial counts, signed errors and timeout arguments. |
| RX/TX callback ABI | Typed arguments match GCC; final empty RX sample and one-shot TX notification checked. |
| A5 context | Complete interrupt disassembly and synthetic exception-frame execution through live RAM vector: inherited A5, no callback-owner switch. |
| Neo legacy routing mismatch | ROM addresses show UART1/UART2 mixed across operations. This is firmware behavior, not a GCC return-register problem. |
| Physical serial/infrared transport | **Unverified.** AS UART transport model is a stub; controlled register tests are not a cable or optical link. |

the disassembly record below records the outer dispatcher
and common UART handler. The test deliberately uses a foreign A5 with callbacks
that do not depend on applet globals. It does not demonstrate safety of an
arbitrary callback, real asynchronous preemption, or correct cancellation on
kill-focus. An inherited A5 can already be right; a trampoline is needed when
that cannot be guaranteed for GOT-dependent code.

Register bit names were compared with the manufacturer's
[MC68EZ328 manual, section 11](https://www.nxp.com/docs/en/reference-manual/MC68EZ328UM.pdf).

## Tests and reproduction

Maintained sources: [C probe](serial-probe.c),
[AS Lua template](serial-as-probe.lua.in), [Neo Lua template](serial-neo-probe.lua.in),
and [builder](make-serial-probe.py). Callback-path and full-width error checks
are part of the common runner. Timed/flow-control exploratory execution is not
proof of physical handshaking.

The maintained combined runner, executed from the workspace root, is:

```sh
python3 betawise/research/abi-audit/run.py /tmp/os3k-syscall-validation
```

Only the cases explicitly described here are covered; running that command is
not blanket certification of this entire subsystem. The shared
[ROM inventory](../../abi-audit/rom-inventory.json),
[slot table](../../abi-audit/slots.tsv), and
[ABI audit](../../abi-audit/README.md) define firmware/ABI scope.

## Additional investigation details

These observations retain their original firmware and test scope; tentative
interpretations are not additional API guarantees.

Syscalls 79–87 (0xA13C–0xA15C). Contracts were traced in the AS3000 3.01 ROM
and Neo 3.21 ROM and exercised in MAME. Transfers are binary; neither
function appends a NUL. Configuration, callbacks, and interrupt masks are
system-wide, so these operations require coordination with other users.

@par Neo compatibility
These legacy entries do not consistently select the same UART on Neo:
configuration of the wired mode, baud changes, status queries, reads, and
receiver disabling address UART1; writes, callback interrupt enabling, and
receiver enabling address UART2. The common callback handler still samples
UART1. Infrared configuration initializes UART2 using a separate ROM handler.
Do not assume the AS3000 3.01 receive/callback workflow works unchanged on Neo.
These differences describe the tested ROM, not a portable channel selector.

@par Verification limits
MAME register checks and controlled UART input/output verified the transfer
branches and counts. AS3000 3.01's UART model has no working serial transport;
those tests injected register samples and captured register writes, rather
than proving communication with another device. Physical cable/infrared
transfer and electrical timing remain unverified.
Hardware bit meanings were checked against the MC68EZ328 User's Manual,
section 11: https://www.nxp.com/docs/en/reference-manual/MC68EZ328UM.pdf

Syscall 0xA140. Enables receive and transmit and automatic RTS flow control.
Errors can leave hardware partially changed; reset/reconfigure after failure.
AS3000 3.01 also reads and discards one receive-register sample during setup.
AS3000 3.01 selects wired or infrared mode using the same UART. The tested Neo
infrared branch instead initializes UART2 to fixed 38400 baud, 8N1, ignoring
CTS; it ignores baud_rate and the other configuration groups, even invalid
ones. Neo's wired branch configures UART1. See this header's compatibility note.

Syscall 0xA148. Reads UART1 on Neo. When unconfigured, AS3000 3.01 writes zero
to both outputs, while the tested Neo writes 0xFF to both. No pointer may be NULL.

Syscall 0xA150. Addresses UART1 on both tested systems. Transmission and
callback registration are retained; a blocking read may therefore still wait.

Syscall 0xA154. Does not enable the UART as a whole or set configured state.
Addresses UART1 on AS3000 3.01 and UART2 on the tested Neo. The sampled byte is
discarded if present; toggling receiver enable can also reset the hardware FIFO.

## Disassembly and caller record

This static evidence is not regenerated by the runtime probes.

```text
Serial callback A5 path — AS3000 3.01 and Neo 3.21
Follow-up to the ABI audit, 2026-11-18.

These instruction listings are from fresh Ghidra Dump.java runs of the original
manufacturer ROMs, with the load bases recorded in rom-inventory.json. Full
functions are included so an A5 write cannot be hidden by a selected excerpt.
The startup vector stores below were checked against the ROM disassembly and
are also asserted against live RAM by serial-*-probe.lua.in.

AS3000 3.01 startup:
  004C8A0A  move.l #0x004CD28E,(0x0110).w
  (interrupt initialization repeats this store at 004CD03C)
  SerialConfigure registers 004D0226 for interrupt mask 4 through 004CD12E;
  the registration stores the function pointer at 0000422E, without saving A5.

Neo 3.21 startup:
  00418A2A  move.l #0x00420310,(0x0110).w
  (interrupt initialization repeats this store at 0041FFEA)
  SerialConfigure's wired path registers 00425260 for interrupt mask 4 through
  00420104; the function pointer resides at 000048A6, without a saved A5.

Neither level-four dispatcher nor either common UART handler writes A5.
The dispatcher branches on UART pending bit 2, fetches the installed function
pointer, calls it with JSR, and ultimately uses RTE. The UART handler pushes
callback arguments and calls the registered RX/TX pointer directly with JSR.
Consequently A5 is inherited from the interrupted context, not selected using
the applet that owns the callback. It can already be correct, but is not
established/restored to a callback-owner GOT by this firmware path.

This conclusion covers this mask-4 common-handler path, not all firmware
callbacks or Neo UART2's distinct infrared protocol interrupt handler.

AS — complete Ghidra instruction listings
004cd28e movem.l {  A4 A1 A0 D7 D1 D0},-(SP)
004cd292 move.l (0xfffff30c).w,D7
004cd296 moveq #0x5c,D0
004cd298 and.b D7b,D0b
004cd29a beq.w 0x004cd33c
004cd29e movea.l #0x4d0518,A4
004cd2a4 btst.l 0x3,D7
004cd2a8 beq.b 0x004cd2ca
004cd2aa tst.l (0x0000422a).l
004cd2b0 beq.b 0x004cd2bc
004cd2b2 movea.l (0x0000422a).l,A0
004cd2b8 jsr (A0)
004cd2ba bra.b 0x004cd2ca
004cd2bc pea (0x2b1).w
004cd2c0 pea (0x404306).l
004cd2c6 jsr (A4)
004cd2c8 addq.l #0x8,SP
004cd2ca btst.l 0x2,D7
004cd2ce beq.b 0x004cd2f0
004cd2d0 tst.l (0x0000422e).l
004cd2d6 beq.b 0x004cd2e2
004cd2d8 movea.l (0x0000422e).l,A0
004cd2de jsr (A0)
004cd2e0 bra.b 0x004cd2f0
004cd2e2 pea (0x2be).w
004cd2e6 pea (0x404318).l
004cd2ec jsr (A4)
004cd2ee addq.l #0x8,SP
004cd2f0 btst.l 0x4,D7
004cd2f4 beq.b 0x004cd316
004cd2f6 tst.l (0x00004232).l
004cd2fc beq.b 0x004cd308
004cd2fe movea.l (0x00004232).l,A0
004cd304 jsr (A0)
004cd306 bra.b 0x004cd316
004cd308 pea (0x2cb).w
004cd30c pea (0x40432a).l
004cd312 jsr (A4)
004cd314 addq.l #0x8,SP
004cd316 btst.l 0x6,D7
004cd31a beq.b 0x004cd33c
004cd31c tst.l (0x00004236).l
004cd322 beq.b 0x004cd32e
004cd324 movea.l (0x00004236).l,A0
004cd32a jsr (A0)
004cd32c bra.b 0x004cd33c
004cd32e pea (0x2d8).w
004cd332 pea (0x40433c).l
004cd338 jsr (A4)
004cd33a addq.l #0x8,SP
004cd33c movem.l (SP)+,{ D0 D1 D7 A0 A1 A4}
004cd340 rte
004d0226 movem.l {  A4 A3 D7 D6},-(SP)
004d022a movea.l #0x43ba,A4
004d0230 tst.l (A4)
004d0232 beq.b 0x004d0270
004d0234 bra.b 0x004d024e
004d0236 moveq #0x0,D0
004d0238 move.b D7b,D0b
004d023a move.l D0,-(SP)
004d023c moveq #0x0,D0
004d023e move.w D7w,D0w
004d0240 asr.l #0x8,D0
004d0242 moveq #0x0,D1
004d0244 move.b D0b,D1b
004d0246 move.l D1,-(SP)
004d0248 movea.l (A4),A0
004d024a jsr (A0)
004d024c addq.l #0x8,SP
004d024e move.w (0xfffff904).w,D7w
004d0252 btst.l 0xd,D7
004d0256 bne.b 0x004d0236
004d0258 moveq #0x0,D0
004d025a move.b D7b,D0b
004d025c move.l D0,-(SP)
004d025e moveq #0x0,D0
004d0260 move.w D7w,D0w
004d0262 asr.l #0x8,D0
004d0264 moveq #0x0,D1
004d0266 move.b D0b,D1b
004d0268 move.l D1,-(SP)
004d026a movea.l (A4),A0
004d026c jsr (A0)
004d026e addq.l #0x8,SP
004d0270 movea.l #0x43be,A3
004d0276 tst.l (A3)
004d0278 beq.b 0x004d02b6
004d027a move.l #-0x6fa,D6
004d0280 movea.l D6,A0
004d0282 move.b (A0),D7b
004d0284 moveq #0x0,D0
004d0286 move.b D7b,D0b
004d0288 moveq #0x0,D1
004d028a move.b (0x000043c4).l,D1b
004d0290 and.l D1,D0
004d0292 beq.b 0x004d02b6
004d0294 movea.l (A3),A4
004d0296 clr.l (A3)
004d0298 move.b #-0x2,D0b
004d029c and.b (A0),D0b
004d029e move.b D0b,(A0)
004d02a0 move.b #-0x48,D0b
004d02a4 and.b (0xfffff901).w,D0b
004d02a8 move.b D0b,(0xfffff901).w
004d02ac moveq #0x0,D0
004d02ae move.b D7b,D0b
004d02b0 move.l D0,-(SP)
004d02b2 jsr (A4)
004d02b4 addq.l #0x4,SP
004d02b6 movem.l (SP)+,{  D6 D7 A3 A4}
004d02ba rts

NEO — complete Ghidra instruction listings
00420310 movem.l {  A4 A1 A0 D7 D1 D0},-(SP)
00420314 move.l (0xfffff30c).w,D7
00420318 moveq #0x5c,D0
0042031a and.b D7b,D0b
0042031c beq.w 0x004203be
00420320 movea.l #0x425562,A4
00420326 btst.l 0x3,D7
0042032a beq.b 0x0042034c
0042032c tst.l (0x000048a2).l
00420332 beq.b 0x0042033e
00420334 movea.l (0x000048a2).l,A0
0042033a jsr (A0)
0042033c bra.b 0x0042034c
0042033e pea (0x2b1).w
00420342 pea (0x4459aa).l
00420348 jsr (A4)
0042034a addq.l #0x8,SP
0042034c btst.l 0x2,D7
00420350 beq.b 0x00420372
00420352 tst.l (0x000048a6).l
00420358 beq.b 0x00420364
0042035a movea.l (0x000048a6).l,A0
00420360 jsr (A0)
00420362 bra.b 0x00420372
00420364 pea (0x2be).w
00420368 pea (0x4459bc).l
0042036e jsr (A4)
00420370 addq.l #0x8,SP
00420372 btst.l 0x4,D7
00420376 beq.b 0x00420398
00420378 tst.l (0x000048aa).l
0042037e beq.b 0x0042038a
00420380 movea.l (0x000048aa).l,A0
00420386 jsr (A0)
00420388 bra.b 0x00420398
0042038a pea (0x2cb).w
0042038e pea (0x4459ce).l
00420394 jsr (A4)
00420396 addq.l #0x8,SP
00420398 btst.l 0x6,D7
0042039c beq.b 0x004203be
0042039e tst.l (0x000048ae).l
004203a4 beq.b 0x004203b0
004203a6 movea.l (0x000048ae).l,A0
004203ac jsr (A0)
004203ae bra.b 0x004203be
004203b0 pea (0x2d8).w
004203b4 pea (0x4459e0).l
004203ba jsr (A4)
004203bc addq.l #0x8,SP
004203be movem.l (SP)+,{ D0 D1 D7 A0 A1 A4}
004203c2 rte
00425260 movem.l {  A4 A3 D7 D6},-(SP)
00425264 movea.l #0x5d9c,A4
0042526a tst.l (A4)
0042526c beq.b 0x004252aa
0042526e bra.b 0x00425288
00425270 moveq #0x0,D0
00425272 move.b D7b,D0b
00425274 move.l D0,-(SP)
00425276 moveq #0x0,D0
00425278 move.w D7w,D0w
0042527a asr.l #0x8,D0
0042527c moveq #0x0,D1
0042527e move.b D0b,D1b
00425280 move.l D1,-(SP)
00425282 movea.l (A4),A0
00425284 jsr (A0)
00425286 addq.l #0x8,SP
00425288 move.w (0xfffff904).w,D7w
0042528c btst.l 0xd,D7
00425290 bne.b 0x00425270
00425292 moveq #0x0,D0
00425294 move.b D7b,D0b
00425296 move.l D0,-(SP)
00425298 moveq #0x0,D0
0042529a move.w D7w,D0w
0042529c asr.l #0x8,D0
0042529e moveq #0x0,D1
004252a0 move.b D0b,D1b
004252a2 move.l D1,-(SP)
004252a4 movea.l (A4),A0
004252a6 jsr (A0)
004252a8 addq.l #0x8,SP
004252aa movea.l #0x5da0,A3
004252b0 tst.l (A3)
004252b2 beq.b 0x004252f0
004252b4 move.l #-0x6fa,D6
004252ba movea.l D6,A0
004252bc move.b (A0),D7b
004252be moveq #0x0,D0
004252c0 move.b D7b,D0b
004252c2 moveq #0x0,D1
004252c4 move.b (0x00005da6).l,D1b
004252ca and.l D1,D0
004252cc beq.b 0x004252f0
004252ce movea.l (A3),A4
004252d0 clr.l (A3)
004252d2 move.b #-0x2,D0b
004252d6 and.b (A0),D0b
004252d8 move.b D0b,(A0)
004252da move.b #-0x48,D0b
004252de and.b (0xfffff901).w,D0b
004252e2 move.b D0b,(0xfffff901).w
004252e6 moveq #0x0,D0
004252e8 move.b D7b,D0b
004252ea move.l D0,-(SP)
004252ec jsr (A4)
004252ee addq.l #0x4,SP
004252f0 movem.l (SP)+,{  D6 D7 A3 A4}
004252f4 rts
```
