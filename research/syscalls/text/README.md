# Native text editor, scanning, statistics, and forms

## Scope and reproduction

Slots 175–194 (A2BC–A308), declared in
[`os3k_text.h`](../../../os3k/os3k_text.h) and included by os3k.h.
This follow-up reconstructs the interfaces from original ROM instructions,
manufacturer caller patterns, and execution, rather than SDK declarations.
Ghidra's guessed pointer/byte types were not adopted without checking stack
loads, result stores, and callers. Several decompilations miss jump-table arms
or misinterpret numeric status/count results as pointers.

From the workspace root:

```sh
python3 betawise/research/syscalls/text/run-probe.py /tmp/os3k-text-validation
python3 betawise/research/syscalls/text/find-vendor-calls.py
```

[run-probe.py](run-probe.py) checks the shared firmware hashes, compiles
[typed-probe.c](typed-probe.c), links current syscall stubs, and expands
[probe.lua.in](probe.lua.in). Results, binaries, linker scripts and fresh emulator
NVRAM go only into the supplied output directory. No generated logs are kept here.
MAME executables and the original images listed in the
[ROM inventory](../../abi-audit/rom-inventory.json) are required.

[InspectText.java](InspectText.java) prints each dispatch entry's instructions
and decompilation from an existing correctly based big-endian 68000 ROM program:

```sh
/opt/ghidra/support/analyzeHeadless /tmp/your-projects project-name \
  -process program-name -noanalysis -readOnly \
  -scriptPath betawise/research/syscalls/text \
  -postScript InspectText.java as3000
```

Use neo_3_21 or neo_3_01 for the corresponding inventoried program. AS3000 3.01 and
Neo 3.21 were inspected and executed in this follow-up. Neo 3.01
addresses are recorded, but its behavior was not separately executed/certified.
The original investigation also decompiled the internal token walkers, gap
manager, insertion/deletion helpers, and file resolver/checker.

## Verification boundary

Final suite on 2026-11-18: **75 cases on each of AS3000 3.01 and Neo 3.21**, including
two compiled-C cases with 28 individually numbered assertions. The C fixture
calls every newly exposed entry and checks native struct sizes/offsets,
argument ordering, enum/Boolean stack arguments, the eight-byte floating output,
status outputs, and state transitions. Raw calls seed D0/A0 and check SP and
completion before checking results. Returning an incidental address in D0 does
not make an entry a public pointer-return function; no additional pointer bridge
is needed for this group.

The fixture creates a consistent two-record RAM filesystem with **separate**
normal-file and clipboard buffers. It seeds the real allocator/checker bounds
and executes the original file resolver, gap manager, renderer, and form code.
Main entry/buffer are AS 1042/4558, Neo 10D2/1509C. The following entry is the
clipboard; each buffer has 65536 bytes. This proves operations on these controlled
records, not applet installation, allocator stress, flash persistence, or every
possible existing device file layout. No text routine is substituted by a stub.

An early fixture incorrectly aliased clipboard and form storage. Native form
editing then overwrote the apparent form: that was a fixture error, not evidence
of a firmware defect. Only the separated-record fixture supports the final claims.

These are direct native API/message tests in a running emulator, not a complete
physical-key AlphaWord regression. End-to-end USB/cable/IrDA sending and printer
interoperability are not certified by transfer-state opening/cancellation tests.

