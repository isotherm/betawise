# Candidate native AS3000 flash services

Read-only disassembly checked against the local System 3.01 and SmallROM
packages on 2026-11-19. No flash calls, ID commands or erases were executed.
These are private implementation entry points, not qualified public syscalls.

| Service | System 3.01 address | Observed behavior |
|---|---|---|
| Program wrapper | 4C2B06 | Copies 432 bytes from 4C2BB2 up to 4C2D62 into caller RAM, calls 4D12C4, invokes copied worker, then calls 4D12CC |
| Erase wrapper | 4C2B5C | Copies 648 bytes from 4C2D64 up to 4C2FEC into caller RAM, invokes worker between the same two system helper calls |
| Program worker | 4C2BB2 | Source/destination/count byte loop; Intel-compatible and AMD-compatible command paths |
| Erase worker | 4C2D64 | Erases the containing sector, computes sector begin/end through output pointers; Intel 128-KiB and AMD 64-KiB main-region maps |
| SmallROM program worker | 402C96 | Similar command dispatch and byte loop; not a safe direct ROM call while the chip is busy |
| SmallROM erase worker | 402E42 | Similar sector calculation and command dispatch; must execute from RAM |

The full-ROM workers check magic DEADF00D at RAM 086E and consume/clear it.
SmallROM workers check the same magic at 1C9A. Do not set these and call casually;
reconstruct calling convention and normal caller preconditions first.
Both paths manipulate CSA protection at FFFFF110. The observed worker exit sets
protection rather than preserving an arbitrary incoming complete register state.
The wrapper helper effects, interrupt contract, timeout behavior, and status
semantics require execution-level qualification.

The explicit recognized full-ROM AMD-compatible 1-MiB ID pairs are 01/37,
01/5B, 20/DC, 20/5B, 04/37 and 04/5B, with two alternative unlock-address
patterns. Intel-compatible dispatch recognizes packed 89998999 and 89899D9D,
matching the updater's Micron/older-Intel category. The inspected dispatch does
not show Intel B3 D2/D3 recognition or the B6/EB bottom-device IDs associated with the 512-KiB catalog entries.
Do not assume an arbitrary chip is supported merely because this ROM shipped
with some AS3000 hardware. Failure/unsupported-device status must be verified.

A reasonable FileManager-private backend interface can hide whether operations
come from a version-qualified native wrapper or the existing independent Intel
B3 worker. Keep capacity/sector geometry and archive-bound checks in File Manager.
Even a firmware backend must have its entire erase extent checked against the
owned reservation before issuing a command; validating only the requested byte
address is insufficient. Add AMD-compatible algorithms only where no qualified
native path is available. The list of vendor part names does not require one
separate implementation per name.

Next qualification: verify original ROM hashes; trace wrapper callers to derive
arguments, helper effects and error contract; emulate supported chip IDs and
write/erase behavior with disposable flash; test each map including boundary
refusal, timeout and error handling. NEO needs its own independent analysis.
No production backend change is made by this note.

## ROM string inventory and NEO13 follow-up

A byte/string search of all seven local Ghidra .OS3KOS packages found the full
vendor/part labels in UpdaterReverterROM.OS3KOS:
file offsets 2090h (AMD Am29LV004B), 20A1h (ST M29W004), 20AEh
(Fujitsu MBM29LV004), the 1-MiB AMD/ST/Fujitsu names at 20D8h..212Bh,
and Intel/Micron MT names at 2156h and 2170h. The complete literal-string
inventory, all duplicate offsets, image hash and manufacturer geometry references
are in the [device catalog](updater-chip-geometries.md). These are package offsets,
not established runtime call addresses. Additional copies occur later in this
package. They were not found in the separate AS3000 or NEO main/SmallROM
packages. Main packages do contain generic flash-family/vendor diagnostic text.
The AS3000 full package has supplementary content beyond its first 256 KiB;
file offsets must not be naively converted to main-ROM addresses there.

Only one standalone AS3000 main/SmallROM pair is available in this inventory;
there is no newer standalone AS3000 revision here to compare. The updater/reverter
is a separate additional image and a useful candidate for further analysis.

NEO 2013 System 3 has a newer CFI-based implementation: program wrapper 412D1A
copies 230 bytes (412DCE..412EB4) to caller RAM; erase/query wrapper 412D78 copies
450 bytes (412EB6..413078). Size probe 4130CC calls the latter. The program
worker issues byte CFI command 98h to 4000AA, checks QRY at alternate-byte
addresses starting 400021, reads the primary command-set identifier, and accepts
0002h or 0004h. It then issues AMD-style unlock/A0 commands and programs words.
It does not implement Intel primary command-set 0001h in this inspected program
path. Thus newer NEO firmware is useful for CFI geometry research but is not
evidence that NEO carries an Intel B3, nor a generic Intel-B3 backend.

