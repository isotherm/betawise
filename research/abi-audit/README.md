# OS3K / GCC ABI audit — 2026-11-18

## Current implementation

All thirteen previously missing pointer-return bridges are now implemented
in `os3k/os3k_abi_bridge.c`, alongside the five existing bridges (including Neo
FontDialogCreate). Raw stubs remain in `syscall.c` under private `_OS3K_` names.
`tests/qemu/abi-bridges.py` exercises all eighteen production bridges using the
repository ABI flags and `-fno-builtin`, checking argument forwarding and both
NULL and non-NULL D0-only mock results. Run `make -C tests/qemu check`.
These mock tests complement the original firmware evidence below; they do not
extend its ROM coverage. Markup aggregate normalization also resides in
`os3k/os3k_abi_bridge.c`; slot 270 retains its void public contract.

The table below describes current production remediation. Original ROM findings
and their coverage limits remain unchanged.

## Result and scope

**All eighteen exposed D0-only pointer-return services now have D0-to-A0 bridges.** Two additional pointer services, FindStringChar and strrchr, return a correct A0 without a bridge. The initial seventeen-service ROM results agree across the AS3000 3.01, Neo 3.01, and Neo 3.21 firmware examined. DialogGetItemText and AppletGetResources were subsequently verified on AS3000 3.01/Neo 3.21; FontDialogCreate was verified on Neo 3.21. Production wrapper checks cover all eighteen adapters, independently of that ROM coverage.

A separate confirmed compatibility problem affects ROM formatted I/O: **`ll` is not supported by the tested integer conversion paths**. A 64-bit variadic argument can leave the following conversion reading the wrong word. Six newer Neo services return a **six-byte aggregate through hidden A1 storage**. Their input convention matches this GCC configuration, but their secondary A0 value is storage+4, unlike GCC-generated aggregate callees.

The original audit examined Betawise HEAD `0923f2dfe2d1a6e879be57229329dd7f16fa440a` plus uncommitted header/wrapper changes. Subsequent implementation consolidated the pointer bridges and six markup normalization wrappers in `os3k/os3k_abi_bridge.c`. The original raw-ROM tests remain useful for reproducing the native conventions; they are not tests of every production adapter.

The compiler is Debian `m68k-linux-gnu-gcc 12.2.0-13`, with the repository's `-m68000 -msoft-float -msep-data -fomit-frame-pointer -fshort-enums -fno-store-merging -Os` flags. Findings must not be generalized to every m68k GCC target or different compiler options. Generated compiler assembly, ROM machine-code paths, and execution of the original firmware in MAME were compared. No firmware service was replaced by a host implementation.

**Coverage boundary:** the initial 17 typed pointer-returning services were traced in all three ROMs; the later DialogGetItemText and AppletGetResources additions are bounded to the AS3000 3.01/Neo 3.21 follow-up. Entry code for every populated Neo 3.21 dispatch slot was screened for additional pointer and hidden-result conventions. This found three further pointer services and six aggregate services. This is not a complete reconstruction of every undocumented service's prototype. There are 264 raw stubs, 238 with raw-stub public-header declarations. Eighteen pointer-return services and six markup services instead expose public wrappers with private raw declarations; the remaining private entries include ClearScreen and SetCursor. These counts describe the current source and `slots.tsv`. Undeclared symbols cannot be certified for arbitrary C calls. The audit therefore does **not** assert that all undocumented services or every path in every firmware version are ABI-safe.

## Pointer remediation table

The ROM's authoritative pointer is D0 in every row. “A0 correct” means every return path in that routine supplies the intended pointer in A0 too; merely happening to match on one branch does not qualify.

