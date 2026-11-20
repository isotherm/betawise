# Recovery-hook cold-boot investigation

Scope: private, disposable MAME flash/RAM only. No physical flash writes, flash
command qualification, applet movement, archive recovery engine or deployable
patcher. No screen acceptance claims: observations are CPU/memory control-flow
traces, with native startup and normal Y/Enter input. Firmware source files are
not modified. Raw logs and source hashes accompany the findings below.

## The proposed NEO13 normal-ROM hook is unsuitable

The byte change at 450182 (FF -> 7F) correctly changes BSR.W 4500FC at 450180
into BSR.W 4580FC. However, both stock and patched cold-boot fixtures registered
applets and reached AlphaWord (sampled PC 472C2C) without executing this call or
the inserted stub. Registration initialization at 4130FC was observed.

Disassembly explains a portability problem: 450180 belongs to routine 450150,
called at 41F94A and 41F98E. Both callers branch around that routine when RAM
041A is 2. A syntactically valid patch is therefore not a universal startup hook.
The no-erase normal-firmware proposal at this address must not be deployed.

## Alternatives in the available firmware

| Image | Hook instruction | One-bit change | Stub location used by fixture |
|---|---|---|---|
| AS3000 System 3.01 | JMP 4C7390 at 4C0086 | Byte at 4C0089: 4C -> 48 | 487390, inside applet space |
| NEO System 3.01 (2005) | JMP 5C6BD0 at 5C0086 | Byte at 5C0089: 5C -> 58 | 586BD0, inside applet space |
| NEO System 3.21 (2013), SmallROM candidate | JSR 46FF98 at 40143E | Byte at 401443: 98 -> 18 | 46FF18, manufacturer-supplied FF space in main system firmware |

The AS3000 and NEO05 targets illustrate a direct jump into a reserved recovery
block without needing a normal-ROM code cave. They are not necessarily the
highest available applet erase blocks. The address must satisfy the bit-clearing
constraint; it cannot be chosen arbitrarily. These examples require their block
to remain reserved for as long as the firmware hook remains installed.

The AS3000 and NEO05 cold-boot fixtures executed the original entry instruction,
then the inserted stub in patched cases, and continued into normal firmware or
AlphaWord. Stock controls did not execute a recovery stub. This validates those
cold-boot control-flow candidates, not all reset/wake/recovery paths.

The stub used for these experiments is only 16 bytes: write RECV marker value
52454356h to disposable RAM 30000h, then JMP to the original destination. Its
marker-write tap establishes execution; the RAM marker is later cleared by
native startup and is not a persistent completion indicator. An attempted read
tap at the original callee did not reliably identify its opcode fetch because
of CPU prefetch/PC observation timing; it is not used as an acceptance condition.
A production stub must preserve the required machine state and contain none of
this test-marker instrumentation.

The NEO13 SmallROM fixture also passed: observed order was candidate 40143E,
recovery marker at PC 46FF22, then registration initialization 4130FC. Both
stock and patched fixtures reached AlphaWord (sampled PC 472C2C). This supports
cold-boot feasibility of the one-bit SmallROM hook while leaving its original
updater routine bytes unchanged. It does not prove updater reachability under
failure or warm-start coverage. The stub is in the existing main-ROM FF region,
not in the unconfirmed physical gap after the SmallROM package.

Eight successful stock/patched traces (including rejection of the original
NEO13 normal-ROM candidate) are retained in `results/`. No ROM bytes are
included there; manifests record the original images and disposable flash hashes.

## Last-block recovery capsule

Erasing and writing an independent recovery capsule before moving applets is
reasonable if the chosen block is confirmed free/owned or all of its existing
live contents already have committed replacements. It means the last block of
an explicitly reserved applet region, not necessarily the physical chip tail:
AS3000 system firmware occupies the physical tail, and NEO firmware packages
also carry identity/calibration material near 5FFFFF.

The capsule should validate its own bounds/version/integrity, copy every needed
instruction and constant to reserved RAM, establish a safe stack, and only then
program/erase the flash. An applet's ordinary registered entry and A5 globals
cannot be assumed available. Journal all relocation state persistently; do not
infer it from equal blocks or partially erased bytes.

Copying recovery to RAM does not make erasing its only flash copy restart-safe.
If a permanent firmware hook jumps there unconditionally, that capsule must
remain intact on flash across every interruption, including between transactions
and after File Manager removal. Updating or moving it needs another bootable
copy and a safe selector, or a permanent small stub that locates versioned
capsules. A bit-cleared hook cannot be restored to its original bits without an
erase. Do not begin applet movement until the capsule and hook are verified.

If using a vendor-side stub, keep its format deliberately small: no transaction
means continue the original path; a valid committed transaction selects a
validated independent recovery entry; a pending transaction with invalid recovery
data must stop safely rather than enter a partially relocated applet chain.
The actual on-device recovery data structures and RAM worker are not implemented.

## Remaining qualification

* Trace warm start, power wake, cold RAM loss and explicit vendor updater entry.
* Verify vendor updater remains reachable when the recovery capsule is missing
  or damaged; leaving its bytes untouched does not by itself prove reachability.
* Test every interruption boundary in a realistic NOR model, including hook
  programming, descriptor commits, sector erase and partial programming.
* Respect physical chip geometry, protection and supply requirements. A single
  bit transition is not proof of atomic programming under power loss.
* Determine firmware integrity-check coverage and effects of system updates.
* Verify stack/A5/register/interrupt contracts and RAM-copy independence.
* Repeat on other firmware revisions by exact image hash; no universal offsets.

## Reproduction

From the Betawise root, select a new private output directory:

```sh
python3 research/flash-b3/recovery-hook/run.py /tmp/flash-hooks-new
```

Optional trailing model names select `neo13`, `neo13small`, `neo05` or `as3000`.
The runner stages full disposable flash images, stock and changed controls,
unchanged loader ROMs, case-specific Lua and source hashes. NEO05 uses a complete
2005 CPU-visible NVRAM image with the 2013 package loader; the emulator's ST NOR
profile remains provisional and does not identify the fitted hardware. AS3000
NVRAM flash serialization uses native little-endian uint16 words on this host.
The tests execute a prepatched image, not the act of programming a hook bit.
