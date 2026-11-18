# MicroPython on OS3K

The OS3K port lives beside the upstream MicroPython submodule, which is
pinned to v1.29.0. Initialize the submodule and build from the Betawise
root with the m68k GCC toolchain.

```sh
git submodule update --init applets/MicroPython/upstream
make -C os3k
make -C applets/MicroPython/os3k -j4
```

Mount `applets/MicroPython/os3k/MicroPython.OS3KApp` in MAME's AS3000
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

The applet uses the [OS3K compiler runtime](../../../os3k/compiler-rt/README.md)
for both integer helpers and soft float.
