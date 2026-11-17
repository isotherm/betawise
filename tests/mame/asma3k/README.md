# AS3000 firmware tests

Place the full AS3000 firmware image, `OS3KROM.OS3KOS`, in this
directory beside `check.sh`. MAME also needs the small boot ROM. Place
`SMALLOS3KROM.OS3KOS` here or use a MAME build with the file already
at `roms/asma3k/smallos3krom.os3kos` relative to its executable. The runner also searches that `roms` directory for the KS0066
LCD character ROM.

Build Betawise and use a MAME build with the AS3000 SmartApplet image
slot. The runner defaults to the sibling `../mame/mame` executable and
runs all cases. Pass an executable path to override it and an optional
case name to run just that case.

```sh
make -j4
git submodule update --init applets/MicroPython/upstream
make -C applets/MicroPython/os3k -j4
./tests/mame/asma3k/check.sh
./tests/mame/asma3k/check.sh ../mame/mame memory
```

Each case starts with fresh temporary NVRAM, mounts one applet, answers the
backup-battery prompts, and selects the applet through the firmware menu.
The runner uses SDL's dummy video driver to keep the desktop untouched.
It prints each result and a final pass count. Its output directory
contains the MAME logs, generated Lua scripts, snapshots, and test NVRAM.

- `hello` checks the Hello World greeting.
- `memory` checks Ctrl+G, indexed scratchpad addressing, hex and ASCII editing,
  pointer following, Backspace, and Clear File. It verifies the guest RAM bytes.
- `invoke` calls a small routine in scratch RAM and checks all six arguments
  in order via the return value.
- `syscall` invokes ClearScreen through `!0`.
- `string` checks a temporary string argument and Shift+Return's pause.
- `handler` invokes a scratch routine that builds a 68000 exception frame and
  jumps to DebugTool's bus-error handler. It checks the error flag and return
  value after `RTE`.
- `micropython` checks the REPL prompt, arithmetic and division errors, Backspace,
  multiline input, and allocation stress.

The AS3000 driver uses two KS0066 character LCD controllers. Lua reads their
DDRAM save items for text assertions. The runner also saves screenshots for
visual inspection. The frame schedules and key matrix are specific to the
AS3000 driver and firmware used here.

The handler check uses a synthetic exception frame. The regular AS3000 driver
returns open-bus data on unmapped reads, so the check covers handler stack
cleanup and recovery without testing the CPU's generation of vector 2.
