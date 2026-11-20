# Terminal Font

This is a monospaced font to allow a readable 44x8 terminal on the AlphaSmart Neo.
It is provided as a standard Neo Font so that it may also be used in AlphaWord.

The font is intended to replicate, as much as possible, the font used on the
AlphaSmart 3000, which was driven by an ST7066 or similar LCD controller.

## Console font sizes

The selected faces are X11 Misc Fixed 4×6 (`glyphs-4x6.png`), X11 Misc Fixed
5×7 (`glyphs-5x7.png`), and the original Terminal 6×8 (`glyphs.png`).
With full cells on the Neo's 264×64 LCD, they fit 66×10, 52×9, and 44×8
characters respectively. `make` builds all three applets.

| File | Face |
| --- | --- |
| `NeoFontTerminal.OS3KApp` | Original Terminal 6×8 |
| `NeoFontTerminal-Small.OS3KApp` | Small 5×7 |
| `NeoFontTerminal-Tiny.OS3KApp` | Tiny 4×6 |

Install one font at a time. The variants share applet ID `0xA1F0`.
