# Idle power policy and battery measurement

## Scope

Slots 72–78 and 93–99, including the shared dialog/Send-speed integration probe.

Caller contracts remain in [`os3k_power.h`](../../../os3k/os3k_power.h). This page records
**evidence and confidence**, not a duplicate normative API reference. No new
reverse-engineering claims are inferred from the documentation reorganization.

## Findings and open questions

| Finding | Status and evidence |
|---|---|
| Idle sleep versus automatic power-off | Different flags/consumers; state and RTC alarm transitions checked. |
| Temporary/saved timeout | Bounds 4..59; native setting dialog commits both; temporary setter changes one and does not rearm by itself. |
| Chemistry and voltage units | ROM NiMH/Alkaline strings and Control Panel division by 1000 support the names/units. |
| Normalization and curves | Controlled GPIO timing; AS factor 600/shift10, Neo factor 2096/shift16. ROM arithmetic and sample boundaries exercised. |
| Factory calibration | Stable/outlier/stable-zero branches run; flash persistence **not certified**. Zero is accepted, creating an invalid subsequent divisor. |
| Charging diagnostics | Cached words and update mechanics traced; physical charge-termination interpretation remains **partial**. Names must not be treated as a complete charger specification. |
| Battery UI | Live character/bitmap RAM plus timed and native-key dismissal tested. Physical analogue accuracy/charging hardware remain unverified. |

Factory procedure text in the original Control Panel requires AC power and
batteries removed. This is not ordinary battery measurement. The test injects
controlled samples, validates the RAM divisor, and runs the original attempted
flash-write path; it does not certify factory reference accuracy or persistence.

Detailed original trace/caller records: the disassembly record below.

## Tests and reproduction

Maintained mixed-subsystem sources: [C probe](battery-dialog-probe.c),
[Lua template](battery-dialog-probe.lua.in), [builder](make-battery-dialog-probe.py).
Dialog-pointer and Send-speed cases intentionally live in this integration
probe once; [dialog](../dialog/README.md) and [keyboard](../keyboard/README.md)
link here rather than maintaining copies. The separate timeout/native-dialog
checks are in the [localization integration probe](../localization/README.md).

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

Syscalls 93–99 (0xA174–0xA18C) are power-management operations, rather than
localization. The flags, active timeout, and saved user timeout are global.
These declarations cover the traced AS3000 3.01 and Neo 3.21 implementations.

Syscall 0xA178. Only updates the flag; does not itself suspend, power off,
or change the automatic power-off alarm. Vendor Control Panel and applets
temporarily disable it around operations that need continued execution.

Syscall 0xA184. This can differ from the saved user preference after a
temporary change. Fresh initialization uses four minutes in the traced ROMs.

Syscall 0xA188. Changes the active byte only, retaining the user's saved
timeout. Does not itself rearm the RTC alarm. Vendor AlphaQuiz raises short
intervals to 30 minutes for an operation and then calls RestoreAutoPowerOffMinutes().

Selectors 0–10 read cached fields. They do not refresh measurements. The
charging counters are described by their observed state-machine updates;
their physical charge-termination interpretation is not fully established. All results are 32-bit integers,
including zero-extended word fields. No pointer result or output structure
is involved. The calibration selector has materially different side effects.

< Cached raw_sample * normalization_constant / calibration_divisor (600 on AS3000 3.01, 2096 on tested Neo).

< Smallest raw sample tracked during the current charging-monitoring phase.

< Factory A/D calibration: take sixteen measurements, reject samples differing from their mean by more than 20, then replace calibration and attempt to persist it to flash. The vendor Control Panel requires AC power with batteries removed. Returns 1 for unstable samples, otherwise 0; even a stable zero sample is accepted. Zero is not proof of a valid divisor or successful flash persistence.

Syscall 0xA130. AS3000 3.01 constructs a three-bit value from hardware straps;
the tested Neo initializes the value to 0x10. Battery measurement/display
paths treat zero as unsupported and nonzero as supported. This is a numeric
revision code, not a canonical bool or a Neo 2/wireless detector.

Syscall 0xA120. Does nothing when GetHardwareRevision() is zero. Otherwise
temporarily overrides charging-control state, measures the GPIO transition
delay and updates raw/normalized samples. Normalization is raw times 600
(AS3000 3.01) or 2096 (tested Neo), divided by the current calibration divisor. The divisor must be nonzero; the ROM
does not guard division by zero. This does not return the measurement;
use GetBatteryDischargePercent(), GetBatteryMillivolts(), or BatteryDiagnostic().
AS3000 3.01 also samples battery chemistry; Neo's chemistry is detected during
initialization rather than by this refresh. A missing/immediate hardware
transition can produce zero. Actual analogue accuracy is not certified by
emulator register-injection tests.