| Opcode / slot | Public function | A0 contract in ROM | Current remediation |
|---|---|---|---|
| A070 / 28 | FindStringChar | Correct result, including NULL | None |
| A074 / 29 | FindByteInRange | End-of-range argument, not result | Bridged in os3k_abi_bridge.c |
| A078 / 30 | FindBufferPattern | Scratch value / call target; not a result contract | Bridged in os3k_abi_bridge.c |
| A114 / 69 | DialogGetItemText | Address of the pointer-table entry, not its stored text pointer | Bridged in os3k_abi_bridge.c; AS3000 3.01/Neo 3.21 follow-up |
| A278 / 158 | AppletGetResources | Scratch descriptor/call target; result supplied in D0 | Bridged in os3k_abi_bridge.c; AS3000 3.01/Neo 3.21 typed follow-up |
| A1C4 / 113 | FileBindState | Scratch/output-variable address | Bridged in os3k_abi_bridge.c |
| A1F0 / 124 | FileAccessName | Scratch address or advanced copy-source address | Bridged in os3k_abi_bridge.c |
| A220 / 136 | ClipboardGetData | File record address, while D0 is record->data | Bridged in os3k_abi_bridge.c |
| A358 / 214 | memchr | End of searched range, also on misses | Bridged in os3k_abi_bridge.c |
| A360 / 216 | memcpy | Advanced source pointer | Bridged in os3k_abi_bridge.c |
| A364 / 217 | memmove | Advanced destination on forward copy; original destination on backward copy | Bridged in os3k_abi_bridge.c |
| A368 / 218 | memset | Destination+length; correct only for zero length | Bridged in os3k_abi_bridge.c |
| A380 / 224 | strcat | Advanced source pointer | Bridged in os3k_abi_bridge.c |
| A384 / 225 | strchr | No result placed in A0 | Bridged in os3k_abi_bridge.c |
| A38C / 227 | strcpy | Advanced source pointer | Bridged in os3k_abi_bridge.c |
| A394 / 229 | strncat | Advanced source pointer | Bridged in os3k_abi_bridge.c |
| A39C / 231 | strncpy | Source/scratch pointer, not destination contract | Bridged in os3k_abi_bridge.c |
| A3A0 / 232 | strrchr | Correct last match or NULL | None |
| A3A4 / 233 | strstr | Scratch pointer; NULL/empty-substring paths also lack the A0 contract | Bridged in os3k_abi_bridge.c |
| A41C / 263 | FontDialogCreate | D0-only constructed object pointer | Bridged in os3k_abi_bridge.c; Neo 3.21 follow-up |

There are **20 tracked pointer services = 18 bridged + 2 directly compatible**, including Neo FontDialogCreate. Ignoring a libc function's result can conceal the defect. GCC can also replace a builtin call, inline a small copy, or infer a destination result, hiding the error in one build and exposing it in another. Audit tests use `-fno-builtin` and real calls.

The file-pointer probes use deliberately constructed RAM file records and the real ROM resolver/copy code. They establish ABI behavior, not a successful end-to-end file installation or persistence test. FileBindState also writes full 32-bit pointers through `void **`; those output stores do not need a register bridge. A caller that only uses those mirrors may never see the broken function result.

### Additional Neo pointer-related services

| Opcode / slot | Observed behavior in Neo 3.21 | Required treatment |
|---|---|---|
| A41C / 263 | Returns a constructed UI object pointer, or zero, in D0 | Exposed as FontDialogCreate with a production pointer bridge; Neo 3.21 coverage |
| A438 / 270 | Forwards strcpy's D0 destination result after copying a ROM-owned string into caller storage | Exposed as void WirelessCopySelectedNetworkId; no pointer result promised or bridge needed |

The original A41C finding is now supported by typed FontDialogCreate probes; see [font-dialog research](../syscalls/dialog/README.md). A438's public contract is the void copy operation WirelessCopySelectedNetworkId. Its incidental D0 address does not make it a public pointer-return service.

### Correct bridging mechanism

Follow the existing FindByteInRange/FindBufferPattern design:

```c
extern uintptr_t _OS3K_memchr(const void *, int, size_t); /* raw A358 */
void *memchr(const void *p, int c, size_t n) {
    return (void *)_OS3K_memchr(p, c, n);
}
```

Rename the raw stub and let a normal C wrapper call it. Its integer declaration makes GCC read D0, and its public pointer return makes GCC populate A0. This also handles NULL correctly. Use the full original parameter list and widths for each wrapper.

**Do not append `move.l D0,A0; rts` immediately after an A-line word and expect it to execute.** The handler removes the exception frame and jumps to the service; the service's RTS uses the caller's existing return address. It does not resume at the next word in the stub. A separate called raw stub is essential. Likewise, simply inserting a JSR/BSR introduces another return address and shifts arguments; an assembly bridge must explicitly forward the argument frame. The included bridge probe does so for three arguments; it is test code, not a generic implementation.

Do not globally copy D0 to A0 in the dispatcher. Unknown void/aggregate services, existing machine-code callers, and ROM-internal conventions need separate treatment.

## Parameter, result, and register audit

