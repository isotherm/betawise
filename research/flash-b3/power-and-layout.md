# Battery-powered flash and archive placement

## Decision from the available evidence

**Do not shelve the archive on the assumption that USB is electrically required.**
The B3 chip supports low-voltage programming, AlphaSmart documents serial
SmartApplet installation, and System 3.01's serial updater has a real battery
voltage admission check rather than an unconditional USB requirement.

This establishes intended battery-capable operation, not physical qualification
of the experimental writer on a particular board. No available board schematic
establishes the actual VPP rail or its switching. MAME cannot validate its
voltage, load regulation or brownout behavior. Before enabling the writer on
hardware, identify the fitted part and verify VCC/VPP on battery power, including
during controlled programming/erase of independently owned storage. No physical
flash writes or board measurements were performed in this investigation.

## Chip voltage requirements

The user-supplied May 1997 Intel 28F008B3/28F016B3 datasheet, section 3.4
(printed p.26), explicitly supports programming/erasing with **2.7-3.6 V VPP**.
VCC must also meet its specified operating range. Twelve volts is an optional
production-programming mode with stricter endurance/exposure limits, not a
required flash-write voltage. See the [archive report](archive-options.md)
for those limits and the exact supplied PDF hash.

Three nominal 1.5-V alkaline cells provide 4.5 V; three nominal 1.2-V NiMH cells
provide 3.6 V. These pack values do not establish the voltage at the flash pins.
Regulator dropout, diode/switch losses, load transients and cell condition matter.
USB's nominal 5 V is also not the B3's low-voltage VPP programming range.
The board's power circuit, not the number of cells alone, determines suitability.
The TI comparison cannot establish the AlphaSmart's flash rail or requirements.

The supplied datasheet's DC tables list flash program/erase supply currents in
the tens of milliamps when VCC and VPP loads are considered. A board can remain
usable for intermittent typing yet lack sufficient loaded voltage for an erase.
The lockout threshold is not a safe programming threshold: p.32 says operation
is not guaranteed outside the valid programming-voltage ranges, even above the
lower lockout point.

## Manufacturer documentation

These are original AlphaSmart manuals hosted on third-party mirrors, rather
than third-party descriptions of the hardware:

