# AlphaSmart NEO tests in MAME

Use the `asneo` driver from the asneo-emu overlay. The Betawise
`tests/mame/asma3k/check.sh` runner targets the AS3000. The Neo procedures here
use original Neo firmware and a disposable copy of virtual flash.

## Apply and build the overlay

The examples use Bash and assume `betawise`, `mame`, and `asneo-emu`
are sibling checkouts. Set `workspace` to their parent directory.

```bash
workspace=/home/kirk/Projects/Alphasmart
mame_dir="$workspace/mame"
overlay="$workspace/asneo-emu"
```

If your MAME checkout already has the local `neo-overlay` branch, select
it with `git -C "$mame_dir" switch neo-overlay`. It contains the driver
import (`ec9d9c8833b`) and the LCD scrolling fix (`55ac737039b`).

To recreate that overlay from the AS3000 `alphasmart` branch, run the
following with a clean working tree and the asneo-emu checkout at
`d3595f6`. The two device configuration edits adapt the driver to this
MAME checkout's API.

```bash
git -C "$mame_dir" switch -c neo-overlay alphasmart
cp "$overlay"/src/mame/skeleton/{alphasneo.cpp,neo_*.h} \
    "$mame_dir/src/mame/skeleton/"
bash "$overlay/scripts/register-neo-driver.sh" "$mame_dir"
sed -i 's/PRINTER(cfg,m_usb_printer);/PRINTER(cfg,m_usb_printer,0);/' \
    "$mame_dir/src/mame/skeleton/alphasneo.cpp"
sed -i 's/BITBANGER(config,m_stream);/BITBANGER(config,m_stream,0);/' \
    "$mame_dir/src/mame/skeleton/neo_vz_uart.h"
sed -i 's/(y+c.start_line)%128/(y+c.start_line)%height/' \
    "$mame_dir/src/mame/skeleton/neo_lcd.h"
```

Build the Neo-only executable and confirm the driver is registered.

```bash
make -C "$mame_dir" -j2 SUBTARGET=neodiag SOURCES=src/mame/skeleton/alphasneo.cpp
"$mame_dir/neodiag" asneo -listmedia
```

## Run the existing MicroPython session

If the original test directory is still available, copy its saved flash
and RAM before starting MAME. It contains MicroPython with wrapped input
(`01f429c`), NeoFontTerminal, and stock AlphaWord Plus.

```bash
test_run=/tmp/bw-neo-cursor.pcwFOh
session=$(mktemp -d)
cp -a "$test_run/cfg" "$session/cfg"
cp -a "$test_run/long-line" "$session/nvram"
cd "$session"

/home/kirk/Projects/Alphasmart/mame/neodiag asneo \
    -rompath "$test_run/roms" \
    -cfg_directory cfg \
    -nvram_directory nvram \
    -window
```

Press **F9** for SmartApplets, select **MicroPython** with the arrow keys,
and press **Enter**. Betawise selects NeoFontTerminal automatically.

## Create a fresh test session

Use private copies of the Neo update packages `os3kneorom.os3kos` and
`smallos3kneorom.os3kos`, plus the stock Neo
`alphawordplus.os3kapp`. The package SHA-1s declared by this driver are
`30ebd3be9da12186b8f8146b251f0e21b95e74ad` for the system package and
`78a3bcbb81773335f69c8964ff8a114e0d7712cf` for SmallROM.

Build the applets from Betawise and stage the packages. Adjust
`neo_files` to point to your local firmware and stock applet directory.