| Area | Compiler evidence and firmware comparison | Finding |
|---|---|---|
| Data model | Pointer/int/long/size_t are 32 bits; big endian; char/bool 1 byte, short 2 bytes | Matches the typed ROM API |
| Ordinary pointer arguments | Entry SP+4, SP+8, etc.; pointers occupy one 32-bit stack slot | Compatible; returning a pointer in A0 does **not** imply pointer parameters arrive in address registers |
| Narrow scalar and enum arguments | GCC reserves four bytes each and places a byte/word in the low end (SP+7/SP+6 for first argument). ROM reads those offsets after adjusting for its saved registers/locals | Compatible with current declarations and flags |
| Scalar return widths | Full int/long in D0; byte/word declarations consume D0.b/D0.w. GCC explicitly masks when widening unsigned byte/word results | Do not treat undefined upper D0 bits as part of a narrow result; no additional mismatch established in current typed API |
| Boolean results | GCC consumes the low byte and assumes canonical C bool values | Current bool declarations require 0/1 results, not arbitrary nonzero statuses. No new counterexample established; unknown status-returning calls must not be guessed as bool |
| Pointer output parameters | ROM stores a 32-bit address into caller storage; e.g. FileBindState mirrors and CallSysInt font/scratch pointer selectors | Compatible; independent of pointer return registers |
| Stack cleanup | Caller cleans ordinary argument slots. A-line handler removes only its six-byte exception frame | Compatible. Direct raw probes return with SP advanced exactly four bytes by RTS |
| Preserved registers | GCC expects D2–D7/A2–A6 preserved; D0/D1/A0/A1 and condition codes are volatile | All 35 tested pointer cases preserved D2–D7, A2–A6, and SP on both tested machines. This is not an all-syscalls/all-callback-path proof |
| Applet globals | `-msep-data` uses A5. Current startup and wrappers preserve it | Ordinary tested calls preserved A5. Timer callback caveat below remains material |
| Floating point | `-msoft-float`: float in D0, double in D0:D1; no FP0 dependency | Compiler convention measured; no typed scalar floating-return syscall currently declared |

The handler was independently inspected: the Neo 3.21 path at `0x426768` reads the faulting A-line opcode, masks with `0x7FC`, indexes the table at `0x44F15E`, drops six stack bytes, and jumps via A0. Thus A0 initially contains the service entry address, not an argument/result. A syscall that never writes A0 can return that code address to a GCC pointer caller. Its supervisor-mode behavior is part of the existing OS3K execution model, not a general user-mode Unix trap ABI.

### Serial follow-up: slots 79–87

**No syscall-entry ABI wrapper is required for the nine serial prototypes.**
The AS3000 3.01 and Neo 3.21 implementations were traced in Ghidra and called
from compiled C using the repository ABI flags. None returns a pointer or an
aggregate. This follow-up does not separately certify the Neo 3.01
implementations; their dispatch addresses remain recorded in `slots.tsv`.

| Opcode / slot | Public function | ABI treatment |
|---|---|---|
| A13C / 79 | SerialReset | uint32_t zero in D0; direct stub |
| A140 / 80 | SerialConfigure | Two four-byte argument slots, including SerialConfig_e; int32_t in D0; direct stub |
| A144 / 81 | SerialSetBaudRate | uint32_t input; int32_t in D0, including negative errors; direct stub |
| A148 / 82 | SerialGetStatus | Two pointer arguments, one byte written through each; void return; direct stub |
| A14C / 83 | SerialSetReceiveCallback | Ordinary function pointer argument; void return; direct stub, with callback caveat below |
| A150 / 84 | SerialDisableReceiver | No arguments; void return; direct stub |
| A154 / 85 | SerialEnableReceiver | No arguments; void return; direct stub |
| A158 / 86 | SerialRead | Six four-byte argument slots; byte terminator and word timeout right-aligned; uint32_t count through pointer; int32_t in D0; direct stub |
| A15C / 87 | SerialWrite | Seven four-byte argument slots; word timeout and byte event enum right-aligned; uint32_t count through pointer; int32_t in D0; direct stub, with callback caveat below |

With `-fshort-enums`, SerialConfig_e is four bytes; SerialReadFlags_e,
SerialWriteFlags_e, SerialRxStatus_e, SerialTxStatus_e, and SerialTxEvent_e are
one byte. The probes contain size assertions. Scalar arguments still occupy
four-byte slots. There is no memory-facing enum output in these serial
prototypes: SerialGetStatus explicitly takes uint8_t pointers. Buffer pointers,
callback pointers, and count-output pointers do not need the D0-to-A0 adaptation
required for a function that **returns** a pointer.

**Conditional callback trampoline requirement:** SerialReceiveCallback_t receives
a status byte and a data byte in two ordinary four-byte stack slots;
SerialTransmitCallback_t receives one status byte in one slot. These fetches
agree with GCC. However, both ROM handler paths call the saved function pointer
without acquiring the applet's A5/GOT base. A callback that accesses applet
globals or makes GOT-dependent calls requires the owning applet's A5. It may
already be correct when that applet was interrupted. Unless correctness can be
guaranteed for every possible callback context, use an entry trampoline that
saves the interrupted A5, establishes the owning applet's A5, calls the C handler
with the same arguments, and restores A5 before returning. A normal C wrapper around
SerialSetReceiveCallback or SerialWrite alone does not fix the later callback.
Handlers deliberately independent of A5 can be registered directly. No generic
trampoline has been added by this investigation.

