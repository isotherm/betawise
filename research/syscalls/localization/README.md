# Localization, character classes, and font correlation

## Scope

Slots 88–92: lowercase bytes, key conversion, matrix layouts, bitmap predicate, and layout dialog.

Caller contracts remain in
[`os3k_localization.h`](../../../os3k/os3k_localization.h). This page records
**evidence and confidence**, not a duplicate normative API reference. No new
reverse-engineering claims are inferred from the documentation reorganization.

## Findings and open questions

| Finding | Status and evidence |
|---|---|
| Lowercase byte mapping | All 256 inputs checked on AS3000 3.01/Neo 3.21. Neo differs at the A6/A7 upper-table entry. Raw byte mapping is recorded below. |
| Four matrix layouts and description output | Typed one-byte enum plus adjacent sentinels; 256 matrix inputs per layout; native choices 1–4 and Escape. |
| Matrix holes | Alternative layouts index with a base-map FF; the observed outputs are not valid key contracts. |
| #91 exact behavior | Identical fixed 32-byte bitmaps, 183 true / 73 false, all inputs checked. Full bitmap is recorded below. |
| #91 purpose | **Unresolved.** Thirty validated exports among 54 manufacturer images, no resolved call candidates. Static scanning does not exclude unknown dynamic calls. |
| #91 font interpretation | Not current-font availability: battery codes and many normal Neo glyphs are rejected. Legacy text-repertoire/compatibility filtering is an **inference**, not a recovered consumer contract. |

[Charset comparison](charset-comparison.png) shows selected bytes. Both the
default Neo ROM font and the user's NeoFontTerminal bitmap include rejected
DD/DE/FD/FE glyphs; D0/D7 also have AS controller glyphs in the emulator asset.
AS CGROM is marked NEEDS REDUMP, so its shapes are not physical certification.
The terminal font is a 2020 Alpaxo custom font, not a manufacturer ROM font.

## Tests and reproduction

Maintained sources: [localization-probe.c](localization-probe.c),
[Lua template](localization-probe.lua.in), [builder](make-localization-probe.py).
Reference search: [script](investigate-character-bitmap.py) and
the summary above (rerun the script for per-image output).

From the workspace root:

```sh
python3 betawise/research/syscalls/localization/investigate-character-bitmap.py
```

The script checks source hashes against the shared inventories. It does not
prove there are no dynamically dispatched callers.

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

Syscall 0xA160. Converts ASCII A–Z and the following ordered byte pairs;
all other bytes are returned unchanged. The table below is AS3000 3.01; the tested
Neo substitutes A7 for A6 in the upper row, leaving both A6 and A7 unchanged.
These are the actual ROM mappings,
including unusual pairs; do not substitute a host locale or Unicode mapping.
```text
upper: 8E 80 81 82 87 88 89 90 91 92 93 98 99 A6 C9 CB CE C4 83 02 06 07
lower: 03 84 85 86 8B 8C 8D 94 95 96 97 9A 9E A7 C0 CD CF 08 01 8E 9D 9C
```
In particular, 02 becomes 8E and a separate call on 8E becomes 03, so this
function is not idempotent for every firmware byte. No string is modified.

Syscall 0xA16C. This fixed 256-bit classification is independent of the active
layout and does not inspect a font descriptor. The tested AS3000 3.01 and July
Neo 3.21 use byte-identical bitmaps and reject the same 73 byte values.
It includes ASCII 0x20–0x7E and several control/extended bytes, and
excludes some bytes handled by ToLowerTextByte(). It is not C isprint(),
isalpha(), or a Unicode-validity test. The bitmap's broader policy meaning
has not been established from a consumer; the name denotes its ROM group.
A follow-up scan found 30 vendor images exporting this syscall but no resolved
call references. Every accepted byte has an AS3000 3.01 LCD translation, but 41
rejected bytes also do, including the battery icon bytes 91–94 drawn by
DrawBatteryStatus(). This is consistent with an ordinary-text whitelist,
rather than a complete display-capability query; that purpose is an inference.
The default Neo ROM font and NeoFontTerminal also have distinct glyphs for
rejected bytes such as DD, DE, FD and FE. A legacy AS3000 3.01 text-repertoire or
compatibility filter carried into Neo remains possible, but no caller has
established that purpose. It does not report the active Neo font's coverage.
The complete bitmap, as sixteen big-endian words indexed by the input's
high nibble, is:
```text
2100 F002 FFFF FFFF FFFF FFFF FFFF 7FFF
8009 8200 4BBE B873 FFFF 9F7E FFFF 9FFE
```
Bit (character & 15) is tested within that word. For example 00 is false,
08 and 0D are true, 7F and 8E are false, and FF is true.

Both output pointers are optional. The description is overwritten with the
ROM's localized "Layout:" label, a space, and the short arrangement name.
In the tested English ROMs the longest result is "Layout: QWERTY" (14 bytes
including NUL). The ROM has no capacity parameter: supply ample storage for
the installed language resources, not merely the English length.
