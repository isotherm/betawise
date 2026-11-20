# Issue #15 contract audit — 2026-11-20

Audited [issue #15](https://github.com/isotherm/betawise/issues/15) and all 38
linked closure documents against the current public headers/stubs, original ROM
instructions, and execution of original firmware in MAME. The issue was fetched
on 2026-11-20, including its stated static-only coverage. The conclusions below
are independently checked; a static regression count is not evidence that a
reconstructed contract is correct.

## Scope and reproduction

Static inspection covers the hash-verified AS3000 3.01, Neo 3.01, and Neo 3.21
images in [rom-inventory.json](../../abi-audit/rom-inventory.json). Live execution
covers **AS3000 3.01 and Neo 3.21**. Neo 3.01 remains a static comparison;
no physical device or exhaustive branch certification is claimed.

From the AlphaSmart workspace root:

```sh
python3 betawise/research/syscalls/file/audit-contracts.py /tmp/issue15-static
python3 betawise/research/syscalls/coverage/run-core.py /tmp/issue15-file --group file
python3 betawise/research/syscalls/coverage/run-core.py /tmp/issue15-applet --group applet
```

The static runner emits private disassembly, binary spans and a hash/address
manifest for all 38 entries. Vector-to-next-target spans may include private
helpers; they are extraction bounds, not assertions of one-function size.
Discriminating instruction anchors cover the resolver, selector comparisons and
narrow returns. These assertions supplement the data-flow inspection rather
than replacing it. Original firmware/disassembly is not committed.

The live runner uses fresh NVRAM/config and controlled disposable RAM records.
It executes A-line stubs, validates return PC/SP within bounded frame deadlines,
and checks explicit register/memory results. Five poisoned-register cases enter
the original ROM handlers directly: the A-line dispatcher itself uses D0 as
scratch, so poisoning D0 before dispatch would not isolate the callee's return
width. Ordinary A-line cases remain independently exercised. Compiled C uses the
current production stubs and headers with the repository ABI flags and
`-fno-builtin`; `contract.dis` retains the generated masking instructions.

For file/applet cases, `case-NNN-before.ram` and `case-NNN-after.ram` contain,
in order, the two 0x48-byte records at `entry`, 0x100 bytes at `data`, and 0x124
bytes at the cached-open global (AS 3442 / Neo 34D2). STATE lines retain PC/SP,
D0/A0/A5; PASS/FAIL lines retain assertions and raw D0. Hash manifests accompany
the successful logs. No screen interpretation, screenshots, or model inference
is used as an oracle. Dialog coverage here verifies cancellation/return state,
not display layout.

## 1. Negative returns are real

The issue confused two resolver outputs. The resolver returns a descriptor
pointer in D0, writes a **32-bit status** through its second argument, and may
write a canonical **16-bit token** through its third argument. Failure clears
the pointer/token, but writes a negative status. The public handlers return the
status local on failure, not the null pointer or cleared token.

Resolver entries: AS `004E366E`, Neo 3.01 `005E5FE2`, Neo 3.21 `0043C3E4`.
Their failure stores include `MOVEQ #-7,D0; MOVE.L D0,(A0)` and
`MOVEQ #-1,D0; MOVE.L D0,(A0)`. Follow the caller's final load from its status
local through the diagnostic/consistency helper; inspecting only the resolver's
D0 pointer loses this distinction.

| Call | Live missing-token/current result | Live empty-folder result | Successful result |
|---|---:|---:|---|
| A1A0 FileEraseCurrent | -1 | -7 | zero-extended token |
| A1A4 FileDeleteAtPosition | -1 | -7 | full-width removed count |
| A1A8 FileGetCurrentId | -1 | -7 | zero-extended token |
| A1C8 FileOpen | -1 | -7 | zero-extended token |
| A1D8 FileResetPasswords | -1 | -7 | count; cancel returns 0 |

The A1D8 initial zero store occurs **before** resolution; resolution can overwrite
it. Password-reset authorization success and failure are also exercised live: two
entries reset to the firmware default `write` and return 2 after authorization;
an incorrect password leaves both entries untouched and returns 0. The clipboard
password is preserved. Cancellation and initial lookup failure are tested separately.

The `int32_t` declarations and negative-error documentation must stay. In
particular, the linked A1A0/A1A8/A1C8 `uint16_t` signatures would truncate the
observed error values; A1A4/A1D8 `uint32_t` would obscure their signed meaning.
Successful tokens `0x8001` are returned as `00008001`, not sign-extended.
Failed FileOpen preserves the cached context and position. Only mode byte 1
rewinds: tested 0, 1, 2, 0x101, 0x100, and a nonzero upper file-ID word.

A230 does leave full-width D0=`FFFFFFFF` and stores that context sentinel.
The existing public `void` API deliberately discards it; exposing an integer
return is optional and is not required to repair an ABI defect.

## 2. A1B8 selectors

| Input | Verified behavior |
|---|---|
| ordinary size | unsigned capacity clamp; update logical length, clamp position, synchronize mirrors; raise saved length if necessary |
| -2 | use saved length, then common clamp/update |
| -3 | save current logical length, then common update |
| -4 | if cached-open pointer is zero, copy logical to saved lengths throughout the non-clipboard global table, run allocation maintenance, return 0; otherwise leave lengths unchanged and return resolver status |
| -5 | reserve max(logical length,512), then common update using logical length |
| -6 | request **min(saved length, logical length+40)**, then common capacity clamp/update |
| -1 / unrecognized negative | no dedicated branch; unsigned capacity comparison clamps to capacity |

The issue's **max** for -6 is reversed. At AS `004E195E–004E1972` (Neo 3.01
`005E42BA–005E42CE`, Neo 3.21 `0043A6BC–0043A6D0`), D0 is logical+40;
`CMP.L saved,D0; BLS` goes to the logical+40 assignment. Otherwise saved is
selected. Live `(logical,saved)=(6,100)` gives 46; `(6,20)` gives 20;
`(6,46)` gives 46. Both orderings reject the issue's proposed maximum.