The executable probe calls all nine production-named raw stubs from C, verifies
binary counts, a nonzero 0xD3 terminator, signed -502/-515 results, nonzero
16-bit timeout arguments, status output bytes, and caller stack recovery. It
then invokes the original ROM UART handler with controlled register samples,
checks typed callback arguments and the extra final not-ready receive sample,
and verifies the transmit callback is one-shot. Before handler entry, the test
changes A5 from the applet GOT to 0x12345678. Both callbacks record **0x12345678**,
confirming that no applet-base switch occurred. Its stores use absolute probe
addresses so the test itself remains valid with that foreign A5.

A further disassembly check covers the outer interrupt path, not just the UART
handler. Startup writes vector RAM[0x110] = 0x4CD28E on AS3000 3.01 and 0x420310 on
Neo 3.21. These complete level-four dispatchers do not write A5. For UART pending
bit 2 they load the installed handler from 0x422E / 0x48A6 and call it at
0x4CD2DE / 0x420360. The common UART handler likewise does not write A5 before
its RX/TX callback calls. Registration stores the callback pointer without
capturing its applet A5. [serial-interrupt-paths.txt](../syscalls/serial/README.md) preserves the complete
Ghidra instruction listings and vector/registration provenance. Therefore the
normal path inherits the interrupted context's A5; there is no outer ROM
callback-owner switch hidden from the original direct-handler test.

The reproducible probe also enters that live RAM vector with a synthetic
68000 exception frame and controlled UART pending status. Both callbacks retain
the foreign A5 through the dispatcher, and RTE restores the test frame correctly.
This exercises the complete vector/dispatch/handler/callback/return path, but is
not a test of hardware-triggered asynchronous delivery/preemption or a physical
serial link. AS3000 3.01 UART emulation
is a transport stub. On the tested Neo, these legacy entries mix UART1 and
UART2; the probe supplies separate controlled register samples where needed.
That routing difference is a firmware semantic/compatibility issue, not a GCC
ABI mismatch, and a return-register wrapper would not repair it.

### Localization/power follow-up: slots 88–99

No new ABI wrapper is required for these twelve entries. Ghidra entry/exit
inspection and compiled-C execution on AS3000 3.01 and Neo 3.21 establish:

| Slots | ABI contract |
|---|---|
| 88 / ToLowerTextByte | Byte argument in a four-byte slot; result in D0.b |
| 89 / _OS3K_TranslateKeyToChar | KeyMod_e word in a four-byte slot; char result in D0.b |
| 90 / TranslateMatrixKey | Byte argument; one-byte Key_e result with production -fshort-enums |
| 91 / IsLocalizationTextByte | Byte argument; canonical 0/1 in D0.b |
| 92 / ConfigureKeyboardLayout | bool and two pointers in ordinary four-byte slots; full uint32_t result 1; one-byte enum and character-buffer outputs |
| 93, 95 / IsIdleSleepEnabled, IsAutoPowerOffEnabled | Canonical flag state consumed as bool from D0.b |
| 94, 96 / SetIdleSleepEnabled, SetAutoPowerOffEnabled | bool in low byte of a four-byte slot; void result |
| 97 / GetAutoPowerOffMinutes | uint8_t in D0.b |
| 98 / SetAutoPowerOffMinutes | uint8_t argument; full signed int32_t result 0/-1 |
| 99 / RestoreAutoPowerOffMinutes | No arguments; void result |

None returns a pointer, aggregate, or 64-bit scalar. The pointer outputs in slot
92 need no register bridge. Its KeyboardLayout_e object **must be one byte**;
without -fshort-enums the ROM store and the C enum object disagree. The probe
asserts the enum size and preserves sentinels immediately on either side of
that output. A char result from slot 89 needs a uint8_t cast before unsigned
widening on signed-char builds; this is documented in os3k_keyboard.h and does
not require an assembly bridge. Boolean getters are certified for the canonical
states written at boot/by their typed bool setters, not arbitrary RAM corruption.

[localization-probe.c](../syscalls/localization/localization-probe.c), [localization-probe.lua.in](../syscalls/localization/localization-probe.lua.in), and
[make-localization-probe.py](../syscalls/localization/make-localization-probe.py) are integrated into `run.py`. They use the current
raw syscall stubs, test all 256 lowercase/classification inputs and 256 matrix
inputs under each of four arrangements, check key modifiers and accent state,
and exercise all twelve entries from C. Alternative-layout results for matrix
holes are characterized as out-of-map ROM reads, not certified key values.
Native matrix key presses exercise all four layout selections and Escape on
both machines; the native power-setting dialog verifies the lifecycle of saved
versus temporary timeouts. RTC checks establish 60 seconds per minute and that
timeout-only changes/same-value enable do not rearm the alarm. Separate Neo
context cases exercise accent bypass for no context and Calculator ID 0xA002.

The new evidence does not change the thirteen outstanding pointer bridges.
Slot 91's bitmap is exact, but its broader classification policy remains
unresolved. No callback ABI is introduced here, and this follow-up does not
separately certify the Neo 3.01 implementations. API semantics and Doxygen live
in os3k_localization.h, os3k_power.h, and the existing os3k_keyboard.h rather
than being duplicated here. Manufacturer caller leads and trace provenance are
recorded in `localization-power-evidence.txt`.

