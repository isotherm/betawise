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

The `backspace` case in `check-repl.py` covers deletion at the end and middle,
deleting to empty, repeated Backspace, Ctrl+U, arithmetic, wrapping and scrolling.
It retains independent Neo 3.21 cursor-coordinate assertions at `0x5c72/0x5c74`.
The AS3000 `micropython-repl-*` cases also exercise Backspace.

## Direct LCD memory evaluation (required for iteration and acceptance)

Run the complete suite, or select cases:

```bash
python3 tests/mame/asneo/check.py
python3 tests/mame/asneo/check.py --baseline-only
python3 tests/mame/asneo/check-repl.py --case history completion modes
python3 tests/mame/asneo/check-repl.py --case logo-graphics --verify-rendering
python3 tests/mame/asneo/check-repl.py --verify-only /tmp/saved-run/history
python3 -m unittest discover -s tests/mame/asneo -p 'test_*.py'
```

Both runners build current sources by default and use disposable flash. Select
`--variant minimal|core|basic|extra` as appropriate; EXTRA is the default. Override
`--mame`, `--firmware` and `--output` for local paths. Use `--no-build` only when
deliberately testing existing artifacts.

`NeoLCD.lua` reads the two LCD controllers' DDRAM save items and display flags
without touching guest MMIO ports. It reconstructs native 264x64 pixels using the
page layout, scroll origin, axis direction, inversion, display-off and all-points
modes. `neo_reader.py` decodes exact glyphs from the canonical
`applets/NeoFontTerminal/glyphs.png`; unknown/ambiguous cells fail. There is no
network inference, OCR model or screenshot input in automated evaluations.
Pillow is used for the local font and independently authored graphics references.

Actions wait for RAM to remain unchanged across three observations, together with
input readiness and explicit expected text/graphics checkpoints. A change alone
is insufficient. Normal keys have a minimum 3-frame hold and 8-frame interval;
interrupt keys retain 30-frame holds to span the VM's periodic keyboard scans.
Logo acknowledges each character using its complete input tail and solid cursor,
and commands finish at an empty prompt. Action deadlines are 15 emulated seconds,
with a separate process watchdog. Startup recovery/menu navigation retains its
conservative fixed schedule. Run unthrottled at native 60 Hz, without frame skips;
`-speed 2` adds no benefit to `-nothrottle`.

At a checkpoint, the harness calls the real firmware cursor-hide API when needed,
observes its return, and records three identical complete RAM/flag samples. It
restores the cursor, CPU registers and scratch RAM before continuing. Captures
are `checkpoint-NN.lcd.json`; Python independently reconstructs and validates
them. Cursor helpers require the pinned Neo 3.21 firmware. Fixed addresses are
test instrumentation, not portable applet APIs. `--verify-rendering` additionally
compares all 16896 reconstructed pixels against MAME's display for diagnostics.

Runs retain RAM records, decoded text, logs, input hashes, explicit expectations
and `results.json`. Graphics actual/expected/difference PNGs are generated from
RAM for review. Failure screenshots are diagnostics only. Replay a saved record:

```bash
python3 tests/mame/asneo/neo_reader.py checkpoint-01.lcd.json --json --expect '123'
```

The reader also accepts historical native PNGs and integer enlargements for
manual investigation. Those are not the regression input. Historical captures
with a visible cursor remain unverified; rerun the scenario. Terminal decoding
supports the verified 6x8 font only; menus and other fonts require their own
verified profiles. Never fall back to model vision or guess covered characters.

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
errors, and exact local LCD RAM assertions; it retains logs, RAM records,
decoded text, input hashes and `results.json`. An echoed command does not satisfy
an expected full output line.

Logo draws a separate, solid editor cursor. Its text tests first fill the display,
then decode the completed transcript at `(0,0,132,56)`, excluding the eighth-row
editor and the entire right graphics pane. Prompt-state checks read only the first
cell, away from the cursor. After `ct`/`cleartext`, separate regions verify the
empty rows and the prompt returning to the top. This tests output and completed input,
with independent editor acknowledgements; graphics are checked separately. Region decoding still rejects every
unknown glyph in the selected region. For example:

```bash
python3 tests/mame/asneo/neo_reader.py logo.png --region 0 0 132 56 --expect '42'
```

## Reusable screen bitmap fixtures

See the [generic fixture guide](fixtures/README.md) for authoring, provenance and
review instructions. Applet subdirectories document only their own expectations.

`screen_bitmap.py` verifies any applet's screen against a reviewed black-and-white
PNG. It is independent of Logo, fonts, text decoding and shape recognition. A
fixture may cover the full 264x64 LCD or an arbitrary `(x, y, width, height)` region;
the reference PNG must have exactly that region's dimensions. Every selected
pixel must match, including the background. Blank regions are supported.

```bash
python3 tests/mame/asneo/screen_bitmap.py /tmp/run/checkpoint-01.lcd.json \
  --reference tests/mame/asneo/fixtures/my-applet/expected-screen.png --output /tmp/bitmap-check
# Or check just a region:
python3 tests/mame/asneo/screen_bitmap.py /tmp/run/checkpoint-01.lcd.json \
  --reference tests/mame/asneo/fixtures/my-applet/expected-region.png --region 12 8 120 40 \
  --output /tmp/bitmap-region
```

From Python, use `screen_bitmap.compare([record], reference, output, region=None)`.
`NeoLCD.lua` supplies the reusable RAM sampler/record writer for applet harnesses.
The comparator validates the record's three identical complete RAM/flag samples,
then reconstructs pixels. It writes actual/expected PNGs and a difference image
on mismatch; the CLI also writes a JSON report with bounds, pixel count and the
reference hash. References are never regenerated by verification. Historical PNG
pairs are supported for investigation; automated Neo tests use RAM records.

The Logo wrapper retains its independent geometry/reference consistency check and
separator-free shape statistics. Its square/turtle fixtures remain under
`fixtures/logo-graphics`, while pixel comparison goes through the generic tool.
Capture setup remains the calling harness's responsibility: wait for the expected
completed state and hide any cursor/turtle that should not be in the reference.

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

Every checkpoint requires three identical controller RAM/flag samples. Reports include
foreground bounds/counts and reference hashes. Actual/expected pane PNGs are saved;
a mismatch additionally saves a difference PNG and fails with the wrong-pixel
count. The tests never update reference images automatically. See the fixture
README for coordinates and reference authoring instructions.

Interrupt keys during running code are held for 30 emulated frames to span the
VM's periodic keyboard scans. Ordinary typing uses shorter taps. These checks
validate interrupt assignment and recovery, not minimum keypress duration or
physical keyboard latency.
