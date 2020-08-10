BetaWise
========
Tools to create applets for the AlphaSmart NEO / NEO2.

**IMPORTANT**: Back up all documents first! Use of the debug tools applet
allows modifying memory, performing I/O, and calling arbitrary routines. A
system reset may be required after certain operations. A bricked unit could
also result from indiscriminate use. Proceed at your own risk!

Compiling
---------
Any `m68k-elf` gcc cross compiler should work. On Windows, I have successfully
used [MinGW GCC for M68K](https://sourceforge.net/projects/mingw-gcc-68k-elf/).

Installing Custom Applets
-------------------------
Using [Neo Manager](https://support.renaissance.com/techkb/techkb/13002475e.asp):
* File → Add to Applet List... → select [AppletFile].OS3KApp
* SmartApplets tab → select [AppletName] → Add
  * Optional: checking *Delete SmartApplets that are not in the Install List…*
    saves you a keypress or two during debug (**will delete existing applets**)
* Send List tab → Send

Note: On Windows 7 x64, Neo Manager stores custom applets in
`%LocalAppData%\VirtualStore\Program Files (x86)\AlphaSmart\AlphaSmart Manager 2\SmartApplets`

Debugging Tool
--------------
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

Tips
----
* To start in applet chooser, hold `Left Shift` + `Tab` while turning on.
* To reset to defaults, hold `Right Shift` + `Backspace` while turning on. The
  reset password is `tommy`.

Work in Progress
----------------
Much research and work is still needed:
* Improve stdio compatibility
* Make betawise static library for easier linking
* Investigate different statuses from `ProcessMessage`
* Find and document timer functions and messages
* Find and document non-message keyboard functions
* Eliminate need to put global data into a structure

Contributions welcome!
