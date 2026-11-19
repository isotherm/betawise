# IrDA and slots 146–173 integration

## Scope and reproduction

The main new interface is [`os3k_irda.h`](../../../os3k/os3k_irda.h), slots
162–172. This fixture also tests adjacent file, applet, dialog, and exception
services once, rather than duplicating fixtures across subsystem directories.
Their API contracts remain in the corresponding public headers.

From the workspace root:

```sh
python3 betawise/research/syscalls/irda/run-probe.py /tmp/os3k-services
python3 betawise/research/syscalls/irda/find-vendor-calls.py
```

[run-probe.py](run-probe.py) checks the shared ROM hashes, compiles the public
C [typed probe](typed-probe.c), links the production resource-pointer bridge,
and fills the [Lua fixture](probe.lua.in). All binaries, seeded NVRAM, and logs
are disposable outputs under the supplied directory. This does not modify the
original firmware files or normal emulator state. Existing MAME executables and
the original images in the [inventory](../../abi-audit/rom-inventory.json) are required.

## Repeating Ghidra inspection

[InspectServices.java](InspectServices.java) reads dispatch targets and prints each
entry's instructions and decompilation. Use a separate Ghidra project with the
inventoried big-endian 68000 ROM mapped at its inventory base. For an existing
project/program, the command pattern is:

```sh
/opt/ghidra/support/analyzeHeadless /tmp/your-projects project-name \
  -process program-name -noanalysis -readOnly \
  -scriptPath betawise/research/syscalls/irda \
  -postScript InspectServices.java as3000
```

Use neo_3_21 or neo_3_01 for the other inventoried programs. Instruction and
decompiler output goes to stdout; do not retain generated project/output files
in this directory. Ghidra's guessed types are starting points for instruction
review, not authoritative prototypes. The original follow-up used the same
instruction/decompiler procedure plus targeted internal-function/caller tracing.

## Evidence and limits

Entry instructions and decompilation were inspected afresh in Ghidra for every
slot on AS3000 3.01 and Neo 3.21. Stack loads and stores determine parameter
widths; incidental upper D0 bits do not determine return types. Raw MAME calls
seed D0/A0, exercise RAM transitions, and check stack restoration. Compiled C
calls check argument layout, one-byte enum returns, structures, output pointers,
and the production D0-to-A0 bridge. Neo 3.01 targets are listed for
reference; this fixture does **not** execute that ROM.

The final suite passed on 2026-11-18: **105 AS3000 3.01 cases and 99 Neo cases**,
including the compiled-C case with its individual assertions. The suite also
reads native localized strings to identify UI/exception purposes.

| Path | Coverage |
|---|---|
| Password flag/check/user name | Byte preservation, case-insensitive exact comparison, mismatch/prefix/empty inputs, profile slots/current slot, disabled mode, optional name output; typed C checks. |
| Master-password dialog | Native physical matrix keys spell secret then Enter; native Escape cancellation. Wrong-password retry loop traced but not separately automated. |
| System dispatcher | No-mask/unknown-mask returns; latched special-key success; Ghidra traces for other handlers. Does not certify all external-link workflows. |
| Applet masks | System bit, full-word query/clear, context bits 0/1/7/31, target bounds/no-op paths; typed C checks. |
| Resources | Context resolution/NULL, read/write/find, odd padding, zero-length reader behavior, search failure/optional outputs, compiled pointer bridge. |
| Printer dialog | Physical Escape and Down/Enter; selection changes from 0 to 1. Does not send a print job or exercise every printer-applet fallback. |
| IrDA configure/max | Full argument slots, word/byte outputs, retained callbacks, copied names. Overlong-name overflow is established by disassembly, not deliberately triggered. |
| Discovery/connect/open/disconnect/close success | AS3000 3.01 wrapper isolation only: lower transport routines temporarily become MOVEQ #0,D0; RTS in emulator memory; controlled state/peer completions injected. Original bytes restored. These are not native peer negotiations. |
| Neo channel tests | Native error/rejection paths for all entries, native outbound selector/data/control/credit packing, pending-transmit state, query completion with a controlled peer response. No writable NOR instruction substitution is claimed. |
| IAS query/parser | Invalid prerequisites, controlled completion/status, and original native parser extracting 0x78 from integer 0x12345678 on both ROMs. |
| Receive/status callbacks | Original printer receiver forwards three bytes through compiled C callback; original channel reset delivers status 35. Callbacks deliberately work without GOT-dependent globals; no A5 owner-switch claim. |
| Begin/end | Native reset/setup/teardown, one-byte saved idle flag and sentinels; AS hardware-present and absent read taps; absent returns -1302 without touching output. |
| Exception getter | Distinct full-width output values, typed C, and handler frame/marker disassembly. Does not deliberately crash the machine. |

**Physical infrared transport, peer interoperability, asynchronous preemption,
callback cancellation on focus loss, and persistence are not certified.**

## Firmware mechanics

ROM strings `Device`, `DeviceName`, `IrDA:IrCOMM`, `IrLPT`,
`IrDA:IrLMP:LsapSel`, and `IrDA:TinyTP:LsapSel` identify the stack and service
records. Native initialization gives five eight-byte channel records: state at
+0, remote selector +1, local selector +2, callback pointer +4. This internal
array is documented here, not exposed as a writable public struct. Local
selectors are 0, 0x51, 0x52, 0x53, and the printer-service default 0x54.

The link state byte uses idle 1 and connected/secondary states 3/2. Public
channel operations require 2 or 3. Open uses a control packet beginning 1,0
(and 5 for channels 3/4), then waits for channel state 1. Close sends 2,1 and
clears its channel state. The 5/10-tick deadlines compare wrapping byte counters;
these are firmware protocol counters, not a public millisecond duration.

