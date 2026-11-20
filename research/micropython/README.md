# MicroPython port research and implementation notes

See the [user guide](../../applets/MicroPython/README.md) for installation,
profile selection, and REPL controls.

## Build from source

From the BetaWise repository root:

```sh
git submodule update --init applets/MicroPython/upstream
make -C os3k
cd applets/MicroPython
```

Build with the pinned `upstream` submodule initialized. From this directory:

```sh
make -j4               # build minimal, core, basic, and extra
make core -j4          # build only core
make VARIANT=basic -j4 # another way to select one level
```

Each profile has its own core library, generated headers, wrapper objects,
ELF, and image in `build-<profile>/`. Intermediate names stay identical:
`MicroPython.o`, `MicroPython.elf`, and `MicroPython.OS3KApp`. Only the published
images in this applet directory carry the profile name:

- `MicroPython-minimal.OS3KApp`
- `MicroPython-core.OS3KApp`
- `MicroPython-basic.OS3KApp`
- `MicroPython-extra.OS3KApp`

The named targets can run together under `make -j` because all intermediate files are isolated.
A configuration stamp clears generated metadata fragments and recreates the
archive when port settings change, preventing disabled modules from surviving
an incremental build through stale registrations or archive members.
`applet` / `variant` builds and publishes the selected `VARIANT` (core when
not specified). `BUILD=/some/path` can override its build directory; use a
separate directory for each profile.

The profiles select upstream `MICROPY_CONFIG_ROM_LEVEL_MINIMUM`,
`CORE_FEATURES`, `BASIC_FEATURES`, and `EXTRA_FEATURES`, respectively. They are standard upstream
profiles for their defaults, with OS3K overrides for unsupported facilities.

| Profile | Main differences |
| --- | --- |
| `minimal` | Small interpreter; most optional builtins and modules omitted |
| `core` | Common builtins, sets, bytearray, array, namedtuple, gc, io, struct, math |
| `basic` | Core plus Unicode strings/checking and additional builtin conveniences |
| `extra` | Basic plus JSON, regex, binascii/CRC32, SHA-256, random, heapq, deflate, framebuffer objects, deque/OrderedDict, memoryview, diagnostics, cooperative scheduler and standard streams |

All profiles retain the compiler, REPL, software single-precision floats,
garbage collector and stack checks. `mpconfigport.h` supplies `OS3K_MP_STACK_KB`
(4) and `OS3K_MP_HEAP_KB` (16, or 64 for extra), in KiB (1024 bytes).
The stack setting is an execution limit, not a separately allocated stack. Minimal,
core, and basic retain a 16 KiB heap; extra uses 64 KiB so the default 32 KiB
deflate window and normal VM objects fit. `sys.platform` is `"os3k"`. Complex numbers remain disabled. `time` is
explicitly disabled even in basic because its HAL hooks are not implemented.
Filesystem probes report no files and `open()` raises `OSError(95)`;
no filesystem adapter is supplied. Extra supplies stdout/stderr and their
binary buffers through the existing console output hook. Blocking `input()`,
and stdin reads raise `OSError(95)` rather than blocking the event-driven
REPL. `micropython.kbd_intr()` configures the cooperative interrupt character
(default Ctrl+C; `-1` disables it).

`uctypes` is disabled: upstream encodes descriptors in at least 31 small-int
bits, while the required two-byte-aligned REPR_B format provides 30. Complex
numbers remain disabled as in the other profiles. Long integers are also
unchanged, so CRC32 results and other integers outside the small-int range
raise `OverflowError`.

`select` and `_asyncio` are disabled: this port has no useful pollable device
streams or integrated async event loop. Thread/GIL, socket/lwIP and TLS support
are explicitly disabled too; no multiprocessing or subprocess implementation
is bundled. `micropython.schedule()` remains a cooperative callback queue, not
threading. Its callbacks run in a bounded batch after successful REPL commands
and at the VM's normal branch checks.

Filesystem placeholders, `input()`, standard streams and their binary buffers
are retained for future OS3K integration. Their existing unsupported-operation
behavior is unchanged. `StringIO`, `BytesIO`, and RAM-backed `DeflateIO` remain
available. The framebuffer module only draws into RAM, with no automatic LCD
transfer.

From the BetaWise repository root, run the matching firmware tests after
building the named profiles:

```sh
./tests/mame/asma3k/check.sh ../mame/mame micropython-minimal
./tests/mame/asma3k/check.sh ../mame/mame micropython-core
./tests/mame/asma3k/check.sh ../mame/mame micropython-basic
./tests/mame/asma3k/check.sh ../mame/mame micropython-extra
```