### Dialog, Send speed and battery follow-up: slots 69–78

Slot 69 is now `DialogGetItemText(uint8_t, uint8_t*)` with a **production bridge**.
The raw `_OS3K_DialogGetItemText` is privately declared as returning uintptr_t.
Its D0 points to the caller-owned item string, while A0 points to the ROM's
pointer-table entry. The C wrapper returns the integer as const char*, making
GCC populate A0. Typed MAME calls verify two distinct pointer results and their
stored display lengths (which need not equal strlen). Index and output-pointer
validation remain the caller's responsibility; this bridge only corrects ABI.

Slots 70/71 use a byte SendSpeed_e and four-byte stack slot, with full signed
int32_t setter status. Neo's special request DE sets saved/active speed to 4;
AS3000 3.01 rejects it. Slots 72 and 78 are void. Slots 73/77 deliberately use enums
whose UINT32_MAX member fixes their width at four bytes: cached battery type is
a full D0 result and the diagnostic dispatch compares a full 32-bit selector.
Slots 74/75 return uint32_t; slot 76 returns the numeric hardware revision byte,
not bool (Neo returns 10 hexadecimal). No other bridge is needed.

[battery-dialog-probe.c](../syscalls/power/battery-dialog-probe.c), [battery-dialog-probe.lua.in](../syscalls/power/battery-dialog-probe.lua.in), and
[make-battery-dialog-probe.py](../syscalls/power/make-battery-dialog-probe.py) are integrated into the runner. Original ROM
measurement loops receive controlled GPIO transitions, including calibration
normalization (AS3000 3.01 600; Neo 3.21 2096), while the original curve arithmetic,
factory delays, calibration checks and UI execute. The curve helper shifts by
10 on AS3000 3.01 and 16 on Neo; tests cover both chemistries, unknown type, interval
boundaries, zero samples and exhaustion. Read-only diagnostic selectors and
unsupported full-width selectors are checked. Small-ROM descriptor/calibration
reads are explicit read-tap fixtures, not evidence of factory flash contents.

Factory calibration takes sixteen samples: stable samples update the RAM
divisor, an outlier is rejected, and **sixteen stable zero samples are accepted**.
The latter exposes a validity gap: success status alone does not establish a
usable nonzero divisor. Calibration uses the ROM flash-write path, but successful
physical/persistent flash programming is not certified. The vendor Control Panel
explicitly instructs AC power with batteries removed. Subsequent measurement
normalization requires a nonzero divisor and does not guard that division.

Live LCD RAM checks confirm the AS3000 3.01 four-CGRAM-character battery graphic and
Neo bitmap. Both timed and key-dismissed reports return in MAME. The tests use
valid per-ROM normalization before UI entry so an unintended low-battery warning
does not consume the dismissal key. Factory/analogue accuracy on real hardware
and Neo 3.01 execution remain outside this follow-up's certification.

A separate #91 investigation is reproducible with
`python3 betawise/research/syscalls/localization/investigate-character-bitmap.py`.
`vendor-image-inventory.json` records 54 deduplicated manufacturer images and
hashes. Thirty export a validated A16C stub, but the expanded static search finds
no resolved call/address references. The AS3000 3.01 LCD translation comparison shows
183 accepted bytes all have non-FF translations, while 41 rejected bytes also
have non-FF translations. In particular, #78 renders the rejected battery glyph
bytes 91–94. This rules out treating #91 as a complete display-capability test;
an ordinary-text whitelist remains a plausible **inference**, not a recovered
vendor contract. Exact output behavior and this limit remain in Doxygen.

The [filed glyph comparison](../syscalls/localization/charset-comparison.png) shows selected accepted
and rejected glyphs side by side; the corresponding observations are recorded
in the "#91 font-coverage correlation" section of [results.txt](results.txt).
Green rows are accepted by #91; red rows are rejected on both ROMs.

The subsequent NeoFontTerminal correlation confirms identical #91 bitmaps in
AS3000 3.01 and Neo 3.21. AS3000 3.01's translated character-controller CGROM/CGRAM path
and Neo's software font bitmaps are different rendering mechanisms. Of 73
rejected bytes, 72 have nonblank default Neo font bitmaps (00 is the generic
filled-block glyph, and A0 is blank); the custom NeoFontTerminal bitmap has the
same nonblank/fallback counts. Both fonts include glyphs for DD/DE/FD/FE, which
map to FF fallback in the AS3000 3.01 text-to-LCD table. D0 and D7 are rejected even
though the AS CGROM asset also has corresponding glyphs. A shared legacy text
repertoire/compatibility predicate remains possible, but actual current-font
coverage is demonstrably not what the routine computes. NeoFontTerminal is the
user's 2020 Alpaxo custom font, not a manufacturer ROM font; the default Neo
font was independently checked at descriptor 44AF8E, bitmap 44A38E. The AS CGROM
comparison uses MAME's KS0066 F05 asset, marked NEEDS REDUMP, and does not certify
the glyph shapes on a physical device.

