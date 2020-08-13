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

Tips
----
* To start in applet chooser, hold `Left Shift` + `Tab` while turning on.
* To reset to defaults, hold `Right Shift` + `Backspace` while turning on. The
  reset password is `tommy`.

Work in Progress
----------------
Much research and work is still needed:
* Improve stdio compatibility
* Investigate different statuses from `ProcessMessage`
* Find and document timer functions and messages
* Find and document non-message keyboard functions
* Eliminate need to put global data into a structure

Contributions welcome!
