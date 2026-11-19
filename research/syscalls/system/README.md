# System-internal selector services

## Scope

`CallSysInt()` / `0xA2B8`, including firmware-generation differences.

Caller contracts remain in [`os3k.h`](../../../os3k/os3k.h). This page records
**evidence and confidence**, not a duplicate normative API reference. No new
reverse-engineering claims are inferred from the documentation reorganization.

## Findings and open questions

| Finding | Status and evidence |
|---|---|
| AS3000 3.01 supports selectors 0000/0300/0400/0500/0600 | ROM dispatch comparison and historical MAME probes. |
| Neo 3.21 scalar and pointer output widths | ROM stores and typed calls; see ABI audit and earlier observations. |
| 3000/5000 scratch arena | Vendor MathFacts/Responder/KeyWords/transfer callers pair pointer and size; controlled vendor allocator execution filled 8192 bytes and rejected another byte. This does not establish exclusive ownership. |
| 8000 returns 1 on Neo 3.21, absent on AS3000 3.01/Neo 3.01 | ROM-generation comparison; **extended-syscall capability probe is a hypothesis**, not a vendor-established meaning. No resolved caller among 54 images. |
| 0D00/0E00 opaque byte flag | Wireless Updater clears it on initialization and sets it after its update flow. Broader policy and consumers remain unresolved. |
| 9000 predicate | Returns 1 when Neo RAM byte 042C equals 1. No resolved caller; CoWriter's literal 9000 was an applet-ID lookup, not this selector. Meaning unresolved. |
| Wireless 0A00/0C00 | No-radio diagnostic path / 13-byte copy observed. Successful transport, all field meanings, and real-radio behavior remain unverified. |
| A000 contrast initialization | Flash programming is attempted, but live persistence was not established. A handled selector is not operation success. |

The Neo 3.21 ROM has 285 table slots through A470; Neo 3.01 ends at
A3B0 with 237 slots. Older dispatch masking can alias higher opcodes. The 8000
hypothesis must not be presented as a guarantee for every extended function.

## Tests and reproduction

Shared ABI probes and vendor inventories remain under abi-audit. There is no
maintained full-radio integration test or exhaustive CallSysInt policy test.

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

< Always zero in the traced Neo implementation.

AS3000 3.01 supports only 0x0000, 0x0300, 0x0400, 0x0500, and 0x0600 below.
Other entries describe the tested Neo 3.21 ROM and are not guaranteed
on older firmware. Caller investigation covered 54 distinct available applet
images after byte-identical copies were deduplicated, including font resources.
Numeric selector names are retained where caller evidence does not establish
a higher-level meaning. Scalar data is a readable or writable uint32_t, including
Boolean values and byte-valued setters. Pointers are written into caller
pointer storage, not returned as the function result. Byte buffers and the
firmware version have the sizes stated on their entries.

< data unused. Draw firmware, wireless, network, MAC, and channel diagnostics.
Performs radio queries and can wait for transport. The no-radio-response path
returned in MAME; successful radio responses remain unverified.

< const uint8_t[13] input: copy into the ROM block, then attempt wireless
command 7 up to 1000 times. Transport completion and field meanings remain
unresolved. Neo only.

< const uint32_t* input: store its low byte in a shared ROM flag. Vendor
Wireless Updater writes 0 on MSG_INIT and 1 after its update path, before
queuing the Applets key. The broader system policy remains unresolved. Paired
with 0x0E00; Neo only.

< uint32_t* output: zero-extended flag written by 0x0D00. No caller identified
in the available vendor applet images. Its relationship to the Wireless Updater
use is known, but its broader consumer/policy remains unresolved. Neo only.

< uint8_t[13] output: copy the opaque ROM block used by 0x0C00. Exactly 13
bytes; field meanings unresolved. Neo only.

< void** output: shared, ROM-owned scratch arena (0x5DBE in the tested Neo ROM).
MathFacts Offline, Responder, Wireless File Transfer, and KeyWords Wireless pair
it with 0x5000 and suballocate consecutive slices. Wireless Updater also uses
the buffer directly. This query does not allocate or grant exclusive ownership;
do not free the pointer. Neo only.

< uint32_t* output: scratch arena capacity in bytes, 8192 in the tested Neo ROM.
Vendor allocators use this as the bound for slices from
SYS_INT_GET_SCRATCH_BUFFER_PTR. Executing the MathFacts allocator in MAME filled
all 8192 bytes and rejected the next byte. Neo only.

uint32_t* output: unconditional 1 in the tested Neo 3.21 ROM,
even without wireless detection. Unsupported by the Neo 3.01
ROM, which returns false and leaves data untouched.
@par Inferred purpose
Likely an extended-syscall capability probe: an applet can query this
through the existing dispatcher before using the extended syscall block.
This interpretation follows the ROM-generation comparison; no confirmed
caller was found in the 54 audited applet images. It does not guarantee
that every extended slot is populated or behaves identically across ROMs.

< uint32_t* output: 1 iff RAM byte 0x042C equals 1. No caller identified in the
available vendor applet images. A 0x9000 argument in CoWriter is an applet ID
passed to AppletFindById(), not this selector. Higher-level meaning remains
unresolved. Neo only.

< const uint32_t* input: low byte initializes the flash contrast bias with
marker 'Y' only if both stored bytes are erased. Does not apply contrast
immediately. Persistence unverified live. Neo only.

Betawise intercepts row-height and row-count queries when its own Neo font
is initialized. All other selectors go to the ROM. The ROM output pointers
are unchecked, so callers must supply valid storage of the documented size.

## Slot 173: retained exception information

A2B4 copies three required 32-bit outputs from AS 3D6E/72/76, Neo3DFE/3E02/3E06.
The exception handler at AS 4D0CDE records DEADBE00 for bus error, DEADAE00 for
address error, DEADCD00 for illegal instruction, DEADD000 for divide by zero.
For bus/address frames it saves the fault address at frame +0A and PC at +02;
for other handled exceptions it saves PC as the second word and zero as the third.
Boot code at 4D069A increments the marker and recognizes the corresponding
low-byte-01 variants. The getter itself has no validation or clearing.
Seeded distinct values and compiled-C output checks are covered by the
[shared fixture](../irda/README.md); a real crash/reboot lifecycle was not induced.

## Empty slots 278–281 (2026-11-18)

Original image hashes were checked against the shared ROM inventory. Direct
big-endian reads and a read-only Ghidra check of the Neo 3.21 dispatch
table agree that these are null pointers, not functions returning zero:

| Slot | Opcode | Neo 3.21 table address | Stored pointer |
|---|---|---|---|
| 278 | A458 | 44F5B6 | 00000000 |
| 279 | A45C | 44F5BA | 00000000 |
| 280 | A460 | 44F5BE | 00000000 |
| 281 | A464 | 44F5C2 | 00000000 |

Neighbor 277 points to 434980; neighbor 282 points to 434B14. This bounds the
four-entry hole within the populated extended table. AS3000 3.01 and Neo 3.01
Neo have only 237 entries (0–236): these indices are outside their dispatch
tables, rather than four allocated zero slots. No null target was executed.
The existing omission of stubs is correct; syscall.c now explains the gap.
