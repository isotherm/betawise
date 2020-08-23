Debugging Tool
==============

**IMPORTANT**: Back up all documents first! Use of the debug tools applet
allows modifying memory, performing I/O, and calling arbitrary routines. A
system reset may be required after certain operations. A bricked unit could
also result from indiscriminate use. Proceed at your own risk!

* `Tab`: Move cursor between nibble mode (left side) and byte mode (right side).
* `Left`, `Right`: Move the cursor left or right by one nibble or byte.
* `Up`, `Down`: Move the cursor up or down by one line (8 bytes).
* `Ctrl`+`Up`, `Ctrl`+`Down`: Move the cursor up or down by one screen (32 bytes).
* `Home`, `End`: Go to beginning or end of line.
* `Ctrl`+`Home`, `Ctrl`+`End`: Go to beginning or end of screen.
* `Ctrl`+`G`: Go to address.
  * The address may be entered as decimal or hex (prefixed by `0x`).
  * The debugger reserves 256 bytes for use as a user scratch area. The address
    may have a suffix of `+` to index into access this scratch pad.
* `Ctrl`+`I`: Invoke function with up to 6 arguments.
  * The function address and parameters may be entered as above.
  * A function address with a `!` prefix indicates a system call, e.g. `!0`
    calls `ClearScreen`, `!1` calls `SetCursor`, etc.
  * A parameter beginning with a `$` prefix creates a temporary string (null
    terminated) containing all characters following the `$`.
  * When ready, select "Call" and press `Enter` to invoke. Use `Shift`+`Enter`
    to wait for a key before printing the return value after the call.
* `Ctrl`+`Shift`+`G`: Go to address specified by memory at cursor.
* `Backspace`: Restore cursor to address prior to last jump.
* `Ctrl`+`R`: Refresh the screen (i.e. if displayed memory is volatile).