* [AlphaSmart 3000 User's Guide](https://tilde.club/~jy4m/alphasmart/AlphaSmart%203000%20User%27s%20Guide.pdf),
  printed pp.39-40: instructions explicitly offer **serial/COM or USB** for
  loading a SmartApplet. The serial procedure connects the printer/serial port
  and turns the AlphaSmart on; it does not require connecting USB.
* [AlphaSmart 3000 Quick Start](https://mirrors.apple2.org.za/Apple%20II%20Documentation%20Project/Peripherals/Other/AlphaSmart/Manuals/AlphaSmart%203000%20-%20Quick%20Start.pdf),
  printed p.5: the Serial Printer/Mac Updater Cable is described as a standard
  eight-pin serial cable, suitable for installing SmartApplets from a serial Mac.

Combined with the serial-mode battery guard below, this is strong evidence that
non-USB flash installation was a supported workflow. It is not a measurement of
a specific board's VPP or a guarantee at depleted battery voltage. The common
factory-calibration instruction to use external power with batteries removed
is a calibration procedure; it does not establish a universal USB-only rule.

## Firmware trace and executed battery-gate probe

AS3000 System 3.01 full ROM SHA-256:
`732dca63399d883071ce41cc07ad352fa19c506b7b8c307ace16d5c53e6f8708`.

* `0x004C0ADE..0x004C0B16` installs UART receive handling and sets RAM `0x0420`
  to **mode 5**. Its receive callback `0x004C0B18` assembles Manager packets from
  UART bytes, calls the shared handler, and sends replies through the serial writer.
* Manager begin-applet command `06`, `0x004C1138..0x004C1162`, checks mode 5,
  calls battery refresh `0x004D1EC4` and millivolt conversion `0x004D2008`, then
  rejects readings **below 3500 mV** through response `8D`.
* A zero conversion uses the fallback 3500 value in this path. That is original
  firmware behavior, not a policy the archive writer should copy as proof of
  adequate voltage. Unknown measurement should not authorize an erase.
* Applet erase-all `0x004C206C..0x004C20AA` uses the same 3500-mV gate. A separate
  firmware-write initialization path near `0x004C0E20` uses 3700 mV.
* Conversion is `2322281 / cached_sample + 887`, or zero for a zero sample.
  These are firmware pack-voltage estimates, not flash VPP measurements.

Reproduce without programming or erasing flash:

```sh
python3 research/flash-b3/power/run.py /tmp/as3000-flash-power
```

The output directory must be new and HelloWorld must already be built. The
runner calls the real packet parser/handler with mode 5 and controlled cached
voltage samples. It disables hardware measurement refresh in the fixture so
the seeded sample remains stable; it does not simulate a real battery or ADC.

Executed results:

| Seeded sample | Converted value | Native begin-applet result |
|---:|---:|---|
| 889 | 3499 mV | Rejected, response `8D`, no write cursor selected |
| 888 | 3502 mV | Accepted, response `46`, write cursor selected |

No data packets or finalization are sent. The experiment confirms the threshold
branch, not successful physical flash programming. Low-level native workers
seen so far clear processor CSA write protection and issue flash commands;
that is not a board-level VPP wiring map or proof that no other enable exists.

## Front-loaded archive applet

The first normal AS3000 applet begins at **`0x408000`**, not a 64-KiB boundary.
For the B3 layout, an illustrative front-loaded image could be:

```text
400000..407FFF  Existing SmallROM/system area: preserve
408000..40FFFF  Archive applet header, code, and padding (32 KiB)
410000..41FFFF  Journal: one whole 64-KiB block
420000..45FFFF  Example archive data: four whole blocks, 256 KiB
460000..460003  Applet footer, outside erasable archive blocks
460004..       Other applets restored afterward
```

This is a proposed image layout, not an installed partition. It assumes the
code/metadata fit before `0x410000` and actual B3 geometry has been verified.
Adding a dedicated collection spare needs another whole block. Do not treat
the sole committed journal block as both the journal and disposable GC workspace.

A packaging step can append padding/storage and a new footer and update the
image's declared ROM size after an ordinary link; a custom linker script is not
required for this layout. The installer must put the image first. Later normal
append installs do not move it, but desktop erase/reinstall/repacking may.
New code that outgrows its initial allocation also needs an explicit migration.

For bottom-boot B3, the initial 64 KiB has eight 8-KiB parameter blocks. Some
whole 8-KiB blocks after a small front applet might be independently usable,
but this is chip-orientation-specific and not supported by the current main-block
driver. For top-boot B3, the first 64-KiB block includes SmallROM and the applet's
first 32 KiB; it must not be erased for journal recycling.

## Oversized reservation at arbitrary placement

Let B=64 KiB and X be a desired archive payload rounded to a multiple of B.
For an interior storage span with arbitrary placement:

* **X+B bytes** guarantee at least X bytes in complete erase blocks.
* **X+2B bytes** guarantee X payload bytes plus one complete journal block.
* A separate GC spare needs another complete block beyond those allocations.

Those span sizes exclude code, header and footer. If a span length is a multiple
of B but starts off-boundary, its two excluded fringes together occupy B bytes.
They share erase blocks with neighboring image content and cannot normally be
erased independently. They are not a freely recyclable additional journal.

Useful optional fringe contents include immutable format/version information,
archive/device UUID, creation parameters, or one-pass append-only records such
as checkpoint hints, block CRC summaries, diagnostic events and wear estimates.
Each append consumes previously erased bytes. Once full, leave it full unless
the entire containing block can be safely rewritten with all its other content.
Use these as redundant/rebuildable hints, not the sole authoritative commit
record or the only information needed to restore the archive.

## Journal wear

With a 64-byte record, a 64-KiB block has roughly 1000 usable record slots after
metadata. At the supplied revision's 10,000 erase-cycle specification, an ideal
append-only journal has a budget around **10 million small records**, before
accounting for other overhead, retries or wear. This can comfortably exceed
normal use, but is not a calendar-lifetime/retention guarantee.

A scratch block erased during every collection or large backup has a different
wear rate. Repeatedly rewriting a single index also loses the append-only
advantage. Keep metadata records small and append-only, rotate collection roles
among data blocks, and retain a committed old copy until its replacement verifies.

## neotools migration support

The repository already has the individual building blocks:

* `read_applet_list` / the applet-list command;
* `applets fetch ID PATH` / `fetch_applet` for complete image download;
* `applets remove-all` for applet-area erase, not whole-chip erase;
* `applets install` / `install_applet` to put images back in order.

What is missing is a transactional **backup/validate/reorder/restore** operation.
It should save all images and RAM files/settings that the reinitialization may
affect, verify lengths, signatures, footers and hashes before erasing, install
the archive image first, restore other applets, then restore and verify user
data. Image backups alone are not backups of applet RAM files or live settings.
Preserve SmallROM, the system OS and device-specific calibration/identity data.
No migration or erase was performed, and no new neotools command is implemented.

## Next hardware gate

Proceed with format/migration design on the evidence of intended battery use.
Keep real archive writing disabled until the fitted part and its VCC/VPP supply
are checked on the target board, with an adequately powered battery-only write
and erase verified in owned disposable storage. If VPP actually requires an
external source on that board, or sags outside the valid range under load,
standalone backup remains disabled and the battery-only feature should be shelved
for that hardware. A USB-connected success alone would not satisfy that gate.

## Restore scope and immutable fringes

The intended backup restores logical files and persistent settings to their last
committed backup after RAM loss. Do not restore transient A5 working/global
spaces as raw memory images: rebuild applet runtime state through initialization
and apply version-compatible saved settings through understood interfaces.
Settings outside applet file contents must be inventoried before claiming a
complete restore. This remains a design requirement, not implemented behavior.

Useful fringe records include owner/contact information, settings snapshots,
and initial recovery metadata. Append new versions with checksums and durable
commit markers; once the fringe fills, continue in the recyclable archive.
Optional encryption may store public key material or wrapped keys here; storing
an unprotected decryption key beside ciphertext provides no confidentiality
against someone reading the flash. No encryption implementation is proposed yet.

For arbitrary placement, let H include the header, executable and other immutable
prefix bytes, F include the footer, and B be the erase size. Reserving
H + n*B + B + F guarantees n complete recyclable blocks after the prefix and
before the footer. The extra B is alignment allowance, not necessarily physical
padding: its fringes can contain finite append-only records. A formula written
APP_SIZE + n*B + APP_SIZE is not generally sufficient if APP_SIZE means the
actual small executable size; the second APP_SIZE must cover the worst-case
alignment allowance and footer. If it means a full-block reservation, that
qualification should be explicit. Collection spare blocks are included in n,
not additional usable live-data capacity.

The 128-KiB observation comes from the AS3000 System 3.01 ROM's own native erase
worker, not an AS2000 dump: see firmware-boundary.md, addresses
4C2DCE..4C2E34. It establishes a legacy Intel-device branch with a 128-KiB
main-region step. It does not establish which chip is fitted to any particular
AS3000, nor that every AS3000 has 128-KiB physical erase blocks. The supplied
28F008B3 datasheet describes 64-KiB main blocks. Physical chip identification and
matching the firmware branch remain necessary; the software stepping and the
physical geometry must not be conflated.
