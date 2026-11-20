# NEO firmware gaps, compression and differential archives

## Read-only NEO package findings

The July 2013 NEO System 3.21 package contains a substantial erased-value run:

| Runtime range (inclusive) | Bytes | Evidence |
|---|---:|---|
| `0x452E8E..0x46FF7F` | **119,026** (116.24 KiB) | Actual consecutive `FF` bytes inside the supplied main segment |
| `0x40581A..0x405FFF` | 2,022 | Omitted between SmallROM's end and the next package destination; **not confirmed unused physical flash** |
| `0x406400..0x40FFFF` | 39,936 | Omitted after the rounded identity transfer; **not confirmed unused physical flash** |

The first range is more meaningful than emulator-filled gaps: those `FF` bytes
are actually in the manufacturer package. However, they are still firmware-owned
padding, not a qualified general-purpose archive partition.

On a device with 64-KiB main erase blocks:

```text
450000..452E8D  Nonblank firmware material
452E8E..45FFFF  First part of FF run
460000..46FF7F  Second part of FF run
46FF80..46FFFF  Nonblank 128-byte firmware trailer region
```

Thus the run contains **no complete blank 64-KiB block**. The lower surrounding
block contains firmware; the upper contains a trailer whose bytes match the
trailer at `0x44FF80`. Do not infer that this duplicate is dispensable: no boot,
recovery, checksum or compatibility proof has established that. Appending a
finite journal might eventually be possible after checking write protection
and runtime integrity checks, but recycling it would require rewriting
firmware/trailer data. Firmware replacement would overwrite it as well.

At eight bytes per record the large run has a theoretical 14,878 record slots
before headers and any split-boundary waste. This is capacity only, not a safe
permission to program it. The NEO chip/geometry must be identified independently;
the emulator's ST M29W320EB profile is explicitly a provisional candidate.
The AS3000 B3 worker must not be reused as a NEO flash command implementation.

The November 2005 NEO System 3.01 main package, mapped at `0x5C0000`, has **no
`FF` run of 256 bytes or more** in its 256-KiB main region. The favorable 2013
padding finding does not generalize to all NEO ROM revisions.

Reproduce using existing local private firmware, without writing any ROM:

```sh
python3 research/flash-b3/neo-space.py
```

The script verifies hashes against the existing ABI inventory. The 2013
segments follow the established SmallROM transfer contract implemented in
`mame/src/mame/skeleton/neo_firmware.h`:

* source `00000..5FFFF` -> runtime `410000..46FFFF`;
* source `60000..603FF` -> runtime `406000..4063FF`;
* source `60400..607FF` -> runtime `5FFC00..5FFFFF`.

The header is part of the first transfer, and the secondary logical 20-byte
segment is physically rounded to 1 KiB. Its zero padding is not erased flash.
Neither update package is a complete physical NOR dump; unknown address ranges
are not counted as free storage. No NEO emulator source was changed.

## Compression: worth including in the record format

Archive logical files and their metadata, rather than unused RAM capacities,
when possible. Preserve the native bytes exactly, including formatting tokens,
line endings, passwords and required file metadata. Plain-text export is a
separate conversion, not the representation used for lossless backup.

Use independently compressed chunks, initially evaluating 4-8 KiB. Record the
codec/version, compressed and original lengths, and a checksum of reconstructed
bytes. Store raw chunks when compression plus framing is larger. Validate all
lengths and enforce output limits during decoding; corrupted archive data must
not cause an unbounded allocation or overwrite.

Candidates to measure on the actual 68000 include:

