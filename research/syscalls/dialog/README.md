# Dialogs and text input

## Scope

List-dialog builder, text entry, and slot 69 (`A114`) item-text retrieval.

Caller contracts remain in [`os3k_dialog.h`](../../../os3k/os3k_dialog.h). This page records
**evidence and confidence**, not a duplicate normative API reference. No new
reverse-engineering claims are inferred from the documentation reorganization.

## Findings and open questions

| Finding | Status and evidence |
|---|---|
| Singleton list storage, retained strings and item lengths | Historical builder probes and ROM layout. |
| Slot 69 returns item text in D0, table-entry address in A0 | Full entry disassembly; **production bridge** in `os3k_abi_bridge.c`. |
| Typed public pointer and output byte | Maintained integration probe checks two caller-owned pointers, lengths 3/7, including a length differing from strlen. |
| Invalid item index / NULL length output | ROM has no validation. Not certified by safe-call fixtures; callers must supply valid inputs. |

AS3000 3.01 item-pointer/length tables: 3E7E / 3F7E; Neo 3.21: 3F10 / 4010.
The original AlphaWord Plus callers use D0 directly; GCC needs the bridge.
The Neo 3.01 was not separately executed in this follow-up.

## Tests and reproduction

The maintained mixed-subsystem probe is stored once under [power/](../power/):
[battery-dialog-probe.c](../power/battery-dialog-probe.c),
[Lua template](../power/battery-dialog-probe.lua.in), and
[builder](../power/make-battery-dialog-probe.py). Its two valid item cases are
not a fresh exhaustive dialog audit. The common runner regenerates AS/Neo execution output.

The maintained combined runner, executed from the workspace root, is:

```sh
python3 betawise/research/abi-audit/run.py /tmp/os3k-syscall-validation
```

Only the cases explicitly described here are covered; running that command is
not blanket certification of this entire subsystem. The shared
[ROM inventory](../../abi-audit/rom-inventory.json),
[slot table](../../abi-audit/slots.tsv), and
[ABI audit](../../abi-audit/README.md) define firmware/ABI scope.

## Slot 157: printer selection

A274 is the printer selector, established by native localized strings and its
seven-entry printer table, not a language selector. AS3000 3.01 table 4E8FB4 has
40-byte entries: HP LaserJet/DeskJet/Lexmark, HP DeskWriter, Canon, Apple
ImageWriter, Apple StyleWriter, Citizen, Epson. The saved index is AS D74 /
Neo 3.21 DA0. Enter commits and Escape cancels; some selections require applet
A010. Physical Down/Enter changes index 0 to 1 in the maintained test.

Probe ownership and scope: [IrDA/services integration](../irda/README.md).

## Neo font-aware dialogs: slots 247–269 and 277

Fresh original Neo 3.21 Ghidra traces identify a separate font-aware dialog
engine, rather than additional calls to the older singleton character-cell
builder. All these slots are outside the AS3000 3.01 and Neo 3.01 dispatch tables.
Public declarations are in os3k_dialog.h, with FontDialog-prefixed names to
keep the two families distinct. No imported SDK names were used.

