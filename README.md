# BetaWise
Several custom applets for the AlphaSmart NEO / NEO2, and tools to create them.

## Compiling
Any `m68k-elf` gcc cross compiler should work. On Windows, I have successfully
used [MinGW GCC for M68K](https://sourceforge.net/projects/mingw-gcc-68k-elf/).

Initialize the pinned MicroPython submodule before building all applets
from the repository root.

```sh
git submodule update --init applets/MicroPython/upstream
make -j4
```

OS3K applets should support two-byte alignment for their RAM and flash
bases. Firmware placement may put either base two bytes past a
four-byte boundary, even when the linker aligns offsets within an
applet image.

## Testing
Run `make -C tests/qemu check` for the relocation and startup tests.
For firmware-driven applet tests, see [AS3000 MAME tests](tests/mame/asma3k/README.md)
and [NEO MAME tests](tests/mame/asneo/README.md).

## Installing Custom Applets
Using [Neo Manager](https://support.renaissance.com/techkb/techkb/13002475e.asp):
* File → Add to Applet List... → select [AppletFile].OS3KApp
* SmartApplets tab → select [AppletName] → Add
  * Optional: checking *Delete SmartApplets that are not in the Install List…*
    saves you a keypress or two during debug (**will delete existing applets**)
* Send List tab → Send

Note: On Windows 7 x64, Neo Manager stores custom applets in
`%LocalAppData%\VirtualStore\Program Files (x86)\AlphaSmart\AlphaSmart Manager 2\SmartApplets`

## Tips
* To start in applet chooser, hold `Left Shift` + `Tab` while turning on.
* To reset to defaults, hold `Right Shift` + `Backspace` while turning on. The
  reset password is `tommy`.

## Work in Progress
Much research and work is still needed:
* Improve stdio compatibility
* Investigate different statuses from `ProcessMessage`
* Find and document timer functions and messages
* Find and document non-message keyboard functions
* Eliminate need to put global data into a structure

Contributions welcome!