```bash
betawise_dir="$workspace/betawise"
neo_files="$workspace/Ghidra/Alphasmart Neo/SmartApplets"
git -C "$betawise_dir" submodule update --init applets/MicroPython/upstream
make -C "$betawise_dir/os3k"
make -C "$betawise_dir/applets/NeoFontTerminal"
make -C "$betawise_dir/applets/MicroPython" -j4

session=$(mktemp -d)
cd "$session"
mkdir -p roms/asneo cfg nvram/asneo
ln -s "$neo_files/os3kneorom.os3kos" roms/asneo/
ln -s "$neo_files/smallos3kneorom.os3kos" roms/asneo/
"$mame_dir/neodiag" asneo -rompath roms -verifyroms
SDL_VIDEODRIVER=dummy "$mame_dir/neodiag" asneo -rompath roms \
    -cfg_directory cfg -nvram_directory seed -video none -sound none \
    -nothrottle -seconds_to_run 2
```

The short headless run assembles the update packages into a 4 MiB
`seed/asneo/flashnv` file. Append the applets to a copy of that image at
the Neo applet storage offset, `0x70000`. Keep only the seeded flash in
`nvram/asneo` so the firmware rebuilds the directory on first boot.

```bash
python3 - "$neo_files" "$betawise_dir" <<'PY'
from pathlib import Path
import sys

neo_files, betawise = map(Path, sys.argv[1:])
images = [
    neo_files / "alphawordplus.os3kapp",
    betawise / "applets/MicroPython/MicroPython-core.OS3KApp",
    betawise / "applets/NeoFontTerminal/NeoFontTerminal.OS3KApp",
]
flash = bytearray(Path("seed/asneo/flashnv").read_bytes())
assert len(flash) == 0x400000, "unexpected flash size"
offset = 0x70000
for image in images:
    data = image.read_bytes()
    end = offset + len(data)
    assert data[:4] == bytes.fromhex("c0ffeead"), image
    assert int.from_bytes(data[4:8], "big") == len(data), image
    assert end <= len(flash) and all(b == 0xff for b in flash[offset:end]), image
    flash[offset:end] = data
    offset = end
Path("nvram/asneo/flashnv").write_bytes(flash)
PY

cat > cfg/asneo.cfg <<'XML'
<mameconfig version="10">
  <system name="asneo">
    <input>
      <port tag=":NOR_PROFILE" type="CONFIG" mask="1" defvalue="0" value="1" />
    </input>
  </system>
</mameconfig>
XML

"$mame_dir/neodiag" asneo -rompath roms -cfg_directory cfg \
    -nvram_directory nvram -window
```

The config selects the ST NOR profile so the firmware detects the flash
geometry and scans the applets. Answer **Y** at the backup battery prompt,
then press **Enter** when asked to restart. Open SmartApplets with **F9**.

## Smoke checks

- Enter `6*7` and check that the REPL prints `42`.
- Check that the banner and prompt start at the left edge on separate rows,
  and that the blinking cursor stays within the eight-pixel prompt row.
- Enter `1` several times. Check that the result and next prompt stay
  visible as the console scrolls.
- Enter `len("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ")`.
  Check that the input wraps and the result is `52`.

For Hello World or DebugTool, substitute its built `.OS3KApp` for the
MicroPython image in the seeding step and use a fresh session. USB applet
installation is outside this procedure.

## Readline Backspace regression

`MicroPythonBackspace.lua` uses a fresh virtual flash containing AlphaWord,
UCBLogo, MicroPython EXTRA, and NeoFontTerminal, in that order. Launch it with
`-autoboot_delay 0 -autoboot_script /absolute/path/MicroPythonBackspace.lua`.
It opens MicroPython through native keyboard events, types `a=1234`, deletes
the final digit, and evaluates `a`. Screenshots show `a=123` and the result
`123`. It also deletes a one-character draft to empty, presses Backspace again,
and deletes from the middle of an assignment, checking the edited result `124`.
The empty-input checks require the cursor to remain just after one prompt.
It also verifies that generic Ctrl+U clears a draft without changing the
previously evaluated value.
The probe checks Neo 3.21 cursor-structure coordinates at RAM addresses
`0x5c72/0x5c74`; those fixed-address reads belong to the test, not the applet.
The AS3000 `micropython-repl-*` cases also exercise Backspace.