The 2005 NEO main and SmallROM packages contain the old packed Intel/Micron ID
comparisons; the 2013 NEO packages do not contain those comparison constants.
These are static findings, not execution or physical chip qualification.

## NEO13 bit-clearing recovery-hook candidate (2026-11-20)

Read-only inspection of the local NEO13 packages found real SmallROM calls to
trailer entries, resolving the earlier uncertainty about whether the duplicated
trailers are referenced. SmallROM 40130E calls 46FFA2 after a version check;
40143E is JSR absolute 46FF98 followed by RTS. Older-layout alternatives call
5FFFA2 at 401476 and 5FFF98 at 401486. The earlier literal search for the trailer
base addresses did not cover these interior entry addresses.

At SmallROM file offset 143E, verified bytes are `4E B9 00 46 FF 98`.
Changing only the final byte from 98h to 18h (clearing bit 7) redirects that
call from 46FF98 to 46FF18. The latter is inside the manufacturer package's
confirmed FF run; bytes 46FF18..46FF7F (104 bytes) were checked as FF. A short
trampoline there could jump to a larger recovery stub elsewhere in the run,
then invoke the original 46FF98 entry on the normal path. This is a candidate,
not a validated boot hook or a produced patch. The replacement retains JSR
and its return-address behavior; register and condition-code effects still need
preservation. Trace all startup/reset paths before claiming recovery coverage.

The hook byte is at runtime 401443, inside SmallROM, while the new stub is in
main system firmware. Program and verify the complete stub first, then commit
the hook last, with no applet relocation begun until the installed hook is
qualified. A one-bit logical transition does not establish hardware atomicity
or safe read margins after interrupted programming. Chip protection, repeat
programming limits, firmware integrity checks, exception behavior and physical
voltage remain qualification requirements. Firmware updates can overwrite the
stub or hook; archive migration must then be disabled until recovery is restored.
No physical flash or firmware file was modified.

## Normal-system-ROM alternative (2026-11-20)

A static bit-clearing search also found a real instruction in normal NEO13
system firmware, without modifying SmallROM:

* Runtime 450180 / package offset 40180: `61 00 FF 7A`, BSR.W 4500FC.
* Clear only bit 7 of the byte at 450182: `61 00 7F 7A`.
* New destination: 450182 + 7F7A = 4580FC, inside the confirmed FF run.
* First 64 bytes at destination were checked as FF in the package.

A trampoline at 4580FC could preserve the original BSR return address and
arguments, check a migration transaction, and normally JMP to original 4500FC.
The original callee is an ASIC register-writing helper; the call is in routine
450150, so the fact that it is executable code does NOT yet establish it as a
safe early-boot interception point. Its reachability, invocation frequency,
register/flags contract, and ordering before applet scanning remain unverified.
This is a concrete encoding candidate, not a qualified recovery hook.

The earlier absolute JMP/JSR destination search found no matches from normal
main-segment firmware into the FF run. PC-relative BSR displacement changes
provide additional candidates, as above; raw encoding matches in data must be
rejected by disassembly/control-flow analysis. A verified early-boot candidate
is still required before implementing the scheme.

Prefer a fixed small firmware stub with a versioned recovery descriptor that
locates a validated executable recovery capsule. The capsule must survive outside
all sectors the transaction may erase and must be callable without applet
registration/A5 initialization. Arbitrary scans for FileManager headers are not
sufficient: incomplete/duplicate images need identity, length, integrity and
transaction-generation validation. Updates should write/verify a new capsule
before committing its descriptor and before reclaiming the old capsule.
Normal-system-ROM patching leaves SmallROM's original recovery implementation
intact, but program interruption, firmware checksum behavior, chip protection,
reset-path coverage and firmware-update replacement still require qualification.
No patch image or physical write was produced.

## Executed hook follow-up

The [cold-boot verification](recovery-hook/README.md) rejects 450180 as a universal
NEO13 startup hook: stock and patched images reach AlphaWord without it. It
records successful cold-boot trampolines through the NEO13 SmallROM candidate,
and normal-firmware entry jumps on AS3000 and NEO05. These are control-flow
experiments on prepatched disposable images, not physical programming or a
recovery implementation. Raw traces, hashes and a reproducer are included.