Data packets prepend remote/local selectors; a nonzero control argument sets
remote bit 7 and bypasses the open-channel check. Channels 3/4 add transport
credit framing and consume one credit for nonempty data; channel 3 adds a zero
control length byte. The negotiated word less six limits the application length.
Pending transmission can return raw status 20 even after packet construction.
Only the low return byte is meaningful. Status 19 is distinct from the public
channel-state rejection 21; unknown status codes must remain reportable.

The configured receive callback is reached from printer channel 4 after its
credit byte is removed. It receives `(pointer, byte count)` in 32-bit stack
slots; only the count's low byte is loaded by a GCC byte parameter. The status
callback receives one stack-slot status byte (35 on table reset, 36 on received
printer-channel close). Caller return values are ignored. The inspected dispatch
paths contain no callback-owner A5 load. That does not establish whether the
inherited A5 will be correct in every asynchronous/focus scenario.

IAS query slot 170 retains a **one-byte** result pointer. The native parser
assembles a 32-bit IAS integer but stores with MOVE.B. A query waits on shared
transaction state, not on modification of the caller's query buffer. Unsupported
result formats set the output byte to FF; the response status is separate.
The broad IAS protocol/request construction is not exposed as a new struct.

Slot 163 resets its shared service record before validation. It caps the first
name at 20 bytes, then erroneously checks that first name's counter while
copying the second. A short first name therefore leaves the second copy
unbounded. Callback installation also precedes final validation. The public
contract restricts both strings to fewer than 20 bytes; there is no new wrapper
silently changing firmware behavior.

## Dispatch targets

Addresses are absolute within the inventoried ROMs. Ghidra's inferred C types
were cross-checked against instructions rather than accepted automatically.

| Slot / opcode | Name | AS3000 3.01 | Neo 3.01 | Neo 3.21 |
|---|---|---|---|---|
| 146 / A248 | FileGetPasswordProtection | 004DFF5A | 005E28B6 | 00438CB4 |
| 147 / A24C | FileCheckMasterPassword | 004E04F6 | 005E2E52 | 00439250 |
| 148 / A250 | FilePromptMasterPassword | 004E051C | 005E2E78 | 00439276 |
| 149 / A254 | FileGetUserName | 004DFEEA | 005E2846 | 00438C44 |
| 150 / A258 | FileSetPasswordProtection | 004DFF62 | 005E28BE | 00438CBC |
| 151 / A25C | SystemHandleEvents | 004C87B8 | 005C801E | 00418794 |
| 152 / A260 | AppletGetDisableMask | 004DF664 | 005E1FC4 | 004383C2 |
| 153 / A264 | AppletDisable | 004DF676 | 005E1FD6 | 004383D4 |
| 154 / A268 | AppletClearDisableMask | 004DF68C | 005E1FEC | 004383EA |
| 155 / A26C | AppletAddContextLock | 004DF69E | 005E1FFE | 004383FC |
| 156 / A270 | AppletRemoveContextLock | 004DF6CA | 005E202A | 00438428 |
| 157 / A274 | DialogSelectPrinter | 004C5B58 | 005C5BCC | 00416040 |
| 158 / A278 | AppletGetResources | 004D43C4 | 005D65C2 | 00427A22 |
| 159 / A27C | AppletReadResource | 004D44AA | 005D66A8 | 00427B08 |
| 160 / A280 | AppletWriteResource | 004D44F4 | 005D66F2 | 00427B52 |
| 161 / A284 | AppletFindResource | 004D443A | 005D6638 | 00427A98 |
| 162 / A288 | IrdaGetMaxPayload | 004C4F50 | 005C4F72 | 004153E6 |
| 163 / A28C | IrdaConfigureService | 004C4F5C | 005C4F7E | 004153F2 |
| 164 / A290 | IrdaDiscover | 004C5026 | 005C5048 | 004154BC |
| 165 / A294 | IrdaConnect | 004C510A | 005C512C | 004155A0 |
| 166 / A298 | IrdaOpenChannel | 004C5146 | 005C5168 | 004155DC |
| 167 / A29C | IrdaDisconnect | 004C526C | 005C528E | 00415702 |
| 168 / A2A0 | IrdaCloseChannel | 004C52AC | 005C52CE | 00415742 |
| 169 / A2A4 | IrdaSend | 004C5374 | 005C5396 | 0041580A |
| 170 / A2A8 | IrdaQueryByte | 004C5780 | 005C57A2 | 00415C16 |
| 171 / A2AC | IrdaBegin | 004E3F1C | 005E6890 | 0043CCBA |
| 172 / A2B0 | IrdaEnd | 004E3F8C | 005E68BC | 0043CCE6 |
| 173 / A2B4 | GetExceptionInfo | 004D0ECE | 005D4EAC | 00425F22 |

## Manufacturer caller search

[find-vendor-calls.py](find-vendor-calls.py) searches the inventoried original
images for validated contiguous syscall exports and direct/adjacent
immediate-delta PC-relative call/address patterns. Candidates still need
instruction-level review; this is not a dynamic call graph or proof of absence.
The initial loose scan followed registers too far and produced stale-register
false positives; the maintained scanner restricts immediate-load/use adjacency.

Control Panel's confirmed A248 call at file offset EA8 chooses between its
string-table entries 3/4: “Turn file passwords ON...” / “Turn file passwords
OFF...”. This identifies the otherwise unconsumed ROM byte getter/setter.
The table is at file offset 44C4; entries 3/4 point to 532D/5347. The default
master-password text is a separate ROM-owned string; live comparisons deliberately
use a controlled secret rather than treating a guessed default as evidence.
