# TODO

Open work from internal discussion and current source/research. Roadmap items
and applet ideas are possibilities, not commitments.

## Betawise

### ABI audit

- Verify timer/UART/IrDA callback cancellation after focus loss and A5
  ownership; add trampolines where needed
  ([serial](research/syscalls/serial/README.md),
  [timers](research/syscalls/time/README.md)).
- Document the ROM formatter/scanner dialect; add software conversions if 64-bit
  formatting is needed. Verify stream variants separately.
- Extend branch-level and compiled-call checks for the syscall prototypes
  ([slot inventory](research/abi-audit/slots.tsv)).
- Extend newer typed probes to the Neo 3.01 ROM where calls exist; current
  follow-ups mostly cover AS3000/Neo 3.21.

### Syscalls

#### Applet lifecycle and headers

- Resolve USB message payloads, nonzero replies, and transfer policies
  ([research](research/syscalls/applet/README.md)).
- Resolve header trailing-word meanings and `MSG_SETFOCUS` payload across firmware versions.
- Verify firmware enforcement of header flags and Manager's `0x40` replacement
  policy; investigate independent `0x20`/`0x80` checks. Leave the wireless flag
  name unchanged for now.
- Establish a reliable AS3000/Neo capability check for graphics applets;
  distinguish Manager filtering from runtime requirements.
- Test a default launcher that ignores USB attach, then launches the app under
  test while USB supplies power (saves batteries and plug cycles).

#### Display

- Resolve the Neo cursor address/layout per ROM or replace direct RAM access
  ([implementation](os3k/os3k_display.c),
  [research](research/syscalls/display/README.md)).
- Implement the AS3000 screen flash for the console bell.

#### File and text

- Resolve file-record byte `0x1F` and flag bits `0x01`/`0x04`/`0x08`
  ([research](research/syscalls/file/README.md)).
- Verify the plain clipboard copy's stack-dependent limit and linked-content conversions.
- Expand editor/scanner tests for expression syntax, malformed tokens, opaque
  state fields, and form control types
  ([research](research/syscalls/text/README.md)).
- Resolve transfer-message payloads and test complete file/form send workflows.
- Test native installation, allocator stress, and persistence beyond seeded RAM fixtures.

#### Serial, IrDA, and time

- Trace native callers to explain Neo's mixed UART1/UART2 routing; compare both
  Neo ROMs before changing wrappers
  ([research](research/syscalls/serial/README.md)).
- Extend the maintained timer checks to callback scheduling, service shutdown, and focus changes.
- Verify real serial/IrDA peers, asynchronous callbacks, printer fallback, and
  complete print jobs ([research](research/syscalls/irda/README.md)).

#### Localization and power

- Identify #91's consumer/policy; its bitmap is not a font-coverage test
  ([research](research/syscalls/localization/README.md)).
- Verify battery calibration persistence, analogue accuracy, and charging
  diagnostics on hardware ([research](research/syscalls/power/README.md)).

#### System, speech, and wireless

- Resolve system selectors' 13-byte wireless block, `0x0D00`/`0x0E00` flag
  policy, and `0x9000` predicate
  ([research](research/syscalls/system/README.md)).
- Verify audible speech and real asynchronous queue callbacks
  ([research](research/syscalls/speech/README.md)).
- Decode #284's wireless receive-status bytes and verify physical radio delivery
  ([research](research/syscalls/wireless/README.md)).

### Runtime and toolchain roadmap

- Qualify the shared compiler-rt runtime with another GCC version and
  representative external C code; check helper calls, 68000 instructions,
  relocations, and arithmetic edge cases
  ([runtime](os3k/compiler-rt/README.md)).
- Add an optional OS3K-compatible `libm` as applets need functions beyond compiler builtins.
- Measure removable GOT slots, flash size, and execution cost in representative
  applets; distinguish call-only slots from address-taken symbols.
