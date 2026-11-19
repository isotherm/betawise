# MicroPython for OS3K

Build with the pinned `upstream` submodule initialized. From this directory:

```sh
make -j4               # build minimal, core, and basic
make core -j4          # build only core
make VARIANT=basic -j4 # another way to select one level
```

Each variant has its own core library, generated headers, wrapper objects,
ELF, and image in `build-<variant>/`. Intermediate names stay identical:
`MicroPython.o`, `MicroPython.elf`, and `MicroPython.OS3KApp`. Only the published
images in this applet directory carry the variant name:

- `MicroPython-minimal.OS3KApp`
- `MicroPython-core.OS3KApp`
- `MicroPython-basic.OS3KApp`

The named targets can run together under `make -j` because all intermediate files are isolated.
`applet` / `variant` builds and publishes the selected `VARIANT` (core when
not specified). `BUILD=/some/path` can override its build directory; use a
separate directory for each variant.

The variants select upstream `MICROPY_CONFIG_ROM_LEVEL_MINIMUM`,
`CORE_FEATURES`, and `BASIC_FEATURES`, respectively. They are standard upstream
profiles, rather than the earlier hand-picked optional-module configuration.

| Variant | Main differences |
| --- | --- |
| `minimal` | Small interpreter; most optional builtins and modules omitted |
| `core` | Common builtins, sets, bytearray, array, namedtuple, gc, io, struct, math |
| `basic` | Core plus Unicode strings/checking and additional builtin conveniences |

All variants retain the compiler, REPL, software single-precision floats,
garbage collector, stack checks, a 16 KiB heap, and a 4 KiB execution stack
budget. `sys.platform` is `"os3k"`. Complex numbers remain disabled. `time` is
explicitly disabled even in basic because its HAL hooks are not implemented.
Filesystem probes report no files and `open()` raises `OSError(95)`;
no filesystem adapter or standard-stream objects are supplied.

From the BetaWise repository root, run the matching firmware tests after
building the named variants:

```sh
./tests/mame/asma3k/check.sh ../mame/mame micropython-minimal
./tests/mame/asma3k/check.sh ../mame/mame micropython-core
./tests/mame/asma3k/check.sh ../mame/mame micropython-basic
```

These cases check arithmetic, profile-specific features, file-operation
failures, and allocation stress in real applet startup under MAME. The older
`micropython` case in the same runner targets the expanded optional-module
configuration (JSON, regex, deque, etc.); it is not a standard-profile test.

`make clean` removes all standard variant build directories (including
ELF/relocation files) and published images. It also removes a custom `BUILD`
when supplied. It does not touch the upstream source submodule at `upstream/`.

## REPL and execution

Mount `applets/MicroPython/MicroPython-core.OS3KApp` in MAME's AS3000
SmartApplet slot. To test a new installation, use fresh NVRAM so the
firmware scans its flash. Complete the initial setup and select
MicroPython in the SmartApplets menu.

Type `print(6 * 7)` and press Return. The REPL supports expression display,
assignments, Backspace, and multiline input terminated by a blank line.
Variables persist when switching applets and are initialized anew when the
firmware sends `WM_INIT`. The REPL redraws the current prompt and unfinished
input when it regains focus. A visible cursor marks the input position.
Long input lines wrap on the display. Each statement can contain up to
511 characters, including newlines in multiline input. The VM has a 16 KiB heap.

Keyboard input arrives in OS3K key and character messages so the guest can
continue handling applet switching between commands. Betawise's blocking
`getchar()` would keep the message callback running. Output uses Betawise
`putchar()` and `fputs()` for wrapping and scrolling with the active font.
Python statements execute synchronously, so the REPL processes no
keyboard messages until the current statement returns.

MicroPython and UCBLogo use the shared `os3k/setjmp.h` contract and the jump
implementation in `libos3k.a`; MicroPython does not own a private copy.

## Object alignment

The port uses `MICROPY_OBJ_REPR_B`, which can tag two-byte-aligned
addresses in both RAM and flash. OS3K supplies the original A5 on each
callback. Representation B uses signed 30-bit immediate integers, from
−536,870,912 to 536,870,911. This build has no wide-integer fallback.

The MicroPython core and port still use `-malign-int` for the layout of
objects and their pointer fields. In testing, removing this option
caused the allocation stress case to fail. The compiler flag does not
change where OS3K loads the applet.

## Floating point

Python floats use software IEEE 754 single precision. The REPL supports
decimal literals, arithmetic, comparisons, `divmod`, and `round`.
The port builds the required math routines from MicroPython's bundled
libm. The minimal profile leaves complex numbers and the `math` module
disabled.

The applet uses the [OS3K compiler runtime](../../os3k/compiler-rt/README.md)
for both integer helpers and soft float.

## Time porting hooks

`time` is not enabled yet. The common module in `upstream/extmod/modtime.c`
requires five HAL functions (declared in `upstream/py/mphal.h`):

| Hook | OS3K implementation work |
| --- | --- |
| `mp_hal_ticks_ms()` | Wrap `GetUptimeMilliseconds()`; account for service shutdown. |
| `mp_hal_delay_ms(ms)` | Use wrapping elapsed-time subtraction for deadlines. `SleepCentiseconds()` is only coarse (10 ms plus boundary overhead), and its deadline comparison is not wrap-safe. |
| `mp_hal_ticks_us()` | Add a higher-resolution hardware clock, or explicitly document millisecond-resolution values expressed in microseconds. |
| `mp_hal_delay_us(us)` | Add a calibrated hardware-timed delay. `BusyLoop()` counts iterations, not microseconds, and depends on CPU speed/interrupts. |
| `mp_hal_ticks_cpu()` | Supply a suitable fast hardware counter or a documented lower-resolution proxy; there is no existing OS3K cycle-counter wrapper. |

`ticks_add()` and `ticks_diff()` are implemented by MicroPython already.
The basic module needs neither threads nor timer callbacks. If using
`TimerStart()` for waits, handle the five-slot pool, 3 ms minimum, 10-second
maximum, service-stop notification, and cancellation. Avoid calling Python
from its interrupt callback; the firmware does not restore the applet's A5.
Current REPL execution is synchronous, so a blocking delay also blocks normal
applet message handling.

Calendar functions are separate optional work: `mp_time_time_get()` and
`mp_hal_time_ns()` supply epoch timestamps, and `mp_time_localtime_get()`
supplies current calendar fields (plus shared timeutils for conversion).
`GetUptimeSeconds()` is startup-relative RTC time, wrapping after 512 days;
it is not a calendar timestamp. Nanosecond timestamps also need large-integer
support, which this minimum port does not currently enable.
