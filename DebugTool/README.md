Debugging Tool
==============
* `Tab`: Move cursor between nibble mode (left side) and byte mode (right side).
* `Left`, `Right`: Move the cursor left or right by one nibble or byte.
* `Up`, `Down`: Move the cursor up or down by one line (8 bytes).
* `Ctrl`+`Up`, `Ctrl`+`Down`: Move the cursor up or down by one screen (32 bytes).
* `Home`, `End`: Go to beginning or end of line.
* `Ctrl`+`Home`, `Ctrl`+`End`: Go to beginning or end of screen.
* `Ctrl`+`G`: Go to address.
  * The address may be entered as decimal or hex (prefixed by `0x`).
  * The address may have a suffix of `,a5` to access the debugger global data.
    The debugger reserves the first 256 bytes of this data as a scratch area.
* `Ctrl`+`I`: Invoke function with up to 6 arguments.
  * The function address and parameters may be entered as above.
  * A function address with a `!` prefix indicates a system call, e.g. `!0`
    calls `ClearScreen`, `!1` calls `SetCursor`, etc.
  * A parameter beginning with a `$` prefix creates a temporary string (null
    terminated) containing all characters following the `$`.
* `Ctrl`+`Shift`+`G`: Go to address specified by memory at cursor.
* `Backspace`: Restore cursor to address prior to last jump.
* `Ctrl`+`R`: Refresh the screen (i.e. if displayed memory is volatile).