The -4 condition is the **cached pointer**, not whether the resolver can find a
fallback current file. Live tests distinguish cached/open, uncached fallback,
empty-folder uncached (snapshot returns 0), and empty-folder cached (returns -7).
The global loop ends before the clipboard descriptor. -4 does not take the
ordinary resize path when the cached pointer is nonzero.

`FILE_LENGTH_SNAPSHOT_ALL` and `FILE_LENGTH_GROW_TOWARD_SAVED` expose the verified
selectors. No speculative -1 command name was added. Tested unknown negatives
include -1, -7, and INT32_MIN.

## 3. A1B4 conflict: the existing greater-of contract is correct

Both aggregate loops load logical length, compare it unsigned to reserved
length, and select reserved only if logical <= reserved. Thus each term is
**max(logical,reserved)**. The AS comparison blocks start at `004E17FC` and
`004E1840`; equivalent Neo blocks are asserted in the static runner.

A two-entry live fixture uses `(logical,reserved)=(6,20)` and `(30,10)`.
`0xFD` returns 36; both `0xFE` and `0xFF` return 50. A minimum would return 16.
The table/group loop bounds differ as documented; the clipboard is outside this
global non-clipboard table. Ordinary and `0xFC` failures return resolver status,
confirmed live as -1/-7; A1BC does too. Keep signed result declarations.

## 4. Narrow returns

| Call | Native defined width | Poisoned direct-ROM result |
|---|---|---|
| A218 ClipboardGetFormat | byte | A5B6C700 -> A5B6C781 for format 81 |
| A234 AppletGetContextId, selected | word | A5B6C7D8 -> A5B6BEEF |
| A234, unset | word | A5B6C7D8 -> A5B60000 |
| A240 AppletGetName, index 0 rejection | byte | A5B6C7D8 -> A5B6C700 |
| A244 AppletSendMessage, index 0 rejection | byte | A5B6C7D8 -> A5B6C700 |

The issue is correct about these narrow contracts. Current `uint8_t`,
`uint16_t`, and `bool` declarations are appropriate. Compiled C widens them with
`AND.L #255,D0` / `AND.L #65535,D0`. The compiled probe executes real production
stubs and checks false rejection results, selected/unset IDs, and format 81 on
both live ROMs. No bridge is required; a full-register wrapper would be wrong.

## 5. Full 38-row review

All rows below were compared to public source declarations and three-ROM
instruction paths and executed on both live ROMs. “Agrees” is bounded to the
listed paths; it does not certify untested allocation/UI/transport branches.
The existing enum arguments occupy ordinary four-byte stack slots and the ROM
reads their low bytes; this agrees with the repository's `-fshort-enums` build.

