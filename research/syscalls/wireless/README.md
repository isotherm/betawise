# Cached wireless receive status, slot 284

## Contract and ROM scope

A470/284 points to 0x43F63C in the inspected Neo 3.21 image. The complete body
is `move.w (0x43E).l,D0; rts`: no parameters, a 16-bit result, no clearing or
other side effects. It is outside the 237-entry AS3000 3.01 and Neo 3.01 tables.
The prototype is `uint16_t WirelessGetLastReceiveStatus(void)` in
[os3k_wireless.h](../../../os3k/os3k_wireless.h).

## Producer trace

The wireless receive processor at 0x43F542, called by the system pump at
0x417CD0, handles its state 2 by calling 0x43ED3E with buffer 0x134D1. That
routine exchanges SPI bytes, searches for A5, reads a big-endian packet length
at offsets 2–3, and validates an additive byte checksum. The processor's
ordinary-data branch at 0x43F5D4 zero-extends byte 0x134D8 (received packet
offset 7), ORs FF00 into it, and stores it at 0x43E. Control/status paths can
bypass that store. The separate getter reports a cached value, not freshness,
a hardware-presence test, or a syscall-availability query.

The low-byte protocol meanings remain unresolved: no semantic enum is
introduced. Do not assume FFxx is a signed failure merely because of the high
byte, or that each received packet necessarily replaces it. The exact producer
branch conditions remain relevant to future wireless research.

## Live tests and reproduction

The [speech probe runner](../speech/run-probe.py) contains the combined tail-slot
fixtures so test infrastructure is maintained once. Its Ghidra script also
includes this getter and the receive processor.

Four direct getter fixtures cover 0000, 1234, FF80, FFFF, checking word width
and retention. Selected continuation fixtures initially isolated the store
with packet bytes 00, 12 and FF. Full entry-point receive fixtures subsequently
execute 0x43F542 and its original SPI/parser helpers, providing synthetic GPIO
ready/ack and SPI samples through MAME read taps. Checksum-valid nine-byte
packets with byte 7 equal to 12, 80 and FF produce FF12, FF80 and FFFF; the
following syscall reads agree. A no-packet fixture leaves an existing FF42
unchanged. No ROM routine is replaced. These are controlled protocol samples,
not physical radio delivery or a complete wireless callback/API investigation.
Compiled C also verifies the public word-return signature.

The shared [ROM inventory](../../abi-audit/rom-inventory.json) identifies the
original images and hashes. No resolved direct vendor caller was found by the
[tail-stub scan](../speech/find-vendor-calls.py); unsupported instruction or
export patterns and indirect calls are not ruled out.

## Slots 237–246 and 270: packet, association and transfer services

These were freshly traced in Neo 3.21; all are outside the AS3000 3.01/Neo 3.01
237-entry dispatch tables. Native scalar returns are narrower than Ghidra's
initial inferred types in several entries.

| Slot | Target | Clean-room name | Native arguments / result |
|---:|---|---|---|
| 237 | 43ED3E | WirelessReceivePacket | Receive storage pointer; signed low word: frame length, 0 no data, FFFE request failure, FFFD no synchronization, FFFC checksum failure, or 4 handshake timeout. |
| 238 | 43ECEE | WirelessSendPacket | Command word, payload pointer, payload-length word, flags byte; low-byte status. Six attempts with a centisecond delay after failure. |
| 239 | 43DACA | WirelessIsAvailable | No arguments; cached availability byte from 12C9A. The wrapper body tail-calls 43E86E and retains its D0 byte. |
| 240 | 43DAD2 | WirelessIsOperationPending | No arguments; canonical boolean from association state 11944 != 0. |
| 241 | 43DAE2 | WirelessBeginAssociation | Callback pointer; void. If idle, stores callback at 1215C and selects manager state 5. Busy calls leave both unchanged. |
| 242 | 43DAFC | WirelessBeginDisassociation | Callback pointer; void. Idle selects state 7 and synchronously draws status through 43E896, including SleepCentiseconds(200). |
| 243 | 43EFE8 | WirelessSynchronizeDeviceInfo | No arguments; low byte 0 success, 1 failure. Exchanges command 6/eight device bytes and command 7/twelve bytes from the opaque block queried by CallSysInt selector 0F00, with deadlines. |
| 244 | 43F29A | WirelessWriteTransfer | Data pointer, requested word length; submitted word count or 8 not ready, 9 timeout, 10 oversize framing. |
| 245 | 43F508 | WirelessReadTransfer | Output pointer; copies cached block length from 134D9/134DA out of 134DB, returns low-word length. No capacity or terminator. |
| 246 | 43F0D8 | WirelessConfigureTransfer | Mode word (0802 send, 0803 receive, FFFF reset), subtype byte, 32-bit size, callback pointer; low-byte status 0/3/5/8. |
| 270 | 43496A | WirelessCopySelectedNetworkId | Output string pointer, void; strcpy from 12C70. This belongs with wireless despite neighboring dialog entries. |

### Packet format and transport boundaries

