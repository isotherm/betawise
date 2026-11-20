# MicroPython for OS3K

Run Python interactively on your AlphaSmart with the MicroPython SmartApplet.
Choose a profile below, install its `MicroPython-<profile>.OS3KApp` image using
[Neo Manager](../../README.md#installing-custom-applets), and select MicroPython
from the SmartApplets menu.

## Choose a profile

All profiles include the compiler, interactive REPL, garbage collection, and
software single-precision floats. Each profile includes the features of the
preceding profile.

### Minimal

`MicroPython-minimal.OS3KApp` — the smallest interpreter, with most optional
builtins and modules omitted. Uses a 16 KiB heap; the `math` module is unavailable.

### Core

`MicroPython-core.OS3KApp` — adds common builtins, sets, `bytearray`, `array`,
`namedtuple`, `gc`, `io`, `struct`, and `math`. Uses a 16 KiB heap.

### Basic

`MicroPython-basic.OS3KApp` — adds Unicode strings/checking and additional
builtin conveniences to core. Uses a 16 KiB heap.

### Extra

`MicroPython-extra.OS3KApp` — adds JSON, regex, binascii/CRC32, SHA-256, `random`,
`heapq`, `deflate`, framebuffer objects, `deque`/`OrderedDict`, `memoryview`,
diagnostics, cooperative scheduler callbacks, and standard streams. Uses a
64 KiB heap to accommodate the default 32 KiB deflate window.

`sys.stdout`, `sys.stderr`, and their binary buffers write to the console.
`framebuf` draws into RAM; it does not automatically update the LCD. `os` has no filesystem backend.

## Use the REPL

Type `print(6 * 7)` and press Return. Expressions display their results, and
assignments keep their values between commands. Enter multiline statements
with explicit indentation, then a blank line to execute them.

| Key | Action |
| --- | --- |
| Up / Down | Recall eight lines of history |
| Left / Right, Home / End | Move the cursor |
| Backspace / Delete | Edit input |
| Tab | Complete names and attributes, or insert four spaces after whitespace |
| Ctrl+C | Cancel unfinished input or interrupt running Python |
| Ctrl+D at an empty primary prompt | Reset Python, clearing variables, imports, and history |
| Ctrl+D on an empty continuation line | Finish and execute the input |
| Ctrl+D within a nonempty line | Delete at the cursor |

Long input lines wrap, and the input buffer grows within the available heap.
Auto-indent is disabled in every profile. Variables persist when switching
applets, and the prompt and unfinished input reappear when you return. A new
firmware initialization starts a fresh Python session.

Python commands execute synchronously. Long-running native C functions cannot
be interrupted until they return to the Python VM.
`micropython.kbd_intr()` changes the interrupt character (Ctrl+C by default;
`-1` disables it).

## Limitations

- Integers range from −536,870,912 to 536,870,911. Larger values, including
  some CRC32 results, raise `OverflowError`; there is no wide-integer fallback.
- Floats use single precision. Complex numbers and `uctypes` are unavailable.
- The `time`, `select`, and `_asyncio` modules are unavailable in every profile.
  Threads, networking, TLS, multiprocessing, and subprocesses are unsupported.
  Scheduled callbacks run cooperatively; they do not run in a separate thread.
- There is no filesystem adapter. File probes report no files, and `open()`
  raises `OSError(95)`.
- Blocking `input()` and stdin reads raise `OSError(95)`.

## Building and technical notes

To build an image from source, see the [build instructions](../../research/micropython/README.md#build-from-source).
The same page covers firmware tests, implementation details, and remaining
porting work.
