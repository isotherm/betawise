# AS3000 flash backup and archive options

## Recommendation

Use a separate **Backup** applet for a first full-RAM recovery image, if that
feature is wanted. For everyday per-file archive/unarchive/delete behavior,
use a versioned append-only record store with one spare erase block and
copy-before-erase collection. Keep it independent of the File Manager UI so
other applets can share the storage layer. Start with copy-to-archive and
restore, then enable automatic removal from RAM only after durable commit and
file metadata/import policies are qualified.

Do not implement automatic shutdown backup yet. Reliable power-loss behavior,
board flash controls, and a recoverable archive format matter more than adding
a power-off hook.

## Hardware facts and limits

Source: the user-supplied Intel **28F008B3 / 28F016B3 Smart 3 Advanced Boot
Block Byte-Wide** preliminary datasheet, May 1997 PDF, SHA-256
`17efc2631f394ae00ed05e4f8c9ded3e247177f68c68eb3d1884839a6d82e8cc`.
Local source: `/home/kirk/Downloads/28F008B3.PDF`. Page references below are
the document's printed pages. The PDF was inspected as reference data only.

* **1 MiB, x8 device** for 28F008B3, not a word-wide 28F800B3. Manufacturer
  `89`, top boot device `D2`, bottom boot device `D3` (Table 5, p.17).
* **Fifteen 64-KiB main blocks plus eight 8-KiB parameter blocks** (section 2.2,
  pp.11-13). Bottom boot puts parameter blocks at chip offsets `00000..0FFFF`;
  top boot puts them at `F0000..FFFFF`. The orientation is not given by the
  board diagram's abbreviated part number; identify it instead of guessing.
* Erase operates on a **whole physical block**, setting every bit to 1.
  Programming clears selected bits from 1 to 0. A normal `memmove` cannot
  implement flash compaction: it cannot turn a programmed 0 into 1.
* The supplied revision specifies **10,000 program/erase cycles per block**
  (Table 1, p.5; section 2.2). Do not substitute a newer B3 family's 100,000-cycle
  specification without identifying the actual silicon and applicable revision.
* VCC/VPP programming operation is specified at 2.7-3.6 V. At 12-V VPP,
  the supplied document limits exposure to **1,000 main-block cycles**,
  **2,500 parameter-block cycles**, and **80 total hours** at 12 V
  (section 3.4 and DC-characteristic footnotes). A 12-V production-programming
  option is not permission to assume a 10,000-cycle daily archive at 12 V.
* Neither the chip label nor its datasheet proves how this board drives VPP,
  WP#, or reset. Low VPP/protection failures must be surfaced, not retried
  indefinitely as if they were media space exhaustion.

