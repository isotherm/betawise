# AS3000 applet flash limit

## Finding

In **AS3000 System 3.01**, the applet-flash limit is not an existing configurable
RAM end pointer. The install and capacity-reporting paths independently load
the immediate ROM constant `0x004C0000`. Scanning, validation and erase-all
have additional hard-coded limits. A user-configurable tail partition therefore
needs firmware changes, not just a write to an existing RAM location.

Scope: full ROM package SHA-256
`732dca63399d883071ce41cc07ad352fa19c506b7b8c307ace16d5c53e6f8708`,
loaded at `0x004C0000`. Addresses below are specific to that firmware, not NEO.
No source ROM, physical device or production firmware was modified.

## Allocation and relevant RAM

The helper at `0x004C2FEE` starts at `0x00408000`, validates each image through
`0x004C3248`, and advances by its header's declared ROM size. It returns the
first invalid-header address as the append position, or zero at its scan limit.
There is no 512-byte alignment step in this loop.

Manager command `06` computes available flash at `0x004C117C..0x004C1182` as
**immediate `0x004C0000` minus append position**. Command `1A` independently
performs the same calculation at `0x004C267C..0x004C2682` for response `58`.
Neither loads a flash-limit variable from RAM.

| RAM | Observed role |
|---|---|
| `0x354E` | Boundary of applet **RAM** allocations, reduced by applet RAM usage |
| `0x0872` | Aggregate file-RAM reservation, subtracted from free RAM |
| `0x044E` | Current flash write cursor, advanced after programming |
| `0x0452` | Start/header of the applet being installed, used for registration |
| `0x042E` / `0x0432` | RAM receive buffer cursor/end |
| `0x0442` / `0x0446` | Outgoing transfer cursor/end |
| `0x0456` | Current Manager callback; rewritten as protocol state changes |

None is a persistent flash allocation ceiling. Changing `0x044E` redirects a
write; it does not constrain subsequent writes. A callback pointer is not an
established durable hook, and File Manager's removable RAM is unsuitable for
a firmware-owned limit that must work before that applet launches.

## Patch map

Define an exclusive **AppletFlashEnd = ArchiveBegin**, aligned to the actual
erase geometry. For a qualified B3 and a 256-KiB tail, this is `0x480000`.
The system OS remains at `0x4C0000`; its mapping must not change.

| Runtime site | Existing behavior | Required change |
|---|---|---|
| `0x004C117C`, Manager `06` | Load physical end into D5 for install admission | Use configured end; reject overflow/out-of-range append and track declared transfer end |
| `0x004C267C`, Manager `1A` | Load physical end into D6 for free-space reply | Report only space before archive; clamp at zero |
| `0x004C3028`, append scan | Immediate stop `0x4BFF78`, physical end minus `0x88` | Stop before archive, retaining header-room requirement |
| `0x004C327E`, header validator | Image start plus ROM size can extend to `0x4C0000` | Require complete image before archive; reject arithmetic overflow |
| `0x004C14F2..0x004C1542`, Manager `0B` | Program buffer `0x046C`, length `0x043A`, at cursor `0x044E`, then advance | Check every block against archive and declared transfer end before programming |
| `0x004C20B0`, Manager `11` erase-all | Loop to immediate `0x4C0000` | Stop ordinary applet erasure before archive; archive reset is separate |
| Cold/warm initialization | No archive-limit initialization found | Load/validate persistent configuration before scans and Manager operations |

The `MOVE.L #constant,Dn` at the first two sites can become an equal-length
`MOVE.L (absolute_RAM_address),Dn`. Thus **adding** a RAM limit is practical,
but the existing firmware must be patched to read it. The scan threshold
needs a corresponding `limit - 0x88` value or helper. No unused system RAM
location has yet been selected or verified safe for these new globals.