## Offline screen evaluation (required for iteration and acceptance)

Use local bitmap matching for Neo screen checks. Do not use OpenAI/model vision
as the screen reader. Screenshot capture itself needs no model or network.
Install Pillow in the test environment; no OCR service, API key or ML model is
required. The canonical font is `applets/NeoFontTerminal/glyphs.png`, rather than
a separately copied font strip.

Run the full headless Backspace/middle-edit/empty-input/Ctrl+U regression:

```bash
python3 tests/mame/asneo/check.py
# Override locations if necessary:
python3 tests/mame/asneo/check.py --mame ../mame/neodiag \
  --firmware '../Ghidra/Alphasmart Neo/SmartApplets' --output /tmp/neo-check-new
```

The runner builds the current applet sources and prepares disposable flash from
local firmware (MicroPython EXTRA by default), sends native keyboard events, retains cursor RAM
checks, and asserts actual rendered assignments/results, arithmetic, wrapped input
and scrolling (12 checkpoints). At each checkpoint Lua pauses the key schedule,
calls the real `SetCursorMode(CURSOR_MODE_HIDE)` firmware API, waits two display
frames, and captures two PNGs on the next two frames (about 50 and 67 ms). It then
restores the original cursor mode and resumes the guest. Python requires two
identical cursor-free readings; unknown glyphs or cursor pixels fail.

The capture helper temporarily preserves CPU registers and 4 KiB of scratch RAM
while calling the firmware on a private stack. It does not edit framebuffer pixels
or modify the applet. Fixed cursor addresses and this test helper are specific to
Neo 3.21 / the documented 2013 firmware, not a portable OS API. The runner saves
logs, input hashes, screenshots, decoded `.txt` files and `results.json`. Successful
Lua cursor checks alone do not mean the screen checks passed.

Decode a single native screenshot or an exact integer enlargement:

```bash
python3 tests/mame/asneo/neo_reader.py screen.png --json --expect '123'
python3 tests/mame/asneo/neo_reader.py --burst checkpoint-01-*.png --expect '>>> a=1234'
python3 tests/mame/asneo/check.py --verify-only /tmp/neo-check-new
python3 -m unittest discover -s tests/mame/asneo -p 'test_*.py'
```

Burst files must be ordered captures from one idle checkpoint. Do not use unrelated
screens as a burst. Unknown glyphs, ambiguous nonblank glyphs, cursor overlays,
nonuniform scaling and unsupported image geometry fail with a nonzero exit code.
The reader preserves spaces and internal blank lines and reports zero-based cell
coordinates for failures. It uses exact pixels, with no fuzzy nearest-character
matching and no network calls. A malformed image or screenshot with another font
must not be accepted as evidence.

The old screenshot-only smoke procedure above is superseded for terminal text:
assert exact full lines for arithmetic (`42`), wrapped length (`52`), banner/prompt
layout and scrolling. When adding a new scenario, add its idle capture checkpoint
and expected text to the regression. Cursor geometry still needs independent
coordinate assertions. Firmware menus, proportional fonts and Logo graphics need
verified font profiles or deterministic pixel checks; this 6x8 reader must fail
closed for those screens, never fall back to AI inspection. Historical screenshots with a visible cursor remain unverified; rerun the scenario. Fixtures in `fixtures/` preserve one clean
and one cursor-on real MAME screenshot from the supplied historical session.

## Neo readline parity and Logo text coverage

`check.py` now runs the 12-check baseline followed by the maintained extended
suite. Use `--baseline-only` to run just the original regression. The extended
suite ports the AS3000 `MicroPythonRepl.lua` scenarios to native Neo matrix keys:

- Editing: Backspace/delete to empty, Ctrl+U/F/K and insertion in the middle.
- Terminal: clear/home, escape sequences split across Python writes, absolute and
  relative cursor placement/clamping, all line-erase modes, display erasing above
  and below the cursor, and clearing/positioning after hardware scrolling. Exact
  regions assert text placement and blank cells, with the REPL prompt outside them.