- Prototype near PC-relative calls and far veneers with GOT fallback using stock
  tools; measure actual RAM savings before adopting linker relaxation.
- Evaluate direct A5-relative RAM and PC-relative flash references using the
  historical compiler/binutils work as a reference (`63f2bd3`…`80cf07a`,
  `9df5d0e`).
- Evaluate a custom GCC build for ROM-compatible pointer returns and removal of
  redundant read-only A5 saves; qualify compiler-generated helper ABI too.
- Explore a versioned shared service for duplicated terminal/Betawise code if
  measurements justify it; define A5 call gates, callbacks, and per-applet
  state.
- Investigate a run-only heap that safely shares space with files and releases
  it on exit; installed applet RAM and fixed-block pools do not provide that
  contract.

### Integration

- Review Sam's Betawise contributions and agree on small, dependency-ordered
  imports; recheck what remains after the local syscall work.
- Identify specific neo-re applets/fixes worth porting rather than importing the whole branch.

## MicroPython

- Revisit screen/state resume on `neo-applet-screen-resume`; choose a portable
  approach and verify switching between programming applets on hardware.
- Measure float-enabled image size, heap use, and REPL performance; consider the
  `math` module after compatible libm support.

## UCBLogo

- Reject unsupported AS3000 hardware before accessing Neo LCD registers.
- Revisit screen/turtle resume on `neo-applet-screen-resume`; verify independent
  snapshots and switching on hardware.
- Measure procedure/recursion heap use and recovery from memory exhaustion
  before reducing the 48 KiB arena.
- Explore a smaller Logo interpreter with parameterized procedures, recursion,
  and fixed-point turtle math; target 8–16 KiB interpreter RAM.

## MAME

### AlphaSmart 3000 and Neo

- Add MC68VZ328 support alongside MAME's existing DragonBall/EZ devices, sharing
  compatible implementations; validate VZ register/reset differences, clocks,
  timers, interrupt routing, GPIO, and UARTs against the manual and Neo
  firmware, then migrate the Neo driver's SoC handling to the device. Determine
  SPI connectivity before attaching a peripheral.
- Follow up on PR #15317 to be sure asma3k gets merged.
- Coordinate upstreaming the Neo/LCD changes with the asneo-emu maintainer.
- Qualify Neo NOR/CFI, USB, and low-power timing against firmware and hardware
  (`mame/src/mame/skeleton/alphasneo.cpp`).
- Add AS3000 USB support (`mame/src/mame/skeleton/alphasma3k.cpp`).
- Measure the AS3000/Pro LCD controller clocks; current configurations use the
  datasheet's typical 270 kHz.

### LCD controller

- Implement ST7565R serial/pin-level bus behavior and busy/reset recovery timing
  (`mame/src/devices/video/st7565.cpp`).
- Model the static-indicator waveform and analogue drive/contrast behavior when
  supported by evidence.

### USB roadmap

- Design a transfer-oriented socket bridge without USB-library dependencies in
  MAME; handle enumeration, cancellation, and emulated-time completions.
- Prototype exporting the emulated device via USB/IP to Linux `vhci_hcd` for
  normal desktop applications.
- Explore external USB/IP, usbredir, or libusb helpers for physical peripherals
  attached to the emulated host.

### AlphaSmart Pro / 2000

- Implement ADB/PS/2 and obtain the correct character ROM (`mame/src/mame/skeleton/alphasma.cpp`).
- Investigate asma2k reads of nonexistent internal register `0x0001` in both BIOSes.

## Applet and companion ideas

- Consider an editor with undo, screenplay formatting, markdown, or vi/Emacs-style keybindings.
- Validate the [File Manager prototype](applets/FileManager/README.md) on native
  firmware; complete special-file move metadata, then flash backup/restore and
  USB object transport. Flash compaction and descriptor/endpoint control remain
  research work.
- Consider a text-adventure interpreter.
