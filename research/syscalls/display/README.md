# Display, cursor, graphics, and font state

## Scope

Slots 0–19, especially A028–A03C, and battery UI slot 78.

Caller contracts remain in [`os3k_display.h`](../../../os3k/os3k_display.h). This page records
**evidence and confidence**, not a duplicate normative API reference. No new
reverse-engineering claims are inferred from the documentation reorganization.

## Findings and open questions

| Finding | Status and evidence |
|---|---|
| Save/restore ownership and shadow versus pixel state | ROM traces and historical AS3000 3.01/Neo MAME probes. |
| Text-span fixed 40-column traversal and cursor side effects | Controlled shadow/position fixtures; retain the bounds requirements in API documentation. |
| AS3000 3.01 A048 no-op and A04C absent | ROM dispatch/body comparison; do not infer Neo support on an old ROM. |
| Neo save/restore shifted capture | **Historical emulator limitation**, observed with the earlier LCD model lacking dummy-read behavior. The model was changed in another investigation; this is not a firmware defect or a claim about the current binary. |
| Battery icon/report | Maintained battery integration probe checks LCD DDRAM/CGRAM or bitmap RAM, timed return, and native Return dismissal. |
| Direct cursor RAM address | Betawise's direct reference is Neo-version-specific. No portable getter has been established; keep the implementation TODO in context. |

Do not generalize pixel tests to physical LCD timing or every installed font.
The #91 bitmap is not active-font coverage; see [localization](../localization/README.md).

## Tests and reproduction

No maintained comprehensive graphics, packing, or shadow-buffer fixture is supplied. Earlier emulator and display observations retain their stated historical scope.

The maintained combined runner, executed from the workspace root, is:

```sh
python3 betawise/research/abi-audit/run.py /tmp/os3k-syscall-validation
```

Only the cases explicitly described here are covered; running that command is
not blanket certification of this entire subsystem. The shared
[ROM inventory](../../abi-audit/rom-inventory.json),
[slot table](../../abi-audit/slots.tsv), and
[ABI audit](../../abi-audit/README.md) define firmware/ABI scope.

## Additional investigation details

These observations retain their original firmware and test scope; tentative
interpretations are not additional API guarantees.

The ABI stores these two bytes in row, column order. Span traversal wraps
after column 40, including in the tested Neo ROM.

System 3 syscall `0xA028`. AS3000 3.01 captures all 160 LCD character bytes and
the ROM cursor row, column, and visibility into the text shadow. Neo
captures LCD pixels, its live text grid, logical and pixel cursor positions,
visibility, and font pointer. A later save replaces the earlier snapshot;
ROM dialogs can use the same storage. Betawise's private display state is
outside this snapshot.
@note The tested Neo MAME LCD model omits the dummy-read pipeline. Its
captured pixel image is shifted by one column within each controller;
this is an emulation limitation observed during live validation.

In normal mode, shadow byte 0xFF is drawn as a space. AS3000 3.01 passes other
bytes through its normal character translation. Highlight mode draws solid
character cells on AS3000 3.01, using a distinct glyph at cursor_position when
that cell is inside the span. Neo instead draws inverse versions of the
shadow characters. The tested Neo renderer uses the ROM system font.

System 3 syscall `0xA048`. This call is a no-op in the tested AS3000 3.01 ROM.
The Neo source contains w bytes per eight-pixel-high band. Bit 0 is the
top pixel of a band. The next byte is the next column. A second band
starts after w bytes.

System 3 syscall `0xA04C`. The tested AS3000 3.01 ROM has an empty dispatch slot.
Bitmap packing matches DrawBitmap(). The Neo ROM offsets y by its screen
roll and rejects rectangles extending beyond width 264 or height 66.

## Native editor rendering follow-up

The [text investigation](../text/README.md) covers native editor/form geometry and the Neo-only font replacement at slot 183. Its 94-byte editor and 44-byte scan structures are separate from the existing two-byte TextPosition_t display coordinate type.

## Vendor RasterOp callers and applet header flags

A scan of the 54 distinct original images in the shared vendor inventory found
14 images with the validated `A048 A04C A06C` export sequence. Exporting a stub
alone does not establish that an applet calls it. Resolved call candidates occur
in these four images; the first call in each was independently disassembled and
its indexed-PC destination checked against the A04C stub:

| Applet | Header flags at 0x10 | Example call → A04C stub (file offsets) | Resolved reference candidates |
|---|---|---|---|
| MathFacts in a Flash RP | `0xFF00010E` | `0x9BE → 0x122DA` | 19 |
| MathFacts in a Flash Offline | `0xFF00010E` | `0x5284 → 0xA61E` | 21 |
| Wireless File Transfer | `0xFF000113` | `0x28F6 → 0xB23C` | 4 |
| KeyWords Wireless | `0xFF000100` | `0x15E40 → 0x16D4C` | 6 |

All four set flag `0x0100`, currently exposed as
`APPLET_FLAG_WIRELESS_REQUIRED`. This correlation does **not** identify a
RasterOp-specific flag or a general extended-syscall availability contract.
Responder, Accelerated Reader, and Wireless Updater also set that bit without a
resolved direct RasterOp call in this scan. Indirect calls and unsupported
instruction patterns remain possible.

All four have zero at header offset `0x7C` (the minimum AlphaSmart Manager
version field), and words `0, 1, 2` at `0x88..0x93`. Those values also occur in
noncallers and do not establish a minimum firmware version.

Fresh Neo 3.21 Ghidra disassembly/decompilation confirmed lower-header-word bit
8 checks at `0x41843C` and `0x4184C4`. These checks skip applet-switch paths when
the bit is set; they do not themselves test the RasterOp dispatch entry or ROM
version. The full installation/launch meaning of this flag has not been
re-established by this investigation. No new live-MAME flag mutation test was
performed for this question.

Reproduce the hash-checked scan from the workspace root:

```sh
python3 betawise/research/syscalls/display/find-rasterop-callers.py
```

The scanner recognizes short/word PC-relative calls, absolute calls, address
references, and adjacent immediate-delta indexed-PC references. It reports
candidates rather than treating arbitrary A04C words as calls. Original files
are read without modification. RasterOp's dispatch slot is zero in the inspected
AS3000 3.01 and Neo 3.01 images, and populated in the Neo 3.21 image; the shared ROM
inventory identifies those exact images.
