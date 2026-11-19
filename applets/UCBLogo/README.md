# UCBLogo for AlphaSmart NEO

This is a SmartApplet that runs a small profile of the Berkeley Logo
interpreter. It opens at a blank `? ` prompt with the turtle at the center
of the right pane. Type `repeat 4 [fd 20 rt 90]` to draw a sample square.
Use **Ctrl+U** to clear the input, **Backspace** to edit, and **Enter** to
evaluate. Try `print 42` for text output or `cs` to clear the canvas and
return the turtle to its origin. Use `ct` or `cleartext` to clear the
transcript and return to the top-left prompt.

Define a procedure by entering each line separately:

```logo
to square :size
  repeat 4 [fd :size rt 90]
end
square 20
```

The prompt changes to `>` while collecting the body and returns to `?`
after `END`. Use **Ctrl+C** to discard an unfinished definition. The body
buffer holds up to 1,023 bytes including newlines. Definitions live in
memory for the current applet session. Use `erase [square]` or
`er [square]` to remove a procedure before defining it again.

The profile includes forward, back, left, right, pen up and down, home, clear
screen, repeat, repcount, print, and show/hide turtle (`ST`/`HT`). The left pane
keeps a scrolling transcript of submitted commands and output. It shows a
visible cursor at the end of the current input. File operations and the rest of
the upstream primitive set are outside this profile. Execution has a step limit
so a large loop or move yields back to OS3K.

## Build

Run from the BetaWise root to initialize the pinned UCBLogo submodule and
build the applet.

```sh
git submodule update --init applets/UCBLogo/upstream
make -C os3k
make -C applets/UCBLogo -j4
```

The output is `applets/UCBLogo/UCBLogo.OS3KApp`. The pristine submodule is
pinned to [UCBLogo 6.2.5](https://github.com/jrincayc/ucblogo-code/releases/tag/version_6.2.5)
(commit `a0215433f07384e4c42fe247525867f11e6f32f7`). Set `UPSTREAM` to use
another checkout of the same revision.

`prepare.py` copies the upstream sources into `build/` and adapts those
copies for OS3K. It registers only the supported primitives, embeds the
English message table, reduces the node segment size, adds an evaluation
step limit, and routes text and turtle drawing through the applet's hooks.
The build uses an OS3K configuration, a small runtime, and software floating
point in place of the desktop entry point and terminal backend. The turtle
outline uses the triangle geometry from the 6.2.5 release.

UCBLogo is GPLv3 or later under the upstream source notices. Floating
point uses the [OS3K compiler runtime](../../os3k/compiler-rt/README.md).

## OS3K backend

The applet consumes OS3K message events directly and sends Logo text output to
its left pane. `term.c` is excluded from the build, so there is no termcap
dependency and no need for a BetaWise terminal emulator. The build embeds the
English Messages table and supplies a bounded heap. UCBLogo computes turtle
motion and calls BetaWise `DrawLine` for each pen stroke. `DrawLine` updates the
LCD through `DrawPixel` and OS3K `RasterOp`. The turtle outline uses pixel
inversion so removing it restores the pixels underneath. The text pane writes
font glyphs directly and stores only eight rows of characters for scrolling and
focus redraw. When the transcript scrolls, the left LCD controller advances its
start line by eight pixels and reuses the old top row as the new bottom row. The
graphics pane clears when the applet regains focus, while the text transcript
remains available.

## Emulator test and memory

Use the [NEO MAME setup](../../tests/mame/asneo/README.md) with the stock
AlphaWord Plus and the UCBLogo image in the flash seed. Open **UCBLogo** in
SmartApplets. The right pane should show the turtle on a blank canvas. For the
automated run, start with fresh NVRAM and add these options to the
documented MAME command. Use an absolute path for the Lua script.

```text
-video none -sound none -nothrottle -seconds_to_run 130
-autoboot_delay 0 -autoboot_script /absolute/path/to/applets/UCBLogo/smoke.lua
```

The optional script checks the blank prompt, types the square command, then
types and runs `print 42` and `cs` twice without clearing input between
commands. It also checks hiding, turning, and showing the turtle and defines and
calls a parameterized square procedure, then erases and redefines it. The
verified run showed the expected pixels for all four edges, `42` in the output
row, and a cleared trail after `cs`. Physical NEO hardware has not been tested.

The earlier memory measurement build was 63,480 bytes and requested 65,128
bytes of applet RAM, excluding the firmware call stack. That includes a
49,152-byte heap, a 4,000-byte initial GC mark stack and a 2,112-byte LCD
bitmap.

| Checkpoint | Heap in use | Node slots | Occupied nodes | Free slots |
| --- | ---: | ---: | ---: | ---: |
| Initialized, before drawing | 39,088 B | 1,024 | 902 | 72 |
| After the square | 44,064 B | 1,152 | 955 | 147 |
| After `print 42` | 44,260 B | 1,152 | 976 | 126 |
| After `cs` | 44,412 B | 1,152 | 989 | 113 |

Each node slot is 36 bytes. The 50 emergency reserve slots are separate
from the occupied and free counts. The square triggered one more
128-slot segment, adding 4,608 bytes of node storage. In a separate run
that forced full GC at each checkpoint, the collector retained 889
nodes before drawing and 922 after the square. Forced collection changes
the allocator's behavior, so the table uses a run without it.

Fresh emulator runs with 39, 40 and 42 KiB heaps produced the same
screens as the 48 KiB baseline. Each peaked at 39,568 bytes and rejected
one allocation. The 38 and 43 KiB runs failed the screen comparison.
The 44 KiB run passed with no rejected allocations. The default remains
48 KiB.

To reproduce a smaller build, run the following from this directory.
Use `HEAP_KIB=48` to restore the default image. Add `PROFILE_GC=1` to
measure retained nodes via forced full collection. The smoke script
prints the counters to the MAME log.

```sh
make -W runtime.c HEAP_KIB=39
```
