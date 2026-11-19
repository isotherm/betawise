# AS3000 firmware tests

Place the full AS3000 firmware image, `OS3KROM.OS3KOS`, in this directory beside
`check.sh`. MAME also needs the small boot ROM. Place `SMALLOS3KROM.OS3KOS` here
or use a MAME build with the file already at `roms/asma3k/smallos3krom.os3kos`
relative to its executable. The runner also searches that `roms` directory for
the KS0066 LCD character ROM.

Build Betawise and use a MAME build with the AS3000 SmartApplet image slot. The
runner defaults to the sibling `../mame/mame` executable and runs all cases.
Pass an executable path to override it and an optional case name to run just
that case.

```sh
git submodule update --init applets/MicroPython/upstream
make -j4
./tests/mame/asma3k/check.sh
./tests/mame/asma3k/check.sh ../mame/mame memory
```

Each case starts with fresh temporary NVRAM, mounts one applet, answers the
backup-battery prompts, and selects the applet through the firmware menu. The
runner uses SDL's dummy video driver to keep the desktop untouched. It prints
each result and a final pass count. Its output directory contains the MAME logs,
generated Lua scripts, snapshots, and test NVRAM.

- `hello` checks the Hello World greeting.
- `memory` checks Ctrl+G, indexed scratchpad addressing, hex and ASCII editing,
  pointer following, Backspace, and Clear File. It verifies the guest RAM bytes.
- `invoke` calls a small routine in scratch RAM and checks all six arguments in
  order via the return value.
- `syscall` invokes ClearScreen through `!0`.
- `string` checks a temporary string argument and Shift+Return's pause.
- `handler` invokes a scratch routine that builds a 68000 exception frame and
  jumps to DebugTool's bus-error handler. It checks the error flag and return
  value after `RTE`.
- `micropython` checks the REPL prompt, arithmetic and division errors,
  Backspace, multiline input, allocation stress, and single-precision float
  arithmetic. It also checks `gc`, `micropython`, `sys`, `array`, `struct`, and
  basic `collections`, including imported objects surviving allocation pressure
  and garbage collection.

The AS3000 driver uses two KS0066 character LCD controllers. Lua reads their
DDRAM save items for text assertions. The runner also saves screenshots for
visual inspection. The frame schedules and key matrix are specific to the AS3000
driver and firmware used here.

The handler check uses a synthetic exception frame. The regular AS3000 driver
returns open-bus data on unmapped reads, so the check covers handler stack
cleanup and recovery without testing the CPU's generation of vector 2.

For firmware investigation of native installation and file preservation, see
[Manager finalization and warm registration](../../../research/applet-installation/README.md).

The MicroPython case also covers deque/OrderedDict, memory diagnostics, mutable
buffers and views, JSON, hex/Base64, regex, and in-memory JSON streams.

## Standard MicroPython profiles

Build the isolated images with `make -C applets/MicroPython -j4 minimal core basic extra`.
Run `check.sh ../mame/mame micropython-minimal` (or `micropython-core` /
`micropython-basic` / `micropython-extra`) to test the image in `build-<variant>/` directly. The cases
check common arithmetic and allocation stress, absence of optional features in
minimal, core collection/math support and file-operation failures, and basic's
Unicode/next-default support. `time` is disabled until its HAL is implemented.
All profiles also check Tab indentation, Ctrl+C draft cancellation, a caught
interrupt from an infinite loop, and Ctrl+D clearing interpreter state.
The default suite uses `micropython-core`. The legacy `micropython` case requires the expanded optional-module configuration;
use the profile-specific cases for these standard ROM levels.

The extra profile case checks additional data-processing modules, math,
collections, memoryviews, cooperative scheduler callbacks, console output,
interrupt configuration and unsupported input errors. It also checks that
select, asyncio, threading, process and network modules are absent.
`uctypes` is excluded because REPR_B has only 30 small-int bits.

## Readline REPL checks

Run `check.sh ../mame/mame micropython-repl-minimal` (also `-core`, `-basic`,
or `-extra`) for focused checks of history, name/attribute completion, cursor
editing, Tab indentation, allocation stress, Ctrl+C and Ctrl+D, plus raw/paste
mode entry, execution, cancellation and return to the friendly REPL. These use the
same upstream event-driven REPL in every build profile.

The AS3000 ROM translates ASCII through a text-to-LCD table at `0x4ea62c`: in
particular, `g/p/q/y` become controller bytes `00/02/03/01`, and backslash/tilde
become `DA/D5`. The shared test LCD reader decodes these bytes before comparing
text. Raw DDRAM is not ASCII; the REPL test checks the full `KeyboardInterrupt`
message after an uncaught interrupt.

`check.sh ../mame/mame micropython-facilities-extra` is a focused check for
disabled select/asyncio/thread/process/network modules, retained stdout/stderr
text and binary streams, memory streams, and the existing `input()`/stdin/open
unsupported-operation stubs.