These cases check arithmetic, profile-specific features, file-operation
failures, allocation stress, Tab indentation, Ctrl+C cancellation/interruption,
and Ctrl+D state reset in real applet startup under MAME. Extra also checks
its modules, scheduler callbacks, console streams, disabled concurrency/network
modules, and unsupported-operation errors. The older
`micropython` case in the same runner targets the expanded optional-module
configuration (JSON, regex, deque, etc.); it is not a standard-profile test.

`make clean` removes all standard profile build directories (including
ELF/relocation files) and published images. It also removes a custom `BUILD`
when supplied. It does not touch the upstream source submodule at `upstream/`.

## REPL implementation

Mount `applets/MicroPython/MicroPython-core.OS3KApp` in MAME's AS3000
SmartApplet slot. To test a new installation, use fresh NVRAM so the
firmware scans its flash. Complete the initial setup and select
MicroPython in the SmartApplets menu.

Type `print(6 * 7)` and press Return. The REPL supports expression display,
assignments, Backspace, and multiline input terminated by a blank line.
Variables persist when switching applets and are initialized anew when the
firmware sends `WM_INIT`. The REPL redraws the current prompt and unfinished
input when it regains focus. A visible cursor marks the input position.
Long input lines wrap on the display. Input now uses a GC-rooted, growable buffer
rather than the former fixed 511-character limit. Heap size depends on the
selected profile as described above.

Keyboard input arrives in OS3K key and character messages so the guest can
continue handling applet switching between commands. Betawise's blocking
`getchar()` would keep the message callback running. Output uses Betawise
`putchar()` and `fputs()` for wrapping and scrolling with the active font.
Python statements execute synchronously. Every 256 VM branch checks, the port
scans the keyboard without allowing the scanner to put an active calculation
to sleep. The configured `mp_interrupt_char` schedules `KeyboardInterrupt`;
Ctrl+C is the default, and `micropython.kbd_intr()` can select another character
within the running command. Control-letter encoding is generic, and `-1`
disables interrupts. Up to 64 unmatched keys are saved as typeahead and replayed
when execution returns; Ctrl+D has no separate evaluation-time action. Long-running native C
functions remain uninterruptible until they return to the VM. No timer is used.

Every profile uses MicroPython's upstream event-driven REPL and shared readline
editor. Up/Down recall eight lines of history; Left/Right, Home/End, Backspace,
and Delete edit input. Ctrl-letter input uses the same generic translation as
the evaluation-time interrupt hook; readline provides Ctrl+F/K/N/P/U and its
other enabled editing bindings, while the dispatcher handles context-sensitive
REPL commands. Tab completes global/builtin names and attributes, or
inserts four spaces after whitespace (including a continuation newline).
Auto-indent is disabled consistently across profiles, so indentation remains
explicit. The LCD adapter supplies cursor movement and erasure without VT100
output or blocking input calls. Cursor-back emits `\b` through Betawise's
existing console handling, which owns cursor movement and wrapping. The
`GetCursorPos()` wrapper remains available to other callers. History strings and the input buffer use the
existing GC heap; no larger heap is reserved for readline.

Ctrl+C cancels unfinished input while preserving variables. Ctrl+D at an empty
primary prompt soft-resets Python, clearing variables, imports, and history.
During a compound statement, Ctrl+D finishes and executes the input; within a
nonempty line, it deletes at the cursor, following upstream readline behavior.

Complete image sizes compared with the earlier custom-editor builds are below.
Changes include readline and the OS3K feature exclusions, not readline alone.
Sizes include relocation bytes.

| Profile | Earlier image | Current image | Change |
| --- | ---: | ---: | ---: |
| minimal | 113,920 B | 118,672 B | 4,752 B (4.64 KiB) |
| core | 159,692 B | 164,372 B | 4,680 B (4.57 KiB) |
| basic | 162,504 B | 167,192 B | 4,688 B (4.58 KiB) |
| extra | 221,436 B | 221,676 B | 240 B (0.23 KiB) |

The pinned upstream event REPL restarts the prompt whenever its input buffer
is empty, even during editing. The port's `repl_process_char()` adapter sends
ordinary friendly-mode edits directly to readline, and sends Enter and
context-sensitive REPL commands to upstream pyexec. Ctrl+D on a nonempty line
remains an edit; on an empty line it finishes a compound statement or resets
the VM. Raw mode and paste-mode input stay on the upstream path. A single
paste-mode flag tracks that routing; the line buffer and compiler remain
upstream-owned. No upstream source is copied or modified. Editing and
completion receive a fresh GC stack anchor and exception guard on each event.

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

`time` is not enabled yet. `mp_hal_ticks_ms()` is retained for future time-module integration;
`select` and `_asyncio` are disabled. The common time module in `upstream/extmod/modtime.c`
requires five HAL functions (declared in `upstream/py/mphal.h`):

| Hook | OS3K implementation work |
| --- | --- |
| `mp_hal_ticks_ms()` | Implemented with `GetUptimeMilliseconds()`; requires the firmware timer service to be running. |
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