| Slot | Target | Clean-room function | Argument/result observations |
|---:|---|---|---|
| 247 | 433C18 | FontDialogUpdate | Object, signed region byte, four text pointers, indentation word; void. NULL object no-op. |
| 248 | 433DE0 | FontDialogSetTitle | Object, signed region byte, left-title pointer; wrapper calls 247 with indentation 55. |
| 249 | 433E0A | FontDialogSetRightTitle | Same shape for right-title pointer. |
| 250 | 433E34 | FontDialogSetBody | Same shape for body/item text; resets selection. |
| 251 | 433E5E | FontDialogSetInput | Object, region byte, input string; initializes copied 86-byte input state and redraws. |
| 252 | 433F60 | FontDialogSetTitleAnimation | Object, region byte, style byte, enabled byte. Styles 5–7 target the right-title rectangle; other styles target the left. |
| 253 | 433FE2 | FontDialogSetRowAnimation | Object, region, style, enabled, body-row byte; same animation storage as 252. |
| 254 | 434048 | FontDialogSetInputAnimation | Object, region, style, enabled; separate input animation storage. |
| 255 | 43409C | FontDialogSetProperty | Object, region, property byte 0–7, low-bit value; selector i writes flag bit 7-i. Invalid selectors do nothing. |
| 256 | 4343A2 | FontDialogHandleKey | Character/control word (printable bytes, literal tab, or supported Ctrl-style key words); void. Uses global dialog, focus and selected input. |
| 257 | 4341FE | FontDialogGetSelection | No arguments; low-byte selection from active region +111. |
| 258 | 43420A | FontDialogSetSelection | Selection byte, stored without checking. |
| 259 | 434218 | FontDialogClearItemAttributes | Mask byte; clears selected item's displayed-row attributes and resets visible-line selection. |
| 260 | 434230 | FontDialogSetRowAttributes | Mask byte ORed into active region +44 + selected-byte * 26. |
| 261 | 43425C | FontDialogDrawRegion | Object, signed region byte; redraws and can update shared selection/layout state. |
| 262 | 4342D4 | FontDialogDestroy | Object pointer, nullable; clears each associated input record, then count=0 and default-region byte=5. |
| 263 | 433B86 | FontDialogCreate | Template byte; pointer in D0. Public C bridge converts it to GCC A0. |
| 264 | 433DAE | FontDialogSetContent | Object, region byte, pointer to four text pointers; forwards to 247 with indentation zero. |
| 265 | 434348 | FontDialogActivate | Object and signed region byte; changes shared current object/region. Same object does not reselect; NULL directly selects a current-object region. |
| 266 | 4347D0 | FontDialogGetInput | **Ignored first argument**, output pointer; low-byte chosen region index. Uses global active/default region. |
| 267 | 43430E | FontDialogWrapText | Object, region byte, text pointer; native wrapper forwards the helper's low-word line count in D0, despite initially inferred void. |
| 268 | 43467C | FontDialogUpdateAnimations | Event byte; FF and active association UI suppress work. |
| 269 | 43490C | FontDialogTextWidth | Text pointer, byte-limit word; low-word current-font width. Slash markup skips the following byte unless it is another slash. |
| 277 | 434980 | FontDialogSetBitmap | Selector byte 1–8, eight-byte bitmap descriptor pointer, two word offsets truncated to unsigned bytes. Invalid selectors no-op. |

Slot 270 copies a selected wireless-record string and is documented with
[wireless](../wireless/README.md). Slots 271–276 are the separate
[markup scanner](../markup/README.md).

### Ownership and private object layout

The allocator scans twenty 1504-byte entries at 7E92. Each contains:

| Offset | Size | Established role |
|---:|---:|---|
| 0 | 2 | Region count; zero marks a free object. |
| 2 | 1 | Default input/focus region; 5 means automatic/current behavior. |
| 3 | 1 | Template selector. |
| 4 | 5 × 300 | Region storage. |

Most calls do not check region bounds. Objects are owned by this fixed ROM
pool, so the public type is opaque instead of inviting caller allocation or
ABI-dependent mutation. The complete private bytes are not all understood.
FontDialogDestroy does not invalidate global current-object/region pointers;
using those after destruction is unsafe.

Within each 300-byte region:

| Offset | Size | Established role |
|---:|---:|---|
| 000 | 26 | Left-title drawing rectangle, text pointer at +8. |
| 01A | 26 | Right-title drawing rectangle, text pointer at +8. |
| 034 | 6 × 26 | Body-row drawing rectangles; text at row+8, drawing attributes at row+16. |
| 0D0 | 26 | Input rectangle; editor pointer at +8, input renderer kind at +12. |
| 0EA | 26 | Scroll-indicator rectangle. |
| 104 | 4 | Borrowed body/item text pointer. |
| 108 | 4 | Other retained template/internal storage; not assigned a semantic public name. |
| 10C | 1 | Retained template byte. |
| 10D | 1 | Region flags. |
| 10E | 1 | Body item/line extent calculated while laying out text. |
| 10F | 1 | Template line-height factor. |
| 110 | 1 | Visible selected-row state. |
| 111 | 1 | Body item/line selection byte. |
| 112 | 12 | Input-animation record. |
| 11E | 12 | Title/body-row animation record. |
| 12A | 1 | Template border/divider mode. |
| 12B | 1 | Trailing alignment/reserved byte. |

Each 26-byte drawing rectangle begins with word y, x, width, height, then a
32-bit borrowed text pointer. Renderer kind is at 12, drawing x at 14,
attribute byte at 16, alignment policy at 17, and an animation-record pointer
at 22. Bytes 13 and 18–21 are not promoted to guessed public members.
Input state is shared separately at F43E + region * 86: a protected-prefix
length word, a word used by input/choice handling, an 81-byte text area at 4,
and current-length byte at 85. Strings used as titles/body stay borrowed;
input initializer copies its text. Keep initial input below eighty bytes:
its legacy boundary behavior was not made safe by the syscall wrapper.

Current dialog, region and focused-region byte live at F412/F416/F41A.
The original emulator incorrectly mapped these low addresses to VZ MMIO,
which caused a real address fault during activation. The corrective emulator
work is recorded in asneo-emu/docs/R17_LOW_RAM_MMIO_20261118.md; it changes
address decoding, not firmware behavior.

### Flag masks and text formatting

