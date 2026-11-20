# AS3000 updater flash geometry cross-check

Identified device names from literal part-number strings in the local compiled
AS3000 updater/reverter image, then checked the manufacturers' datasheets on
2026-11-19. Sizes below are bytes (KiB), not Kwords. Generic firmware labels
omit boot/revision suffixes; the bottom-boot variants are made explicit below.
No physical flash operation or driver change was performed.

## Firmware evidence and provenance

Source image: `Ghidra/Alphasmart 3000/SmartApplets/UpdaterReverterROM.OS3KOS`
(relative to the AlphaSmart workspace), 84,070 bytes, SHA-256:
`99c255505aea0f2d040715d5ccb8af032ed3e5f1762a1883cc7969539408f7ef`.

The following exact ASCII strings are present in the compiled image. Offsets are
zero-based **package file offsets**, not runtime flash addresses. Each listed
occurrence was checked directly against the binary.

| Literal firmware string | Package offsets |
|---|---|
| `AMD - Am29LV004B` | `0x02090`, `0x0B4A8` |
| `ST - M29W004` | `0x020A1`, `0x0B4B9` |
| `Fujitsu - MBM29LV004` | `0x020AE`, `0x0B4C6` |
| `AMD - Am29LV008BB` | `0x020D8`, `0x0B4F0` |
| `AMD - Am29LV800BB` | `0x020EA`, `0x0B502` |
| `ST - M29W008` | `0x020FC`, `0x0B514` |
| `ST - M29W800` | `0x02109`, `0x0B521` |
| `Fujitsu - MBM29LV008` | `0x02116`, `0x0B52E` |
| `Fujitsu - MBM29LV800` | `0x0212B`, `0x0B543` |
| `Intel/Micron - MT28F008B3` | `0x02156`, `0x145FE` |
| `Intel/Micron - MT28F800B3` | `0x02170`, `0x14618` |

These strings establish the device names represented in the firmware. The
sector sizes below are sourced independently from manufacturer datasheets;
strings alone do not prove geometry or which hardware configurations shipped.
The table is a researched device catalog, not a transcription of source code.
Numeric IDs and native service behavior cited below are separately supported
by [ROM disassembly](native-services.md). The Intel B3 rows are additional
manufacturer-documented comparisons, not strings found in this updater image.

## Verified sector maps

Define these bottom-boot layouts, in increasing byte-address order:

* A: 16 + 8 + 8 + 32 + 7*64 KiB = 512 KiB, 11 sectors.
* B: 16 + 8 + 8 + 32 + 15*64 KiB = 1024 KiB, 19 sectors.
* C: 16 + 8 + 8 + 96 + 7*128 KiB = 1024 KiB, 11 sectors.
* D: 8*8 + 15*64 KiB = 1024 KiB, 23 sectors.

| Updater part / resolved variant | Map | Manufacturer evidence |
|---|---|---|
| AMD Am29LV004B / bottom Am29LV004BB | A | AMD publication 21522, sector architecture and bottom-boot address table |
| ST M29W004 / M29W004B, compatible successor M29W004BB | A | ST M29W004BT/BB summary, memory maps; successor explicitly backward compatible |
| Fujitsu MBM29LV004 / MBM29LV004BC | A | Fujitsu DS05-20864-3E, features and sector architecture |
| AMD Am29LV008BB | B | AMD publication 21524, flexible sector architecture |
| AMD Am29LV800BB | B | AMD publication 21490, byte-mode sector architecture |
| ST M29W008 / M29W008B | B | ST M29W008T/B, Memory Blocks, p.3; bottom table 3B |
| ST M29W800 / M29W800AB | B | ST M29W800AT/AB, Memory Blocks and bottom-boot map |
| Fujitsu MBM29LV008 / MBM29LV008BA | B | Fujitsu DS05-20858-4E, features and sector architecture |
| Fujitsu MBM29LV800 / MBM29LV800BA | B | Fujitsu DS05-20845-4E, byte-mode sector architecture |
| Micron MT28F008B3 | C | Micron 09005aef81136a91, Rev. E 6/04, p.8 Figure 3 |
| Micron MT28F800B3 | C | Same Micron datasheet and figure; byte addresses, not word addresses |
| Intel 28F008B3 | D | Intel 3-Volt Advanced Boot Block datasheet, blocking table and memory maps |
| Intel 28F800B3 | D | Same Intel datasheet; byte totals independent of x16 word addressing |