Two constant edits demonstrate admission/reporting, but are insufficient for
archive protection. In particular, the program call at `0x004C151A` has no
archive guard: protection should not depend on the host honoring its initial
declared image size. Erase-all also uses an independent physical limit.

Do not globally replace every `0xB8000` or `0x4C0000` byte pattern. Some are
format maxima, progress-bar scales, actual OS addresses, or system-update
destinations. Changes need to follow each value's use.

## Erase and recovery caveats

The native program/erase wrappers at `0x004C2B06` / `0x004C2B5C` copy their
workers into RAM. The erase worker's older Intel-signature branch at
`0x004C2DCE..0x004C2E34` has a **128-KiB main-region step** and old boot-block
boundaries. That is not the supplied B3's 64-KiB geometry. A 64-KiB boundary
could split a native 128-KiB erase operation. Qualified B3 support must agree
across native Manager erase/program paths and File Manager's private driver;
changing endpoint constants alone does not establish chip compatibility.

SmallROM recovery/system updating has separate erase/program paths and RAM
state. It is not protected by the two full-ROM experimental edits. Other
revisions, external programmers and independent update helpers need separate
review. Initially qualify preservation only through the normal Manager path
and require archive export before recovery/firmware updates.

No independent compactor was certified in this pass. Normal applet removal
changes header flags/directory state; the append helper still advances by
header extents. Any desktop repacking workflow that uses erase-all must
inherit its new boundary.

## Persistent preference and resizing

A complete configurable design also needs:

1. Versioned, checksummed, redundant committed configuration at a fixed
   discoverable location outside normal applet erasure.
2. A firmware-owned RAM limit initialized before Manager/scanning, including
   when File Manager is absent or has not run.
3. Validation against identified geometry and existing applet extents before
   activation. Growing the archive must reject or relocate overlapping applets.
4. A safe corrupt-configuration policy: refuse affected writes rather than
   silently reopening an existing archive as free space.
5. A resize procedure that exports/relocates live records before releasing
   archive blocks, and never changes the active limit mid-transfer.

A fixed build-time reservation is a smaller first experiment than configurable
resizing, but still needs all write/erase protection paths handled.

## Executed proof

From the Betawise root, using a new output directory:

```sh
make -C applets/HelloWorld
python3 research/flash-b3/boundary/run.py /tmp/as3000-flash-boundary
```

The runner checks the original ROM hash, creates private temporary copies,
and changes only immediate operands at package offsets `0x117E` / `0x267E`
to `0x480000` in the limited variant. It calls the real packet parser/handler
`0x004C0C3A` with a controlled RAM frame. USB transport is bypassed; firmware
parsing/admission logic is not. No data blocks, flash programming, erases,
or install finalization are performed. MAME's modified-ROM checksum warning
is expected. The original firmware hash remains unchanged.

Observed with HelloWorld ending at `0x408484`:

| Firmware | Returned free flash | Begin a 512-KiB image | Resulting write cursor |
|---|---:|---|---:|
| Stock | `0xB7B7C` | Accepted, response `46` | `0x408484` |
| Two endpoints set to `0x480000` | `0x77B7C` | Rejected, response `88` | `0` |

Reducing RAM `0x354E` from `0x3D7AE` to `0x2D7AE` changed reported RAM from
`0x0390` to `0x0290` units of 256 bytes, but changed **neither free-flash result**.
It is restored before begin-install. Both cases passed. The limited variant
reports exactly 256 KiB less flash and refuses a request that would overlap it.

This proves the accounting/admission sites and distinguishes RAM from flash
allocation. It does not prove complete isolation, erase safety, power-loss
handling, NEO support or physical hardware behavior. No deployable firmware
patch is supplied or installed by this investigation.

Subsequent [manufacturer datasheet verification](updater-chip-geometries.md)
confirms that the 128-KiB branch matches physical Micron MT28F008B3/800B3
and older Intel SmartVoltage geometry; it is not Intel 28F008B3 geometry.