The AS3000 driver maps flash at `400000..4FFFFF`, 256 KiB RAM at
`000000..03FFFF`, applets at flash offsets `08000..BFFFF`, and the full OS at
`C0000..FFFFF`. That is **736 KiB of nominal applet image space**, shared by
all installed applets, not free archive capacity. This layout excludes both
ends of the chip from general archive use. The new driver deliberately uses
only main blocks at CPU addresses **410000..4BFFFF** (at most 704 KiB/11 blocks,
before subtracting other applets and the archive applet's placement).

An `0xFF` span is not proof that it is available. It can be padding inside an
installed applet, another applet's allocation, or part of a block containing
code. The earlier in-image design would require a pool declared in the owning
applet's image/header size, before its footer, with only whole erase blocks.
That design can waste almost 64 KiB aligning an arbitrarily placed image.
It is no longer implemented: there is no padded archive array or custom linker.
The preferred configurable tail partition needs firmware-enforced ownership;
see the [firmware boundary investigation](firmware-boundary.md). Until that
policy exists, production code authorizes no archive storage.

## Design choices

| Option | Behavior | Advantages | Costs and failure behavior |
|---|---|---|---|
| Single fixed RAM snapshot | Erase the same slot and save 256 KiB plus manifest/checksum | Smallest conceptual recovery feature; good in a separate Backup applet | Old backup is destroyed before the new one commits; four payload blocks plus metadata, typically five reserved blocks; long pause and concentrated wear |
| Alternating A/B RAM snapshots | Write the inactive slot, verify, commit its generation, then retire the old slot | Last committed backup survives interruption; simple recovery selection | Roughly twice the storage; full 256-KiB slots with in-slot headers commonly need five blocks each, consuming ten of the eleven eligible main blocks before placement/code/other applets |
| Circular snapshot slots | Rotate among N independent committed slots | History and erase wear spread across N slots | Full raw-RAM slots are too large for a useful multi-slot ring in a 1-MiB AS3000; more practical for compressed or logical-file snapshots; compression worst case still needs capacity |
| Append-only file backup, restore-only UI | Append immutable named/versioned file records; expose list/restore/export; clear the archive explicitly when full | No individual compactor needed initially; many small saves per erase; old versions retained | Fills eventually; clearing must be explicit and loses history; restore should create a copy and resolve collisions |
| Managed per-file archive (TI-like) | Archive frees RAM after verified commit; unarchive restores to RAM; delete appends a tombstone | Closest to a user-managed extension of RAM; can reclaim obsolete versions | Needs durable metadata, generations, free-space accounting, a spare block, and power-fail-safe garbage collection; unarchive can fail if RAM is full |
| Hybrid checkpoint plus change log | Periodically checkpoint files/index, append changed files between checkpoints | Lower write amplification than full images; efficient lookup/recovery | Most complex format and recovery; avoid one repeatedly erased metadata block; checkpointing still requires spare capacity |

A raw RAM dump is a **recovery image**, not automatically a safe machine
hibernation image. CPU registers, peripherals, active applet allocation,
firmware version, and flash layout may differ at restore. The RAM writer itself
uses stack and RAM code; capturing a coherent image needs a defined quiescent
state and treatment of that workspace. Whole-RAM restore must run through a
recovery/bootstrap path, not overwrite the currently executing applet's stack
and file tables. The present programming API accepts ordinary RAM sources
starting at `0x400`; it does not implement a coherent vector-inclusive RAM
snapshot or restore engine. Logical file archives are more portable between
applet updates and allow individual recovery.

## Compaction and deletion

Restore-only storage can omit collection: append until full, then offer explicit
export/reset. Deletion can hide entries without reclaiming bytes; that is a valid
first version if the UI clearly distinguishes logical deletion from free space.

For TI-like management, reserve at least **one already-erased 64-KiB spare block**:

1. Select a victim containing obsolete records and enough reclaimable space.
2. Copy its live records into erased space; write fresh record headers, lengths,
   owner applet ID, name, tag, generation, checksums, and separate commit bytes.
3. Verify the new records and durably commit the relocation before retiring any
   source. Recovery tolerates both copies and chooses committed generations.
4. Erase the old victim only once its live data has a durable replacement.
   It becomes the next spare. Interrupted erasure must be recoverable from the
   committed copies; an incompletely erased block is not allocatable.

Source and destination cannot be the same erase block. Keep the index rebuildable
from records or redundantly checkpoint it; a single hot superblock defeats wear
leveling. A full pool of live data cannot be compacted into more capacity. Small
files should share log blocks, not each consume a 64-KiB erase unit. Whole-record
placement simplifies collection; large files require chunk records with a
manifest committed last. Program each commit/tombstone byte separately from
data and avoid relying on CPU multi-byte stores being power-fail atomic.

For archive/unarchive, commit and verify the archive **before deleting RAM**.
For unarchive, finish the RAM restore and metadata setup before tombstoning the
archive. Interruption may leave duplicates; it must not leave neither copy.
Treat passwords/protected files and applet-specific encodings explicitly.

## Wear if a full backup happened at every power-off

These calculations use the supplied **10,000-cycle** value at the applicable
low-voltage programming conditions. They are simple cycle-budget estimates,
not a calendar-life guarantee; initial wear, temperature, retention, incomplete
operations, metadata hotspots, and hardware quality can shorten useful life.

Assume every backup erases each block of the same fixed snapshot slot once:

| Backups/power-offs per day | Cycles/year per slot block | 10,000 cycles reached after |
|---:|---:|---:|
| 1 | 365 | 27.4 years |
| 4 | 1,460 | 6.85 years |
| 10 | 3,650 | 2.74 years |
| 20 | 7,300 | 1.37 years |
| 50 | 18,250 | 200 days |

Four payload blocks erased together each consume **one cycle each per backup**;
the slot does not reach its limit after 10,000/4 backups. Alternating two slots
approximately doubles backups before the same per-block budget is reached, if
no shared metadata block wears first. An N-slot ring provides an analogous
N-fold distribution. Appending small records amortizes erase cycles over all
records that fit, but collection adds write amplification; actual benefit
depends on live-data occupancy, record size, and update frequency. Skip
unchanged backups and avoid erasing a metadata block on every shutdown.

At 12-V VPP, apply the much smaller main-block budget: for example 1,000 main
block cycles at ten backups/day is about **100 days**, subject also to the
80-hour exposure limit. This is why board voltage/wiring qualification matters.

## Latency and power-off concerns

Table 17 (p.44), **VPP=2.7 V column**, excluding system overhead:

| Operation | Typical | Listed maximum (sampled, not 100% tested) |
|---|---:|---:|
| One-byte program | 17 us | 165 us |
| Full 64-KiB main-block programming | 1.23 s | 3.69 s |
| Main-block erase | 1.8 s | 8.0 s |
| Parameter-block erase | 1.0 s | 5.0 s |

Four erased/reprogrammed full payload blocks therefore total **12.12 s typical**
or **46.76 s using the listed maxima**, before verification, record metadata,
interrupt handling, and software overhead. An in-slot manifest that requires
a fifth erase block adds another block erase (not necessarily another full
block's worth of programming). A fresh append into pre-erased space avoids the
erase delay, but a complete 256-KiB payload still needs programming time.
Actual voltage, board clock, chip history and algorithms can change timing.

Battery removal or brownout is not a cooperative shutdown event. A manual
power button could defer shutdown while a backup finishes, but a low battery
may not sustain the program/erase current. The old committed backup must stay
intact until a new one verifies. Running with interrupts masked for seconds
also blocks the firmware's normal timing, keyboard, USB, and power handling.
The first driver is intentionally synchronous; qualified suspend/resume,
watchdog management and stable-power checks are needed before integrating it
with normal interactive or power-off behavior.

## Implemented foundation and qualification boundary

File Manager's private [`Flash.h`](../../applets/FileManager/Flash.h),
`Flash.c` and self-contained `FlashWorker.S`
assembly worker provide geometry, partition opening/identification,
bounded reads, preflighted byte programming, complete-block erase, verification,
progress and raw status reporting. They use no invented syscall. They reject
NEO, unknown flash IDs, unexpected bus mappings, 0-to-1 requests, unaligned erase
starts and cross-pool accesses. Production opening currently refuses storage;
only the research build authorizes a scoped virtual region in MAME.

Both builds use the unchanged standard SDK linker and CRT. `FlashRun` explicitly
copies the ROM worker into a temporary aligned stack buffer for each operation;
there is no special RAM section, permanent worker copy or in-image reservation.
The SDK library and public headers contain no flash driver. The worker masks interrupt levels
1-6, saves/restores the complete CSA and SR, and returns to read-array mode
before executing a flash return address. It cannot tolerate NMI, exceptions,
watchdog reset, or another flash user while busy. Its polling budget is a
**soft overrun indicator**: after expiry it still waits for ready before it can
safely return. A stuck device can hang indefinitely. There is no unverified
reset-pin toggle, voltage switch, or hard timeout that returns into busy flash.

The AS3000 SmallROM byte-program/erase paths at `402C96` and `402E42` show
byte-wide command access and temporary CSA bit-15 clearing. They recognize
older signatures including `89998999` and `89899D9D`; their old boot-block
boundaries are **not** the B3 geometry. Their presence does not provide a
portable public syscall or prove B3/board support. Do not copy their erase
sizes into this driver. NEO requires its own identified chip and board contract.

The processor reference is the
[NXP MC68EZ328 User's Manual](https://www.nxp.com/docs/en/reference-manual/MC68EZ328UM.pdf),
section 4.2.2.2: CSA bit 15 is read-only protection, bit 7 is bus width, bits
3-1 select size, and bit 0 enables the mapping. Preserve wait states and flash
timing rather than replacing the register with a guessed constant.

No physical device was programmed. No archive UI, persistent record format,
RAM snapshot, restore/compaction engine, power-off hook, or USB transport is
claimed implemented. See [validation notes](README.md) for actual coverage.
