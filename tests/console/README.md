# Basic console prototype

Betawise character streams parse a small VT100 subset before calling the existing
display backend. This is per-applet code, not a ROM syscall hook. No formatter
resolver is used. ROM `printf`, `fprintf`, and scanning functions keep their
original paths; mixing their screen output with terminal streams is discouraged.

## Supported behavior

- `putchar`, `fputc(stdout)`, `puts`, and `fputs(stdout)` share persistent escape
  state across calls. Other output tokens retain ROM behavior.
- Printable bytes, CR, LF, BS, bell, and non-destructive eight-column tabs.
- CSI `A/B/C/D` moves the cursor with a default distance of one, clamped to the
  screen. CSI `H/f` sets one-based row/column, with omitted/zero values meaning 1.
- CSI `J/K` erases the display/line with modes 0, 1, and 2, preserving the cursor.
  `ESC [ 2 J` does not home the cursor: send `ESC [ H` separately.
- CSI `6n` queues `ESC [ row ; column R` for stdin. `ConsoleReadPending()` lets
  event-driven applets retrieve these replies without blocking on physical keys.
- `getchar`, `getc(stdin)`, and `fgetc(stdin)` read unsigned bytes without echo.
  Arrow keys generate normal-mode CSI A/B/C/D; ASCII Ctrl characters are encoded.
  Existing MicroPython Home/End/Delete sequences are retained. Key releases are
  ignored and the Applets key retains its system behavior.
- `fgets` reads at most size-1 bytes and NUL terminates; it retains the first CR
  or LF, does no editing/echo, and treats input escape sequences as bytes.

This deliberately retains Betawise's cooked LF (implicit carriage return),
backspace wrapping, and `puts` without an implicit newline. It is not a complete
VT100 or POSIX terminal. CSI motion stops at the screen edges; multirow
line-editor reflow is not implemented. Inverse video/SGR, application cursor mode, terminal
identification, screen-size queries, scroll regions, alternate screens, and
additional escape families are not implemented. Unsupported CSI sequences are
consumed without rendering; overlong or excess parameters are discarded through
the final byte. Cursor replies have a bounded 32-byte queue; a new reply is
ignored if there is insufficient room. Focus acquisition resets parser and
queued input/replies. The physical-key sequence already being read finishes
before a subsequent cursor report is delivered.

DebugTool retains puts/putchar: formatted hex/address strings contain no ESC,
and its glyph path substitutes controls before writing them. Its arrow keys
already use physical MSG_KEY events. File Manager uses direct display/event APIs. Logo's split
text/graphics UI and runtime are unchanged. MicroPython emits terminal cursor
and erase controls and passes ESC output through; it retains its event-driven
REPL and unsupported blocking Python stdin. Upstream readline's VT100 helpers
use snprintf/vsnprintf from shared/libc/printf.c, reusing mp_vprintf. Its overlapping
stdout functions have private build-local names and are discarded when unused,
so the SDK output HAL cannot recurse through upstream putchar.

## Checks

Run from the workspace root:

```sh
bash betawise/tests/console/check.sh
python3 betawise/tests/console/run-rom.py /tmp/betawise-console-rom
make -C betawise/applets/MicroPython -j4
bash betawise/tests/mame/asma3k/check.sh "$PWD/mame/mame" micropython-repl-core
bash betawise/tests/mame/asma3k/check.sh "$PWD/mame/mame" micropython-repl-extra
(cd betawise && python3 tests/mame/asneo/check-repl.py --case terminal)
```

The host test checks split escape sequences, defaults/clamping, partial erases,
malformed sequences, cursor reports, raw fallback streams, key sequences,
Ctrl+Space/NUL, fgets bounds, and focus/reset input behavior. The ROM probe
executes the production parser and renderer after original-ROM boot in MAME,
using disposable RAM/NVRAM and hash-checked AS3000 3.01/Neo 3.21 images. It checks
cursor invariants, position reports through getc, erase, and scroll-then-erase;
snapshots are retained as diagnostics. Neo screen acceptance now uses the
maintained exact-pixel reader and the `terminal` scenario below, rather than
model/human screenshot recognition. These are emulator tests, not physical-hardware
certification. The REPL checks exercise editing/history/control keys and Python
output containing clear/home, absolute positioning, and erase-line sequences.

The DebugTool memory check joins hex columns across rows: adding library state
can move the scratch buffer so an edited sequence spans two display rows.

Validated 2026-11-19: host contracts PASS; production ROM integration PASS on
AS3000 3.01 and Neo 3.21 (RAM/control-flow checks); AS3000 MicroPython core and
extra REPL suites PASS including terminal-output assertions; DebugTool memory
suite PASS. All four MicroPython profiles, DebugTool, File Manager, and unchanged
Logo build successfully. The extra REPL script finishes at frame 21426 on the
50 Hz display (about 429 emulated seconds), so its harness limit is now 480
seconds instead of 420. The two console objects contain 1907 bytes of code and
52 bytes of BSS in the current 68000 build, excluding existing renderer/glue.

Reverification after the Neo reader update adds 11 exact-pixel terminal
assertions in `tests/mame/asneo/check-repl.py --case terminal`. These exercise
Python output on the installed applet with NeoFontTerminal, using stable
cursor-free captures and exact text/blank-cell regions. They pass without
changing the production console. The native-key harness now maps @ to Shift+2
so the scenario can define its escape-output helper through actual keyboard
input. Cursor-position reports remain covered by the compiled ROM probe, not
by unsupported blocking Python stdin.

The full post-reapply verification passes: all four MicroPython profiles and
SDK/NeoFontTerminal/DebugTool/File Manager/Logo builds; 14 offline-reader/graphics
unit tests; host console contracts; AS3000 core/extra REPL and DebugTool memory
suites; console and formatter probes on both ROMs; 88 Neo rendered-screen/graphics
assertions (12 baseline, 11 terminal, 65 extended REPL/Logo). Artifacts are under
`/tmp/betawise-reverify-*`. Production code needed no fixes during this pass.