### 64-bit values

The actual compiler probe returns `0x1122334455667788ULL` as D0=`0x11223344`, D1=`0x55667788`. A 64-bit input occupies eight consecutive stack bytes, most significant longword first, with **two-byte alignment** under these flags. The mixed argument probe places a pointer at entry SP+4, byte slot at +8, word slot at +12, 64-bit value at +16/+20, and final pointer at +24. Do not use a prototype with one 32-bit parameter to call an eight-byte argument service: every following parameter would move.

**There are no explicit int64_t/uint64_t/long-long parameters or returns in the current typed syscall declarations.** Consequently there is no currently exposed fixed-signature 64-bit syscall to approve or bridge. This absence is not evidence that the undocumented table has no such functions. In particular, ROM internal floating helpers observed returning eight-byte objects via hidden storage must not be exposed as GCC scalar `double` functions without a separate analysis/bridge.

Varargs are the real current 64-bit entry point. With original ROM execution on **both AS3000 3.01 and Neo 3.21**:

| Probe | Observed result | Interpretation |
|---|---|---|
| sprintf `%d %ld %x`, three 0x12345678 arguments | `305419896 305419896 12345678` | Ordinary int/long width and stack traversal agree |
| sprintf `%.1f`, IEEE double 1.5 | `1.5` | Promoted double's high/low stack word order agrees |
| sscanf `%f`, input `1.5` | Writes `3FC00000`, next word unchanged | Correct 32-bit float destination |
| sscanf `%lf`, input `1.5` | Writes `3FF80000 00000000`, following word unchanged | Correct 64-bit double destination |
| sprintf `%lld %x`, 0x1122334455667788ULL then 0x55 | `ld 11223344` | Unsupported `ll`; `%x` reads the first word of the unconsumed 64-bit value |
| sscanf `%lld`, input `1234567890123` | Zero assignments; destination unchanged | No long-long scan support in this path |

**Remediation:** document the limited formatter/scanner dialect and provide software conversion/wrappers if long-long formatting/scanning is needed. The exposed printf/fprintf/sprintf and scanf/fscanf/sscanf families must not promise unrestricted modern C formatting. Execution probes directly cover sprintf/sscanf; related stream entries' complete paths were not separately executed in this audit. A pointer-register fix does not solve this problem. `long double` is 12 bytes in this toolchain and was not validated against the ROM; do not infer `%Lf` support from the successful double test.

### Structures and unions

Most declared syscall structures are passed by pointer. The exception is MarkupResult_t, returned by value through hidden A1 storage and normalized by the public wrappers. Compiled static assertions verified these pointer-passed layouts:

| Type | Size / important offsets |
|---|---|
| BufferSearch_t | 18; pointer fields 4/12; case_sensitive 16 |
| FileInfo_t | 0x48; name 0x34; file_id 0x44 |
| AppletHeader_t | 0x94; entryPoint 0x84 |
| FontHeader_t | 16; bitmap_data 12 |
| TextPosition_t | 2 |
| FirmwareVersion_t | 4 |

No extra indirection/register adaptation is needed for these objects. Pointer fields remain 32-bit values in memory. Their natural longword alignment is two bytes, so compiling with incompatible packing/alignment options can still break them.

The new Neo slots **A43C/A440/A444/A448/A44C/A450 (271–276)** save an incoming hidden **A1** pointer and copy exactly six bytes into it. The data has a status byte at offset 0 and a longword at offset 2; byte 1 is padding, not an independently established semantic field. All six epilogues copy one longword with postincrement and then one word. Consequently they finish with A0=output+4.

GCC's aggregate return rules are size/mode dependent, not simply “structures use a hidden pointer.” The probe shows a four-byte integer-field struct in D0, an eight-byte two-integer struct in D0:D1, and six-/ten-/twelve-byte examples through a hidden A1 buffer. A six-byte `struct { uint8_t status; uint32_t count; }` has the observed size/offsets and **does use A1**, even though it is smaller than eight bytes.

A compiled C call through a function pointer to **A444** successfully received status 12/count 0 on its empty-input path. A second raw call confirmed a six-byte write, untouched boundary sentinel, and A0=output+4. Thus **the hidden input register is compatible**, but GCC-generated aggregate callees return the original output address in A0. The tested GCC caller reads its own stack buffer and does not depend on that secondary A0 value. This is a latent convention difference, not a demonstrated failure of that caller. The public markup APIs now use C wrappers that normalize the aggregate return, padding, and successful offsets; see [markup research](../syscalls/markup/README.md). Do not declare them as six-byte scalar integers, ordinary pointers, D0/D1 structs, or functions with an extra ordinary first stack argument. That initial probe executed only A444; the subsequent maintained markup runner executes all six typed public calls and a filter callback. Its directed coverage is documented in the markup research.

