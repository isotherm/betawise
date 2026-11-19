# Keyboard input, character conversion, and Send speed

## Scope

Keyboard event services through A0D0, A164 conversion, and slots 70/71.

Caller contracts remain in [`os3k_keyboard.h`](../../../os3k/os3k_keyboard.h). This page records
**evidence and confidence**, not a duplicate normative API reference. No new
reverse-engineering claims are inferred from the documentation reorganization.

## Findings and open questions

| Finding | Status and evidence |
|---|---|
| Narrow modifiers/key events and output widths | ROM loads/stores, historical keyboard probes, and shared ABI compiler checks. |
| A0B4–A0C8 repeat/sticky/slow settings | Earlier Ghidra and controlled state probes. Slow-key delay was derived as an 80 ms multiplier; end-to-end latency also depends on polling. |
| A164 accent composition | Maintained localization probe checks modifiers, repeated accents, composition/fallback byte, and selected-app context behavior. |
| Neo Calculator exception | Guard tests context absent or header ID A002; vendor Calculator supplies that ID. Do not apply the exception to every applet or firmware release. |
| Send speed 1–4, Neo special DE | Maintained power integration probe checks active/saved state and full-width 0/-1 setter status. Original strings label VERY Slow/Slow/Fast/Fastest. |

A164's fallback requeues an event; checking only its immediate character is not
an exhaustive queue-lifecycle proof. Input settings and UART baud/host Send
speed are distinct controls. Native key pulses in the maintained tests are
posted after dialogs drain the queue; early natural-keyboard assumptions did
not work on every Neo emulator build.

## Tests and reproduction

The [legacy keyboard group](../coverage/README.md) checks queue order, empty
reads, readiness, modifier replacement, held-key sampling, accessibility
settings, fresh-key waits, text entry exit, and list-dialog selection. The
localization and Send-speed suites supply the remaining conversion/settings checks.

Maintained character tests: [localization/](../localization/README.md).
Maintained Send-speed checks: [power/](../power/README.md).

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

Conversion is stateful: Alt+E, Alt+Grave, Alt+I, Alt+U and Alt+N start acute,
grave, circumflex, diaeresis and tilde composition. A first accent returns 0;
a supported next letter returns its composed byte. A second accent emits the
previous accent. An unsupported next key emits the accent and requeues that
key for later processing. This shared state can affect subsequent callers.
The tested Neo skips composition when no applet context is selected or the
selected header ID is 0xA002 (the vendor Calculator); its ordinary key-table conversion still runs.

Tested Neo only: set both saved and active values to Fastest. Getter returns 4,
not 0xDE. AS3000 3.01 rejects this request.

Syscall 0xA11C. Values 1 through 4 change the active setting only. The tested
Neo also accepts SEND_SPEED_FASTEST_AND_DEFAULT, replacing both active and
saved user settings with 4. This is a RAM setting operation, not a claim of
flash persistence. Invalid values leave both settings unchanged.

## Slot 151: native event dispatch

A25C is now SystemHandleEvents(), with SystemEventFlags_e used by its prototype.
The keyboard wrapper uses the named special-key flag. It still returns full
32-bit 0, 8, or -9; byte-return rules for nearby services do not apply here.
The special-key latched-event fast path and disabled/unknown masks are tested.
Other bits enter native cable/send, infrared transfer, and pending external-link
workflows; complete peer-driven behavior is not covered by those fast-path tests.
See [the integration fixture](../irda/README.md).
