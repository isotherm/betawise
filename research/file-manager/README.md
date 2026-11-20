# AS3000 File Manager address-error investigation

The original applet reproduced the user's **address error at `0x3FF8D`** in
AS3000 System 3.01 MAME. A minimal installation put the reported next instruction
at `0x4087D6`; the user's `0x423EEA` depends on their installed applet layout/build.
Both the address and observed generated instruction identify a stack alignment
fault, independent of file contents.

The baseline `RedrawScreen` began:

```asm
link.w  a6,#-52
movem.l d2/a2-a5,-(sp)
move.l  #0x53797374,-37(a6)   ; "Syst" -> odd address 0x3FF8D
move.l  #0x656D0000,-33(a6)   ; "em"
```

`char name[37] = "System"` allowed an odd stack address. GCC lowered its
aggregate initialization to `MOVE.L`; `-fno-store-merging` does not prevent this
aggregate initializer lowering. A real 68000 cannot access an odd word/longword.
The first instruction faults; the firmware reports the next instruction.

The fix uses a 38-byte explicitly two-byte-aligned name buffer, initializes only
its first byte, and pads other applet-name buffers/rows to an even stride.
It also removes the obsolete local pointer-return workaround in favor of the
SDK's now-existing `FileAccessName` ABI bridge. Return handling was independently
fixed: AS3000 sends `MSG_CHAR('\r')`, so the chooser normalizes Return into its
existing Enter navigation path.

## Native firmware reproduction and verification

```sh
make -C os3k
make -C applets/FileManager
python3 research/file-manager/run.py /tmp/os3k-file-manager
```

The output directory must be new. The runner requires the existing local stock
AS3000 SmallROM, System 3.01 and AlphaWord Plus images. It builds a disposable
flash fixture, lets MAME install File Manager after AlphaWord, and uses **native
cold boot, directory discovery, keyboard handling and file API operations**.
No guest file descriptors or allocator records are synthesized. No proprietary
ROMs are copied into the repository. Firmware/font files remain local.

Coverage: enter File Manager, choose AlphaWord's folder with Return, list File 1
and tag `0x002D`, open Rename, replace the name with `script.py`, commit the name,
open Delete confirmation, cancel, and verify the name/tag survive. Screenshots
and logs are saved under the supplied output directory. LCD save-item text has
some glyph-byte substitutions in this MAME build; actual rendered screenshots
show the expected text. Assertions use unaffected substrings plus the tag.

The baseline address error was reproduced; the fixed chooser, native listing,
rename dialog, committed rename, delete confirmation and cancellation all
passed. See the execution output for pass markers. These checks
do not certify moves, file deletion, all password policies, NEO, warm installation,
or physical hardware. The removed host `test_move.c` has not been restored.
