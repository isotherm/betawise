# AS3000 warm applet installation on System 3.01

MAME's SmartApplet slot appends an image to flash during machine reset. On a
warm boot, this ROM uses the existing applet directory in backed-up RAM. In a
test with AlphaWord Plus 3.0D already registered, Hello World's image persisted
at guest address `0x420cdc`, but the menu listed only AlphaWord. The second
header was read at PC `0x4c3254` during full initialization and was skipped
during the warm boot.

## Reset markers

These addresses describe the bundled ROM with System 3.01 and 256 KiB of RAM.
Tests used disposable copies of both NVRAM files.

| Guest RAM word | Value on warm boot | Result of clearing the word |
| --- | --- | --- |
| `0x3566` | `0x0d89` | The branch at `0x4c748c` falls through. Firmware clears file RAM, scans flash via `0x4c3038`, and registers Hello World. The existing AlphaWord text is lost. |
| `0x3568` | `0x2675` | Firmware reinitializes registered applets, including a verified `WM_INIT` for AlphaWord. The AlphaWord text survives. Hello World stays absent from the menu. |

The startup code also recognizes `0xbddf` at `0x418` and clears both markers
and `0x3564`. That routes through full initialization. The reinitialization code
dispatches message `0x18` at `0x4c78b0` and calls the applet entry at
`0x4c78b6`.

## File-preserving proof of concept

1. Mount AlphaWord Plus on fresh NVRAM, complete the guest's initial setup,
   enter `saved file test` in File 1, and quit MAME.
2. Relaunch with the same NVRAM and mount Hello World. After MAME exits, copy
   the resulting flash and RAM. Use another copy of that flash with fresh RAM,
   complete initialization, and exit to obtain a reference applet directory.
3. Copy these ranges from the reference RAM into the warm test copy. The
   offsets refer to the third directory slot and the RAM allocation floor.

   | Offset | Size | Field |
   | --- | --- | --- |
   | `0x0d8a` | 4 | Applet header pointer, `0x420cdc` |
   | `0x0e06` | 2 | Applet ID, `0xa1a0` |
   | `0x0e4a` | 4 | Applet flags, `0xff000000` |
   | `0x0eca` | 4 | Header metadata copied from applet offset `0x80` |
   | `0x34d6` | 4 | Applet A5 base, `0x3ca2a` |
   | `0x354e` | 4 | New applet RAM allocation floor, `0x3ca2a` |

4. Clear the word at `0x3568` in the warm test copy. Boot it with no image
   mounted. Breakpoints at `0x408094` and `0x420dc4` recorded message `0x18`
   in the first stack argument for AlphaWord Plus and Hello World, respectively.

The boot displayed the initialization screen without asking to restart. Both
applets appeared in the menu. Hello World displayed its greeting, and File 1
still showed the saved text after switching back. This is a two-applet test
with a fileless addition requiring 94 bytes of applet RAM. The file heap and
applet RAM share the same address space.

## Implementation options

- A ROM-specific incremental registration path could append a directory entry
  while the CPU is held in reset, preserve existing A5 bases, and request the
  reinitialization at `0x3568`. It would need ROM identification, bounds and
  file allocation checks, and tests with full files, applets that own files,
  replacement, and removal. A new applet's RAM allocation can collide with
  live file pages.
- The firmware's native updater provides a hardware-oriented path. The
  archived Updater source handles command 80 by starting a watchdog reset
  for the ROM updater, or asking the user to power-cycle after the RAM
  updater. PG3 is the device power latch. MAME would need to model the
  power transition and the updater transport and implement flash command
  handling. The AS3000 manual documents Command+Ctrl+U for entering the
  updater and warns that SmartApplet downloads erase files and settings.
  Capturing an actual Manager session for System 3.01 would establish
  whether it preserves files by export and restore or through a different
  registration path.
- Export, normal full initialization, and reimport through the firmware's
  file-transfer interface would leave allocation decisions to the guest.
  That requires working file transfer in both directions.

A power cycle by itself takes the warm path tested above. Jumping to the scan
routine by changing PC would also require constructing its firmware call
context and separating directory rebuilding from the file allocator.
