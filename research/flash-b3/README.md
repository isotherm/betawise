# Intel 28F008B3 driver investigation

See [archive options, geometry, endurance and latency](archive-options.md) for
the design report and datasheet references. The experimental driver lives
privately in `applets/FileManager/{Flash.h,Flash.c,FlashWorker.S}`. It is not part
of the SDK. Both the applet and this probe use the unchanged standard linker
script and CRT; this probe compiles the same applet-local sources.

The [firmware boundary investigation](firmware-boundary.md) traces normal
Manager allocation, reporting, scanning and erase-all, distinguishes RAM
allocator addresses from flash limits, and records an executed two-constant
admission/reporting experiment for a 256-KiB tail reservation.

The [battery-power and layout investigation](power-and-layout.md) records the
chip's voltage requirements, original serial-update documentation, the executed
3.5-V serial-updater admission test, front-loaded/oversized image options,
journal/fringe uses and the missing neotools migration workflow.

The [NEO space and compression investigation](neo-space-and-compression.md)
identifies the 2013 package's 116-KiB erased-value run, its firmware/trailer
erase-block constraints, and bounded compressed/differential record options.
Read-only package inspection and host sizing scripts reproduce the findings.

The File Manager UI does **not yet call the driver**; the normal manager image
reserves no archive capacity, retains no unused RAM worker, and performs no
flash command writes.

Production `FlashOpen` returns `FLASH_NO_STORAGE` on supported hardware until
a firmware-enforced partition policy is implemented. The probe compiles
`Flash.c` with `FILE_MANAGER_FLASH_PROBE`, authorizing only the model's virtual
`0x410000..0x41FFFF` region. This is not real flash ownership or a reservation.

## Validation

```sh
make -C os3k
make -C research/flash-b3
python3 research/flash-b3/run.py /tmp/os3k-flash-b3
```

Use a new output directory. The probe uses test applet ID `A1FF` and is not a
normal applet build target. It is inert unless explicitly armed by a RAM cookie
and then given `p`. **Never install the probe on physical hardware.**

The stock AS3000 MAME driver maps flash as read-only/no-op-write memory, not a
flash device. `model.lua` installs scoped byte-command taps as a **CUI fixture**;
actual linked 68000 driver/worker code, firmware startup, applet RAM relocation,
stack, CPU flags and instruction execution remain real emulator behavior.
It supports just the commands used by the probe, not a full flash model.

The local EZ328 emulation also assigns `m_csa = data` in `csa_w`, ignoring the
byte-write mask. Native boot leaves `CSA=8080` despite SmallROM's `8007` setup.
The explicit CUI fixtures seed `8007` before opening storage. The stock-MAME
negative case does not seed or emulate anything and correctly refuses the
unsupported mapping. No emulator sources were changed by this work.

Four executed cases:

* Bottom-boot B3 `89:D3`: 257-byte programming using odd source/destination
  addresses, unchanged-byte skipping, 0-to-1 preflight rejection, boundary and
  unaligned erase rejection, full-block erase/verification and reads.
* Top-boot B3 `89:D2`: same workflow with the other identity.
* Unsupported Intel `89:99`: identification/return-to-array only, zero program
  or erase operations.
* Unmodified stock MAME mapping: refusal with zero command writes.

Injected programming cases verify VPP/program-error status propagation (`98`),
a soft-budget overrun that waits for ready and returns `FLASH_WAIT_EXCEEDED`,
and readback corruption that returns `FLASH_VERIFY_ERROR`. The fixture asserts
RAM execution and masked interrupts on every command and non-array read; it
checks return to array mode and restoration of CSA and interrupt mask. It also
checks top/bottom parameter-block geometry. Supported cases issue exactly 260
program operations (257 original + 3 injected) and one erase, preventing an
unchanged rewrite from silently adding programming pulses.

The **272-byte** worker is an ordinary ROM code blob. `FlashRun` copies it into
a two-byte-aligned 512-byte temporary stack buffer for each command operation;
the buffer is reclaimed on return. Volatile byte stores prevent GCC from
discarding the copy as dead data stores when the destination is subsequently
executed through a function pointer. Intra-worker branches are PC-relative and
the blob has no external flash call or relocation. The fixture checks that its
source is in ROM while commands and non-array reads execute from RAM.
There is no custom linker script, CRT-managed RAM worker or RWX RAM image.
The standard QEMU startup/relocation and production ABI checks remain applicable.

These results validate command ordering, guards, RAM placement and software
failure paths **against the fixture**. They do not validate real B3 timing,
retention/endurance, partial-power erase behavior, electrical write timing,
VPP/WP#/reset wiring, interrupt/NMI/watchdog safety, hardware USB, or successful
programming/persistence on a physical AS3000. The driver rejects NEO rather
than extrapolating the AS3000 chip/layout to it.

The [updater chip geometry cross-check](updater-chip-geometries.md) verifies
all supplied updater families and distinguishes Micron B3 128-KiB main blocks
from Intel B3 64-KiB main blocks.

[Native service candidates](native-services.md) records the existing AS3000
RAM-copy wrappers and their supported chip dispatch, with remaining qualification
requirements before reuse.