| Slot / opcode | Public symbol | Result contract and directed live coverage / discrepancy |
|---|---|---|
| 102 A198 | FileWriteBuffer | signed long; insert content/count, missing/empty resolver errors, invalid mode -5; agrees |
| 103 A19C | FileReadBuffer | signed long; read/count/position, resolver errors, NULL destination -6; agrees |
| 104 A1A0 | FileEraseCurrent | signed long token/error; zero-fill and length reset, -1/-7, high-bit token; issue's zero-failure and word-return claims rejected |
| 105 A1A4 | FileDeleteAtPosition | signed long count/error; shift, zero request, count clamp and unchanged position, -1/-7; issue rejected |
| 106 A1A8 | FileGetCurrentId | signed long token/error; fallback/current, -1/-7, high-bit token; issue rejected |
| 107 A1AC | FileGetFreeSpace | unsigned long metric; original active fixture positive result; 512-byte rounding and saved/reserved maximum traced statically, allocator stress not tested |
| 108 A1B0 | FileGetUnusedCapacity | unsigned long; capacity-logical and resolver-failure zero; no negative contract |
| 109 A1B4 | FileQuerySize | signed long; ordinary/FC, FD/FE/FF and -1/-7; existing maximum is correct, linked unsigned signature loses signed errors |
| 110 A1B8 | FileSetCurrentLength | signed long; all selectors and lookup errors; linked unsigned signature and -6 maximum are wrong |
| 111 A1BC | FileGetCapacity | signed long; capacity and -1/-7; linked unsigned signature loses signed errors |
| 112 A1C0 | FileSetFolder | signed long previous folder/error; existing-folder selection and invalid-folder -64/output guard; -7 branch traced; agrees |
| 114 A1C8 | FileOpen | signed long token/error; modes, failed-context preservation, upper argument bits, -1/-7; issue rejected |
| 115 A1CC | FileClose | void; clears cache, fallback still resolves; agrees |
| 116 A1D0 | FileUpdateFlags | signed long token/error; set/clear and invalid mask -14; agrees |
| 117 A1D4 | FileTestFlags | signed long boolean/error; set/clear and invalid mask -14; agrees |
| 118 A1D8 | FileResetPasswords | signed long count/error; cancel/failed authorization 0, successful two-entry reset 2, initial lookup -1/-7; issue rejected |
| 119 A1DC | FileAccessPassword | signed long token/error; set/read seven-byte NUL-terminated password, >6 characters -8, invalid-ID/output guard; agrees |
| 122 A1E8 | FileCreate | signed long token/error; create and invalid group -10; minimum allocation branch traced; agrees within controlled fixture |
| 123 A1EC | FileDelete | signed long zero/error; created-entry deletion, invalid ID -1; agrees |
| 125 A1F4 | FileAccessTag | signed long zero/error; tag read/write and invalid-ID/output guard; agrees |
| 126 A1F8 | FileFindByTag | signed long token/error; hit and miss -11; agrees |
| 127 A1FC | FileResolveId | signed long token/error; hit and miss -1; agrees |
| 128 A200 | ClipboardCopy | signed long count/error; copy content, oversized source count -128; agrees |
| 129 A204 | ClipboardCut | signed long count/error; cut content, zero format -129; agrees |
| 130 A208 | ClipboardPaste | signed long count/error; raw insert content; plain conversion remains helper-specific, not exhaustively tested |
| 131 A20C | ClipboardSet | signed long count/error; content/format, NULL -6, zero format -129; agrees |
| 132 A210 | ClipboardGet | signed long count/error; raw copy/guard and NULL -6; **version-specific plain-copy limit**, detailed below |
| 133 A214 | ClipboardClear | void; zero length and format observed; agrees |
| 134 A218 | ClipboardGetFormat | byte; format and clear, upper-bit poison, compiled C; issue correct |
| 135 A21C | ClipboardGetLength | unsigned long; raw and cleared length; plain helper path traced; agrees within conversion limits |
| 137 A224 | ClipboardGetCapacity | unsigned long; allocation capacity observed; agrees |
| 138 A228 | ClipboardResize | unsigned long; requested length grant observed; allocator limit/stress not exhausted |
| 140 A230 | AppletClearContext | public void; native signed long -1 confirmed and context cleared; deliberate public omission |
| 141 A234 | AppletGetContextId | word; selected/unset, poisoned upper word, compiled C; issue correct |
| 142 A238 | AppletFindByName | unsigned long index; prefix lookup and miss at first table hole; agrees |
| 143 A23C | AppletFindById | unsigned long index; ID lookup and miss; system-zero ambiguity traced; agrees |
| 144 A240 | AppletGetName | byte bool; name copy, 36-character source bound plus NUL, zero-index rejection, poison and compiled C; 37-byte buffer declaration is correct |
| 145 A244 | AppletSendMessage | byte bool; rejection and callback/message/param/status forwarding, poison and compiled C; issue correct |

### Additional discrepancy: A210 plain mode differs by generation

The linked A210 closure correctly identifies a difference that the prior header
comment generalized too broadly. AS3000/Neo 3.01 pass caller count as source
length and pass an **uninitialized local** as output limit. Neo 3.21 initializes
the local from caller count and passes the clipboard logical length as source
length. The Neo 3.21 handler is four bytes longer. The header now states the
version boundary explicitly.

The live fixture has logical clipboard `XYZ`, retained bytes `pq`, requested
count 5, and a destination guard. With the old local deliberately seeded to 0
and 2, AS3000 returns 0 and 2; Neo 3.21 returns 3 in both cases. Destination
bytes and the first untouched guard are asserted. Seeding disposable stack RAM
characterizes a firmware bug; it is not a supported application workaround.

## Acceptance evidence

The final workspace evidence is under `.research-work/issue15/` outside the
Betawise repository. Static extraction passed all three images. The four live
groups pass **301 cases** (122 file cases per ROM, 28 AS3000 and 29 Neo applet cases), including
a full-capacity erase of deliberately nonzero bytes. Final `verified-file` and
`verified-applet` runs retain logs, raw case records,
compiler artifacts, and hashes. The library build and changed-header Doxygen
check pass. The repository-wide documentation check fails on existing
ConsoleKeySequence/ConsoleReadPending and stdio documentation warnings; rerunning
Doxygen with the original HEAD versions of the changed headers reproduces those
same warnings (`docs-baseline/doxygen.log`).
The row list contains exactly the issue's 38 entries; nearby baseline services
remain exercised but are not counted as extra audited issue entries.