- History: Up, Ctrl+P recall, Ctrl+N returning to empty input, then execution.
- Completion: user-defined names, attributes, Tab indentation, and state surviving
  allocation stress; EXTRA also checks scheduled callback order.
- Interrupts: draft cancellation, Ctrl+C interrupting an infinite loop and
  preserving interpreter state; EXTRA also checks reassignment to Ctrl+X/plain x.
- Modes: Ctrl+D soft reboot clears state; raw execution, return to friendly REPL,
  paste execution/cancellation and continued friendly execution.
- Logo text: `print`, the square command transcript, procedure definition,
  parameter passing, erase/redefinition, Backspace, Ctrl+U, definition prompts/
  cancellation and `ct`/`cleartext` clearing the transcript.

Run selected cases without rerunning the baseline:

```bash
python3 tests/mame/asneo/check-repl.py
python3 tests/mame/asneo/check-repl.py --case terminal
python3 tests/mame/asneo/check-repl.py --case history completion modes
python3 tests/mame/asneo/check-repl.py --variant core --case interrupts modes
python3 tests/mame/asneo/check-repl.py --verify-only /tmp/saved-run/history
```

Profiles `minimal`, `core`, `basic`, and `extra` use their corresponding built
image; EXTRA-only checks are gated explicitly. The default remains EXTRA. Both runners rebuild current sources before testing;
use `--no-build` only when deliberately testing existing binary artifacts. Build
output is retained in `build.log`.
Each case has fresh disposable flash, a generated native-key script and explicit
expectations in `checks.json`. The runner requires a completion marker, no Lua
errors, and exact local screenshot assertions; it retains logs, screenshots,
decoded text, input hashes and `results.json`. An echoed command does not satisfy
an expected full output line.

Logo draws a separate, solid editor cursor. Its text tests first fill the display,
then decode the completed transcript at `(0,0,132,56)`, excluding the eighth-row
editor and the entire right graphics pane. Prompt-state checks read only the first
cell, away from the cursor. After `ct`/`cleartext`, separate regions verify the
empty rows and the prompt returning to the top. This tests output and completed input,
not the live editor cursor or turtle drawing. Region decoding still rejects every
unknown glyph in the selected region. For example:

```bash
python3 tests/mame/asneo/neo_reader.py logo.png --region 0 0 132 56 --expect '42'
```

## Logo graphics checks

The default suite includes `logo-graphics`; run it separately with:

```bash
python3 tests/mame/asneo/check-repl.py --case logo-graphics
```

Twenty checkpoints compare the entire right pane `(132,0,132,64)` with independent
Pillow geometry and stored monochrome references in `fixtures/logo-graphics/`.
The square is drawn with `ht` first: all four expected edges must be present,
its interior empty, and no extra pixels may exist. Bounds alone cannot qualify a
PASS. A 20-step square has inclusive bounds x=66..86/y=12..32 and 80 edge pixels,
excluding the separator.

The suite also checks the initial turtle, show/hide over existing strokes without
leaving damage, text scrolling without graphics movement, repeated clears,
pen-up/down, backward retracing, the diagonal home stroke, a parameterized
10-step square, right/left turns, moving with the pen up and cleartext preserving
the graphics. Turtle references use explicit cardinal-heading vertices; they do
not derive expectations from the applet's rasterizer or screenshots.

Every checkpoint requires two identical right-pane captures. Reports include
foreground bounds/counts and reference hashes. Actual/expected pane PNGs are saved;
a mismatch additionally saves a difference PNG and fails with the wrong-pixel
count. The tests never update reference images automatically. See the fixture
README for coordinates and reference authoring instructions.

Interrupt keys during running code are held for 30 emulated frames to span the
VM's periodic keyboard scans. Ordinary typing uses shorter taps. These checks
validate interrupt assignment and recovery, not minimum keypress duration or
physical keyboard latency.