The property selectors reverse the bit order: 0=80 title, 1=40 input handling,
2=20 populated-input underline, 3=10 body, 4=08 body-overflow state,
5=04 selected-item highlighting, 6=02 single-visible-line selection,
7=01 unconditional underline. Drawing recomputes bit 08. It is therefore not
an independent immutable focus-enable flag. Title rendering, input processing,
body layout and selection paths were cross-checked at 441BAA/4343A2/4346C8.

The word at 7E8C is indentation used by caret markup, not a font mask: 432A40
adds it to text drawing x when caret formatting is active. Slot 247 accepts
it explicitly, 248–250/251 use 55, and 264 uses zero. Slash escapes formatting;
`//` measures as one slash, while other slash-prefixed control bytes are
excluded by 269. A trailing incomplete slash escape is not a safe general
input contract.

### Template and animation confidence

The ROM accepts templates 5–9 and 11–21; 10 and other values fail. Template
8/9 share base descriptors, while stored IDs can still select different
navigation behavior. The maintained fixtures record each template's region
count and default focus. Public enum names deliberately retain their numeric
IDs, with geometry descriptions, instead of inventing manufacturer UI names.
Styles 1/2 draw five-/two-pixel blinking carets. Style 4 animates a
12×8 radio-signal bitmap. Decoding the actual packed ROM images establishes
style 5 as combined up/down arrows (7×9), 6 as an up arrow (7×4), and 7 as a
down arrow (7×4). These have descriptive public enum names. Style 3 updates
native animation state but has no dedicated drawing branch in 433896; its
intended visible role remains unresolved and its enum retains the numeric ID.
The bitmap-decoding script below reproduces the shape evidence.

### Inline bitmap registration

Slot 277 is **not a font-registration service**. Its descriptor is two word
pixel dimensions followed by a 32-bit packed bitmap pointer. The registration
array contains word x/y offsets and a borrowed descriptor pointer, eight bytes
per slot. 432FB2 reads those fields and draws the bitmap; 43294E accounts for
its width plus two pixels when text begins with selector 1–8. The drawing
helper treats selector 8 specially and uses a built-in bitmap instead of the
registered slot; only selectors 1–7 consistently select custom images. Wireless File
Transfer and Responder supply matching descriptors. Keep both descriptor and
pixel data alive; registering NULL clears a slot.

### Reproduction and manufacturer callers

```sh
python3 betawise/research/syscalls/dialog/run-neo-probe.py /path/on/disk/dialog-probe
python3 betawise/research/syscalls/dialog/inspect-bitmaps.py
```

MathFacts RP at file offset E28 calls slot 267 and immediately consumes D0.w
with a comparison against one (E30/E32). This independently establishes the
line-count return rather than treating the wrapper as void.

The shared [tail inspector](../wireless/InspectNeoTail.java) covers every entry;
additional helper targets include initialization 433AFC, 4337B0, 43385A, 434298, 434852, 441A4A,
441BAA, 44254E, 44259A, 442634, 442A1A and 442A98. The
[caller scanner](../wireless/find-vendor-calls.py) resolves MathFacts,
Responder, Wireless File Transfer, Accelerated Reader and KeyWords Wireless
uses. The opaque object choice and private layout above come from original
constructors and consumers, not a guessed SDK structure.

### Completed live verification (2026-11-18)

The maintained Neo runner passes **126 cases** after the low-RAM/MMIO decode
correction. It executes original ROM targets, with no replacement firmware
handlers. Coverage includes all sixteen accepted template IDs, pool release
and twenty-object exhaustion, all eight property selectors in both directions,
borrowed content pointers versus copied input, protected-prompt editing and
extraction with boundary sentinels, selection and row attributes, animation
records, text measurement and wrapping, bitmap registration boundaries, and
all public declarations through a compiled C probe. The constructor's D0-to-A0
bridge is exercised by that compiled probe. Slot 270's cached-string copy is
also tested here because it shares the font-aware UI group in the ROM.

For slot 267, `first\rsecond` returns one: carriage return terminates the
paragraph. A longer paragraph returns multiple wrapped lines. Neither case
replaces the stored body-row pointer. The first draft of the fixture incorrectly
expected pointer replacement and then counted across the carriage return;
those expectations were corrected from observed behavior and the native
442634 loop. Long test input now has separate storage so it cannot overwrite
borrowed title/body strings. A native redraw snapshot was inspected after
editing; the image is regenerable and is not checked in.

This is directed contract coverage, not exhaustive navigation or animation
coverage for every template. Animation style 3 remains unnamed; private object
bytes not listed above remain unresolved. Actual radio hardware is outside the
scope of these dialog tests. Doxygen syntax checks and XML parameter/return
coverage checks pass for all 41 public APIs in the 237–277 follow-up; generated
documentation is discarded.
