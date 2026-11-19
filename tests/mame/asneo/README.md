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