| Slots | Evidence and maintained cases |
|---|---|
| 175 | Read-only/deferred initialization, geometry, file-flag requirement, invalid rectangle, real editable initialization and 512-byte gap; typed C. |
| 176 | Idle/unhandled/current-file guard, ordinary character insertion, Shift+Right selection and status output; typed C. |
| 177 | Native reopen/consistency path after editing; typed C. External arbitrary file corruption/recovery is not certified. |
| 178 | Start/viewport/caret origins, invalid-origin partial reset; typed C and reserved-byte sentinels. |
| 179 | Raw/plain copies, reverse plain traversal, hidden/protected filtering, case-sensitive/insensitive searches, empty-gap scan, empty-form expansion; typed C. Complex expression dialect and all malformed tokens remain outside coverage. |
| 180–182 | Read-only/current-file errors; real forward deletion, plain insertion, unchanged scan offset, caret placement and native layout; typed C. |
| 183 | AS unimplemented return; Neo read-only rejection, preserve-all-null request, and replacement of all four pointers with valid cloned descriptors followed by layout. |
| 184 | Ensures gap space and saves viewport as current file position. Gap remains present; exact resulting ordinary content checked. |
| 185–186 | 124-byte initialization, retained editor pointer, unknown-message clearing, full 60001 open/name copy, reply 17, 60020 Escape/cancel reply 8; typed C. |
| 187 | Six full-text examples and a bounded subrange, all counters, optional form output, exact double bytes and adjacent sentinel; typed C. |
| 188–189 | Invalid plain text, empty form, BODY/INPUT examples, canonical generated FORMDATA, reparse without reset preserving scroll, native control edit to Q, unhandled messages, focus/count fields; typed C. Not all input/control kinds. |
| 190–191 | 124-byte form transfer initialization; unhandled/open/name handling and cancel from an injected valid phase; typed C. No claim of complete transport progression. |
| 192 | Select main file explicitly, remove edited generated values, regenerate defaults. Native caller/disassembly support a void interface. |
| 193 | Case-insensitive rendered-text match and exhaustion; matching scroll row checked, typed C. Empty needle excluded. |
| 194 | Clear leaves 512 raw gap bytes, then plain scan yields zero. Typed C; retained-form behavior is traced rather than exhaustively exercised. |

## Layout and editor mechanics

The native structures are 94-byte editor, 44-byte scan, 124-byte transfer state,
and 74-byte form state. The new names avoid the existing two-byte display
TextPosition_t. Word-aligned 32-bit fields are intentional: e.g. editor maximum
length at 02, form offsets at 02, and scan conversion pointer at 26.

Editor raw caret and gap boundaries are 24/28/2C. Selection drawing state is
36, anchor offset 38, saved anchor flags 34, and caret display coordinates
3C/3E. The remaining opaque words are preserved rather than assigned speculative
public policies. Transfer state embeds a complete scan record at 50. Native form
state contains raw form/data boundaries, top-row word 1A, active tag offset 26,
control count 2A and focused index 2C. Native fields not fully mapped remain
opaque in the public types; no claim of a complete control-state schema is made.

Editable initialization creates a gap, ordinarily 512 A7 bytes, within the raw
file. In the tested sequence `Hello world.\rNext line` becomes 534 stored bytes.
Scanning five plain bytes reaches raw offset 517. Deleting the following space
and inserting XYZ produces `HelloXYZworld.\rNext line`, still with the gap.
184 saves viewport position; it does not compact the raw file. 194 clears ordinary
content but leaves 512 gap bytes, so FileQuerySize() and plain scan length differ.

AS coordinates are zero-based text cells. Neo accepts pixel bounds, but also
applies compatibility heuristics: full-screen 0,0,40,4 becomes 264 by 66; smaller
legacy sums can scale horizontal coordinates by 6 and vertical by 16. The field
at 14 is a native vertical extent (full-screen test: 4 on AS, 64 on Neo), not a
portable count of lines. Four Neo font pointers occupy 4E/52/56/5A. Slot 183
replaces nonnull pointers, recalculates that extent and lays out; AS returns
02000003 without reading those parameters.

Flags 1 and 2 select read-only and deferred initialization. Neo also copies
input flags 4/8/10 into private state bits at 0D; their broader rendering policies
have not been established, so speculative enum names are not exported. Read-only
or deferred initialization demands file flag 04. That is evidence of a consumer
of this file bit, not proof of its entire filesystem policy.

## Scanner conversion and editing