The shared transmitter at 12CA4 builds an A5 sync byte, flags byte, big-endian
total-length word, big-endian command word, payload and additive checksum.
The checksum makes the sum of all transmitted bytes zero modulo 256. The
payload-size guard is 2058 bytes. NULL payload retains already-prepared bytes
in shared storage rather than sending an empty payload. Command bit 15
suppresses the separate acknowledgment exchange. The receive parser searches
for A5, reads the advertised length and verifies the byte checksum; its
maximum accepted advertised length is 2068. There is no caller capacity
argument: the public header states the necessary allocation, not a promise of
robustness against every malformed or inconsistent frame length.

Transfer data after the six-byte packet header uses a status/event byte at
packet offset 6, a byte at offset 7 cached by slot 284, then a two-byte data
length at offsets 8–9 and copied data beginning at offset 10. The meanings of
all protocol event/tag values remain incomplete. No guessed exhaustive
protocol-status enum was added.

### Association and transfer lifecycle

Association manager 43D84C invokes the retained callback with three four-byte
stack slots: manager phase 5/6/7, completion boolean, and borrowed network
information at 12448 (or zero on its failure path). It clears the state and
callback after notification. No callback-owner A5 switch occurs. Manufacturer
UI strings include “Disassociating from %s” and “Not associated to a Network”.
The availability cache is set when initialization recognizes a module version
in the supported range; it is not the current association flag.

The selected record is copied as 42 bytes from the network list into 12C70
before command 0201 is sent. Its string at 12C81 is used in the disassociation
message. Slot 270 copies the *leading* string, not that later display name.
The complete 42-byte record layout/field meanings are not yet established;
its borrowed callback pointer remains const void* in the public interface.

Transfer send state is at 134B0; receive state at 134CA. Configure's guard is
`send == 0 || receive == 0`, so an active upload can be replaced while the
receive engine is idle. Only when **both** are active does the ordinary setup
path return busy (8). This is actual firmware behavior; callers must coordinate
ownership. FFFF bypasses that check and performs synchronous transport reset.
Upload setup stores the full byte count, an eight-bit ceil(size/2048) block
counter (which can wrap), subtype, callback, and command. Successful writes
advance the offset and change the send state from 1 to 3. Their numeric error
codes can collide with small successful byte counts.

Send-pump 43F3BE requests data by callback(offset, length, 11) and interprets
its low-byte return 9 as a timeout/error. Receive-pump 43F542 calls the same
shape with received frame length or a zero status length. Completion uses
zero, transport timeout uses 9, and protocol event bytes can be forwarded.
The byte/word callback argument widths agree with compiled GCC callbacks;
all occupy four-byte stack slots and inherit A5. No claim is made that arbitrary
GOT-dependent callbacks are safe across applet focus changes.

### Maintained verification

```sh
python3 betawise/research/syscalls/wireless/find-vendor-calls.py
python3 betawise/research/syscalls/wireless/run-probe.py /path/on/disk/wireless-probe
```

`InspectNeoTail.java` defaults to every original target in slots 237–277 and
prints both Ghidra instructions and inferred C. Run against the Neo 3.21 image
mapped at 410000. Inferred void/wide returns were checked against the actual
register writes and manufacturer uses, not accepted automatically.

The directed suite passes 50 cases: cached getters, busy/no-op behavior,
association state transitions, flags/command/length/checksum framing, native
SPI receives, no-packet handling, oversized send failure, synchronization
failure and controlled success, valid native ACK parsing, receive-request
setup and checksum rejection, upload framing/state/offset, cached binary copy lengths 0/1/3/256,
invalid/busy/setup modes, transfer block-count boundaries, native association
completion callbacks, and compiled C calls/callbacks. GPIO and SPI read taps
provide controlled electrical samples; no ROM routine is replaced. Slot 242's
status path and delays run natively. Slot 270's cached-string copy is included
in the dialog runner because it shares the same tail-range fixture.

Actual association with an access point, a peer wireless transfer, successful
initialization handshake with a real module, cancellation/reset across applet
focus changes, and a complete malformed-packet audit remain outside coverage.
These limits are not claims about physical radio behavior.

### Manufacturer evidence

The hash-checked export scan recognizes A3B4/A3B8/A3BC and validates each tail
stub independently. Fourteen original images export the block; 673 reference
candidates resolve into slots 237–277. MathFacts RP/offline, Responder, Wireless
File Transfer, Accelerated Reader and KeyWords Wireless call transfer setup,
read/write and association services. Wireless Updater directly calls 243.
The scan is a caller inventory, not proof that unobserved/indirect calls do not
exist. Representative candidates were independently disassembled to verify
adjacent move-immediate/indexed-PC target arithmetic and surrounding stack
arguments. Research does not use imported SDK or reverse-engineering names.

The successful synchronization fixture supplies one data response plus native
six-byte ACK frames. It confirms that 243 can return success after sending
both commands without consuming a distinct second data response: its first
nonzero receive-length variable is not reset before the second receive loop.
Do not interpret its zero result as proof of two independent replies.

The suite was repeated successfully after the Neo low-SRAM/high-MMIO mapping
correction (2026-11-18); no firmware target was patched for that regression.