Unions and other aggregate modes are not present in the typed syscall interface; no ROM union-return contract has been established. Adding a new by-value aggregate API requires a fresh compiler/ROM comparison for its exact type.

### Enums, callbacks, and build options

`-fshort-enums` is an ABI requirement for memory-facing enums: measured CursorMode_e=1, KeyMod_e=2, Message_e=4. Scalar enum arguments still occupy four-byte argument slots. In particular, GetCursorMode writes **one byte through CursorMode_e***. Without the short-enum option, a four-byte enum output object can retain garbage or acquire the written byte at the wrong significance on this big-endian target. All modules sharing these interfaces must use consistent options.

The TimerEvent_e argument is a byte enum passed in a normal four-byte stack slot, which agrees with GCC's callback argument fetch. However **TimerCallback_t does not automatically acquire the applet A5 data base**. The current header already documents this. A callback that accesses applet globals or calls A5/GOT-dependent functions needs a valid A5 arrangement or trampoline; a trivial no-globals callback does not establish general safety. Asynchronous context, callback lifetime, and cancellation remain separate requirements. This audit did not exercise timer interrupt callbacks.

Applet message entry uses the ordinary stack argument frame and supplies A5; the current crt0 establishes initialized data/GOT and tail-dispatches through BwProcessMessage. A wrapper that preserves the stack and A5 is consistent with that model. A function pointer written to firmware must point to executable code with the appropriate callback contract, not a pointer-return bridge chosen just because both are addresses.

Do not mix `-mshort`, register-parameter conventions, `-mrtd`, hard-float objects, different structure-return flags, or four-byte structure alignment into this library without an explicit ABI boundary. GCC's compiler-generated calls to memcpy/memset resolve to the same public bridges as explicit application calls.

## Evidence and reproduction

### Current bridge validation — 2026-11-18

After bridge consolidation, the combined runner passed the eighteen-adapter
QEMU checks and all eleven AS3000 3.01/Neo 3.21 MAME groups. Evidence directory:
`/tmp/betawise-abi-research-refresh`. The localization probe now links the
production bridge module for its compiler-generated memcpy call and explicitly
retains every Lua-invoked entry when garbage-collecting unused sections.
The separate markup runner also passed all forty directed cases, including
six typed production wrappers and a callback, with evidence under
`/tmp/betawise-markup-research-refresh`.

```sh
python3 betawise/research/abi-audit/run.py /tmp/betawise-abi-research-refresh
python3 betawise/research/syscalls/markup/run-probe.py /tmp/betawise-markup-research-refresh
```

These checks validate the current source arrangement without extending the
firmware, hardware, callback-lifetime, or branch coverage claimed below.

### Firmware inventory and original execution records

`rom-inventory.json` records source paths, SHA-256, load bases, table offsets, and every table target. These are update packages interpreted at their known execution bases, not claimed full flash dumps. The populated counts are AS3000 3.01=219, Neo 3.01=219, Neo 3.21=264. The old tables end at A3B0; the Neo 3.21 table ends at A470, with holes A458–A464. The Neo 3.21-only entries must not be probed on old firmware: its smaller dispatch mask can alias the opcode to another service.

`slots.tsv` lists all 285 positions through A470, including holes, available raw symbols, declarations, pointer remediation, and per-ROM addresses. A “declared” row means shared ABI rules reviewed, not that its full semantic prototype was independently reconstructed here. Unprototyped rows remain explicitly uncertified.

Run from the workspace with the existing local compilers, MAME binaries, and private ROM files:

```sh
python3 betawise/research/abi-audit/run.py
```

The runner first executes `tests/qemu/abi-bridges.py` against all eighteen production pointer adapters, then verifies the three recorded main-ROM hashes, compiles probes with production ABI flags plus `-fno-builtin`, and creates a fresh temporary directory for emulator NVRAM/config/logs. It reads private firmware without modifying it. It executes **35 pointer cases on each of AS3000 3.01 and Neo 3.21**, six variadic characterization cases on each, and the compiled-C/aggregate Neo probe. It requires success markers and rejects Lua errors. Successful final run: `/tmp/abi-audit/final-validation` (five original runner groups passed). Captured execution observations are saved in `results.txt`. Generated compiler assembly is `compiler.s` in the temporary evidence directory. The Neo 3.01 comparison is **static only**.

