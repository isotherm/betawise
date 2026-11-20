# Logo right-pane references

These 132x64 monochrome PNGs are authored from declared geometry in
`../../logo_graphics.py`, never copied from emulator output. Coordinates are
relative to the right LCD controller; x=0 is the fixed vertical separator.
The drawing origin is (66,32), with Logo heading zero pointing up.

- `blank`: separator only.
- `square20`: four edges spanning x=66..86 and y=12..32, 80 black pixels
  excluding the separator. Every interior and exterior pixel must be white.
- `square10`: x=66..76, y=22..32, 40 edge pixels.
- `pen-line`: x=66..81, y=22; a pen-up move must leave no vertical trail.
- `home-line`: that horizontal line plus the diagonal (76,22)..(66,32).
- `north`: triangle vertices (69,32), (63,32), (66,22).
- `east`: vertices (66,35), (66,29), (76,32).
- `north-moved`: north triangle translated ten pixels up.
- `square20-north`: the north turtle XORed onto the square.

The turtle dimensions follow the documented LCD scaling of upstream's 6-pixel
half-base and 19-pixel nose by 0.55: round(3.3)=3 and round(10.45)=10.
Pillow draws each geometric edge, excluding its final endpoint; XOR models the
reversible turtle overlay. This checks the rasterizer independently of the
applet's integer line implementation. Cardinal headings avoid floating-point
trigonometry in the reference oracle.

Tests verify the stored PNG against its declared geometry and then compare every
right-pane pixel in two stable captures. They do not rewrite references. To
explicitly re-author them after reviewing a geometry change:

    python3 tests/mame/asneo/logo_graphics.py --write-references

A geometry/reference change is a test expectation change and needs review;
a failing emulator screenshot is not sufficient justification.