Syscall 0xA12C. AS3000 3.01 computes 2322281 / sample + 887; the tested Neo uses
7881064 / sample + 997. Integer division truncates. No hardware refresh is
performed and neither chemistry nor calibration validity is checked here.

## Disassembly and caller record

This static evidence is not regenerated by the runtime probes.

```text
Slots 69–78 clean-room evidence — 2026-11-18

Manufacturer ROM hashes/load bases: rom-inventory.json. Entry addresses for
all three ROMs remain in slots.tsv. This follow-up executed AS3000 3.01 and July
Neo 3.21 in MAME; the Neo 3.01 is not separately certified here.

Ghidra working traces:
 /tmp/os3k-clean/slots69-as-ghidra.log
 /tmp/os3k-clean/slots69-neo-ghidra.log
 /tmp/os3k-clean/battery-extra-as.log
 /tmp/os3k-clean/battery-extra-neo.log

69 / A114: DialogGetItemText, AS 004CB358 / Neo 0041B69A.
 Byte one-based index and required byte-length output pointer. No bounds/null
 checking. Retained pointer table AS 3E7E / Neo 3F10, length table AS 3F7E /
 Neo 4010. A0 ends at the table entry; D0 holds its stored text pointer.
 New os3k_abi_bridge.c wrapper privately declares uintptr_t raw result, converts
 it to const char*, and thereby establishes GCC A0. Typed MAME probes recover
 two different caller-owned strings, lengths 3 and 7, including a length unlike
 strlen. Original AlphaWord Plus caller 694A / Neo 7388 uses returned D0 as
 text; Inspiration Outliner BEEC does likewise.

70/71 / A118/A11C: GetSendSpeed / SetSendSpeed.
 AS 004CAE14 / 004CAAD0; Neo 0041B150 / 0041ADF2.
 Byte enum getter/setter; setter full int32 status 0/-1. Active AS 41FF / Neo
 4291; saved setting immediately before it. Valid normal settings 1..4.
 ROM strings: VERY Slow, Slow, Fast (default), Fastest. Neo additionally
 recognizes DE and writes both saved/active bytes to 4. AS rejects DE.
 In text-transfer code AS 004D2582 onward, speed 4/3/2/1 selects delay values
 0/1/3/5. These are keyboard-emulated Send speeds, not UART baud rates.
 Inspiration Outliner 821A temporarily selects 2. Both invalid and valid
 requests, plus the Neo-only saved-setting behavior, were tested in MAME.

72 / A120: RefreshBatteryMeasurements.
 AS 004D1EC4 / Neo 004272DA. Takes no arguments and has no meaningful result.
 Temporarily overrides charging state, then calls hardware timing helper
 AS 004D1E3E / Neo 0042726A. Input transition is AS F431 bit4 or Neo F449 bit5.
 Test read taps supply transition after exactly 700 / 2600 loop failures;
 the unmodified ROM captures those counts. Raw/calibration/normalized fields
 start at AS 43EC/43E8/43F0 or Neo 7DDC/7DD8/7DE0.
 Normalize raw*600/cal on AS, raw*2096/cal on Neo. Explicit cal=600 fixture
 demonstrates normalized 700 on AS and 9082 on Neo (integer truncation).
 Charging state and low-byte charge flag are restored. Revision zero skips
 hardware and preserves cached measurement. AS refresh also reads chemistry;
 Neo chemistry is selected at initialization.

73 / A124: GetBatteryType.
 AS 004D1E36 / Neo 00427262, cached full D0 at AS 43E4 / Neo 7DD4.
 0=NiMH, 1=Alkaline, FFFFFFFF=unknown. ROM AS 004CBA58 tests this getter and
 selects localized string 62 ('    NiMH') for zero or 63 ('Alkaline') for one.
 BatteryType_e intentionally remains 32 bits using its unknown enumerator.

74 / A128: GetBatteryDischargePercent.
 AS 004D1FD4 / Neo 004273F6. Cached-input piecewise-linear lookup, not a fresh
 hardware reading. AS tables 004EA7F0 (NiMH), 004EA760 (alkaline); Neo 0044C6C6
 and 0044C642. Twelve-byte records: threshold, multiplier, signed offset;
 terminal multiplier zero produces 100. Below first threshold produces 0.
 Formula ((sample*multiplier + offset) modulo 2^32) >> 10 (AS) or >>16 (Neo).
 Unknown chemistry returns 0. UI explicitly displays 100 minus this result;
 it is consumed capacity, not remaining capacity. MAME covers both curves,
 interval thresholds/interiors, terminal range, sample zero, and unknown type.

75 / A12C: GetBatteryMillivolts.
 AS 004D2008 / Neo 0042742A. Cached normalized sample zero -> zero; otherwise
 2322281/sample+887 on AS or 7881064/sample+997 on Neo, unsigned integer division.
 Vendor Control Panel 3A1E and 3A86 calls this after refreshing, divides by 1000
 and formats cut-off/charging voltage as decimal volts. This confirms mV units.

76 / A130: GetHardwareRevision.
 AS 004D1E2C / Neo 00427258. Numeric byte, not bool. AS boot constructs bits
 from GPIO straps at 004D1C74 onward (0..7); Neo boot writes 10 hexadecimal
 at 00427080. Zero/nonzero gates measurement and battery UI support.
 Control Panel and AlphaWord use this support test. Not a wireless/model query.

77 / A134: BatteryDiagnostic.
 AS 004D23AC / Neo 00427870. Full 32-bit selector; recognized 0..10, 41, 42;
 all others return 0. It does not take a value/output pointer. Selectors 0..10
 expose charge flag, calibration, raw/normalized samples, and seven word
 fields. Word outputs are zero-extended. Full-width unsupported selectors are
 verified to avoid a mistaken byte-only prototype. Charging word mechanics
 are traced in AS 004D209A/004D214A..004D2284; physical termination policy is
 not fully certified by the tests. Doxygen describes those limits.
 41 validates the small-ROM descriptor then reads calibration at 4003F0.
 Read taps explicitly supply per-ROM descriptor text and 12345678; this proves
 output width/lookup behavior, not original flash contents or persistence.
 42 takes 16 timing samples after PLL-reference-clock delays. If any sample
 differs from their integer mean by more than 20 it returns 1 without replacing
 calibration. Otherwise it updates the RAM divisor, attempts a 4-byte flash
 write, and returns 0. Factory delay loops and original flash-write path run;
 physical flash persistence is not certified. Stable/unstable/zero samples
 tested. Stable zero is accepted and stores zero, a validity gap; later refresh
 division is unguarded. Test restores a nonzero divisor before any later call.
 Original Control Panel 2BE2 reads selector 41 and 2D70 invokes selector 42.
 Its strings at 54F1/5509/5529 explicitly require the AC adapter, batteries
 removed, then the 'c' key. This is a factory A/D procedure, not normal refresh.

78 / A138: DrawBatteryStatus.
 AS 004D1A32 / Neo 00426EC0. Single uint8 duration in seconds, void result.
 Nonzero waits seconds*100 through centisecond sleep; zero prints the capacity
 report and waits for a fresh key. Revision zero skips drawing and only delays.
 Original Control Panel 1CF6 passes zero for the interactive battery status.
 AS animates CGRAM slots 4..7, draws text bytes 91..94 at row1 columns37..40;
 Neo draws a 41x16 bitmap at x222,y0. Live LCD DDRAM/CGRAM and bitmap RAM
 checks confirm writes while the report is active. Both timed and native-Return
 dismissal paths complete. UI tests normalize with the per-ROM reference
 constant so an artificial low-battery warning does not consume the key.
 End clears upper-right area and invalidates text shadow, without saving/restoring
 the old screen. Header does not promise physical battery/voltage accuracy.

#91 follow-up:
 investigate-character-bitmap.py and character-bitmap-results.txt preserve the
 expanded static reference search across 54 unique manufacturer applet/font
 images. Thirty images have A168/A16C/A170-validated exports; no resolved call
 references were identified. Unrecognized dynamic calls are not excluded.
 Full disassembly is a byte -> high-nibble-selected 16-bit word -> low-nibble
 bit -> canonical Boolean operation. There is no hidden locale/state test.
 Its 183 accepted bytes all map to non-FF AS LCD translations, but 41 rejected
 bytes do too. In particular, #78 visibly draws rejected bytes 91..94 and the
 typed post-display #91 calls still return false. Thus this is not a complete
 'can draw this byte' predicate. An ordinary-document-text whitelist is
 plausible, but no consumer was found to establish that intended policy.
 No stronger public name/Unicode interpretation has been adopted.

Reproduce: python3 betawise/research/abi-audit/run.py [temporary-output-dir]
Final clean run: /tmp/os3k-clean/abi-battery-dialog-final (eleven groups passed).
No physical power/charging, physical calibration accuracy, flash persistence,
or Neo 3.01 execution is certified by this follow-up.
```
