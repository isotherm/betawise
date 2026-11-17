# AS3000 firmware tests

Build Betawise, then point the runner at an AS3000 MAME build with the
SmartApplet image slot and a directory containing the AS3000 ROM set.

```sh
make -j4
./mame-test/check.sh ../mame/mame ../mame/roms
./mame-test/check.sh ../mame/mame ../mame/roms memory
```

Each case starts with fresh temporary NVRAM, mounts one applet, answers the
backup-battery prompts, and selects the applet through the firmware menu.
The runner prints the output directory, which contains the MAME logs, generated
Lua scripts, snapshots, and test NVRAM. It requires a per-case Lua pass marker.

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

The AS3000 driver uses two KS0066 character LCD controllers. Lua reads their
DDRAM save items for text assertions. The runner also saves screenshots for
visual inspection. The frame schedules and key matrix are specific to the
AS3000 driver and firmware used here.

The handler check uses a synthetic exception frame. The regular AS3000 driver
returns open-bus data on unmapped reads, so the check covers handler stack
cleanup and recovery without testing the CPU's generation of vector 2.
