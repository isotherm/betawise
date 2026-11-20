# Screen bitmap fixtures

A fixture is an expected screen image used as a test input. The reusable verifier
is [`screen_bitmap.py`](../screen_bitmap.py); this directory holds the reviewed
images and their provenance. Each applet can have its own subdirectory.

## Creating a fixture for any applet

1. Specify the completed state being tested and the full screen or region
   `(x, y, width, height)` to check. Neo screens are 264x64 pixels.
2. Author a PNG with exactly the selected region's dimensions, using pure black
   for lit pixels and pure white for the background. Include all expected pixels;
   the verifier checks both foreground and background, without tolerances.
3. Document the commands/setup, coordinates, expected result and reference source
   alongside the image. Prefer independently specified geometry or layout. A
   reviewed known-good capture can also be a reference, but record its provenance
   and why it is correct; do not turn a failing capture into its own expectation.
4. Capture three identical LCD RAM/flag samples with `NeoLCD.lua`, after the
   applet reaches its expected state. Hide cursors or turtles through the applet
   or firmware API if they should not appear in the expected image.
5. Compare the record with the reference using the generic tool:

   ```bash
   # From the betawise repository root:
   python3 tests/mame/asneo/screen_bitmap.py /tmp/run/checkpoint-01.lcd.json \
     --reference tests/mame/asneo/fixtures/my-applet/expected.png \
     --region 12 8 120 40 --output /tmp/bitmap-check
   ```

Omit `--region` for a full-screen fixture. The Python API is
`screen_bitmap.compare([record], reference, output, region=None)`.

Verification produces actual/expected images, a difference image on mismatch,
and (through the CLI) a JSON report. It never updates reference images.
Reference changes change the test's expectations and should be reviewed with
the behavior change they describe. Fixture authoring is separate from validation.

## Existing fixtures

- [`logo-graphics/`](logo-graphics/README.md): independent square, line and turtle
  references for Logo's right pane. Its README explains those specific shapes
  and their coordinates; it is not a requirement for other applets.
- `backspace-clean.png` and `backspace-cursor.png`: historical terminal captures
  used to test the glyph reader's acceptance/rejection behavior. These are reader
  test inputs, rather than reference screens for the generic bitmap verifier.