The manufacturer documents establish three different maps for the device names
found in the updater image (A, B and C), plus Intel B3 comparison map D.
In particular, Micron's 128-KiB main blocks are physical erase blocks, not merely
software groups of 64-KiB blocks. The initially presumed Intel/Micron B3
interchangeability was wrong. The Intel and Micron B3 names have different maps.

## IDs and the combined Intel/Micron label

Micron MT28F008B3 identifies with manufacturer compatibility byte 89h and bottom
part byte 99h. MT28F800B3 uses 89h and 9Dh in byte mode (889Dh in word mode).
See Micron pp.5-6 device-identification tables and p.9 identification explanation.
The ROM workers compare packed values 89998999h and 89899D9Dh, as recorded
in the disassembly notes. The bus-read behavior must explain packing/repetition;
do not treat those packed values as JEDEC manufacturer IDs.

Intel B3 IDs differ: 28F008B3 bottom D3h (top D2h), and 28F800B3 bottom 8893h
(top 8892h). Thus the updater label “Intel/Micron - MT28F...B3” cannot be
interpreted as saying Intel 28F008B3 has map C.

Intel's older 28F008BV/BE and 28F800BV/CV/CE families do have map C. Intel's
8-Mbit SmartVoltage datasheet and specification update identify 99h for the
bottom byte-wide part and 889Dh for the bottom word-wide part. This makes the
combined Intel/Micron category plausible for those older Intel devices and
Micron MT parts. It does not identify a chip on a particular shipped AS3000.

## Design consequence

Use identified device-specific capacity, sector map and command implementation.
A hard-coded 64-KiB erase operation is unsafe for map C. For these bottom-boot
parts the AS3000 first applet offset 8000h starts at a 32-KiB sector in maps A/B,
a 96-KiB sector in map C, and an 8-KiB parameter sector in map D. Therefore the
claim that erasing the first applet necessarily erases SmallROM was too broad:
it depends on orientation and exact geometry. Top-boot parts differ again.

The AS3000 ROM branch at 4C2DCE..4C2E34 agrees with map C and the packed IDs in its dispatch;
it is no longer merely an unexplained 128-KiB software step. Which configurations
actually shipped still needs chip markings or read-ID evidence.

## Manufacturer sources (some hosted by distributors/archive mirrors)

* AMD Am29LV004B: https://www.manuallib.com/download/pdf1/SPANSION-AM29LV004B-DATA-SHEET.PDF
* AMD Am29LV008B: https://www.alldatasheet.com/html-pdf/178650/AMD/AM29LV008BT-90ED/201/3/AM29LV008BT-90ED.html
* AMD Am29LV800B: https://www.alldatasheet.es/html-pdf/55497/AMD/AM29LV800B/49/1/AM29LV800B.html
* ST M29W004BT/BB: https://dtsheet.com/doc/318470/stmicroelectronics-m29w004bb
* ST M29W008T/B: https://dtsheet.com/doc/318474/stmicroelectronics-m29w008t
* ST M29W800AT/AB: https://www.knjn.com/datasheets/M29W800.pdf
* Fujitsu MBM29LV004TC/BC: https://datasheet4u.com/pdf/509670/MBM29LV004TC.pdf
* Fujitsu MBM29LV008TA/BA: https://datasheet4u.com/datasheet-pdf/Fujitsu/MBM29LV008BA-12/pdf.php?id=1114563
* Fujitsu MBM29LV800TA/BA: https://www.ele.uva.es/~jesus/davinci/mbm29lv800ba.pdf
* Micron MT28F008B3/MT28F800B3: https://media.digikey.com/pdf/Data%20Sheets/Micron%20Technology%20Inc%20PDFs/MT28F(008,800)B3.pdf
* Intel B3: https://media.digikey.com/pdf/Data%20Sheets/Intel%20PDFs/28F008,016,160,320,640,800B3.pdf
* Intel older SmartVoltage: https://datasheet4u.com/pdf/1183864/28F008BV-T.pdf
* Intel older ID correction: https://downloads.bl4ckb0x.de/download.intel.com/design/archives/flash/docs/29768801.pdf

The Micron p.8 map was also downloaded and visually inspected; it explicitly
labels seven 128KB blocks, one 96KB block, two 8KB blocks and one 16KB block.