The runner now also builds [serial-probe.c](../syscalls/serial/serial-probe.c) and fills the two
`serial-*-probe.lua.in` templates through [make-serial-probe.py](../syscalls/serial/make-serial-probe.py), using the current
`syscall.c` rather than a possibly stale archive. It runs the serial follow-up
on AS3000 3.01 and Neo 3.21 and requires its final success marker with no Lua errors.
The follow-up full run at `/tmp/os3k-clean/abi-serial-vector` passed all seven
runner groups, including the original five. Serial observations are appended to
`results.txt`; compiler artifacts and expanded Lua remain in the temporary
output directory. No ROM images are embedded in the added probe sources.

The runner also builds and runs the localization/power probes on both systems,
bringing the full suite to nine groups. Successful final localization/power run:
`/tmp/os3k-clean/abi-localization-final2`. Execution markers are appended to
`results.txt`; generated machine code, expanded Lua and MAME state remain in
the disposable output directory.

The dialog/battery probes add two further runner groups (eleven total).
Final successful run: `/tmp/os3k-clean/abi-battery-dialog-final`. New markers are
appended to `results.txt`; [battery/dialog evidence](../syscalls/power/README.md) records ROM addresses,
manufacturer caller evidence and verification limits. These probes also execute
the new production DialogGetItemText bridge, unlike the older illustrative
memchr bridge probe.

The compiled-C test deliberately demonstrates that unadapted memchr returns the wrong A0 end pointer, that a separate D0 bridge returns the correct pointer, and that the six-byte hidden-A1 call works. A PASS in this illustrative probe means the native defect/convention was reproduced. The separate QEMU tests exercise the production pointer bridges; the markup runner exercises the production aggregate wrappers.

Initial exploratory runs revealed that MAME can continue after a Lua assertion and print later success text. The final harness stops on its first assertion and the runner rejects any Lua error. Only the final clean runs are counted above.

No hardware execution, all-branches callback validation, full undocumented prototype reconstruction, or certification of other ROM releases is claimed. The pointer bridges and markup normalization are implemented. Restricted variadic-format contracts, callback A5 ownership/lifetime, and coverage of other firmware releases remain limitations; retain these constraints when extending the API. The nine serial entry stubs need no additional ABI bridge.

## Subsystem evidence organization

Subsystem probes and historical notes are indexed in [syscalls/](../syscalls/README.md).
The integration runner remains here and resolves moved sources relative to its own path.

## Slots 146–173 follow-up

[IrDA/services research](../syscalls/irda/README.md) records per-slot return widths,
callback arguments, public structures, and bounded tests. AppletGetResources
was the nineteenth tracked pointer service and fourth production bridge at that follow-up. It now shares os3k_abi_bridge.c with the other seventeen pointer bridges.

## Slots 175–194 follow-up

[Text/editor/form research](../syscalls/text/README.md) documents 94/44/124/74-byte
structures, eleven-argument statistics with an eight-byte double output, and
full-width transport messages. No additional pointer-return bridge is needed:
D0 addresses inferred by Ghidra are counts/statuses here, not pointer contracts.
Bounded MAME and compiled-C coverage is listed per entry; transport peers and
complete control/expression dialects remain outside that coverage.

## Slots 282–284 follow-up

Neo 3.21 speech enqueue/control and the cached wireless receive word were
verified with fresh Ghidra traces and typed MAME calls. These scalar-return
stubs require no D0-to-A0 bridge. Speech callbacks receive inherited A5;
callback-owner context is not installed. See [speech research](../syscalls/speech/README.md)
and [wireless status research](../syscalls/wireless/README.md) for exact scope.

## Slots 237–277 follow-up

Forty-one original Neo 3.21 targets were freshly inspected in Ghidra. Slots
237–246 and 270 are wireless packet/association/transfer services, 247–269
and 277 belong to font-aware dialogs and inline bitmaps, and 271–276 form a
restricted markup parser. The new dialog constructor is an additional D0
pointer-return service and has a production D0-to-A0 bridge. The thirteen other pointer bridges were implemented subsequently; all eighteen now reside in os3k_abi_bridge.c.

All six markup APIs now have C normalization wrappers: native hidden A1
storage is compatible with GCC's input convention, while the public C return
normalizes A0, padding, and undefined successful offsets. Raw private stubs
remain `_OS3K_` prefixed. The prior recommendation to avoid direct public
aggregate stubs is therefore followed. Six compiled calls and a filter
callback execute in the maintained markup runner. Wireless scalar entries
need no additional return-register adapter; association and transfer callbacks
inherit A5, confirmed using deliberately foreign A5.

[Wireless research](../syscalls/wireless/README.md), [font dialogs](../syscalls/dialog/README.md),
and [markup](../syscalls/markup/README.md) record per-slot evidence and scope.
The repaired Neo low-RAM/MMIO mapping now passes its directed native-addressing
regression. Live suites pass 126 font-dialog cases, 50 wireless cases and 40
markup cases, including the compiled adapters. The existing 80-case speech
suite and all eleven common ABI groups also pass on the resulting builds.
These counts establish directed coverage, not physical-radio certification.