179 has ten actual stack arguments. Return is a 32-bit count, or 0/1 with a
needle; Ghidra's byte-pointer inference is not a pointer contract. Its private
token walkers update scan flags: B1 sets region bits and B2 clears them.
Flags 01/02 are filtered independently by the two skip arguments. The maintained
cases show ordinary `ahb` becoming `ab` when the corresponding region is skipped.
The hidden bit also suppresses ordinary layout-coordinate updates; the protected
bit is used by edit helpers. These are text-stream flags, distinct from FileInfo_t
flags and from the applet disable masks.

Plain mode strips token framing, maps ordinary text equivalents, and unwraps
B0-character-B0 escapes. Raw mode counts stored bytes and stops at token
boundaries. Neither mode appends NUL. Reverse traversal emits bytes in reverse
traversal order (`fed` from the end of `abcdef`). Search forces plain conversion
and disables form expansion; its success rewinds the raw scan offset to the
match. ASCII case folding is tested; the full firmware byte-fold table is shared
with the previously documented localization behavior.

The expand-forms argument is not an inline insertion at the form marker. For
`a A8 <FORM></FORM> terminator b`, normal plain mode yields `ab`; expansion yields
`ab<FORM></FORM>` after the ordinary scan. The terminator is A9 on AS and 9D on
Neo. Populated/nested expressions and localized conversion envelopes are not
exhaustively exercised; do not infer full HTML serialization from this example.

180 copies the scan state before determining its deletion end. 181 inserts
without advancing the external scan; its encoded source can have boundary
region bits rewritten, so that source is writable. Plain insertion escapes
reserved bytes. 182 then applies a scan position to the actual editor caret,
clamping editable positions away from the internal gap and relaying out.

## Statistics: eleven parameters and an eight-byte output

187 is an output-parameter operation, not a count-return or pointer-return call.
It takes file/start/end followed by eight optional outputs: ordinary characters,
separators, separator-terminated words, paragraphs, CRs, sentences, an eight-byte
IEEE double quantity, and evaluated form characters. The double is ordinary
character count / 2048. Original AlphaWord has page-count UI strings, but the
public name states the confirmed arithmetic rather than assuming a physical page
layout. For three characters, the exact output is `3F580000 00000000`.

| Input | Characters | Separators | Words | Paragraphs | CRs | Sentences |
|---|---:|---:|---:|---:|---:|---:|
| abc | 3 | 0 | 0 | 1 | 0 | 0 |
| abc followed by space | 4 | 1 | 1 | 1 | 0 | 0 |
| one two three. followed by space | 15 | 3 | 3 | 1 | 0 | 2 |
| one CR two CR | 8 | 2 | 2 | 3 | 2 | 0 |
| empty | 0 | 0 | 0 | 0 | 0 | 0 |

A pending sentence boundary can be counted again at the end; this is the actual
firmware output, not a modern linguistic algorithm. Supplying the final form-count
pointer enables extra form evaluation, separate from ordinary character counting.
Complex form-count arithmetic is traced but not exhaustively certified. An
invalid file does not initialize all optional outputs; callers need a valid file.

## Forms and transfer callbacks

188 parses the firmware form dialect, initializes focus, normalizes newlines,
and may append generated values. An INPUT with NAME a and VALUE hello becomes:

```text
<FORM><INPUT TYPE="TEXT" NAME="a" VALUE="hello"></FORM><FORMDATA>hello               </FORMDATA>
```

In that example the descriptor has form end 96, FORMDATA tag/content/end offsets
55/65/85, one control, active index zero. Native MSG_CHAR Q replaces the field's
selected initial text and writes Q plus padding to FORMDATA. The form editor uses
the separate clipboard record while doing this. Select the actual form file
before calling 192; it uses current-file length operations. Reset discards edited
FORMDATA and rebuilds the original hello default. Rendered search is byte-wise
case-insensitive and updates top_row, not a raw text selection.