* [heatshrink](https://github.com/atomicobject/heatshrink): embedded LZSS with
  configurable window/lookahead, streaming interfaces and static allocation.
  Its decoder buffer includes a `2^window_bits` history plus input buffering;
  encoder memory and CPU costs must be budgeted separately.
* [LZ4 blocks](https://github.com/lz4/lz4/blob/dev/doc/lz4_Block_format.md): a
  simple byte-oriented format to compare. Independent small blocks bound the
  restore working set; default desktop encoder memory use is not an embedded
  RAM budget, and desktop speed does not predict 68000 performance.

Compression saves erase cycles only when the allocator packs more records into
the available blocks or avoids rewriting unchanged data. Erasing the same
fixed-size snapshot slot on every save would squander much of the wear benefit.
Reserve compaction space based on worst-case incompressible input, not expected
text ratios, and include code/RAM, metadata, CRC and block-fragmentation costs.

## Differential/incremental records

For repeated revisions of a document or program, changes can be far smaller
than even a compressed full snapshot. They do not help a collection of unrelated
files with no shared base. Skip byte-identical versions before writing anything.

A small initial delta representation can express:

```text
base version ID
unchanged prefix length
bytes to remove from base
inserted byte count and inserted bytes
expected reconstructed length/checksum
commit marker written last
```

This one-splice representation handles an append or a localized edit cheaply.
For multiple distant edits it may include much unchanged text; compress the
recipe, use several copy/insert commands, or fall back to a full snapshot.
Avoid a fixed-offset XOR-only design: insertion near the beginning shifts later
bytes and makes a tiny text edit appear to change the whole file.

Two useful base policies are:

* **Differential against a full checkpoint:** one base plus one patch to restore;
  simple dependency graph, but patches grow as more edits accumulate.
* **Incremental against the previous version:** small recent patches, but restore
  walks a chain. Cap the chain, for example at four to seven deltas, and write
  another compressed full checkpoint when the cap or size threshold is reached.

Choose raw full, compressed full or compressed delta by total stored cost,
subject to restore RAM/time limits. A compressed full may be preferable even
if a delta is slightly smaller, to break a long dependency chain.

Every delta names its exact committed base; never depend on a mutable RAM file
or a recycled flash address. Check base identity and verify reconstructed length
and checksum. Append and verify the new payload before committing a new latest
version/index; keep the old valid version intact on interruption.

Garbage collection must retain the complete dependency closure of every kept
version. It cannot erase a base merely because the user has deleted that old
version from the visible history. It can materialize a new compressed full,
verify and commit its manifest, then reclaim dependencies that no retained
version still needs. Historical retention policies and rebasing therefore
affect both utilization and write amplification.

## Executed host sizing experiment

```sh
python3 research/flash-b3/compression/measure.py
```

This uses the already available host Python `lz4` package (observed version
`4.0.2+dfsg`), 8-KiB independent blocks, raw fallback, an assumed 64-byte record
header and assumed 16-byte chunk framing. Those are accounting assumptions,
not an implemented on-device archive format. Inputs are repository Markdown,
not personal AlphaWord documents. No claims about real user compression ratios
or target CPU speed are made.

Executed standalone-file sizes including those assumed overheads:

| Input | Original bytes | Encoded bytes |
|---|---:|---:|
| Betawise README | 2,327 | 1,962 |
| File Manager README | 8,540 | 6,370 |
| neotools README | 3,978 | 2,616 |
| Deterministic incompressible control | 32,768 | 32,896 |

The incompressible case falls back to raw payload and only incurs framing.
The script prints content hashes so results can be distinguished when the
repository documents change.

For 21 synthetic revisions derived from the File Manager README:

| Edit pattern | Raw payload total | Independent compressed full records | Compressed full/delta mix, max seven deltas |
|---|---:|---:|---:|
| Append one short line per revision | 190,890 | 135,935 | 21,970 |
| Replace a localized string per revision | 179,340 | 133,955 | 20,819 |

Each mixed history contains 18 deltas and three full checkpoints. All compressed
payloads were decoded and compared, and every splice recipe was applied and
compared byte-for-byte. These constructed cases intentionally favor deltas;
they demonstrate the opportunity rather than predict general document behavior.
An embedded codec, restore engine, persistent index and GC implementation have
not been added by this experiment.

## Suggested order

First design versioned records with raw fallback, checksums and durable commits.
Then qualify independent compression on real document/source samples and the
target CPU. Add bounded deltas only once full-copy recovery and GC are sound.
The prospective gains are large enough to prioritize this over relying on
small firmware fringes, but neither technique removes the need for safe erase
geometry, sufficient voltage and a separately recoverable archive layout.

## Trailer follow-up and image builder feasibility

The 128-byte trailer is not merely one or two integers. At both `44FF80` and
`46FF80`, its first 38 bytes decode as executable 68000 entry stubs:

```text
MOVEA.L #$0007FFF0,A7
MOVEA.L #0,A6
JMP     $00418A06
JMP     $00418C8C
ANDI.W  #$F0FF,SR
JMP     $00410082
JMP     $00413406
```

The latter part includes version bytes, `System 3 Neo`, and a July 2013 build
stamp. A literal big-endian 32-bit search of the package found no `0046FF80`
or `0044FF80` constants. That does not establish non-use: entry can be computed,
selected by another boot image, or depend on hardware address mapping. The
identical copies make redundancy plausible, but do not prove either removable.

A host image builder is feasible in principle, but adding bytes before existing
firmware is not relocation: absolute jump targets above would still reference
the old addresses. Prefer keeping all firmware addresses fixed and reserving an
aligned applet-space partition, with installation accounting, scanners and all
erase paths honoring it. A leading archive in applet space also needs an explicit
skip rule or a valid reservation applet; an erased hole may terminate enumeration.
The AS3000 installer constants previously investigated do not establish the
corresponding NEO patch sites. A builder would need version/hash-specific patches,
verified readback/backups, recovery qualification and firmware integrity handling.
No modified firmware image has been produced by this investigation.
