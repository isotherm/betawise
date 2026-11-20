# UCBLogo for AlphaSmart NEO

Draw with turtle graphics and write small Logo procedures on your AlphaSmart.
Install `UCBLogo.OS3KApp` using [Neo Manager](../../README.md#installing-custom-applets),
then select **UCBLogo** from the SmartApplets menu.

## Draw your first square

The applet opens at a `?` prompt. The left pane shows your commands and output;
the right pane shows the turtle and its drawing. Type this and press Enter:

```logo
repeat 4 [fd 20 rt 90]
```

Try `print 42` for text output. Use `cs` to clear the drawing and return the
turtle to the center. Use `ct` to clear the text pane.

## Commands

| Command | Action |
| --- | --- |
| `fd 20` / `forward 20` | Move forward 20 steps |
| `bk 20` / `back 20` | Move backward 20 steps |
| `rt 90` / `right 90` | Turn right 90 degrees |
| `lt 90` / `left 90` | Turn left 90 degrees |
| `pu` / `penup` | Move without drawing |
| `pd` / `pendown` | Draw while moving |
| `home` | Return the turtle to its origin |
| `cs` / `clearscreen` | Clear the drawing and return the turtle to its origin |
| `st` / `showturtle`, `ht` / `hideturtle` | Show or hide the turtle |
| `repeat 4 [fd 20 rt 90]` | Repeat a list of commands |
| `repcount` | Report the current repeat count inside a loop |
| `print 42` | Print a value in the text pane |
| `ct` / `cleartext` | Clear the text pane |
| `erase [square]` / `er [square]` | Remove a procedure |

## Define a procedure

Enter each line separately:

```logo
to square :size
  repeat 4 [fd :size rt 90]
end
square 20
```

The prompt changes to `>` while you enter the body, then returns to `?` after
`end`. Call `square 30` to draw another size. Erase a procedure before defining
it again, using `erase [square]`.

## Editing and session behavior

| Key | Action |
| --- | --- |
| Enter | Run a command or submit a procedure line |
| Backspace | Edit the current input |
| Ctrl+U | Clear the current input |
| Ctrl+C | Discard an unfinished procedure definition |

The text pane scrolls as you enter commands. When you return from another
applet, the text transcript remains, but the graphics pane clears.

## Limitations

This is a small Logo profile with the commands listed above. File operations
and the rest of the desktop interpreter's primitive set are unavailable.
Procedures stay in memory for the current applet session and cannot be saved
to a file. A procedure body can contain up to 1,023 bytes, including newlines.
Long loops and moves have an execution step limit, so they may stop before
finishing.

## Building and technical notes

See the [UCBLogo research notes](../../research/ucblogo/README.md) for building
from source, emulator tests, backend details, and memory measurements.
The applet uses UCBLogo 6.2.5 and is licensed under GPLv3 or later.