186 and 191 dispatch by the low message byte but also inspect full prefixes on
specific arms. Therefore their message parameter is full Message_e, not the byte
suggested by incomplete Ghidra output. On 60001 both copy a name capped at 39
bytes into transfer state and enter phase 6; editor reply is 17, form reply is 0.
For cancellation, the editor checks the full 60020 code, while the form key arm
requires phase 7. Escape moves to phase 8 and reply 8. The test injects phase 7
for that form arm; it does not simulate a completed native transfer to get there.

A native request block's leading words are length and data pointer; key messages
point that data pointer at a 16-bit key. The remaining message-specific payload
has not been fully reconstructed, so handlers take void* rather than a guessed
public request structure. Unknown message tests show reply zero and unhandled
return. Production ABI is normal stack arguments; no callback A5 trampoline is
provided by these synchronous handler calls.

192 ends by calling the form initializer, leaving incidental D0. All four
identified AS AlphaWord callers (CAE, 9832, D0FA, D318) ignore that result. Its
public return is void rather than elevating an incidental helper result into an
error-reporting guarantee. 194 explicitly returns zero even when some internal
operations fail; it is not a reliable status propagator.

## Manufacturer caller search

[find-vendor-calls.py](find-vendor-calls.py) checks original image hashes and
validated contiguous exports. Thirty images expose the block; the restricted
static search found 521 call/address candidates. Adjacency checks avoid the stale
register false positives of a looser scan, but candidates still require review.
Unknown dynamic patterns are not excluded by zero matches.

AlphaWord uses editor/form transfer initializers and handlers, form reset, the
scanner editing calls, and statistics. Beamer uses read-only editor initialization
and scanner calls. KeyWords uses statistics. These are corroborating original
caller patterns, not imported SDK prototypes. Confirmed AS AlphaWord reset
call sites were inspected for return usage as described above.

## Dispatch targets

| Slot / opcode | Public name | AS3000 3.01 | Neo 3.01 | Neo 3.21 |
|---|---|---|---|---|
| 175 / A2BC | TextEditorInit | 004D4548 | 005D6746 | 00427E04 |
| 176 / A2C0 | TextEditorHandleMessage | 004D4E62 | 005D712C | 0042882C |
| 177 / A2C4 | TextEditorRefresh | 004D49C0 | 005D6C8A | 00428366 |
| 178 / A2C8 | TextScanInit | 004D540A | 005D76CA | 00429102 |
| 179 / A2CC | TextScanRead | 004D54A8 | 005D7786 | 004291BE |
| 180 / A2D0 | TextScanDelete | 004D5A00 | 005D7CE6 | 0042971E |
| 181 / A2D4 | TextScanInsert | 004D5A9C | 005D7D82 | 004297BA |
| 182 / A2D8 | TextEditorSetScanPosition | 004D5AFA | 005D7DE0 | 00429818 |
| 183 / A2DC | TextEditorSetFonts | 004D5B90 | 005D7E76 | 004298AE |
| 184 / A2E0 | TextEditorSavePosition | 004D4972 | 005D6C3C | 0042830E |
| 185 / A2E4 | TextEditorTransferInit | 004D4BD8 | 005D6EA2 | 00428598 |
| 186 / A2E8 | TextEditorTransferHandleMessage | 004D4C0E | 005D6ED8 | 004285CE |
| 187 / A2EC | TextGetStatistics | 004D5B98 | 005D7F06 | 0042993E |
| 188 / A2F0 | TextFormInit | 004D9EA8 | 005DC5DC | 0042ECDC |
| 189 / A2F4 | TextFormHandleMessage | 004DA3A2 | 005DCAE2 | 0042F200 |
| 190 / A2F8 | TextFormTransferInit | 004DA728 | 005DCEAC | 0042F6A8 |
| 191 / A2FC | TextFormTransferHandleMessage | 004DA7C4 | 005DCF48 | 0042F744 |
| 192 / A300 | TextFormReset | 004DA328 | 005DCA68 | 0042F186 |
| 193 / A304 | TextFormFind | 004DAA18 | 005DD19C | 0042F998 |
| 194 / A308 | TextEditorClear | 004D4A6C | 005D6D36 | 0042842C |
