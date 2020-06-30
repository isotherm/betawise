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

Work in Progress
----------------
Much research and work is still needed:
* Improve stdio compatibility
* Make betawise static library for easier linking
* Investigate different statuses from `ProcessMessage`
* Find and document timer functions and messages
* Find and document graphic drawing routines
* Find and document non-message keyboard functions
* Eliminate need to put global data into a structure

Contributions welcome!
